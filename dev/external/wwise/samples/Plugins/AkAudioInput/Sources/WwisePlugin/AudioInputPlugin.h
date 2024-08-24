//////////////////////////////////////////////////////////////////////
//
// AudioInputPlugin.h
//
// Audio Input Wwise plugin implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "SoundInput.h"

// Audio input plugin property names
static LPCTSTR szAudioInputGain        = L"AudioInputGain";

class AudioInputPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	AudioInputPlugin();
	~AudioInputPlugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin
	virtual void SetPluginPropertySet( AK::Wwise::IPluginPropertySet * in_pPSet );

	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable ) const;

    virtual bool GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const;

	virtual bool DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const;

	virtual bool Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const;

	static void GetFormatCallbackFunc(
		AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
		);

	static void ExecuteCallbackFunc(
		AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
	    );

	static const short CompanyID;
	static const short PluginID;

private:
	SoundInput m_Input;
	AK::Wwise::IPluginPropertySet * m_pPSet;
};
