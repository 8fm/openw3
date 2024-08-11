/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "mbParamVector.h"
#include "graphConnectionRebuilder.h"
#include "materialOutputSocket.h"
#include "materialBlockCompiler.h"


IMPLEMENT_ENGINE_CLASS( CMaterialParameterVector );

CMaterialParameterVector::CMaterialParameterVector()
	: m_vector( 0.0f, 0.5f, 1.0f, 2.0f )
{
}

IProperty* CMaterialParameterVector::GetParameterProperty() const
{
	static CProperty* prop = GetClass()->FindProperty( CNAME( vector ) );
	ASSERT( prop );
	return prop;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterVector::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Value ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 127, 127, 127 ) ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterVector::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	if ( m_parameterName )
	{
		// Allocate constant
		Int32 constantRegister = -1;
		String paramNameWithoutSpaces = m_parameterName.AsString();
		paramNameWithoutSpaces.ReplaceAll(TXT(" "), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("-"), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("["), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT("]"), TXT("_"));
		paramNameWithoutSpaces.ReplaceAll(TXT(","), TXT("_"));
		CodeChunk varname = CodeChunk::Printf( true, "%s_vector", UNICODE_TO_ANSI( paramNameWithoutSpaces.AsChar() ) );
		CodeChunk var = compiler.GetShader(shaderTarget).Param( MDT_Float4, &constantRegister, varname );

		// Create dynamic data that will bind value
		compiler.GetMaterialCompiler()->Param( m_parameterName, constantRegister );
		return var;
	}
	else
	{
		// Evaluate directly in code
		return m_vector;
	}
}

#endif
