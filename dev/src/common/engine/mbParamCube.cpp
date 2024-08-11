/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "mbParamCube.h"
#include "graphConnectionRebuilder.h"
#include "materialOutputCubeSocket.h"
#include "materialBlockCompiler.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameterCube );

CMaterialParameterCube::CMaterialParameterCube()
{
}

IProperty* CMaterialParameterCube::GetParameterProperty() const
{
	static IProperty* prop = GetClass()->FindProperty( CNAME( cube ) );
	ASSERT( prop );
	return prop;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterCube::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputCubeSocketSpawnInfo( CNAME( Cube ) ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterCube::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
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

	// Allocate texture
	Int32 textureIndex = -1;
	CodeChunk textureName = CodeChunk::Printf( false, "t_%s", parameterName.AsChar() );
	CodeChunk varTexture = shader.Texture( MST_Cubemap, &textureIndex, textureName );

	// Create dynamic data that will bind value
	compiler.GetMaterialCompiler()->Param( CName( ANSI_TO_UNICODE( parameterName.AsChar() ) ), textureIndex );

	// Return sampler name
	return textureName;
}

#endif