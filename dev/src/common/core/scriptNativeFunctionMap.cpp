/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"

TNativeFunc	CScriptNativeFunctionMap::m_nativeFunctions[ MAX_NATIVE_FUNCTIONS ];
TNativeGlobalFunc CScriptNativeFunctionMap::m_nativeGlobals[ MAX_NATIVE_FUNCTIONS ];

CScriptNativeFunctionMap::CScriptNativeFunctionMap()
	: m_nextFreeIndex( 256 )
{
	// Reset
	Red::System::MemorySet( &m_nativeFunctions, 0, sizeof( m_nativeFunctions ) );
	Red::System::MemorySet( &m_nativeGlobals, 0, sizeof( m_nativeGlobals ) );
}

void CScriptNativeFunctionMap::SetOpcode( EScriptOpcode opcode, TNativeGlobalFunc function )
{
	ASSERT( opcode < OP_Max );
	ASSERT( !m_nativeGlobals[ opcode ] );
	m_nativeGlobals[ opcode ] = function;
}

Uint32 CScriptNativeFunctionMap::RegisterClassNative( TNativeFunc function )
{
	ASSERT( function );
	ASSERT( m_nextFreeIndex < MAX_NATIVE_FUNCTIONS );
	Uint32 functionIndex = m_nextFreeIndex;
	m_nativeFunctions[ functionIndex ] = function;
	m_nextFreeIndex++;
	return functionIndex;
}

Uint32 CScriptNativeFunctionMap::RegisterGlobalNative( TNativeGlobalFunc function )
{
	ASSERT( function );
	ASSERT( m_nextFreeIndex < MAX_NATIVE_FUNCTIONS );
	Uint32 functionIndex = m_nextFreeIndex;
	m_nativeGlobals[ functionIndex ] = function;
	m_nextFreeIndex++;
	return functionIndex;
}