/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialInputBoolSocket.h"
#include "graphConnection.h"
#include "materialBlockCompiler.h"
#include "materialOutputBoolSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialInputBoolSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CMaterialInputBoolSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CMaterialInputSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to correct sockets
	if ( !otherSocket->IsA< CMaterialOutputBoolSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

CMaterialOutputBoolSocket* CMaterialInputBoolSocket::GetDestinationSocket() const
{
	// Get connected socket
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CMaterialOutputBoolSocket >( connection->GetDestination() );
	}

	// Not connected
	return NULL;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialInputBoolSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get compiled value from destination socket
	CMaterialOutputBoolSocket* destination = GetDestinationSocket();
	ASSERT( destination );
	return destination->Compile( compiler, shaderTarget );
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialInputBoolSocketSpawnInfo::MaterialInputBoolSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CMaterialInputBoolSocket >() )
{
	// Setup color and name 
	m_color = Color::YELLOW;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Input;
	m_placement = LSP_Right;
}

#endif
