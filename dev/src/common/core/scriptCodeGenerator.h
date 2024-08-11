/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CScriptCodeNode;

/// Byte code generator for compiled script functions
class CScriptCodeGenerator
{
protected:
	/// Label
	struct CodeLabel
	{
		CScriptCodeNode*	m_label;			//!< Label node
		Uint16				m_writeOffset;		//!< Where to write label offset
	};

public:
	typedef TDynArray< CodeLabel, MC_ScriptCompilation, MemoryPool_ScriptCompilation > CodeLabels;
	typedef TDynArray< Uint8, MC_ScriptCompilation, MemoryPool_ScriptCompilation > BufferU8;
	typedef TDynArray< Uint32, MC_ScriptCompilation, MemoryPool_ScriptCompilation > BufferU32;

protected:

	String					m_sourceFile;	//!< Source file from which this code is generated
	Uint32					m_sourceLine;	//!< Line at the source file this code starts
	CodeLabels				m_labels;		//!< Labels
	BufferU8				m_code;			//!< Generated code
	BufferU32				m_breakpoints;	//!< Generated breakpoints
	BufferU32				m_savepoints;	//!< Generated savepoints

public:
	//! Get the source file
	RED_INLINE const String& GetSourceFile() const { return m_sourceFile; }

	//! Get the source line
	RED_INLINE Uint32 GetSourceLine() const { return m_sourceLine; }

	//! Get generated code
	RED_INLINE const BufferU8& GetCode() const { return m_code; }

	//! Get generated breakpoints
	RED_INLINE const BufferU32& GetBreakpoints() const { return m_breakpoints; }

	//! Get generated savepoints
	RED_INLINE const BufferU32& GetSavepoints() const { return m_savepoints; }

public:
	CScriptCodeGenerator( const String& file, Uint32 line );
	~CScriptCodeGenerator();

	//! Get current offset
	Uint16 GetOffset() const;

	//! Emit general data to code stream
	void WriteData( const void* data, Uint32 size );

	//! Output opcode to code stream
	void WriteOpcode( EScriptOpcode opcode );

	//! Output byte to code stream
	void WriteByte( Uint8 data );

	//! Output word to code stream
	void WriteWord( Uint16 data );

	//! Output integer to code stream
	void WriteInt( Int32 data );

	//! Output float
	void WriteFloat( Float data );

	//! Output string
	void WriteString( const String& string );

	//! Output name
	void WriteName( CName name );

	//! Output pointer to object
	void WritePointer( void* object );

	//! Write label
	void WriteLabel( CScriptCodeNode* label );

public:
	//! Generate code from code node
	void GenerateCode( CScriptCodeNode* rootCodeNode );

private:
	CScriptCodeGenerator& operator=( const CScriptCodeGenerator& ) { return *this; }
};
