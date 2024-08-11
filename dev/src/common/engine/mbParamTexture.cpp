/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbParamTexture.h"
#include "graphConnectionRebuilder.h"
#include "materialOutputTextureSocket.h"
#include "materialOutputSocket.h"
#include "materialBlockCompiler.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameterTexture );

CMaterialParameterTexture::CMaterialParameterTexture()
	: m_isAtlas( false )
{
}

IProperty* CMaterialParameterTexture::GetParameterProperty() const
{
	static IProperty* prop = GetClass()->FindProperty( CNAME( texture ) );
	ASSERT( prop );
	return prop;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterTexture::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputTextureSocketSpawnInfo( CNAME( Texture ) ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterTexture::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Create dynamic data that will bind value
	CodeChunk parameterName;

	IMaterialShaderCompiler& shader = compiler.GetShader( shaderTarget );

	if ( m_parameterName )
	{
		String paramNameWithoutSpaces = m_parameterName.AsString();
		paramNameWithoutSpaces.ReplaceAll(TXT(" "), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("-"), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("["), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("]"), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT(","), TXT("_"));
		parameterName = CodeChunk( UNICODE_TO_ANSI( paramNameWithoutSpaces.AsChar() ), false );
	}
	else
	{
		parameterName = shader.AutomaticName();
		parameterName.SetConst( false );
	}

	// Determine sampler type
	EMaterialSamplerType samplerType = MST_Texture;

	// Allocate sampler
	Int32 textureIndex = -1;
	CodeChunk textureName = CodeChunk::Printf( true, "t_%s", parameterName.AsChar() );
	CodeChunk varSampler = shader.Texture( samplerType, &textureIndex, textureName );
	compiler.GetMaterialCompiler()->Param( CName( ANSI_TO_UNICODE(parameterName.AsChar() )), textureIndex, shaderTarget );

	return textureName;
}

#endif