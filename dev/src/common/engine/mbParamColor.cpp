/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "mbParamColor.h"
#include "graphConnectionRebuilder.h"
#include "materialOutputSocket.h"
#include "materialBlockCompiler.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameterColor );

CMaterialParameterColor::CMaterialParameterColor()
	: m_color( 255, 255, 255 )
{
}

IProperty* CMaterialParameterColor::GetParameterProperty() const
{
	static IProperty* prop = GetClass()->FindProperty( CNAME( color ) );
	ASSERT( prop );
	return prop;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterColor::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Color ), Color::WHITE ) );

	{
		MaterialOutputSocketSpawnInfo a( TXT("xxxx"), CNAME( Red ), Color::RED );
		a.m_isVisible = false;
		a.m_isVisibleByDefault = false;
		CreateSocket( a );
	}

	{
		MaterialOutputSocketSpawnInfo a( TXT("yyyy"), CNAME( Green ), Color::GREEN );
		a.m_isVisible = false;
		a.m_isVisibleByDefault = false;
		CreateSocket( a );
	}


	{
		MaterialOutputSocketSpawnInfo a( TXT("zzzz"), CNAME( Blue ), Color::BLUE );
		a.m_isVisible = false;
		a.m_isVisibleByDefault = false;
		CreateSocket( a );
	}

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ), Color( 127, 127, 127 ) ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterColor::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
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
		CodeChunk varname = CodeChunk::Printf( true, "%s_color", UNICODE_TO_ANSI( paramNameWithoutSpaces.AsChar() ) );
		CodeChunk var = compiler.GetShader(shaderTarget).Param( MDT_Float4, &constantRegister, varname );

		// Create dynamic data that will bind value
		compiler.GetMaterialCompiler()->Param( m_parameterName, constantRegister );
		return var;
	}
	else
	{
		// Evaluate directly in code
		return m_color.ToVector();
	}
}

#endif
