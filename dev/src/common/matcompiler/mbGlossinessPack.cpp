/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

class CMaterialBlockGlossinessPack : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockGlossinessPack, CMaterialBlock, "Deprecated", "Pack/Unpack Glossiness" );

public:
	Bool	m_unpack;

public:
	CMaterialBlockGlossinessPack()
	{
		m_unpack = false;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

	virtual String GetCaption() const
	{
		if (m_unpack)
		{
			return TXT("Unpack Glossiness");
		}
		else
		{
			return TXT("Pack Glossiness");
		}
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		const Float glossiness_pack_max_bits		= 12; // max glossiness value is (1 << glossiness_pack_max_bits)
		
		CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, 1.0f ).x();

		if ( m_unpack )
		{
			const Float unpack_scale	= glossiness_pack_max_bits - 1;
			const Float unpack_bias		= 1;

			CodeChunk b = SHADER_VAR_FLOAT( Exp2( a * unpack_scale + unpack_bias ), shaderTarget );
			return Float4( b, b, b, b );
		}

		else
		{
			const Float pack_scale		= 1.f / (glossiness_pack_max_bits - 1);
			const Float pack_bias		= -pack_scale;

			CodeChunk b = SHADER_VAR_FLOAT( Log2( a ) * pack_scale + pack_bias, shaderTarget );
			return Float4( b, b, b, b );
		}
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockGlossinessPack)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_unpack, TXT("Check to unpack") );
END_CLASS_RTTI()


IMPLEMENT_ENGINE_CLASS( CMaterialBlockGlossinessPack );

#endif