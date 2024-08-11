/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_OPCODE_EXAMINER_H__
#define __SCRIPT_OPCODE_EXAMINER_H__

#include "rttiSerializer.h"

class CScriptOpCodeExaminer
{
public:
	struct DisassembledLine
	{
		Uint32 m_line;
		String m_details;
	};

	struct DisassembledFunction
	{
		String m_file;
		TDynArray< DisassembledLine > m_lines;
	};


	CScriptOpCodeExaminer();
	~CScriptOpCodeExaminer();

	void Examine( const CFunction* function );

	RED_INLINE const TDynArray< DisassembledFunction >& GetOutput() const { return m_output; }

private:
	template< typename T >
	RED_INLINE void Skip( const Uint8*& position );

	template< typename T >
	RED_INLINE void Peek( const Uint8* position, T& value );

	template< typename T >
	RED_INLINE void Read( const Uint8*& position, T& value );

	template< typename T >
	RED_INLINE String ReadConst( const Uint8*& position );

	RED_INLINE String ReadStringConst( const Uint8*& position );

	EScriptOpcode ReadOpcode( const Uint8*& position );
	void StartNewLine();

	void ReadBreakpoint( const Uint8*& position );
	String ReadSwitchLabel( const Uint8*& position );


private:
	TDynArray< DisassembledFunction > m_output;
};

#endif // __SCRIPT_OPCODE_EXAMINER_H__
