/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialOutputSocket.h"
#include "graphConnection.h"
#include "materialBlock.h"
#include "materialBlockCompiler.h"
#include "materialInputSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

IMPLEMENT_ENGINE_CLASS( CMaterialOutputSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialOutputSocket::OnSpawned( class CGraphBlock* block, const class GraphSocketSpawnInfo& info )
{
	CGraphSocket::OnSpawned( block, info );

	m_swizzle = (( MaterialOutputSocketSpawnInfo& ) info ).m_swizzle;
}

Bool CMaterialOutputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// Check base conditions first
	if ( !CGraphSocket::CanConnectTo( otherSocket ))
	{
		return false;
	}

	// We can connect only to material input sockets
	if ( !otherSocket->IsA< CMaterialInputSocket >() )
	{
		return false;
	}

	// We can connect
	return true;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

Int32 CMaterialOutputSocket::GetShaderTarget() const
{
	Int32 target = 0;

	for ( Uint32 i=0; i<m_connections.Size(); i++ )
	{
		CGraphConnection* connection = m_connections[i];
		if ( connection->IsActive() && connection->IsValid() )
		{
			CGraphSocket* input = connection->GetDestination();
			if ( input->GetBlock() )
			{
				target |= SafeCast< CMaterialBlock >(input->GetBlock())->GetShaderTarget();
			}
		}
	}
	return target;
}

CodeChunk CMaterialOutputSocket::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Compile block
	CMaterialBlock* block = SafeCast< CMaterialBlock>( GetBlock() );
	CodeChunk blockCode = compiler.CompileBlock( block, shaderTarget, this );

	// Add output swizzle
	if ( !m_swizzle.Empty() )
	{
		// Check if swizzle is a member definition. Member definitions begin with '_'
		if ( m_swizzle[0] == '_' )
		{
			return CodeChunk::Printf( blockCode.IsConst(), "(%s%s)", blockCode.AsChar(), UNICODE_TO_ANSI( m_swizzle.AsChar() ) );
		}
		else if ( m_swizzle[0] == '[' )
		{
			return CodeChunk::Printf( blockCode.IsConst(), "(%s%s)", blockCode.AsChar(), UNICODE_TO_ANSI( m_swizzle.AsChar() ) );
		}
		else
		{
			return CodeChunk::Printf( blockCode.IsConst(), "(%s).%s", blockCode.AsChar(), UNICODE_TO_ANSI( m_swizzle.AsChar() ) );
		}
	}

	// No swizzle
	return blockCode;
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

MaterialOutputSocketSpawnInfo::MaterialOutputSocketSpawnInfo()
	: GraphSocketSpawnInfo( ClassID< CMaterialOutputSocket >() )
	, m_swizzle()
{
	// Output sockets supports multi links
	m_isMultiLink = true;

	// Setup color and name 
	m_color = Color::WHITE;
	m_name = CNAME( Out );

	// Setup placement and direction
	m_direction = LSD_Output;
	m_placement = LSP_Left;
}

MaterialOutputSocketSpawnInfo::MaterialOutputSocketSpawnInfo( const CName& name, const Color& color )
	: GraphSocketSpawnInfo( ClassID< CMaterialOutputSocket >() )
{
	// Output sockets supports multi links
	m_isMultiLink = true;

	// Setup color and name 
	m_color = color;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Output;
	m_placement = LSP_Left;
}


MaterialOutputSocketSpawnInfo::MaterialOutputSocketSpawnInfo( const String& swizzle, const CName& name, const Color& color )
	: GraphSocketSpawnInfo( ClassID< CMaterialOutputSocket >() )
	, m_swizzle( swizzle )
{
	// Output sockets supports multi links
	m_isMultiLink = true;

	// Setup color and name 
	m_color = color;
	m_name = name;

	// Setup placement and direction
	m_direction = LSD_Output;
	m_placement = LSP_Left;
}

#endif // NO_EDITOR_GRAPH_SUPPORT
