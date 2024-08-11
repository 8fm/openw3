/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "SFLib.h"

#include <crtdbg.h>
#include <guiddef.h>
#include <objidl.h>
#include <typeinfo.h>

// serializer.

using namespace SoundFrame;

//-------------------------------------------------------------------------------------------------
//  

CSoundFrame::CSoundFrame( IClient * pClient )
	: m_hwndServer( NULL )
	, m_hwndMessage( NULL )
	, m_pClient( pClient )
	, m_pCopyData( NULL )
	, m_bProjectInfoValid( false )
	, m_dwProcessID( 0 )
{
	m_wszProjectName[ 0 ] = 0;
	m_wszOriginalPath[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	m_cfWObjectID = static_cast<CLIPFORMAT>( ::RegisterClipboardFormat( _T("WwiseObjectID") ) ); // FIXME: 
}

CSoundFrame::~CSoundFrame()
{
	_ASSERT( m_pCopyData == NULL );

	if ( m_hwndServer )
		::SendMessage( m_hwndServer, WM_SF_SHUTDOWN, (WPARAM) m_hwndMessage, 0 );

	if ( m_hwndMessage ) 
		::DestroyWindow( m_hwndMessage );	
}

bool CSoundFrame::Connect( DWORD in_dwProcessID )
{
	m_dwProcessID = in_dwProcessID;

	if ( m_hwndMessage == NULL )
	{
		m_hwndMessage = ::CreateWindow( _T( "STATIC" ), SF_CLIENTWINDOWNAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
		::SetWindowLongPtr( m_hwndMessage, GWLP_WNDPROC, (LONG_PTR) MsgWndProc );
		::SetWindowLongPtr( m_hwndMessage, GWLP_USERDATA, (LONG_PTR) this );
	}

	HWND hwndServer = ::FindWindowEx( HWND_MESSAGE, NULL, NULL, SF_SERVERWINDOWNAME ) ;
	while ( hwndServer )
	{
		DWORD dwProcessID = 0;
		DWORD dwThreadID = ::GetWindowThreadProcessId( hwndServer, &dwProcessID );

		if ( in_dwProcessID == 0
			|| dwProcessID == in_dwProcessID )
		{
			::PostMessage( hwndServer, WM_SF_STARTUP, (WPARAM) m_hwndMessage, 0 );
			return true;
		}

		hwndServer = ::FindWindowEx( HWND_MESSAGE, hwndServer, NULL, SF_SERVERWINDOWNAME );
	}

	return false;
}

bool CSoundFrame::IsConnected()  const
{
	return m_hwndServer != NULL;
}

bool CSoundFrame::PlayEvents( const AkUniqueID * in_pEvents, long in_cEvents, AkGameObjectID in_gameObjectID )
{
	if ( !in_pEvents || !in_cEvents || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<int>( in_cEvents );

	for ( int i = 0; i < in_cEvents; ++i )
		bytes.Write<AkUniqueID>( in_pEvents[ i ] );

	SendCopyData( SF_COPYDATA_PLAYEVENTS, bytes );

	return true;
}

bool CSoundFrame::PlayEvents( LPCWSTR * in_pszEvents, long in_cEvents, AkGameObjectID in_gameObjectID )
{
	if ( !in_pszEvents || !in_cEvents || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<int>( in_cEvents );

	for ( int i = 0; i < in_cEvents; ++i )
		bytes.WriteString( in_pszEvents[ i ] );

	SendCopyData( SF_COPYDATA_PLAYEVENTSBYSTRING, bytes );

	return true;
}

bool CSoundFrame::ExecuteActionOnEvent( AkUniqueID in_eventID, AK::SoundEngine::AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID /* = AK_INVALID_GAME_OBJECT */, AkTimeMs in_uTransitionDuration /* = 0 */, AkCurveInterpolation in_eFadeCurve /* = AkCurveInterpolation_Linear */ )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUniqueID>( in_eventID );
	bytes.Write<AK::SoundEngine::AkActionOnEventType>( in_ActionType );
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<AkTimeMs>( in_uTransitionDuration );
	bytes.Write<AkCurveInterpolation>( in_eFadeCurve );

	SendCopyData( SF_COPYDATA_EXECUTEACTIONONEVENT, bytes );

	return true;
}

/** NOT IMPLEMENTED
bool CSoundFrame::ResolveDialogueEvent( AkUniqueID in_dialogueEventID, const AkUniqueID * in_aArgumentValues, long in_cNumArguments, ISoundObject ** out_ppSoundObject )
{
	if ( !in_aArgumentValues || !in_aArgumentValues || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUniqueID>( in_dialogueEventID );
	bytes.Write<int>( in_cNumArguments );

	for ( int i = 0; i < in_cNumArguments; ++i )
		bytes.Write<AkUniqueID>( in_aArgumentValues[ i ] );

	SendCopyData( SF_COPYDATA_RESOLVE_DIALOGUEEVENT, bytes );

	if ( m_pCopyData )
	{
		*out_ppSoundObject = (ISoundObject *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppSoundObject = NULL;
		return false;
	}
}

bool CSoundFrame::ResolveDialogueEvent( LPCWSTR in_pszDialogueEvent, LPCWSTR * in_aArgumentValues, long in_cNumArguments, ISoundObject ** out_ppSoundObject )
{
	if ( !in_aArgumentValues || !in_cNumArguments || !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszDialogueEvent );
	bytes.Write<int>( in_cNumArguments );

	for ( int i = 0; i < in_cNumArguments; ++i )
		bytes.WriteString( in_aArgumentValues[ i ] );

	SendCopyData( SF_COPYDATA_RESOLVE_DIALOGUEEVENTBYSTRING, bytes );

	if ( m_pCopyData )
	{
		*out_ppSoundObject = (ISoundObject *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppSoundObject = NULL;
		return false;
	}
}
**/

bool CSoundFrame::SetPlayBackMode( bool in_bPlayback ) const
{
	if ( !m_hwndServer )
		return false;

	::SendMessage( m_hwndServer, in_bPlayback? WM_SF_PLAY : WM_SF_STOP, (WPARAM) m_hwndMessage, 0 );

	return true;
}

bool CSoundFrame::GetCurrentState( AkStateGroupID in_stateGroupID, IState** out_ppCurrentState ) const
{
	if ( !m_hwndServer || !out_ppCurrentState )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false );	// We are not using string
	bytes.Write<AkStateGroupID>( in_stateGroupID );

	SendCopyData( SF_COPYDATA_REQCURRENTSTATE, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentState = (IState *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentState = NULL;
		return false;
	}
}

bool CSoundFrame::GetCurrentState( LPCWSTR in_szStateGroupName, IState** out_ppCurrentState ) const
{
	if ( !m_hwndServer || !in_szStateGroupName || !out_ppCurrentState )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true );	// We are using string
	bytes.WriteString( in_szStateGroupName );

	SendCopyData( SF_COPYDATA_REQCURRENTSTATE, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentState = (IState *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentState = NULL;
		return false;
	}
}

bool CSoundFrame::SetCurrentState( AkStateGroupID in_stateGroupID, AkStateID in_currentStateID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false );	// Not using string
	bytes.Write<AkStateGroupID>( in_stateGroupID );
	bytes.Write<AkStateGroupID>( in_currentStateID );

	SendCopyData( SF_COPYDATA_SETCURRENTSTATE, bytes );

	return true;
}

bool CSoundFrame::SetCurrentState( LPCWSTR in_szStateGroupName, LPCWSTR in_szCurrentStateName )
{
	if ( !m_hwndServer || !in_szStateGroupName || !in_szCurrentStateName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true );	// Using string
	bytes.WriteString( in_szStateGroupName );
	bytes.WriteString( in_szCurrentStateName );

	SendCopyData( SF_COPYDATA_SETCURRENTSTATE, bytes );

	return true;
}

bool CSoundFrame::GetCurrentSwitch( AkSwitchGroupID in_switchGroupID, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID ) const
{
	if ( !m_hwndServer || !out_ppCurrentSwitch )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not using string
	bytes.Write<AkSwitchGroupID>( in_switchGroupID );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_REQCURRENTSWITCH, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentSwitch = (ISwitch *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentSwitch = NULL;
		return false;
	}
}

bool CSoundFrame::GetCurrentSwitch( LPCWSTR in_szSwitchGroupName, ISwitch** out_ppCurrentSwitch, AkGameObjectID in_gameObjectID ) const
{
	if ( !m_hwndServer || !out_ppCurrentSwitch )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Not using string
	bytes.WriteString( in_szSwitchGroupName );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_REQCURRENTSWITCH, bytes );

	if ( m_pCopyData )
	{
		*out_ppCurrentSwitch = (ISwitch *) m_pCopyData;

		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppCurrentSwitch = NULL;
		return false;
	}
}

bool CSoundFrame::SetCurrentSwitch( AkSwitchGroupID in_switchGroupID, AkSwitchStateID in_currentSwitchID, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not using String
	bytes.Write<AkSwitchGroupID>( in_switchGroupID );
	bytes.Write<AkSwitchStateID>( in_currentSwitchID );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETCURRENTSWITCH, bytes );

	return true;
}

bool CSoundFrame::SetCurrentSwitch( LPCWSTR in_szSwitchGroupName, LPCWSTR in_szCurrentSwitchName, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szSwitchGroupName || !in_szCurrentSwitchName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using String
	bytes.WriteString( in_szSwitchGroupName );
	bytes.WriteString( in_szCurrentSwitchName );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETCURRENTSWITCH, bytes );

	return true;
}

bool CSoundFrame::RegisterGameObject( AkGameObjectID in_gameObjectID, LPCWSTR in_szGameObjectName )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.WriteString( in_szGameObjectName );

	SendCopyData( SF_COPYDATA_REGISTERGAMEOBJECT, bytes );

	return true;
}

bool CSoundFrame::UnregisterGameObject( AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_UNREGISTERGAMEOBJECT, bytes );

	return true;
}

bool CSoundFrame::SetRTPCValue( AkRtpcID in_gameParameterID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not Using string
	bytes.Write<AkRtpcID>( in_gameParameterID );
	bytes.Write<AkRtpcValue>( in_value );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::SetRTPCValue( LPCWSTR in_szGameParameterName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szGameParameterName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using string
	bytes.WriteString( in_szGameParameterName );
	bytes.Write<AkRtpcValue>( in_value );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_SETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::ResetRTPCValue( AkRtpcID in_gameParameterID, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not Using string
	bytes.Write<AkRtpcID>( in_gameParameterID );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_RESETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::ResetRTPCValue( LPCWSTR in_szGameParameterName, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szGameParameterName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using string
	bytes.WriteString( in_szGameParameterName );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_RESETRTPCVALUE, bytes );

	return true;
}

bool CSoundFrame::PostTrigger( AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( false ); // Not Using String
	bytes.Write<AkTriggerID>( in_triggerID );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_TRIGGER, bytes );

	return true;
}

bool CSoundFrame::PostTrigger( LPCWSTR in_szTriggerName, AkGameObjectID in_gameObjectID )
{
	if ( !m_hwndServer || !in_szTriggerName )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<bool>( true ); // Using String
	bytes.WriteString( in_szTriggerName );
	WriteGameObjectID( bytes, in_gameObjectID );

	SendCopyData( SF_COPYDATA_TRIGGER, bytes );

	return true;
}

bool CSoundFrame::SetActiveListeners( AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<AkUInt32>( in_uiListenerMask );

	SendCopyData( SF_COPYDATA_SETACTIVELISTENER, bytes );
	
	return true;
}
	
bool CSoundFrame::SetAttenuationScalingFactor( AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_GameObjectID );
	bytes.Write<AkReal32>( in_fAttenuationScalingFactor );

	SendCopyData( SF_COPYDATA_SETATTENUATIONSCALINGFACTOR, bytes );

	return true;
}

bool CSoundFrame::SetMultiplePositions(AkGameObjectID in_GameObjectID, const AkSoundPosition * in_pPositions, AkUInt16 in_NumPositions, AK::SoundEngine::MultiPositionType in_eMultiPositionType /* = MultiPositionType_MultiDirections */ )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_GameObjectID );
	if (!in_pPositions)
		in_NumPositions = 0;
		
	bytes.Write<AkUInt16>( in_NumPositions );
	if (in_NumPositions)
	{
		for (AkUInt16 i(0); i < in_NumPositions; ++i)
		{
			bytes.Write<AkSoundPosition>( in_pPositions[i] );
		}
	}
	bytes.Write<AK::SoundEngine::MultiPositionType>( in_eMultiPositionType );

	SendCopyData( SF_COPYDATA_SETMULTIPLEPOSITIONS, bytes );

	return true;
}

bool CSoundFrame::SetPosition( AkGameObjectID in_gameObjectID, const AkSoundPosition& in_rPosition )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<AkSoundPosition>( in_rPosition );

	SendCopyData( SF_COPYDATA_SETPOSITION, bytes );

	return true;
}

bool CSoundFrame::SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_uiIndex )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkListenerPosition>( in_rPosition );
	bytes.Write<AkUInt32>( in_uiIndex );

	SendCopyData( SF_COPYDATA_SETLISTENERPOSITION, bytes );

	return true;
}

bool CSoundFrame::SetListenerScalingFactor( AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUInt32>( in_uiIndex );
	bytes.Write<AkReal32>( in_fAttenuationScalingFactor );

	SendCopyData( SF_COPYDATA_SETLISTENERSCALINGFACTOR, bytes );

	return true;
}

bool CSoundFrame::SetListenerSpatialization( AkUInt32 in_uiIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUInt32>( in_uiIndex );
	bytes.Write<bool>( in_bSpatialized );
	bytes.Write<bool>( in_pVolumeOffsets? true : false );	// Do we have a Speaker Volumes struct

	if( in_pVolumeOffsets )
		bytes.Write<AkSpeakerVolumes>( *in_pVolumeOffsets );

	SendCopyData( SF_COPYDATA_SETLISTENERSPATIALIZATION, bytes );

	return true;
}

bool CSoundFrame::SetGameObjectAuxSendValues( AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<AkUInt32>( in_aEnvironmentValues? in_uNumEnvValues : 0 );

	if( in_aEnvironmentValues && in_uNumEnvValues > 0 )
	{
		for(unsigned int i = 0; i < in_uNumEnvValues; ++i)
			bytes.Write<AkAuxSendValue>( in_aEnvironmentValues[i] );
	}

	SendCopyData( SF_COPYDATA_SETGAMEOBJECTENVIRONMENTSVALUES, bytes );

	return true;
}
	
bool CSoundFrame::SetGameObjectOutputBusVolume( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_gameObjectID );
	bytes.Write<AkReal32>( in_fControlValue );

	SendCopyData( SF_COPYDATA_SETGAMEOBJECTDRYLEVELVALUE, bytes );

	return true;
}
	
bool CSoundFrame::SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_ObjectID );
	bytes.Write<AkUInt32>( in_uListener );
	bytes.Write<AkReal32>( in_fObstructionLevel );
	bytes.Write<AkReal32>( in_fOcclusionLevel );

	SendCopyData( SF_COPYDATA_SETOBJECTOBSTRUCTIONANDOCCLUSION, bytes );

	return true;
}

bool CSoundFrame::PostMsgMonitor( LPCWSTR in_pszMessage )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszMessage );

	SendCopyData( SF_COPYDATA_POSTMONITORINGMESSAGE, bytes );

	return true;
}
bool CSoundFrame::StopAll( AkGameObjectID in_GameObjID )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	WriteGameObjectID( bytes, in_GameObjID );

	SendCopyData( SF_COPYDATA_STOPALL, bytes );

	return true;
}

bool CSoundFrame::StopPlayingID( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration /* = 0 */, AkCurveInterpolation in_eFadeCurve /* = AkCurveInterpolation_Linear */ )
{
	if ( !m_hwndServer )
		return false;
	
	SFWriteBytesMem bytes;
	bytes.Write<AkPlayingID>( in_playingID );
	bytes.Write<AkTimeMs>( in_uTransitionDuration );
	bytes.Write<AkCurveInterpolation>( in_eFadeCurve );

	SendCopyData( SF_COPYDATA_STOPPLAYINGID, bytes );

	return true;
}

bool CSoundFrame::GetEventList( IEventList ** out_ppEventList ) const
{
	return GetObjectList( out_ppEventList, WM_SF_REQEVENTS );
}

bool CSoundFrame::GetEvent( AkUniqueID in_eventID, IEvent ** out_ppEvent ) const
{
	return GetObject<IEvent, IEventList>( in_eventID, out_ppEvent, SFType_Event );
}

bool CSoundFrame::GetEvent( LPCWSTR in_pszEvent, IEvent ** out_ppEvent ) const
{
	return GetObjectByString<IEvent, IEventList>( in_pszEvent, out_ppEvent, SFType_Event );
}

bool CSoundFrame::GetEvents( const AkUniqueID * in_pEvents, long in_cEvents, IEventList ** out_ppEventList ) const
{
	return GetObjects( in_pEvents, in_cEvents, out_ppEventList, SFType_Event );
}

bool CSoundFrame::GetEvents( LPCWSTR * in_pszEvents, long in_cEvents, IEventList ** out_ppEventList ) const
{
	return GetObjectsByString( in_pszEvents, in_cEvents, out_ppEventList, SFType_Event );
}

bool CSoundFrame::GetDialogueEventList( IDialogueEventList ** out_ppDialogueEventList ) const
{
	return GetObjectList( out_ppDialogueEventList, WM_SF_REQDIALOGUEEVENTS );
}

bool CSoundFrame::GetDialogueEvent( AkUniqueID in_dialogueEventID, IDialogueEvent ** out_ppDialogueEvent ) const
{
	return GetObject<IDialogueEvent, IDialogueEventList>( in_dialogueEventID, out_ppDialogueEvent, SFType_DialogueEvent );
}

bool CSoundFrame::GetDialogueEvent( LPCWSTR in_pszDialogueEvent, IDialogueEvent ** out_ppDialogueEvent ) const
{
	return GetObjectByString<IDialogueEvent, IDialogueEventList>( in_pszDialogueEvent, out_ppDialogueEvent, SFType_DialogueEvent );
}

bool CSoundFrame::GetDialogueEvents( const AkUniqueID * in_pDialogueEvents, long in_cDialogueEvents, IDialogueEventList ** out_ppDialogueEventList ) const
{
	return GetObjects( in_pDialogueEvents, in_cDialogueEvents, out_ppDialogueEventList, SFType_DialogueEvent );
}

bool CSoundFrame::GetDialogueEvents( LPCWSTR * in_pszDialogueEvents, long in_cDialogueEvents, IDialogueEventList ** out_ppDialogueEventList ) const
{
	return GetObjectsByString( in_pszDialogueEvents, in_cDialogueEvents, out_ppDialogueEventList, SFType_DialogueEvent );
}

CSoundFrame::DnDType 
	CSoundFrame::GetDnDType( IDataObject * in_pDataObject )
{
	FORMATETC fmt;
	fmt.cfFormat = m_cfWObjectID;
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.lindex = -1;
	fmt.ptd = NULL;
	fmt.tymed = TYMED_HGLOBAL;

	DnDType result = TypeUnknown;

	HRESULT hr = in_pDataObject->QueryGetData( &fmt );
	if ( hr == S_OK )
	{
		STGMEDIUM stg;

		hr = in_pDataObject->GetData( &fmt, &stg );
		if ( hr == S_OK )
		{
			SFReadBytesMem bytes( ::GlobalLock( stg.hGlobal ), (long) ::GlobalSize( stg.hGlobal ) );

			long count = bytes.Read<long>();
			if ( count )
			{
				for ( long i = 0; i < count; i++ )
				{
					long objectType = bytes.Read<long>();
					long customParam = bytes.Read<long>();
					DnDType type = ConvertType( objectType, customParam );
					UINT32 uiId = bytes.Read<UINT32>();

					if( type == TypeUnknown )
					{
						result = TypeUnknown;
						break;
					}
					
					if( result == TypeUnknown )
					{
						result = type;
					}
					else if( type != result )
					{
						result = TypeUnknown;
						break;
					}
				}
			}

			::GlobalUnlock( stg.hGlobal );
            ReleaseStgMedium( &stg );
		}
	}

	return result;
}

bool CSoundFrame::ProcessEventDnD( IDataObject * in_pDataObject, IEventList ** out_ppEventList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppEventList, SFType_Event );
}

bool CSoundFrame::ProcessStateGroupDnD( IDataObject * in_pDataObject, IStateGroupList ** out_ppStateGroupList	)
{
	return ProcessObjectDnD( in_pDataObject, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::ProcessSwitchGroupDnD( IDataObject * in_pDataObject, ISwitchGroupList ** out_ppSwitchGroupList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppSwitchGroupList, SFType_SwitchGroup );
}

bool CSoundFrame::ProcessGameParameterDnD( IDataObject * in_pDataObject, IGameParameterList ** out_ppGameParameterList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppGameParameterList, SFType_GameParameter );
}

bool CSoundFrame::ProcessTriggerDnD( IDataObject * in_pDataObject,	ITriggerList ** out_ppTriggerList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppTriggerList, SFType_Trigger );
}

bool CSoundFrame::ProcessAuxBusDnD( IDataObject * in_pDataObject,	IAuxBusList ** out_ppAuxBusList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppAuxBusList, SFType_AuxBus );
}

bool CSoundFrame::ProcessDialogueEventDnD( IDataObject * in_pDataObject, IDialogueEventList ** out_ppDialogueEventList )
{
	return ProcessObjectDnD( in_pDataObject, out_ppDialogueEventList, SFType_DialogueEvent );
}

CSoundFrame::DnDType CSoundFrame::ConvertType( long in_lType, long in_lCustomParam )
{
	switch( in_lType )
	{
	case WEventType:
		return TypeEvent;
	case WStateGroupType:
		return TypeStates;
	case WSwitchGroupType:
		return TypeSwitches;
	case WGameParameterType:
		return TypeGameParameters;
	case WTriggerType:
		return TypeTriggers;
	case WAuxBusType:
		return TypeAuxBus;
	case WDialogueEventType:
		return TypeDialogueEvent;
	}

	return TypeUnknown;
}

bool CSoundFrame::GetDnDObjectList( IDataObject * in_pDataObject, AkUniqueID ** out_pIDArray, long& out_lCount )
{
	FORMATETC fmt;
	fmt.cfFormat = m_cfWObjectID;
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.lindex = -1;
	fmt.ptd = NULL;
	fmt.tymed = TYMED_HGLOBAL;

	bool result = false;

	HRESULT hr = in_pDataObject->QueryGetData( &fmt );
	if ( hr == S_OK )
	{
		STGMEDIUM stg;

		hr = in_pDataObject->GetData( &fmt, &stg );
		if ( hr == S_OK )
		{
			SFReadBytesMem bytes( ::GlobalLock( stg.hGlobal ), (long) ::GlobalSize( stg.hGlobal ) );

			out_lCount = bytes.Read<long>();
			if ( out_lCount )
			{
				(*out_pIDArray) = new AkUniqueID[ out_lCount ];

				for ( long i = 0; i < out_lCount; ++i )
				{
					long type = bytes.Read<long>();
					long customParam = bytes.Read<long>();
					(*out_pIDArray)[ i ] = bytes.Read<UINT32>();
				}

				result = true;
			}

			::GlobalUnlock( stg.hGlobal );
            ReleaseStgMedium( &stg );
		}
	}

	return result;
}

bool CSoundFrame::GetSoundObject( AkUniqueID in_soundObjectID, ISoundObject ** out_ppSoundObject ) const
{
	return GetObject<ISoundObject, ISoundObjectList>( in_soundObjectID, out_ppSoundObject, SFType_SoundObject );
}

bool CSoundFrame::GetSoundObjects( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects, ISoundObjectList ** out_ppSoundObjectList ) const
{
	return GetObjects( in_pSoundObjects, in_nSoundObjects, out_ppSoundObjectList, SFType_SoundObject );
}

bool CSoundFrame::GetStateGroupList( IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjectList( out_ppStateGroupList, WM_SF_REQSTATES );
}

bool CSoundFrame::GetStateGroup( AkUniqueID in_stateGroupID, IStateGroup ** out_ppStateGroup ) const
{
	return GetObject<IStateGroup, IStateGroupList>( in_stateGroupID, out_ppStateGroup, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroup( LPCWSTR in_pszStateGroup, IStateGroup ** out_ppStateGroup ) const
{
	return GetObjectByString<IStateGroup, IStateGroupList>( in_pszStateGroup, out_ppStateGroup, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroups( const AkUniqueID * in_pStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjects( in_pStateGroups, in_cStateGroups, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::GetStateGroups( LPCWSTR * in_pszStateGroups, long in_cStateGroups, IStateGroupList ** out_ppStateGroupList ) const
{
	return GetObjectsByString( in_pszStateGroups, in_cStateGroups, out_ppStateGroupList, SFType_StateGroup );
}

bool CSoundFrame::GetSwitchGroupList( ISwitchGroupList ** out_ppSwitchGroupList	) const
{
	return GetObjectList( out_ppSwitchGroupList, WM_SF_REQSWITCHES );
}
	
bool CSoundFrame::GetSwitchGroup( AkUniqueID in_switchGroupID,	ISwitchGroup ** out_ppSwitchGroup	) const
{
	return GetObject<ISwitchGroup, ISwitchGroupList>( in_switchGroupID, out_ppSwitchGroup, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroup( LPCWSTR in_pszSwitchGroup, ISwitchGroup ** out_ppSwitchGroup ) const
{
	return GetObjectByString<ISwitchGroup, ISwitchGroupList>( in_pszSwitchGroup, out_ppSwitchGroup, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroups( const AkUniqueID * in_pSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const
{
	return GetObjects( in_pSwitchGroups, in_cSwitchGroups, out_ppSwitchGroupList, SFType_SwitchGroup );
}
	
bool CSoundFrame::GetSwitchGroups( LPCWSTR * in_pszSwitchGroups, long in_cSwitchGroups, ISwitchGroupList ** out_ppSwitchGroupList ) const
{
	return GetObjectsByString( in_pszSwitchGroups, in_cSwitchGroups, out_ppSwitchGroupList, SFType_SwitchGroup );
}

bool CSoundFrame::GetGameObjectList( IGameObjectList ** out_ppGameObjectList ) const
{
	return GetObjectList( out_ppGameObjectList, WM_SF_REQGAMEOBJECTS );
}

bool CSoundFrame::GetEventOriginalFileList( LPCWSTR  in_pszEvent, IOriginalFileList ** out_ppOriginalFileList ) const
{
	if ( !in_pszEvent ) 
		return false;

	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszEvent );

	SendCopyData( SF_COPYDATA_REQORIGINALFROMEVENT, bytes );

	if ( m_pCopyData )
	{
		*out_ppOriginalFileList = (IOriginalFileList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppOriginalFileList = NULL;
		return false;
	}
}

bool CSoundFrame::GetDialogueEventOriginalFileList( LPCWSTR in_pszDialogueEvent, IOriginalFileList ** out_ppOriginalFileList ) const
{
	if ( !in_pszDialogueEvent ) 
		return false;

	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszDialogueEvent );

	SendCopyData( SF_COPYDATA_REQORIGINALFROMDIALOGUEEVENT, bytes );

	if ( m_pCopyData )
	{
		*out_ppOriginalFileList = (IOriginalFileList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppOriginalFileList = NULL;
		return false;
	}
}

bool CSoundFrame::EventHasVoiceContent( LPCWSTR  in_pszEvent ) const
{
	if ( !in_pszEvent ) 
		return false;

	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszEvent );

	SendCopyData( SF_COPYDATA_REQEVENTHASVOICECONTENT, bytes );

	bool bHasVoiceContent = false;
	if ( m_pCopyData )
	{
		bHasVoiceContent = *((bool*)m_pCopyData);

		delete m_pCopyData;
		m_pCopyData = NULL;
	}
	return bHasVoiceContent;
}

bool CSoundFrame::DialogueEventHasVoiceContent( LPCWSTR  in_pszDialogueEvent	) const
{
	if ( !in_pszDialogueEvent ) 
		return false;

	SFWriteBytesMem bytes;
	bytes.WriteString( in_pszDialogueEvent );

	SendCopyData( SF_COPYDATA_REQDIALOGUEEVENTHASVOICECONTENT, bytes );

	bool bHasVoiceContent = false;
	if ( m_pCopyData )
	{
		bHasVoiceContent = *((bool*)m_pCopyData);

		delete m_pCopyData;
		m_pCopyData = NULL;

		return true;
	}
	return bHasVoiceContent;
}

bool CSoundFrame::GetGameParameterList( IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjectList( out_ppGameParameterList, WM_SF_REQGAMEPARAMETERS );
}

bool CSoundFrame::GetConversionSettingsList( IConversionSettingsList ** out_ppConversionSettingsList ) const
{
	return GetObjectList( out_ppConversionSettingsList, WM_SF_REQCONVERSIONSETTINGS );
}

bool CSoundFrame::GetGameParameter( AkUniqueID in_gameParameterID, IGameParameter ** out_ppGameParameter ) const
{
	return GetObject<IGameParameter, IGameParameterList>( in_gameParameterID, out_ppGameParameter, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameter( LPCWSTR in_pszGameParameter, IGameParameter ** out_ppGameParameter ) const
{
	return GetObjectByString<IGameParameter, IGameParameterList>( in_pszGameParameter, out_ppGameParameter, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameters( const AkUniqueID * in_pGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjects( in_pGameParameters, in_cGameParameters, out_ppGameParameterList, SFType_GameParameter );
}
	
bool CSoundFrame::GetGameParameters( LPCWSTR * in_pszGameParameters, long in_cGameParameters, IGameParameterList ** out_ppGameParameterList ) const
{
	return GetObjectsByString( in_pszGameParameters, in_cGameParameters, out_ppGameParameterList, SFType_GameParameter );
}

bool CSoundFrame::GetTriggerList( ITriggerList ** out_ppTriggerList ) const
{
	return GetObjectList( out_ppTriggerList, WM_SF_REQTRIGGERS );
}
	
bool CSoundFrame::GetTrigger( AkUniqueID in_triggerID, ITrigger ** out_ppTrigger	) const
{
	return GetObject<ITrigger, ITriggerList>( in_triggerID, out_ppTrigger, SFType_Trigger );
}
	
bool CSoundFrame::GetTrigger( LPCWSTR in_pszTrigger, ITrigger ** out_ppTrigger ) const
{
	return GetObjectByString<ITrigger, ITriggerList>( in_pszTrigger, out_ppTrigger, SFType_Trigger );
}
	
bool CSoundFrame::GetTriggers( const AkUniqueID * in_pTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const
{
	return GetObjects( in_pTriggers, in_cTriggers, out_ppTriggerList, SFType_Trigger );
}
	
bool CSoundFrame::GetTriggers( LPCWSTR * in_pszTriggers, long in_cTriggers, ITriggerList ** out_ppTriggerList ) const
{
	return GetObjectsByString( in_pszTriggers, in_cTriggers, out_ppTriggerList, SFType_Trigger );
}

bool CSoundFrame::GetArgument( AkUniqueID in_ArgumentID, IArgument ** out_ppArgument ) const
{
	return GetObject<IArgument, IArgumentList>( in_ArgumentID, out_ppArgument , SFType_Argument );
}

bool CSoundFrame::GetArguments( const AkUniqueID * in_pArguments, long in_cArguments, IArgumentList ** out_ppArgumentList ) const
{
	return GetObjects( in_pArguments, in_cArguments, out_ppArgumentList, SFType_Argument  );
}

bool CSoundFrame::GetAuxBusList( IAuxBusList ** out_ppAuxBusList ) const
{
	return GetObjectList( out_ppAuxBusList, WM_SF_REQAUXBUS );
}
	
bool CSoundFrame::GetAuxBus( AkUniqueID in_environmentID, IAuxBus ** out_ppAuxBus ) const
{
	return GetObject<IAuxBus, IAuxBusList>( in_environmentID, out_ppAuxBus, SFType_AuxBus );
}
	
bool CSoundFrame::GetAuxBus( LPCWSTR in_pszAuxBus, IAuxBus ** out_ppAuxBus ) const
{
	return GetObjectByString<IAuxBus, IAuxBusList>( in_pszAuxBus, out_ppAuxBus, SFType_AuxBus );
}
	
bool CSoundFrame::GetAuxBus( const AkUniqueID * in_pAuxBus, long in_cAuxBus, IAuxBusList ** out_ppAuxBusList ) const
{
	return GetObjects( in_pAuxBus, in_cAuxBus, out_ppAuxBusList, SFType_AuxBus );
}
	
bool CSoundFrame::GetAuxBus( LPCWSTR * in_pszAuxBus, long in_cAuxBus, IAuxBusList ** out_ppAuxBusList ) const
{
	return GetObjectsByString( in_pszAuxBus, in_cAuxBus, out_ppAuxBusList, SFType_AuxBus );
}

const WCHAR * CSoundFrame::GetCurrentProjectName() const
{
	if ( m_hwndServer && !m_bProjectInfoValid )
	{
		::SendMessage( m_hwndServer, WM_SF_REQPROJ, (WPARAM) m_hwndMessage, 0 );
	}

	return m_wszProjectName;
}

const WCHAR * CSoundFrame::GetCurrentProjectOriginalRoot() const
{
	if ( m_hwndServer && !m_bProjectInfoValid )
	{
		::SendMessage( m_hwndServer, WM_SF_REQPROJ, (WPARAM) m_hwndMessage, 0 );
	}

	return m_wszOriginalPath;
}

GUID CSoundFrame::GetCurrentProjectID() const
{
	if ( m_hwndServer && !m_bProjectInfoValid )
	{
		::SendMessage( m_hwndServer, WM_SF_REQPROJ, (WPARAM) m_hwndMessage, 0 );
	}

	return m_guidProject;
}

bool CSoundFrame::GetAkUniqueID( const GUID& in_guid, AkUniqueID& out_uniqueID ) const
{
	if ( !m_hwndServer )
	{
		out_uniqueID = AK_INVALID_UNIQUE_ID;
		return false;
	}

	SFWriteBytesMem bytes;
	bytes.Write<GUID>( in_guid );

	SendCopyData( SF_COPYDATA_REQAKUNIQUEID, bytes );

	if ( m_pCopyData )
	{
		out_uniqueID = *((AkUniqueID*)m_pCopyData);

		delete m_pCopyData;
		m_pCopyData = NULL;

		return true;
	}
	else
	{
		out_uniqueID = AK_INVALID_UNIQUE_ID;
		return false;
	}
}

bool CSoundFrame::ProcessDefinitionFiles( LPCWSTR * in_pszPaths, long in_nFiles )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<int>( in_nFiles );

	for ( int i = 0; i < in_nFiles; ++i )
		bytes.WriteString( in_pszPaths[ i ] );

	SendCopyData( SF_COPYDATA_PROCESSDEFINITIONFILES, bytes );

	return true;
}

bool CSoundFrame::GenerateSoundBanks( LPCWSTR * in_pszBanks, long in_nBanks, 
									  LPCWSTR * in_pszPlatforms, long in_nPlatforms,
									  LPCWSTR * in_pszLanguages, long in_nLanguages )
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<int>( in_nBanks );
	for ( int i = 0; i < in_nBanks; ++i )
		bytes.WriteString( in_pszBanks[ i ] );

	bytes.Write<int>( in_nPlatforms );
	for ( int i = 0; i < in_nPlatforms; ++i )
		bytes.WriteString( in_pszPlatforms[ i ] );

	bytes.Write<int>( in_nLanguages );
	for ( int i = 0; i < in_nLanguages; ++i )
		bytes.WriteString( in_pszLanguages[ i ] );

	m_pCopyData = (void *) 1; // default to WWISE_ERROR_CODE_ERROR == 1

	SendCopyData( SF_COPYDATA_GENERATESOUNDBANKS, bytes );

	bool bResult = (AkUInt32) m_pCopyData != 1; 
	m_pCopyData = NULL;

	return bResult;
}

bool CSoundFrame::ConvertExternalSources( LPCWSTR * in_pszPlatforms, long in_cPlatforms,
											LPCWSTR * in_pszFileSourcesInput, long in_cFileSourcesInput,
											LPCWSTR * in_pszFileSourcesOutput, long in_cFileSourcesOutput )
											
{
	if ( !m_hwndServer )
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<int>( in_cPlatforms );
	for ( int i = 0; i < in_cPlatforms; ++i )
		bytes.WriteString( in_pszPlatforms[ i ] );

	bytes.Write<int>( in_cFileSourcesInput );
	for ( int i = 0; i < in_cFileSourcesInput; ++i )
		bytes.WriteString( in_pszFileSourcesInput[ i ] );
		
	bytes.Write<int>( in_cFileSourcesOutput );
	for ( int i = 0; i < in_cFileSourcesOutput; ++i )
		bytes.WriteString( in_pszFileSourcesOutput[ i ] );

	m_pCopyData = (void *) 1; // default to WWISE_ERROR_CODE_ERROR == 1

	SendCopyData( SF_COPYDATA_CONVERTEXTERNALSOURCES, bytes );

	bool bResult = (AkUInt32) m_pCopyData != 1; 
	m_pCopyData = NULL;

	return bResult;
}

// -----------------------------------------------------------------------------

bool CSoundFrame::ListenAttenuation( const AkUniqueID * in_pSoundObjects, long in_nSoundObjects )
{
	if ( !m_hwndServer )
		return false;
	
	COPYDATASTRUCT sCopyData;

	sCopyData.dwData = SF_COPYDATA_LISTENRADIUSFORSOUNDS;
	sCopyData.cbData = in_nSoundObjects * sizeof( AkUniqueID );
	sCopyData.lpData = (void *) in_pSoundObjects;

	::SendMessage( m_hwndServer, WM_COPYDATA, (WPARAM) m_hwndMessage, (LPARAM) &sCopyData );

	return true;
}

// -----------------------------------------------------------------------------

void CSoundFrame::OnServerStartup( HWND in_hwndServer, ULONG in_ulServerVersion )
{
	// Do not accept connection from incompatible server.
	if ( in_ulServerVersion != SF_SOUNDFRAME_VERSION_CURRENT )
		return;

	// Do not connect if already connected.
	if ( m_hwndServer != NULL )
		return;

	// Filter by process id if requested.
	if ( m_dwProcessID != 0 )
	{
		DWORD dwProcessID = 0;
		DWORD dwThreadID = ::GetWindowThreadProcessId( in_hwndServer, &dwProcessID );
		if ( dwProcessID != m_dwProcessID )
			return;
	}

	m_bProjectInfoValid = false;
	m_wszProjectName[ 0 ] = 0;
	m_wszOriginalPath[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	m_hwndServer = in_hwndServer;

	::SendMessage( m_hwndServer, WM_SF_CLIENTVERSION, (WPARAM) m_hwndMessage, (LPARAM) SF_SOUNDFRAME_VERSION_CURRENT );

	if ( m_pClient )
	{
		m_pClient->OnConnect( true );
	}
}

void CSoundFrame::OnServerShutdown()
{
	m_hwndServer = NULL;

	m_bProjectInfoValid = false;
	m_wszProjectName[ 0 ] = 0;
	m_wszOriginalPath[ 0 ] = 0;
	m_guidProject = GUID_NULL;

	if ( m_pClient )
	{
		m_pClient->OnConnect( false );
	}
}

template< class TObject, class TList >
TList* CSoundFrame::GetCopyDataObjectList( SFReadBytesMem& bytes ) const
{
	long cObjects = bytes.Read<long>();

	TList * pList = new TList();
	pList->SetSize( cObjects );

	for ( int i = 0; i < cObjects; ++i )
	{
		pList->SetAt( i, TObject::From( &bytes ) );
	}

	return pList;
}

template<class TList>
bool CSoundFrame::ProcessObjectDnD( IDataObject * in_pDataObject, TList ** out_ppObjectList, SFObjectType in_eType )
{
	AkUniqueID* pIdArray = NULL;
	long lGuidCount = 0;

	bool result = GetDnDObjectList( in_pDataObject, &pIdArray, lGuidCount );

	if( result )
	{
		result = GetObjects( pIdArray, lGuidCount, out_ppObjectList, in_eType );

		delete [] pIdArray;
	}

	return result;
}

template<class TList>
bool CSoundFrame::GetObjectList( TList ** out_ppObjectList, UINT in_msg ) const
{
	if ( !out_ppObjectList ) 
		return false;

	::SendMessage( m_hwndServer, in_msg, (WPARAM) m_hwndMessage, 0 );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

template<class TObject, class TList>
bool CSoundFrame::GetObject( AkUniqueID in_id, TObject ** out_pObject, SFObjectType in_eType ) const
{
	if ( !out_pObject ) 
		return false;

	TList * pObjectList = NULL;
	bool result = GetObjects( &in_id, 1, &pObjectList, in_eType );
	if ( result )
	{
		*out_pObject = pObjectList->Next();
		if ( *out_pObject )
			(*out_pObject)->AddRef();
		pObjectList->Release();
	}
	else
	{
		*out_pObject = NULL;
	}

	return result;
}

template<class TObject, class TList>
bool CSoundFrame::GetObjectByString( LPCWSTR in_szObject, TObject ** out_pObject, SFObjectType in_eType ) const
{
	if ( !out_pObject ) 
		return false;

	TList * pObjectList = NULL;
	bool result = GetObjectsByString( &in_szObject, 1, &pObjectList, in_eType );
	if ( result )
	{
		*out_pObject = pObjectList->Next();
		if ( *out_pObject )
			(*out_pObject)->AddRef();
		pObjectList->Release();
	}
	else
	{
		*out_pObject = NULL;
	}

	return result;
}

template<class TList>
bool CSoundFrame::GetObjects( const AkUniqueID * in_pObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const
{
	if ( !out_ppObjectList || !in_cObjects ) 
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<long>( in_eType );
	bytes.Write<long>( in_cObjects );

	for( int i = 0; i < in_cObjects; ++i )
		bytes.Write<AkUniqueID>( in_pObjects[i] );

	SendCopyData( SF_COPYDATA_REQOBJECTS, bytes );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

template<class TList>
bool CSoundFrame::GetObjectsByString( LPCWSTR * in_pszObjects, long in_cObjects, TList ** out_ppObjectList, SFObjectType in_eType ) const
{
	if ( !out_ppObjectList || !in_cObjects ) 
		return false;

	SFWriteBytesMem bytes;

	bytes.Write<long>( in_eType );
	bytes.Write<long>( in_cObjects );

	for ( int i = 0; i < in_cObjects; ++i )
		bytes.WriteString( in_pszObjects[ i ] );

	SendCopyData( SF_COPYDATA_REQOBJECTSBYSTRING, bytes );

	if ( m_pCopyData )
	{
		*out_ppObjectList = (TList *) m_pCopyData;
		m_pCopyData = NULL;
		return true;
	}
	else
	{
		*out_ppObjectList = NULL;
		return false;
	}
}

void CSoundFrame::SendCopyData( DWORD in_dwMessageID, SFWriteBytesMem& in_rWriteByte ) const
{
	COPYDATASTRUCT sCopyData = {0};

	sCopyData.dwData = in_dwMessageID;
	sCopyData.cbData = in_rWriteByte.Count();
	sCopyData.lpData = in_rWriteByte.Bytes();

	::SendMessage( m_hwndServer, WM_COPYDATA, (WPARAM) m_hwndMessage, (LPARAM) &sCopyData );
}

BOOL CSoundFrame::ReceiveCopyData( HWND hwndSender, COPYDATASTRUCT * pCopyData )
{
	// Reject WM_COPYDATA from unknown source.
	if ( ( m_hwndServer == NULL ) || ( m_hwndServer != hwndSender ) )
		return FALSE;

	switch( pCopyData->dwData )
	{
	case SF_COPYDATA_OBJECTLIST:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );		

			SFObjectType eType = (SFObjectType) bytes.Read<long>();

			switch( eType )
			{
			case SFType_Event:
				m_pCopyData = GetCopyDataObjectList<SFEvent, EventList>( bytes );
				break;
			case SFType_DialogueEvent:
				m_pCopyData = GetCopyDataObjectList<SFDialogueEvent, DialogueEventList>( bytes );
				break;
			case SFType_SoundObject:
				m_pCopyData = GetCopyDataObjectList<SFSoundObject, SoundObjectList>( bytes );
				break;
			case SFType_StateGroup:
				m_pCopyData = GetCopyDataObjectList<SFStateGroup, StateGroupList>( bytes );
				break;
			case SFType_SwitchGroup:
				m_pCopyData = GetCopyDataObjectList<SFSwitchGroup, SwitchGroupList>( bytes );
				break;
			case SFType_GameObject:
				m_pCopyData = GetCopyDataObjectList<SFGameObject, GameObjectList>( bytes );
				break;
			case SFType_GameParameter:
				m_pCopyData = GetCopyDataObjectList<SFGameParameter, GameParameterList>( bytes );
				break;
			case SFType_Trigger:
				m_pCopyData = GetCopyDataObjectList<SFTrigger, TriggerList>( bytes );
				break;
			case SFType_AuxBus:
				m_pCopyData = GetCopyDataObjectList<SFAuxBus, AuxBusList>( bytes );
				break;
			case SFType_Argument:
				m_pCopyData = GetCopyDataObjectList<SFArgument, ArgumentList>( bytes );
				break;
			case SFType_ConversionSettings:
				m_pCopyData = GetCopyDataObjectList<SFConversionSettings, ConversionSettingsList>( bytes );
				break;
			default:
				_ASSERT("Missing SFObjectType!!");
			}
		}
		break;

	case SF_COPYDATA_NOTIF:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			SFNotifData notifData = bytes.Read<SFNotifData>();
			
			switch ( notifData.eType )
			{
			case SFType_Event:
				m_pClient->OnEventNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_DialogueEvent:
				m_pClient->OnDialogueEventNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_SoundObject:
				m_pClient->OnSoundObjectNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_StateGroup:
				m_pClient->OnStatesNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_SwitchGroup:
				m_pClient->OnSwitchesNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_GameObject:
				m_pClient->OnGameObjectsNotif( (IClient::Notif) notifData.eNotif, (AkGameObjectID)notifData.objectID );
				break;

			case SFType_GameParameter:
				m_pClient->OnGameParametersNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_Trigger:
				m_pClient->OnTriggersNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_Argument:
				m_pClient->OnArgumentsNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			case SFType_AuxBus:
				m_pClient->OnAuxBusNotif( (IClient::Notif) notifData.eNotif, (AkUniqueID)notifData.objectID );
				break;

			default:
				_ASSERT("Missing SFObjectType!!");
			}
		}
		break;

	case SF_COPYDATA_PROJ:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_guidProject = bytes.Read<GUID>();
			bytes.ReadString( m_wszProjectName, kStrSize );
			bytes.ReadString( m_wszOriginalPath, MAX_PATH );

			m_bProjectInfoValid = true;
		}
		break;

	case SF_COPYDATA_CURRENTSTATE:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = (VOID*) SFState::From( &bytes );
		}
		break;

	case SF_COPYDATA_CURRENTSWITCH:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = (VOID*) SFSwitch::From( &bytes );
		}
		break;

	case SF_COPYDATA_AKUNIQUEID:
		{
			// Answer to SF_COPYDATA_REQAKUNIQUEID

			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = new AkUniqueID;

			long readCount = 0;
			bytes.ReadBytes( m_pCopyData, sizeof(AkUniqueID), readCount );
			_ASSERT( readCount == sizeof(AkUniqueID) );
		}
		break;

	case SF_COPYDATA_ORIGINALLIST:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = GetCopyDataObjectList<SFOriginalFile, OriginalFileList>( bytes );
		}
		break;

	case SF_COPYDATA_HASVOICECONTENT:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			m_pCopyData = new bool;

			long readCount = 0;
			bytes.ReadBytes( m_pCopyData, sizeof(bool), readCount );
			_ASSERT( readCount == sizeof(bool) );
		}
		break;

	case SF_COPYDATA_OBJECTPATH:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );
			m_pCopyData = bytes.ReadAndCopyString();
		}
		break;

	case SF_COPYDATA_CONVERTEXTERNALSOURCESRESULT:
	case SF_COPYDATA_GENERATESOUNDBANKSRESULT:
		{
			SFReadBytesMem bytes( pCopyData->lpData, pCopyData->cbData );

			AkUInt32 result = 0;

			long readCount = 0;
			bytes.ReadBytes( &result, sizeof(AkUInt32), readCount );
			_ASSERT( readCount == sizeof(AkUInt32) );

			m_pCopyData = (void *) result;
		}
		break;

	default: // Unknown data ! 
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------

LRESULT CALLBACK CSoundFrame::MsgWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CSoundFrame * pSndFrame = (CSoundFrame *) (LONG_PTR) GetWindowLongPtr( hwnd, GWLP_USERDATA ) ;
	if ( pSndFrame )
	{
		switch ( msg ) 
		{
		case WM_SF_STARTUP:
			pSndFrame->OnServerStartup( (HWND) wParam, (ULONG) lParam );
			return TRUE;

		case WM_SF_SHUTDOWN:
			pSndFrame->OnServerShutdown();
			return TRUE;

		case WM_COPYDATA:
			return pSndFrame->ReceiveCopyData( (HWND) wParam, (COPYDATASTRUCT *) lParam );
		}
	}

	return DefWindowProc( hwnd, msg, wParam, lParam );
}

void CSoundFrame::WriteGameObjectID(SFWriteBytesMem &in_rWriter, AkGameObjectID in_id) const
{
	//Wwise game object IDs are always 64 bits.
	AkWwiseGameObjectID idForWwise = in_id;
	if (in_id == AK_INVALID_GAME_OBJECT)
		idForWwise = WWISE_INVALID_GAME_OBJECT;

	in_rWriter.Write<AkWwiseGameObjectID>(idForWwise);	
}

WObjectType ClassToWObjectType(const char *in_szClassName)
{
	struct TypePair
	{
		char* szTypeName;
		WObjectType type;
	};

	static TypePair aTypeNames[] = 
	{
		{"class SFAction", WEventActionType },
		{"class SFArgument", WSwitchGroupType },
		{"class SFArgumentValue", WSwitchType},
		{"class SFDialogueEvent", WDialogueEventType },
		{"class SFEnvironment", WEffectPluginType },		
		{"class SFEvent", WEventType},
		{"class SFGameParameter", WGameParameterType },
		{"class SFSoundObject", WSoundType },
		{"class SFState", WStateType },
		{"class SFStateGroup", WStateGroupType },
		{"class SFSwitch", WSwitchType },
		{"class SFSwitchGroup", WSwitchGroupType },
		{"class SFTrigger", WTriggerType },
		{"", WObjectTypeUnknown}
	};

	//This function also supports the "list" variations.  
	size_t len = strlen(in_szClassName);
	const char *pList = strstr(in_szClassName, "List");
	if (pList)
		len -= 4;	//Length of "List".

	int i = 0;
	do 
	{
		if(strncmp(aTypeNames[i].szTypeName, in_szClassName, len) == 0)
			return aTypeNames[i].type;
		i++;
	} while (aTypeNames[i].type != WObjectTypeUnknown);

	return WObjectTypeUnknown;
}

bool CSoundFrame::GetWwiseObjectPath( const AK::SoundFrame::ISFObject* in_pObj, const AK::SoundFrame::ISFObject* in_pParent, LPWSTR out_szBuffer, long in_lLength ) const
{
	if ( in_pObj == NULL || !m_hwndServer || !out_szBuffer || in_lLength <= 0)
		return false;

	const type_info& info = typeid(*in_pObj);
	WObjectType wtype = ClassToWObjectType(info.name());
	if (wtype == WObjectTypeUnknown)
		return false;

	SFWriteBytesMem bytes;
	bytes.Write<AkUniqueID>( in_pObj->GetID() );	//Write the ID of the object

	//Write the ID of the parent (if needed to resolve)
	if (wtype == WSwitchType || wtype == WStateType)
	{
		if (in_pParent == NULL)
			return false;
		
		bytes.Write<AkUniqueID>( in_pParent->GetID() );
	}
	else
		bytes.Write<AkUniqueID>( 0 );

	//Write the type
	bytes.Write<WObjectType>( wtype );
	SendCopyData( SF_COPYDATA_OBJECTPATH, bytes );

	if ( m_pCopyData )
	{
		wcsncpy_s(out_szBuffer, in_lLength, (LPWSTR)m_pCopyData, in_lLength );
		out_szBuffer[in_lLength - 1] = 0;
		free(m_pCopyData);
		m_pCopyData = NULL;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------

bool AK::SoundFrame::Create( IClient * in_pClient, ISoundFrame ** out_ppSoundFrame )
{
	if ( !out_ppSoundFrame )
		return false;

	*out_ppSoundFrame = NULL;

	if ( !in_pClient )
		return false;

	CSoundFrame * pSoundFrame = new CSoundFrame( in_pClient );

	if ( pSoundFrame )
	{
		*out_ppSoundFrame = static_cast<ISoundFrame *>( pSoundFrame );
		return true;
	}
	else
	{
		return false;
	}
}
