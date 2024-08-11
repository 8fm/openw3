#include "build.h"
#include "mbInterpolator.h"
#include "graphConnectionRebuilder.h"
#include "materialInputSocket.h"
#include "materialOutputSocket.h"
#include "graphSocket.h"
#include "materialBlockCompiler.h"

IMPLEMENT_ENGINE_CLASS( CMaterialBlockInterpolator );

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CMaterialBlockInterpolator::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
CodeChunk CMaterialBlockInterpolator::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	ASSERT(shaderTarget == MSH_PixelShader);

	if ( HasInput( CNAME( In ) ) )
	{
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket* socket = m_sockets[i];
			if ( socket && socket->GetName() == CNAME( In ) )
			{
				if ( socket->IsA< CMaterialInputSocket >() )
				{
					CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
					CMaterialBlock* value = static_cast< CMaterialBlock* >(is->GetDestinationSocket()->GetBlock());
					if (value)
					{
						//CodeChunk modifier = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						//CodeChunk vertexPosition = compiler.GetShader(MSH_VertexShader).Data( CodeChunk::Printf(true, "VertexPosition") );
						//vertexPositionModifier = CodeChunk::Printf( false, "%s += %s.xyz;\r\nFatVertex = CalcPosition(FatVertex, %s);", vertexPosition.AsChar(), modifier.AsChar(), vertexPosition.AsChar() );
						//compiler.GetShader(MSH_VertexShader).Statement( vertexPositionModifier );

						/*		CodeChunk vsData = m_vertexShader->Data( semantic );
						CodeChunk psData = Var( MDT_Float2, 0.0f );
						m_compiler->Connect( MDT_Float2, vsData, psData );
						output = psData;*/

						CodeChunk vsData = compiler.CompileBlock( (CMaterialBlock*)value, MSH_VertexShader );
						CodeChunk psData = compiler.GetShader(MSH_PixelShader).Var( MDT_Float4, 0.0f );
						compiler.GetMaterialCompiler()->Connect(MDT_Float4, vsData, psData, CodeChunk::Printf(true, "%s", socket->GetName().AsAnsiChar() ) );
						return psData;
					}
				}
			}
		}
	}

	return CodeChunk::EMPTY;
}
#endif