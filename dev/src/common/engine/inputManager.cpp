/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inputManager.h"
#include "inputListener.h"
#include "testFramework.h"
#include "../core/scriptStackFrame.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "game.h"
#include "baseEngine.h"
#include "inputMappingSaver.h"
#include "inputDeviceManager.h"
#include "../core/keyboardRecognizer.h"
#include "../core/xmlReader.h"

//#pragma optimize("",off)

IMPLEMENT_ENGINE_CLASS( SInputAction );
IMPLEMENT_ENGINE_CLASS( CInputManager );

IMPLEMENT_RTTI_ENUM( EInputSensitivityPreset );

RED_DEFINE_STATIC_NAME( INVALID );
SInputAction SInputAction::INVALID = SInputAction( CNAME(INVALID) );

extern IInputDeviceManager* GInputDeviceManager;

namespace Config
{
	extern TConfigVar< Bool > cvForcePad;
	TConfigVar< String > cvUserMappingsConfigFilename( "InputMappings", "UserFilename", TXT("input"), eConsoleVarFlag_ReadOnly );
	TConfigVar< String > cvDefaultMappingsConfigFilename( "InputMappings", "DefaultFilename", TXT("input_qwerty"), eConsoleVarFlag_ReadOnly );
	TConfigVar< String > cvDefaultLayoutDescriptionFilename( "InputMappings", "LayoutDescriptions", TXT("config\\keyboard_layouts_description.xml"), eConsoleVarFlag_ReadOnly );
}


void SButtonAxisMapping::NotifyKeyVal( EInputKey iKey, Float val )
{
	auto it = m_keyVal.Find( iKey );
	if( it == m_keyVal.End() )
	{
		m_keyVal.Insert( iKey, val );
	}
	else
	{
		it->m_second = val;
	}

	m_totalVal = 0.0f;
	for( auto it2 = m_keyVal.Begin(); it2 != m_keyVal.End(); ++it2 )
	{
		m_totalVal += it2->m_second;
	}
}



namespace Consts
{
	const String userMappingsFileExtension( TXT("settings") );
}

//*********************************************************************************************************************************************
//*********************************************************************************************************************************************
CInputManager::CInputManager()
	: m_enableLog				( false )
	, m_suppressSendingEvents	( false )
	, m_sensitivityPreset		( EInputSensitivityPreset::ISP_Normal )
	, m_storedContext			( CName::NONE )
	, m_lastRecognizedDevice	( InputUtils::IDT_UNKNOWN )
	, m_gestureSystem			( NULL )
{
	m_currentContext = m_context2bindings.End();

	// Create key mapping
	CInputKeys::FillInputKeyMap( m_keyNameToCodes );
	CInputKeys::FillInputKeyMap( m_keyCodeToNames );

	// Load key definitions
	
	String filename = Config::cvUserMappingsConfigFilename.Get();

#ifdef RED_PLATFORM_WINPC
	Bool userMappingsLoaded = false;
	if( DoesUserMappingsExist( filename ) == true )
	{
		userMappingsLoaded = LoadUserMappings();
	}
	
	if( userMappingsLoaded == true )
	{
		LOG_ENGINE( TXT("User input mappings loaded.") );
	}
	else
	{
		LoadDefaultMappings();
		LOG_ENGINE( TXT("Default input mappings loaded.") );
	}
#else
	LoadDefaultMappings();
#endif

	Reset();
}

CInputManager::~CInputManager()
{
	for( Uint32 i = 0 ; i < m_scriptActionListeners.Size(); ++i )
	{
		delete m_scriptActionListeners[i];
	}
}

void CInputManager::Reset()
{	
	{ // clear action listeners
		m_actionListeners.Clear();

		for( Uint32 i = 0 ; i < m_scriptActionListeners.Size(); ++i )
		{
			delete m_scriptActionListeners[i];
		}

		m_scriptActionListeners.Clear();
	}

	if( m_gestureSystem )
	{
		m_gestureSystem->UnregisterAllListeners();
	}

	m_ignoredGameInputs.ClearFast();
	m_suppressSendingEvents = false;
	m_currentContext = m_context2bindings.End();

	// Clear all m_actionToActivate caches
	TArrayMap<CName, SInputContext>::iterator contextIterator = m_context2bindings.Begin();
	TArrayMap<CName, SInputContext>::iterator contextIteratorEnd = m_context2bindings.End();

	for( ; contextIterator != contextIteratorEnd; ++contextIterator )
	{
		TArrayMap<EInputKey, SInputActionActivator>& keysArray = contextIterator->m_second.m_key2activator;
		TArrayMap<EInputKey, SInputActionActivator>::iterator keysIterator = keysArray.Begin();
		TArrayMap<EInputKey, SInputActionActivator>::iterator keysIteratorEnd = keysArray.End();
		for( ; keysIterator != keysIteratorEnd; ++keysIterator )
		{
			keysIterator->m_second.m_actionToActivate = nullptr;
		}
	}

	SetCurrentContext( CNAME( Exploration ) );
}


void CInputManager::SoftReset()
{
#ifndef RED_FINAL_BUILD
	if( GGame->GetGameplayConfig().m_disableResetInput )
	{
		return;
	}
#endif

	m_durationActivators.Clear();
	m_delayedAxisActivations.Clear();
	DeactivateAllInputActions(); 
}



void CInputManager::SetCurrentContext( CName context )
{
	if( m_currentContext != m_context2bindings.End() && m_currentContext->m_first == context )
	{
		return;
	}

	TArrayMap<CName, SInputContext>::iterator	newContext = m_context2bindings.Find(context);
	if( newContext == m_context2bindings.End() )
	{
		return;
	}

	TDynArray< SInputAction* > actionsToDeactivate;
	actionsToDeactivate.Reserve( 64 );

	if( m_currentContext != m_context2bindings.End() )
	{
		ShareAxisActivators( m_currentContext->m_second, newContext->m_second );

		EInputKey currentKey = IK_None;
		TArrayMap<EInputKey, SInputActionActivator>	&			newKeysArr	=	newContext->m_second.m_key2activator;		
		TArrayMap<EInputKey, SInputActionActivator>::iterator	newKeysIter =	newKeysArr.Begin();
		TArrayMap<EInputKey, SInputActionActivator>	&			oldKeysArr	=	m_currentContext->m_second.m_key2activator;		
		TArrayMap<EInputKey, SInputActionActivator>::iterator	oldKeysIter	=	oldKeysArr.Begin();

		for( ; oldKeysIter != oldKeysArr.End(); ++oldKeysIter )
		{
			auto durationActivator = Find( m_durationActivators.Begin(), m_durationActivators.End(), &oldKeysIter->m_second );
			if( durationActivator == m_durationActivators.End() && ( oldKeysIter->m_second.m_actionToActivate == NULL || oldKeysIter->m_second.m_actionToActivate->m_value == 0.f ) )
			{
				continue;
			}

			if( oldKeysIter->m_first != currentKey )
			{
				currentKey	= oldKeysIter->m_first;
				newKeysIter = newKeysArr.Find( currentKey );
			}

			Bool found = false;
			TArrayMap<EInputKey, SInputActionActivator>::iterator newKeysIterCopy = newKeysIter;
			while(  newKeysIterCopy != newKeysArr.End() && newKeysIterCopy->m_first == currentKey  )
			{
				if( oldKeysIter->m_second == newKeysIterCopy->m_second )
				{
					TSortedArray<SInputAction>::iterator actionIter = newContext->m_second.m_inputEvents.Find( oldKeysIter->m_second.m_actionName );
					if( actionIter !=  newContext->m_second.m_inputEvents.End() )
					{
						found = true;
						SInputAction& action = GetInputActionByActivator( oldKeysIter->m_second );
						if( action != SInputAction::INVALID )
						{
							if( !action.m_contextChangeHandled )
							{
								action.m_contextChangeHandled = true;

								(*actionIter).m_value = action.m_value;
								(*actionIter).m_lastFrameValue = action.m_lastFrameValue;
								(*actionIter).activationTime = action.activationTime;

								action.m_value = 0.f;
								action.m_lastFrameValue = 0.f;
								action.activationTime = EngineTime();
							}
						}	

						if( durationActivator != m_durationActivators.End() )
						{
							newKeysIterCopy->m_second.m_data2.m_timeToActivation = (*durationActivator)->m_data2.m_timeToActivation;
							m_durationActivators.EraseFast( durationActivator );
							m_durationActivators.PushBack( &newKeysIterCopy->m_second );
						}
					}
				} 
				++newKeysIterCopy;
			}

			if( found == false && oldKeysIter->m_second.m_actionToActivate )
			{
				oldKeysIter->m_second.m_data2.m_hasBeenActivated = false;
				actionsToDeactivate.PushBack( oldKeysIter->m_second.m_actionToActivate );
			}
		}
	}

	if( m_currentContext != m_context2bindings.End() )
	{
		for( auto& action : m_currentContext->m_second.m_inputEvents )
		{
			action.m_contextChangeHandled = false;
		}
	}

	for( auto& action : newContext->m_second.m_inputEvents )
	{
		action.m_contextChangeHandled = false;
	}


	m_currentContext = newContext;

	for ( auto i = actionsToDeactivate.Begin(); i != actionsToDeactivate.End() ; ++i )
	{
		DeactivateInputAction( **i );
	}

	for( Int32 i = m_delayedAxisActivations.SizeInt()-1; i >= 0; --i )
	{
		if( GetInputActionByName( m_delayedAxisActivations[i].m_first->m_aName ) == SInputAction::INVALID)
		{
			m_delayedAxisActivations.RemoveAtFast(i);
		}
	}

	for( Int32 i = m_durationActivators.SizeInt()-1; i >= 0; --i )
	{
		if( GetInputActionByName( m_durationActivators[i]->m_actionName ) == SInputAction::INVALID)
		{
			m_durationActivators.RemoveAtFast(i);
		}
	}

	ReprocessInput();
}

void CInputManager::SetSensitivityPreset(EInputSensitivityPreset sensitivityPreset)
{
	m_sensitivityPreset = sensitivityPreset;
}

Bool CInputManager::ProcessInput( const BufferedInput& input )
{
	// mouse and keyboard input are treated as "PC" simply in this context, so they are equal
	const InputUtils::EInputDeviceType previousDevice = ( m_lastRecognizedDevice == InputUtils::IDT_MOUSE ) ? InputUtils::IDT_KEYBOARD : m_lastRecognizedDevice;

	if( m_currentContext == m_context2bindings.End() )
	{
		SetCurrentContext( CNAME( Exploration ) );
	}

	if( m_currentContext == m_context2bindings.End() || m_suppressSendingEvents )
	{
		ProcessFakeAxisOnly( input );
		return false;	
	}

	BufferedInput::const_iterator end = input.End();
	for( BufferedInput::const_iterator it = input.Begin(); it != end; ++it )
	{
		EInputKey key	 = it->key;
		KeyState state	 = (KeyState)it->action;
		Float data		 = it->data;

		{ // for process fake input
			auto it = m_reprocessableInput.Find( key );
			if( it != m_reprocessableInput.End() )
			{
				it->m_second.m_pressed = ( state == KeyState::KS_Pressed );
			}
		}

		InputUtils::EInputDeviceType deviceType = InputUtils::GetDeviceType( key );
		if( deviceType != InputUtils::IDT_UNKNOWN )
		{
			m_lastRecognizedDevice = deviceType;
		}

		{ // invert on pad
			if( ( key == IK_Pad_RightAxisY ) && ( SInputUserConfig::GetIsInvertCameraY() ) )
			{
				data = -data;
			}

			if( ( key == IK_Pad_RightAxisX ) && ( SInputUserConfig::GetIsInvertCameraX() ) )
			{
				data = - data;
			}
		}

		if( ProcessSpecialMouseInput( m_currentContext->m_second, key, state, data ) )
		{
			continue;
		}

		if( deviceType == InputUtils::IDT_KEYBOARD || deviceType == InputUtils::IDT_MOUSE )
		{
			ProcessFakeAxisInput( key, state, data );
		}

		CName contextName = m_currentContext->m_first; // if context changes in response to event stop processing 
		TArrayMap<EInputKey, SInputActionActivator>::iterator iter = m_currentContext->m_second.m_key2activator.Find(key);
		TArrayMap<EInputKey, SInputActionActivator>::iterator end = m_currentContext->m_second.m_key2activator.End();
		for( ; ( iter != end ) && iter->m_first == key && m_currentContext->m_first == contextName; ++iter )
		{
			SInputActionActivator	& activator = iter->m_second;
			SInputAction			& action	= GetInputActionByActivator(activator);
			if( m_ignoredGameInputs.Exist( action.m_aName ) )
			{
				continue;
			}

			if( activator.m_activationState == KS_Duration )
			{
				if( (state == KS_Pressed || ( state == KS_Axis && data != 0.f ) ) && m_durationActivators.FindPtr( &activator ) == NULL ) 
				{
					activator.m_data2.m_timeToActivation = activator.m_data.m_durationIdleTime; 
					m_durationActivators.PushBack( &activator );
				}
				else if( state == KS_Released || ( state == KS_Axis && data == 0.f  ) )
				{
					Bool removed = m_durationActivators.RemoveFast( &activator );
					DeactivateInputAction( action );
				}
			}
			else if( activator.m_activationState == KS_DoubleTap )
			{
				if( state == KS_Pressed )
				{
					if(  Double( GEngine->GetRawEngineTime() ) - activator.m_data2.m_lastClickTime < activator.m_data.m_dblClickTime )
					{
						ActivateInputAction( action );
					}
					else 
					{
						activator.m_data2.m_lastClickTime = GEngine->GetRawEngineTime();
					}
				}
				else if( state == KS_Released )
				{
					DeactivateInputAction( action );
				}
			}
			else if( activator.m_activationState == KS_Axis )	//buttons grouped as axis ex WSAD
			{
				auto iter = m_delayedAxisActivations.Find( &action );
				if( iter == m_delayedAxisActivations.End() )
				{
					iter = m_delayedAxisActivations.Insert( &action, 0.f );
				}
				if( state == KS_Pressed )	
				{
					if( activator.m_data2.m_hasBeenActivated == true ) continue;
					activator.m_data2.m_hasBeenActivated = true;
					iter->m_second += activator.m_data.m_axisValue;
				}
				else if( state == KS_Released )	
				{
					activator.m_data2.m_hasBeenActivated = false;
					iter->m_second = 0.0f;
				}
				else if(state == KS_Axis)		iter->m_second  = data;		
			}
			else if( activator.m_activationState == KS_Pressed ) //press/release buttons
			{
				if( state == KS_Pressed )	ActivateInputAction( action );
				if( state == KS_Axis )
				{
					Float modifiedData = data;
					if( key == IK_Pad_RightAxisY || key == IK_Pad_RightAxisX )
					{
						modifiedData *= GetSensitivityModifier( m_sensitivityPreset );
					}
					ActivateInputAction( action, modifiedData );
				}
				if( state == KS_Released )	DeactivateInputAction( action );
			}
		}
	}

	const InputUtils::EInputDeviceType currentDevice = ( m_lastRecognizedDevice == InputUtils::IDT_MOUSE ) ? InputUtils::IDT_KEYBOARD : m_lastRecognizedDevice;
	if( previousDevice != currentDevice )
	{
		CallEvent( CNAME( OnInputDeviceChanged ) );
	}

	return false;
}

void CInputManager::UpdateInput( Float timeDelta )
{
	if( !m_suppressSendingEvents )
	{
		for( Uint32 i = 0; i < m_durationActivators.Size(); i++ )
		{
			SInputActionActivator	& activator = *m_durationActivators[i];
			if( m_ignoredGameInputs.Exist( activator.m_actionName ) == false && activator.m_activationState == KS_Duration && activator.m_data2.m_timeToActivation != 0.f	)
			{	
				activator.m_data2.m_timeToActivation -= GEngine->GetLastTimeDelta(); //Engine time unscaled
				if( activator.m_data2.m_timeToActivation < 0.f )
				{
					activator.m_data2.m_timeToActivation = 0.f;
					SInputAction & action = GetInputActionByActivator(activator);
					ActivateInputAction( action );			
				}
			}
		}
		for( Int32 i = m_delayedAxisActivations.SizeInt()-1; i >= 0; --i )
		{
			auto pair = m_delayedAxisActivations[i];
			ActivateInputAction( *pair.m_first, pair.m_second );
			if( Abs<Float>(pair.m_second) < NumericLimits<Float>::Epsilon() )
			{
				m_delayedAxisActivations.RemoveAtFast(i);
			}
		}
		Uint64 currentTick = GEngine->GetCurrentEngineTick();
		for( Int32 i = m_delayedValueUpdate.SizeInt()-1; i >= 0; --i )
		{
			auto pair = m_delayedValueUpdate[i];
			if( pair.m_second < currentTick )
			{
				pair.m_first.m_first->m_lastFrameValue = pair.m_first.m_second;
				m_delayedValueUpdate.RemoveAtFast(i);
			}
		}
	}
}

void CInputManager::IgnoreGameInput( const CName& name, Bool ignore )
{
	if( ignore )
	{
		if( !m_ignoredGameInputs.Exist( name ) )
		{
			SInputAction & action = GetInputActionByName(name);
			DeactivateInputAction( action );
			m_ignoredGameInputs.PushBack( name );
			for ( Int32 i = m_durationActivators.Size()-1 ; i >=0 ; i-- )
			{
				if( m_durationActivators[i]->m_actionName == name )
				{
					m_durationActivators.RemoveAtFast(i);
				}
			}
		}
	}
	else
	{
		m_ignoredGameInputs.Remove( name );
	}
}

static Bool ParseKeyDefinition( const Char* txt, String& gameKey, KeyState& activationState, Float& additionalData, Bool& reprocess )
{
	reprocess = false;
	additionalData = 0.f;

	// Header
	GParseWhitespaces( txt );
	if ( !GParseKeyword( txt, TXT("(") ) ) return false;

	// Game key name
	if ( GParseKeyword( txt, TXT("Action=") ) )
	{
		if ( !GParseToken( txt, gameKey ) ) return false;
		GParseKeyword( txt, TXT(",") );
	}

	// State in which input is active
	if ( GParseKeyword( txt, TXT("State=") ) )
	{
		String tempState;
		if ( GParseToken( txt, tempState ) )
		{
			GParseKeyword( txt, TXT(",") );
			if( tempState == TXT("Pressed") )
			{
				activationState = KS_Pressed;
			}
			else if( tempState == TXT("Duration") )
			{
				activationState = KS_Duration;
				if ( GParseKeyword( txt, TXT("IdleTime=") ) )
				{
					if( !GParseFloat( txt, additionalData ) ) return false;
					GParseKeyword( txt, TXT(",") );
				}
			}
			else if( tempState == TXT("DoubleTap") )
			{
				activationState = KS_DoubleTap;
				if ( GParseKeyword( txt, TXT("IdleTime=") ) )
				{
					if( !GParseFloat( txt, additionalData ) ) return false;
					GParseKeyword( txt, TXT(",") );
				}
			}
			else if( tempState == TXT("Axis") )
			{
				activationState = KS_Axis;
				if ( GParseKeyword( txt, TXT("Value=") ) )
				{
					if( !GParseFloat( txt, additionalData ) ) return false;
					GParseKeyword( txt, TXT(",") );
				}
			}
		}		
	}
	else
	{
		activationState = KS_Pressed;
	}

	if( GParseKeyword( txt, TXT( "Reprocess" ) ) )
	{
		reprocess = true;
		GParseKeyword( txt, TXT(",") );		
	}

	// Ending
	if ( !GParseKeyword( txt, TXT(")") ) ) return false;
	return true;
}

Bool CInputManager::LoadKeyDefinitions( Config::Legacy::CConfigLegacyFile* file )
{
	CName previousContext = CName::NONE;
	if( m_currentContext != m_context2bindings.End() )
	{
		previousContext = m_currentContext->m_first;	
	}

	m_context2bindings.ClearFast();
	m_reverseActionKeyMapping.Clear();

	if( !file )
	{
		return false;
	}

	auto end = file->GetSections().End();
	for( auto sectionIt = file->GetSections().Begin(); sectionIt != end; ++sectionIt )
	{
		const CName sectionName = CName( sectionIt->m_first );
		Config::Legacy::CConfigLegacySection* section = sectionIt->m_second;
		if( section == nullptr )
		{
			continue;
		}

		if( sectionName == CNAME(InputSettings) )		// Ignore special section with additional input system settings
		{
			continue;
		}

		auto contextIter = m_context2bindings.Find( sectionName );	
		if( contextIter == m_context2bindings.End() )
		{
			contextIter = m_context2bindings.Insert( sectionName, SInputContext() );		
		}

		SInputContext& context = contextIter->m_second;
		LoadContextFromSection( context, section, file, 32 );
	}

	m_currentContext = m_context2bindings.End();

	if( previousContext != CName::NONE )
	{
		SetCurrentContext( previousContext );
	}

	return true;
}

namespace SaverHelpers
{
	String GetActionName( const SInputActionActivator& activator )
	{
		return activator.m_actionName.AsString();
	}

	String GetKeyState( const SInputActionActivator& activator )
	{
		KeyState actionState = activator.m_activationState;
		String actionStateStr;

		// actionState to string
		switch( actionState )
		{
		case KS_Axis:
			actionStateStr = TXT("Axis");
			break;
		case KS_Duration:
			actionStateStr = TXT("Duration");
			break;
		case KS_DoubleTap:
			actionStateStr = TXT("DoubleTap");
			break;
		case KS_Pressed:
			actionStateStr = String::EMPTY;
			break;
		case KS_Released:
			actionStateStr = String::EMPTY;
			break;
		case KS_None:
			actionStateStr = String::EMPTY;
			break;
		}

		return actionStateStr;
	}

	Float GetActionValue( const SInputActionActivator& activator )
	{
		return activator.m_data.m_axisValue;
	}

	Bool GetActionReprocess( const TSortedArray<SInputAction>& inputEvents, const String& actionName )
	{
		for( const SInputAction& action : inputEvents )
		{
			if( action.m_aName.AsString() == actionName )
			{
				return action.m_reprocessInput;
			}
		}

		return false;
	}

	void GatherContextEntries( CInputMappingSaver& saver, const CName& contextName, const SInputContext& context )
	{
		CInputMappingActionStringBuilder actionStringBuilder;
		TArrayMap<EInputKey, SInputActionActivator> inputActionActivator = context.m_key2activator;

		if( inputActionActivator.Empty() == true )
		{
			saver.AddEmptyContext( contextName );
		}

		for( auto& keyActivatorMapIt : inputActionActivator )
		{
			// Start with fresh builder
			actionStringBuilder.Reset();

			// Get some input structures stuff
			EInputKey key = keyActivatorMapIt.m_first;
			SInputActionActivator& activator = keyActivatorMapIt.m_second;

			RED_FATAL_ASSERT( activator.m_actionName != CName::NONE, "Action can't be none" );

			// Convert that stuff to strings
			String actionName = SaverHelpers::GetActionName( activator );
			String actionState = SaverHelpers::GetKeyState( activator );
			Float idleTimeOrValueOrWhatever = SaverHelpers::GetActionValue( activator );

			const TSortedArray<SInputAction>& inputEvents = context.m_inputEvents;
			Bool reprocess = SaverHelpers::GetActionReprocess( inputEvents, actionName );

			// Put those strings to final string builder
			actionStringBuilder.SetActionName( actionName );
			actionStringBuilder.SetReprocess( reprocess );
			actionStringBuilder.SetState( actionState );
			actionStringBuilder.SetIdleTime( idleTimeOrValueOrWhatever );

			// Construct final string and add entry to saver
			String actionEntryString = actionStringBuilder.Construct();
			saver.AddEntry( contextName, key, actionEntryString );
		}
	}
}

void CInputManager::SaveUserMappings()
{
#ifdef RED_PLATFORM_WINPC
	String userDirectory;
	Bool result = GetOrCreateUserDirectory( userDirectory );
	if( result == false )
	{
		RED_ASSERT( false, TXT("InputManager saving mappings: can't create saving directory.") );
		return;
	}

	String filename = Config::cvUserMappingsConfigFilename.Get();
	String absoluteFilePath = userDirectory + filename + TXT(".") + Consts::userMappingsFileExtension;
	Int32 validVersion = GetValidMappingVersion();
	CInputMappingSaver saver( filename, absoluteFilePath, validVersion );

	for( auto contextIterator = m_context2bindings.Begin(); contextIterator != m_context2bindings.End(); contextIterator++ )
	{
		CName& contextName = contextIterator->m_first;
		SInputContext& context = contextIterator->m_second;

		SaverHelpers::GatherContextEntries( saver, contextName, context );
	}

	saver.Save();
#endif
}

Bool CInputManager::DoesUserMappingsExist(const String& filename)
{
	String absolutePath = GFileManager->GetUserDirectory() + filename + TXT(".") + Consts::userMappingsFileExtension;
	return GFileManager->FileExist( absolutePath );
}

Bool CInputManager::GetOrCreateUserDirectory(String& outUserDirectoryPath)
{
	outUserDirectoryPath = GFileManager->GetUserDirectory();
	return GSystemIO.CreateDirectory( outUserDirectoryPath.AsChar() );
}

void CInputManager::AddBinding( SInputContext & context, EInputKey key, CName actionName, KeyState activationState, Float additionalData, Bool reprocess )
{
	{ // add forward binding
		TSortedArray<SInputAction>::iterator iter = context.m_inputEvents.Find( actionName );
		if( iter == context.m_inputEvents.End() )
		{
			iter = context.m_inputEvents.Insert( SInputAction( actionName ) );	
		}

		if( reprocess )
		{
			iter->m_reprocessInput = true;
		}

		context.m_key2activator.Insert( key, SInputActionActivator( iter->m_aName, activationState, additionalData ) );
	}

	{ // add reverse binding
		auto it = m_reverseActionKeyMapping.Find( actionName );	
		if( it == m_reverseActionKeyMapping.End() )
		{
			it = m_reverseActionKeyMapping.Insert( actionName, TDynArray< EInputKey >() );		
		}

		TDynArray< EInputKey >& a = it->m_second;
		a.PushBackUnique( key );
	}

	if( reprocess )
	{
		Bool registered = false;
		for( auto it = m_reprocessableInput.Begin(); it != m_reprocessableInput.End(); ++it )
		{
			if( ( it->m_first == key ) && ( it->m_second.m_actionName == actionName ) )
			{
				registered = true;
				break;
			}
		}

		if( !registered )
		{
			m_reprocessableInput.Insert( key, SReprocessInput( actionName ) );
		}
	}
}

void CInputManager::LoadContextFromSection( SInputContext& context, Config::Legacy::CConfigLegacySection* section, Config::Legacy::CConfigLegacyFile* file, Uint32 maxDerivationSafetyCheck  )
{
	RED_ASSERT( section );
	
	if ( maxDerivationSafetyCheck == 0 )
	{
		RED_FATAL_ASSERT( false, "Too many derivations, loop possible!" );
		return;
	}


	const auto& items = section->GetItems();
	for ( auto itemIt = items.Begin(); itemIt != items.End(); ++itemIt )
	{
		EInputKey key = IK_None;
		if ( m_keyNameToCodes.Find( itemIt->m_first, key ) )
		{
			Uint32 defsCount = itemIt->m_second.Size();
			for ( Uint32 i = 0; i < defsCount; ++i )
			{
				String		gameKey;
				KeyState	keyState;
				Float		additionalData = 0.f;
				Bool		reprocess;

				if ( ParseKeyDefinition( itemIt->m_second[i].AsChar(), gameKey, keyState, additionalData, reprocess ) )
				{
					// Link game input definition
					CName gameKeyName( gameKey );
					AddBinding( context, key, gameKeyName, keyState, additionalData, reprocess );
				}
			}
		}
		else
		{
			if( itemIt->m_first == TXT( "DERIVED_FROM" ) )
			{
				Uint32 defsCount = itemIt->m_second.Size();
				for ( Uint32 i = 0; i < defsCount; ++i )
				{
					Config::Legacy::CConfigLegacySection* deriveSection = file->GetSection( itemIt->m_second[ i ] );
					if( deriveSection )
					{
						LoadContextFromSection( context, deriveSection, file, ( maxDerivationSafetyCheck - 1 ) );
					}
				}
			}
		}
	}
}

Float CInputManager::GetSensitivityModifier( EInputSensitivityPreset sensitivityPreset )
{
	switch( sensitivityPreset )
	{
	case EInputSensitivityPreset::ISP_Normal: return SInputUserConfig::GetRightStickCameraSensitivity();
	case EInputSensitivityPreset::ISP_Aiming: return SInputUserConfig::GetRightStickAimSensitivity(); 

	default:
		return 1.0f;
	}
}

Bool CInputManager::ProcessSpecialMouseInput( SInputContext& bindings, enum EInputKey key, enum KeyState state, Float data )
{
	if( ( key == IK_MouseX ||  key == IK_MouseY ) && state == KS_Axis )
	{
		TArrayMap<EInputKey, SInputActionActivator>::iterator iter = bindings.m_key2activator.Find(key);
		TArrayMap<EInputKey, SInputActionActivator>::iterator end = bindings.m_key2activator.End();
		for( ; ( iter != end ) && iter->m_first == key ; ++iter )
		{
			SInputAction			& action	= GetInputActionByActivator( iter->m_second );
			if( action.m_aName == CNAME( GI_MouseDampX ) )
			{
				ActivateInputAction( action, data * SInputUserConfig::GetMouseSensitivity() * (SInputUserConfig::GetIsInvertCameraXOnMouse() ? -1.f : 1.f) );
			}
			else if(  action.m_aName == CNAME( GI_MouseDampY ) )
			{
				ActivateInputAction( action, data * SInputUserConfig::GetMouseSensitivity() * (SInputUserConfig::GetIsInvertCameraYOnMouse() ? -1.f : 1.f) );
			}
		}
		return true;
	}	
	return false;
}


void CInputManager::ProcessFakeAxisInput( EInputKey key, KeyState state, Float data )
{
	if( m_currentContext == m_context2bindings.End() )
	{
		return;
	}

	auto cit = m_context2bindings.Find( CNAME( FakeAxisInput ) );
	RED_FATAL_ASSERT( cit != m_context2bindings.End(), "FakeAxisInput context missing" );
	SInputContext& context = cit->m_second;

	TArrayMap<EInputKey, SInputActionActivator>::iterator iter = context.m_key2activator.Find( key );
	TArrayMap<EInputKey, SInputActionActivator>::iterator end = context.m_key2activator.End();
	for( ; ( iter != end ) && iter->m_first == key; ++iter )
	{
		SInputActionActivator	& activator = iter->m_second;
		SInputAction			& action	= GetInputActionByActivator(activator);

		RED_FATAL_ASSERT( activator.m_activationState == KS_Axis, "FakeAxisInput should be filled by only KS_Axis actions" );

		{
			auto iter = m_delayedAxisActivations.Find( &action );

			if( iter == m_delayedAxisActivations.End() )
			{
				iter = m_delayedAxisActivations.Insert( &action, 0.f );
			}

			if( state == KS_Pressed )	
			{
				if( activator.m_data2.m_hasBeenActivated )
				{
					continue;
				}

				activator.m_data2.m_hasBeenActivated = true;
				iter->m_second += activator.m_data.m_axisValue;

				NotifyAxisKeyVal( activator.m_actionName, key, activator.m_data.m_axisValue );
			}
			else if( state == KS_Released )	
			{
				activator.m_data2.m_hasBeenActivated = false;
				iter->m_second = 0.0f;

				NotifyAxisKeyVal( activator.m_actionName, key, 0.0f );
			}
		}
	}
}


void CInputManager::ProcessFakeAxisOnly( const BufferedInput& input )
{
	BufferedInput::const_iterator end = input.End();
	for( BufferedInput::const_iterator it = input.Begin(); it != end; ++it )
	{
		EInputKey key	 = it->key;
		KeyState state	 = (KeyState)it->action;
		Float data		 = it->data;

		InputUtils::EInputDeviceType deviceType = InputUtils::GetDeviceType( key );

		if( deviceType == InputUtils::IDT_KEYBOARD )
		{
			ProcessFakeAxisInput( key, state, data );
		}
	}
}


const TDynArray<CName> CInputManager::GetGameEventNames() const
{
	TDynArray<CName> names;	
	for( auto cIter = m_context2bindings.Begin() ; cIter != m_context2bindings.End(); cIter++ )
	{
		for( auto aIter = cIter->m_second.m_inputEvents.Begin(); aIter != cIter->m_second.m_inputEvents.End(); aIter++ )
		{
			names.PushBack( aIter->m_aName );
		}
	}
	return names;
}

EInputKey CInputManager::FindFirstIKForGameInput( const CName& name, Float activation, Bool isUsingPad ) 
{
	if( m_currentContext != m_context2bindings.End() )
	{
		for( auto aIter = m_currentContext->m_second.m_key2activator.Begin(); aIter != m_currentContext->m_second.m_key2activator.End(); aIter++ )
		{
			const SInputAction & action = GetInputActionByName( aIter->m_second.m_actionName );
			if( FilterPadEvents( aIter->m_first ) == isUsingPad && action.m_aName == name )
			{
				return aIter->m_first;
			}
		}
	}
	return IK_None;
}



void CInputManager::ActivateInputAction( SInputAction & action, Float actiValue )
{
	if( action != SInputAction::INVALID && action.m_value != actiValue )
	{
		action.activationTime =  GEngine->GetRawEngineTime();
		action.m_lastFrameValue = action.m_value;
		action.m_value = actiValue;		
		m_delayedValueUpdate.PushBack(  MakePair( MakePair( &action, action.m_value ), GEngine->GetCurrentEngineTick() ) );
		NotifyActionChange( action );
	}
}

void CInputManager::DeactivateInputAction( SInputAction & action )
{
	if( action.m_value != 0.f )
	{		
		action.m_lastFrameValue = action.m_value;
		action.m_value = 0.f;		
		m_delayedValueUpdate.PushBack(  MakePair( MakePair( &action, action.m_value ), GEngine->GetCurrentEngineTick() ) );
		NotifyActionChange( action );
	}
}

void CInputManager::DeactivateAllInputActions()
{
	if( m_currentContext != m_context2bindings.End() )
	{
		TArrayMap<CName, SInputContext>::iterator tempContext = m_currentContext;;
		for( auto i = tempContext->m_second.m_inputEvents.Begin(); i != tempContext->m_second.m_inputEvents.End(); ++i )
		{
			DeactivateInputAction( *i );
		}
	}
}

void CInputManager::NotifyActionChange( SInputAction& action )
{
	if( m_enableLog && !( action.m_aName == CNAME( GI_MouseDampX ) || action.m_aName == CNAME( GI_MouseDampY ) ) )
	{
		RED_LOG( Input, TXT(" action %s state changed, new value %f"), action.m_aName.AsString().AsChar(), action.m_value );
	}

	// Notify all listeners registered for that particular event name

	auto iter = m_actionListeners.Find( action.m_aName );
	for( ; iter != m_actionListeners.End() && iter->m_first == action.m_aName; )
	{
		if ( iter->m_second && iter->m_second->OnGameInputEvent( action ) )
		{
			m_actionListeners.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	// Notify all listeners registered for all events

	iter = m_actionListeners.Find( CName::NONE );
	for( ; iter != m_actionListeners.End() && !iter->m_first; ++iter )
	{
		if ( iter->m_second && iter->m_second->OnGameInputEvent( action ) )
		{
			m_actionListeners.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}


SInputAction & CInputManager::GetInputActionByActivator( SInputActionActivator & activator )
{
	if( activator.m_actionToActivate == NULL || activator.m_actionToActivate == &SInputAction::INVALID)
	{
		activator.m_actionToActivate = &GetInputActionByName( activator.m_actionName );
	}
	if ( activator.m_actionToActivate )
	{
		return *activator.m_actionToActivate;
	}
	return SInputAction::INVALID;
}


SInputAction & CInputManager::GetInputActionByName( CName name )
{
	if( m_currentContext != m_context2bindings.End() )
	{
		TSortedArray<SInputAction>::iterator eventIter = m_currentContext->m_second.m_inputEvents.Find( name );
		if( eventIter !=  m_currentContext->m_second.m_inputEvents.End() )
		{
			return *eventIter;
		}
	}
	return SInputAction::INVALID;
}

const SInputAction & CInputManager::GetInputActionByName( CName name ) const
{
	if( m_currentContext != m_context2bindings.End() )
	{
		TSortedArray<SInputAction>::const_iterator eventIter = m_currentContext->m_second.m_inputEvents.Find( name );
		if( eventIter !=  m_currentContext->m_second.m_inputEvents.End() )
		{
			return *eventIter;
		}
	}
	return SInputAction::INVALID;
}

SInputAction & CInputManager::GetInputActionByName( CName name, SInputContext& context )
{
	TSortedArray<SInputAction>::iterator eventIter = context.m_inputEvents.Find( name );
	if( eventIter !=  context.m_inputEvents.End() )
	{
		return *eventIter;
	}

	return SInputAction::INVALID;
}

Float CInputManager::GetActionValue( CName actionName ) const
{
	if( LastUsedPCInput() )
	{
		if( actionName == CNAME( GI_AxisLeftX ) || actionName == CNAME( GI_AxisLeftY ) )
		{
			return CheckAxisKeyVal( actionName );
		}
	}

	const SInputAction & action = GetInputActionByName(actionName);
	return action.m_value;
}

Float CInputManager::GetLastActivationTime( CName actionName )
{
	SInputAction & action = GetInputActionByName(actionName);
	return action != SInputAction::INVALID && GEngine ? Min<Float>( 10.f, GEngine->GetRawEngineTime() - action.activationTime ) : FLT_MAX;
}

void CInputManager::NotifyAxisKeyVal( CName axis, EInputKey iKey, Float val )
{
	auto it = m_buttonAxisMapping.Find( axis );
	if( it == m_buttonAxisMapping.End() )
	{
		it = m_buttonAxisMapping.Insert( axis, SButtonAxisMapping() );
	}

	it->m_second.NotifyKeyVal( iKey, val );
}


Float CInputManager::CheckAxisKeyVal( CName axis ) const
{
	auto it = m_buttonAxisMapping.Find( axis );
	if( it == m_buttonAxisMapping.End() )
	{
		return 0.0f;
	}

	if( m_currentContext == m_context2bindings.End() )
	{
		return 0.0f;
	}

	Bool existsInCurrentContext = false;
	auto ieEnd = m_currentContext->m_second.m_inputEvents.End();
	for( auto ieit = m_currentContext->m_second.m_inputEvents.Begin(); ieit != ieEnd; ++ieit )
	{
		if( ieit->m_aName == axis )
		{
			existsInCurrentContext = true;
			break;
		}
	}

	if( !existsInCurrentContext || m_ignoredGameInputs.Exist( axis ) )
	{
		return 0.0f;
	}

	return it->m_second.m_totalVal;
}

CGestureSystem* CInputManager::GetGestureSystem()
{
	if( !( m_gestureSystem ) )
	{
		// works as a singleton factory for gesture system
		// cannot be moved to constructor cause then this pointer is not a valid CObject yet.
		m_gestureSystem = CreateObject< CGestureSystem >( this );
	}

	return m_gestureSystem;
}


void CInputManager::StoreContext( const CName& newContext )
{
	CName contextToStore = ( ( m_currentContext != m_context2bindings.End() ) ? m_currentContext->m_first : CName::NONE );

	RED_ASSERT( contextToStore != CName::NONE );
	m_storedContext.PushBack( contextToStore );

	if( newContext != CName::NONE )
	{
		SetCurrentContext( newContext );
	}
}

void CInputManager::RestoreContext( const CName& storedContext, bool contextCouldChange )
{
	if( m_storedContext.Empty() )
	{
		RED_ASSERT( false, TXT( "Too many restore context calls? Something may break down." ) );
		SetCurrentContext( CNAME( Exploration ) );
		return;
	}

	CName restoredContext = m_storedContext.PopBack();
	RED_ASSERT( restoredContext != CName::NONE );

	CName currContext = ( ( m_currentContext != m_context2bindings.End() ) ? m_currentContext->m_first : CName::NONE );
	RED_ASSERT( currContext != CName::NONE );

	if( currContext != storedContext )
	{
		// the context has changed, so don't restore anymore.
		RED_ASSERT( contextCouldChange, TXT( "Restored context that was marked as 'CAN NOT CHANGE' but it has changed." ) );
		return;
	}

	if( restoredContext != CName::NONE )
	{
		SetCurrentContext( restoredContext );
	}
	else
	{
		SetCurrentContext( CNAME( Exploration ) );
	}
}


void CInputManager::ForceDeactivateAction( const CName& actionName )
{
	SInputAction& action = GetInputActionByName( actionName );
	if( action != SInputAction::INVALID )
	{
		DeactivateInputAction( action );
	}
}


void CInputManager::RegisterListener( IInputListener * listener, CName actionName )
{
	{	// register only if not yet registered
		TArrayMap< CName, IInputListener*>::iterator iter =  m_actionListeners.Find( actionName );
		auto mapEnd = m_actionListeners.End();
		for( ; ( iter != mapEnd ) && ( iter->m_first == actionName ); ++iter )
		{
			if( iter->m_second == listener )
			{
				return;
			}
		}
	}

	m_actionListeners.Insert( actionName, listener );
}

void CInputManager::UnregisterListener( IInputListener * listener, CName actionName  )
{
	TArrayMap< CName, IInputListener*>::iterator iter =  m_actionListeners.Find( actionName );
	auto mapEnd = m_actionListeners.End();
	for( ; ( iter != mapEnd ) && ( iter->m_first == actionName ); ++iter )
	{
		if( iter->m_second == listener )
		{
			m_actionListeners.Erase( iter );
			return;
		}
	}
}

void CInputManager::UnregisterListener( IInputListener * listener )
{
	/// @todo MS: optimise this, it is O(n2) now!!!
	while( !( m_actionListeners.Empty() ) )
	{
		auto mapEnd = m_actionListeners.End();

		for( auto iter =  m_actionListeners.Begin(); ; ++iter )
		{
			if( iter == mapEnd )
			{
				return;
			}

			if( iter->m_second == listener )
			{
				m_actionListeners.Erase( iter );
				break;
			}
		}
	}
}


void CInputManager::GetPCKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys )
{
	if( action == CName::NONE )
	{
		return;
	}

	auto it = m_reverseActionKeyMapping.Find( action );	
	if( it == m_reverseActionKeyMapping.End() )
	{
		return;
	}

	auto a = it->m_second;
	for( Uint32 i = 0; i < a.Size(); ++i )
	{
		InputUtils::EInputDeviceType t = InputUtils::GetDeviceType( a[ i ] );
		if( t != InputUtils::IDT_GAMEPAD  )
		{
			outKeys.PushBack( a[ i ] );
		}
	}
}

void CInputManager::GetPadKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys )
{
	if( action == CName::NONE )
	{
		return;
	}

	auto it = m_reverseActionKeyMapping.Find( action );	
	if( it == m_reverseActionKeyMapping.End() )
	{
		return;
	}

	auto a = it->m_second;
	for( Uint32 i = 0; i < a.Size(); ++i )
	{
		InputUtils::EInputDeviceType t = InputUtils::GetDeviceType( a[ i ] );
		if( t == InputUtils::IDT_GAMEPAD )
		{
			outKeys.PushBack( a[ i ] );
		}
	}
}


void CInputManager::GetCurrentKeysForAction( const CName& action, TDynArray< EInputKey >& outKeys )
{
	if( LastUsedPCInput() )
	{
		GetPCKeysForAction( action, outKeys );
	}
	else if( LastUsedGamepad() )
	{
		GetPadKeysForAction( action, outKeys );
	}
}

void CInputManager::LoadDefaultMappings()
{
	String fileName = GetBindingFileNameBasedOnKeyboardLayout();
	Config::Legacy::CConfigLegacyFile* file = SConfig::GetInstance().GetLegacy().GetFile( fileName.AsChar() );
	LoadKeyDefinitions( file );
}

#ifdef RED_PLATFORM_WINPC
Bool CInputManager::LoadUserMappings()
{
	String userDirectory;
	Bool result = GetOrCreateUserDirectory( userDirectory );
	if( result == false )
	{
		return false;
	}

	Config::Legacy::CConfigLegacyFile* file = SConfig::GetInstance().GetLegacy().GetFileWithAbsolutePathAndExtension( 
		userDirectory.AsChar(), Config::cvUserMappingsConfigFilename.Get().AsChar(), Consts::userMappingsFileExtension.AsChar() );

	result = CheckMappingFileVersionMatch( file );
	if( result == false )
	{
		WARN_ENGINE( TXT("User mappings file version is invalid") );
		return false;
	}

	result = LoadKeyDefinitions( file );
	return result;
}
#endif

Bool CInputManager::CheckMappingFileVersionMatch(Config::Legacy::CConfigLegacyFile* file)
{
	Int32 fileVersion =  GetMappingVersionFromFile( file );
	Int32 validVersion = GetValidMappingVersion();

	return fileVersion == validVersion;
}

Int32 CInputManager::GetMappingVersionFromFile(Config::Legacy::CConfigLegacyFile* file)
{
	const Config::Legacy::TConfigLegacySections& sections = file->GetSections();
	Config::Legacy::CConfigLegacySection* settingsSection = nullptr;
	Bool found = sections.Find( Config::cvInputMappingSettingsSectionName.Get(), settingsSection );
	if( found == true )
	{
		if( settingsSection != nullptr )
		{
			String versionString;
			settingsSection->ReadValue( TXT("Version"), versionString );
			Int32 versionId = 0;
			FromString( versionString, versionId );

			return versionId;
		}
	}

	return -1;
}

Int32 CInputManager::GetValidMappingVersion()
{
	Config::Legacy::CConfigLegacyFile* file = SConfig::GetInstance().GetLegacy().GetFile( Config::cvDefaultMappingsConfigFilename.Get().AsChar() );
	return GetMappingVersionFromFile( file );
}

Bool CInputManager::LastUsedPCInput() const
{
#ifdef RED_PLATFORM_ORBIS
	return false;
#endif

#ifdef RED_PLATFORM_DURANGO
	return false;
#endif

	return ( ( m_lastRecognizedDevice == InputUtils::IDT_KEYBOARD ) || ( m_lastRecognizedDevice == InputUtils::IDT_MOUSE ) );
}

Bool CInputManager::LastUsedGamepad() const
{
#ifdef RED_PLATFORM_ORBIS
	return true;
#endif

#ifdef RED_PLATFORM_DURANGO
	return true;
#endif

	if ( Config::cvForcePad.Get() )
	{
		return true;
	}

	return ( m_lastRecognizedDevice == InputUtils::IDT_GAMEPAD );
}


void CInputManager::ShareAxisActivators( SInputContext& oldContext, SInputContext& newContext )
{
	TDynArray< CName > axisActions;
	axisActions.Reserve( 4 );

	for( auto it = oldContext.m_key2activator.Begin(); it != oldContext.m_key2activator.End(); ++it )
	{
		if( it->m_second.m_activationState == KS_Axis )
		{
			auto it2 = newContext.m_key2activator.Find( it->m_first );
			if( it2 == newContext.m_key2activator.End() )
			{
				continue;
			}

			if( it2->m_second.m_activationState != KS_Axis )
			{
				continue;
			}

			if( it->m_second.m_actionName != it2->m_second.m_actionName )
			{
				continue;
			}

			it2->m_second.m_data2.m_hasBeenActivated = it->m_second.m_data2.m_hasBeenActivated;

			axisActions.PushBackUnique( it->m_second.m_actionName );
		}
	}

	for( Uint32 i = 0; i < axisActions.Size(); ++i )
	{
		SInputAction& ia1 = GetInputActionByName( axisActions[ i ], oldContext );
		SInputAction& ia2 = GetInputActionByName( axisActions[ i ], newContext );

		if( ia1 != SInputAction::INVALID && ia2 != SInputAction::INVALID )
		{
			ia2.m_value = ia1.m_value;
			ia2.m_lastFrameValue = ia1.m_lastFrameValue;
			ia2.activationTime = ia1.activationTime;
		}

		for( auto it = m_delayedAxisActivations.Begin(); it != m_delayedAxisActivations.End(); ++it )
		{
			if( it->m_first->m_aName == axisActions[ i ] )
			{
				it->m_second = ia1.m_value;
			}
		}
	}
}

/****************************** Input binding ******************************/

void CInputManager::SetKeyboardMouseActionBinding(const CName& action, EInputKey mainKey, EInputKey alternativeKey)
{
	if( action == CNAME(MoveForward) || action == CNAME(MoveBackward) || action == CNAME(MoveLeft) || action == CNAME(MoveRight) )
	{
		// Hack for movement binding
		SetKeybardMouseActionBindingMovement( action, mainKey, alternativeKey );
	}
	else
	{
		// Regular input binding
		SetKeybardMouseActionBindingRegular( action, mainKey, alternativeKey );
	}
}

void CInputManager::SetKeybardMouseActionBindingRegular( const CName& action, EInputKey mainKey, EInputKey alternativeKey )
{
	TDynArray<SInputContext*> contexts;
	FindAllContextsForAction( action, contexts );

	for( SInputContext* context : contexts )
	{
		SInputActionActivator activator;
		Bool found = EraseActionFromContext( context, action, false, activator );
		if( found == false )
		{
			continue;
		}

		// Assign new bindings for that activator
		// Always insert main keys, even if it's IK_None (which means - no key is bound to the action)
		{
			RED_FATAL_ASSERT( activator.m_actionName != CName::NONE, "Action can't be none" );
			context->m_key2activator.Insert( mainKey, activator );
		}

		if( alternativeKey != IK_None )		// Don't insert alternative keys if they are only IK_None (we have main key for that)
		{
			RED_FATAL_ASSERT( activator.m_actionName != CName::NONE, "Action can't be none" );
			context->m_key2activator.Insert( alternativeKey, activator );
		}

		ClearActionCacheForContext( context );
	}

	RebindReverseMappingForKeyboardMouse( action, mainKey, alternativeKey );
}

void CInputManager::SetKeybardMouseActionBindingMovement( const CName& action, EInputKey mainKey, EInputKey alternativeKey )
{
	RED_FATAL_ASSERT( action == CNAME(MoveForward) || action == CNAME(MoveBackward) || action == CNAME(MoveLeft) || action == CNAME(MoveRight), "Incorrect action name for movement binding." );

	SetKeybardMouseActionBindingRegular( action, mainKey, alternativeKey );		// Bind regular action (e.g. MoveForward)
	
	// Get the axis action name for current movement action
	CName axisActionToRebind;
	Bool isPositiveAxisValue;

	if( action == CNAME(MoveForward) )					// Forward
	{
		axisActionToRebind = CNAME(GI_AxisLeftY);
		isPositiveAxisValue = true;
	}
	else if( action == CNAME(MoveBackward) )			// Backward
	{
		axisActionToRebind = CNAME(GI_AxisLeftY);
		isPositiveAxisValue = false;
	}
	else if( action == CNAME(MoveLeft) )				// Left
	{
		axisActionToRebind = CNAME(GI_AxisLeftX);
		isPositiveAxisValue = false;
	}
	else												// Right
	{
		axisActionToRebind = CNAME(GI_AxisLeftX);
		isPositiveAxisValue = true;
	}

	TDynArray<SInputContext*> contexts;
	FindAllContextsForAction( axisActionToRebind, contexts );

	for( SInputContext* context : contexts )
	{
		SInputActionActivator activator;
		Bool found = EraseMovementActionFromContext( context, axisActionToRebind, isPositiveAxisValue, false, activator );
		if( found == false )
		{
			continue;
		}

		// Assign new bindings for that activator
		// Always insert main keys, even if it's IK_None (which means - no key is bound to the action)
		{
			RED_FATAL_ASSERT( activator.m_actionName != CName::NONE, "Action can't be none" );
			context->m_key2activator.Insert( mainKey, activator );
		}

		if( alternativeKey != IK_None )		// Don't insert alternative keys if they are only IK_None (we have main key for that)
		{
			RED_FATAL_ASSERT( activator.m_actionName != CName::NONE, "Action can't be none" );
			context->m_key2activator.Insert( alternativeKey, activator );
		}

		ClearActionCacheForContext( context );
	}
}

void CInputManager::SetPadActionBinding( const CName& action, EInputKey key )
{
	TDynArray<SInputContext*> contexts;
	FindAllContextsForAction( action, contexts );

	for( SInputContext* context : contexts )
	{
		SInputActionActivator activator;
		Bool found = EraseActionFromContext( context, action, true, activator );
		if( found == false )
		{
			continue;
		}

		// Assign new binding for that activator
		// Always insert main keys, even if it's IK_None (which means - no key is bound to the action)
		{
			context->m_key2activator.Insert( key, activator );
		}

		ClearActionCacheForContext( context );
	}

	RebindReverseMappingForPad( action, key );
}

void CInputManager::ClearActionCacheForContext( SInputContext* context )
{
	// Clear action cache
	TArrayMap<EInputKey, SInputActionActivator>& keysArray = context->m_key2activator;
	TArrayMap<EInputKey, SInputActionActivator>::iterator keysIterator = keysArray.Begin();
	TArrayMap<EInputKey, SInputActionActivator>::iterator keysIteratorEnd = keysArray.End();
	for( ; keysIterator != keysIteratorEnd; ++keysIterator )
	{
		keysIterator->m_second.m_actionToActivate = nullptr;
	}
}

void CInputManager::FindAllContextsForAction(const CName& action, TDynArray<SInputContext*>& outContexts)
{
	auto contextIterator = m_context2bindings.Begin();
	const auto& contextIteratorEnd = m_context2bindings.End();

	// Iterate all contexts and look for the one with that action
	for(; contextIterator != contextIteratorEnd; ++contextIterator )
	{
		SInputContext& context = contextIterator->m_second;

		Bool hasAction = DoesContextHasAction( context, action );
		if( hasAction == true )
		{
			outContexts.PushBack( &(contextIterator->m_second) );
		}
	}
}

Bool CInputManager::DoesContextHasAction(const SInputContext& context, const CName& action)
{
	auto activatorIterator = context.m_key2activator.Begin();
	const auto& activatorIteratorEnd = context.m_key2activator.End();

	for(; activatorIterator != activatorIteratorEnd; ++activatorIterator )
	{
		const SInputActionActivator& currentActivator = activatorIterator->m_second;
		if( currentActivator.m_actionName == action )
		{
			return true;
		}
	}

	return false;
}

Bool CInputManager::EraseActionFromContext(SInputContext* context, const CName& action, Bool isPad, SInputActionActivator& outErasedActivator)
{
	Bool found = false;

	// Assuming that the action is in the given context
	for( Int32 i=context->m_key2activator.Size()-1; i>=0; --i )
	{
		TPair< EInputKey, SInputActionActivator >& activatorPair = context->m_key2activator[i];
		EInputKey currentActivatorKey = activatorPair.m_first;
		SInputActionActivator& currentActivator = activatorPair.m_second;

		if( currentActivator.m_actionName == action && FilterPadEvents( currentActivatorKey ) == isPad )
		{
			// Keep erasing all activators for given action and device (isPad)
			// Remember only last activator (activators for the same device should be identical)
			RED_FATAL_ASSERT( currentActivator.m_actionName != CName::NONE, "Action can't be none" );
			outErasedActivator = currentActivator;
			found = true;
			context->m_key2activator.RemoveAt( i );
		}
	}

	return found;
}

Bool CInputManager::EraseMovementActionFromContext(SInputContext* context, const CName& action, Bool isPositiveValue, Bool isPad, SInputActionActivator& outErasedActivator)
{
	Bool found = false;

	// Assuming that the action is in the given context
	for( Int32 i=context->m_key2activator.Size()-1; i>=0; --i )
	{
		TPair< EInputKey, SInputActionActivator >& activatorPair = context->m_key2activator[i];
		EInputKey currentActivatorKey = activatorPair.m_first;
		SInputActionActivator& currentActivator = activatorPair.m_second;

		if( currentActivator.m_actionName == action && FilterPadEvents( currentActivatorKey ) == isPad )
		{
			// Erase only one axis action - distinguish them based on axis value
			if( ((currentActivator.m_data.m_axisValue > 0.0f) && (isPositiveValue == true)) || ((currentActivator.m_data.m_axisValue < 0.0f) && (isPositiveValue == false)) )
			{
				// Keep erasing all activators for given action and device (isPad)
				// Remember only last activator (activators for the same device should be identical)
				RED_FATAL_ASSERT( currentActivator.m_actionName != CName::NONE, "Action can't be none" );
				outErasedActivator = currentActivator;
				found = true;
				context->m_key2activator.RemoveAt( i );
			}
		}
	}

	return found;
}

void CInputManager::RebindReverseMappingForKeyboardMouse(const CName& action, EInputKey mainKey, EInputKey alternativeKey)
{
	TDynArray< EInputKey >& keys = FindOrCreateReverseMappingForAction( action );
	RemoveAllKeysForDevice( keys, false );	// Erase old keyboard mouse mappings

	if( mainKey != IK_None )
	{
		keys.PushBackUnique( mainKey );
	}

	if( alternativeKey != IK_None )
	{
		keys.PushBackUnique( alternativeKey );
	}
}

void CInputManager::RebindReverseMappingForPad(const CName& action, EInputKey key)
{
	TDynArray< EInputKey >& keys = FindOrCreateReverseMappingForAction( action );
	RemoveAllKeysForDevice( keys, true );	// Erase old pad mappings

	if( key != IK_None )
	{
		keys.PushBackUnique( key );
	}
}

TDynArray<EInputKey>& CInputManager::FindOrCreateReverseMappingForAction( const CName& action )
{
	auto it = m_reverseActionKeyMapping.Find( action );	
	if( it == m_reverseActionKeyMapping.End() )
	{
		it = m_reverseActionKeyMapping.Insert( action, TDynArray< EInputKey >() );		
	}

	return it->m_second;
}

void CInputManager::RemoveAllKeysForDevice( TDynArray<EInputKey>& keys, Bool isPad )
{
	for( Int32 i=(Int32)keys.Size()-1; i>=0; --i )
	{
		if( FilterPadEvents( keys[i] ) == isPad )
		{
			keys.RemoveAt( i );
		}
	}
}


void CInputManager::ReprocessInput()
{
	if( m_currentContext == m_context2bindings.End() )
	{
		return;
	}

	BufferedInput fakeBufferedInput;

	for( auto& action : m_currentContext->m_second.m_inputEvents )
	{
		if( action.m_reprocessInput && ( action.m_value == 0.0f ) )
		{
			for( auto& reprocessable : m_reprocessableInput )
			{
				if( ( reprocessable.m_second.m_actionName == action.m_aName ) && ( reprocessable.m_second.m_pressed ) )
				{
					SBufferedInputEvent fakeInputEvent;
					fakeInputEvent.key = reprocessable.m_first;
					fakeInputEvent.action = IACT_Press;
					fakeInputEvent.data = 1.0f;
					fakeBufferedInput.PushBack( fakeInputEvent );
				}
			}
		}
	}

	if( !fakeBufferedInput.Empty() )
	{
		ProcessInput( fakeBufferedInput );
	}
}


/***************************************************************************/

void CInputManager::funcRegisterListener( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable >, _listener, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;

	IScriptable* listener = _listener.Get();
	if( listener )
	{
		for( Uint32 i = 0; i < m_scriptActionListeners.Size(); ++i )
		{
			if( actionName == m_scriptActionListeners[i]->GetActionName() )
			{
				//only one script listener per action is allowed - if exists overwrite
				*m_scriptActionListeners[i] = CScriptInputListener( listener, eventName, actionName );
				return;
			}
		}
		//dont have listener for this action yet - register
		m_scriptActionListeners.PushBack( new CScriptInputListener( listener, eventName, actionName ) );
		RegisterListener( m_scriptActionListeners.Back() , actionName );
	}
}

void CInputManager::funcUnregisterListener( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable >, _listener, NULL );
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;

	for( Uint32 i = 0 ; i < m_scriptActionListeners.Size() ; ++i ) 	
	{
		if( _listener == m_scriptActionListeners[i]->GetHandle() && actionName == m_scriptActionListeners[i]->GetActionName() )
		{
			UnregisterListener( m_scriptActionListeners[i], actionName );
			delete m_scriptActionListeners[i];
			m_scriptActionListeners.RemoveAtFast( i );		
		}
	}
}

void CInputManager::funcGetLastActivationTime( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetLastActivationTime( actionName ) );
}

void CInputManager::funcGetActionValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetActionValue( actionName ) );
}

void CInputManager::funcGetAction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_STRUCT( SInputAction, GetInputActionByName(actionName) );
}

void CInputManager::funcIgnoreGameInput( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Bool, ignore, true );
	FINISH_PARAMETERS;

	IgnoreGameInput( name, ignore );
}

void CInputManager::funcClearIgnoredInput( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ClearIgnoredInput();
}

void CInputManager::funcIsInputIgnored( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( m_ignoredGameInputs.Exist( name ) );
}

void CInputManager::funcSetContext( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, context, CName::NONE );
	FINISH_PARAMETERS;

	SetCurrentContext( context );
}


void CInputManager::funcGetContext( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( m_currentContext != m_context2bindings.End() ? m_currentContext->m_first : CName::NONE );
}


void CInputManager::funcStoreContext( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, newContext, CName::NONE );
	FINISH_PARAMETERS;

	StoreContext( newContext );
}


void CInputManager::funcRestoreContext( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, storedContext, CName::NONE );
	GET_PARAMETER( Bool, couldHaveChanged, false );
	FINISH_PARAMETERS;
	RestoreContext( storedContext, couldHaveChanged );
}


void CInputManager::funcEnableLog( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, log, false );
	FINISH_PARAMETERS;
	m_enableLog = log;
}

void CInputManager::funcLastUsedPCInput( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( LastUsedPCInput() );
}

void CInputManager::funcLastUsedGamepad( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( LastUsedGamepad() );
}

void CInputManager::funcForceDeactivateAction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	FINISH_PARAMETERS;
	ForceDeactivateAction( actionName );
}

void CInputManager::funcGetPCKeysForAction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;
	GetPCKeysForAction( actionName, outKeys );
}

void CInputManager::funcGetPadKeysForAction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;
	GetPadKeysForAction( actionName, outKeys );
}

void CInputManager::funcGetCurrentKeysForAction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, actionName, CName::NONE );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;
	GetCurrentKeysForAction( actionName, outKeys );
}

void CInputManager::funcGetPCKeysForActionStr( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, actionName, String() );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;

	GetPCKeysForAction( CName( actionName ), outKeys );
}

void CInputManager::funcGetPadKeysForActionStr( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, actionName, String() );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;
	GetPadKeysForAction( CName( actionName ), outKeys );
}

void CInputManager::funcGetCurrentKeysForActionStr( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, actionName, String() );
	GET_PARAMETER_REF( TDynArray< EInputKey >, outKeys, TDynArray< EInputKey >() );
	FINISH_PARAMETERS;
	GetCurrentKeysForAction( CName( actionName ), outKeys );
}

void CInputManager::funcUsesPlaystationPad( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifdef RED_PLATFORM_ORBIS
	RETURN_BOOL( true );
#else
	RETURN_BOOL( false );
#endif
}

void CInputManager::funcSetInvertCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, invert, false );
	FINISH_PARAMETERS;
	SInputUserConfig::SetIsInvertCameraY( invert ); 
}

const CName CInputManager::GetLastUsedDeviceName() const
{
	IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
	if( inputDeviceManager != nullptr )
	{
		return inputDeviceManager->GetLastUsedDeviceName();
	}

	return CName::NONE;
}

void CInputManager::funcGetLastUsedDeviceName(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetLastUsedDeviceName() );
}

String CInputManager::GetBindingFileNameBasedOnKeyboardLayout() const
{
#ifdef RED_PLATFORM_WINPC
	// Default result is input_qwerty filename
	String result = Config::cvDefaultMappingsConfigFilename.Get();

	// Get out local id
	String keyboardType = HardwareInstrumentation::GetCurrentKeyboardLocaleID();

	// Open description XML file
	String filepath = GFileManager->GetBaseDirectory() + Config::cvDefaultLayoutDescriptionFilename.Get();
	String xmlContent;
	Bool fileLoaded = GFileManager->LoadFileToString( filepath, xmlContent, true );

	if( fileLoaded == false )
	{
		WARN_ENGINE( TXT("Can't load file for keyboard layout to binding file description: %ls"), filepath.AsChar() );
	}
	else
	{
		CXMLReader reader( xmlContent );

		if( reader.BeginNode( TXT("KeyboardLayoutToFileMapping") ) == true )
		{
			// Iterate through all defined layout descriptors
			while( reader.BeginNode( TXT("Layout") ) == true )
			{
				String idsString;
				reader.Attribute( TXT("ids"), idsString );

				Bool keyboardLocaleIdFound = false;

				// Check if this layout contains our locale id
				TDynArray<String> ids = idsString.Split( TXT(";") );
				for( const String& id : ids )
				{
					if( keyboardType == id )
					{
						keyboardLocaleIdFound = true;
						break;
					}
				}

				// If so, then return this layout's filename
				if( keyboardLocaleIdFound == true )
				{
					reader.Attribute( TXT("file"), result );
					break;
				}

				reader.EndNode();
			}

			reader.EndNode( false );
		}
		else
		{
			WARN_ENGINE( TXT("Can't load keyboard layout to binding file. Using QWERTY keyboard layout as default. File is corrupted: %ls"), filepath.AsChar() );
		}
	}

	return result;
#else
	return Config::cvDefaultMappingsConfigFilename.Get();
#endif
}

//#pragma optimize("",on)
