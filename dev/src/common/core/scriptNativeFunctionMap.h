/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define MAX_NATIVE_FUNCTIONS		4096

// Native function handler for CObject's
class CScriptStackFrame;
typedef void ( *TNativeGlobalFunc )( class IScriptable* context, CScriptStackFrame&, void* );
typedef void ( IScriptable::*TNativeFunc )( CScriptStackFrame&, void* );

/// Mapping of native functions to opcodes
class CScriptNativeFunctionMap
{
protected:
	static TNativeFunc			m_nativeFunctions[ MAX_NATIVE_FUNCTIONS ];			//!< Mapping of indices to native functions
	static TNativeGlobalFunc	m_nativeGlobals[ MAX_NATIVE_FUNCTIONS ];			//!< Mapping of indices to global native functions
	Uint32						m_nextFreeIndex;									//!< Next allocatable native function index

public:
	CScriptNativeFunctionMap();

	//! Set function ( opcodes )
	void SetOpcode( EScriptOpcode opcode, TNativeGlobalFunc function );

	//! Register native function
	Uint32 RegisterClassNative( TNativeFunc function );

	//! Register native global function
	Uint32 RegisterGlobalNative( TNativeGlobalFunc function );

public:
	//! Get class native function
	static RED_INLINE TNativeFunc GetClassNativeFunction( Uint32 index )
	{
		return m_nativeFunctions[ index ];
	}

	//! Get global native function
	static RED_INLINE TNativeGlobalFunc GetGlobalNativeFunction( Uint32 index )
	{
		return m_nativeGlobals[ index ];
	}
};


