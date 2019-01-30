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
			D102/1D
				5270
				9101/11/8801-70617261676F6E2D726669642E62F6D
				5101/04/5400-504944

	Enter hunt phase parameters --> autoselect = 0
	=== > Désactivation de l'envoie SELECT APPLI automatique lors de la phase anticollision
	Mettre après version CSC autoselect à 0
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
	DEFINE & GLOBAL VARIABLES
*****************************************************************/

#define FAILURE 0
#define SUCCESS 1
#define DO_NOT_FREE 0
#define FREE 1

static FILE *trace;
static int step = 1;

/*****************************************************************
	FUNCTIONS
*****************************************************************/

void execute(void);
static char search_csc(DWORD* result);
static char reset_csc(DWORD* result, unsigned char io_data[]);
static char version_csc(DWORD* result, unsigned char io_data[]);
static char configure_buffer(DWORD* result);
static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data
);
static char nfc_forum_type_4_select(const char* info, DWORD* result, DWORD* length, unsigned char io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, ...);
static char nfc_forum_type_4_read_binary(const char* info, DWORD* result, DWORD* length, unsigned char io_data[],
	byte cla, byte ins, byte p1, byte p2, byte le);

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

static void display_data_string(unsigned char* data, DWORD length)
{
	printf("\t[Data STR=");

	for (unsigned int i = 0; i < length; ++i)
	{
		printf("%c", data[i]);
	}

	printf(", Length=%d]\n", length);
}

static void display_data_hex(unsigned char* data, DWORD length)
{
	printf("\t[Data HEX=0x");

	for (unsigned int i = 0; i < length; ++i)
	{
		printf("%02X", data[i]);
	}

	printf(", Length=%d]\n", length);
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

static void copy_string(char* source, char* destination, unsigned int length)
{
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

	unsigned char io_data[256];

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
						copy_string(io_data, NDEF_data, MLe);
						NDEF_data += MLe;
					}

					offset = MLe * read_cycles;

					// Length:5; CLA:00 INS:B0 P1/P2:Offset Lc:- Data:- Le:MLe
					if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
						0x00, 0xB0, (byte)(offset >> 8), (byte)offset, MLe))
					{
						return;
					}

					copy_string(io_data, NDEF_data, MLe);
					NDEF_data -= MLe * read_cycles;

					printf("NDEF Data:\n");
					display_data_hex(NDEF_data, buffer_length);
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
static char reset_csc(DWORD* result, unsigned char data[])
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

static char version_csc(DWORD* result, unsigned char data[])
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

static char nfc_forum_type_4_select(const char* info, DWORD* result, DWORD* length, unsigned char io_data[], int len,
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
		strcpy(message, "NFC Forum Type 4 Read Binary (");
		strcat(message, info);
		strcat(message, ") failure!");

		return mess(message, FREE, *result);
	}
}

static char nfc_forum_type_4_read_binary(const char* info, DWORD* result, DWORD* length, unsigned char io_data[],
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