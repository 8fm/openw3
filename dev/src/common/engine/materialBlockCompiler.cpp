/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialBlockCompiler.h"
#include "materialGraph.h"
#include "materialBlock.h"
#include "materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CMaterialBlockCompiler::CMaterialBlockCompiler( CMaterialGraph* graph, IMaterialCompiler* compiler )
	: m_graph( graph )
	, m_compiler( compiler )
{
	// Gather roots
	for ( Uint32 i=0; i<graph->GraphGetBlocks().Size(); i++ )
	{
		// A root block
		CMaterialBlock* block = Cast< CMaterialBlock >( graph->GraphGetBlocks()[i] );
		if ( block && block->IsRootBlock() )
		{
			 m_roots.PushBack( static_cast<CMaterialRootBlock*>( block ) );
		}
	}
}

CMaterialBlockCompiler::~CMaterialBlockCompiler()
{

}

void CMaterialBlockCompiler::CompileRoots()
{
	// Compile all roots
	for ( Uint32 i=0; i<m_roots.Size(); i++ )
	{
		CMaterialRootBlock* block = m_roots[i];
		block->Compile( *this );
	}
}

CodeChunk CMaterialBlockCompiler::CompileBlock( CMaterialBlock* block, EMaterialShaderTarget shaderTarget, const CMaterialOutputSocket* socket /*= NULL */ )
{
	ASSERT( block );
	ASSERT( block->GetGraph() == m_graph );

	// If block has been already compiled use the compiled output
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_blocksMutex );

		if ( THashMap< CMaterialOutputSocket*, CodeChunk >* results = m_blocks.FindPtr( block ) )
		{
			if ( CodeChunk* chunkPtr = results->FindPtr( const_cast<CMaterialOutputSocket*>(socket) ) )
			{
				return *chunkPtr;
			}
		}
	}

	// Compile
	CodeChunk chunk = block->Compile( *this, shaderTarget, const_cast<CMaterialOutputSocket*>(socket) ); //HACK

	// Store
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_blocksMutex );
		m_blocks[ block ].Insert( const_cast<CMaterialOutputSocket*>(socket), chunk );
	}
	return chunk;
}

Bool CMaterialBlockCompiler::IsTerrainShader() const
{
	for ( Uint32 i=0; i<m_graph->GraphGetBlocks().Size(); i++ )
	{
		CMaterialBlock* block = Cast< CMaterialBlock >( m_graph->GraphGetBlocks()[i] );
		if ( block &&
			( block->GetClass()->GetName() == TXT("CMaterialTerrainMaterialSampler")
			|| block->GetClass()->GetName() == TXT("CMaterialTerrainMaterialBlending") ) )
		{
			return true;
		}
	}

	// No terrain block, assume no terrain material
	return false;
}

#endif
