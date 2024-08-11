#include "build.h"

#include "googleanalytics.h"

#if !defined( NO_TELEMETRY )

//////////////////////////////////////////////////////////////////////////
//! This class is a singleton, so its creation is thread-safe
CEditorGoogleAnalyticsService::CEditorGoogleAnalyticsService() : CRedTelemetryService( TXT("google_analytics_service"), Telemetry::BT_GOOGLE_ANALYTICS )
{
	m_internalTimer = EngineTime::GetNow();
}

//////////////////////////////////////////////////////////////////////////
CEditorGoogleAnalyticsService::~CEditorGoogleAnalyticsService()
{
}

//////////////////////////////////////////////////////////////////////////
void CEditorGoogleAnalyticsService::SetScreen( const String& screenName )
{
	//LOG_ENGINE( TXT( "log screen %s" ), screenName.AsChar() );
	GetInterface()->Log( screenName, String::EMPTY );
}

//////////////////////////////////////////////////////////////////////////
void CEditorGoogleAnalyticsService::LogT( const String& name, const String& category )
{
	//LOG_ENGINE( TXT( "log name category %s/%s %ims" ), name.AsChar(), category.AsChar(), (Int32)((Double)(EngineTime::GetNow()-m_internalTimer)*1000.0) );
	GetInterface()->LogV( name, category, (Int32)((Double)(EngineTime::GetNow()-m_internalTimer)*1000.0) );
}

//////////////////////////////////////////////////////////////////////////
void CEditorGoogleAnalyticsService::LogV( const String& name, const String& category, Int32 value )
{
	//LOG_ENGINE( TXT( "log name category value %s/%s v: %i" ), name.AsChar(), category.AsChar(), value );
	GetInterface()->LogV( name, category, value );
}

//////////////////////////////////////////////////////////////////////////
void CEditorGoogleAnalyticsService::LogVL( const String& name, const String& category, Int32 value, const String& label )
{
	//LOG_ENGINE( TXT( "log name category value label %s/%s/%s v: %i" ), name.AsChar(), category.AsChar(), label.AsChar(), value );
	GetInterface()->LogVL( name, category, value, label );
}

#endif //NO_TELEMETRY