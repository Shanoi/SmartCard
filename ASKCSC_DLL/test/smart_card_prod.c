#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <conio.h>
#include "../askcsc.h"
#include "select.h"

#define FAILURE 0
#define SUCCESS 1
#define DO_NOT_FREE 0
#define FREE 1

static FILE *trace;
static int step = 1;

static void execute(void);
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

static void display_data_hex(unsigned char data[], DWORD length)
{
	printf("\t[Data=0x");

	for (unsigned int i = 0; i < length; ++i)
	{
		printf("%02X", data[i]);
	}

	printf(", Length=%d]\n", length);
}

static void execute(void)
{
	DWORD result;
	DWORD length;
	DWORD search_mask;

	BYTE COM;

	sCARD_SearchExt  search_struct;
	sCARD_SearchExt* search = &search_struct;

	unsigned char io_data[256];

	search->CONT = 0;
	search->ISOB = 1;
	search->ISOA = 1;
	search->TICK = 0;
	search->INNO = 0;
	search->MIFARE = 0;
	search->MV4k = 0;
	search->MV5k = 0;
	search_mask = SEARCH_MASK_ISOA | SEARCH_MASK_ISOB;

	if (search_csc(&result) && reset_csc(&result, io_data) &&
		configure_buffer(&result) &&
		search_card(&result, search, search_mask, 0x01, 100, &COM, &length, io_data))
	{
		if (!nfc_forum_type_4_select("Application", &result, &length, io_data, 13,
			0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00) ||
			(io_data[1] != 0x90 && io_data[2] != 0x00) ||
			(io_data[1] != 0x62 && io_data[2] != 0x00 && io_data[3] != 0x90 && io_data[4] != 0x00))
		{
			return;
		}

		if (nfc_forum_type_4_select("CC", &result, &length, io_data, 7,
			0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03))
		{
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

				if (nfc_forum_type_4_select("NDEF", &result, &length, io_data, 7,
					0x00, 0xA4, 0x00, 0x0C, 0x02, io_data[10], io_data[11]))
				{
					int read_cycles = buffer_length / MLe;
					int offset = 0;
					byte* NDEF_data = malloc(sizeof(byte) * buffer_length);
					NDEF_data = "";

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

						if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
							0x00, 0xB0, (byte)(offset >> 8), (byte)offset, MLe))
						{
							return;
						}

						strcat(NDEF_data, io_data);
						display_data_hex(NDEF_data, MLe);
						NDEF_data += MLe;
					}

					offset = MLe * read_cycles;

					if (!nfc_forum_type_4_read_binary("NDEF", &result, &length, io_data,
						0x00, 0xB0, (byte)(offset >> 8), (byte)offset, MLe))
					{
						return;
					}

					strcat(NDEF_data, io_data);
					NDEF_data -= MLe * (read_cycles - 1);

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

#ifdef _SMART_CARDS_PROD_

int main(void)
{
	fopen_s(&trace, "trace.txt", "w+");
	execute();
	fclose(trace);
	printf("Press a Key.\n");
	_getch();
	return 0;
}

#endif //_SMART_CARDS_PROD_