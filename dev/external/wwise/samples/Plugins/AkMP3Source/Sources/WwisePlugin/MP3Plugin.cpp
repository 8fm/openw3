//////////////////////////////////////////////////////////////////////
//
// MP3Plugin.cpp
//
// Audio Input Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MP3Plugin.h"
#include "Help\TopicAlias.h"
#include <AK\Plugin\AkMP3SourceFactory.h>
#include <windowsx.h>
#include "AK/Tools/Common/AkAssert.h"

#define PLAYPOS_UPDATE_RATE 100 // milliseconds

using namespace AK;
using namespace Wwise;

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short MP3Plugin::CompanyID = AKCOMPANYID_AUDIOKINETIC;
const short MP3Plugin::PluginID = AKSOURCEID_MP3;

AkMP3BaseInfo MP3Plugin::m_info = { 0 };
AkUInt32 MP3Plugin::m_uStartAt = 0;
AkMP3Tag MP3Plugin::m_tag = { 0 };
TCHAR MP3Plugin::m_tszFilename[ MAX_PATH ] = { 0 };
AkTimeMs MP3Plugin::m_msPlayPos = 0;

static LPTSTR TimeMsToString( AkTimeMs in_msTime, LPTSTR out_tszTime )
{
	_stprintf_s( out_tszTime, 256, _T("%2d:%02d.%03d" ), in_msTime / 60 / 1000, ( in_msTime / 1000 ) % 60, in_msTime % 1000 );
	return out_tszTime;
}

// Constructor
MP3Plugin::MP3Plugin()
	: m_pPSet( NULL )
{
}

// Destructor
MP3Plugin::~MP3Plugin()
{
}

// Implement the destruction of the Wwise source plugin.
void MP3Plugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void MP3Plugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Get access to UI resource handle.
HINSTANCE MP3Plugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool MP3Plugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = IDD_MP3PLUGIN_BIG;
		out_pTable = NULL;
		return true;

	case ContentsEditorDialog:
		out_uiDialogID = IDD_MP3PLUGIN_SMALL;
		out_pTable = NULL;
		return true;
	}

	return false;
}

void MP3Plugin::UpdateUI( HWND in_hWnd )
{
	const size_t kBufSize = 256;
	TCHAR tszTmp[ kBufSize ];

	HWND hwndCtrl = ::GetDlgItem( in_hWnd, IDC_CHANNELS );
	_itot_s( m_info.uNumChannels, tszTmp, kBufSize, 10 );
	::SetWindowText( hwndCtrl, tszTmp );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_SAMPLERATE );
	_itot_s( m_info.uSampleRate, tszTmp, kBufSize, 10 );
	::SetWindowText( hwndCtrl, tszTmp );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_DURATION );
	::SetWindowText( hwndCtrl, TimeMsToString( m_info.msDuration, tszTmp ) );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_TRACK );
	::SetWindowText( hwndCtrl, m_tag.wszTrack );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_ARTIST );
	::SetWindowText( hwndCtrl, m_tag.wszArtist );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_ALBUM );
	::SetWindowText( hwndCtrl, m_tag.wszAlbum );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_FILENAME );
	::SetWindowText( hwndCtrl, m_tszFilename );

	hwndCtrl = ::GetDlgItem( in_hWnd, IDC_STARTPOS );
	ScrollBar_SetRange( hwndCtrl, 0, 100, FALSE );
	ScrollBar_SetPos( hwndCtrl, m_uStartAt, TRUE );
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool MP3Plugin::WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	switch ( in_message )
	{
	case WM_INITDIALOG:
		UpdateUI( in_hWnd );
		::SetTimer( in_hWnd, 1, PLAYPOS_UPDATE_RATE, NULL );
		break;

	case WM_DESTROY:
		::KillTimer( in_hWnd, 1 );
		break;

	case WM_COMMAND:
		{
			WORD wmId = LOWORD( in_wParam ); 
			WORD wmEvent = HIWORD( in_wParam ); 

			switch ( wmEvent )
			{
			case BN_CLICKED:
				{
					OPENFILENAME ofn;
					::ZeroMemory( &ofn, sizeof( ofn ) );

					ofn.lStructSize = sizeof( ofn );
					ofn.hwndOwner = in_hWnd;
					ofn.lpstrFilter = _T("MP3 Files (*.mp3)\0*.mp3\0All Files (*.*)\0*.*\0");
					ofn.lpstrFile = m_tszFilename;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
					ofn.lpstrDefExt = _T("mp3");
					if ( ::GetOpenFileName( &ofn ) )
					{
						if ( AkMP3InfoRead( m_tszFilename, m_info, &m_tag ) )
						{
							AkMP3PrepareForPlayback( m_tszFilename, m_info, (AkTimeMs) ( m_uStartAt / 100.0f * m_info.msDuration ), PlayerCallback );
						}

						UpdateUI( in_hWnd );
					}
				}
				break;
			}
		}
		break;

	case WM_HSCROLL:
		{
			WORD wCode = LOWORD( in_wParam );
			if (wCode != SB_THUMBTRACK )
			{
				m_uStartAt = (AkUInt32) ::SendMessage( (HWND) in_lParam, TBM_GETPOS, 0, 0 );
				AkMP3PrepareForPlayback( m_tszFilename, m_info, (AkTimeMs) ( m_uStartAt / 100.0f * m_info.msDuration ), PlayerCallback );
			}
		}
		break;

	case WM_TIMER:
		{
			TCHAR tszTmp[ 256 ];
			::SetWindowText( ::GetDlgItem( in_hWnd, IDC_PLAYPOS ), TimeMsToString( m_msPlayPos, tszTmp ) );
		}
		break;

	default:
		break;
	}

	return false;
}

// Store current plugin settings into banks when asked to.
bool MP3Plugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{	
	CComVariant varProp;

	m_pPSet->GetValue( in_guidPlatform, szToken, varProp );
	in_pDataWriter->WriteInt32( varProp.uintVal );

	return true;
}

bool MP3Plugin::DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const
{
	HINSTANCE hInst = AfxGetStaticModuleState()->m_hCurrentResourceHandle;

	if ( ! wcscmp( in_szPropertyName, szToken ) )
	{
		::LoadString( hInst, IDS_MP3_TOKEN, out_szDisplayName, in_unCharCount );
		return true;	
	}

	return false;
}

bool MP3Plugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	switch(in_eDialog) 
	{
	case AK::Wwise::IAudioPlugin::SettingsDialog:
		::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, ONLINEHELP::MP3_Input_Properties, 0 );
		break;

	case AK::Wwise::IAudioPlugin::ContentsEditorDialog:
		::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, ONLINEHELP::MP3_Input_Contents, 0 );
		break;

	default:
		AKASSERT( !"@MP3Plugin::Help: Unrecognized in_eDialog option!" );
		return false;
	}

	return true;
}

void MP3Plugin::PlayerCallback( void* in_pCookie, AkTimeMs in_msPos )
{
	m_msPlayPos = in_msPos;
}

