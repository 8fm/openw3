#include "build.h"
#include "scriptThreadSerializer.h"
#include "gameSave.h"
#include "scriptStackFrame.h"

///////////////////////////////////////////////////////////////////////////////

CScriptThreadSerializer::~CScriptThreadSerializer()
{
	m_params.ClearPtr();
}

void CScriptThreadSerializer::Record( const CName& savepointName, const TDynArray< SSavePointValue* >& params )
{
	m_params.ClearPtr();
	m_name = savepointName;
	m_params = params;
}

void CScriptThreadSerializer::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(savePointData) );

	// Save name of the save point
	saver->WriteValue( CNAME( name ), m_name );

	// Save properties
	const Uint32 count = m_params.Size();
	saver->WriteValue( CNAME(propertyCount), count );
	for ( Uint32 i=0; i<m_params.Size(); i++ )
	{
		SSavePointValue* val = m_params[i];
		val->SaveState( saver );
	}
}

void CScriptThreadSerializer::RestoreState( CScriptStackFrame& stack, IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(savePointData) );

	// Load name of the save point
	loader->ReadValue( CNAME( name ), m_name );

	// Clear current crap
	m_params.ClearPtr();

	// Load properties
	const Uint32 count = loader->ReadValue< Uint32 >( CNAME(propertyCount) );
	for ( Uint32 i=0; i<count; i++ )
	{
		SSavePointValue* val = new SSavePointValue();
		val->RestoreState( loader );
		m_params.PushBack( val );
	}

	// Restore thread state
	RestoreThreadState( stack );
}

void CScriptThreadSerializer::RestoreThreadState( CScriptStackFrame& stack )
{
	const CFunction* scriptFunction = stack.m_function;
	ASSERT( scriptFunction, TXT( "Invalid script function" ) );

	// restore the values of function properties
	for ( TDynArray< SSavePointValue* >::iterator it = m_params.Begin(); it != m_params.End(); ++it )
	{
		SSavePointValue* saveValue = *it;

		// Find property
		CProperty* prop = scriptFunction->FindProperty( saveValue->m_name );
		if ( !prop )
		{
			WARN_CORE( TXT( "Script thread deserialization: Required property '%ls' doesn't exist in the present verison of function '%ls'" ), saveValue->m_name.AsString().AsChar(), scriptFunction->GetName().AsString().AsChar() );
			continue;
		}

		// Verify that its type hasn't changed
		if ( saveValue->m_type != prop->GetType()->GetName() )
		{
			WARN_CORE( TXT( "Script thread deserialization: Type of '%ls' property has changed from '%ls' to '%ls" ), saveValue->m_name.AsString().AsChar(), saveValue->m_type.AsString().AsChar(), prop->GetType()->GetName().AsString().AsChar());
			continue;
		}

		// recover the property value
		const void* srcData = saveValue->m_value.GetData();
		if ( prop->IsFuncLocal() )
		{
			prop->Set( (void*)stack.m_locals, srcData );
		}
		else if ( prop->IsInFunction() )
		{
			prop->Set( (void*)stack.m_params, srcData );
		}
	}

	// set new code start point
	stack.m_code = scriptFunction->GetCode().GetCodeBehindSavePoint( m_name );
}

///////////////////////////////////////////////////////////////////////////////

SSavePointValue::SSavePointValue()
	: m_name( CName::NONE )
	, m_type( CName::NONE )
{
}

SSavePointValue::SSavePointValue( CScriptStackFrame& frame, CProperty& property )
{
	m_name = property.GetName();

	IRTTIType* propType = property.GetType();
	m_type = propType->GetName();

	m_value.Init( propType->GetName(), NULL );

	if ( property.IsFuncLocal() )
	{
		property.Get( (void*)frame.m_locals, m_value.GetData() );
	}
	else if ( property.IsInFunction() )
	{
		property.Get( (void*)frame.m_params, m_value.GetData() );
	}
}

void SSavePointValue::SaveState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(savePointValue) );

	// Save name and type
	saver->WriteValue( CNAME(name), m_name );
	saver->WriteValue( CNAME(type), m_type );

	// Save value
	IRTTIType* type = m_value.GetRTTIType();
	ASSERT( type );
	saver->WriteValue( CNAME(value), type, m_value.GetData() );
}

void SSavePointValue::RestoreState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(savePointValue) );

	// Load name and type
	loader->ReadValue( CNAME(name), m_name );
	loader->ReadValue( CNAME(type), m_type );

	// Initialize type
	m_value.Init( m_type, NULL );
	ASSERT( m_value.GetRTTIType() );

	// Load
	loader->ReadValue( CNAME(value), m_value.GetRTTIType(), m_value.GetData(), NULL );
}

///////////////////////////////////////////////////////////////////////////////
