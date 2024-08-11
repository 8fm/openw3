/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "scriptCodeNodeCompilationPool.h"

CScriptCodeNodeCompilationPool::CScriptCodeNodeCompilationPool()
:	m_firstFreeIndex( 0 )
{
}

CScriptCodeNodeCompilationPool::~CScriptCodeNodeCompilationPool()
{
	m_pool.Clear();
}

CScriptCodeNode* CScriptCodeNodeCompilationPool::Create( const CScriptFileContext& context, EScriptOpcode opcode )
{
	Uint32 nodePool = m_firstFreeIndex / GROW_SIZE;
	Uint32 index = m_firstFreeIndex % GROW_SIZE;

	++m_firstFreeIndex;

	if( nodePool >= m_pool.Size() )
	{
		m_pool.Grow();
		m_pool.Back().Resize( GROW_SIZE );
	}

	CScriptCodeNode& node = m_pool[ nodePool ][ index ];

	node = std::move( CScriptCodeNode( context, opcode ) );
	return &node;
}
