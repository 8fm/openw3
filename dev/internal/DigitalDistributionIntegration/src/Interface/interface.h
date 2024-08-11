/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "languages.h"
#include "red.h"

namespace DDI
{
	enum class InitialisationStatus
	{
		Init_Successful,
		Init_Failure,
	};
}

// Digital Distribution Integration
class Interface
{
public:
	Interface();
	virtual ~Interface() = 0;

	typedef void (*InitCallback)( Interface* instance, DDI::InitialisationStatus status );
	virtual void Initialise( InitCallback callback ) = 0;
	virtual void Update() = 0;

	//////////////////////////////////////////////////////////////////////////
	// Platform
	//////////////////////////////////////////////////////////////////////////
	virtual const wchar_t* GetPlatformIdentifier() const = 0;

	virtual DDI::Language GetTextLanguage() const = 0;
	virtual DDI::Language GetSpeechLanguage() const = 0;

	// Hack because it doesn't really integrate so cleanly this way
	// and let's not abstract this too much to something meaningless
	virtual Bool HACK_ShowSteamControllerBindingPanel() const { return false; }
	virtual Bool HACK_IsUsingSteamController() const { return false; }

	//////////////////////////////////////////////////////////////////////////
	// Feature Support (Returns false if not supported)
	//////////////////////////////////////////////////////////////////////////
	virtual Bool SupportsAchievements() const = 0;
	virtual Bool SupportsStatistics() const = 0;
	virtual Bool SupportsPresence() const = 0;

	//////////////////////////////////////////////////////////////////////////
	// Profile
	//////////////////////////////////////////////////////////////////////////
	virtual Bool GetPlayerName( AnsiChar* buffer, size_t size ) const = 0;

	//////////////////////////////////////////////////////////////////////////
	// Achievements
	//////////////////////////////////////////////////////////////////////////
	virtual Bool UnlockAchievement( const AnsiChar* achievement ) = 0;
	virtual Bool LockAchievement( const AnsiChar* achievement ) = 0;
	virtual Bool IsAchievementUnlocked( const AnsiChar* achievement ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Statistics
	//////////////////////////////////////////////////////////////////////////
	virtual Bool SetStat( const AnsiChar* name, Int32 value );
	virtual Bool SetStat( const AnsiChar* name, Float value );
	virtual Bool SetAverageStat( const AnsiChar* name, Float countThisSession, Double sessionLength );

	virtual Bool IncrementStat( const AnsiChar* name, Int32 value );
	virtual Bool IncrementStat( const AnsiChar* name, Float value );

	virtual Bool GetStat( const AnsiChar* name, Int32& value );
	virtual Bool GetStat( const AnsiChar* name, Float& value );

	//////////////////////////////////////////////////////////////////////////
	// Rich Presence
	//////////////////////////////////////////////////////////////////////////
	virtual Bool SetPresence( const AnsiChar* presence ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Witcher 2 save import
	//////////////////////////////////////////////////////////////////////////
	virtual Uint32 GetNumberOfCloudStoragePaths() const;
	virtual Bool GetCloudStoragePath( Uint32 index, AnsiChar* buffer, size_t size ) const;

	//////////////////////////////////////////////////////////////////////////
	// Debugging
	//////////////////////////////////////////////////////////////////////////
	typedef void (*DebugLogFunc)( const AnsiChar* message );
	virtual Bool SetDebugLogListener( DebugLogFunc callback );
};
