/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inputUserConfig.h"
#include "inputKeyToDeviceMapping.h"
#include "userProfile.h"
#include "rawInputManager.h"
#include "gestureSystem.h"
#include "../core/engineTime.h"
#include "../core/configVarLegacyWrapper.h"

///////////////////////////////////////////////////////////////////////////////

class CConfigSection;
class CConfigFile;
class CGestureSystem;

enum KeyState
{
	KS_None = 0,
	KS_Pressed,
	KS_Released,
	KS_Axis,
	KS_Duration,
	KS_DoubleTap
};


///////////////////////////////////////////////////////////////////////////////

struct SInputAction
{
	DECLARE_RTTI_STRUCT( SInputAction );
public:
	CName		m_aName;
	Float		m_value;
	Float		m_lastFrameValue;
	EngineTime  activationTime;
	Bool		m_contextChangeHandled;
	Bool		m_reprocessInput;

	SInputAction( CName _action = CName(), Float _value = 0.f ) : m_aName( _action ), m_value( _value ), m_lastFrameValue( 0.f ), m_contextChangeHandled( false ), m_reprocessInput( false )
	{};

	Bool operator==( const SInputAction& second ) const	{ return m_aName == second.m_aName; }
	Bool operator!=( const SInputAction& second ) const	{ return m_aName != second.m_aName; }
	Bool operator<( const SInputAction& second )  const	{ return m_aName <  second.m_aName; }


	static SInputAction INVALID;
};

BEGIN_CLASS_RTTI( SInputAction );
	PROPERTY( m_aName );
	PROPERTY( m_value );
	PROPERTY( m_lastFrameValue );
END_CLASS_RTTI();

enum EInputSensitivityPreset : CEnum::TValueType
{
	ISP_Normal,
	ISP_Aiming
};

BEGIN_ENUM_RTTI( EInputSensitivityPreset );
	ENUM_OPTION( ISP_Normal );
	ENUM_OPTION( ISP_Aiming );
END_ENUM_RTTI();

struct SInputActionActivator
{
	KeyState		m_activationState;

	union Data1   //const values
	{
		Float			m_durationIdleTime; // KS_Duration this value defines how long the key must be pressed in order to be activated
		Float			m_dblClickTime;		// KS_DoubleTap interval time for double tap
		Float			m_axisValue;		// KS_Axis internal value for axis
	} m_data; 
	
	union Data2	 //mutable values
	{
		Bool			m_hasBeenActivated; // KS_Axis		If activator contributed to final axis value
		Float			m_timeToActivation;	// KS_Duration	Time in which, if button is being held, the activation will occur (initially durationIdleTime) - this value changes within each tick
		Double			m_lastClickTime;	// KS_DoubleTap This is the time that the key was last time pressed. (Used to determine the double tap event)
	} m_data2;
	
	CName			m_actionName;
	SInputAction *	m_actionToActivate; // caching 

	SInputActionActivator()
	{}
	SInputActionActivator( CName _actionName  , KeyState _activationState = KS_Pressed, Float _durationIdleTime = 0.f )
		: m_actionName( _actionName ), m_activationState( _activationState ), m_actionToActivate( NULL )
	{
		m_data.m_durationIdleTime = _durationIdleTime ;
		m_data2.m_hasBeenActivated = false;
	}

	Bool operator==( SInputActionActivator & ref )
	{ return m_actionName == ref.m_actionName && m_activationState == ref.m_activationState && Abs<Float>(m_data.m_axisValue-ref.m_data.m_axisValue) < NumericLimits<Float>::Epsilon(); }
};

struct SInputContext
{
	TSortedArray<SInputAction>						m_inputEvents;
	TArrayMap<EInputKey, SInputActionActivator>		m_key2activator;

	TDynArray< CName >								m_derivedFrom;
};

struct SButtonAxisMapping
{
	TArrayMap< EInputKey, Float >	m_keyVal;
	Float							m_totalVal;

	SButtonAxisMapping() : m_totalVal( 0.0f ) {}
	void NotifyKeyVal( EInputKey iKey, Float val );
};

struct SReprocessInput
{
	Bool	m_pressed;
	CName	m_actionName;

	SReprocessInput() : m_pressed( false ) {}
	SReprocessInput( const CName& actionName ) : m_pressed( false ), m_actionName( actionName ) {}
};

//////////////////////////////////////////////////////////////////////////////
class IInputListener;
class CScriptInputListener;

class CInputManager : public CObject, public IRawInputListener
{
	friend class CReplayTestCase;

	DECLARE_ENGINE_CLASS( CInputManager, CObject, 0 );

public:

protected:

	TArrayMap<CName, SInputContext>::iterator			m_currentContext;
	TArrayMap<CName, SInputContext>						m_context2bindings;

	TArrayMap< CName, TDynArray< EInputKey > >			m_reverseActionKeyMapping;

	TDynArray<SInputActionActivator*>					m_durationActivators;

	TArrayMap<SInputAction* , Float>					m_delayedAxisActivations;
	TArrayMap< TPair< SInputAction*, Float > , Uint64>	m_delayedValueUpdate;
	TDynArray< CName >									m_ignoredGameInputs;

	TArrayMap< CName, IInputListener*>					m_actionListeners;
	TDynArray<CScriptInputListener*>					m_scriptActionListeners;

	TArrayMap< CName, SButtonAxisMapping >				m_buttonAxisMapping;

	Bool												m_suppressSendingEvents;

	EInputSensitivityPreset								m_sensitivityPreset;

	THashMap< String, EInputKey >						m_keyNameToCodes;
	THashMap< EInputKey, String, EInputKeyHashFunc >	m_keyCodeToNames;

	TArrayMap< EInputKey, SReprocessInput >				m_reprocessableInput;

	Bool												m_enableLog;


	TDynArray< CName >									m_storedContext;

	InputUtils::EInputDeviceType						m_lastRecognizedDevice;

private:
	CGestureSystem*										m_gestureSystem;

public:

	CInputManager();
	virtual ~CInputManager();

	//new way of handling input
	void RegisterListener( IInputListener * listener, CName actionName );
	void UnregisterListener( IInputListener * listener, CName actionName );
	void UnregisterListener( IInputListener* listener );

	//! Process input
	virtual Bool ProcessInput( const BufferedInput& input );
	Bool ProcessSpecialMouseInput( SInputContext& bindings, enum EInputKey key, enum KeyState state, Float data );
	void ProcessFakeAxisInput( EInputKey key, KeyState state, Float data );
	void ProcessFakeAxisOnly( const BufferedInput& input );
	void UpdateInput( Float timeDelta );

	RED_INLINE const THashMap< String, EInputKey >& GetKeyNameToCodeMap() const { return m_keyNameToCodes; }
	RED_INLINE const THashMap< EInputKey, String, EInputKeyHashFunc >& GetKeyCodeToNameMap() const { return m_keyCodeToNames; }
	RED_INLINE const TDynArray< CName >& GetStoredContext() const { return m_storedContext; }
	RED_INLINE const CName& GetCurrentContext() const { return ( m_currentContext != m_context2bindings.End() ) ? m_currentContext->m_first : CName::NONE; }

	//! Get all unique bound game input names
	const TDynArray<CName> GetGameEventNames() const;

	//! Deny processing of defined game inputs
	void IgnoreGameInput( const CName& name, Bool ignore );
	RED_INLINE Bool IsInputIgnored( const CName& name ) const { return m_ignoredGameInputs.Exist( name ); }
	RED_INLINE void ClearIgnoredInput() { m_ignoredGameInputs.ClearFast(); }

	//! Find first input key for given game input
	EInputKey FindFirstIKForGameInput( const CName& name, Float activation, Bool isUsingPad = false );

	//! hard reset on game load 
	void Reset();

	//! soft reset on lost focus
	void SoftReset();

	RED_INLINE void SuppressSendingEvents( const Bool suppress ) 
	{
		if( suppress && !m_suppressSendingEvents )
		{
			SoftReset();
		}
		m_suppressSendingEvents = suppress; 
	}

	void SetCurrentContext( CName context );
	
	void SetSensitivityPreset( EInputSensitivityPreset sensitivityPreset );

	//! Reload all user settings
	RED_INLINE void ReloadSettings() 
	{
		LoadDefaultMappings();
	}


	SInputAction & GetInputActionByName( CName name );
	const SInputAction & GetInputActionByName( CName name ) const;

	SInputAction & GetInputActionByName( CName name, SInputContext& context );

	SInputAction & GetInputActionByActivator( SInputActionActivator & activator );

	Float GetActionValue( CName actionName ) const;
	Float GetLastActivationTime( CName actionName );

public:

	void NotifyAxisKeyVal( CName axis, EInputKey iKey, Float val );
	Float CheckAxisKeyVal( CName axis ) const;

public:

	CGestureSystem* GetGestureSystem();

public:

	void StoreContext( const CName& newContext = CName::NONE );
	void RestoreContext( const CName& storedContext, bool contextCouldChange );

	void ForceDeactivateAction( const CName& actionName );

protected:

	void ActivateInputAction( SInputAction& action, Float actiValue = 1.f );
	void DeactivateInputAction( SInputAction& action );
	void NotifyActionChange( SInputAction& action );
	void DeactivateAllInputActions();


	//! Removes an action from pending array
	Bool RemovePendingEvent( const SInputAction& action );

	Bool LoadKeyDefinitions( Config::Legacy::CConfigLegacyFile* fileName );
	void AddBinding( SInputContext & context, EInputKey key, CName action, KeyState activationState = KS_Pressed, Float additionalData = 0.f, Bool reprocess = false );

	void LoadContextFromSection( SInputContext& context, Config::Legacy::CConfigLegacySection* section, Config::Legacy::CConfigLegacyFile* file, Uint32 maxDerivationSafetyCheck );

	//! Filter pad events, returns true if EInputKey matches pad action name
	RED_INLINE Bool FilterPadEvents( enum EInputKey key ) const { return ( key >= IK_Pad_First && key <= IK_Pad_Last ); }

	Float GetSensitivityModifier( EInputSensitivityPreset sensitivityPreset );

	/***** Input rebinding and saving *****/

public:
	void SaveUserMappings();

	void SetKeyboardMouseActionBinding( const CName& action, EInputKey mainKey, EInputKey alternativeKey );

	void SetKeybardMouseActionBindingRegular( const CName& action, EInputKey mainKey, EInputKey alternativeKey );
	void SetKeybardMouseActionBindingMovement( const CName& action, EInputKey mainKey, EInputKey alternativeKey );

	void SetPadActionBinding( const CName& action, EInputKey key );

	void ClearActionCacheForContext( SInputContext* context );

protected:
	void LoadDefaultMappings();

	Bool CheckMappingFileVersionMatch(Config::Legacy::CConfigLegacyFile* file);
	Int32 GetMappingVersionFromFile(Config::Legacy::CConfigLegacyFile* file);
	Int32 GetValidMappingVersion();

#ifdef RED_PLATFORM_WINPC
	Bool LoadUserMappings();
#endif

	Bool DoesUserMappingsExist(const String& filename);
	Bool GetOrCreateUserDirectory( String& outUserDirectoryPath );

	void FindAllContextsForAction(const CName& action, TDynArray<SInputContext*>& outContexts);
	Bool DoesContextHasAction(const SInputContext& context, const CName& action);

	Bool EraseActionFromContext(SInputContext* context, const CName& action, Bool isPad, SInputActionActivator& outErasedActivator);
	Bool EraseMovementActionFromContext(SInputContext* context, const CName& action, Bool isPositiveValue, Bool isPad, SInputActionActivator& outErasedActivator);

	void RebindReverseMappingForKeyboardMouse(const CName& action, EInputKey mainKey, EInputKey alternativeKey);
	void RebindReverseMappingForPad(const CName& action, EInputKey key);

	TDynArray<EInputKey>& FindOrCreateReverseMappingForAction( const CName& action );
	void RemoveAllKeysForDevice( TDynArray<EInputKey>& keys, Bool isPad );

	/**************************************/

public:

	void GetPCKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys );
	void GetPadKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys );
	void GetCurrentKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys );

public:

	Bool LastUsedPCInput() const;
	Bool LastUsedGamepad() const;
	const CName GetLastUsedDeviceName() const;

private:
	void ShareAxisActivators( SInputContext& oldContext, SInputContext& newContext );

	void ReprocessInput();

protected:
	void funcGetLastActivationTime( CScriptStackFrame& stack, void* result );
	void funcGetActionValue( CScriptStackFrame& stack, void* result );
	void funcIgnoreGameInput( CScriptStackFrame& stack, void* result );
	void funcClearIgnoredInput( CScriptStackFrame& stack, void* result );
	void funcIsInputIgnored( CScriptStackFrame& stack, void* result );
	void funcSetContext( CScriptStackFrame& stack, void* result );
	void funcGetContext( CScriptStackFrame& stack, void* result );
	void funcUnregisterListener( CScriptStackFrame& stack, void* result );
	void funcRegisterListener( CScriptStackFrame& stack, void* result );
	void funcGetAction( CScriptStackFrame& stack, void* result );
	void funcStoreContext( CScriptStackFrame& stack, void* result );
	void funcRestoreContext( CScriptStackFrame& stack, void* result );
	void funcEnableLog( CScriptStackFrame& stack, void* result );
	void funcLastUsedPCInput( CScriptStackFrame& stack, void* result );
	void funcLastUsedGamepad( CScriptStackFrame& stack, void* result );
	void funcForceDeactivateAction( CScriptStackFrame& stack, void* result );
	void funcGetPCKeysForAction( CScriptStackFrame& stack, void* result );
	void funcGetPadKeysForAction( CScriptStackFrame& stack, void* result );
	void funcGetCurrentKeysForAction( CScriptStackFrame& stack, void* result );
	void funcGetPCKeysForActionStr( CScriptStackFrame& stack, void* result );
	void funcGetPadKeysForActionStr( CScriptStackFrame& stack, void* result );
	void funcGetCurrentKeysForActionStr( CScriptStackFrame& stack, void* result );
	void funcUsesPlaystationPad( CScriptStackFrame& stack, void* result );
	void funcSetInvertCamera( CScriptStackFrame& stack, void* result );
	void funcGetLastUsedDeviceName( CScriptStackFrame& stack, void* result );
	String GetBindingFileNameBasedOnKeyboardLayout() const;
};

BEGIN_CLASS_RTTI( CInputManager );
	PARENT_CLASS( CObject )
	PROPERTY( m_gestureSystem );
	NATIVE_FUNCTION( "GetLastActivationTime", funcGetLastActivationTime );
	NATIVE_FUNCTION( "GetActionValue", funcGetActionValue );
	NATIVE_FUNCTION( "GetAction", funcGetAction );
	NATIVE_FUNCTION( "IgnoreGameInput", funcIgnoreGameInput );
	NATIVE_FUNCTION( "ClearIgnoredInput", funcClearIgnoredInput );
	NATIVE_FUNCTION( "IsInputIgnored", funcIsInputIgnored );
	NATIVE_FUNCTION( "SetContext", funcSetContext );
	NATIVE_FUNCTION( "GetContext", funcGetContext );
	NATIVE_FUNCTION( "UnregisterListener", funcUnregisterListener );
	NATIVE_FUNCTION( "RegisterListener", funcRegisterListener );
	NATIVE_FUNCTION( "EnableLog", funcEnableLog );
	NATIVE_FUNCTION( "StoreContext", funcStoreContext );
	NATIVE_FUNCTION( "RestoreContext", funcRestoreContext );
	NATIVE_FUNCTION( "LastUsedPCInput", funcLastUsedPCInput );
	NATIVE_FUNCTION( "LastUsedGamepad", funcLastUsedGamepad );
	NATIVE_FUNCTION( "ForceDeactivateAction", funcForceDeactivateAction );
	NATIVE_FUNCTION( "GetPCKeysForAction", funcGetPCKeysForAction );
	NATIVE_FUNCTION( "GetPadKeysForAction", funcGetPadKeysForAction );
	NATIVE_FUNCTION( "GetCurrentKeysForAction", funcGetCurrentKeysForAction );
	NATIVE_FUNCTION( "GetPCKeysForActionStr", funcGetPCKeysForActionStr );
	NATIVE_FUNCTION( "GetPadKeysForActionStr", funcGetPadKeysForActionStr );
	NATIVE_FUNCTION( "GetCurrentKeysForActionStr", funcGetCurrentKeysForActionStr );
	NATIVE_FUNCTION( "UsesPlaystationPad", funcUsesPlaystationPad );
	NATIVE_FUNCTION( "SetInvertCamera", funcSetInvertCamera );
	NATIVE_FUNCTION( "GetLastUsedDeviceName", funcGetLastUsedDeviceName );
END_CLASS_RTTI();
