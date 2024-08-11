/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION


/// Return 0 if sampled pixel WorldPosition is within InteriorVolume, 1 otherwise
class CMaterialBlockVolumeBlend : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVolumeBlend, CMaterialBlock, "System Samplers", "Volume Blend" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( X ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Value ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		using namespace CodeChunkOperators;

		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

		CodeChunk result = Float4 ( 1.f, 1.f, 1.f, 1.f );

		CodeChunk coord	= compiler.GetPS().Data( "ScreenUV" ).xy();

		CodeChunk x	 = CompileInput( compiler, CNAME( X ), shaderTarget, Float4 (1.0f, 1.0f, 1.0f, 1.0f) );

		CodeChunk worldPosition;		
		if( HasInput( CNAME( Value ) ) )
		{
			worldPosition = CompileInput( compiler, CNAME( Value ), shaderTarget, Float4 (0.0f, 0.0f, 0.0f, 0.0f) );
		}	
		else
		{
			worldPosition = SHADER_DATA( "WorldPosition", shaderTarget );
		}

		CodeChunk param = compiler.GetPS().AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float %s = CalculateVolumeCut( %s, %s );", param.AsChar(), coord.AsChar(), worldPosition.AsChar() );
		compiler.GetPS().Statement( call );
		
		result = param * x;

		return result;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockVolumeBlend , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVolumeBlend );

#endif