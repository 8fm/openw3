/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/renderFragment.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

//Shift color using HSL transform
class CMaterialBlockShiftColor : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockShiftColor, CMaterialBlock, "Deprecated", "Color shift (Simple)" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk color = CompileInput( compiler, CNAME( Color ), shaderTarget );
		CodeChunk colorShift1 = PS_CONST_FLOAT44( PSC_ColorOne );
		CodeChunk colorShift2 = PS_CONST_FLOAT44( PSC_ColorTwo );

		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget ).w();
		
		CodeChunk identityTransform( "float4x4( 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 )", true );

		CodeChunk colorTransform = CodeChunk::Printf( false,
			"( ( %s < 0.333 ) ? (%s) : ( (%s > 0.666) ? (%s) : (%s) ) )" ,
			mask.AsChar(), colorShift1.AsChar(), mask.AsChar(), colorShift2.AsChar(), identityTransform.AsChar() );
		
		CodeChunk newColor = Mul( color, colorTransform );

		CodeChunk blend = Abs( 0.5f - mask ) * 2.0f;
		return Lerp( color, newColor, blend );
	}

	virtual Uint32 CalcRenderingFragmentParamMask() const
	{
		return RFMP_ColorShiftMatrices;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockShiftColor )
	PARENT_CLASS( CMaterialBlock )
END_CLASS_RTTI()


IMPLEMENT_ENGINE_CLASS( CMaterialBlockShiftColor );

#endif