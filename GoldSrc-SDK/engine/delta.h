#pragma once

#include "const.h"

#include <cstddef>

#define DELTA_FIELD_NAME_BYTES 32
#define DELTA_ENCODER_NAME_BYTES 32

struct delta_s;

typedef void ( *encoder_t )( delta_s*, const unsigned char*, const unsigned char* );

typedef struct
{
	int sendcount;
	int receivedcount;
} delta_stats_t;

typedef struct delta_description_s
{
	int fieldType;
	char fieldName[DELTA_FIELD_NAME_BYTES];
	int fieldOffset;
	short fieldSize;
	int significant_bits;
	float premultiply;
	float postmultiply;
	short flags;
	delta_stats_t stats;
} delta_description_t;

typedef struct delta_s
{
	int dynamic;
	int fieldCount;
	char conditionalencodename[DELTA_ENCODER_NAME_BYTES];
	encoder_t conditionalencode;
	delta_description_t* pdd;
} delta_t;

typedef struct delta_encoder_s
{
	delta_encoder_s* next;
	char* name;
	encoder_t conditionalencode;
} delta_encoder_t;

typedef struct delta_link_s
{
	delta_link_s* next;
	delta_description_t* delta;
} delta_link_t;

typedef struct delta_definition_s
{
	char* fieldName;
	int fieldOffset;
} delta_definition_t;

typedef struct delta_registry_s
{
	delta_registry_s* next;
	char* name;
	delta_t* pdesc;
} delta_registry_t;

typedef struct delta_definition_list_s
{
	delta_definition_list_s* next;
	char* ptypename;
	int numelements;
	delta_definition_t* pdefinition;
} delta_definition_list_t;

extern delta_t g_MetaDelta;

qboolean				DELTA_Load( char* name, delta_t** description, char* filename );
void					DELTA_RegisterDescription( char* name );
delta_t**				DELTA_LookupRegistration( char* name );
void					DELTA_AddEncoder( char* name, encoder_t conditionalencode );
void					DELTA_Shutdown();

delta_description_t*	DELTA_FindField( delta_t* fields, const char* name );
int						DELTA_FindFieldIndex( delta_t* fields, const char* name );
void					DELTA_SetField( delta_t* fields, const char* name );
void					DELTA_UnsetField( delta_t* fields, const char* name );
void					DELTA_SetFieldByIndex( delta_t* fields, int index );
void					DELTA_UnsetFieldByIndex( delta_t* fields, int index );

void					DELTA_MarkSendFields( unsigned char* from, unsigned char* to, delta_t* fields );
int						DELTA_CheckDelta( unsigned char* from, unsigned char* to, delta_t* fields );
int						DELTA_TestDelta( unsigned char* from, unsigned char* to, delta_t* fields );
void					DELTA_WriteMarkedFields( unsigned char* from, unsigned char* to, delta_t* fields );
int						DELTA_WriteDelta( unsigned char* from, unsigned char* to, qboolean force, delta_t* fields,
							void ( *callback )() );
int						_DELTA_WriteDelta( unsigned char* from, unsigned char* to, qboolean force, delta_t* fields,
							void ( *callback )(), int sendFields );
int						DELTA_ParseDelta( unsigned char* from, unsigned char* to, delta_t* fields, int bufferSize );
