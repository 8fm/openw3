//////////////////////////////////////////////////////////////////////
//
// SinePlugin.cpp
//
// Sine tone Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "SinePlugin.h"
#include "TopicAlias.h"
#include <AK\Plugin\AkSineSourceFactory.h>

using namespace AK;
using namespace Wwise;

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short SinePlugin::CompanyID = AKCOMPANYID_AUDIOKINETIC;
const short SinePlugin::PluginID = AKSOURCEID_SINE;

// Delay property names
static LPCWSTR szSineFrequency = L"SineFrequency";
static LPCWSTR szSineGain = L"SineGain";
static LPCWSTR szSineDuration = L"SineDuration";
static LPCWSTR szChannelMask = L"ChannelMask";

// Table of display name resources ( one for each property )
struct DisplayNameInfo
{
	LPCWSTR wszPropName;
	UINT    uiDisplayName;
};

// A table binding property to display names for convenience
static DisplayNameInfo g_DisplayNames[] = 
{
	{ szSineFrequency, IDS_SINEFREQ },
	{ szSineGain, IDS_SINEGAIN },
	{ szSineDuration, IDS_SINEDURATION },
	{ szChannelMask, IDS_OUTPUTCHANNEL },
	{ NULL, NULL }
};


// Constructor
SinePlugin::SinePlugin()
	: m_pPSet( NULL )
{
}

// Destructor
SinePlugin::~SinePlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void SinePlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void SinePlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Get access to UI resource handle.
HINSTANCE SinePlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool SinePlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = IDD_SINEPLUGIN_BIG;
		out_pTable = NULL;
		return true;

	case ContentsEditorDialog:
		out_uiDialogID = IDD_SINEPLUGIN_SMALL;
		out_pTable = NULL;
		return true;
	}

	return false;
}

// Store current plugin settings into banks.
bool SinePlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

	m_pPSet->GetValue( in_guidPlatform, szSineFrequency, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szSineGain, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szSineDuration, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	m_pPSet->GetValue( in_guidPlatform, szChannelMask, varProp );
	in_pDataWriter->WriteInt32( varProp.intVal );

	return true;
}

// Allow Wwise to retrieve a user friendly name for that property (e.g. Undo etc.).
bool SinePlugin::DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const
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

	AKASSERT( !"Need display name for property" );
	return false;
}

// Implement online help when the user clicks on the "?" icon .
bool SinePlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	DWORD dwTopic = ONLINEHELP::Sine_Properties;
	if ( in_eDialog == AK::Wwise::IAudioPlugin::ContentsEditorDialog )
		dwTopic = ONLINEHELP::Sound_Object_Sine;

	::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, dwTopic, 0 );

	return true;
}
