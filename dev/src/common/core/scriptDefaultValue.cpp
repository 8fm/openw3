/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptDefaultValue.h"
#include "scriptingSystem.h"

CScriptDefaultValue::CScriptDefaultValue( const CScriptFileContext* context, const String& value )
:	m_context( *context )
,	m_value( value )
{
}

CScriptDefaultValue::CScriptDefaultValue( const CScriptFileContext* context, CName name, const String& value )
:	m_context( *context )	
,	m_value( value )
,	m_name( name )
{
}

CScriptDefaultValue::~CScriptDefaultValue()
{
	m_subValues.ClearPtr();
}

void CScriptDefaultValue::SetName( CName name )
{
	ASSERT( !m_name );
	ASSERT( name );
	m_name = name;
}

void CScriptDefaultValue::AddSubValue( CScriptDefaultValue* value )
{
	ASSERT( value );
	m_subValues.PushBack( value );
}

String CScriptDefaultValue::ToString() const
{
	if ( IsComplex() )
	{
		String out;
		
		// Add header
		if ( m_value.Empty() )
		{
			// Structure header
			out = TXT("{ ");
		}
		else
		{
			// Object header
			out = m_value;
			out += TXT("( ");
		}

		// Emit sub values
		const Uint32 numSubValues = m_subValues.Size();
		for ( Uint32 i=0; i<numSubValues; i++ )
		{
			// Add value
			CScriptDefaultValue* val = m_subValues[i];
			out += val->ToString();

			// Add separator
			if ( i < (numSubValues-1) )
			{
				out += TXT(", ");
			}
		}

		// Add tail
		if ( m_value.Empty() )
		{
			// Structure tail
			out += TXT(" }");
		}
		else
		{
			// Object tail
			out += TXT(" )");
		}

		// Done
		return out;
	}
	else if ( IsNamed() )
	{
		// Named value
		return String::Printf( TXT("%ls = %ls"), m_name.AsString().AsChar(), m_value.AsChar() );
	}
	else
	{
		// Unnamed value
		return m_value;
	}
}
