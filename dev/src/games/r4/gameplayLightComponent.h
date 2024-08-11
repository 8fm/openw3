/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/customIntrusiveList.h"
#include "../../common/engine/renderFrame.h"

class CCityLightManager;

class CGameplayLightComponent : public CInteractionComponent
{
	friend class CCityLightManager;

	DECLARE_ENGINE_CLASS( CGameplayLightComponent, CInteractionComponent, 0 )
	DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( CGameplayLightComponent, lights )

private:
	Bool	m_isLightOn;
	Bool	m_isCityLight;
	Bool	m_isInteractive;
	Bool	m_isAffectedByWeather;

	Bool	m_currentState;

	Float	m_timeToToggle;

	CCityLightManager*	m_cityLightManager;

	//for fire source triggering explosions when necessary
	TagList	m_tagList;

public:
	CGameplayLightComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnPropertyPostChange( IProperty* property ) override;

	void SetLight			( Bool toggle, Bool system = true );
	void SetFadeLight		( Bool toggle, Bool system = true );
	void SetCityLight		( Bool toggle ) { m_isCityLight = toggle; }
	void SetInteractive		( Bool toggle );
	void SetWeatherReaction	( Bool toggle ) { m_isAffectedByWeather = toggle; }

	Bool IsLightOn() const				{ return m_isLightOn; }
	Bool IsCityLight() const			{ return m_isCityLight; }
	Bool IsInteractive() const			{ return m_isInteractive; }
	Bool IsAffectedByWeather() const	{ return m_isAffectedByWeather; }

	
	// -------------------------------------------------------------------------
	// Debugging
	// -------------------------------------------------------------------------

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );


private:
	void SetInitialLight		( Bool toggle );	
	void UpdateInteraction		();
	void ProcessToggle			( Bool toggle, Bool system, Bool allowLightsFade );

	void funcSetLight			( CScriptStackFrame& stack, void* result );
	void funcSetFadeLight		( CScriptStackFrame& stack, void* result );
	void funcSetCityLight		( CScriptStackFrame& stack, void* result );
	void funcSetInteractive 	( CScriptStackFrame& stack, void* result );
	void funcSetWeatherReaction	( CScriptStackFrame& stack, void* result );

	void funcIsLightOn				( CScriptStackFrame& stack, void* result );
	void funcIsCityLight			( CScriptStackFrame& stack, void* result );
	void funcIsInteractive		( CScriptStackFrame& stack, void* result );
	void funcIsAffectedByWeather	( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CGameplayLightComponent );
	PARENT_CLASS( CInteractionComponent );

	PROPERTY_EDIT( m_isLightOn,				TXT("Default state") );
	PROPERTY_EDIT( m_isCityLight,			TXT("City lights toggle based on TOD") );
	PROPERTY_EDIT( m_isInteractive,			TXT("Toggled thru interaction and signs") );
	PROPERTY_EDIT( m_isAffectedByWeather,	TXT("Rain turns off fire lights") );

	NATIVE_FUNCTION( "SetLight",			funcSetLight );
	NATIVE_FUNCTION( "SetFadeLight",		funcSetFadeLight );
	NATIVE_FUNCTION( "SetInteractive",		funcSetInteractive );

	NATIVE_FUNCTION( "IsLightOn",			funcIsLightOn );
	NATIVE_FUNCTION( "IsCityLight",			funcIsCityLight );
	NATIVE_FUNCTION( "IsInteractive",		funcIsInteractive );
	NATIVE_FUNCTION( "IsAffectedByWeather",	funcIsAffectedByWeather );
END_CLASS_RTTI();

// a light that saves it's state
class CPersistentLightComponent : public CGameplayLightComponent	 
{
	DECLARE_ENGINE_CLASS( CPersistentLightComponent, CGameplayLightComponent, 0 )

public:
	virtual void OnSaveGameplayState( IGameSaver* saver );
	virtual void OnLoadGameplayState( IGameLoader* loader ); 
	virtual Bool CheckShouldSave() const { return true; }	
};

BEGIN_CLASS_RTTI( CPersistentLightComponent )
	PARENT_CLASS( CGameplayLightComponent )
END_CLASS_RTTI()


