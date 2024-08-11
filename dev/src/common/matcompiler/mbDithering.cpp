/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Dithering block
class CMaterialBlockDithering : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockDithering, CMaterialBlock, "Deprecated", "Dithering" );

protected:
	Int32 m_pixelWidth;
	Int32 m_pixelHeight;

public:
	CMaterialBlockDithering()
		: m_pixelWidth( 4 )
		, m_pixelHeight( 4 )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Alpha ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		
		if ( !HasInput( CNAME(Alpha) ) || !HasInput( CNAME(Texture) ) )
		{
			return Vector::ONES;
		}

		CMaterialInputTextureSocket* textureSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( Texture ) ) );

		if ( NULL == socket )
		{
			return Vector::ONES;
		}

		CodeChunk vpos			= PS_DATA( "ScreenVPOS" );
		CodeChunk uv			= (vpos.xy() + 0.5f) * Float2 ( 1.f / Max(1,m_pixelWidth), 1.f / Max(1,m_pixelHeight) );
		CodeChunk textureName	= textureSocket->Compile( compiler, shaderTarget );
		CodeChunk samplerName	= compiler.GetShader( shaderTarget ).SamplerState( TA_Wrap, TA_Wrap, TA_Wrap, TFMin_Anisotropic, TFMag_Linear, TFMip_Linear, TCF_None );
		CodeChunk sample		= PS_VAR_FLOAT( Tex2D( textureName, samplerName, uv ).x() );
		CodeChunk alpha			= PS_VAR_FLOAT( CompileInput( compiler, CNAME( Alpha ), shaderTarget ).w() );

		return CodeChunk ( Vector::ONES ) * CodeChunk::Printf( false, "((%s) <= (%s) ? 1.0 : 0.0)", sample.AsChar(), alpha.AsChar() );
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockDithering )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_pixelWidth,  TXT("Pixel width") );
	PROPERTY_EDIT( m_pixelHeight, TXT("Pixel height") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockDithering );

#endif