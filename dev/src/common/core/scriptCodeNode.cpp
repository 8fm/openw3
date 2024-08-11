/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "scriptCodeNode.h"
#include "scriptCodeGenerator.h"
#include "property.h"
#include "function.h"

CScriptCodeNode::CScriptCodeNode()
:	m_opcode( OP_Nop )
,	m_offset( 0 )
,	m_next( nullptr )
,	m_label( nullptr )
,	m_skipLabel( nullptr )
{
}

CScriptCodeNode::CScriptCodeNode( const CScriptFileContext& context, EScriptOpcode opcode )
:	m_context( context )
,	m_opcode( opcode )
,	m_offset( 0 )
,	m_next( nullptr )
,	m_label( nullptr )
,	m_skipLabel( nullptr )
{
}

CScriptCodeNode::~CScriptCodeNode()
{
}

void CScriptCodeNode::GenerateCode( CScriptCodeGenerator& generator )
{
	// Remember offset at which this opcode is placed
	m_offset = generator.GetOffset();
  
	// Label targets do not get opcode generated, it's only a marked
	if ( m_opcode == OP_Target )
	{
		return;
	}

	// Write opcode to stream
	generator.WriteOpcode( m_opcode );
  
	// Write opcode data 
	switch ( m_opcode )
	{
		// Breakpoint
		case OP_Breakpoint:
		{
			generator.WriteInt( m_context.m_line );
			generator.WriteByte( 0 );
			break;
		}

		// Byte constant
		case OP_ByteConst:
		{
			ASSERT( m_value.m_integer >= 0 && m_value.m_integer <= 255 );
			generator.WriteByte( (Uint8)m_value.m_integer );
			break;
		}   
   
		// Integer constant
		case OP_IntConst:
		{
			generator.WriteInt( m_value.m_integer );
			break;
		}
   
		// Short constant
		case OP_ShortConst:
		{
			Int16 shortValue = (Int16) m_value.m_integer;
			generator.WriteData( &shortValue, sizeof(shortValue) );
			break;
		}

		// Float constant
		case OP_FloatConst:
		{
			generator.WriteFloat( m_value.m_float );
			break;
		}

		// String constant
		case OP_StringConst:
		{
			generator.WriteString( m_value.m_string );
			break;
		}

		// Name constant
		case OP_NameConst:
		{
			CName nameToWrite( m_value.m_string );
			generator.WriteName( nameToWrite );
			break;
		}

		// Switch
		case OP_Switch:
		{
			ASSERT( m_label );
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			generator.WriteLabel( m_label );
			break;
		}
   
		// Switch label
		case OP_SwitchLabel:
		{
			ASSERT( m_skipLabel );
			generator.WriteLabel( m_skipLabel );
			ASSERT( m_label );
			generator.WriteLabel( m_label );
			break;
		}

		// Default switch
		case OP_SwitchDefault:
		{
			break;
		}

		// Jumps
		case OP_Jump:
		case OP_Skip:   
		case OP_JumpIfFalse:
		{
			ASSERT( m_label );
			generator.WriteLabel( m_label );
			break;   
		};   
   
		// Conditional expression
		case OP_Conditional:
		{
			ASSERT( m_label );
			generator.WriteLabel( m_label );
			ASSERT( m_skipLabel );
			generator.WriteLabel( m_skipLabel );
			break;
		}
   
		// Construct struct
		case OP_Constructor:
		{
			ASSERT( m_value.m_structure );
			generator.WriteByte( (Uint8)m_value.m_integer );
			generator.WritePointer( m_value.m_structure );
			break;
		};   

		// Function call
		case OP_FinalFunc:
		{
			ASSERT( m_value.m_function );
			generator.WriteLabel( m_label );
			generator.WriteWord( (Uint16)m_context.m_line );
			generator.WritePointer( m_value.m_function );
			break;
		}

		// Entry function call
		case OP_EntryFunc:
		{
			ASSERT( m_value.m_function );
			generator.WriteLabel( m_label );
			generator.WriteName( m_value.m_function->GetName() );
			break;
		}

		// Virtual function
		case OP_VirtualFunc:
		case OP_VirtualParentFunc:
		{
			ASSERT( m_value.m_function );
			generator.WriteLabel( m_label );
			generator.WriteWord( (Uint16)m_context.m_line );
			generator.WriteName( m_value.m_function->GetName() );
			break;
		}

		// Context
		case OP_Context:
		{
			ASSERT( m_value.m_integer < 65556 );
			generator.WriteLabel( m_label );
			break;
		}

		// Properties
		case OP_LocalVar:
		case OP_ParamVar:
		case OP_ObjectVar:
		case OP_ObjectBindableVar:
		case OP_DefaultVar:
		{
			ASSERT( m_value.m_property );
			generator.WritePointer( m_value.m_property );
			break;
		}

		// Dynamic casting
		case OP_DynamicCast:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		// Generic equality/inequality check
		case OP_TestEqual:
		case OP_TestNotEqual:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		// Access to structure member
		case OP_StructMember:
		{
			ASSERT( m_value.m_property );
			generator.WritePointer( m_value.m_property );
			break;
		}

		// Object creation
		case OP_New:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		case OP_EnumToString:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		case OP_EnumToInt:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		case OP_IntToEnum:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		// Array element
		case OP_ArrayElement:
		{
			ASSERT( m_value.m_type );
			generator.WritePointer( m_value.m_type );
			break;
		}

		// Array opcodes
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
		{
			ASSERT( m_value.m_type );
			ASSERT( m_value.m_type->GetType() == RT_Array );
			generator.WritePointer( m_value.m_type );
			break;
		}

		case OP_SavePoint:
		{
			ASSERT( m_label );
			ASSERT( !m_value.m_string.Empty() );

			generator.WriteLabel( m_label );

			CName nameToWrite( m_value.m_string );
			generator.WriteName( nameToWrite );

			break;
		}

		case OP_SaveValue:
		{
			ASSERT( !m_value.m_string.Empty() );

			CName nameToWrite( m_value.m_string );
			generator.WriteName( nameToWrite );

			break;
		}

		case OP_SavePointEnd:
		{
			break;
		}

		default:
		{
			// Ignored.
			break;
		}
	};   
}

CScriptCodeNode* CScriptCodeNode::Glue( CScriptCodeNode* left, CScriptCodeNode* right )
{
	// Easy case - no left part
	if ( !left )
	{
		return right;
	}

	// Easy case - no right part
	if ( !right )
	{
		return left;
	}

	// Find the last node in the left list
	CScriptCodeNode* last = left;
	while ( last->m_next )
	{
		last = last->m_next;
	}

	last->m_next = right;

	// Return head
	return left;
}

const Char* CScriptCodeNode::GetOpcodeName( EScriptOpcode opcode )
{
#define CHECK_OPCODE( x )	case x: return TXT(#x) + 3;

	switch ( opcode )
	{
		CHECK_OPCODE( OP_Nop );
		CHECK_OPCODE( OP_Null );
		CHECK_OPCODE( OP_IntOne );
		CHECK_OPCODE( OP_IntZero );
		CHECK_OPCODE( OP_IntConst );
		CHECK_OPCODE( OP_ShortConst );
		CHECK_OPCODE( OP_FloatConst );
		CHECK_OPCODE( OP_StringConst );
		CHECK_OPCODE( OP_NameConst );
		CHECK_OPCODE( OP_ByteConst );
		CHECK_OPCODE( OP_BoolTrue );
		CHECK_OPCODE( OP_BoolFalse );
		CHECK_OPCODE( OP_Target );
		CHECK_OPCODE( OP_LocalVar );
		CHECK_OPCODE( OP_ParamVar );
		CHECK_OPCODE( OP_DefaultVar );
		CHECK_OPCODE( OP_ObjectVar );
		CHECK_OPCODE( OP_ObjectBindableVar );
		CHECK_OPCODE( OP_Switch );
		CHECK_OPCODE( OP_SwitchLabel );
		CHECK_OPCODE( OP_SwitchDefault );
		CHECK_OPCODE( OP_Jump );
		CHECK_OPCODE( OP_JumpIfFalse );
		CHECK_OPCODE( OP_Skip );
		CHECK_OPCODE( OP_Conditional );
		CHECK_OPCODE( OP_Constructor );
		CHECK_OPCODE( OP_FinalFunc );
		CHECK_OPCODE( OP_VirtualFunc );
		CHECK_OPCODE( OP_VirtualParentFunc );
		CHECK_OPCODE( OP_ParamEnd );
		CHECK_OPCODE( OP_Return );
		CHECK_OPCODE( OP_StructMember );
		CHECK_OPCODE( OP_Context );
		CHECK_OPCODE( OP_Assign );
		CHECK_OPCODE( OP_TestEqual );
		CHECK_OPCODE( OP_TestNotEqual );
		CHECK_OPCODE( OP_New );
		CHECK_OPCODE( OP_Delete );
		CHECK_OPCODE( OP_This );
		CHECK_OPCODE( OP_Parent );
		CHECK_OPCODE( OP_SavePoint );
		CHECK_OPCODE( OP_SaveValue);
		CHECK_OPCODE( OP_SavePointEnd );
		CHECK_OPCODE( OP_Breakpoint );
		CHECK_OPCODE( OP_BoolToByte );
		CHECK_OPCODE( OP_BoolToInt );
		CHECK_OPCODE( OP_BoolToFloat );
		CHECK_OPCODE( OP_BoolToString );
		CHECK_OPCODE( OP_ByteToBool );
		CHECK_OPCODE( OP_ByteToInt );
		CHECK_OPCODE( OP_ByteToFloat );
		CHECK_OPCODE( OP_ByteToString );
		CHECK_OPCODE( OP_IntToBool );
		CHECK_OPCODE( OP_IntToByte );
		CHECK_OPCODE( OP_IntToFloat );
		CHECK_OPCODE( OP_IntToString );
		CHECK_OPCODE( OP_IntToEnum );
		CHECK_OPCODE( OP_FloatToBool );
		CHECK_OPCODE( OP_FloatToByte );
		CHECK_OPCODE( OP_FloatToInt );
		CHECK_OPCODE( OP_FloatToString );
		CHECK_OPCODE( OP_NameToBool );
		CHECK_OPCODE( OP_NameToString );
		CHECK_OPCODE( OP_StringToBool );
		CHECK_OPCODE( OP_StringToByte );
		CHECK_OPCODE( OP_StringToInt );
		CHECK_OPCODE( OP_StringToFloat );
		CHECK_OPCODE( OP_ObjectToBool );
		CHECK_OPCODE( OP_ObjectToString );
		CHECK_OPCODE( OP_EnumToString );
		CHECK_OPCODE( OP_EnumToInt );
		CHECK_OPCODE( OP_DynamicCast );
		CHECK_OPCODE( OP_ArrayClear );
		CHECK_OPCODE( OP_ArraySize );
		CHECK_OPCODE( OP_ArrayResize );
		CHECK_OPCODE( OP_ArrayFindFirst );
		CHECK_OPCODE( OP_ArrayFindFirstFast );
		CHECK_OPCODE( OP_ArrayFindLast );
		CHECK_OPCODE( OP_ArrayFindLastFast );
		CHECK_OPCODE( OP_ArrayContains );
		CHECK_OPCODE( OP_ArrayContainsFast );
		CHECK_OPCODE( OP_ArrayPushBack );
		CHECK_OPCODE( OP_ArrayPopBack );
		CHECK_OPCODE( OP_ArrayInsert );
		CHECK_OPCODE( OP_ArrayRemove );
		CHECK_OPCODE( OP_ArrayRemoveFast );
		CHECK_OPCODE( OP_ArrayGrow );
		CHECK_OPCODE( OP_ArrayErase );
		CHECK_OPCODE( OP_ArrayEraseFast );
		CHECK_OPCODE( OP_ArrayLast );
		CHECK_OPCODE( OP_ArrayElement );
		CHECK_OPCODE( OP_EntryFunc );
		CHECK_OPCODE( OP_GetGame );
		CHECK_OPCODE( OP_GetPlayer );
		CHECK_OPCODE( OP_GetCamera );
		CHECK_OPCODE( OP_GetHud );
		CHECK_OPCODE( OP_GetSound );
		CHECK_OPCODE( OP_GetDebug );
		CHECK_OPCODE( OP_GetTimer );
		CHECK_OPCODE( OP_GetInput );
		CHECK_OPCODE( OP_GetTelemetry );
	}

#undef CHECK_OPCODE

	HALT( "Unknown opcode" );
	return TXT("Unknown");
}

void CScriptCodeNode::PrintTree()
{
	CScriptCodeNode* node = this;

	// Show next item
	while ( node )
	{
		node->PrintMe();

		node = node->m_next;
	}
}

void CScriptCodeNode::PrintMe()
{
	String out;

	// Property info
	if ( m_value.m_property )
	{   
		out += TXT(" (prop: ");
		out += m_value.m_property->GetName().AsString();
		out += TXT(")");
	}

	// Property info
	if ( m_value.m_function )
	{   
		out += TXT(" (func: ");
		out += m_value.m_function->GetName().AsString();
		out += TXT(")");
	}

	// Extra
	if ( m_opcode == OP_IntConst )
	{
		out += TXT(" ");
		out += ToString( m_value.m_integer );
	}
	else if ( m_opcode == OP_ShortConst )
	{
		out += TXT(" ");
		out += ToString( m_value.m_integer );
	}
	else if ( m_opcode == OP_FloatConst )
	{
		out += TXT(" ");
		out += ToString( m_value.m_float );
	}
	else if ( m_opcode == OP_StringConst )
	{
		out += TXT(" ");
		out += m_value.m_string;
	}
	else if ( m_opcode == OP_NameConst )
	{
		out += TXT(" ");
		out += m_value.m_string;
	}
	else if ( m_opcode == OP_ByteConst )
	{
		out += TXT(" ");
		out += ToString( m_value.m_integer );
	}
	else if ( m_opcode == OP_Context )
	{
		out += TXT(" typeSize:");
		out += ToString( m_value.m_integer );
	}

	// Label offset
	if ( m_label )
	{
		out += String::Printf( TXT("   Label 0x%X"), m_label );
	}

	// Show line
	LOG_CORE( TXT("0x%X:  %ls %ls"), this, GetOpcodeName( m_opcode ), out.AsChar() );
}
