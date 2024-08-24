//////////////////////////////////////////////////////////////////////
//
// ToneGenPlugin.cpp
//
// Tone generator Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "ToneGenPlugin.h"
#include "TopicAlias.h"
#include <AK\Plugin\AkToneSourceFactory.h>
#include <AK/Tools/Common/AkAssert.h>

using namespace AK;
using namespace Wwise;

// Bind non static text UI controls to properties for property view
AK_BEGIN_POPULATE_TABLE(ToneGenProp)
	AK_POP_ITEM(IDC_CHECK_SWEEPFREQ, szSweepFreq, L"")
	AK_POP_ITEM(IDC_RADIO_FREQSWEEPLIN, szSweepFreqType, L"")
	AK_POP_ITEM(IDC_RADIO_FIXLENGTH, szDurMode, L"")
AK_END_POPULATE_TABLE()

// Table of display name resources ( one for each property )
struct DisplayNameInfo
{
	LPCWSTR wszPropName;
	UINT    uiDisplayName;
};

static DisplayNameInfo g_DisplayNames[] = 
{
	{ szStartFreq, IDS_STARTFREQ },
	{ szStartFreqRandMin, IDS_STARTFREQRANDMIN },
	{ szStartFreqRandMax, IDS_STARTFREQRANDMAX },
	{ szStopFreq, IDS_STOPFREQ },
	{ szStopFreqRandMin, IDS_STOPFREQRANDMIN },
	{ szStopFreqRandMax, IDS_STOPFREQRANDMAX },
	{ szSweepFreq, IDS_SWEEPFREQ },
	{ szSweepFreqType, IDS_SWEEPFREQTYPE },
	{ szWaveType, IDS_WAVETYPE },
	{ szWaveGain, IDS_WAVEGAIN },
	{ szDurMode, IDS_DURMODE },
	{ szFixDur, IDS_FIXDUR },
	{ szAttackTime, IDS_ATTACKTIME },
	{ szDecayTime, IDS_DECAYTIME },
	{ szSustainTime, IDS_SUSTAINTIME },
	{ szSustainLevel, IDS_SUSTAINVAL },
	{ szReleaseTime, IDS_RELEASETIME },
	{ szChannelMask, IDS_CHANNELMASK },
	{ NULL, NULL }
};

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short ToneGenPlugin::CompanyID = AKCOMPANYID_AUDIOKINETIC;
const short ToneGenPlugin::PluginID = AKSOURCEID_TONE;

//If the same class is used to generate motion data, it needs a separate ID
const short ToneGenPlugin::MotionPluginID = AKSOURCEID_MOTIONTONE;

// Constructor
ToneGenPlugin::ToneGenPlugin(AkUInt16 in_idPlugin)
: m_pPSet( NULL )
, m_hwndPropView( NULL )
, m_hwndObjPane( NULL )
{
	m_idDialogBig = in_idPlugin == PluginID ? IDD_TONEGENPLUGIN_BIG : IDD_MOTIONTONEGENPLUGIN_BIG;
	m_idDialogSmall = in_idPlugin == PluginID ? IDD_TONEGENPLUGIN_SMALL : IDD_MOTIONTONEGENPLUGIN_SMALL;
}

// Destructor
ToneGenPlugin::~ToneGenPlugin()
{

}

// Implement the destruction of the Wwise source plugin.
void ToneGenPlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void ToneGenPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Take necessary action on property changes. 
// Note: user also has the option of catching appropriate message in WindowProc function.
void ToneGenPlugin::NotifyPropertyChanged( const GUID & in_guidPlatform, LPCWSTR in_szPropertyName )
{
	if ( !wcscmp( in_szPropertyName, szWaveType ) )
	{
		if ( m_hwndPropView ) 
			EnableFrequencyControls( m_hwndPropView, SettingsDialog );

		if ( m_hwndObjPane )
			EnableFrequencyControls( m_hwndObjPane, ContentsEditorDialog );
	}
}

// Get access to UI resource handle.
HINSTANCE ToneGenPlugin::GetResourceHandle() const
{
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool ToneGenPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	CComVariant varProp;

	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = m_idDialogBig;
		out_pTable = ToneGenProp;

		return true;

	case ContentsEditorDialog:
		out_uiDialogID = m_idDialogSmall;
		out_pTable = NULL;

		return true;
	}

	return false;
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool ToneGenPlugin::WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	switch ( in_message )
	{
	case WM_INITDIALOG:
		{
			if ( in_eDialog == SettingsDialog ) 
				m_hwndPropView = in_hWnd;
			else if ( in_eDialog == ContentsEditorDialog ) 
				m_hwndObjPane = in_hWnd;

			EnableFrequencyControls( in_hWnd, in_eDialog );
		}
		break;

	case WM_DESTROY:
		{
			if ( in_eDialog == SettingsDialog ) 
				m_hwndPropView = NULL;
			else if ( in_eDialog == ContentsEditorDialog ) 
				m_hwndObjPane = NULL;
		}
		break;

	// Catch window command actions (regardless if it is object pane or property view) to enable/disable controls
	case WM_COMMAND:
		{
			// Notification code
			switch ( HIWORD( in_wParam ) )
			{

			case BN_CLICKED:

				switch ( LOWORD( in_wParam ) )
				{
				case IDC_CHECK_SWEEPFREQ:

					if ( IsDlgButtonChecked( in_hWnd, IDC_CHECK_SWEEPFREQ ) == BST_CHECKED )
					{
						EnableFrequencySweeping( in_hWnd, in_eDialog, true );	// Enable controls
					}
					else if ( IsDlgButtonChecked( in_hWnd, IDC_CHECK_SWEEPFREQ ) == BST_UNCHECKED )
					{
						EnableFrequencySweeping( in_hWnd, in_eDialog, false );	// Disable controls
					}

					break;

				case IDC_RADIO_FIXLENGTH:

					if ( IsDlgButtonChecked( in_hWnd, IDC_RADIO_FIXLENGTH ) == BST_CHECKED )
					{
						EnableEnvelopeControls( in_hWnd, in_eDialog, false ); // Disable envelope controls
					}
					else if ( IsDlgButtonChecked( in_hWnd, IDC_RADIO_FIXLENGTH ) == BST_UNCHECKED )
					{	
						EnableEnvelopeControls( in_hWnd, in_eDialog, true ); // Enable controls
					}

					break;

				} // End switch low word

				break;	// End BN_CLICKED notification case		

			} // End switch hi word (notification code)

		} // End command window event
		break;

	case WM_ENABLE:
		{
			// Enable/Disable all child controls

			HWND hWnd = ::GetWindow( in_hWnd, GW_CHILD );
			while( hWnd )
			{
				::EnableWindow( hWnd, in_wParam == TRUE );
				hWnd = ::GetWindow( hWnd, GW_HWNDNEXT );
			}

			if ( in_wParam == TRUE )
			{
				EnableFrequencyControls( in_hWnd, in_eDialog );
			}
		}
		return true;
	}

	out_lResult = 0;
	return false;
}

// Store current plugin settings into banks when asked to.
bool ToneGenPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

	m_pPSet->GetValue( in_guidPlatform, szWaveGain, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szStartFreq, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szStopFreq, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szStartFreqRandMin, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szStartFreqRandMax, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szSweepFreq, varProp );
	in_pDataWriter->WriteBool( varProp.boolVal != 0 );

	m_pPSet->GetValue( in_guidPlatform, szSweepFreqType, varProp );
	in_pDataWriter->WriteInt32( varProp.intVal );

	m_pPSet->GetValue( in_guidPlatform, szStopFreqRandMin, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szStopFreqRandMax, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szWaveType, varProp );
	in_pDataWriter->WriteInt32( varProp.intVal );

	m_pPSet->GetValue( in_guidPlatform, szDurMode, varProp );
	in_pDataWriter->WriteInt32( varProp.intVal );

	m_pPSet->GetValue( in_guidPlatform, szFixDur, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szAttackTime, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szDecayTime, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szSustainTime, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szSustainLevel, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szReleaseTime, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szChannelMask, varProp );
	in_pDataWriter->WriteInt32( varProp.intVal );

	return true;
}

// Allow Wwise to retrieve a user friendly name for that property (e.g. Undo etc.).
bool ToneGenPlugin::DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const
{
	for ( DisplayNameInfo * pDisplayNameInfo = g_DisplayNames; pDisplayNameInfo->wszPropName; pDisplayNameInfo++ )
	{
		if ( !wcscmp( in_szPropertyName, pDisplayNameInfo->wszPropName ) )
		{
			// Get resource handle
			HINSTANCE hInst = AfxGetStaticModuleState()->m_hCurrentResourceHandle;
			::LoadString( hInst, pDisplayNameInfo->uiDisplayName, out_szDisplayName, in_unCharCount );
			return true;
		}
	}

	AKASSERT( 0 && "Need display name for property" );
	return false;
}

// Implement online help when the user clicks on the "?" icon .
bool ToneGenPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	DWORD dwTopic = ONLINEHELP::Tone_Generator_Properties;
	if ( in_eDialog == AK::Wwise::IAudioPlugin::ContentsEditorDialog )
		dwTopic = ONLINEHELP::Sound_Object_TG;

	::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, dwTopic, 0 );

	return true;
}

// Enable/disable frequency sweeping controls
void ToneGenPlugin::EnableFrequencySweeping( HWND in_hWnd, eDialog in_eDialog, bool in_bEnable )
{
	if ( in_eDialog == SettingsDialog )
	{
		// Note: Check the wave type  before activating controls
		CComVariant varProp;
		m_pPSet->GetValue( m_pPSet->GetCurrentPlatform(), szWaveType, varProp );
		// Only enable for oscillator types, always disabled when asked to however.
		if ( ( varProp.intVal == 0 || varProp.intVal == 1 || varProp.intVal == 2 || varProp.intVal == 3 ) || !in_bEnable )
		{
			EnableDlgItem( in_hWnd, IDC_RADIO_FREQSWEEPLIN, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_RADIO_FREQSWEEPLOG, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_STATIC_STOPFREQ, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_RANGE_STOPFREQ, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_STATIC_STOPFREQRANDMIN, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_RANGE_STOPFREQRANDMIN, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_STATIC_STOPFREQRANDMAX, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_RANGE_STOPFREQRANDMAX, in_bEnable );
			EnableDlgItem( in_hWnd, IDC_INTERPOLATION_STATIC, in_bEnable );
		}
	}
}

// Enable/disable envelope controls
void ToneGenPlugin::EnableEnvelopeControls( HWND in_hWnd, eDialog in_eDialog, bool in_bEnable )
{
	if ( in_eDialog == SettingsDialog )
	{
		EnableDlgItem( in_hWnd, IDC_STATIC_ATTACKTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_ATTACKTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_DECAYTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_DECAYTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_SUSTAINTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_SUSTAINTIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_SUSTAINLEVEL, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_SUSTAINLEVEL, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_RELEASETIME, in_bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_RELEASETIME, in_bEnable );

		EnableDlgItem( in_hWnd, IDC_RANGE_FIXDUR, !in_bEnable );
	}
}

// Enable/disable all frequency controls
void ToneGenPlugin::EnableFrequencyControls( HWND in_hWnd, eDialog in_eDialog )
{
	CComVariant varProp;
	m_pPSet->GetValue( m_pPSet->GetCurrentPlatform(), szWaveType, varProp );
	bool bEnable = ( varProp.intVal == 0 || varProp.intVal == 1 || varProp.intVal == 2 || varProp.intVal == 3 );

	// For both the object pane view and property view
	EnableDlgItem( in_hWnd, IDC_RANGE_STARTFREQ, bEnable );

	// Most controls only exist in property view
	if ( in_eDialog == SettingsDialog )
	{	
		EnableDlgItem( in_hWnd, IDC_BOX_STATIC_FREQUENCY, bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_STARTFREQ, bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_STARTFREQ, bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_STARTFREQRANDMIN, bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_STARTFREQRANDMIN, bEnable );
		EnableDlgItem( in_hWnd, IDC_STATIC_STARTFREQRANDMAX, bEnable );
		EnableDlgItem( in_hWnd, IDC_RANGE_STARTFREQRANDMAX, bEnable );
		EnableDlgItem( in_hWnd, IDC_CHECK_SWEEPFREQ, bEnable );

		// Enable sweep frequency control if necessary 
		CComVariant varProp;
		m_pPSet->GetValue( m_pPSet->GetCurrentPlatform(), szSweepFreq, varProp );
		bool bFreqSweep = (varProp.boolVal ? true : false) && bEnable;
		EnableFrequencySweeping( in_hWnd, in_eDialog, bFreqSweep );
	}
}

void ToneGenPlugin::EnableDlgItem( HWND in_hwndDialog, UINT in_uiDlgItem, BOOL in_bEnable )
{
	HWND hwndItem = GetDlgItem( in_hwndDialog, in_uiDlgItem );
	AKASSERT( hwndItem );
	::EnableWindow( hwndItem, in_bEnable );
}
