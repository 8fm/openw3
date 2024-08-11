/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "materialSampler.h"
#include "materialBlockCompiler.h"

IMPLEMENT_RTTI_ENUM( ETextureFilteringMin );
IMPLEMENT_RTTI_ENUM( ETextureFilteringMag );
IMPLEMENT_RTTI_ENUM( ETextureFilteringMip );
IMPLEMENT_RTTI_ENUM( ETextureAddressing );
IMPLEMENT_RTTI_ENUM( ETextureComparisonFunction );

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSampler );

CMaterialBlockSampler::CMaterialBlockSampler()
	: m_addressU( TA_Wrap )
	, m_addressV( TA_Wrap )
	, m_addressW( TA_Wrap )
	, m_filterMin( TFMin_Anisotropic )
	, m_filterMag( TFMag_Linear )
	, m_filterMip( TFMip_Linear )
	, m_comparisonFunction( TCF_None )
{
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialBlockSampler::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket /*= NULL*/ ) const
{
	// let the overriding classes do the job
	return CodeChunk::EMPTY;
}

CodeChunk CMaterialBlockSampler::BindSamplerState( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget ) const
{
	return compiler.GetShader( shaderTarget ).SamplerState( m_addressU, m_addressV, m_addressW, m_filterMin, m_filterMag, m_filterMip, m_comparisonFunction );
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION
