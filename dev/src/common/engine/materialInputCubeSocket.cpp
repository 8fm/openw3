/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialInputCubeSocket.h"
#include "graphConnection.h"
#include "materialBlockCompiler.h"
#include "materialOutputCubeSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialInputCubeSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CMaterialInputCubeSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CGraphSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to material output sockets
	if ( !otherSocket->IsA< CMaterialOutputCubeSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

CMaterialOutputCubeSocket* CMaterialInputCubeSocket::GetDestinationSocket() const
{
	// Get connected socket
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CMaterialOutputCubeSocket >( connection->GetDestination() );
	}

	// Not connected
	return NULL;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialInputCubeSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get compiled value from destination socket
	CMaterialOutputCubeSocket* destination = GetDestinationSocket();
	ASSERT( destination );
	return destination->Compile( compiler, shaderTarget );
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialInputCubeSocketSpawnInfo::MaterialInputCubeSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CMaterialInputCubeSocket >() )
{
	// Setup color and name 
	m_color = Color::WHITE;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Input;
	m_placement = LSP_Right;
}

#endif
