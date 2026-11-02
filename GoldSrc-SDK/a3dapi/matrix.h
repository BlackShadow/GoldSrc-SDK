#pragma once

void	__cdecl Matrix_Translate( const float offset[3], float matrix[16] );
void	__cdecl Matrix_Scale( const float scale[3], float matrix[16] );
void	__cdecl Matrix_Multiply( const float left[16], const float right[16], float out[16] );
void	__cdecl Matrix_Identity( float matrix[16] );
void	__cdecl Matrix_GetTranslation( const float matrix[16], float out[3] );
void	__cdecl Matrix_TransformVector( const float in[3], const float matrix[16], float out[3] );
