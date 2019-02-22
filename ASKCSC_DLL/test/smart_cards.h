#pragma once

/*****************************************************************
	INCLUDE
*****************************************************************/

// Standard C Library
#include <stdio.h>

// WinAPI
#include <windows.h>

// Askcsc DLL
#include "../askcsc.h"

/*****************************************************************
	DEFINE & GLOBAL VARIABLES, STRUCTURES, ENUM...
*****************************************************************/

#define FAILURE 0
#define SUCCESS 1

#define DO_NOT_FREE 0
#define FREE        1

#define YES "Yes"
#define NO  "No"

#define SELECT        "SELECT"
#define READ_BINARY   "Read Binary"
#define UPDATE_BINARY "Update Binary"

// Error Logging file
static FILE *trace;

// Track the step number of a single program run
static int step = 1;

// APDU Command Code Error
static byte CLA_UNKNOWN[] = { 0x6E, 0x00 };
static byte INS_UNKNOWN[] = { 0x6D, 0x00 };
static byte LC_INCORRECT[] = { 0x67, 0x00 };
static byte LE_INCORRECT[] = { 0x6C, 0x00 };
static byte AID_LID_UNKNOWN[] = { 0x6A, 0x82 };
static byte NO_COMPLIANT_STATE[] = { 0x69, 0x86 };
static byte P1_2_INCORRECT_SELECT[] = { 0x6A, 0x86 };
static byte P1_2_INCORRECT_READUPDATE[] = { 0x6B, 0x00 };
static byte OFFSET_LC_INCORRECT[] = { 0x6A, 0x87 };
static byte OFFSET_LE_INCORRECT[] = { 0x6C, 0x00 };

enum TNF {
	Empty, NFC_Forum_Type, Media_Type, Absolute_URI,
	NFC_Forum_External_Type, TNF_Unknown, Unchanged, Reserved
};

/*
 * mb            : 1-bit Message Begin, useful when interpreted considering the ME flag's value.
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
 *
 * corrupted     : 1 if the record is corrupted, 0 otherwise.
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

// GUI
static HWND write_field;
static HWND log_field;

/*****************************************************************
	FUNCTION DECLARATIONS
*****************************************************************/

/******************** AskCSC / NFC Funcions *********************/

static void initialize(void);
static char search_csc(DWORD* result);
static char reset_csc(DWORD* result, byte io_data[]);
static char version_csc(DWORD* result, byte io_data[]);
static char configure_buffer(DWORD* result);
static char search_card(
	DWORD* result, sCARD_SearchExt* search, DWORD search_mask,
	BYTE forget, BYTE timeout, LPBYTE COM, LPDWORD length, BYTE* data
);

/****************************************************************/

/********************** NFC Forum Type 4 ************************/

static char nfc_forum_type_4_command(const char* command_type, const char *command_info,
	DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, ...);
static char nfc_forum_type_4_command_varargs(const char* command_type, const char *command_info,
	DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, ...);
static char nfc_forum_type_4_update_binary(const char* info, DWORD* result, DWORD* length, byte io_data[], int len,
	byte cla, byte ins, byte p1, byte p2, byte lc, byte* data);

static Record* parse_ndef_file(byte* data, int* total_records);
static Record* parse_smart_poster(byte* data, int data_length, char* total_records);
static Record parse_record(byte* data, int* byte_index, int NLEN);
static void read(void);
static void dumb_read(void);
static void write(byte* data, unsigned int data_length);

/****************************************************************/

/*********************** Display Funcions ***********************/

static char mess(char* text, char to_free, DWORD result);
static void display_TNF(char tnf);
static void display_data_string_to_hex(byte* data, DWORD length);
static void display_data_hex_to_string(byte* data, DWORD length);
static void display_records(Record* records, DWORD length, int level);

/****************************************************************/

/******************** Conversion Functions **********************/

static long hex_to_dec(byte* hex, int from, int to);
static byte* str_to_hex(byte* data);

/****************************************************************/

/*********************** Util Functions *************************/

static unsigned int len(byte* str);
static void copy_string(char* source, char* destination, unsigned int length, unsigned int source_offset);
static void flush();
static void free_records(Record* records, int length);
static void clean();

// GUI Functions
static void AppendText(HWND hwnd, char *new_text);

/****************************************************************/