/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialInputSocket.h"
#include "graphConnection.h"
#include "materialBlockCompiler.h"
#include "materialOutputSocket.h"


#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialInputSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CMaterialInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CGraphSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to material output sockets
	if ( !otherSocket->IsA< CMaterialOutputSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

CMaterialOutputSocket* CMaterialInputSocket::GetDestinationSocket() const
{
	// Get connected socket
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CMaterialOutputSocket >( connection->GetDestination() );
	}

	// Not connected
	return NULL;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialInputSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, const CodeChunk& defaultValue /*=CodeChunk::EMPTY*/ ) const
{
	// Get compiled value from destination socket
	CMaterialOutputSocket* destination = GetDestinationSocket();
	if ( destination )
	{
		return destination->Compile( compiler, shaderTarget );
	}

	// No destination compile default value
	return SHADER_VAR_FLOAT4( defaultValue ? defaultValue : CodeChunk( 0.0f ), shaderTarget );
}

#endif

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )


#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialInputSocketSpawnInfo::MaterialInputSocketSpawnInfo( const CName& name, Bool isVisible )
	: GraphSocketSpawnInfo( ClassID< CMaterialInputSocket >() )
{
	// Setup color and name 
	m_color = Color::WHITE;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Input;
	m_placement = LSP_Right;

	// Visiblity
	m_isVisible = isVisible;
	m_isVisibleByDefault = isVisible;
}

#endif // NO_EDITOR_GRAPH_SUPPORT
