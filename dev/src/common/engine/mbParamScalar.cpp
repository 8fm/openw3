/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "mbParamScalar.h"
#include "graphConnectionRebuilder.h"
#include "materialBlockCompiler.h"
#include "materialOutputSocket.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameterScalar );

CMaterialParameterScalar::CMaterialParameterScalar()
	: m_scalar( 1.0f )
{
}

IProperty* CMaterialParameterScalar::GetParameterProperty() const
{
	static IProperty* prop = GetClass()->FindProperty( CNAME( scalar ) );
	ASSERT( prop );
	return prop;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterScalar::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Value ), Color::WHITE ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterScalar::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
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
		CodeChunk varname = CodeChunk::Printf( true, "%s_scalar", UNICODE_TO_ANSI( paramNameWithoutSpaces.AsChar() ) );
		CodeChunk var = compiler.GetShader(shaderTarget).Param( MDT_Float, &constantRegister, varname );

		// Create dynamic data that will bind value
		compiler.GetMaterialCompiler()->Param( m_parameterName, constantRegister, shaderTarget );
		return var;
	}
	else
	{
		// Evaluate directly in code
		return m_scalar;
	}
}

#endif
