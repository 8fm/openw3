/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Calculate Local Reflection
class CMaterialBlockLocalReflection : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockLocalReflection, CMaterialBlock, "System Samplers", "Local Reflection" );

public:
	Bool	m_premultipliedAlpha;
	Float	m_inputNormalStrength;

public:
	CMaterialBlockLocalReflection()
		: m_premultipliedAlpha( true )
		, m_inputNormalStrength( 1.f )
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( AlphaScale ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Data ),   Color::WHITE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ),  Color::WHITE ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		// Deprecated stuff...

		CodeChunk reflection = Float4( Vector::ZEROS );
		reflection = PS_VAR_FLOAT4( reflection );
		
		CodeChunk result		= reflection;
		return result;
	}

};

BEGIN_CLASS_RTTI( CMaterialBlockLocalReflection )
	PARENT_CLASS( CMaterialBlock )		
	PROPERTY_EDIT( m_premultipliedAlpha, TXT("Multiply RGB by output alpha") );	
	PROPERTY_EDIT( m_inputNormalStrength, TXT("Input normal strength" ) );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockLocalReflection );

#endif