#include "matrix_inverse.h"

#include <string.h>

void __cdecl Matrix_Inverse( const float matrix[16], float out[16] )
{
	if ( matrix[3] < 0.0f || matrix[3] > 0.0f ||
		matrix[7] < 0.0f || matrix[7] > 0.0f ||
		matrix[11] < 0.0f || matrix[11] > 0.0f ||
		matrix[15] < 1.0f || matrix[15] > 1.0f )
	{
		Matrix_InverseGeneral( matrix, out );
		return;
	}

	double firstCofactor = (double)matrix[5] * matrix[10] - (double)matrix[6] * matrix[9];
	float cofactors[3];
	cofactors[0] = (float)firstCofactor;
	cofactors[1] = (float)( (double)matrix[9] * matrix[2] - (double)matrix[1] * matrix[10] );
	cofactors[2] = (float)( (double)matrix[6] * matrix[1] - (double)matrix[2] * matrix[5] );

	double determinant = ( (double)matrix[4] * cofactors[1] + (double)matrix[8] * cofactors[2] ) +
		firstCofactor * matrix[0];
	float roundedDeterminant = (float)determinant;

	if ( !( determinant < 0.0 || determinant > 0.0 ) )
	{
		static const float identity[16] =
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		memcpy( out, identity, sizeof( identity ) );
		return;
	}

	double reciprocal = 1.0 / roundedDeterminant;
	float scaledFirstColumn[4];
	scaledFirstColumn[0] = (float)( reciprocal * matrix[0] );
	scaledFirstColumn[1] = (float)( reciprocal * matrix[4] );
	scaledFirstColumn[2] = (float)( reciprocal * matrix[8] );
	scaledFirstColumn[3] = (float)( reciprocal * matrix[12] );

	float inverse[16];
	inverse[0] = (float)( cofactors[0] * reciprocal );
	inverse[1] = (float)( cofactors[1] * reciprocal );
	inverse[2] = (float)( cofactors[2] * reciprocal );
	inverse[3] = 0.0f;
	inverse[4] = (float)( (double)matrix[6] * scaledFirstColumn[2] -
		(double)matrix[10] * scaledFirstColumn[1] );
	inverse[5] = (float)( (double)matrix[10] * scaledFirstColumn[0] -
		(double)matrix[2] * scaledFirstColumn[2] );
	inverse[6] = (float)( (double)matrix[2] * scaledFirstColumn[1] -
		(double)matrix[6] * scaledFirstColumn[0] );
	inverse[7] = 0.0f;
	inverse[8] = (float)( (double)matrix[9] * scaledFirstColumn[1] -
		(double)matrix[5] * scaledFirstColumn[2] );

	float negativeInverseZY = (float)( (double)matrix[9] * scaledFirstColumn[0] -
		(double)matrix[1] * scaledFirstColumn[2] );
	inverse[9] = -negativeInverseZY;
	inverse[10] = (float)( (double)matrix[5] * scaledFirstColumn[0] -
		(double)matrix[1] * scaledFirstColumn[1] );
	inverse[11] = 0.0f;

	float translationMinorXY = (float)( (double)matrix[13] * scaledFirstColumn[1] -
		(double)matrix[5] * scaledFirstColumn[3] );
	float translationMinorXZ = (float)( (double)matrix[13] * scaledFirstColumn[2] -
		(double)matrix[9] * scaledFirstColumn[3] );
	double translationMinorYX = (double)matrix[1] * scaledFirstColumn[3] -
		(double)matrix[13] * scaledFirstColumn[0];

	inverse[12] = (float)-( ( (double)matrix[6] * translationMinorXZ -
		(double)matrix[10] * translationMinorXY ) + (double)matrix[14] * inverse[8] );
	inverse[13] = (float)( ( (double)matrix[2] * translationMinorXZ +
		(double)matrix[14] * negativeInverseZY ) + matrix[10] * translationMinorYX );
	inverse[14] = (float)-( ( (double)matrix[2] * translationMinorXY +
		matrix[6] * translationMinorYX ) + (double)matrix[14] * inverse[10] );
	inverse[15] = 1.0f;

	memcpy( out, inverse, sizeof( inverse ) );
}

void __cdecl Matrix_InverseGeneral( const float matrix[16], float out[16] )
{
	double reciprocalA = 1.0 / ( (double)matrix[0] * matrix[5] - (double)matrix[1] * matrix[4] );
	double inverseA00 = reciprocalA * matrix[5];
	double inverseA01 = -( reciprocalA * matrix[1] );
	double inverseA10 = -( reciprocalA * matrix[4] );
	double inverseA11 = matrix[0] * reciprocalA;
	float inverseA[4] =
	{
		(float)inverseA00, (float)inverseA01,
		(float)inverseA10, (float)inverseA11
	};

	float productAB[4];
	productAB[0] = (float)( inverseA00 * matrix[2] + inverseA01 * matrix[6] );
	productAB[2] = (float)( inverseA10 * matrix[2] + inverseA11 * matrix[6] );
	productAB[1] = (float)( (double)inverseA[1] * matrix[7] + (double)inverseA[0] * matrix[3] );
	productAB[3] = (float)( (double)inverseA[3] * matrix[7] + (double)inverseA[2] * matrix[3] );

	float productCA[4];
	productCA[0] = (float)( (double)inverseA[2] * matrix[9] + (double)inverseA[0] * matrix[8] );
	productCA[2] = (float)( (double)inverseA[2] * matrix[13] + (double)inverseA[0] * matrix[12] );
	productCA[1] = (float)( (double)inverseA[3] * matrix[9] + (double)inverseA[1] * matrix[8] );
	productCA[3] = (float)( (double)inverseA[3] * matrix[13] + (double)inverseA[1] * matrix[12] );

	double schur00 = ( (double)productCA[0] * matrix[2] + (double)productCA[1] * matrix[6] ) - matrix[10];
	float schur10 = (float)( ( (double)productCA[2] * matrix[2] +
		(double)productCA[3] * matrix[6] ) - matrix[14] );
	double schur01 = ( (double)productCA[1] * matrix[7] + (double)productCA[0] * matrix[3] ) - matrix[11];
	float roundedSchur01 = (float)schur01;
	float schur11 = (float)( ( (double)productCA[3] * matrix[7] +
		(double)productCA[2] * matrix[3] ) - matrix[15] );

	double reciprocalSchur = 1.0 / ( schur00 * schur11 - schur01 * schur10 );
	float roundedReciprocalSchur = (float)reciprocalSchur;
	double inverseSchur00 = (double)roundedReciprocalSchur * schur11;
	double inverseSchur01 = -( (double)roundedReciprocalSchur * roundedSchur01 );
	double inverseSchur10 = -( reciprocalSchur * schur10 );
	double inverseSchur11 = roundedReciprocalSchur * schur00;
	float inverseSchur[4] =
	{
		(float)inverseSchur00, (float)inverseSchur01,
		(float)inverseSchur10, (float)inverseSchur11
	};

	out[8] = (float)( inverseSchur00 * productCA[0] + inverseSchur01 * productCA[2] );
	out[12] = (float)( inverseSchur10 * productCA[0] + inverseSchur11 * productCA[2] );
	out[9] = (float)( (double)inverseSchur[0] * productCA[1] + (double)inverseSchur[1] * productCA[3] );
	out[13] = (float)( (double)inverseSchur[2] * productCA[1] + (double)inverseSchur[3] * productCA[3] );

	float productRight00 = (float)( (double)inverseSchur[0] * productAB[0] +
		(double)inverseSchur[2] * productAB[1] );
	float productRight10 = (float)( (double)inverseSchur[0] * productAB[2] +
		(double)inverseSchur[2] * productAB[3] );
	out[2] = productRight00;
	out[6] = productRight10;

	double productRight01 = (double)inverseSchur[1] * productAB[0] +
		(double)inverseSchur[3] * productAB[1];
	float roundedProductRight01 = (float)productRight01;
	out[3] = roundedProductRight01;
	float productRight11 = (float)( (double)inverseSchur[1] * productAB[2] +
		(double)inverseSchur[3] * productAB[3] );
	out[7] = productRight11;

	out[0] = (float)( inverseA[0] - ( productCA[2] * productRight01 +
		(double)productCA[0] * productRight00 ) );
	out[4] = (float)( inverseA[2] - ( (double)productCA[2] * productRight11 +
		(double)productCA[0] * productRight10 ) );
	out[10] = -inverseSchur[0];
	out[1] = (float)( inverseA[1] - ( (double)productCA[3] * roundedProductRight01 +
		(double)productCA[1] * productRight00 ) );
	out[14] = -inverseSchur[2];
	out[11] = -inverseSchur[1];
	out[5] = (float)( inverseA[3] - ( (double)productCA[3] * productRight11 +
		(double)productCA[1] * productRight10 ) );
	out[15] = -inverseSchur[3];
}
