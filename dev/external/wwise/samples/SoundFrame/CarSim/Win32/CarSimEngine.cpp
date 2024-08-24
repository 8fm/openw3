//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "CarSimEngine.h"
#include "ICarSimUI.h"
#include "CarSimHelpers.h"

#include <math.h>

namespace
{
	static const ULONG FRAME_TIMER_ID = 1;
}

CarSimEngine::CarSimEngine()
	: m_pSoundFrame( NULL )
	, m_pCarmSimUI( NULL )
	, m_timerInterval( 17 )	// 17 ms ~ 60 fps
	, m_dwLastFrameTime( 0 )
	, m_gameObjectID( IGameObject::s_InvalidGameObject )
	, m_bUseAuxBus( false )
	, m_dblDryVolume( 1.0 )
	, m_dblWetVolume( 0.0 )
{
	m_csGameObjectName.LoadString( IDS_CAR_SIM_DEFAULT_GAME_OBJECT );
}
	
CarSimEngine::~CarSimEngine()
{
	ASSERT( m_pSoundFrame == NULL );
}

BEGIN_MESSAGE_MAP(CarSimEngine, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CarSimEngine::OnConnect( bool in_bConnect )
{
	if( in_bConnect )
	{
		CreateAndUpdateGameObject();
		SetTimer( FRAME_TIMER_ID, m_timerInterval, NULL );
	}
	else
	{
		KillTimer( FRAME_TIMER_ID );
	}

	if( m_pCarmSimUI )
		m_pCarmSimUI->OnSimulationSettingsChanged();
}
	
void CarSimEngine::OnEventNotif( Notif in_eNotif, AkUniqueID in_eventID )
{
	if( in_eNotif == Notif_Removed )
		m_assignment.RemoveObject( in_eventID );

	if( m_pCarmSimUI )
		m_pCarmSimUI->OnNotif( ICarSimUI::Type_Event, in_eNotif, in_eventID );
}

void CarSimEngine::OnDialogueEventNotif( Notif in_eNotif, AkUniqueID in_dialogueEventID )
{
	// Nothing to do ...
}

void CarSimEngine::OnSoundObjectNotif( Notif in_eNotif, AkUniqueID in_soundObjectID )
{
	// Nothing to do ...
}
	
void CarSimEngine::OnStatesNotif( Notif in_eNotif, AkUniqueID in_stateGroupID )
{
	// Nothing to do ...
}
	
void CarSimEngine::OnSwitchesNotif( Notif in_eNotif, AkUniqueID in_switchGroupID )
{
	// Nothing to do ...
}
	
void CarSimEngine::OnGameObjectsNotif( Notif in_eNotif, AkGameObjectID in_gameObjectID )
{
	// Can happen if Wwise is connected to remote game and the game unregister an object with the 
	// same ID as our. Or if another instance of the Car Sim randomly have the same Game object ID.
	if( in_eNotif == Notif_Removed && in_gameObjectID == m_gameObjectID )
		CreateAndUpdateGameObject( true );
}
	
void CarSimEngine::OnGameParametersNotif( Notif in_eNotif, AkUniqueID in_gameParameterID )
{
	if( in_eNotif == Notif_Removed )
		m_assignment.RemoveObject( in_gameParameterID );

	if( m_pCarmSimUI )
		m_pCarmSimUI->OnNotif( ICarSimUI::Type_GameParameter, in_eNotif, in_gameParameterID );
}
	
void CarSimEngine::OnTriggersNotif( Notif in_eNotif, AkUniqueID in_triggerID )
{
	// Nothing to do ...
}

void CarSimEngine::OnArgumentsNotif( Notif in_eNotif, AkUniqueID in_argumentID )
{
	// Nothing to do ...
}
	
void CarSimEngine::OnAuxBusNotif( Notif in_eNotif, AkUniqueID in_AuxBusID )
{
	if( in_eNotif == Notif_Removed )
		m_assignment.RemoveObject( in_AuxBusID );

	if( m_pCarmSimUI )
		m_pCarmSimUI->OnNotif( ICarSimUI::Type_AuxBus, in_eNotif, in_AuxBusID );
}

void CarSimEngine::OnButtonGearUp()
{
	GearUp();
}

void CarSimEngine::OnButtonGearDown()
{
	GearDown();
}
	
void CarSimEngine::OnAxisThrottle( const double & in_rValue )
{
	m_car.SetThrottle( in_rValue );
}

void CarSimEngine::OnAxisAuxBusWet( const double & in_rValue )
{
	if( in_rValue >= 0.0 )
	{
		m_dblWetVolume = in_rValue;
		OnAuxBusChanged();
	}
}

void CarSimEngine::Init( ICarSimUI* in_pCarmSimUI )
{
	CreateEx( 0, _T( "STATIC" ), NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL );

	m_pCarmSimUI = in_pCarmSimUI;

	if( m_pSoundFrame )
		m_pSoundFrame->Release();

	if ( SoundFrame::Create( this, &m_pSoundFrame ) )
		m_pSoundFrame->Connect();

	m_simulationSerializer.Init( this );
	m_carParamSerializer.Init( this );
}

void CarSimEngine::Term()
{
	KillTimer( FRAME_TIMER_ID );

	m_assignment.Clear();
	
	ReleaseClear( m_pSoundFrame );

	DestroyWindow();
}

void CarSimEngine::ResetToDefault()
{
	CString csObjectName;
	csObjectName.LoadString( IDS_CAR_SIM_DEFAULT_GAME_OBJECT );

	m_assignment.Clear();

	m_bUseAuxBus = false;
	m_dblDryVolume = 1.0;
	m_dblWetVolume = 0.0;

	m_simulationSerializer.ClearFileName();
	m_carParamSerializer.ClearFileName();

	m_car.ResetSimulationData();
	m_car.SetDefaultCarParam();

	ChangeGameObject( csObjectName );

	if( m_pCarmSimUI )
		m_pCarmSimUI->OnSimulationSettingsChanged();
}
	
const ISoundFrame* CarSimEngine::GetSoundFrame() const
{
	return m_pSoundFrame;
}
	
const Car& CarSimEngine::GetCar() const
{
	return m_car;
}

Car& CarSimEngine::GetCar()
{
	return m_car;
}

CarSimSimulationSerializer& CarSimEngine::GetSimulationSerializer()
{
	return m_simulationSerializer;
}

CarParamSerializer& CarSimEngine::GetCarParamSerializer()
{
	return m_carParamSerializer;
}

CarSimJoystickMgr& CarSimEngine::GetJoystickMgr()
{
	return m_joystickMgr;
}

void CarSimEngine::SetFrameInterval( ULONG in_ulInterval )
{
	// Make sure we don't have a time intervale of 0.
	m_timerInterval = max( 1, in_ulInterval );

	if( m_pSoundFrame && m_pSoundFrame->IsConnected() )
	{
		KillTimer( FRAME_TIMER_ID );
		SetTimer( FRAME_TIMER_ID, m_timerInterval, NULL );
	}
}

const CString& CarSimEngine::GetGameObjectName() const
{
	return m_csGameObjectName;
}
	
void CarSimEngine::ChangeGameObject( const CString& in_rNewGameObjectName )
{
	if( in_rNewGameObjectName != m_csGameObjectName )
	{
		Stop();
		m_csGameObjectName = in_rNewGameObjectName;

		if( m_pSoundFrame && m_pSoundFrame->IsConnected() )
			CreateAndUpdateGameObject( true );
	}
}

const CarSimAssignment& CarSimEngine::GetAssignement() const
{
	return m_assignment;
}

void CarSimEngine::SetAssignement( const CarSimAssignment& in_rAssignment )
{
	m_assignment.Assign( in_rAssignment );

	OnSpeedChanged();
	OnRPMChanged();
	OnLoadChanged();
	OnAuxBusChanged();
}

bool CarSimEngine::GetAuxBusSettings( double& out_rDryVolume, double& out_rWetVolume ) const
{
	out_rDryVolume = m_dblDryVolume;
	out_rWetVolume = m_dblWetVolume;

	return m_bUseAuxBus;
}

void CarSimEngine::SetAuxBusSettings( bool in_bUseAuxBus, const double& in_rDryVolume, const double& in_rWetVolume )
{
	m_bUseAuxBus = in_bUseAuxBus;
	m_dblDryVolume = in_rDryVolume;
	m_dblWetVolume = in_rWetVolume;

	OnAuxBusChanged();
}

ULONG CarSimEngine::GetFrameInterval() const
{
	return m_timerInterval;
}

void CarSimEngine::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == FRAME_TIMER_ID )
		OnFrame();
}

void CarSimEngine::Play()
{
	if( m_pSoundFrame && m_assignment.m_EnginePlayEvent != AK_INVALID_UNIQUE_ID )
	{
		m_pSoundFrame->PlayEvents( &m_assignment.m_EnginePlayEvent, 1, m_gameObjectID );
	}
}

void CarSimEngine::Stop()
{
	if( m_pSoundFrame && m_assignment.m_EngineStopEvent != AK_INVALID_UNIQUE_ID )
	{
		m_pSoundFrame->PlayEvents( &m_assignment.m_EngineStopEvent, 1, m_gameObjectID );
	}
}

void CarSimEngine::GearUp()
{
	if ( m_car.GetCurrentGear() < m_car.GetCarParam().m_nNumOfGear - 1 )
	{
		if( m_pSoundFrame && m_assignment.m_UpShiftEvent != AK_INVALID_UNIQUE_ID )
		{
			m_pSoundFrame->PlayEvents( &m_assignment.m_UpShiftEvent, 1, m_gameObjectID );
		}

		// Shift gear
		m_car.SetCurrentGear( m_car.GetCurrentGear() + 1 );

		OnGearChanged();
	}
}

void CarSimEngine::GearDown()
{
	if ( m_car.GetCurrentGear() > 0 )
	{
		if ( m_pSoundFrame && m_assignment.m_DownShiftEvent != AK_INVALID_UNIQUE_ID )
		{
			m_pSoundFrame->PlayEvents( &m_assignment.m_DownShiftEvent, 1, m_gameObjectID );
		}

		// Shift gear
		m_car.SetCurrentGear( m_car.GetCurrentGear() - 1 );

		OnGearChanged();
	}
}

void CarSimEngine::OnFrame()
{
	m_joystickMgr.Poll( this );
	
	// Positive forces
	const double dblEngine = m_car.GetCarEngineForce();

	// Negative forces
	const double dblDrag = m_car.GetDragForce();
	const double dblFrictionForce = m_car.GetFrictionForce();

	const double dblAcceleration = ( dblEngine + dblDrag + dblFrictionForce ) / m_car.GetCarParam().m_dblWeight;

	if ( fabs(dblAcceleration) > 0 )
	{
		DWORD dwCurrentFrameTime = ::GetTickCount();
		DWORD dwTimeDifference = ( m_dwLastFrameTime != 0 )? dwCurrentFrameTime - m_dwLastFrameTime : m_timerInterval;

		m_dwLastFrameTime = dwCurrentFrameTime;

		m_car.SetSpeed( m_car.GetSpeed() + ( dblAcceleration * dwTimeDifference / 1000 ) );
		if ( m_car.GetSpeed() < 0 )
			m_car.SetSpeed( 0.0 );

		OnSpeedChanged();

		OnRPMChanged();
	}

	const double dblLoad = min( 1, max( -1, dblAcceleration / 10 ) );
	if ( m_car.GetLoad() != dblLoad )
	{
		m_car.SetLoad( dblLoad );
		OnLoadChanged();
	}

	if( m_pCarmSimUI )
		m_pCarmSimUI->UpdateFrame();
}

void CarSimEngine::CreateAndUpdateGameObject( bool in_bNewID /*= false*/ )
{
	if( ! m_pSoundFrame )
		return;

	AkGameObjectID oldID = m_gameObjectID;
	if( in_bNewID || m_gameObjectID == IGameObject::s_InvalidGameObject )
	{
		// Generate a random number for the game object ID.
		::srand( (unsigned)time( NULL ) );
		m_gameObjectID = rand();
	}

	// Make sure we have a new game object
	m_pSoundFrame->UnregisterGameObject( oldID );
	m_pSoundFrame->RegisterGameObject( m_gameObjectID, m_csGameObjectName );

	OnSpeedChanged();
	OnRPMChanged();
	OnLoadChanged();
	OnAuxBusChanged();
}

void CarSimEngine::OnGearChanged()
{
	if( m_pCarmSimUI )
		m_pCarmSimUI->UpdateFrame();
}

void CarSimEngine::OnSpeedChanged()
{
	if( m_pSoundFrame && m_assignment.m_SpeedParam )
		m_pSoundFrame->SetRTPCValue( m_assignment.m_SpeedParam, (AkRtpcValue)m_car.GetSpeed(), m_gameObjectID );
}

void CarSimEngine::OnRPMChanged()
{
	if( m_pSoundFrame && m_assignment.m_RPMParam )
		m_pSoundFrame->SetRTPCValue( m_assignment.m_RPMParam, (AkRtpcValue)m_car.GetRPM(), m_gameObjectID );
}

void CarSimEngine::OnLoadChanged()
{
	if( m_pSoundFrame && m_assignment.m_LoadParam )
		m_pSoundFrame->SetRTPCValue( m_assignment.m_LoadParam, (AkRtpcValue)m_car.GetLoad(), m_gameObjectID );
}

void CarSimEngine::OnAuxBusChanged()
{
	if( m_pSoundFrame )
	{
		if( m_bUseAuxBus && m_assignment.m_AuxBus )
		{
			AkAuxSendValue AuxBusValue = {0};
			AuxBusValue.auxBusID = m_assignment.m_AuxBus;
			AuxBusValue.fControlValue = (AkReal32)m_dblWetVolume;

			m_pSoundFrame->SetGameObjectAuxSendValues( m_gameObjectID, &AuxBusValue, 1 );
			m_pSoundFrame->SetGameObjectOutputBusVolume( m_gameObjectID, (AkReal32)m_dblDryVolume );
		}
		else
		{
			m_pSoundFrame->SetGameObjectAuxSendValues( m_gameObjectID, NULL, 0 );
			m_pSoundFrame->SetGameObjectOutputBusVolume( m_gameObjectID, 1.0f );
		}
	}
}