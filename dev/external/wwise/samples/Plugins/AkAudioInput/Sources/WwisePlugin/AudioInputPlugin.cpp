//////////////////////////////////////////////////////////////////////
//
// AudioInputPlugin.cpp
//
// Audio Input Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "AudioInputPlugin.h"
#include "Help\TopicAlias.h"
#include <AK\Plugin\AkAudioInputSourceFactory.h>
#include "SoundInput.h"

#define _USE_MATH_DEFINES
#include <math.h>

// Microphone Input
using namespace AK;
using namespace Wwise;

void AudioInputPlugin::GetFormatCallbackFunc(
    AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
    )
{
}

void AudioInputPlugin::ExecuteCallbackFunc(
    AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
    )
{
	AkSampleType * pSamples = io_pBufferOut->GetChannel( 0 );

	for ( int i = 0; i < io_pBufferOut->MaxFrames(); ++i )
		pSamples[ i ] = (float) sin( (double) i / 256.0 * M_PI * 2.0 );

	io_pBufferOut->eState = AK_DataReady;
	io_pBufferOut->uValidFrames = io_pBufferOut->MaxFrames();
}

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short AudioInputPlugin::CompanyID = AKCOMPANYID_AUDIOKINETIC;
const short AudioInputPlugin::PluginID = AKSOURCEID_AUDIOINPUT;

// Constructor
AudioInputPlugin::AudioInputPlugin()
	: m_pPSet( NULL )
{
	m_Input.InputOn();
}

// Destructor
AudioInputPlugin::~AudioInputPlugin()
{
	m_Input.InputOff();
}

// Implement the destruction of the Wwise source plugin.
void AudioInputPlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void AudioInputPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Get access to UI resource handle.
HINSTANCE AudioInputPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool AudioInputPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = IDD_AUDIOINPUTPLUGIN_BIG;
		out_pTable = NULL;
		return true;

	case ContentsEditorDialog:
		out_uiDialogID = IDD_AUDIOINPUTPLUGIN_SMALL;
		out_pTable = NULL;
		return true;
	}

	return false;
}

// Store current plugin settings into banks when asked to.
bool AudioInputPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

	m_pPSet->GetValue( in_guidPlatform, szAudioInputGain, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	return true;
}


// Allow Wwise to retrieve a user friendly name for that property (e.g. Undo etc.).
bool AudioInputPlugin::DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const
{
	// Get resource handle
	HINSTANCE hInst = AfxGetStaticModuleState()->m_hCurrentResourceHandle;

	if ( ! wcscmp( in_szPropertyName, szAudioInputGain ) )
	{
		::LoadString( hInst, IDS_AUDIOINPUT_GAIN, out_szDisplayName, in_unCharCount );
		return true;	
	}

	return false;
}

// Implement online help when the user clicks on the "?" icon .
bool AudioInputPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	DWORD dwTopic = ONLINEHELP::Audio_Input_Property;
	if ( in_eDialog == AK::Wwise::IAudioPlugin::ContentsEditorDialog )
		dwTopic = ONLINEHELP::Audio_Input_Contents;

	::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, dwTopic, 0 );

	return true;
}
