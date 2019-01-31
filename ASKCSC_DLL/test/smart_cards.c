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
				5270 ---> Smart Poster
				9101/11/8801-70617261676F6E2D726669642E62F6D ---> 91 -> Short record. 11 -> 17 octets pour la ressource. Utiliser les tables pour le type de l'URI  0x70... URL encodée
				5101/04/5400-504944 --> UTF 8 dont on ne connait pas la longueur

	Enter hunt phase parameters --> autoselect = 0
	=== > Désactivation de l'envoie SELECT APPLI automatique lors de la phase anticollision
	Mettre après version CSC autoselect à 0

	Création d'une instanciation sir carte RFID

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

	Write en fonction de MaxLc pour savoir le découpage que l'on va faire

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
} Record;

/*****************************************************************
	FUNCTIONS
*****************************************************************/

void execute(void);
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
static char nfc_forum_type_4_update_binary(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...);
static Record read_payload(byte* payload, int length);

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

static void display_records(Record* records, DWORD length)
{
	for (unsigned int i = 0; i < length; ++i, ++records)
	{
		printf("[Record:\n\t--- Flags ---\n\t- MB: %s\n\t- ME: %s\n\t- CF: %s\n\t- SR: %s\n\t- IL: %s\n\t- TNF: ",
			records->mb ? YES : NO, records->me ? YES : NO, records->cf ? YES : NO,
			records->sr ? YES : NO, records->il ? YES : NO);
		display_TNF(records->tnf);
		printf("\n\t- Type Length: %d\n\t- Payload Length: %d\n\t",
			records->type_length, records->payload_length);

		if (records->il)
		{
			printf("- ID Length: %d\n\t- ID: 0x", records->id_length);

			for (unsigned int j = 0; j < records->id_length; ++j)
			{
				printf("%02X", records->id[j]);
			}
		}

		if (records->type_length)
		{
			printf("\n\t- Type: 0x");

			for (unsigned int j = 0; j < records->type_length; ++j)
			{
				printf("%02X", records->type[j]);
			}
		}

		if (records->payload_length)
		{
			printf("\n\t- Payload: 0x");

			for (unsigned int j = 0; j < records->payload_length; ++j)
			{
				printf("%02X", records->payload[j]);
			}
		}

		printf("\nLength=%d]\n", length);
	}
}

// Actually useless...
static long hex_to_dec(char* hex)
{
	long base = 1; // 16^0 = 1;
	long dec = 0;
	int length = strlen(hex);
	int stop = 0;

	if (length > 2 && hex[0] == '0' && hex[1] == 'x')
	{
		stop += 2;
	}

	for (int i = length - 1; i >= stop; --i)
	{
		if (hex[i] >= '0' && hex[i] <= '9')
		{
			dec += (hex[i] - '0') * base;
		}
		else if (hex[i] >= 'a' && hex[i] <= 'f')
		{
			dec += (hex[i] - 'a') * base;
		}
		else if (hex[i] >= 'A' && hex[i] <= 'F')
		{
			dec += (hex[i] - 'A') * base;
		}

		base *= 16; // power replacement;
	}

	return dec;
}

static void copy_string(char* source, char* destination, unsigned int length, unsigned int offset)
{
	source += offset;
	for (; length > 0; *destination++ = *source++, --length);
}

void execute(void)
{
	DWORD result;
	DWORD length;
	DWORD search_mask;

	BYTE COM;

	sCARD_SearchExt  search_struct;
	sCARD_SearchExt* search = &search_struct;

	byte io_data[256];

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
				byte MLe_byte = io_data[5];
				byte MLc_byte = io_data[6];
				byte buffer_length_bytes[2] = { io_data[12], io_data[13] };

				int MLe = MLe_byte;
				int MLc = MLc_byte;
				int buffer_length = buffer_length_bytes[1] + (buffer_length_bytes[0] << 8);

				printf("Read CC Info Details:\n");
				printf("\t[MLe=%#04x (%d), NDEF Max File Size=0x%02X%02X (%d), NDEF File ID=0x%02X%02X]\n",
					MLe_byte, MLe,
					buffer_length_bytes[1], buffer_length_bytes[2], buffer_length,
					io_data[10], io_data[11]);

				// Select & Read NDEF File
				// Length:7; CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E104 Le:-
				if (nfc_forum_type_4_select("NDEF", &result, &length, io_data, 7,
					0x00, 0xA4, 0x00, 0x0C, 0x02, io_data[10], io_data[11]))
				{
					int read_cycles = buffer_length / MLe;
					int offset = 0;
					unsigned int bytes_read = 0;
					byte* NDEF_data = (byte*)malloc(sizeof(byte) * buffer_length);
					Record* payloads = (Record*)malloc(sizeof(Record) * (read_cycles + 1));

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

						// Why the number of data read is always the number we ordered 
						// to read + 3?
						//display_data_string(io_data, length);
						copy_string(io_data, NDEF_data, MLe, 1);
						payloads[i] = read_payload(NDEF_data, MLe);
						NDEF_data += MLe;
					}

					offset = MLe * read_cycles;

					// Length:5; CLA:00 INS:B0 P1/P2:Offset Lc:- Data:- Le:MLe
					if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
						0x00, 0xB0, (byte)(offset >> 8), (byte)offset, MLe))
					{
						return;
					}

					copy_string(io_data, NDEF_data, MLe, 1);
					payloads[read_cycles + 1] = read_payload(NDEF_data, MLe);
					NDEF_data -= MLe * read_cycles;

					printf("NDEF Data:\n");
					display_data_hex(NDEF_data, buffer_length);
					//display_records(payloads, read_cycles + 1);
				}
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

static char nfc_forum_type_4_update_binary(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
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

// When a field is omitted from the record, does that mean either the field is equal to 0 or the field is not present?
// I guess the second solution is correct.
static Record read_payload(byte* payload, int length)
{
	Record record;
	int byte_index = 0;

	// Flags
	record.mb = payload[byte_index] & 0x80 ? 1 : 0;
	record.me = payload[byte_index] & 0x40 ? 1 : 0;
	record.cf = payload[byte_index] & 0x20 ? 1 : 0;
	record.sr = payload[byte_index] & 0x10 ? 1 : 0;
	record.il = payload[byte_index] & 0x08 ? 1 : 0;
	record.tnf = payload[byte_index++] & 0x07;

	switch (record.tnf)
	{
	case Empty:
		// record.type_length, record.id_length and record.payload_length are 0
		// record.type, record.id and record.payload are omitted
		return record;
	case TNF_Unknown:
	case Unchanged:
		record.type_length = 0;
		// record.type is omitted
		break;
	default:
		record.type_length = payload[byte_index++];
		break;
	}

	if (!record.sr)
	{
		record.payload_length = payload[byte_index++] & 0xF000;
		record.payload_length += payload[byte_index++] & 0x0F00;
		record.payload_length += payload[byte_index++] & 0x00F0;
		record.payload_length += payload[byte_index++] & 0x000F;
	}
	else
	{
		record.payload_length = payload[byte_index++];
	}

	record.id_length = record.il ? payload[byte_index++] : 0;

	if (record.type_length)
	{
		record.type = (byte*)malloc(sizeof(byte) * record.type_length);

		for (int i = 0; i < record.type_length; ++i)
		{
			record.type[i] = payload[byte_index++];
		}
	}

	if (record.id_length)
	{
		record.id = (byte*)malloc(sizeof(byte) * record.id_length);

		for (int i = 0; i < record.id_length; ++i)
		{
			record.id[i] = payload[byte_index++];
		}
	}

	record.payload = (byte*)malloc(sizeof(byte) * record.payload_length);

	for (int i = 0; i < record.payload_length; ++i)
	{
		record.payload[i] = payload[byte_index++];
	}

	// Final check before exiting
	if (byte_index > length)
	{
		mess("Failed to read payload", DO_NOT_FREE, NULL);
	}

	return record;
}

#ifdef _SMART_CARDS_

int main(void)
{
	fopen_s(&trace, "trace.txt", "w+");
	execute();
	fclose(trace);
	printf("Press a Key.\n");
	_getch();
	return 0;
}

#endif //_SMART_CARDS_


/*

2. Regarder si il y a 1 ou plusieurs record dans le champs NDEF (Comme on a le début et la fin -> 1 seul record)
3. Longeur du champ type  ---> 0x5370 Sp = SmartPoster
		===> Contient un certain nombre de records
4. Longueur du champ payload
5. ID length et longueur -> si il n'est pas là, le champ n'est pas présent

*/