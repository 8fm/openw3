/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "softTriggerAreaComponent.h"
#include "soundEmitter.h"
#include "../core/gatheredResource.h"
#include "soundWallaSystem.h"

RED_DECLARE_NAME( SoundAmbientArea )

enum ESoundAmbientDynamicParameter
{
	ESADP_None
};

BEGIN_ENUM_RTTI( ESoundAmbientDynamicParameter );
ENUM_OPTION( ESADP_None );
END_ENUM_RTTI();

struct SReverbDefinition
{
	DECLARE_RTTI_STRUCT( SReverbDefinition )

	StringAnsi	m_reverbName;
	Bool		m_enabled;
};

BEGIN_CLASS_RTTI( SReverbDefinition )
	PROPERTY_CUSTOM_EDIT( m_reverbName, TXT( "Name of reverb definition" ), TXT( "SoundReverbEditor" ) )
	PROPERTY_EDIT( m_enabled, TXT( "Is reverb effect enabled" ) )
END_CLASS_RTTI()

struct SSoundGameParameterValue
{
	DECLARE_RTTI_STRUCT( SSoundGameParameterValue )

	StringAnsi	m_gameParameterName;
	Float		m_gameParameterValue;

	RED_INLINE bool operator==( const SSoundGameParameterValue& other ) const
	{
		return m_gameParameterName == other.m_gameParameterName;
	}
};

BEGIN_CLASS_RTTI( SSoundGameParameterValue )
	PROPERTY_CUSTOM_EDIT( m_gameParameterName, TXT( "Name" ), TXT( "SoundGameParamterEditor" ) )
	PROPERTY_EDIT( m_gameParameterValue, TXT( "Value" ) )
END_CLASS_RTTI()

struct SSoundParameterCullSettings
{
	DECLARE_RTTI_STRUCT( SSoundParameterCullSettings )

	StringAnsi	m_gameParameterName;
	Float		m_gameParameterCullValue;
	Bool		m_invertCullCheck;

	RED_INLINE bool operator==( const SSoundParameterCullSettings& other ) const
	{
		return m_gameParameterName == other.m_gameParameterName;
	}
};

BEGIN_CLASS_RTTI( SSoundParameterCullSettings )
	PROPERTY_CUSTOM_EDIT( m_gameParameterName, TXT( "Name of the parameter to use for culling" ), TXT( "SSoundGameParameterEditor" ) )
	PROPERTY_EDIT( m_gameParameterCullValue, TXT( "Value to cull at" ) )
	PROPERTY_EDIT( m_invertCullCheck, TXT( "Invert the check for culling" ) )
END_CLASS_RTTI()

struct SSoundAmbientDynamicSoundEvents
{

	SSoundAmbientDynamicSoundEvents()
		:m_triggerOnActivation(true)
		,m_lastTimeWasPlaying(0.f)
		,m_repeatTime(0.f)
		,m_repeatTimeVariance(0.f)
		,m_nextVariance(0.f)
	{};

	DECLARE_RTTI_STRUCT( SSoundAmbientDynamicSoundEvents )


	RED_INLINE bool operator==( const SSoundAmbientDynamicSoundEvents& other ) const
	{
		return m_eventName == other.m_eventName;
	}

	//User settings
	StringAnsi m_eventName;
	Float m_repeatTime;
	Float m_repeatTimeVariance;
	Bool m_triggerOnActivation;

	//Runtime state
	Float m_lastTimeWasPlaying;
	Float m_nextVariance;
};

BEGIN_CLASS_RTTI( SSoundAmbientDynamicSoundEvents )
	PROPERTY_EDIT( m_eventName, TXT( "Name of the sound event to trigger" ) )
	PROPERTY_EDIT( m_repeatTime, TXT( "Base time until sound will be repeasted" ) )
	PROPERTY_EDIT( m_repeatTimeVariance, TXT( "Random variance in repeat time" ) )
	PROPERTY_EDIT( m_triggerOnActivation, TXT( "Play when the zone activates" ) )
END_CLASS_RTTI()


BEGIN_ENUM_RTTI( ESoundParameterCurveType );
ENUM_OPTION( ESPCT_Log3 );
ENUM_OPTION( ESPCT_Sine );
ENUM_OPTION( ESPCT_Log1 );
ENUM_OPTION( ESPCT_InversedSCurve );
ENUM_OPTION( ESPCT_Linear );
ENUM_OPTION( ESPCT_SCurve );
ENUM_OPTION( ESPCT_Exp1 );
ENUM_OPTION( ESPCT_ReciprocalOfSineCurve );
ENUM_OPTION( ESPCT_Exp3 );
END_ENUM_RTTI();

class CWallaSoundEmitter : public CSoundEmitter
{
	friend CSoundAmbientAreaComponent;

public: 
	virtual const Matrix GetSoundPlacementMatrix() const;

	CWallaSoundEmitter& Reverb( class ITriggerManager* triggerManager);
	const Vector & GetSoundPosition() const { return m_soundPosition; }

	void SetSoundPosition(const Vector &pos) { m_soundPosition = pos; }

	virtual StringAnsi GetSoundObjectName() const { return StringAnsi("WallaSoundEmitter"); }

private:

	Vector m_soundPosition;	
};



class CSoundAmbientAreaComponent : public CSoftTriggerAreaComponent, public CSoundEmitter
{
	DECLARE_ENGINE_CLASS( CSoundAmbientAreaComponent, CSoftTriggerAreaComponent, 0 );

	//enum to track of player transition in and out of the area
	enum ESoundAmbientAreaState
	{
		Entering, //Player has entered the are but we've not triggered events
		Inside, //Player had entered the area and we've triggered events
		Exiting, //Player has left the area but we've not triggered events
		Outside //Player has left the area and we've triggered events
	};

	enum EGatewayState
	{
		Gateway_None,	//Not in either section of the gate
		Gateway_Enter,	//In the gate entry section
		Gateway_Exit	//In the exit section of the gate
	};

public:
	RED_FORCE_INLINE const Float GetOuterReverbRatio() const { return m_outerListnerReverbRatio; }
	RED_FORCE_INLINE const Bool IsPriorityParameterMusic() const { return m_priorityParameterMusic; }
	RED_FORCE_INLINE const Float GetParameterEnteringTime() const { return m_parameterEnteringTime; }
	RED_FORCE_INLINE const ESoundParameterCurveType GetParameterEnteringCurve() const { return m_parameterEnteringCurve; }
	RED_FORCE_INLINE const Float GetParameterExitingTime() const { return m_parameterExitingTime; }
	RED_FORCE_INLINE const ESoundParameterCurveType GetParameterExitingCurve() const { return m_parameterExitingCurve; }

	//Returns the rotation of the gateway in radians
	Float GetGatewayRotationInRad() const;

	//Return unit vector of the gateway direction
	Vector GetGatewayDirection() const;

	//Returns unit vector of the up direction
	Vector GetUpDirection() const;
public:
	CSoundAmbientAreaComponent();
	virtual ~CSoundAmbientAreaComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	//Returns true if we should cull the sound based on current parameter values
	Bool ShouldCullFromParameters() const;

	virtual Bool SoundIsActive() const;
	
	virtual Bool SoundIsActive( const char* eventName ) const;

	void ProcessDynamicEvents();

	void TickSounds();

	virtual void OnTickPostUpdateTransform( Float timeDelta );

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	virtual const StringAnsi& GetReverbName();

	//! Calculate beveling radius (used when inserting the trigger shape into the manager)
	virtual Float CalcBevelRadius() const;

	//! Calculate vertical beveling radius (used when inserting the trigger shape into the manager)
	virtual Float CalcVerticalBevelRadius() const;

	Int32 GetAmbientPriority() const;
	const TDynArray< SSoundGameParameterValue >& GetGameParameters() const { return m_parameters; }

#ifdef SOUND_DEBUG

	// Updates debug decoy position
	void UpdateDebug( const Vector& position, Float distance );

	//! Generate fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

#endif

	void OnEnterEvents();

	void OnExitEvents();

	//Returns the current state of the listener in this gateway
	EGatewayState CalcListenerGatewayState();

protected:

#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const { return UNICODE_TO_ANSI( GetFriendlyName().AsChar() ); }
#endif

	StringAnsi								m_soundEvents;

	SReverbDefinition						m_reverb;
	Float									m_outerListnerReverbRatio;

	StringAnsi								m_customEventOnEnter;

	TDynArray< StringAnsi >					m_soundEventsOnEnter;
	TDynArray< StringAnsi >					m_soundEventsOnExit;

	TDynArray< StringAnsi >					m_wallaSoundEvents;
	CWallaSoundEmitter						m_wallaSoundEmitters[Num_WallaDirections];

	///is to removal
	Float									m_intensityParameter;
	Float									m_intensityParameterFadeTime;
	///is to removal

	Bool									m_priorityParameterMusic;
	Float									m_parameterEnteringTime;
	ESoundParameterCurveType				m_parameterEnteringCurve;
	Float									m_parameterExitingTime;
	ESoundParameterCurveType				m_parameterExitingCurve;
	Bool									m_useListernerDistance;
	Bool									m_enterExitEventsUsePosition;

	TDynArray< SSoundGameParameterValue >	m_parameters;
	TDynArray< SSoundParameterCullSettings >  m_parameterCulling;
	TDynArray< ESoundAmbientDynamicParameter > m_dynamicParameters;
	TDynArray< SSoundAmbientDynamicSoundEvents > m_dynamicEvents;

	Bool									m_fitWaterShore;
	Uint32									m_waterGridCellCount;
	Float									m_waterLevelOffset;
	Bool									m_fitFoliage;
	Float									m_foliageMaxDistance;
	Uint32									m_foliageStepNeighbors;
	Float									m_foliageVitalAreaRadius;
	Uint32									m_foliageVitalAreaPoints;

private:

#ifdef SOUND_DEBUG
	Vector									m_decoyPosition;
	Float									m_playerDistance;
#endif

	Float									m_wallaEmitterSpread; //How far the walla emmiters are positioned from the main zone sound position

	Vector									m_lastSourcePosition;
	Vector									m_lastListenerPosition;

	ESoundAmbientAreaState					m_state;

	Bool									m_isGate; //Enables gate behavior for the zone 
	Bool									m_isWalla; //Turns on walla for the zone
	EGatewayState							m_gatewayState; 
	Float									m_gatewayRotation; //Changes the direction of the gate
	Float									m_wallaOmniFactor; //Float between 0.f and 1.f that specifies how much the total number of actors contributes to directional walla zones

	Bool									m_isPlaying			: 1;
	Bool									m_isRegistered		: 1;
	Bool									m_innerActivated	: 1;

	Float						m_maxDistanceVertical;
	SoundWallaMetrics			m_wallaMetrics;
	Float						m_wallaMinDistance; //Actors below min distance contribute an additional point to all directions
	Float						m_wallaMaxDistance; //Actors beyond max distance don't contribute to walla
	Float						m_wallaBoxExtention; //Extends the walla detection box beyond the ambient zone
	Float						m_wallaRotation; //Rotate the directions walla uses for detection/emitters
	Float						m_wallaAfraidRetriggerTime; //Minimum time between triggering afraid sweeteners
	Float						m_wallaAfraidTimer; //Time in seconds until an afraid sweetener is allowed
	Float						m_wallaAfraidDecreaseRate; //Rate in actors per second that the afraid count can decrease


protected:
	// CTriggerArea & CSoftTriggerArea interface implementation
	virtual void EnteredArea( CComponent* component );
	virtual void ExitedArea( CComponent* component );
	virtual void EnteredOuterArea( CComponent* component, const class ITriggerActivator* activator );
	virtual void ExitedOuterArea( CComponent* component, const class ITriggerActivator* activator );

	// Register in the audio system as a valid source of ambient source
	void RegisterAudioSource();

	// Unregister from the audio system
	void UnregisterAudioSource();

protected:
	virtual Bool Play( Bool _loadReasourcesImmediately = false );
	virtual void Stop();
	virtual Bool IsPlaying() const;
	virtual Bool HasFailed() const { return false; }
	virtual Float GetMaxDistance() const { return m_maxDistance; };
	virtual const Vector& GetSoundPosition() const;
	virtual void UpdateSoundPosition( const Vector& listenerPosition );

	// CSoundEmitter interface
	virtual const Matrix GetSoundPlacementMatrix() const;

	Vector GetOcclusionPosition() { return m_lastSourcePosition; }
	void ProcessExitEvents();
	void ProcessEnterEvents();
	void ProcessWalla(Float timeDelta);
	bool WallaIsActive();

#ifndef NO_EDITOR
protected:
	void GenerateWaterAreas( );
	void GenerateFoliageAreas( );
	void RestoreAreaToDefaultBox( );
#endif
	void ProcessDynamicParameter(ESoundAmbientDynamicParameter dynamicParamter);
};

BEGIN_CLASS_RTTI( CSoundAmbientAreaComponent )
	PARENT_CLASS( CSoftTriggerAreaComponent )
	PROPERTY_CUSTOM_EDIT( m_soundEvents, TXT( "Sound events" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_INLINED( m_reverb, TXT( "Reverb effect" ) )
	PROPERTY_EDIT( m_customEventOnEnter, TXT( "" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnEnter, TXT( "Sound events which will be launched when entering trigger" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnExit, TXT( "Sound events which will be launched when exiting trigger" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_EDIT( m_enterExitEventsUsePosition, TXT( "Enter/Exit events will be triggered with position (like ambient sound)" ))
	PROPERTY_EDIT( m_intensityParameter, TXT( "Intensity parameter will be set with selected (non negative value) when entering trigger" ) )
	PROPERTY_EDIT( m_intensityParameterFadeTime, TXT( "Intensity parameter will be faded in selected time" ) )
	PROPERTY_EDIT_RANGE( m_maxDistance, TXT( "Maximum distance from which ambient will be played" ), 0.0f, 1000.0f )
	PROPERTY_EDIT_RANGE( m_maxDistanceVertical, TXT( "Maximum distance from which ambient will be played in vertical direction" ), 0.0f, 1000.0f )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_banksDependency, TXT( "" ), TXT( "SoundBankEditor" ) )
	PROPERTY_EDIT( m_occlusionEnabled, TXT( "Occlusion" ) )
	PROPERTY_EDIT_RANGE( m_outerListnerReverbRatio, TXT( "OuterListnerReverbRatio" ), 0.0f, 1.0f )
	PROPERTY_EDIT( m_priorityParameterMusic, TXT( "IsMusicPriorityParameter" ) )
	PROPERTY_EDIT( m_parameterEnteringTime, TXT( "" ) )
	PROPERTY_EDIT( m_parameterEnteringCurve, TXT( "" ) )
	PROPERTY_EDIT( m_parameterExitingTime, TXT( "" ) )
	PROPERTY_EDIT( m_parameterExitingCurve, TXT( "" ) )
	PROPERTY_EDIT( m_useListernerDistance, TXT("Enables listener distance as a parameter on main area sound"))
	PROPERTY_EDIT( m_isGate, TXT( "Turn on gateway behaviour for this zone") )
	PROPERTY_EDIT( m_gatewayRotation, TXT( "Angle of the gateway in degrees") )
	PROPERTY_EDIT( m_isWalla, TXT("Turn on to have this zone generate walla metrics") )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_wallaSoundEvents, L"Events for FR, BR, BL, FL walla sounds (in that order)", TXT( "AudioEventBrowser") )
	PROPERTY_EDIT( m_wallaEmitterSpread, TXT(""))
	PROPERTY_EDIT( m_wallaOmniFactor, TXT("Float between 0.f and 1.f that specifies how much the total number of actors contributes to directional walla zones"))
	PROPERTY_EDIT( m_wallaMinDistance, TXT("Actors closer than this distance to the listener will not be included in walla"))
	PROPERTY_EDIT( m_wallaMaxDistance, TXT("Actors further than this distance from the listener will not be included in walla"))
	PROPERTY_EDIT( m_wallaBoxExtention, TXT("Extend the size of the walla zone past the ambient zone bounding box"))
	PROPERTY_EDIT( m_wallaRotation, TXT("Rotate the walla emmitters by the angle specified in degrees."))
	PROPERTY_EDIT( m_wallaAfraidRetriggerTime, TXT("Minimum time between triggering afraid sweeteners"))
	PROPERTY_EDIT( m_wallaAfraidDecreaseRate, TXT("Rate in actors per second that the afraid count can decrease"))
	PROPERTY_EDIT_ARRAY( m_parameters, TXT( "" ) )
	PROPERTY_EDIT_ARRAY( m_parameterCulling, TXT( "Set parameter culling values" ) )
	
	PROPERTY_EDIT_NOT_COOKED( m_fitWaterShore, TXT( "Fits the boundaries to water borders inside current area." ) )
	PROPERTY_EDIT_NOT_COOKED( m_waterGridCellCount, TXT( "Defines de number of cells of the water grid (the higher the smoother)." ) )
	PROPERTY_EDIT_NOT_COOKED( m_waterLevelOffset, TXT( "Specifies a water height offset used for auto-fitting." ) )
	PROPERTY_EDIT_NOT_COOKED( m_fitFoliage, TXT( "Fits the boundaries to foliage groups." ) )
	PROPERTY_EDIT_NOT_COOKED( m_foliageMaxDistance, TXT( "Defines the maximum distance between nearby trees within a group." ) )
	PROPERTY_EDIT_NOT_COOKED( m_foliageStepNeighbors, TXT( "Defines the maximum nearby points consider at each step while foliage fitting." ) )
	PROPERTY_EDIT_NOT_COOKED( m_foliageVitalAreaRadius, TXT( "Defines the radius of the vital area of each tree." ) )
	PROPERTY_EDIT_NOT_COOKED( m_foliageVitalAreaPoints, TXT( "Defines the number of points of the vital area of each tree." ) )
	PROPERTY_EDIT_ARRAY( m_dynamicParameters, TXT( "Dynamic parameters to set on this gameobject" ) )
	PROPERTY_EDIT_ARRAY( m_dynamicEvents, TXT( "Events that can use a randomised repeat time") )
END_CLASS_RTTI()
