/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialInputTextureSocket.h"
#include "graphConnection.h"
#include "materialBlockCompiler.h"
#include "materialOutputTextureSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialInputTextureSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CMaterialInputTextureSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CGraphSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to material output sockets
	if ( !otherSocket->IsA< CMaterialOutputTextureSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

CMaterialOutputTextureSocket* CMaterialInputTextureSocket::GetDestinationSocket() const
{
	// Get connected socket
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CMaterialOutputTextureSocket >( connection->GetDestination() );
	}

	// Not connected
	return NULL;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialInputTextureSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get compiled value from destination socket
	CMaterialOutputTextureSocket* destination = GetDestinationSocket();
	ASSERT( destination, TXT("Failed to find the destination socket") );
	return destination ? destination->Compile( compiler, shaderTarget ) : CodeChunk();
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialInputTextureSocketSpawnInfo::MaterialInputTextureSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CMaterialInputTextureSocket >() )
{
	// Setup color and name 
	m_color = Color::WHITE;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Input;
	m_placement = LSP_Right;
}

#endif // NO_EDITOR_GRAPH_SUPPORT
