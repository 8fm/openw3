//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// CarSimDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CarSim.h"
#include "CarSimDlg.h"
#include "CarSimEngine.h"
#include "CarSimFramerateDlg.h"
#include "CarSimjoystickDlg.h"
#include <AK/AkWwiseSDKVersion.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	virtual BOOL OnInitDialog();

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csText;
	GetDlgItemText( IDC_NAME_AND_VERSION, csText );
	csText.Replace( _T("<VERSION>"), CString( AK_WWISESDK_VERSIONNAME ) );
	SetDlgItemText( IDC_NAME_AND_VERSION, csText );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CCarSimDlg dialog

CCarSimDlg::CCarSimDlg(CarSimEngine* in_pCarSimEngine, CWnd* pParent /*=NULL*/)
	: CDialog(CCarSimDlg::IDD, pParent)
	, m_pCarSimEngine( in_pCarSimEngine )
	, m_settingsDialog( in_pCarSimEngine )
	, m_bDirty( false )
{
	ASSERT( m_pCarSimEngine );
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCarSimDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_THROTTLE_SLIDER, m_throttleSlider);
	DDX_Control(pDX, IDC_DRY_VOLUME_SLIDER, m_dryVolumeSlider);
	DDX_Control(pDX, IDC_WET_VOLUME_SLIDER, m_wetVolumeSlider);
}

BEGIN_MESSAGE_MAP(CCarSimDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_COMMAND(ID_FILE_NEW, OnCommandFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnCommandFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnCommandFileSave)
	ON_COMMAND(ID_FILE_SAVEAS, OnCommandFileSaveAs)
	ON_COMMAND(ID_FILE_EXIT, OnCancel)
	ON_COMMAND(ID_SETTINGS_CARSIMASSIGNEMENTS, OnCommandAssignments)
	ON_COMMAND(ID_SETTINGS_JOYSTICK, OnCommandJoystick)
	ON_COMMAND(ID_SETTINGS_FRAMERATE, OnCommandFramerate)
	ON_COMMAND(ID_HELP_ABOUTCARSIM, OnCommandAbout)
	ON_COMMAND(ID_HELP_HELP, OnCommandHelp)
	ON_EN_KILLFOCUS(IDC_GAME_OBJECT_NAME_EDIT, OnKillFocusGameObjectName)
	ON_BN_CLICKED(IDC_PLAY_BUTTON, OnBnClickedPlay)
	ON_BN_CLICKED(IDC_STOP_BUTTON, OnBnClickedStop)
	ON_BN_CLICKED(IDC_GEAR_UP_BUTTON, OnBnClickedGearUp)
	ON_BN_CLICKED(IDC_GEAR_DOWN_BUTTON, OnBnClickedGearDown)
	ON_BN_CLICKED(IDC_ENVIRONMENT_CHECK, OnBnClickedAuxBusCheck)
	ON_BN_CLICKED(IDC_CAR_SETTINGS_BROWSE_BUTTON, OnBnClickedBrowseCarSettings)
	ON_BN_CLICKED(IDC_CAR_SETTINGS_EDIT_BUTTON, OnBnClickedEditCarSettings)
	ON_WM_HSCROLL()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()


// CCarSimDlg message handlers

BOOL CCarSimDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_assignmentDialog.Init( m_pCarSimEngine->GetSoundFrame() );

	m_pCarSimEngine->GetJoystickMgr().Init( GetSafeHwnd() );

	m_throttleSlider.SetRange( -100, 100, TRUE );
	m_throttleSlider.SetPos( 0 );

	m_dryVolumeSlider.SetRange( 0, 100, TRUE );

	m_wetVolumeSlider.SetRange( 0, 100, TRUE );

	OnThrottleChange();

	OnSimulationSettingsChanged();

	return FALSE;
}

void CCarSimDlg::OnOK()
{
	// Do nothing so we don't close on enter key
}

void CCarSimDlg::OnCancel()
{
	m_settingsDialog.Close();

	if( ! AskForSave() )
		return;
	
	CDialog::OnCancel();
}

void CCarSimDlg::OnDestroy()
{
	m_pCarSimEngine->GetJoystickMgr().Term();
}

void CCarSimDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CCarSimDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCarSimDlg::OnSimulationSettingsChanged()
{
	UpdateTitleBarText();

	CString csStatus;
	csStatus.LoadString( m_pCarSimEngine->GetSoundFrame()->IsConnected()? IDS_STATUS_CONNECTED : IDS_STATUS_DISCONNECTED );
	SetDlgItemText( IDC_CONNECTION_STATUS_STATIC, csStatus );

	SetDlgItemText( IDC_GAME_OBJECT_NAME_EDIT, m_pCarSimEngine->GetGameObjectName() );

	CString csFileName = m_pCarSimEngine->GetCarParamSerializer().GetCurrentFileNameOnly();
	if( csFileName.IsEmpty() )
		csFileName.LoadString( IDS_DEFAULT_SETTINGS_FILE_NAME );

	CString csCurrentFileName;
	GetDlgItemText( IDC_CAR_SETTINGS_EDIT, csCurrentFileName );
	if( ! csCurrentFileName.IsEmpty() && csCurrentFileName != csFileName )
		SetDirty( true );

	SetDlgItemText( IDC_CAR_SETTINGS_EDIT, csFileName );

	double dblDryVolume = 0.0;
	double dblWetVolume = 0.0;
	bool bUseAuxBus = m_pCarSimEngine->GetAuxBusSettings( dblDryVolume, dblWetVolume );

	CheckDlgButton( IDC_ENVIRONMENT_CHECK, bUseAuxBus? BST_CHECKED : BST_UNCHECKED );

	m_dryVolumeSlider.SetPos( (int)( dblDryVolume * 100.0 ) );
	m_wetVolumeSlider.SetPos( (int)( dblWetVolume * 100.0 ) );

	OnAuxBusChange();
	UpdateFrame();

	m_assignmentDialog.OnSimulationSettingsChanged();
}
	
void CCarSimDlg::OnNotif( ObjectType in_eObjectType, IClient::Notif in_eNotif, AkUniqueID in_objectID )
{
	m_assignmentDialog.OnNotif( in_eObjectType, in_eNotif, in_objectID );
}

void CCarSimDlg::UpdateFrame()
{
	CString csTemp;
	const Car& rCar = m_pCarSimEngine->GetCar();

	m_throttleSlider.SetPos( (int)(rCar.GetThrottle() * 100.0) );

	csTemp.Format( L"%.2f", rCar.GetThrottle() );
	SetDlgItemText( IDC_THROTTLE_EDIT, csTemp );

	csTemp.Format( L"%d", rCar.GetCurrentGear() + 1 );
	SetDlgItemText( IDC_CURRENT_GEAR_STATIC, csTemp );

	csTemp.Format( L"%.0f", rCar.GetRPM() );
	SetDlgItemText( IDC_CURRENT_RPM_STATIC, csTemp );

	csTemp.Format( L"%.2f", rCar.GetLoad() );
	SetDlgItemText( IDC_CURRENT_LOAD_STATIC, csTemp );

	// Multiply by 3.6 to convert to m/s to Km/h
	csTemp.Format( L"%.2f", rCar.GetSpeed() * 3.6 );
	SetDlgItemText( IDC_CURRENT_SPEED_STATIC, csTemp );

	double dblDryVolume = 0.0;
	double dblWetVolume = 0.0;
	if( m_pCarSimEngine->GetAuxBusSettings( dblDryVolume, dblWetVolume ) )
	{
		
		csTemp.Format( L"%.2f", dblWetVolume );
		SetDlgItemText( IDC_WET_VALUE_STATIC, csTemp );

		m_wetVolumeSlider.SetPos( (int)( dblWetVolume * 100.0 ) );
	}
}

void CCarSimDlg::OnCommandFileNew()
{
	m_settingsDialog.AskForSave();

	if( ! AskForSave() )
		return;

	m_pCarSimEngine->ResetToDefault();

	m_settingsDialog.OnSettingsFileChanged();

	SetDirty( false );
}

void CCarSimDlg::OnCommandFileOpen()
{
	m_settingsDialog.AskForSave();

	if( ! AskForSave() )
		return;

	CString csFilterString;
	csFilterString.Format( L"Car Sim Files (*%s)|*%s||", m_pCarSimEngine->GetSimulationSerializer().GetFileExtension(), 
															m_pCarSimEngine->GetSimulationSerializer().GetFileExtension() );

	CFileDialog fileDialog( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, csFilterString );

	if( fileDialog.DoModal() == IDOK )
	{
		CString csFileName = fileDialog.GetPathName();
		if( !m_pCarSimEngine->GetSimulationSerializer().Load( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_OPEN_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
		{
			m_pCarSimEngine->GetCar().ResetSimulationData();
			OnSimulationSettingsChanged();

			m_settingsDialog.OnSettingsFileChanged();

			SetDirty( false );
		}
	}
}

void CCarSimDlg::OnCommandFileSave()
{
	CString csFileName = m_pCarSimEngine->GetSimulationSerializer().GetCurrentFilePath();
	if( csFileName.IsEmpty() )
		OnCommandFileSaveAs();
	else
	{
		if( !m_pCarSimEngine->GetSimulationSerializer().Save( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_SAVE_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
		{
			SetDirty( false );
		}
	}
}

void CCarSimDlg::OnCommandFileSaveAs()
{
	CString csFilterString;
	csFilterString.Format( L"Car Sim Files (*%s)|*%s||", m_pCarSimEngine->GetSimulationSerializer().GetFileExtension(), 
															m_pCarSimEngine->GetSimulationSerializer().GetFileExtension() );

	CFileDialog fileDialog( FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, csFilterString );

	if( fileDialog.DoModal() == IDOK )
	{
		CString csFileName = fileDialog.GetPathName();
		if( !m_pCarSimEngine->GetSimulationSerializer().Save( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_SAVE_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
		{
			SetDirty( false );
			UpdateTitleBarText();
		}
	}
}

void CCarSimDlg::OnCommandAssignments()
{
	m_assignmentDialog.SetAssignment( m_pCarSimEngine->GetAssignement() );

	if( m_assignmentDialog.DoModal() == IDOK )
	{
		m_pCarSimEngine->SetAssignement( m_assignmentDialog.GetAssignment() );
		SetDirty( true );
	}
}

void CCarSimDlg::OnCommandJoystick()
{
	CarSimJoystickDlg dialog( &( m_pCarSimEngine->GetJoystickMgr() ) );

	dialog.DoModal();
}

void CCarSimDlg::OnCommandFramerate()
{
	_ASSERT( m_pCarSimEngine->GetFrameInterval() > 0 );

	CarSimFramerateDlg dialog;
	// Transform time interval in frame per second
	dialog.SetFramerate( 1000 / m_pCarSimEngine->GetFrameInterval() );

	if( dialog.DoModal() == IDOK )
	{
		// Transform frame per second in time interval in ms
		m_pCarSimEngine->SetFrameInterval( 1000 / dialog.GetFramerate() );
		SetDirty( true );
	}
}

void CCarSimDlg::OnCommandAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CCarSimDlg::OnCommandHelp()
{
	AFX_MANAGE_STATE( ::AfxGetAppModuleState() ) ;

	::AfxGetApp()->HtmlHelp( 0, 0 );
}

void CCarSimDlg::OnKillFocusGameObjectName()
{
	CString csGameObjectName;
	GetDlgItemText( IDC_GAME_OBJECT_NAME_EDIT, csGameObjectName );
	csGameObjectName.Trim();

	if( csGameObjectName != m_pCarSimEngine->GetGameObjectName() )
	{
		m_pCarSimEngine->ChangeGameObject( csGameObjectName );
		SetDirty( true );
	}
}

void CCarSimDlg::OnBnClickedPlay()
{
	m_pCarSimEngine->Play();
}

void CCarSimDlg::OnBnClickedStop()
{
	m_pCarSimEngine->Stop();
}

void CCarSimDlg::OnBnClickedGearUp()
{
	m_pCarSimEngine->GearUp();
}

void CCarSimDlg::OnBnClickedGearDown()
{
	m_pCarSimEngine->GearDown();
}

void CCarSimDlg::OnBnClickedBrowseCarSettings()
{
	m_settingsDialog.AskForSave();

	CString csFilterString;
	csFilterString.Format( L"Car Settings Files (*%s)|*%s||", m_pCarSimEngine->GetCarParamSerializer().GetFileExtension(), 
															m_pCarSimEngine->GetCarParamSerializer().GetFileExtension() );

	CFileDialog fileDialog( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, csFilterString );

	if( fileDialog.DoModal() == IDOK )
	{
		CString csFileName = fileDialog.GetPathName();
		if( !m_pCarSimEngine->GetCarParamSerializer().Load( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_OPEN_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
		{
			OnSimulationSettingsChanged();

			m_settingsDialog.OnSettingsFileChanged();
		}
	}
}

void CCarSimDlg::OnBnClickedEditCarSettings()
{
	m_settingsDialog.Open();
}

void CCarSimDlg::OnBnClickedAuxBusCheck()
{
	OnAuxBusChange();
	SetDirty( true );
}

void CCarSimDlg::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	if( pScrollBar->GetSafeHwnd() == m_throttleSlider.GetSafeHwnd() )
	{
		OnThrottleChange();
	}
	else if( pScrollBar->GetSafeHwnd() == m_dryVolumeSlider.GetSafeHwnd()
				|| pScrollBar->GetSafeHwnd() == m_wetVolumeSlider.GetSafeHwnd() )
	{
		OnAuxBusChange();
	}
}

void CCarSimDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

    if( WA_INACTIVE != nState )
    {
        m_pCarSimEngine->GetJoystickMgr().OnActivate();
    }
}

void CCarSimDlg::OnAuxBusChange()
{
	bool bUseAuxBus = IsDlgButtonChecked( IDC_ENVIRONMENT_CHECK ) == BST_CHECKED;

	double dblDryVolume = (double)m_dryVolumeSlider.GetPos() / 100.0;
	double dblWetVolume = (double)m_wetVolumeSlider.GetPos() / 100.0;

	CString csText;
	csText.Format( L"%.2f", dblDryVolume );
	SetDlgItemText( IDC_DRY_VALUE_STATIC, csText );

	csText.Format( L"%.2f", dblWetVolume );
	SetDlgItemText( IDC_WET_VALUE_STATIC, csText );

	m_dryVolumeSlider.EnableWindow( bUseAuxBus? TRUE : FALSE );
	m_wetVolumeSlider.EnableWindow( bUseAuxBus? TRUE : FALSE );

	m_pCarSimEngine->SetAuxBusSettings( bUseAuxBus, dblDryVolume, dblWetVolume );
}

void CCarSimDlg::OnThrottleChange()
{
	double dblThrottle = (double)m_throttleSlider.GetPos() / 100.0;

	CString csText;
	csText.Format( L"%.2f", dblThrottle );
	SetDlgItemText( IDC_THROTTLE_EDIT, csText );

	m_pCarSimEngine->GetCar().SetThrottle( dblThrottle );
}

void CCarSimDlg::UpdateTitleBarText()
{
	CString csFileName = m_pCarSimEngine->GetSimulationSerializer().GetCurrentFileNameOnly();
	if( csFileName.IsEmpty() )
		csFileName.LoadString( IDS_DEFAULT_FILE_NAME );

	CString csTitleBarText;
	csTitleBarText.Format( IDS_TITLE_BAR_TEXT, csFileName );

	if( m_bDirty )
		csTitleBarText += L"*";

	SetWindowText( csTitleBarText );
}

void CCarSimDlg::SetDirty( bool in_bDirty )
{
	if( m_bDirty != in_bDirty )
	{
		m_bDirty = in_bDirty;

		UpdateTitleBarText();
	}
}

bool CCarSimDlg::AskForSave()
{
	if( m_bDirty )
	{
		CString csMessage, csCaption;
		csMessage.LoadString( IDS_SAVE_SIMULATION_ON_CLOSE );
		csCaption.LoadString( IDS_MESSAGE_CAPTION );

		int result = MessageBox( csMessage, csCaption, MB_YESNOCANCEL | MB_ICONQUESTION );
		if( result == IDCANCEL )
			return false;
		else if( result == IDYES )
			OnCommandFileSave();
	}

	return true;
}