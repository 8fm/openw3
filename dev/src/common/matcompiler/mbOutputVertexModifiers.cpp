/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputVertexModifiers.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

RED_DEFINE_STATIC_NAME( VertexPosition )
RED_DEFINE_STATIC_NAME( VertexNormal )
RED_DEFINE_STATIC_NAME( VertexTangent )
RED_DEFINE_STATIC_NAME( VertexBinormal )

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputVertexModifiers );

using namespace CodeChunkOperators;

CMaterialBlockOutputVertexModifiers::CMaterialBlockOutputVertexModifiers()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputVertexModifiers::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Add sockets
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( VertexPosition ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( VertexNormal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( VertexTangent ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( VertexBinormal ) ) );
}

#endif

void CMaterialBlockOutputVertexModifiers::Compile( CMaterialBlockCompiler& compiler ) const
{
	CodeChunk vertexPositionModifier;
	if ( HasInput( CNAME( VertexPosition ) ) )
	{
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket* socket = m_sockets[i];
			if ( socket && socket->GetName() == CNAME( VertexPosition ) )
			{
				if ( socket->IsA< CMaterialInputSocket >() )
				{
					CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
					CMaterialBlock* value = static_cast< CMaterialBlock* >(is->GetDestinationSocket()->GetBlock());
					if (value)
					{
						CodeChunk modifier = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						CodeChunk vertexPosition = compiler.GetShader(MSH_VertexShader).Data( CodeChunk::Printf(true, "VertexPosition") );
						vertexPositionModifier = CodeChunk::Printf( false, "%s += %s.xyz;\r\nFatVertex = CalcPosition(FatVertex, %s);", vertexPosition.AsChar(), modifier.AsChar(), vertexPosition.AsChar() );
						compiler.GetShader(MSH_VertexShader).Statement( vertexPositionModifier );
					}
				}
			}
		}
	}

	CodeChunk vertexNormalModifier;
	if ( HasInput( CNAME( VertexNormal ) ) )
	{
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket* socket = m_sockets[i];
			if ( socket && socket->GetName() == CNAME( VertexNormal ) )
			{
				if ( socket->IsA< CMaterialInputSocket >() )
				{
					CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
					CMaterialBlock* value = static_cast< CMaterialBlock* >(is->GetDestinationSocket()->GetBlock());
					if (value)
					{
						CodeChunk modifier = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						CodeChunk vertexNormal = compiler.GetShader(MSH_VertexShader).Data( CodeChunk::Printf(true, "VertexNormal") );
						vertexNormalModifier = CodeChunk::Printf( false, "%s += %s.xyz;\r\nFatVertex.WorldNormal = normalize( mul( %s, (float3x3)FatVertex.LocalToWorld ) );", vertexNormal.AsChar(), modifier.AsChar(), vertexNormal.AsChar() );
						compiler.GetShader(MSH_VertexShader).Statement( vertexNormalModifier );
					}
				}
			}
		}
	}

	CodeChunk vertexTangentModifier;
	if ( HasInput( CNAME( VertexTangent ) ) )
	{
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket* socket = m_sockets[i];
			if ( socket && socket->GetName() == CNAME( VertexTangent ) )
			{
				if ( socket->IsA< CMaterialInputSocket >() )
				{
					CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
					CMaterialBlock* value = static_cast< CMaterialBlock* >(is->GetDestinationSocket()->GetBlock());
					if (value)
					{
						CodeChunk modifier = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						CodeChunk vertexTangent = compiler.GetShader(MSH_VertexShader).Data( CodeChunk::Printf(true, "VertexTangent") );
						vertexTangentModifier = CodeChunk::Printf( false, "%s += %s.xyz;\r\nFatVertex.WorldTangent = normalize( mul( %s, (float3x3)FatVertex.LocalToWorld ) );", vertexTangent.AsChar(), modifier.AsChar(), vertexTangent.AsChar() );
						compiler.GetShader(MSH_VertexShader).Statement( vertexTangentModifier );
					}
				}
			}
		}
	}

	CodeChunk vertexBinormalModifier;
	if ( HasInput( CNAME( VertexBinormal ) ) )
	{
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket* socket = m_sockets[i];
			if ( socket && socket->GetName() == CNAME( VertexBinormal ) )
			{
				if ( socket->IsA< CMaterialInputSocket >() )
				{
					CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
					CMaterialBlock* value = static_cast< CMaterialBlock* >(is->GetDestinationSocket()->GetBlock());
					if (value)
					{
						CodeChunk modifier = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						CodeChunk vertexBinormal = compiler.GetShader(MSH_VertexShader).Data( CodeChunk::Printf(true, "VertexBinormal") );
						vertexBinormalModifier = CodeChunk::Printf( false, "%s += %s.xyz;\r\nFatVertex.WorldBinormal = normalize( mul( %s, (float3x3)FatVertex.LocalToWorld ) );", vertexBinormal.AsChar(), modifier.AsChar(), vertexBinormal.AsChar() );
						compiler.GetShader(MSH_VertexShader).Statement( vertexBinormalModifier );
					}
				}
			}
		}
	}
}

#endif