#pragma once

#include "const.h"
#include "gl_model.h"

void		CM_CalcPAS( model_t* model );
void		CM_FreePAS();
byte*		CM_LeafPVS( int leafnum );
byte*		CM_LeafPAS( int leafnum );
void		CM_DecompressPVS( byte* input, byte* decompressed, int byteCount );
qboolean	CM_HeadnodeVisible( mnode_t* node, byte* visibility, int* firstVisibleLeaf );
