#include "build.h"
#include "redTelemetryServiceDDI.h"

const Char* CRedTelemetryServiceDDI::s_serviceName = TXT("ddi");

void CRedTelemetryServiceDDI::Log( const String& name, const String& category )
{
	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() ); 
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, 1 );
	}
}

void CRedTelemetryServiceDDI::LogL( const String& name, const String& category, const String& label )
{
	RED_UNUSED( label );

	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() ); 
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, 1 );
	}
}

void CRedTelemetryServiceDDI::LogV( const String& name, const String& category, const String& value )
{
	RED_UNUSED( value );

	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() ); 
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, 1 );
	}
}

void CRedTelemetryServiceDDI::LogV( const String& name, const String& category, Int32 value )
{
	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() ); 
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, value );
	}
}

void CRedTelemetryServiceDDI::LogVL( const String& name, const String& category, const String& value, const String& label )
{
	RED_UNUSED( value );
	RED_UNUSED( label );

	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() ); 
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, 1 );
	}
}

void CRedTelemetryServiceDDI::LogVL( const String& name, const String& category, Int32 value, const String& label )
{
	RED_UNUSED( label );

	if( IsEventFiltered( name ) )
		return;

	String ddiStatName = String::Printf( TXT("%ls_%ls"), category.AsChar(), name.AsChar() );  
	if( m_delegate )
	{
		m_delegate->LogInt32( ddiStatName, value );
	}
}

CRedTelemetryServiceDDI::CRedTelemetryServiceDDI() : m_delegate( nullptr )
{
	m_filteredEvents[TXT("foff")] = true;
	m_filteredEvents[TXT("hhsl")] = true;
	m_filteredEvents[TXT("hhsr")] = true;
	m_filteredEvents[TXT("au")] = true;
	m_filteredEvents[TXT("pm")] = true;
	m_filteredEvents[TXT("pp")] = true;
	m_filteredEvents[TXT("ed")] = true;
	m_filteredEvents[TXT("glo")] = true;
	m_filteredEvents[TXT("gs")] = true;
	m_filteredEvents[TXT("glu")] = true;
	m_filteredEvents[TXT("gp")] = true;
	m_filteredEvents[TXT("gup")] = true;
	m_filteredEvents[TXT("gprs")] = true;
	m_filteredEvents[TXT("unk")] = true;
	m_filteredEvents[TXT("qc")] = true;
}
