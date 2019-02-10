/*****************************************************************
	SMART CARD TP IMPLEMENTATION FILE

	Authors : BOULET Olivier / LARA Jérémy
*****************************************************************/

/*****************************************************************
	INSTRUCTIONS

	Utilisation des fonctions de la DLL Askcsc. Documentation dans
	askcsc.h.

	- Etape 1 - Détection du lecteur CSC (Contacless Smart Cards) :
		CSC_SearchCSC();
		Code de retour attendu : RCSC_Ok
	- Etape 2 - Réinitialiser le lecteur CSC :
		CSC_ResetCSC();
		Code de retour attendu : RCSC_Ok
	- Etape 3 - Si l'étape 2 a bien fonctionnée, on veut vérifier
	la version du lecteur CSC :
		CSC_VersionCSC();
		Code de retour attendu : RCSC_Ok
		Affichage de la version

	| Remarque : même sans poser de carte sur le lecteur CSC, les
	| étapes 1 à 3 fonctionneront puisqu'elles n'impliquent pas
	| encore de lecture ou d'écriture sur des fichiers.

	- Etape 4 - Mise à jour de la configuration du buffer pour les
	commandes d'envoi/réception de données :
		CSC_EHP_PARAMs_EXT(1, 1, 0, 0, 0, 0, 0, 0, 0);
		Code de retour attendu : RCSC_Ok
	- Etape 5 - Passage en mode recherche du lecteur CSC :
		CSC_SearchCardExt(...);
		Code de retour attendu : RCSC_Ok

		L'appel à cette fonction est nécessaire au moins une fois
		pour faire passer l'appareil en mode recherche. Il faut
		préciser à l'aide des paramètres quels types de cartes on
		recherche.
	- Etape 6 - Sélection de l'application (D2760000850101) par
	envoi de commande :
		# CLA:00 INS:A4 P1:04 P2:00 Lc:07 Data:D2760000850101 Le:00
		CSC_ISOCommand(...);
		Code de retour attendu : RCSC_Ok

		Permet de sélectionner l'application NDEF Tag (nécessaire
		pour pouvoir lire / écrire sur des cartes de ce type là ?).
	- Etape 7 - Sélection du fichier CC (Capability Container) :
		# CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E103 Le:-
		CSC_ISOCommand(...);
		Code de retour attendu : RCSC_Ok
	- Etape 8 - Lecture des données binaires sur le fichier CC
	(15 octets) :
		# CLA:00 INS:B0 P1: 00 P2:00 Lc:- Data:- Le:0F
		CSC_ISOCommand(...);
		Code de retour attendu : RCSC_Ok

		Une fois les données lues, il faut les parser et en extraire
		les informations nécessaires pour l'étape suivante.

		Il faut récupérer le MaxLe (Maximum lecture), MaxLc
		(Maximum écriture), Maximum NDEF File size (Maximum à lire),
		NDEF ID (Identifiant pour effectuer la sélection).
	- Etape 9 - Sélection du fichier NDEF :
		# CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:File ID Le:-
		CSC_ISOCommand(...);
		Code de retour attendu : RCSC_Ok

		Le File ID est celui récupéré dans l'étape 8.
	- Etape 10 - Lecture des données binaires du fichier NDEF :
		# CLA:00 INS:B0 P1: Offset P2:Offset Lc:- Data:- Le:Length
		CSC_ISOCommand(...);
		Code de retour attendu : RCSC_Ok

		L'Offset sera déterminer sur deux octets et évoluera au
		cours de la lecture (via une boucle). La Length correspond
		au MaxLe qui a été extrait dans l'étape 8.
	- Etape 11 - décodage du fichier NDEF :
		0022
			D102/1D ---> Taille
				5370 ---> Smart Poster
					9101/11/8801-70 617261676F6E2D726669642E62F6D ---> 91 -> Short record. 11 -> 17 octets pour la ressource. Utiliser les tables pour le type de l'URI  0x70... URL encodée
					5101/04/5400-504944 --> UTF 8 dont on ne connait pas la longueur

	Enter hunt phase parameters --> autoselect = 0
	=== > Désactivation de l'envoie SELECT APPLI automatique lors de la phase anticollision
	Mettre après version CSC autoselect à 0

	Création d'une instanciation sur carte RFID

	=> 3 Records à créer

	Entête :
	MB 1
	ME 1
	CF 0
	SR 0
	IL
	TNF 00
	==> 0x91
	Type Length = 0x01
	Payload Length =
	ID Length = A calculer
	Type = 0x55
	ID = Ué
	Payload = 0x01 "www.apple.com"

	Entête 2 :
	MB 0
	ME 0
	CF 0
	SR 1
	IL 0
	TNF 00
	==> 0x
	Type Length = 0x01
	Payload Length = 0x12
	ID Length
	Type = 0x54
	ID
	Payload = UTF-8_2 ... => 0x02

	Entête 3 :
	MB 0
	ME 1
	CF 0
	SR 1
	IL 0
	TNF 001 => Si on met unknown =, c'est mal interprété par la suite
	==> 0x
	Type Length = 0x00 ==> Type devient unknown
	Payload Length = 0x08
	ID Length -
	Type -
	ID -
	Payload = UTF-8_2 ... => 0x02

	Write en fonction de MLc pour savoir le découpage que l'on va faire

*****************************************************************/

/*****************************************************************
	ACRONYMS & DEFINITIONS

	APDU   - Application Protocol Data Unit
	C-APDU - Command APDU
	CC     - Capability Container
	Lc     - Length command
	Le     - Length expected
	MLc    - Maximum data Length C-APDU
	MLe    - Maximum data Length R-APDU
	NDEF   - NFC Data Exchange Format
	R-APDU - Response APDU
*****************************************************************/

/*****************************************************************
	INCLUDE
*****************************************************************/

// Standard C Library
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// WinAPI
#include <windows.h>

// MS-DOS
#include <conio.h>

// Askcsc DLL
#include "../askcsc.h"

// Custom
#include "select.h"

/*****************************************************************
	DEFINE & GLOBAL VARIABLES, STRUCTURES, ENUM...
*****************************************************************/

#define FAILURE 0
#define SUCCESS 1

#define DO_NOT_FREE 0
#define FREE        1

#define YES "Yes"
#define NO  "No"

static FILE *trace;
static int step = 1;

enum TNF {
	Empty, NFC_Forum_Type, Media_Type, Absolute_URI,
	NFC_Forum_External_Type, TNF_Unknown, Unchanged, Reserved
};

/*
 * mb: 1-bit Message Begin, useful when interpreted considering the ME flag's value.
 * me            : 1-bit Message End, useful in case of chunked payloads.
 * cf            : 1-bit Chunk Flag, set when this is the first chunk of a chunked NDEF message.
 * sr            : 1-bit Short Record, set when the record is short (8-bits payload length)
 * il            : 1-bit ID Lenght, set if the 8-bits ID field is present in the record.
 * tnf           : 3-bits Type Name Format, indicates the structure of the value of the type
 *				   field. TNF enum associates names to values.
 * type_length   : 8-bits unsigned integer specifying the length in octet of the type field.
 * payload_length: if SR, 8-bits, 32-bits otherwise.
 * id_length     : 8-bits unsigned integer specifying the length in octet of the id field.
 * type          : identifier describing the type of the payload, format depends on TNF flag.
 * id            : uniqueness message identifier.
 * payload       : actual data of length payload_length.
 *
 * sp            : Smart Poster (SP). Type equals 0x5370 (Sp)
 * sp_records    : If this record is a smart poster, then sp_records contains the subrecord(s).
 * nb_sp_records : The number of subrecords of a smart poster record. (1 <= nb_sp_records <= 4).
*/
typedef struct record {
	char mb;
	char me;
	char cf;
	char sr;
	char il;
	char tnf;
	unsigned char type_length;
	unsigned int payload_length;
	unsigned char id_length;
	byte* type;
	byte* id;
	byte* payload;

	char sp;
	struct record* sp_records;
	char nb_sp_records;

	char corrupted;
} Record;

static DWORD result;
static DWORD length;
static byte io_data[256];

static int MLe;
static int MLc;
static byte NDEF_File[2];
static int buffer_length;

static Record* records;
static int record_length;

/*****************************************************************
	FUNCTION DECLARATIONS
*****************************************************************/

static void initialize(void);
static char search_csc(DWORD* result);
static char reset_csc(DWORD* result, byte io_data[]);
static char version_csc(DWORD* result, byte io_data[]);
static char configure_buffer(DWORD* result);
static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data
);
static char nfc_forum_type_4_command(const char* command_type, const char *command_info,
	DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, ...);
static char nfc_forum_type_4_select(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...);
static char nfc_forum_type_4_read_binary(const char* info, DWORD* result, DWORD* length, byte io_data[],
	byte cla, byte ins, byte p1, byte p2, byte le);
static char nfc_forum_type_4_update_binary_hard(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...);
static char nfc_forum_type_4_update_binary(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, byte* data);
static Record* parse_ndef_file(byte* data, int* total_records);
static Record* parse_smart_poster(byte* data, int data_length, char* total_records);
static Record parse_record(byte* data, int* byte_index, int NLEN);
static void read(void);
static void write(byte* data, unsigned int data_length);

/*****************************************************************
	FUNCTION IMPLEMENTATIONS
*****************************************************************/

static char mess(char* text, char to_free, DWORD result)
{
	printf("\nSmart Cards Program Stopped:\n\t%s\n", text);

	fprintf(trace, "\nSmart Cards Program Stopped:\n\t%s", text);
	fprintf(trace, " ---- %02X\n", result);

	if (to_free)
	{
		free(text);
	}

	if (result == 0)
	{
		return FAILURE;
	}

	printf("Function return value: ");

	switch (result)
	{
	case RCSC_Ok:
		printf("RCSC_Ok");
		break;
	case RCSC_NoAnswer:
		printf("RCSC_NoAnswer");
		break;
	case RCSC_CheckSum:
		printf("RCSC_CheckSum");
		break;
	case RCSC_Fail:
		printf("RCSC_Fail");
		break;
	case RCSC_Timeout:
		printf("RCSC_Timeout");
		break;
	case RCSC_Overflow:
		printf("RCSC_Overflow");
		break;
	case RCSC_OpenCOMError:
		printf("RCSC_OpenCOMError");
		break;
	case RCSC_DataWrong:
		printf("RCSC_DataWrong");
		break;
	case RCSC_CardNotFound:
		printf("RCSC_CardNotFound");
		break;
	case RCSC_ErrorSAM:
		printf("RCSC_ErrorSAM");
		break;
	case RCSC_CSCNotFound:
		printf("RCSC_CSCNotFound");
		break;
	case RCSC_BadATR:
		printf("RCSC_BadATR");
		break;
	case RCSC_TXError:
		printf("RCSC_TXError");
		break;
	case RCSC_UnknownClassCommand:
		printf("RCSC_UnknowClassCommand");
		break;
	default:
		printf("Unknown Error = %04X", result);
		break;
	}

	printf("\n");
	CSC_AntennaOFF();
	CSC_Close();
	return FAILURE;
}

static void display_TNF(char tnf)
{
	switch (tnf)
	{
	case Empty:
		printf("Empty(%d)", Empty);
		break;
	case NFC_Forum_Type:
		printf("NFC Forum Type(%d)", NFC_Forum_Type);
		break;
	case Media_Type:
		printf("Media Type(%d)", Media_Type);
		break;
	case Absolute_URI:
		printf("Absolute URI(%d)", Absolute_URI);
		break;
	case NFC_Forum_External_Type:
		printf("NFC Forum External Type(%d)", NFC_Forum_External_Type);
		break;
	case TNF_Unknown:
		printf("Unknown(%d)", TNF_Unknown);
		break;
	case Unchanged:
		printf("Unchanged(%d)", Unchanged);
		break;
	case Reserved:
		printf("Reserved(%d)", Reserved);
		break;
	default:
		return;
	}
}

static void display_data_string(byte* data, DWORD length)
{
	printf("\t[Data STR=");

	for (unsigned int i = 0; i < length; ++i)
	{
		printf("%c", data[i]);
	}

	printf(", Length=%d]\n", length);
}

static void display_data_hex(byte* data, DWORD length)
{
	printf("\t[Data HEX=0x");

	for (unsigned int i = 0; i < length; ++i)
	{
		printf("%02X", data[i]);
	}

	printf(", Length=%d]\n", length);
}

static void display_records(Record* records, DWORD length, int level)
{
	char* indent = (char*)malloc(sizeof(char) * level + 1);
	for (int i = 0; i < level; *indent++ = '\t', ++i);
	*indent = '\0';
	indent -= level;
	printf("%s[Number of records: %d]\n", indent, length);

	for (unsigned int i = 0; i < length; ++i, ++records)
	{
		if (records->corrupted)
		{
			printf("%s[Record Corrupted]\n", indent);
			continue;
		}
		printf("%s[Record:\n\t%s--- Flags ---\n\t%s- MB: %s\n\t%s- ME: %s\n\t%s- CF: %s\n\t%s- SR: %s\n\t%s- IL: %s\n\t%s- TNF: ",
			indent, indent, indent,
			records->mb ? YES : NO, indent,
			records->me ? YES : NO, indent,
			records->cf ? YES : NO, indent,
			records->sr ? YES : NO, indent,
			records->il ? YES : NO, indent);
		display_TNF(records->tnf);
		printf("\n\t%s- Type Length: %d\n\t%s- Payload Length: %d",
			indent, records->type_length, indent, records->payload_length);

		if (records->il)
		{
			printf("\n\t%s- ID Length: %d\n\t%s- ID: 0x", indent, records->id_length, indent);

			for (unsigned int j = 0; j < records->id_length; ++j)
			{
				printf("%02X", records->id[j]);
			}
		}

		if (records->type_length)
		{
			printf("\n\t%s- Type: 0x", indent);

			for (unsigned int j = 0; j < records->type_length; ++j)
			{
				printf("%02X", records->type[j]);
			}

			printf(" ");

			for (unsigned int j = 0; j < records->type_length; ++j)
			{
				printf("%c", records->type[j]);
			}
		}

		if (records->payload_length)
		{
			printf("\n\t%s- Payload: 0x", indent);

			for (unsigned int j = 0; j < records->payload_length; ++j)
			{
				printf("%02X", records->payload[j]);
			}

			printf(" ");

			for (unsigned int j = 0; j < records->payload_length; ++j)
			{
				printf("%c", records->payload[j]);
			}
		}

		printf("\n");

		if (records->sp)
		{
			display_records(records->sp_records, records->nb_sp_records, level + 1);
		}

		printf("%s]\n", indent);
	}

	free(indent);
}

static void free_records(Record* records, int length)
{
	if (!records)
	{
		return;
	}

	for (int i = 0; i < length; records++, ++i)
	{
		free(records->type);
		free(records->id);
		free(records->payload);

		if (records->sp)
		{
			free_records(records->sp_records, records->nb_sp_records);
		}
	}

	records -= length;
	free(records);
}

static unsigned int len(byte* str)
{
	unsigned int len = 0;
	for (; *str != '\0'; str++, ++len);
	return len;
}

static void copy_string(char* source, char* destination, unsigned int length, unsigned int source_offset)
{
	source += source_offset;

	for (; length > 0; *destination++ = *source++, --length);
}

static long hex_to_dec(byte* hex, int from, int to)
{
	long base = 1; // 16^0 = 1;
	long dec = 0;
	int length = (int)strlen(hex);
	int start = from > -1 ? from : 0;
	int end = to > -1 && to < (int)length ? to : (int)length - 1;

	if (length > 2 && hex[start] == '0' && hex[start + 1] == 'x')
	{
		start += 2;
	}

	for (; end >= start; --end)
	{
		if (hex[end] >= '0' && hex[end] <= '9')
		{
			dec += (hex[end] - '0') * base;
		}
		else if (hex[end] >= 'a' && hex[end] <= 'f')
		{
			dec += (hex[end] - 'a' + 10) * base;
		}
		else if (hex[end] >= 'A' && hex[end] <= 'F')
		{
			dec += (hex[end] - 'A' + 10) * base;
		}

		base *= 16; // power replacement;
	}

	return dec;
}

static byte* str_to_hex(byte* data)
{
	unsigned int data_length = len(data);
	byte* hex_data = (byte*)malloc(sizeof(byte) * (data_length / 2));

	if (!hex_data)
	{
		printf("Couldn't allocate memory to convert string to hexadecimal...\n");
		return NULL;
	}

	for (unsigned int i = 0; i < data_length; hex_data++, i += 2)
	{
		*hex_data = (byte)hex_to_dec(data, i, i + 1);
	}

	free(data);
	hex_data -= (data_length / 2);
	return hex_data;
}

static flush()
{
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

void initialize(void)
{
	DWORD search_mask;

	BYTE COM;

	sCARD_SearchExt  search_struct;
	sCARD_SearchExt* search = &search_struct;

	// ISO 14443 Type A and B
	search->CONT = 0;
	search->ISOB = 1;
	search->ISOA = 1;
	search->TICK = 0;
	search->INNO = 0;
	search->MIFARE = 0;
	search->MV4k = 0;
	search->MV5k = 0;
	search_mask = SEARCH_MASK_ISOA | SEARCH_MASK_ISOB;

	// Search & Reset CSC, Configure buffer & Search Card
	if (search_csc(&result) && reset_csc(&result, io_data) &&
		configure_buffer(&result) &&
		search_card(&result, search, search_mask, 0x01, 100, &COM, &length, io_data))
	{
		// Select Application
		// Length:13; CLA:00 INS:A4 P1:04 P2:00 Lc:07 Data:D2760000850101 Le:00
		if (!nfc_forum_type_4_select("Application", &result, &length, io_data, 13,
			0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00) ||
			(io_data[1] != 0x90 && io_data[2] != 0x00) ||
			(io_data[1] != 0x62 && io_data[2] != 0x00 && io_data[3] != 0x90 && io_data[4] != 0x00))
		{
			return;
		}

		// Select & Read CC File
		// Length:7; CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E103 Le:-
		if (nfc_forum_type_4_select("CC", &result, &length, io_data, 7,
			0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03))
		{
			// Length:5; CLA:00 INS:B0 P1:00 P2:00 Lc:- Data:- Le:0F
			if (nfc_forum_type_4_read_binary("CC", &result, &length, io_data, 0x00, 0xB0, 0x00, 0x00, 0x0F))
			{
				MLe = (io_data[4] << 8) + io_data[5];
				MLc = (io_data[6] << 8) + io_data[7];
				NDEF_File[0] = io_data[10];
				NDEF_File[1] = io_data[11];
				buffer_length = (io_data[12] << 8) + io_data[13];

				printf("Read CC Info Details:\n");
				printf("\t[MLe=0x%02X%02X (%d), MLc=0x%02X%02X (%d), NDEF Max File Size=0x%02X%02X (%d), NDEF File ID=0x%02X%02X]\n",
					io_data[4], io_data[5], MLe, io_data[6], io_data[7], MLc,
					io_data[12], io_data[13], buffer_length,
					NDEF_File[0], NDEF_File[1]);
			}
		}
	}
	else
	{
		return;
	}
}

static char search_csc(DWORD* result)
{
	*result = CSC_SearchCSC();

	if (*result == RCSC_Ok)
	{
		printf("%d. Search CSC successful...\n", step++);
		return SUCCESS;
	}
	else
	{
		return mess("Search CSC failure!", DO_NOT_FREE, *result);
	}
}

/*
 * Remark: apparently, resetting the CSC is useless since CSC_SearchCSC()
 * already resets it according to its documentation
 */
static char reset_csc(DWORD* result, byte data[])
{
	*result = CSC_ResetCSC();

	if (*result == RCSC_Ok)
	{
		printf("%d. Reset CSC successful...\n", step++);
		return version_csc(result, data);
	}
	else
	{
		return mess("Reset CSC failure!", DO_NOT_FREE, *result);
	}
}

static char version_csc(DWORD* result, byte data[])
{
	*result = CSC_VersionCSC(data);

	if (*result == RCSC_Ok)
	{
		printf("%d. Version CSC successful...\n\t[Version=%s]\n", step++, data);
		return SUCCESS;
	}
	else
	{
		return mess("Version CSC failure!", DO_NOT_FREE, *result);
	}
}

static char configure_buffer(DWORD* result)
{
	*result = CSC_EHP_PARAMS_EXT(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (*result == RCSC_Ok)
	{
		printf("%d. Buffer configuration successfully updated...\n", step++);
		return SUCCESS;
	}
	else
	{
		return mess("Buffer configuration update failure!", DO_NOT_FREE, *result);
	}
}

static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data)
{
	*result = CSC_SearchCardExt(search, search_mask, forget, timeout, COM, length, data);

	if (*result == RCSC_Ok)
	{
		printf("%d. Card searching successful...\n\t[COM=%d, Length=%lu, ISO ATR=%s]\n", step++, *COM, *length, data);
		return SUCCESS;
	}
	else
	{
		return mess("Card searching failure!", DO_NOT_FREE, *result);
	}
}

static char nfc_forum_type_4_command(const char* command_type, const char *command_info,
	DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, ...)
{
	char index = 0;

	io_data[index++] = cla;
	io_data[index++] = ins;
	io_data[index++] = p1;
	io_data[index++] = p2;

	va_list args;
	va_start(args, p2);

	while (index != len)
	{
		io_data[index++] = va_arg(args, byte);
	}

	va_end(args);

	*result = CSC_ISOCommand(io_data, len, io_data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 %s (%s) successful...\n", step++, command_type, command_info);
		display_data_hex(io_data, *length);
		return SUCCESS;
	}
	else
	{
		char* message = (char*)malloc(sizeof(char) * 100);
		strcpy(message, "NFC Forum Type 4 ");
		strcat(message, command_type);
		strcat(message, " (");
		strcat(message, command_info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

static char nfc_forum_type_4_select(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...)
{
	char index = 0;

	io_data[index++] = cla;
	io_data[index++] = ins;
	io_data[index++] = p1;
	io_data[index++] = p2;
	io_data[index++] = lc;

	va_list args;
	va_start(args, lc);

	while (index != len)
	{
		io_data[index++] = va_arg(args, byte);
	}

	va_end(args);

	*result = CSC_ISOCommand(io_data, len, io_data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Select (%s) successful...\n", step++, info);
		display_data_hex(io_data, *length);
		return SUCCESS;
	}
	else
	{
		char* message = (char*)malloc(sizeof(char) * 100);
		strcpy(message, "NFC Forum Type 4 Select (");
		strcat(message, info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

static char nfc_forum_type_4_read_binary(const char* info, DWORD* result, DWORD* length, byte io_data[],
	byte cla, byte ins, byte p1, byte p2, byte le)
{
	io_data[0] = cla;
	io_data[1] = ins;
	io_data[2] = p1;
	io_data[3] = p2;
	io_data[4] = le;

	*result = CSC_ISOCommand(io_data, 5, io_data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Read Binary (%s) successful\n", step++, info);
		display_data_hex(io_data, *length);
		return SUCCESS;
	}
	else
	{
		char* message = (char*)malloc(sizeof(char) * 100);
		strcpy(message, "NFC Forum Type 4 Read Binary (");
		strcat(message, info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

static char nfc_forum_type_4_update_binary_hard(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...)
{
	char index = 0;

	io_data[index++] = cla;
	io_data[index++] = ins;
	io_data[index++] = p1;
	io_data[index++] = p2;
	io_data[index++] = lc;

	va_list args;
	va_start(args, lc);

	while (index != len)
	{
		io_data[index++] = va_arg(args, byte);
	}

	va_end(args);

	*result = CSC_ISOCommand(io_data, len, io_data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Update Binary (%s) successful...\n", step++, info);
		display_data_hex(io_data, *length);
		return SUCCESS;
	}
	else
	{
		char* message = (char*)malloc(sizeof(char) * 100);
		strcpy(message, "NFC Forum Type 4 Update Binary (");
		strcat(message, info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

static char nfc_forum_type_4_update_binary(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, byte* data)
{
	char index = 0;

	io_data[index++] = cla;
	io_data[index++] = ins;
	io_data[index++] = p1;
	io_data[index++] = p2;
	io_data[index++] = lc;

	while (index != len)
	{
		io_data[index++] = *data++;
	}

	*result = CSC_ISOCommand(io_data, len, io_data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Update Binary (%s) successful...\n", step++, info);
		display_data_hex(io_data, *length);
		return SUCCESS;
	}
	else
	{
		char* message = (char*)malloc(sizeof(char) * 100);
		strcpy(message, "NFC Forum Type 4 Update Binary (");
		strcat(message, info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

// When a field is omitted from the record, does that mean either the field is equal to 0 or the field is not present?
// I guess the second solution is correct.
static Record* parse_ndef_file(byte* data, int* total_records)
{
	int mem_size = sizeof(Record);
	Record* records = (Record*)malloc(mem_size);
	*total_records = 0;

	int byte_index = 0;
	int NLEN = (int)(data[0] << 8) + (int)(data[1]);
	data += 2;

	while (byte_index < NLEN)
	{
		*records = parse_record(data, &byte_index, NLEN);
		mem_size += sizeof(*records);
		records -= *total_records;
		records = (Record*)realloc(records, mem_size);
		(*total_records)++;
		records += *total_records;
	}

	records -= *total_records;
	return records;
}

static Record* parse_smart_poster(byte* data, int data_length, char* total_records)
{
	int mem_size = sizeof(Record);
	Record* records = (Record*)malloc(mem_size);
	*total_records = 0;

	int byte_index = 0;

	while (byte_index < data_length)
	{
		*records = parse_record(data, &byte_index, data_length);
		mem_size += sizeof(*records);
		records -= *total_records;
		records = (Record*)realloc(records, mem_size);
		(*total_records)++;
		records += *total_records;
	}

	records -= *total_records;
	return records;
}

static Record parse_record(byte* record_data, int* byte_index, int NLEN)
{
	Record record;
	record.type = NULL;
	record.id = NULL;
	record.payload = NULL;
	record.sp_records = NULL;
	record.corrupted = 0;

	// Flags
	record.mb = record_data[*byte_index] & 0x80 ? 1 : 0;
	record.me = record_data[*byte_index] & 0x40 ? 1 : 0;
	record.cf = record_data[*byte_index] & 0x20 ? 1 : 0;
	record.sr = record_data[*byte_index] & 0x10 ? 1 : 0;
	record.il = record_data[*byte_index] & 0x08 ? 1 : 0;
	record.tnf = record_data[(*byte_index)++] & 0x07;

	switch (record.tnf)
	{
	case Empty:
		// record.type_length, record.id_length and record.payload_length are 0
		// record.type, record.id and record.payload are omitted
		record.type_length = 0;
		record.payload_length = 0;
		record.id_length = 0;
		record.sp = 0;
		record.nb_sp_records = 0;
		return record;
	case TNF_Unknown:
	case Unchanged:
		record.type_length = 0;
		// record.type is omitted
		break;
	default:
		record.type_length = record_data[(*byte_index)++];
		break;
	}

	if (!record.sr)
	{
		record.payload_length = record_data[(*byte_index)++] & 0xF000;
		record.payload_length += record_data[(*byte_index)++] & 0x0F00;
		record.payload_length += record_data[(*byte_index)++] & 0x00F0;
		record.payload_length += record_data[(*byte_index)++] & 0x000F;
	}
	else
	{
		record.payload_length = record_data[(*byte_index)++];
	}

	record.id_length = record.il ? record_data[(*byte_index)++] : 0;

	if (record.type_length)
	{
		record.type = (byte*)malloc(sizeof(byte) * record.type_length);

		for (int i = 0; i < record.type_length; ++i)
		{
			if (*byte_index >= NLEN)
			{
				record.corrupted = 1;
				return record;
			}

			record.type[i] = record_data[(*byte_index)++];
		}
	}

	if (record.id_length)
	{
		record.id = (byte*)malloc(sizeof(byte) * record.id_length);

		for (int i = 0; i < record.id_length; ++i)
		{
			if (*byte_index >= NLEN)
			{
				record.corrupted = 1;
				return record;
			}

			record.id[i] = record_data[(*byte_index)++];
		}
	}

	record.payload = (byte*)malloc(sizeof(byte) * record.payload_length);

	for (unsigned int i = 0; i < record.payload_length; ++i)
	{
		if (*byte_index >= NLEN)
		{
			record.corrupted = 1;
			return record;
		}

		record.payload[i] = record_data[(*byte_index)++];
	}

	if (record.sp = (record.type_length == 2 && record.type[0] == 0x53 && record.type[1] == 0x70))
	{
		record.sp_records = parse_smart_poster(record.payload, record.payload_length, &(record.nb_sp_records));
	}

	return record;
}

// GUI Button Functions

static void read(void)
{
	// Select & Read NDEF File
	// Length:7; CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E104 Le:-
	if (nfc_forum_type_4_select("NDEF", &result, &length, io_data, 7,
		0x00, 0xA4, 0x00, 0x0C, 0x02, NDEF_File[0], NDEF_File[1]))
	{
		int read_cycles = buffer_length / MLe;
		int offset = 0;
		byte* NDEF_data = (byte*)malloc(sizeof(byte) * buffer_length);

		if (!NDEF_data)
		{
			CSC_AntennaOFF();
			CSC_Close();
			printf("Memory allocation failed!\n");
			return;
		}

		for (int i = 0; i < read_cycles; i++)
		{
			offset = MLe * i;

			// Length:5; CLA:00 INS:B0 P1/P2:Offset Lc:- Data:- Le:MLe
			if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
				0x00, 0xB0, (byte)(offset >> 8), (byte)offset, MLe))
			{
				return;
			}

			copy_string(io_data, NDEF_data, MLe, 1);
			NDEF_data += MLe;
		}

		offset = MLe * read_cycles;

		// Length:5; CLA:00 INS:B0 P1/P2:Offset Lc:- Data:- Le:MLe
		if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
			0x00, 0xB0, (byte)(offset >> 8), (byte)offset, buffer_length - offset))
		{
			return;
		}

		copy_string(io_data, NDEF_data, buffer_length - offset, 1);
		NDEF_data -= offset;

		printf("NDEF Data:\n");
		display_data_hex(NDEF_data, buffer_length);

		if (records)
		{
			free_records(records, record_length);
		}

		records = parse_ndef_file(NDEF_data, &record_length);
		display_records(records, record_length, 0);
		free(NDEF_data);
	}
}

static void write(byte* data, unsigned int data_length)
{
	// Select & Update Binary NDEF File
	// Length:7; CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E104 Le:-
	if (nfc_forum_type_4_select("NDEF", &result, &length, io_data, 7,
		0x00, 0xA4, 0x00, 0x0C, 0x02, NDEF_File[0], NDEF_File[1]))
	{
		// Write URI 0x01(?) 0x61 0x70 0x70 0x6C 0x65 0x2E 0x63 0x6F 0x6D
		// Write Text (?) 0x02, 0x66, 0x72, (?) 0x4C, 0x61, 0x20, 0x62, 0x65,
		// 0x6C, 0x6C, 0x65, 0x20, 0x68, 0x69, 0x73, 0x74, 0x6f, 0x69, 0x72, 0x65
		// Write Data 0x50, 0x4f, 0x4c, 0x59, 0x54, 0x45, 0x43, 0x48
		// 003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348

		const char* info = "NDEF";
		int write_cycles = data_length / MLc;
		int offset = 0;

		for (int i = 0; i < write_cycles; i++)
		{
			offset = MLc * i;

			if (!nfc_forum_type_4_update_binary(info, &result, &length, io_data, MLc,
				0x00, 0xD6, (byte)(offset >> 8), (byte)offset, MLc, data))
			{
				return;
			}

			data += MLc;
		}

		offset = MLc * write_cycles;

		if (!nfc_forum_type_4_update_binary(info, &result, &length, io_data, 5 + data_length - offset,
			0x00, 0xD6, (byte)(offset >> 8), (byte)offset, data_length - offset, data))
		{
			return;
		}
	}
}

#ifdef _SMART_CARDS_

int main(void)
{
	char input = '!';

	fopen_s(&trace, "trace.txt", "w+");
	printf("\n--------------------------------------------------\n");
	printf("Initializing Smart Cards Program Test...\n");
	initialize();
	printf("\n\n\n");

	while (input != '0')
	{
		printf("\n--------------------------------------------------");
		printf("\n------------------- Main Menu --------------------\n");
		printf("\n 0: Exit");
		printf("\n 1: Read");
		printf("\n 2: Write");
		printf("\n--------------------------------------------------\n");

		input = _getch();

		switch (input)
		{
		case '0':
			break;
		case '1':
			read();
			break;
		case '2':
			printf("\n\n\n");
			printf("\n--------------------------------------------------");
			printf("\n------------------ Write Menu --------------------\n");
			printf("\n 0: Exit");
			printf("\n 1: Raw");
			printf("\n--------------------------------------------------\n");

			input = _getch();
			byte* data = (byte*)malloc(sizeof(byte) * buffer_length * 2);
			int data_length = 0;

			switch (input)
			{
			case '0':
				input = '!';
				break;
			case '1':
				printf("Raw Data (hex), press enter to finish writing (Buffer Length = %d): ", buffer_length);
				scanf("%s", data);
				data_length = len(data);

				while (data_length >= buffer_length * 2)
				{
					printf("Raw Data (hex), press enter to finish writing (Buffer Length = %d): ", buffer_length);
					scanf("%s", data);
				}

				flush();
				printf("Proceed writing? [y/n] ");
				scanf("%c", &input);

				if (input == 'n')
				{
					free(data);
					break;
				}
				else if (input == 'y')
				{
					if (data = str_to_hex(data))
					{
						write(data, data_length / 2);
					}

					free(data);
					break;
				}

				free(data);
				printf("Invalid answer (%c)...\n", input);
				input = '!';
				break;
			}

			break;
		default:
			printf("%c is not a valid choice, please choose one of the 'Main Menu' choices ->\n", input);
			break;
		}
	}

	CSC_AntennaOFF();
	CSC_Close();
	fclose(trace);
	printf("Press a Key.\n");
	_getch();
	return 0;
}

#endif //_SMART_CARDS_
