/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "extAnimEvent.h"
#include "extAnimDurationEvent.h"
#include "cameraComponent.h"
#include "lightComponent.h"

class CExtAnimCutsceneEnvironmentEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneEnvironmentEvent )

public:
	CExtAnimCutsceneEnvironmentEvent();

	CExtAnimCutsceneEnvironmentEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName,
		Bool stabilizeBlending, Bool instantEyeAdaptation,	Bool instantDissolve,
		Bool forceSetupLocalEnvironments, Bool forceSetupGlobalEnvironments,
		const String& environmentName, Bool environmentActivate );
	virtual ~CExtAnimCutsceneEnvironmentEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	void OnCutsceneFinish() const;

protected:
	Bool	m_stabilizeBlending;
	Bool	m_instantEyeAdaptation;
	Bool	m_instantDissolve;
	Bool	m_forceSetupLocalEnvironments;
	Bool	m_forceSetupGlobalEnvironments;
	String	m_environmentName;
	Bool	m_environmentActivate;
	Bool	m_forceNoOtherEnvironments;

	// Used only by cutcsenes...
	// Yes, I feel ashamed of this hack, :( but env changes are also non-synced
	// This whole event HAS TO be refactored
	mutable Bool	m_wasActiveBefore;

private:
	CAreaEnvironmentComponent* FindAreaEnvironment() const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneEnvironmentEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_stabilizeBlending,				TXT( "Stabilize blending" ) );
	PROPERTY_EDIT( m_instantEyeAdaptation,			TXT( "Instant eye adaptation" ) );
	PROPERTY_EDIT( m_instantDissolve,				TXT( "Instant dissolve" ) );
	PROPERTY_EDIT( m_forceSetupLocalEnvironments,	TXT( "Force local environments setup for camera" ) );
	PROPERTY_EDIT( m_forceSetupGlobalEnvironments,	TXT( "Force global environments setup for camera" ) );
	PROPERTY_EDIT( m_environmentName,				TXT( "Environment name to activate/deactivate" ) );
	PROPERTY_EDIT( m_environmentActivate,			TXT( "Activate environment with specified name" ) );
	PROPERTY_EDIT( m_forceNoOtherEnvironments,		TXT( "Force no other environments" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneLightEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneLightEvent )

private:
	TagList						m_tag;

	Color						m_color;					//!< Light color
	Float						m_radius;					//!< Light radius
	Float						m_brightness;				//!< Light brightness
	SLightFlickering			m_lightFlickering;			//!< Light flickering
	Bool						m_isEnabled;				//!< Is the light enabled

public:
	CExtAnimCutsceneLightEvent();
	CExtAnimCutsceneLightEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneLightEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_CUSTOM_EDIT( m_tag, TXT("Node tags"), TXT("TagListEditor") );
	PROPERTY_EDIT( m_isEnabled, TXT("Toggles light on and off") );
	PROPERTY_EDIT_RANGE( m_radius, TXT("Light radius"), 0.001f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_brightness, TXT("Brightness level"), 0.0f, 1000.0f );
	PROPERTY_EDIT( m_color, TXT("Light color") );
	PROPERTY_EDIT( m_lightFlickering, TXT("Light flickering") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneBokehDofEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneBokehDofEvent )

private:
	SBokehDofParams	m_bokehDofParams;

public:
	CExtAnimCutsceneBokehDofEvent();
	CExtAnimCutsceneBokehDofEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );

	virtual void ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneBokehDofEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_INLINED( m_bokehDofParams, TXT( "Depth of field parameters" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneBokehDofBlendEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneBokehDofBlendEvent )

private:
	SBokehDofParams	m_bokehDofParamsStart;
	SBokehDofParams	m_bokehDofParamsEnd;

public:
	CExtAnimCutsceneBokehDofBlendEvent();
	CExtAnimCutsceneBokehDofBlendEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );

	virtual void ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneBokehDofBlendEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_INLINED( m_bokehDofParamsStart, TXT( "Depth of field parameters - start" ) );
	PROPERTY_INLINED( m_bokehDofParamsEnd, TXT( "Depth of field parameters - end" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneSetClippingPlanesEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneSetClippingPlanesEvent )

private:
	ENearPlaneDistance		m_nearPlaneDistance;
	EFarPlaneDistance		m_farPlaneDistance;

	SCustomClippingPlanes	m_customPlaneDistance;

public:
	CExtAnimCutsceneSetClippingPlanesEvent();
	CExtAnimCutsceneSetClippingPlanesEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );

	virtual void ProcessEx( const CAnimationEventFired& info, CCameraComponent* component, Float currTime ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneSetClippingPlanesEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_nearPlaneDistance, TXT("Near plane distance") );
	PROPERTY_EDIT( m_farPlaneDistance, TXT("Far plane distance") );
	PROPERTY_INLINED( m_customPlaneDistance, TXT("Custom plane distance") );
END_CLASS_RTTI();
