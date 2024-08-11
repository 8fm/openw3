/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Base block that implements lighting function
class CMaterialBlockLighting : public CMaterialBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialBlockLighting, CMaterialBlock );

public:
};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialBlockLighting );
	PARENT_CLASS( CMaterialBlock );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CMaterialBlockLighting );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace CodeChunkOperators;

/// Phong lighting model
class CMaterialBlockLightingPhong : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockLightingPhong, CMaterialBlock, "Deprecated", "Phong" );	

public:
	CMaterialBlockLightingPhong()
	{
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

	virtual Color GetTitleColor() const
	{
		return Color::RED;
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		return  CompileInput( compiler, CNAME( Diffuse ), shaderTarget, 0.5f );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockLightingPhong)
	PARENT_CLASS(CMaterialBlock)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockLightingPhong );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif