//////////////////////////////////////////////////////////////////////
//
// MP3Plugin.h
//
// Audio Input Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>
#include "../AudioEngineFX/Win32/AkMP3TagReader.h"

static LPCTSTR szToken = L"Token";

class MP3Plugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	MP3Plugin();
	~MP3Plugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin
	virtual void SetPluginPropertySet( AK::Wwise::IPluginPropertySet * in_pPSet );

	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable ) const;
	virtual bool WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

    virtual bool GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const;

	virtual bool DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const;

	virtual bool Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const;

	static const short CompanyID;
	static const short PluginID;

	static void PlayerCallback( void* in_pCookie, AkTimeMs in_msPos );

private:
	void UpdateUI( HWND in_hWnd );

	AK::Wwise::IPluginPropertySet * m_pPSet;

	static AkMP3BaseInfo m_info;
	static AkMP3Tag m_tag;
	static TCHAR m_tszFilename[ MAX_PATH ];
	static AkUInt32 m_uStartAt; // 0-100
	static AkTimeMs m_msPlayPos;
};
