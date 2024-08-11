#pragma once

#if !defined( NO_TELEMETRY )

#include "../../common/core/redTelemetryService.h"

//! This class serves as access point for Google Analytics Service interface.
//! It spawns jobs to update CEditorGoogleAnalyticsService in background
//! Google Analytics setup:
//! ACCOUNT: GA_TEST
//! PROPERTY: GoogleAnalyticsConfig.ini  -> [Game] -> Id
//! App Version: GoogleAnalyticsConfig.ini  -> [Game] -> Version
//! Tracking ID: GoogleAnalyticsConfig.ini  -> [General] -> GoogleTrackingID
//! Custom Dimension:
class CEditorGoogleAnalyticsService: public CRedTelemetryService
{
public:
	CEditorGoogleAnalyticsService();
	~CEditorGoogleAnalyticsService();
	
	// Google Analytics -> Behaviour -> Screens
	void SetScreen( const String& screenName /*Google Analytics -> Screen Name */ );

	// Google Analytics -> Behaviour -> Events
	void LogT( const String& name/*Google Analytics -> Event Action */, const String& category /*Google Analytics -> Event Category */ );
	void LogV( const String& name/*Google Analytics -> Event Action */, const String& category /*Google Analytics -> Event Category */, Int32 value /* Google Analytics -> Event Value */ );
	void LogVL( const String& name/*Google Analytics -> Event Action */, const String& category /*Google Analytics -> Event Category */, Int32 value /* Google Analytics -> Event Value */, const String& label /* Google Analytics -> Event Label */);

protected:
	EngineTime m_internalTimer;
};

typedef TSingleton< CEditorGoogleAnalyticsService, TDefaultLifetime, TCreateUsingNew > SEditorGoogleAnalyticsService;

#define ANALYTICS_INIT() SEditorGoogleAnalyticsService::GetInstance();
#define ANALYTICS_CALL( func ) SEditorGoogleAnalyticsService::GetInstance().func;
#define ANALYTICS_SCREEN( name ) SEditorGoogleAnalyticsService::GetInstance().SetScreen( TXT( name ) );
#define ANALYTICS_EVENT( category, name ) SEditorGoogleAnalyticsService::GetInstance().LogV( TXT( name ), TXT( category ), 0 );
#define ANALYTICS_EVENT_T( category, name ) SEditorGoogleAnalyticsService::GetInstance().LogT( TXT( name ), TXT( category ) );
#define ANALYTICS_EVENT_V( category, name, value ) SEditorGoogleAnalyticsService::GetInstance().LogV( TXT( name ), TXT( category ), value );
#define ANALYTICS_EVENT_VL( category, name, label, value ) SEditorGoogleAnalyticsService::GetInstance().LogVL( TXT( name ), TXT( category ), value, TXT( label ) );

#else

#define ANALYTICS_INIT()
#define ANALYTICS_CALL( func )
#define ANALYTICS_SCREEN( name )
#define ANALYTICS_EVENT( category, name )
#define ANALYTICS_EVENT_T( category, name )
#define ANALYTICS_EVENT_V( category, name, value )
#define ANALYTICS_EVENT_VL( category, name, label, value )

#endif //NO_TELEMETRY
