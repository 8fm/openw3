/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialOutputBoolSocket.h"
#include "graphConnection.h"
#include "materialBlock.h"
#include "materialBlockCompiler.h"
#include "materialInputBoolSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialOutputBoolSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CMaterialOutputBoolSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CMaterialOutputSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to material texture input sockets
	if ( !otherSocket->IsA< CMaterialInputBoolSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
Int32 CMaterialOutputBoolSocket::GetShaderTarget() const
{
	Int32 target = 0;

	for ( Uint32 i=0; i<m_connections.Size(); i++ )
	{
		CGraphConnection* connection = m_connections[i];
		if ( connection->IsActive() )
		{
			CGraphSocket* input = connection->GetDestination();
			target |= SafeCast< CMaterialBlock >(input->GetBlock())->GetShaderTarget();
		}
	}

	return target;
}

CodeChunk CMaterialOutputBoolSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Compile block
	CMaterialBlock* block = SafeCast< CMaterialBlock>( GetBlock() );
	return compiler.CompileBlock( block, shaderTarget );
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialOutputBoolSocketSpawnInfo::MaterialOutputBoolSocketSpawnInfo( const CName& name )
{
	m_class = ClassID< CMaterialOutputBoolSocket >();
	m_name = name;
	m_direction = LSD_Output;
	m_placement = LSP_Left;
	m_color = Color::YELLOW;
	m_isMultiLink = true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT
