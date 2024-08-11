/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_COMPILATION_ALLOCATOR_H__
#define __SCRIPT_COMPILATION_ALLOCATOR_H__

#include "scriptCodeNode.h"

class CScriptCodeNodeCompilationPool : public Red::System::NonCopyable
{
public:
	CScriptCodeNodeCompilationPool();
	~CScriptCodeNodeCompilationPool();

	CScriptCodeNode* Create( const CScriptFileContext& context, EScriptOpcode opcode );

	RED_INLINE Uint32 GetNumElementsUsed() const { return m_firstFreeIndex; }

private:

	static const Uint32 GROW_SIZE = 128u;

	typedef TDynArray< CScriptCodeNode, MC_ScriptCompilation, MemoryPool_ScriptCompilation > NodePool;
	typedef TDynArray< NodePool, MC_ScriptCompilation, MemoryPool_ScriptCompilation > Pool;

	Pool m_pool;
	Uint32 m_firstFreeIndex;
};

#endif // __SCRIPT_COMPILATION_ALLOCATOR_H__
