//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimAssignmentDlg.h"
#include "CarSimHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CarSimAssignmentDlg::CarSimAssignmentDlg( CWnd* pParent )
: CDialog(CarSimAssignmentDlg::IDD, pParent)
, m_pSoundFrame( NULL )
{
}

void CarSimAssignmentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_RPM_GAME_PARAM_COMBO, m_rpmCombo);
	DDX_Control(pDX, IDC_LOAD_GAME_PARAM_COMBO, m_loadCombo);
	DDX_Control(pDX, IDC_SPEED_GAME_PARAM_COMBO, m_speedCombo);
	DDX_Control(pDX, IDC_ENGINE_PLAY_EVENT_COMBO, m_eventPlayCombo);
	DDX_Control(pDX, IDC_ENGINE_STOP_EVENT_COMBO, m_eventStopCombo);
	DDX_Control(pDX, IDC_UP_SHIFT_EVENT_COMBO, m_eventUpShiftCombo);
	DDX_Control(pDX, IDC_DOWN_SHIFT_EVENT_COMBO, m_eventDownShiftCombo);
	DDX_Control(pDX, IDC_ENVIRONMENT_COMBO, m_AuxBusCombo);
}

BEGIN_MESSAGE_MAP(CarSimAssignmentDlg, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CarSimAssignmentDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	OnSimulationSettingsChanged();

	return TRUE;
}
	
void CarSimAssignmentDlg::OnOK()
{
	m_assignment.Clear();

	m_assignment.m_RPMParam = m_rpmCombo.GetSelectedObject();
	m_assignment.m_LoadParam = m_loadCombo.GetSelectedObject();
	m_assignment.m_SpeedParam = m_speedCombo.GetSelectedObject();
	m_assignment.m_EnginePlayEvent = m_eventPlayCombo.GetSelectedObject();
	m_assignment.m_EngineStopEvent = m_eventStopCombo.GetSelectedObject();
	m_assignment.m_UpShiftEvent = m_eventUpShiftCombo.GetSelectedObject();
	m_assignment.m_DownShiftEvent = m_eventDownShiftCombo.GetSelectedObject();
	m_assignment.m_AuxBus = m_AuxBusCombo.GetSelectedObject();

	CDialog::OnOK();
}
	
void CarSimAssignmentDlg::OnDestroy()
{
	m_rpmCombo.ClearCombo();
	m_loadCombo.ClearCombo();
	m_speedCombo.ClearCombo();
	m_eventPlayCombo.ClearCombo();
	m_eventStopCombo.ClearCombo();
	m_eventUpShiftCombo.ClearCombo();
	m_eventDownShiftCombo.ClearCombo();
	m_AuxBusCombo.ClearCombo();
}

void CarSimAssignmentDlg::OnSimulationSettingsChanged()
{
	if( !::IsWindow( GetSafeHwnd() ) )
		return;

	if( m_pSoundFrame && m_pSoundFrame->IsConnected() && m_pSoundFrame->GetCurrentProjectID() != GUID_NULL )
	{
		IGameParameterList* pGameParameterList = NULL;
		m_pSoundFrame->GetGameParameterList( &pGameParameterList );

		m_rpmCombo.FillFromList( pGameParameterList, m_assignment.m_RPMParam );
		m_loadCombo.FillFromList( pGameParameterList, m_assignment.m_LoadParam );
		m_speedCombo.FillFromList( pGameParameterList, m_assignment.m_SpeedParam );

		IEventList* pEventList = NULL;
		m_pSoundFrame->GetEventList( &pEventList );

		m_eventPlayCombo.FillFromList( pEventList, m_assignment.m_EnginePlayEvent );
		m_eventStopCombo.FillFromList( pEventList, m_assignment.m_EngineStopEvent );
		m_eventUpShiftCombo.FillFromList( pEventList, m_assignment.m_UpShiftEvent );
		m_eventDownShiftCombo.FillFromList( pEventList, m_assignment.m_DownShiftEvent );

		IAuxBusList* pAuxBusList = NULL;
		m_pSoundFrame->GetAuxBusList( &pAuxBusList );

		m_AuxBusCombo.FillFromList( pAuxBusList, m_assignment.m_AuxBus );

		ReleaseClear( pGameParameterList );
		ReleaseClear( pEventList );
		ReleaseClear( pAuxBusList );
	}
	else
	{
		m_rpmCombo.AddObjectAndDisable( m_assignment.m_RPMParam );
		m_loadCombo.AddObjectAndDisable( m_assignment.m_LoadParam );
		m_speedCombo.AddObjectAndDisable( m_assignment.m_SpeedParam );
		m_eventPlayCombo.AddObjectAndDisable( m_assignment.m_EnginePlayEvent );
		m_eventStopCombo.AddObjectAndDisable( m_assignment.m_EngineStopEvent );
		m_eventUpShiftCombo.AddObjectAndDisable( m_assignment.m_UpShiftEvent );
		m_eventDownShiftCombo.AddObjectAndDisable( m_assignment.m_DownShiftEvent );
		m_AuxBusCombo.AddObjectAndDisable( m_assignment.m_AuxBus );
	}
}

void CarSimAssignmentDlg::OnNotif( ObjectType in_eObjectType, IClient::Notif in_eNotif, AkUniqueID in_objectID )
{
	if( !::IsWindow( GetSafeHwnd() ) )
		return;

	if( in_eNotif == IClient::Notif_Removed )
	{
		m_rpmCombo.RemoveObject( in_objectID );
		m_loadCombo.RemoveObject( in_objectID );
		m_speedCombo.RemoveObject( in_objectID );
		m_eventPlayCombo.RemoveObject( in_objectID );
		m_eventStopCombo.RemoveObject( in_objectID );
		m_eventUpShiftCombo.RemoveObject( in_objectID );
		m_eventDownShiftCombo.RemoveObject( in_objectID );
		m_AuxBusCombo.RemoveObject( in_objectID );
	}
	else if( in_eNotif == IClient::Notif_Added || in_eNotif == IClient::Notif_Changed )
	{
		OnSimulationSettingsChanged();
	}
}

void CarSimAssignmentDlg::UpdateFrame()
{
	// Nothing To do ...
}

void CarSimAssignmentDlg::Init( const ISoundFrame * in_pSoundFrame )
{
	m_pSoundFrame = in_pSoundFrame;
}

void CarSimAssignmentDlg::SetAssignment( const CarSimAssignment& in_rAssignment )
{
	m_assignment.Assign( in_rAssignment );
}

const CarSimAssignment& CarSimAssignmentDlg::GetAssignment() const
{
	return m_assignment;
}