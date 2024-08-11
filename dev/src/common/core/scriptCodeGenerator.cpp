/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptCodeNode.h"
#include "scriptCodeGenerator.h"
#include "uniPointer.h"

CScriptCodeGenerator::CScriptCodeGenerator(  const String& file, Uint32 line  )
	: m_sourceFile( file )
	, m_sourceLine( line )
{
}

CScriptCodeGenerator::~CScriptCodeGenerator()
{
}

Uint16 CScriptCodeGenerator::GetOffset() const
{
	return (Uint16) m_code.Size();
}

void CScriptCodeGenerator::WriteData( const void* data, Uint32 size )
{
	Uint32 base = static_cast< Uint32 >( m_code.Grow( size ) );
	Red::System::MemoryCopy( &m_code[ base ], data, size );
}

void CScriptCodeGenerator::WriteOpcode( EScriptOpcode opcode )
{
	// Breakpoint opcode
	if ( opcode == OP_Breakpoint )
	{
		const Uint32 offset = m_code.Size();
		m_breakpoints.PushBack( offset );	
	}

	// SavePoint opcode
	if ( opcode == OP_SavePoint )
	{
		const Uint32 offset = m_code.Size();
		m_savepoints.PushBack( offset );
	}

	// Write opcode
	ASSERT( opcode < OP_Max );
	WriteByte( (Uint8) opcode );
}

void CScriptCodeGenerator::WriteByte( Uint8 data )
{
	WriteData( &data, sizeof(data) );
}

void CScriptCodeGenerator::WriteWord( Uint16 data )
{
	WriteData( &data, sizeof(data) );
}

void CScriptCodeGenerator::WriteInt( Int32 data )
{
	WriteData( &data, sizeof(data) );
}

void CScriptCodeGenerator::WriteFloat( Float data )
{
	WriteData( &data, sizeof(data) );
}

void CScriptCodeGenerator::WriteString( const String& string )
{
	// save the string length
	const Uint32 length = string.GetLength();
	WriteData( &length, sizeof(length) );

	// save string chars directly in the code stream
	for ( Uint32 i=0; i<length; ++i )
	{
		const Uint16 chr = (const Uint16) string[i];
		WriteData( &chr, sizeof(chr) );
	}
}

void CScriptCodeGenerator::WriteName( CName name )
{
	Uint32 start = static_cast< Uint32 >( m_code.Grow( sizeof(CName) ) );
	Red::System::MemorySet( &m_code[ start ], 0, sizeof(CName) );
	*(CName*)(&m_code[ start ]) = name;
}

void CScriptCodeGenerator::WritePointer( void* object )
{
	TUniPointer< void > pointer( object );
	WriteData( &pointer, sizeof(pointer) );
}

void CScriptCodeGenerator::WriteLabel( CScriptCodeNode* label )
{
	ASSERT( label );

	// Write placeholder for offset
	Uint16 offset = (Uint16)m_code.Size();
	WriteWord( 0 );

	// Remember label
	CodeLabel codeLabel;
	codeLabel.m_label = label;
	codeLabel.m_writeOffset = offset;
	m_labels.PushBack( codeLabel );
}

void CScriptCodeGenerator::GenerateCode( CScriptCodeNode* rootCodeNode )
{
	// Clear
	m_code.Clear();
	m_labels.Clear();

	// Process all nodes
	for ( CScriptCodeNode* cur = rootCodeNode; cur; cur = cur->m_next )
	{
		cur->GenerateCode( *this );
	}

	// Add NOP at end
	WriteOpcode( OP_Nop );

	// Finalize labels
	for ( Uint32 i=0; i<m_labels.Size(); i++ )
	{
		const CodeLabel& label = m_labels[i];

		// Fix jump offset
		Int16* write = (Int16* )&m_code[ label.m_writeOffset ];
		*write = (Int16)( (Int32)label.m_label->m_offset - (Int32)( label.m_writeOffset + 2 ) );
	}
}
