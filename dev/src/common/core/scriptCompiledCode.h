/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dataBuffer.h"
#include "string.h"

/// Opcodes
enum EScriptOpcode
{
	OP_Nop,							//!< No operation
	OP_Null,						//!< CObject* NULL
	OP_IntOne,						//!< Integer '1'
	OP_IntZero,						//!< Integer '0'
	OP_IntConst,					//!< Int32 constant
	OP_ShortConst,					//!< Short ( 16bit int ) constant
	OP_FloatConst,					//!< Float constant
	OP_StringConst,					//!< String constant
	OP_NameConst,					//!< Name constant
	OP_ByteConst,					//!< Byte constant
	OP_BoolTrue,					//!< True
	OP_BoolFalse,					//!< False
	OP_Breakpoint,					//!< Breakpoint wrapper, generated only in debug code
	OP_Assign,						//!< Assign value
	OP_Target,						//!< Target of a label
	OP_LocalVar,					//!< Access to local variable
	OP_ParamVar,					//!< Access to function parameter variable
	OP_ObjectVar,					//!< Access to object variable
	OP_ObjectBindableVar,			//!< Access to bindable object variable
	OP_DefaultVar,					//!< Access to variable from default object
	OP_Switch,						//!< Switch statement
	OP_SwitchLabel,					//!< Label in switch statement
	OP_SwitchDefault,				//!< Default switch statement
	OP_Jump,						//!< Jump to target
	OP_JumpIfFalse,					//!< Jump if condition is false
	OP_Skip,						//!< Skippable block
	OP_Conditional,					//!< Conditional expression
	OP_Constructor,					//!< Constructor
	OP_FinalFunc,					//!< Call to final function ( static function binding )
	OP_VirtualFunc,					//!< Call to virtual function
	OP_VirtualParentFunc,			//!< Call to derived parent function ( no state machine )
	OP_EntryFunc,					//!< Call to state entry function
	OP_ParamEnd,					//!< End of parameters
	OP_Return,						//!< Return from function
	OP_StructMember,				//!< Access to structure member ( slow )
	OP_Context,						//!< Evaluation context change
	OP_TestEqual,					//!< Test if two given shit is default
	OP_TestNotEqual,				//!< Test if two given shit is default
	OP_New,							//!< Create object
	OP_Delete,						//!< Delete object
	OP_This,						//!< Reference to self
	OP_Parent,						//!< State machine context 
	OP_SavePoint,					//!< Function state SavePoint
	OP_SaveValue,					//!< Value of a function param saved by a SavePoint
	OP_SavePointEnd,				//!< End-of-savepoint-datablock marker

	// Array access opcodes
	OP_ArrayClear,					//!< Clear the array
	OP_ArraySize,					//!< Get the size of the array
	OP_ArrayResize,					//!< Resize array
	OP_ArrayFindFirst,				//!< Find index of first matching element from the array
	OP_ArrayFindFirstFast,			//!< Find index of first matching element from the array ( faster version )
	OP_ArrayFindLast,				//!< Find index of last matching element from the array
	OP_ArrayFindLastFast,			//!< Find index of last matching element from the array ( faster version )
	OP_ArrayContains,				//!< Check if array contains a given item
	OP_ArrayContainsFast,			//!< Check if array contains a given item ( faster version )
	OP_ArrayPushBack,				//!< Add element to array
	OP_ArrayPopBack,				//!< Remove last element from array
	OP_ArrayInsert,					//!< Insert element to array
	OP_ArrayRemove,					//!< Remove element from array
	OP_ArrayRemoveFast,				//!< Remove element from array ( faster version )
	OP_ArrayGrow,					//!< Add space to array
	OP_ArrayErase,					//!< Erase place in array
	OP_ArrayEraseFast,				//!< Fast erase from array, without preserving order of elements
	OP_ArrayLast,					//!< Get the last element from array
	OP_ArrayElement,				//!< Access to array element

	// Static array access opcodes
	OP_StaticArraySize,				//!< Get the size of the static array
	OP_StaticArrayFindFirst,		//!< Find index of first matching element from the static array
	OP_StaticArrayFindFirstFast,	//!< Find index of first matching element from the static array ( faster version )
	OP_StaticArrayFindLast,			//!< Find index of last matching element from the static array
	OP_StaticArrayFindLastFast,		//!< Find index of last matching element from the static array ( faster version )
	OP_StaticArrayContains,			//!< Check if static array contains a given item
	OP_StaticArrayContainsFast,		//!< Check if static array contains a given item ( faster version )
	OP_StaticArrayLast,				//!< Get the last element from static array
	OP_StaticArrayElement,			//!< Access to array static element

	// Casting
	OP_BoolToByte,
	OP_BoolToInt,
	OP_BoolToFloat,
	OP_BoolToString,
	OP_ByteToBool,
	OP_ByteToInt,
	OP_ByteToFloat,
	OP_ByteToString,
	OP_IntToBool,
	OP_IntToByte,
	OP_IntToFloat,
	OP_IntToString,
	OP_IntToEnum,
	OP_FloatToBool,
	OP_FloatToByte,
	OP_FloatToInt,
	OP_FloatToString,
	OP_NameToBool,
	OP_NameToString,
	OP_StringToBool,
	OP_StringToByte,
	OP_StringToInt,
	OP_StringToFloat,
	OP_ObjectToBool,
	OP_ObjectToString,
	OP_EnumToString,
	OP_EnumToInt,
	OP_DynamicCast,

	// Globals
	OP_GetGame,						//!< Access to the game pointer
	OP_GetPlayer,
	OP_GetCamera,
	OP_GetHud,
	OP_GetSound,
	OP_GetDebug,
	OP_GetTimer,
	OP_GetInput,
	OP_GetTelemetry,

	OP_Max
};

/// Breakpoint
struct ScriptBreakpoint
{
	String	m_file;
	Uint32	m_line;

	RED_INLINE ScriptBreakpoint()
		: m_line( 0 )
	{};

	RED_INLINE ScriptBreakpoint( const String& file, Uint32 line )
		: m_file( file )
		, m_line( line )
	{};
};

/// Savepoint
struct ScriptSavepoint
{
	Uint32	m_offset;

	RED_INLINE ScriptSavepoint()
		: m_offset( 0 )
	{};

	RED_INLINE ScriptSavepoint( Uint32 offset )
		: m_offset( offset )
	{};

	const CName& GetName( const Uint8* codeStart ) const;

	const Uint8* GetSavePointEndCode( const Uint8* codeStart ) const;
};

/// Code generator
class CScriptCodeGenerator;

/// Compiled code block
class CScriptCompiledCode
{
	friend class CRTTISerializer;

protected:
	String							m_sourceFile;	//!< Name of the source file this function was compiled from
	String							m_modContext;	//!< Helps show if this came from core source files or a mod
	Uint32							m_sourceLine;	//!< Line in the source file this function was defined
	DataBuffer						m_code;			//!< Compiled script code
	TDynArray< Uint32 >				m_breakpoints;	//!< Offset to breakpoints in code
	TDynArray< ScriptSavepoint >	m_savepoints;	//!< Offsets to savepoints in code

public:
	//! Get the source file this function was compiled from
	RED_INLINE const String& GetSourceFile() const { return m_sourceFile; }

	//! Get the line number in the source file this function was declared
	RED_INLINE Uint32 GetSourceLine() const { return m_sourceLine; }

	//! Get code buffer
	RED_INLINE Uint8* GetCode() { return static_cast< Uint8* >( m_code.GetData() ); }
	RED_INLINE const Uint8* GetCode() const { return static_cast< const Uint8* >( m_code.GetData() ); }

	//! Get end of the code
	RED_INLINE const Uint8* GetCodeEnd() const { return ( const Uint8* ) m_code.GetData() + m_code.GetSize(); }

public:
	CScriptCompiledCode();

	//! Initialize source code buffer
	void Initialize( const CScriptCodeGenerator& generator );

	void CreateBuffer( Uint32 size );

public:
	//! Remove all breakpoints from function
	void BreakpointRemoveAll();

	//! Toggle breakpoint on line, returns true if breakpoint was toggled
	Bool BreakpointToggle( const ScriptBreakpoint& breakpoint, Bool status );

	//! Check if breakpoint is set on line
	Bool BreakpointIsSet( const ScriptBreakpoint& breakpoint ) const;

	//! Get the active breakpoints in this function
	Bool BreakpointGetActive( TDynArray< ScriptBreakpoint >& breakpoints ) const;

public:
	//! Returns a pointer to the code behind the specified savepoint
	const Uint8* GetCodeBehindSavePoint( const CName& savepointName ) const;
};
