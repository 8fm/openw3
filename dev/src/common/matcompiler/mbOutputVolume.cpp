/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputVolume.h"
#include "../matcompiler//materialShaderConstants.h"
#include "../engine/renderFragment.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputVolume );

using namespace CodeChunkOperators;

CMaterialBlockOutputVolume::CMaterialBlockOutputVolume()
	: m_isTwoSided( false )
	, m_noDepthWrite( false )
	, m_inputColorLinear( true )
	, m_maskThreshold( 0.33f )
	, m_blendMode( RBM_None )
	, m_checkRefractionDepth( false )	
{
}

Bool CMaterialBlockOutputVolume::IsTwoSided() const
{
	return m_isTwoSided;
}

Bool CMaterialBlockOutputVolume::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputVolume::GetBlendMode() const
{
	return m_blendMode;
}

Bool CMaterialBlockOutputVolume::IsEmissive() const
{	
	return false;
}

Bool CMaterialBlockOutputVolume::IsAccumulativelyRefracted() const
{	
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputVolume::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputVolume::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Value ) ) );	
}

#endif

void CMaterialBlockOutputVolume::Compile( CMaterialBlockCompiler& compiler ) const
{
	const ERenderingPass pass = compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	// include some utils functions			
	compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

	ASSERT( pass == RP_NoLighting, TXT("Volume materials don't support any other pass than No Lighting") );

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;
	
	// Optional clipping
	compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );

	// Simple pass
	if ( compiler.GetPS().CompileRenderPassSimple( pass ) )
	{
		// Root chunks does not return value
		return;
	}
	
	CodeChunk inputValue = CompileInput( compiler, CNAME( Value ), shaderTarget, Vector::ZEROS );
	CodeChunk resultColor;
	if ( m_isWaterBlended )
	{
		resultColor = inputValue;
	}
	else
	{
		CodeChunk normDist = Saturate( inputValue.x() * CodeChunk( "interiorRangeParams.w", false ) );
		resultColor = Float4( normDist, normDist, normDist, normDist );	
	}

	compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", resultColor );	
}

void CMaterialBlockOutputVolume::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// should not be rendered with anything else
	passes.PushBackUnique( RP_NoLighting );
}

#endif
