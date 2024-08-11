/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Ambient light value
class CMaterialBlockAmbientLight : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockAmbientLight, CMaterialBlock, "System Samplers", "Ambient Light" );

protected:
	Bool	m_ForceGlobalAmbient;

public:
	CMaterialBlockAmbientLight()
		: m_ForceGlobalAmbient( false )
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( WorldNormal ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		// Get normal
		// Check "Normal" and "WorldNormal" input, because due to the fact that the name was changed some graphs can have sockets with name "Normal"
		// and some with "WorldNormal". The new ones will only have input sockets with the name "WorldNormal"
		CodeChunk normal;
		if ( HasInput( CNAME( WorldNormal ) ) )
		{
			normal = CompileInput( compiler, CNAME( WorldNormal ), shaderTarget );
		}
		else if ( HasInput( CNAME( Normal ) ) )
		{
			normal = CompileInput( compiler, CNAME( Normal ), shaderTarget );
		}
		else
		{
			normal = SHADER_DATA( "WorldNormal", shaderTarget );
		}

		// Get pos
		CodeChunk vertexPos = PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );

		// Calc ambient
		CodeChunk ambientResult = compiler.GetPS().AutomaticName();
		const AnsiChar * const funcName = m_ForceGlobalAmbient ? "CalcGlobalEnvProbeAmbient" : "CalcEnvProbeAmbient";
		CodeChunk call = CodeChunk::Printf( false, "float3 %s = %s( %s, %s );", ambientResult.AsChar(), funcName, vertexPos.xyz().AsChar(), normal.xyz().AsChar() );
		compiler.GetPS().Statement( call );

		//
		return Float4 ( ambientResult, 1.f );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockAmbientLight)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_ForceGlobalAmbient, TXT("Force global ambient") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockAmbientLight );

#endif
