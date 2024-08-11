/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/os.h"

#include "../../common/engine/gameSaveManager.h"

#include "userProfileManagerWindowsSaving.h"

class Interface;

namespace DDI
{
	enum class InitialisationStatus;
	enum Language;
}

class CUserProfileManagerWindows : public CUserProfileManagerWindowsSaving
{
public:
	CUserProfileManagerWindows();
	virtual ~CUserProfileManagerWindows() override final;

	//////////////////////////////////////////////////////////////////////////
	// DLL
	//////////////////////////////////////////////////////////////////////////
private:
	void LoadAllInCommandLine( const Char* basePath );
	void LoadAllInConfig();
	void LoadAllInDirectory( const Char* directory );

	Bool Load( const Char* libraryPath );

	void Unload( Interface* userInterface );
	void Unload( Uint32 index );
	void UnloadAll();

	static void LogWrapper( const AnsiChar* message );

	//////////////////////////////////////////////////////////////////////////
	// General
	//////////////////////////////////////////////////////////////////////////
public:
	virtual Bool Initialize() override final;
	virtual void Update() override final;
	virtual Bool Shutdown() override final;
	
private:
	static void* Realloc( void* ptr, size_t size );
	static void Free( const void* ptr );
	static void InitializationComplete( Interface* instance, DDI::InitialisationStatus status );

public:
	virtual Bool HACK_ShowSteamControllerBindingPanel() const override final;
	virtual Bool HACK_IsUsingSteamController() const override final;

	//////////////////////////////////////////////////////////////////////////
	// User Profile
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void DisplayUserProfileSystemDialog() override final;
	virtual void SetActiveUserDefault() override final;
	virtual void SetActiveUserPromiscuous() override final;
	virtual Bool HasActiveUser() const override final;

	virtual String GetActiveUserDisplayNameRaw() const override final;
	virtual Bool GetSafeArea( Float& x, Float& y ) override final { x = 1.0f; y = 1.0f; return true; }

	//////////////////////////////////////////////////////////////////////////
	// Rich Presence
	//////////////////////////////////////////////////////////////////////////
	virtual void DoUserPresence( const CName& presenceName ) override final;

private:
	virtual void MapPresenceInit( Uint32 numEntries ) override final;
	virtual void MapPresence( const CName& name, const String& platform, const String& id ) override final;

	//////////////////////////////////////////////////////////////////////////
	// Statistics 
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void SetStat( const String& name, Int32 value ) override final;
	virtual void SetStat( const String& name, Float value ) override final;
	virtual void SetAverageStat( const String& name, Float countThisSession, Double sessionLength ) override final;

	virtual void IncrementStat( const String& name, Int32 value ) override final;
	virtual void IncrementStat( const String& name, Float value ) override final;

	virtual void GetStat( const String& name, Int32& value ) override final;
	virtual void GetStat( const String& name, Float& value ) override final;

	// debug
	virtual void PrintUserStats( const String& name ) override final;

private:
	void StatCommon( std::function< Bool ( Interface*, const AnsiChar* name ) > proxy, const String& name );

	//////////////////////////////////////////////////////////////////////////
	// System language
	//////////////////////////////////////////////////////////////////////////
public:
	virtual Bool GetSystemLanguageStrings( String& language ) const override final;
	virtual Bool GetSystemLanguageSpeech( String& language ) const override final;

private:
	Bool GetSystemLanguageCommon( String& language, DDI::Language ( Interface::*interfaceFuncToCall )() const ) const;

	//////////////////////////////////////////////////////////////////////////
	// Achievement management
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void UnlockAchievement( const CName& name ) override final;
	virtual void LockAchievement( const CName& name ) override final;
	virtual Bool IsAchievementLocked( const CName& name ) const override final;

	virtual Bool IsAchievementMapped( const CName& name ) const override final;
	virtual void MapAchievementInit( Uint32 numAchievements ) override final;
	virtual void MapAchievement( const CName& name, const String& platform, const String& id ) override final;

private:
	Bool AchievementLookupCommon( const CName& name, Bool ( Interface::*interfaceFuncToCall )( const char* ) ) const;

	//////////////////////////////////////////////////////////////////////////
	// W2 Cloud saving import
	//////////////////////////////////////////////////////////////////////////
public:
	virtual Uint32 GetNumberOfWitcher2CloudSavePaths() const override final;
	virtual String GetWitcher2CloudSavePath( Uint32 index ) const override final;

private:
	// How we refer to achievements (cnames) -> How the platform refers to it
	typedef TSortedMap< CName, String > AchievementMap;
	typedef TSortedMap< CName, Uint32 > PresenceMap;

	struct InterfaceWrapper
	{
		HMODULE m_handle;
		Interface* m_interface;

		AchievementMap m_achievementMap;
		PresenceMap m_presenceMap;

		Bool m_queueForUnload;

		InterfaceWrapper()
		{
		}

		void* operator new( size_t size );
		void operator delete( const void* ptr );
	};

	TDynArray< InterfaceWrapper* > m_interfaces;
};
