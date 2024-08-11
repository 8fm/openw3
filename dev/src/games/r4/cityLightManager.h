/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "r4SystemOrder.h"
#include "gameplayLightComponent.h"
#include "../../common/engine/budgetedContainer.h"

class CGameplayLightComponent;
class CWeatherManager;


// Designated city are toggled on/off
class CCityLightManager : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CCityLightManager, IGameSystem, 0 );	

private:
	typedef TCustomIntrusiveList< CGameplayLightComponent, CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( CGameplayLightComponent, lights ) > LightsContainer;
	typedef LightsContainer::safe_iterator LightsIterator;
	
	Float	m_deltaTotal;

	Bool	m_isEnabled;
	Bool    m_cityLightsOn;
	Bool	m_weatherLightsOn;
	Bool	m_updateEnabled;

	Uint32	m_nightStart;
	Uint32	m_nightEnd;

	CWeatherManager* m_weatherManager;

	LightsContainer m_lights;
	LightsIterator m_lightsIterator;
	Float m_lightToggleTimer;

	void UpdateToggleLights( Bool immediate = false );
	void ToggleLight( CGameplayLightComponent* glComponent, Bool immediate = false );
	void ToggleLights( Bool immediate );
	void TickToggleLights( Float timeDelta );

	Bool GetWeatherConditions();
	Bool GetTODConditions();


public:
	CCityLightManager();
	
	void SetEnabled(Bool toggle)	{ m_isEnabled = toggle; }
	Bool IsEnabled() const				{ return m_isEnabled; }				

	RED_FORCE_INLINE void RegisterComponent( CGameplayLightComponent* glComponent ) { m_lights.PushBack( glComponent ); ToggleLight( glComponent ); } //if we discover new lights as we explore, we should toggle them according to the current conditions (daylight + weather)
	RED_FORCE_INLINE void UnregisterComponent( CGameplayLightComponent* glComponent ) { m_lights.Remove( glComponent ); }

	virtual void OnWorldStart( const CGameInfo& gameInfo );
	virtual void Tick( Float timeDelta );

	ASSING_R4_GAME_SYSTEM_ID( GSR4_CityLightManager );

private:

	void funcSetEnabled			( CScriptStackFrame& stack, void* result );
	void funcIsEnabled			( CScriptStackFrame& stack, void* result );
	void funcForceUpdate		( CScriptStackFrame& stack, void* result );
	void funcSetUpdateEnabled	( CScriptStackFrame& stack, void* result );
	void funcDebugToggleAll		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCityLightManager )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "SetEnabled", funcSetEnabled );
	NATIVE_FUNCTION( "IsEnabled", funcIsEnabled );
	NATIVE_FUNCTION( "ForceUpdate", funcForceUpdate );
	NATIVE_FUNCTION( "SetUpdateEnabled", funcSetUpdateEnabled );
	NATIVE_FUNCTION( "DebugToggleAll", funcDebugToggleAll );
END_CLASS_RTTI();

