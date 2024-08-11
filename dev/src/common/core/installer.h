/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "contentManifest.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IStreamingInstaller;

//////////////////////////////////////////////////////////////////////////
// ILanguageProvider
//////////////////////////////////////////////////////////////////////////
class ILanguageProvider
{
public:
	virtual Bool			SetSpeechInstallLanguage( const String& speechLanguage )=0;
	virtual Bool			GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const=0;
	virtual Bool			GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const=0;

protected:
	ILanguageProvider() {}
	virtual ~ILanguageProvider() {}
};

class IContentInstaller
{
public:
	virtual void							Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )=0;
	virtual Bool							Init()=0;
	virtual Bool							IsReady() const=0;

public:
	virtual void							OnSuspend() {}
	virtual void							OnResume() {}
	virtual void							OnEnterConstrain() {}
	virtual void							OnExitConstrain() {}

public:
	virtual Bool							QueryStreamingInstaller( IStreamingInstaller** outStreamingInstaller ) { return false; }
	virtual Bool							QueryLanguageProvider( ILanguageProvider** outLanguageProvider ) { return false; }

protected:
	IContentInstaller() {}
	virtual ~IContentInstaller() {}
};