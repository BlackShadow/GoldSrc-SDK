// Half-Life model decompiler.

#ifndef MDLDEC_H
#define MDLDEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

typedef unsigned char byte;
typedef int int32;
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef float matrix3x4_t[3][4];

#include "../../engine/studio.h"

#define MAX_SEQUENCE_FILES 32

class CMDLDecompiler
{
public:
	CMDLDecompiler();
	virtual ~CMDLDecompiler();

	//
	// mdldec.cpp
	//
	bool LoadModel(const char *filename);
	void RenameSequences();
	void NameBones();
	// guard: the destination sizes are not in the original signatures; they bound the copies (see mdldec.cpp).
	void ExtractFileBase(const char *path, char *name, size_t nameSize);
	void ExtractFilePath(const char *path, char *directory, size_t directorySize);
	void Log(int level, const char *format, ...);

	//
	// write.cpp
	//
	void WriteQC();
	void WriteTextureModes();
	void WriteTextureGroups();
	void WriteBodyParts();
	void WriteAttachments();
	void WriteControllers();
	void WriteHitboxes();
	void WriteQCSequences();
	int GetActivityName(int activity, char *name);
	void GetMotionTypeName(int type, char *name, bool multiple);

	//
	// smdwrite.cpp
	//
	void WriteReference();
	void WriteTriangle(FILE *file, const mstudiomodel_t *model, const mstudiotexture_t *texture,
		const short *first, const short *second, const short *third);
	void WriteSequences();
	void WriteFrame(FILE *file, int frame, const mstudioanim_t *animation, const mstudioseqdesc_t *sequence);
	void WriteNodes(FILE *file);

	//
	// bmpwrite.cpp
	//
	void WriteTextures();
	void WriteBMP(const byte *pixels, const byte *palette, const char *name, const mstudiotexture_t *texture);

	char m_szOutputPath[261];
	char m_szModelName[65];
	FILE *m_pQCFile;
	byte *m_pModelData;
	int m_nModelSize;
	studiohdr_t *m_pStudioHeader;
	byte *m_pSequenceData[MAX_SEQUENCE_FILES];
	int m_nSequenceSize[MAX_SEQUENCE_FILES];
	studioseqhdr_t *m_pSequenceHeader[MAX_SEQUENCE_FILES];
	byte *m_pTextureData;
	int m_nTextureSize;
	studiohdr_t *m_pTextureHeader;
	matrix3x4_t m_boneTransform[MAXSTUDIOBONES];
	matrix3x4_t m_boneLocal[MAXSTUDIOBONES];
};

// The original stdio treats an exact negative zero as positive zero.
inline double FloatForPrint(double value)
{
	return value == 0.0 ? 0.0 : value;
}

//
// mathlib.cpp
//
float VectorNormalize(vec3_t vector);
void R_ConcatTransforms(const matrix3x4_t first, const matrix3x4_t second, matrix3x4_t result);
void VectorRotate(const vec3_t vector, const matrix3x4_t matrix, vec3_t result);
void VectorTransform(const vec3_t vector, const matrix3x4_t matrix, vec3_t result);
void AngleQuaternion(const vec3_t angles, vec4_t quaternion);
void QuaternionMatrix(const vec4_t quaternion, matrix3x4_t matrix);

#endif
