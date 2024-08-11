/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "scriptsOpcodeExaminer.h"
#include "scriptCodeNode.h"

CScriptOpCodeExaminer::CScriptOpCodeExaminer()
{

}

CScriptOpCodeExaminer::~CScriptOpCodeExaminer()
{

}

template< typename T >
RED_INLINE void CScriptOpCodeExaminer::Skip( const Uint8*& position )
{
	position += sizeof( T );
}

template< typename T >
RED_INLINE void CScriptOpCodeExaminer::Peek( const Uint8* position, T& value )
{
	Red::System::MemoryCopy( &value, position, sizeof( T ) );
}

template< typename T >
RED_INLINE void CScriptOpCodeExaminer::Read( const Uint8*& position, T& value )
{
	Peek( position, value );
	Skip< T >( position );
}

template< typename T >
RED_INLINE String CScriptOpCodeExaminer::ReadConst( const Uint8*& position )
{
	T data;
	Read( position, data );
	return ToString( data );
}

RED_INLINE String CScriptOpCodeExaminer::ReadStringConst( const Uint8*& position )
{
	Uint32 length;
	Read( position, length );

	String value;
	value.Resize( length + 1 );

	Uint32 sizeOfBufferBeingRead = length * sizeof( Char );

	Red::System::MemoryCopy( value.TypedData(), position, sizeOfBufferBeingRead );
	value[ length ] = TXT( '\0' );

	position += sizeOfBufferBeingRead;

	return value;
}

void CScriptOpCodeExaminer::Examine( const CFunction* function )
{
	const CScriptCompiledCode& code = function->GetCode();

	m_output.Grow( 1 );
	DisassembledFunction& disassembledFunction = m_output.Back();
	disassembledFunction.m_file = code.GetSourceFile();

	const Uint8* start		= code.GetCode();
	const Uint8* end		= code.GetCodeEnd();
	const Uint8* position	= start;

	while ( position < end )
	{
		EScriptOpcode opcode = ReadOpcode( position );

		// use simplified dump in cooker
		String prefix = 
			GIsCooker	? String::Printf( TXT( "  (%4u): " ), static_cast< Uint32 >( position - start ) )
						: String::Printf( TXT( "%p (%4u): " ), position, static_cast< Uint32 >( position - start ) );

		String details;

		switch( opcode )
		{
		case OP_Breakpoint:
			StartNewLine();
			ReadBreakpoint( position );
			break;

		case OP_ByteConst:
			details += ReadConst< Uint8 >( position );
			break;

		case OP_IntConst:
			details += ReadConst< Uint32 >( position );
			break;

		case OP_ShortConst:
			details += ReadConst< Int16 >( position );
			break;

		case OP_FloatConst:
			details += ReadConst< Float >( position );
			break;

		case OP_NameConst:
			details += ReadConst< CName >( position );
			break;

		case OP_StringConst:
			details += ReadStringConst( position );
			break;

		case OP_Switch:
			Skip< TUniPointer< CClass > >( position );
			Skip< Uint16 >( position );
			break;

		case OP_SwitchLabel:
			StartNewLine();
			details += ReadSwitchLabel( position );
			break;

		case OP_Jump:
		case OP_Skip:
		case OP_JumpIfFalse:
			Skip< Uint16 >( position );
			break;

		case OP_Conditional:
			Skip< Uint16 >( position );
			Skip< Uint16 >( position );
			break;

		case OP_Constructor:
			Skip< Uint8 >( position );
			Skip< TUniPointer< CClass > >( position );
			break;

		case OP_FinalFunc:
			Skip< Uint16 >( position );
			Skip< Uint16 >( position );
			Skip< TUniPointer< CFunction > >( position );
			break;

		case OP_EntryFunc:
			Skip< Uint16 >( position );
			Skip< CName >( position );
			break;

		case OP_VirtualFunc:
		case OP_VirtualParentFunc:
			Skip< Uint16 >( position );
			Skip< Uint16 >( position );
			Skip< CName >( position );
			break;

		case OP_Context:
			Skip< Uint16 >( position );
			break;

		case OP_LocalVar:
		case OP_ParamVar:
		case OP_ObjectVar:
		case OP_ObjectBindableVar:
		case OP_DefaultVar:
		case OP_StructMember:
			{
				TUniPointer< CProperty > prop;
				Read( position, prop );

				prop->GetName();

				const CClass* parentClass = prop->GetParent();

				if( parentClass )
				{
					details += parentClass->GetName().AsChar();
					details += TXT( "::" );
				}

				details += prop->GetName().AsChar();
			}


			break;

		case OP_DynamicCast:
		case OP_TestEqual:
		case OP_TestNotEqual:
		case OP_New:
		case OP_EnumToString:
		case OP_EnumToInt:
		case OP_IntToEnum:

		case OP_ArrayElement:
		case OP_ArrayClear:
		case OP_ArraySize:
		case OP_ArrayResize:
		case OP_ArrayFindFirst:
		case OP_ArrayFindFirstFast:
		case OP_ArrayFindLast:
		case OP_ArrayFindLastFast:
		case OP_ArrayContains:
		case OP_ArrayContainsFast:
		case OP_ArrayPushBack:
		case OP_ArrayPopBack:
		case OP_ArrayInsert:
		case OP_ArrayRemove:
		case OP_ArrayRemoveFast:
		case OP_ArrayGrow:
		case OP_ArrayErase:
		case OP_ArrayEraseFast:
		case OP_ArrayLast:
			Skip< TUniPointer< CClass > >( position );
			break;

		case OP_SavePoint:
			Skip< Uint16 >( position );
			Skip< CName >( position );
			break;

		case OP_SaveValue:
			Skip< CName >( position );
			break;

		default:
			{
			}
		}

		if ( disassembledFunction.m_lines.Empty() )
			StartNewLine();

		DisassembledLine& line = disassembledFunction.m_lines.Back();

		line.m_details += prefix;
		line.m_details += CScriptCodeNode::GetOpcodeName( opcode );
		line.m_details += TXT( " " );
		line.m_details += details;
		line.m_details += TXT( "\n" );
	}
}

EScriptOpcode CScriptOpCodeExaminer::ReadOpcode( const Uint8*& position )
{
	Uint8 opcode;
	Read( position, opcode );

	return static_cast< EScriptOpcode >( opcode );
}

void CScriptOpCodeExaminer::StartNewLine()
{
	DisassembledFunction& disassembledFunction = m_output.Back();
	disassembledFunction.m_lines.Grow( 1 );
}

void CScriptOpCodeExaminer::ReadBreakpoint( const Uint8*& position )
{
	DisassembledLine& line = m_output.Back().m_lines.Back();

	// Read line from buffer
	Read( position, line.m_line );

	// Ignore flags byte (we don't need to know if the breakpoint is currently set or not)
	Skip< Uint8 >( position );
}

String CScriptOpCodeExaminer::ReadSwitchLabel( const Uint8*& position )
{
	String details;

	Uint16 skip;
	Read( position, skip );
	details = GIsCooker ? String::Printf( TXT( "Skip: (%hu)" ), skip ) : String::Printf( TXT( "Skip: %p (%hu)" ), position + skip, skip );

	Uint16 jump;
	Read( position, jump );
	details += GIsCooker ? String::Printf( TXT( "Jump: (%hu)" ), jump ) : String::Printf( TXT( "Jump: %p (%hu)" ), position + jump, jump );

	return details;
}
