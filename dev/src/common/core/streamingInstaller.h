/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class	IContentListener;
class	ILanguageProvider;
struct	SContentManifest;
struct	SContentPackageEvent;

//////////////////////////////////////////////////////////////////////////
// EContentStallType
//////////////////////////////////////////////////////////////////////////
enum EContentStallType
{
	eContentStall_None,
	eContentStall_FinalContent,
	eContentStall_DiscLaunch,
	eContentStall_Prefetch
};

//////////////////////////////////////////////////////////////////////////
// IStreamingInstaller
//////////////////////////////////////////////////////////////////////////
class IStreamingInstaller
{
public:
	virtual void						Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )=0;
	virtual Bool						IsContentAvailable( CName contentName, Bool& outIsAvailable ) const=0;
	virtual Bool						GetPercentCompleted( CName contentName, Uint32& outPercentCompleted ) const=0;
	virtual Bool						GetResolvedLaunchZero( CName& outLaunchZero ) const=0;

	virtual EContentStallType			GetStallForMoreContent() const { return eContentStall_None; }
	virtual void						PrefetchContent( CName contentName ) {}
	virtual void						ResetPrefetchContent() {}

protected:
	IStreamingInstaller() {}

public:
	virtual ~IStreamingInstaller() {}
};
