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

	Version CSC / Reset CSC
	Enter hunt phase parameters
	Enter hunt phase
	Select Appli
	Select file 0xE103
	Read binary 15 octets
	-> Read binary du reste si nécessaire
	--> Max le (1)
	--> Max lc
	--> LID du fichier NDEF (2)
	--> Max size fichier NDEF (3)

	Select file "NDEF" (2)
	Read binary (1) (3)

*****************************************************************/

/*****************************************************************
	INCLUDE
*****************************************************************/

// Standard C Library
#include <stdio.h>
#include <stdlib.h>

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

static FILE *trace;
static int step = 1;

/*****************************************************************
	FUNCTIONS
*****************************************************************/

void start(void);
static char search_csc(DWORD* result);
static char reset_csc(DWORD* result, unsigned char io_data[]);
static char version_csc(DWORD* result, unsigned char io_data[]);
static char configure_buffer(DWORD* result);
static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data
);
static char nfc_forum_type_4_select(DWORD* result, DWORD* length, unsigned char data[]);
static char nfc_forum_type_4_read_binary(DWORD* result, DWORD* length, unsigned char data[]);
static char ndef_tag_application_select(DWORD* result, DWORD* length, unsigned char data[]);
static char ndef_tag_cc_select(DWORD* result, DWORD* length, unsigned char data[]);
static char ndef_tag_cc_read_binary(DWORD* result, DWORD* length, unsigned char data[]);
static char ndef_tag_ndef_select(DWORD* result, DWORD* length, unsigned char data[]);
static char ndef_tag_ndef_read_binary(DWORD* result, DWORD* length, unsigned char data[]);

static char Mess(LPSTR text, DWORD result)
{
	printf("\nSmart Cards Program Stopped:\n%s\n", text);

	fprintf(trace, "\nSmart Cards Program Stopped: %s", text);
	fprintf(trace, " ---- %02X\n", result);

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

void start(void)
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
		if (!ndef_tag_application_select(&result, &length, io_data) ||
			(io_data[1] != 0x90 && io_data[2] != 0x00) ||
			(io_data[1] != 0x62 && io_data[2] != 0x00 && io_data[3] != 0x90 && io_data[4] != 0x00))
		{
			return;
		}

		// Select & Read CC File
		if (ndef_tag_cc_select(&result, &length, io_data))
		{
			if (ndef_tag_cc_read_binary(&result, &length, io_data))
			{
				byte max_Le_byte = io_data[5];
				byte buffer_length_bytes[2] = { io_data[12], io_data[13] };

				int max_Le = max_Le_byte;
				int buffer_length = buffer_length_bytes[1] + (buffer_length_bytes[0] << 8);

				int read_cycles = buffer_length / max_Le;
				int offset = 0;
				byte* NDEF_data = malloc(buffer_length);

				if (!NDEF_data)
				{
					CSC_AntennaOFF();
					CSC_Close();
					printf("Memory allocation failed!\n");
					return;
				}

				for (int i = 0; i < read_cycles - 1; i++)
				{
					offset = max_Le * i;

					io_data[0] = 0x00;
					io_data[1] = 0xB0;
					io_data[2] = offset & (0xFF << 16); // Offset
					io_data[3] = offset & 0xFF; // Offset
					io_data[4] = max_Le;

					length = 5;

					if (ndef_tag_ndef_read_binary(&result, &length, io_data))
					{
						NDEF_data = io_data;
						NDEF_data += offset;
					}
					else
					{
						return;
					}
				}

				offset = max_Le * read_cycles;

				io_data[0] = 0x00;
				io_data[1] = 0xB0;
				io_data[2] = offset & (0xFF << 16); // Offset
				io_data[3] = offset & 0xFF; // Offset
				io_data[4] = buffer_length - offset;

				length = 5;

				if (ndef_tag_ndef_read_binary(&result, &length, io_data))
				{
					NDEF_data = io_data;
					NDEF_data += offset;
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
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
		return Mess("Search CSC failure!", *result);
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
		return Mess("Reset CSC failure!", *result);
	}
}

static char version_csc(DWORD* result, unsigned char data[])
{
	*result = CSC_VersionCSC(data);

	if (*result == RCSC_Ok)
	{
		printf("%d. Version CSC successful: [%s]\n", step++, data);
		return SUCCESS;
	}
	else
	{
		return Mess("Version CSC failure!", *result);
	}
}

static char configure_buffer(DWORD* result)
{
	*result = CSC_EHP_PARAMS_EXT(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (*result == RCSC_Ok)
	{
		printf("%d. Buffer configuration successfully updated\n", step++);
		return SUCCESS;
	}
	else
	{
		return Mess("Buffer configuration update failure!", *result);
	}
}

static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data)
{
	*result = CSC_SearchCardExt(search, search_mask, forget, timeout, COM, length, data);

	if (*result == RCSC_Ok)
	{
		printf("%d. Card searching successful: [%s]\n", step++, data);
		return SUCCESS;
	}
	else
	{
		return Mess("Card searching failure!", *result);
	}
}

static char nfc_forum_type_4_select(DWORD* result, DWORD* length, unsigned char data[])
{
	*result = CSC_ISOCommand(data, *length, data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Select successful: [Data=0x%x, Length=%x]\n", step++, data, *length);
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Select failure!", *result);
	}
}

static char nfc_forum_type_4_read_binary(DWORD* result, DWORD* length, unsigned char data[])
{
	*result = CSC_ISOCommand(data, *length, data, length);

	if (*result == RCSC_Ok)
	{
		printf("%d. NFC Forum Type 4 Read Binary successful: [Data=0x%x, Length=%x]\n", step++, data, *length);
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Read Binary failure!", *result);
	}
}

static char ndef_tag_application_select(DWORD* result, DWORD* length, unsigned char data[])
{
	// CLA:00 INS:A4 P1:04 P2:00 Lc:07 Data:D2760000850101 Le:00
	data[0] = 0x00;
	data[1] = 0xA4;
	data[2] = 0x04;
	data[3] = 0x00;
	data[4] = 0x07;
	data[5] = 0xD2;
	data[6] = 0x76;
	data[7] = 0x00;
	data[8] = 0x00;
	data[9] = 0x85;
	data[10] = 0x01;
	data[11] = 0x01;
	data[12] = 0x00;

	*length = 13;

	if (nfc_forum_type_4_select(result, length, data))
	{
		printf("\tNDEF Tag Application Select successful...\n");
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Select Application failure!", *result);
	}
}

static char ndef_tag_cc_select(DWORD* result, DWORD* length, unsigned char data[])
{
	// CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E103 Le:-
	data[0] = 0x00;
	data[1] = 0xA4;
	data[2] = 0x00;
	data[3] = 0x0C;
	data[4] = 0x02;
	data[5] = 0xE1;
	data[6] = 0x03;

	*length = 7;

	if (nfc_forum_type_4_select(result, length, data))
	{
		printf("\tNDEF Tag CC Select successful...\n");
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Select CC failure!", *result);
	}
}

static char ndef_tag_cc_read_binary(DWORD* result, DWORD* length, unsigned char data[])
{
	// CLA:00 INS:B0 P1: 00 P2:00 Lc:- Data:- Le:0F
	data[0] = 0x00;
	data[1] = 0xB0;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x0F;

	*length = 5;

	if (nfc_forum_type_4_read_binary(result, length, data))
	{
		printf("\tNDEF Tag CC Read Binary successful...\n");
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Read Binary CC failure!", *result);
	}
}

static char ndef_tag_ndef_select(DWORD* result, DWORD* length, unsigned char data[])
{
	// CLA:00 INS:A4 P1:00 P2:0C Lc:02 Data:E104 Le:-
	data[0] = 0x00;
	data[1] = 0xA4;
	data[2] = 0x00;
	data[3] = 0x0C;
	data[4] = 0x02;
	data[5] = data[10];
	data[6] = data[11];

	*length = 7;

	if (nfc_forum_type_4_select(result, length, data))
	{
		printf("\tNDEF Tag NDEF Select successful...\n");
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Select NDEF failure!", *result);
	}
}

static char ndef_tag_ndef_read_binary(DWORD* result, DWORD* length, unsigned char data[])
{
	if (nfc_forum_type_4_read_binary(result, length, data))
	{
		printf("\tNDEF Tag NDEF Read Binary successful...\n");
		return SUCCESS;
	}
	else
	{
		return Mess("NFC Forum Type 4 Read Binary NDEF failure!", *result);
	}
}

#ifdef _SMART_CARDS_

int main(void)
{
	fopen_s(&trace, "trace.txt", "w+");
	start();
	fclose(trace);
	printf("Press a Key.\n");
	_getch();
	return 0;
}

#endif //_SMART_CARDS_