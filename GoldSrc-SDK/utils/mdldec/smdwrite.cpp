// smdwrite.cpp -- reference meshes and animation source

#include "mdldec.h"

#include <math.h>
#include <string.h>

// guard helper: true when animvalue element [index] lies wholly inside [buf, bufEnd).
// Used to keep WriteFrame's animation walk inside its loaded buffer; returns true for
// every in-bounds read a valid model performs, so valid output is unchanged.
static bool AnimValueInBounds( const mstudioanimvalue_t* values, int index,
		const byte* buf, const byte* bufEnd )
{
	const byte* p = (const byte*)( values + index );
	return p >= buf && p + sizeof( mstudioanimvalue_t ) <= bufEnd;
}

void CMDLDecompiler::WriteReference()
{
	studiohdr_t* header = (studiohdr_t*)m_pModelData;
	const byte* bufEnd = m_pModelData + m_nModelSize;
	mstudiobone_t* bones = (mstudiobone_t*)( m_pModelData + header->boneindex );
	float quaternion[MAXSTUDIOBONES][4];
	matrix3x4_t local;

	for ( int i = 0; i < header->numbones; i++ )
	{
		// guard: numbones and bone->parent come from the file; quaternion and
		// m_boneTransform hold MAXSTUDIOBONES entries, so cap the loop at that and keep
		// the bone inside the loaded buffer. An out-of-range parent is treated as a root
		// so m_boneTransform[parent] is never indexed past the array (doctor.mdl, a v9
		// alpha model read with the v10 layout, has garbage bone parents).
		if ( i >= MAXSTUDIOBONES || (const byte*)&bones[i] < m_pModelData
			|| (const byte*)&bones[i + 1] > bufEnd )
			break;
		AngleQuaternion( &bones[i].value[3], quaternion[i] );
		QuaternionMatrix( quaternion[i], local );
		for ( int axis = 0; axis < 3; axis++ )
			local[axis][3] = bones[i].value[axis];

		if ( bones[i].parent < 0 || bones[i].parent >= MAXSTUDIOBONES )
			memcpy( m_boneTransform[i], local, sizeof( local ) );
		else
			R_ConcatTransforms( m_boneTransform[bones[i].parent], local, m_boneTransform[i] );
	}

	mstudiobodyparts_t* bodyparts = (mstudiobodyparts_t*)( m_pModelData + header->bodypartindex );
	for ( int i = 0; i < header->numbodyparts; i++ )
	{
		// guard: bodypartindex/numbodyparts come from the file; stop when the bodypart
		// descriptor leaves the loaded buffer.
		if ( (const byte*)&bodyparts[i] < m_pModelData || (const byte*)&bodyparts[i + 1] > bufEnd )
			break;
		mstudiomodel_t* models = (mstudiomodel_t*)( m_pModelData + bodyparts[i].modelindex );
		for ( int j = 0; j < bodyparts[i].nummodels; j++ )
		{
			// guard: modelindex/nummodels come from the file; stop when the model
			// descriptor leaves the loaded buffer.
			if ( (const byte*)&models[j] < m_pModelData || (const byte*)&models[j + 1] > bufEnd )
				break;
			mstudiomodel_t* model = &models[j];
			if ( strncmp( model->name, "blank", 5 ) == 0 )
				continue;

			char name[68];
			char filename[268];
			ExtractFileBase( model->name, name, sizeof( name ) );
			// guard: output path (command line) plus a name from the file can exceed filename[268]; the
			// native sprintf overruns it. Take the existing "cannot write" path when it does not fit.
			int written = _snprintf( filename, sizeof( filename ), "%s%s.smd", m_szOutputPath, name );
			filename[sizeof( filename ) - 1] = 0;
			if ( written < 0 || written >= (int)sizeof( filename ) )
			{
				Log( 2, "ERROR: Cannot write %s\r\n", filename );
				return;
			}
			FILE* file = fopen( filename, "w" );
			if ( file == NULL )
			{
				Log( 2, "ERROR: Cannot write %s\r\n", filename );
				return;
			}

			fprintf( file, "version 1\n" );
			WriteNodes( file );
			fprintf( file, "skeleton\n" );
			fprintf( file, "time 0\n" );
			for ( int bone = 0; bone < header->numbones; bone++ )
			{
				// guard: keep the skeleton read inside the loaded buffer.
				if ( (const byte*)&bones[bone + 1] > bufEnd )
					break;
				fprintf( file, "%3i %f %f %f", bone,
						FloatForPrint( bones[bone].value[0] ), FloatForPrint( bones[bone].value[1] ),
						FloatForPrint( bones[bone].value[2] ) );
				fprintf( file, " %f %f %f\n",
						FloatForPrint( bones[bone].value[3] ), FloatForPrint( bones[bone].value[4] ),
						FloatForPrint( bones[bone].value[5] ) );
			}
			fprintf( file, "end\n" );
			fprintf( file, "triangles\n" );

			mstudiomesh_t* meshes = (mstudiomesh_t*)( m_pModelData + model->meshindex );
			for ( int mesh = 0; mesh < model->nummesh; mesh++ )
			{
				// guard: nummesh/meshindex come from the file; stop when the mesh
				// descriptor leaves the loaded buffer.
				if ( (const byte*)&meshes[mesh] < m_pModelData || (const byte*)&meshes[mesh + 1] > bufEnd )
					break;
				studiohdr_t* textureHeader = header->numtextures ? header : (studiohdr_t*)m_pTextureData;
				mstudiotexture_t* textures = (mstudiotexture_t*)( (byte*)textureHeader + textureHeader->textureindex );
				// guard: skinref indexes the textures array; skip a mesh that references a
				// texture outside it.
				if ( meshes[mesh].skinref < 0 || meshes[mesh].skinref >= textureHeader->numtextures )
					continue;
				mstudiotexture_t* texture = &textures[meshes[mesh].skinref];
				short* commands = (short*)( m_pModelData + meshes[mesh].triindex );
				int count;
				// guard: triindex and the per-strip counts come from the file; only read a
				// command word while it lies inside the loaded buffer (a bad triindex or a
				// missing 0 terminator would otherwise walk past it).
				while ( (const byte*)commands >= m_pModelData
					&& (const byte*)( commands + 1 ) <= bufEnd && ( count = *commands++ ) != 0 )
				{
					if ( count > 0 )
					{
						for ( int vertex = 2; vertex < count; vertex++ )
						{
							if ( vertex % 2 )
								WriteTriangle( file, model, texture, commands + 4 * ( vertex - 2 ),
										commands + 4 * ( vertex - 1 ), commands + 4 * vertex );
								else
								WriteTriangle( file, model, texture, commands + 4 * ( vertex - 2 ),
										commands + 4 * vertex, commands + 4 * ( vertex - 1 ) );
							}
						commands += count * 4;
					}
					else
					{
						count = -count;
						for ( int vertex = 2; vertex < count; vertex++ )
							WriteTriangle( file, model, texture, commands, commands + 4 * vertex,
									commands + 4 * ( vertex - 1 ) );
						commands += count * 4;
					}
				}
			}
			fprintf( file, "end\n" );
			Log( 0, "Reference: %s\r\n", filename );
			fclose( file );
		}
	}
}

void CMDLDecompiler::WriteTriangle( FILE* file, const mstudiomodel_t* model,
		const mstudiotexture_t* texture, const short* first, const short* second, const short* third )
{
	const vec3_t* vertices = (const vec3_t*)( m_pModelData + model->vertindex );
	const vec3_t* normals = (const vec3_t*)( m_pModelData + model->normindex );
	const byte* bones = m_pModelData + model->vertinfoindex;
	const byte* bufEnd = m_pModelData + m_nModelSize;
	const short* triangle[3] = { first, second, third };

	fprintf( file, "%s\n", texture->name );
	float sscale = (float)( 1.0 / (float)texture->width );
	float tscale = (float)( 1.0 / (float)texture->height );
	for ( int i = 0; i < 3; i++ )
	{
		const short* vertex = triangle[i];
		// guard: the triangle command words and the vertex/normal/bone indices they hold
		// come from the file; skip a vertex whose command words, referenced vertex or
		// normal, or bone index fall outside the loaded buffer or the fixed
		// m_boneTransform array.
		if ( (const byte*)vertex < m_pModelData || (const byte*)( vertex + 4 ) > bufEnd
			|| vertex[0] < 0 || vertex[0] >= model->numverts
			|| vertex[1] < 0 || vertex[1] >= model->numnorms )
			continue;
		int bone = bones[vertex[0]];
		if ( bone < 0 || bone >= MAXSTUDIOBONES )
			continue;
		vec3_t position;
		vec3_t normal;
		VectorTransform( vertices[vertex[0]], m_boneTransform[bone], position );
		VectorRotate( normals[vertex[1]], m_boneTransform[bone], normal );
		VectorNormalize( normal );
		fprintf( file, "%3i %f %f %f %f %f %f %f %f\n", bone,
				FloatForPrint( position[0] ), FloatForPrint( position[1] ), FloatForPrint( position[2] ),
				FloatForPrint( normal[0] ), FloatForPrint( normal[1] ), FloatForPrint( normal[2] ),
				FloatForPrint( (double)(float)vertex[2] * sscale ),
				FloatForPrint( 1.0 - (double)(float)vertex[3] * tscale ) );
	}
}

void CMDLDecompiler::WriteSequences()
{
	studiohdr_t* header = (studiohdr_t*)m_pModelData;
	const byte* bufEnd = m_pModelData + m_nModelSize;
	mstudioseqdesc_t* sequences = (mstudioseqdesc_t*)( m_pModelData + header->seqindex );
	mstudioseqgroup_t* groups = (mstudioseqgroup_t*)( m_pModelData + header->seqgroupindex );

	for ( int i = 0; i < header->numseq; i++ )
	{
		// guard: seqindex/numseq come from the file; stop when the seqdesc leaves the
		// loaded buffer.
		if ( (const byte*)&sequences[i] < m_pModelData || (const byte*)&sequences[i + 1] > bufEnd )
			break;
		mstudioseqdesc_t* sequence = &sequences[i];
		// guard: seqgroup indexes m_pSequenceHeader/m_pSequenceData (MAX_SEQUENCE_FILES
		// entries); skip a sequence whose group is out of range (doctor.mdl seq 6 claims
		// seqgroup 387,389,206).
		if ( sequence->seqgroup < 0 || sequence->seqgroup >= MAX_SEQUENCE_FILES )
			continue;
		const byte* animBuf;
		const byte* animBufEnd;
		if ( sequence->seqgroup != 0 )
		{
			animBuf = (const byte*)m_pSequenceData[sequence->seqgroup];
			animBufEnd = animBuf + m_nSequenceSize[sequence->seqgroup];
		}
		else
		{
			// guard: groups[0] (for seqgroup 0) must itself be inside the buffer before
			// its offset is read.
			if ( (const byte*)&groups[1] > bufEnd )
				continue;
			animBuf = m_pModelData;
			animBufEnd = bufEnd;
		}
		for ( int blend = 0; blend < sequence->numblends; blend++ )
		{
			const byte* data;
			if ( sequence->seqgroup != 0 )
				data = (const byte*)m_pSequenceHeader[sequence->seqgroup] + sequence->animindex;
			else
				data = m_pModelData + groups[sequence->seqgroup].unused2 + sequence->animindex;
			const mstudioanim_t* animation = (const mstudioanim_t*)data + blend * header->numbones;
			// guard: numblends/animindex come from the file; stop when this blend's
			// animation array of numbones entries would leave its buffer (doctor.mdl
			// seq 6 claims 387,389,207 blends).
			if ( (const byte*)animation < animBuf
				|| (const byte*)( animation + header->numbones ) > animBufEnd )
				break;

			char filename[268];
			// guard: output path (command line) plus a name from the file can exceed filename[268]; the
			// native sprintf overruns it. Take the existing "cannot write" path when it does not fit.
			int written;
			if ( sequence->numblends == 1 )
				written = _snprintf( filename, sizeof( filename ), "%s%s.smd", m_szOutputPath, sequence->label );
			else
				written = _snprintf( filename, sizeof( filename ), "%s%s_blend%i.smd", m_szOutputPath, sequence->label, blend + 1 );
			filename[sizeof( filename ) - 1] = 0;
			if ( written < 0 || written >= (int)sizeof( filename ) )
			{
				Log( 2, "ERROR: Cannot write %s\r\n", filename );
				return;
			}
			FILE* file = fopen( filename, "w" );
			if ( file == NULL )
			{
				Log( 2, "ERROR: Cannot write %s\r\n", filename );
				return;
			}

			fprintf( file, "version 1\n" );
			WriteNodes( file );
			fprintf( file, "skeleton\n" );
			for ( int frame = 0; frame < sequence->numframes; frame++ )
				WriteFrame( file, frame, animation, sequence );
			fprintf( file, "end\n" );
			fclose( file );
			Log( 0, "Sequence: %s\r\n", filename );
		}
	}
}

void CMDLDecompiler::WriteFrame( FILE* file, int frame, const mstudioanim_t* animation,
		const mstudioseqdesc_t* sequence )
{
	studiohdr_t* header = (studiohdr_t*)m_pModelData;
	const mstudiobone_t* bone = (const mstudiobone_t*)( m_pModelData + header->boneindex );
	float fraction = 0.0f;
	// Animation data buffer for this sequence (the model buffer for group 0, else the
	// loaded sequence-group file); the animvalue walks below are kept inside it.
	const byte* animBuf;
	const byte* animBufEnd;
	if ( sequence->seqgroup > 0 && sequence->seqgroup < MAX_SEQUENCE_FILES )
	{
		animBuf = (const byte*)m_pSequenceData[sequence->seqgroup];
		animBufEnd = animBuf + m_nSequenceSize[sequence->seqgroup];
	}
	else
	{
		animBuf = m_pModelData;
		animBufEnd = m_pModelData + m_nModelSize;
	}

	fprintf( file, "time %i\n", frame );
	for ( int i = 0; i < header->numbones; i++, bone++, animation++ )
	{
		vec3_t position;
		vec3_t angles;
		vec3_t nextAngles;
		for ( int axis = 0; axis < 3; axis++ )
		{
			position[axis] = bone->value[axis];
			if ( animation->offset[axis] == 0 )
				continue;

			const mstudioanimvalue_t* values = (const mstudioanimvalue_t*)(
					(const byte*)animation + animation->offset[axis] );
			int k = frame;
			// guard: animation offsets/counts come from the file; keep the walk inside
			// its buffer (a bad offset.total would otherwise run off it). A span that
			// leaves the buffer keeps the bone's default position.
			while ( AnimValueInBounds( values, 0, animBuf, animBufEnd ) && values->num.total <= k )
			{
				k -= values->num.total;
				values += values->num.valid + 1;
			}
			if ( !AnimValueInBounds( values, 0, animBuf, animBufEnd ) )
				continue;

			if ( values->num.valid <= k )
			{
				// guard: values[valid] feeds the output; keep the default if it is out of
				// the buffer (valid models always read it in bounds).
				if ( !AnimValueInBounds( values, values->num.valid, animBuf, animBufEnd ) )
					continue;
				if ( values->num.total > k + 1 )
				{
					float sample = (float)values[values->num.valid].value;
					position[axis] = (float)( (double)sample * bone->scale[axis] + position[axis] );
				}
				else
				{
					// guard: the "next" sample is multiplied by fraction (always 0 here),
					// so its value never affects output; read 0 if it is out of the buffer
					// instead of over-reading or dropping the in-bounds sample.
					float next = AnimValueInBounds( values, values->num.valid + 2, animBuf, animBufEnd )
						? (float)values[values->num.valid + 2].value : 0.0f;
					position[axis] = (float)( ( ( 1.0 - fraction ) * values[values->num.valid].value
							+ (double)fraction * next ) * bone->scale[axis] + position[axis] );
				}
			}
			else
			{
				// guard: values[k + 1] feeds the output.
				if ( !AnimValueInBounds( values, k + 1, animBuf, animBufEnd ) )
					continue;
				if ( values->num.valid <= k + 1 )
				{
					float sample = (float)values[k + 1].value;
					position[axis] = (float)( (double)sample * bone->scale[axis] + position[axis] );
				}
				else
				{
					// guard: "next" is multiplied by 0 (see above); read 0 if out of buffer.
					float next = AnimValueInBounds( values, k + 2, animBuf, animBufEnd )
						? (float)values[k + 2].value : 0.0f;
					position[axis] = (float)( ( ( 1.0 - fraction ) * values[k + 1].value
							+ (double)fraction * next ) * bone->scale[axis] + position[axis] );
				}
			}
		}

		for ( int axis = 0; axis < 3; axis++ )
		{
			if ( animation->offset[axis + 3] == 0 )
			{
				angles[axis] = bone->value[axis + 3];
				nextAngles[axis] = angles[axis];
				continue;
			}

			const mstudioanimvalue_t* values = (const mstudioanimvalue_t*)(
					(const byte*)animation + animation->offset[axis + 3] );
			int k = frame;
			// guard: as in the position walk above, keep every animvalue read inside the
			// animation buffer; a span that leaves it keeps the bone's default angle.
			while ( AnimValueInBounds( values, 0, animBuf, animBufEnd ) && values->num.total <= k )
			{
				k -= values->num.total;
				values += values->num.valid + 1;
			}
			if ( !AnimValueInBounds( values, 0, animBuf, animBufEnd ) )
			{
				angles[axis] = bone->value[axis + 3];
				nextAngles[axis] = angles[axis];
				continue;
			}

			if ( values->num.valid <= k )
			{
				if ( !AnimValueInBounds( values, values->num.valid, animBuf, animBufEnd ) )
				{
					angles[axis] = bone->value[axis + 3];
					nextAngles[axis] = angles[axis];
					continue;
				}
				angles[axis] = (float)values[values->num.valid].value;
				if ( values->num.total <= k + 1 )
				{
					if ( !AnimValueInBounds( values, values->num.valid + 2, animBuf, animBufEnd ) )
						nextAngles[axis] = angles[axis];
					else
						nextAngles[axis] = (float)values[values->num.valid + 2].value;
				}
				else
					nextAngles[axis] = angles[axis];
			}
			else
			{
				if ( !AnimValueInBounds( values, k + 1, animBuf, animBufEnd ) )
				{
					angles[axis] = bone->value[axis + 3];
					nextAngles[axis] = angles[axis];
					continue;
				}
				angles[axis] = (float)values[k + 1].value;
				if ( values->num.valid <= k + 1 )
				{
					if ( values->num.total <= k + 1 )
					{
						if ( !AnimValueInBounds( values, values->num.valid + 2, animBuf, animBufEnd ) )
							nextAngles[axis] = angles[axis];
						else
							nextAngles[axis] = (float)values[values->num.valid + 2].value;
					}
					else
						nextAngles[axis] = angles[axis];
				}
				else
				{
					if ( !AnimValueInBounds( values, k + 2, animBuf, animBufEnd ) )
						nextAngles[axis] = angles[axis];
					else
						nextAngles[axis] = (float)values[k + 2].value;
				}
			}
			angles[axis] = (float)( (double)angles[axis] * bone->scale[axis + 3] + bone->value[axis + 3] );
			nextAngles[axis] = (float)( (double)nextAngles[axis] * bone->scale[axis + 3] + bone->value[axis + 3] );
		}

		if ( bone->parent == -1 )
		{
			vec3_t movement;
			for ( int axis = 0; axis < 3; axis++ )
			{
				movement[axis] = (float)( (double)frame * 1.0 / sequence->numframes * sequence->linearmovement[axis] );
				position[axis] = (float)( (double)position[axis] + movement[axis] );
			}

			float cosine = (float)cos( (double)-1.5707964f );
			float sine = (float)sin( (double)-1.5707964f );
			float x = position[0];
			position[0] = (float)( (double)cosine * position[0] - (double)sine * position[1] );
			position[1] = (float)( (double)sine * x + (double)cosine * position[1] );
			angles[2] = (float)( (double)angles[2] + -1.5707964f );
			for ( int axis = 0; axis < 3; axis++ )
			{
				while ( angles[axis] >= M_PI )
					angles[axis] = (float)( angles[axis] - 2.0 * M_PI );
				while ( angles[axis] < -M_PI )
					angles[axis] = (float)( angles[axis] + 2.0 * M_PI );
			}
		}
		fprintf( file, "%3i   ", i );
		fprintf( file, "%f %f %f %f %f %f\n",
				FloatForPrint( position[0] ), FloatForPrint( position[1] ), FloatForPrint( position[2] ),
				FloatForPrint( angles[0] ), FloatForPrint( angles[1] ), FloatForPrint( angles[2] ) );
	}
}

void CMDLDecompiler::WriteNodes( FILE* file )
{
	studiohdr_t* header = (studiohdr_t*)m_pModelData;
	const mstudiobone_t* bones = (const mstudiobone_t*)( m_pModelData + header->boneindex );
	const byte* bufEnd = m_pModelData + m_nModelSize;
	fprintf( file, "nodes\n" );
	for ( int i = 0; i < header->numbones; i++ )
	{
		// guard: numbones/boneindex come from the file; stop when the bone leaves the
		// loaded buffer.
		if ( (const byte*)&bones[i + 1] > bufEnd )
			break;
		fprintf( file, "%3i \"%s\" %i\n", i, bones[i].name, bones[i].parent );
	}
	fprintf( file, "end\n" );
}
