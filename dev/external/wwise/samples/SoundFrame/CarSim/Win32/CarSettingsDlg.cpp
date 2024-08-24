//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSettingsDlg.h"
#include "Car.h"
#include "CarSimEngine.h"
#include "ICarSimUI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CarSettingsDlg::CarSettingsDlg( CarSimEngine* in_pCarSimeEngine, CWnd* pParent )
:	CDialog(CarSettingsDlg::IDD, pParent)
,	m_pCarSimEngine( in_pCarSimeEngine )
,	m_rCar( in_pCarSimeEngine->GetCar() )
,	m_bDirty( false )
{
	// Modify this dialog if you change the max num of gears ...
	_ASSERT( CarParam::s_nMaxNumOfGear == 7 );
}

void CarSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_NUM_OF_GEARS_COMBO, m_numOfGearCombo);

	DDX_Control(pDX, IDC_GEAR_1_SLIDER, m_gearSliderArray[0]);
	DDX_Control(pDX, IDC_GEAR_2_SLIDER, m_gearSliderArray[1]);
	DDX_Control(pDX, IDC_GEAR_3_SLIDER, m_gearSliderArray[2]);
	DDX_Control(pDX, IDC_GEAR_4_SLIDER, m_gearSliderArray[3]);
	DDX_Control(pDX, IDC_GEAR_5_SLIDER, m_gearSliderArray[4]);
	DDX_Control(pDX, IDC_GEAR_6_SLIDER, m_gearSliderArray[5]);
	DDX_Control(pDX, IDC_GEAR_7_SLIDER, m_gearSliderArray[6]);

	DDX_Control(pDX, IDC_GEAR_1_EDIT, m_gearEditArray[0]);
	DDX_Control(pDX, IDC_GEAR_2_EDIT, m_gearEditArray[1]);
	DDX_Control(pDX, IDC_GEAR_3_EDIT, m_gearEditArray[2]);
	DDX_Control(pDX, IDC_GEAR_4_EDIT, m_gearEditArray[3]);
	DDX_Control(pDX, IDC_GEAR_5_EDIT, m_gearEditArray[4]);
	DDX_Control(pDX, IDC_GEAR_6_EDIT, m_gearEditArray[5]);
	DDX_Control(pDX, IDC_GEAR_7_EDIT, m_gearEditArray[6]);

	DDX_Control(pDX, IDC_WEIGHT_EDIT, m_weightEdit);
	DDX_Control(pDX, IDC_TORQUE_EDIT, m_torqueEdit);
	DDX_Control(pDX, IDC_DRAG_COEFFICIENT_EDIT, m_dragCoefficientEdit);
	DDX_Control(pDX, IDC_FRONT_SURFACE_EDIT, m_frontSurfaceEdit);
	DDX_Control(pDX, IDC_ROLLING_RESISTANCE_EDIT, m_rollingResistenceEdit);
	DDX_Control(pDX, IDC_DIFFERENTIAL_RATIO_EDIT, m_differentialRatioEdit);
	DDX_Control(pDX, IDC_WHEEL_RADIUS_EDIT, m_wheelRadiusEdit);
	DDX_Control(pDX, IDC_MINIMUM_RPM_EDIT, m_minRPMEdit);
	DDX_Control(pDX, IDC_RPM_CUT_OFF_EDIT, m_rpmCutOffEdit);
}

BEGIN_MESSAGE_MAP(CarSettingsDlg, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(ID_DEFAULT_BUTTON, OnBnClickedDefault)
	ON_BN_CLICKED(ID_SAVE_BUTTON, OnBnClickedSave)
	ON_BN_CLICKED(ID_SAVE_AS_BUTTON, OnBnClickedSaveAs)
	ON_CBN_SELCHANGE(IDC_NUM_OF_GEARS_COMBO, OnCbnSelchangeNumOfGearCombo)
	ON_EN_KILLFOCUS(IDC_GEAR_1_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_2_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_3_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_4_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_5_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_6_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_GEAR_7_EDIT, OnKillFocusGearRatioEdit)
	ON_EN_KILLFOCUS(IDC_WEIGHT_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_TORQUE_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_DRAG_COEFFICIENT_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_FRONT_SURFACE_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_ROLLING_RESISTANCE_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_DIFFERENTIAL_RATIO_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_WHEEL_RADIUS_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_MINIMUM_RPM_EDIT, OnKillFocusSettingsEdit)
	ON_EN_KILLFOCUS(IDC_RPM_CUT_OFF_EDIT, OnKillFocusSettingsEdit)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CarSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	for( int i = 0; i < CarParam::s_nMaxNumOfGear; ++i )
	{
		m_gearSliderArray[i].SetRange( (int)( CarParam::s_dblMinGearRatio * 100.0 ), (int)( CarParam::s_dblMaxGearRatio * 100.0 ), TRUE );
		m_gearEditArray[i].Init( false, 2, CarParam::s_dblMinGearRatio, CarParam::s_dblMaxGearRatio );
	}

	m_weightEdit.Init( false, 1, CarParam::s_dblMinWeight, CarParam::s_dblMaxWeight );
	m_torqueEdit.Init( false, 1, CarParam::s_dblMinTorque, CarParam::s_dblMaxTorque );
	m_dragCoefficientEdit.Init( false, 2, CarParam::s_dblMinDragCoefficient, CarParam::s_dblMaxDragCoefficient );
	m_frontSurfaceEdit.Init( false, 2, CarParam::s_dblMinFrontSurface, CarParam::s_dblMaxFrontSurface );
	m_rollingResistenceEdit.Init( false, 3, CarParam::s_dblMinRollingResistence, CarParam::s_dblMaxRollingResistence );
	m_differentialRatioEdit.Init( false, 2, CarParam::s_dblMinDifferentialRatio, CarParam::s_dblMaxDifferentialRatio );
	m_wheelRadiusEdit.Init( false, 2, CarParam::s_dblMinWheelRadius, CarParam::s_dblMaxWheelRadius );
	m_minRPMEdit.Init( false, 0, CarParam::s_dblMinRPM, CarParam::s_dblMaxRPM );
	m_rpmCutOffEdit.Init( false, 0, CarParam::s_dblMinRPM, CarParam::s_dblMaxRPM );

	UpdateDisplay();

	return TRUE;
}
	
void CarSettingsDlg::OnOK()
{
	// Do nothing so we don't close on enter key
}

void CarSettingsDlg::OnCancel()
{
	AskForSave();
	
	DestroyWindow();
}
	
void CarSettingsDlg::OnDestroy()
{
	m_bDirty = false;
}

void CarSettingsDlg::Open()
{
	if( ! ::IsWindow( GetSafeHwnd() ) )
	{
		Create( CarSettingsDlg::IDD );
		ShowWindow( SW_SHOW );
	}
}

void CarSettingsDlg::Close()
{
	if( ::IsWindow( GetSafeHwnd() ) )
		OnCancel();
}

void CarSettingsDlg::OnSettingsFileChanged()
{
	if( ::IsWindow( GetSafeHwnd() ) )
	{
		SetDirty( false );
		UpdateDisplay();	
	}
}

void CarSettingsDlg::AskForSave()
{
	if( m_bDirty )
	{
		CString csMessage, csCaption;
		csMessage.LoadString( IDS_SAVE_SETTINGS_ON_CLOSE );
		csCaption.LoadString( IDS_MESSAGE_CAPTION );

		int result = MessageBox( csMessage, csCaption, MB_YESNO | MB_ICONQUESTION );
		if( result == IDYES )
			OnBnClickedSave();
		
		// If we are still dirty it means that the answer was no or that the save failed or was cancelled
		if( m_bDirty )
		{
			// Revert changes
			CString csFileName = m_pCarSimEngine->GetCarParamSerializer().GetCurrentFilePath();
			if( csFileName.IsEmpty() )
				OnBnClickedDefault();
			else
				 m_pCarSimEngine->GetCarParamSerializer().Load( csFileName );
		}
	}
}

void CarSettingsDlg::OnBnClickedDefault()
{
	m_rCar.SetDefaultCarParam();

	SetDirty( true );

	UpdateDisplay();
}

void CarSettingsDlg::OnBnClickedSave()
{
	CString csFileName = m_pCarSimEngine->GetCarParamSerializer().GetCurrentFilePath();
	if( csFileName.IsEmpty() )
		OnBnClickedSaveAs();
	else
	{
		if( !m_pCarSimEngine->GetCarParamSerializer().Save( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_SAVE_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
			SetDirty( false );
	}
}

void CarSettingsDlg::OnBnClickedSaveAs()
{
	CString csFilterString;
	csFilterString.Format( L"Car Settings Files (*%s)|*%s||", m_pCarSimEngine->GetCarParamSerializer().GetFileExtension(), 
																m_pCarSimEngine->GetCarParamSerializer().GetFileExtension() );

	CFileDialog fileDialog( FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, csFilterString );

	if( fileDialog.DoModal() == IDOK )
	{
		CString csFileName = fileDialog.GetPathName();
		if( !m_pCarSimEngine->GetCarParamSerializer().Save( csFileName ) )
		{
			CString csError;
			csError.Format( IDS_SAVE_ERROR, csFileName );
			MessageBox( csError, NULL, MB_ICONERROR | MB_OK );
		}
		else
		{
			SetDirty( false );
			UpdateTitleBarText();

			ICarSimUI* pParent = dynamic_cast<ICarSimUI*>( GetParent() );
			if( pParent )
				pParent->OnSimulationSettingsChanged();
		}
	}
}

void CarSettingsDlg::OnCbnSelchangeNumOfGearCombo()
{
	m_rCar.GetCarParam().m_nNumOfGear = m_numOfGearCombo.GetCurSel() + 1;

	m_rCar.SetCurrentGear( min( m_rCar.GetCurrentGear(), (size_t)m_numOfGearCombo.GetCurSel() ) );

	SetDirty( true );
	UpdateDisplay();
}

void CarSettingsDlg::OnKillFocusGearRatioEdit()
{
	for( int i = 0; i < CarParam::s_nMaxNumOfGear; ++i )
	{
		if( ! DoubleEqual( m_rCar.GetCarParam().m_gearRatios[i], m_gearEditArray[i].GetValue() ) )
		{
			m_rCar.GetCarParam().m_gearRatios[i] = m_gearEditArray[i].GetValue();

			m_gearSliderArray[i].SetPos( (int)( m_rCar.GetCarParam().m_gearRatios[i] * 100.0 ) );

			SetDirty( true );
		}
	}
}

void CarSettingsDlg::OnKillFocusSettingsEdit()
{
	if( ! DoubleEqual( m_rCar.GetCarParam().m_dblWeight, m_weightEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblWeight = m_weightEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblTorque, m_torqueEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblTorque = m_torqueEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblDragCoefficient, m_dragCoefficientEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblDragCoefficient = m_dragCoefficientEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblFrontSurface, m_frontSurfaceEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblFrontSurface = m_frontSurfaceEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblRollingResistence, m_rollingResistenceEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblRollingResistence = m_rollingResistenceEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblDifferentialRatio, m_differentialRatioEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblDifferentialRatio = m_differentialRatioEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblWheelRadius, m_wheelRadiusEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblWheelRadius = m_wheelRadiusEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblMinRPM, m_minRPMEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblMinRPM = m_minRPMEdit.GetValue();
		SetDirty( true );
	}
	else if( ! DoubleEqual( m_rCar.GetCarParam().m_dblRPMCutOff, m_rpmCutOffEdit.GetValue() ) )
	{
		m_rCar.GetCarParam().m_dblRPMCutOff = m_rpmCutOffEdit.GetValue();
		SetDirty( true );
	}
}

void CarSettingsDlg::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	for( int i = 0; i < CarParam::s_nMaxNumOfGear; ++i )
	{
		if( pScrollBar->GetSafeHwnd() == m_gearSliderArray[i].GetSafeHwnd() )
		{
			m_rCar.GetCarParam().m_gearRatios[i] = ( (double)m_gearSliderArray[i].GetPos() ) / 100.0;

			m_gearEditArray[i].SetValue( m_rCar.GetCarParam().m_gearRatios[i] );

			SetDirty( true );
		}
	}
}

void CarSettingsDlg::UpdateTitleBarText()
{
	CString csFileName = m_pCarSimEngine->GetCarParamSerializer().GetCurrentFileNameOnly();
	if( csFileName.IsEmpty() )
		csFileName.LoadString( IDS_DEFAULT_SETTINGS_FILE_NAME );

	CString csTitleBarText;
	csTitleBarText.Format( IDS_SETTINGS_TITLE_BAR_TEXT, csFileName );

	if( m_bDirty )
		csTitleBarText += L"*";

	SetWindowText( csTitleBarText );
}

void CarSettingsDlg::UpdateDisplay()
{
	UpdateTitleBarText();

	const CarParams& rCarParams = m_rCar.GetCarParam();

	m_numOfGearCombo.SetCurSel( (int)( rCarParams.m_nNumOfGear - 1 ) );

	for( unsigned int i = 0; i < CarParam::s_nMaxNumOfGear; ++i )
	{
		m_gearSliderArray[i].SetPos( (int)( rCarParams.m_gearRatios[i] * 100.0 ) );
		m_gearEditArray[i].SetValue( rCarParams.m_gearRatios[i] );

		// Disable slider and edit that are over the current num of gears
		m_gearSliderArray[i].EnableWindow( ( i < rCarParams.m_nNumOfGear )? TRUE : FALSE );	
		m_gearEditArray[i].EnableWindow( ( i < rCarParams.m_nNumOfGear )? TRUE : FALSE );	
	}

	m_weightEdit.SetValue( rCarParams.m_dblWeight );
	m_torqueEdit.SetValue( rCarParams.m_dblTorque );
	m_dragCoefficientEdit.SetValue( rCarParams.m_dblDragCoefficient );
	m_frontSurfaceEdit.SetValue( rCarParams.m_dblFrontSurface );
	m_rollingResistenceEdit.SetValue( rCarParams.m_dblRollingResistence );
	m_differentialRatioEdit.SetValue( rCarParams.m_dblDifferentialRatio );
	m_wheelRadiusEdit.SetValue( rCarParams.m_dblWheelRadius );
	m_minRPMEdit.SetValue( rCarParams.m_dblMinRPM );
	m_rpmCutOffEdit.SetValue( rCarParams.m_dblRPMCutOff );
}

void CarSettingsDlg::SetDirty( bool in_bDirty )
{
	if( m_bDirty != in_bDirty )
	{
		m_bDirty = in_bDirty;

		UpdateTitleBarText();
	}
}
