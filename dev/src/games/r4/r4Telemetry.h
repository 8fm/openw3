#pragma once

#include "../../common/core/redTelemetryService.h"
#include "../../common/core/redTelemetryServiceDDI.h"

#include "r4TelemetryScriptProxy.h"

#if !defined( NO_TELEMETRY )

struct Event
{
	const Char*			category;
	const Char*			name;
};

//! This class serves as access point for telemetry interface.
//! It spawns jobs to update telemetry in background
//! It translates game-specific events for telemetry strings
class CR4Telemetry: public IGameSystem, public IGameSaveSection, public IRedTelemetryServiceDDIDelegate
{

	DECLARE_ENGINE_CLASS( CR4Telemetry, IGameSystem, 0 )

	enum ECountMethod
	{
		ECM_One,
		ECM_Inc,
		ECM_Max,
		ECM_Min
	};
public:
	CR4Telemetry();
	~CR4Telemetry();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	void LoadConfigs();

	Bool StartCollecting();
	Bool StopCollecting();

	void InitializeDebugWindows();

	void Log( ER4TelemetryEvents gameEvent );
	void LogL( ER4TelemetryEvents gameEvent, const String& label );
	void LogV( ER4TelemetryEvents gameEvent, Int32 value );
	void LogV( ER4TelemetryEvents gameEvent, const String& value );
	void LogVL( ER4TelemetryEvents gameEvent, Int32 value, const String& label );
	void LogVL( ER4TelemetryEvents gameEvent, const String& value, const String& label );

	void LogException( const String& exception );

	void SetStatValue( ER4CommonStats stat, Int32 value );
	void SetStatValue( ER4CommonStats stat, Float value );
	void SetStatValue( ER4CommonStats stat, const String& value );

	Bool RemoveSessionTag( const String& tag );
	void RemoveAllSessionTags();
	void AddSessionTag( const String& tag );

private:
	void OnUserEvent( const EUserEvent& event );

private:
#if !defined( NO_TELEMETRY_DEBUG )
	void GetDebugStatistics();
#endif

	void GetPlayerPosition();
	void GetWorldId();

	Uint32 GetHash( const String& str );

	void EventCategoryToString( ER4TelemetryEvents category, String& categoryStr, String& nameStr ) const;
	void CommonStatToString( ER4CommonStats stat, String& statStr ) const;

	// CRedTelemetryService
private:
	void OnGameStart( const CGameInfo& gameInfo );
	void OnGameEnd( const CGameInfo& gameInfo );

	// IGameSystem
private:
	bool OnSaveGame( IGameSaver* saver );
	void OnLoadGame( IGameLoader* loader );

	//! IRedTelemetryServiceDDIDelegate
private:
	virtual void LogInt32( const String& name, Int32 value ) override;
	virtual void LogFloat( const String& name, Float value ) override;

protected:
	ASSIGN_GAME_SYSTEM_ID( GS_Telemetry )

private:
	String m_loadedSesssionId;
	String m_playerId;
	Bool   m_gameStarted;


	THashMap< String, CRedTelemetryService* > m_telemetryServices;
	
	Events::TListenerHandle m_userEventHandle;


	THashMap< String, ECountMethod > m_ddiStatCountMethod;
};

BEGIN_CLASS_RTTI( CR4Telemetry )
	PARENT_CLASS( IGameSystem )
END_CLASS_RTTI();
#endif
