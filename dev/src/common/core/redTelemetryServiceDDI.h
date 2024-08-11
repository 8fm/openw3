#pragma once

#include "redTelemetryServiceInterface.h"
#include "string.h"
#include "hashmap.h"

class IRedTelemetryServiceDDIDelegate
{
public:
	virtual void LogInt32( const String& name, Int32 value ) = 0;
	virtual void LogFloat( const String& name, Float value ) = 0;
};

//! This interface serves only as a wrapper to for Digital Distribution Interface Lib
class CRedTelemetryServiceDDI: public IRedTelemetryServiceInterface
{
public:

	CRedTelemetryServiceDDI();
	virtual ~CRedTelemetryServiceDDI() {}

	IRedTelemetryServiceDDIDelegate* GetDelegate() const { return m_delegate; }
	void SetDelegate( IRedTelemetryServiceDDIDelegate* val ) { m_delegate = val; }

	RED_FORCE_INLINE Bool IsEventFiltered( const String& name ) const { return m_filteredEvents.KeyExist( name ); }

	//! Those methods are lightweight, they only puts event to lock-less queue
	virtual void Log	( const String& name, const String& category );
	virtual void LogL	( const String& name, const String& category, const String& label );
	virtual void LogV	( const String& name, const String& category, const String& value );
	virtual void LogV	( const String& name, const String& category, Int32 value );
	virtual void LogVL	( const String& name, const String& category, const String& value, const String& label );
	virtual void LogVL	( const String& name, const String& category, Int32 value, const String& label );

	static const Char* s_serviceName; 
	THashMap< String, Bool > m_filteredEvents;
private: 
	IRedTelemetryServiceDDIDelegate* m_delegate;
};
