/**
 * Copyright (c) 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "userProfileManagerLinux.h"

#include "../../../internal/DigitalDistributionIntegration/src/Interface/interface.h"

CUserProfileManagerLinux::CUserProfileManagerLinux()
{
}

CUserProfileManagerLinux::~CUserProfileManagerLinux()
{

}

void CUserProfileManagerLinux::LogWrapper( const AnsiChar* message )
{
	RED_LOG( DDI, TXT( "%hs" ), message );
}

void CUserProfileManagerLinux::LoadAllInCommandLine( const Char* basePath )
{

}

void CUserProfileManagerLinux::LoadAllInDirectory( const Char* directory )
{

}

void CUserProfileManagerLinux::InitializationComplete( Interface* instance, DDI::InitialisationStatus status )
{
	if( status == DDI::InitialisationStatus::Init_Failure )
	{
		RED_LOG_SPAM( UserProfileManager, TXT( "DDI for %ls failed to initialise!" ), instance->GetPlatformIdentifier() );

		// Remove it from the list
		CUserProfileManagerLinux* profileManager = static_cast< CUserProfileManagerLinux* >( GUserProfileManager );

		for( Uint32 i = 0; i < profileManager->m_interfaces.Size(); ++i )
		{
			InterfaceWrapper* wrapper = profileManager->m_interfaces[ i ];

			if( wrapper->m_interface == instance )
			{
				wrapper->m_queueForUnload = true;
			}
		}
	}
}

void* CUserProfileManagerLinux::InterfaceWrapper::operator new( size_t size )
{
	return RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, MC_UserProfileManager, sizeof( InterfaceWrapper ) );
}

void CUserProfileManagerLinux::InterfaceWrapper::operator delete( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_UserProfileManager, ptr );
}

Bool CUserProfileManagerLinux::Load( const Char* libraryPath )
{
	RED_LOG_SPAM( UserProfileManager, TXT( "Loading DDI: %ls" ), libraryPath );
	return true;
}

void CUserProfileManagerLinux::Unload( Uint32 index )
{
	RED_FATAL_ASSERT( index < m_interfaces.Size(), "Invalid user interface index specified" );
}

void CUserProfileManagerLinux::Unload( Interface* userInterface )
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( wrapper->m_interface == userInterface )
		{
			Unload( i );
			break;
		}
	}
}

void CUserProfileManagerLinux::UnloadAll()
{
	while( m_interfaces.Size() > 0 )
	{
		Unload( 0u );
	}
}

Bool CUserProfileManagerLinux::Initialize()
{
	return CUserProfileManagerLinuxSaving::Initialize();
}

void CUserProfileManagerLinux::Update()
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		wrapper->m_interface->Update();

		if( wrapper->m_queueForUnload )
		{
			Unload( i );
		}
	}

	CUserProfileManagerLinuxSaving::Update();
}

Bool CUserProfileManagerLinux::Shutdown()
{
	UnloadAll();

	return true;
}

Bool CUserProfileManagerLinux::HACK_ShowSteamControllerBindingPanel() const
{
	for( auto wrapper : m_interfaces )
	{
		if ( wrapper->m_interface->HACK_ShowSteamControllerBindingPanel() )
			return true;
	}

	return false;
}

Bool CUserProfileManagerLinux::HACK_IsUsingSteamController() const
{
	for( auto wrapper : m_interfaces )
	{
		if ( wrapper->m_interface->HACK_IsUsingSteamController() )
		{
			printf("hep");
			return true;
		}
	}

	return false;
}

void CUserProfileManagerLinux::DisplayUserProfileSystemDialog()
{

}

void CUserProfileManagerLinux::SetActiveUserDefault()
{

}

void CUserProfileManagerLinux::SetActiveUserPromiscuous()
{

}

Bool CUserProfileManagerLinux::HasActiveUser() const
{
	return true;
}

String CUserProfileManagerLinux::GetActiveUserDisplayNameRaw() const
{
	AnsiChar buffer[ 128 ];

	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		const InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( wrapper->m_interface->GetPlayerName( buffer, ARRAY_COUNT( buffer ) ) )
		{
			return String::Printf( TXT( "%hs" ), buffer );
		}
	}

	return String::EMPTY;
}

void CUserProfileManagerLinux::StatCommon( std::function< Bool ( Interface*, const AnsiChar* name ) > proxy, const String& name )
{
	const Uint32 size = 128;
	AnsiChar convertedNameBuffer[ size ];

	if( Red::System::StringConvert( convertedNameBuffer, name.AsChar(), size ) )
	{
		for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
		{
			InterfaceWrapper* wrapper = m_interfaces[ i ];

			if( wrapper->m_interface->SupportsStatistics() )
			{
				if( !proxy( wrapper->m_interface, convertedNameBuffer ) )
				{
					RED_LOG_ERROR( UserProfileManager, TXT( "Failed to set/get statistic: %hs for platform %ls" ), convertedNameBuffer, wrapper->m_interface->GetPlatformIdentifier() );
				}
			}
		}
	}
	else
	{
		RED_LOG_ERROR( UserProfileManager, TXT( "Couldn't convert string %ls from uni to ansi to set user profile statistic" ), name.AsChar() );
	}
}

void CUserProfileManagerLinux::SetStat( const String& name, Int32 value )
{
	StatCommon
	(
		[ value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->SetStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::SetStat( const String& name, Float value )
{
	StatCommon
	(
		[ value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->SetStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::SetAverageStat( const String& name, Float countThisSession, Double sessionLength )
{
	StatCommon
	(
		[ countThisSession, sessionLength ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->SetAverageStat( name, countThisSession, sessionLength );
		},
		name
	);
}

void CUserProfileManagerLinux::IncrementStat( const String& name, Int32 value )
{
	StatCommon
	(
		[ value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->IncrementStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::IncrementStat( const String& name, Float value )
{
	StatCommon
	(
		[ value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->IncrementStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::GetStat( const String& name, Int32& value )
{
	StatCommon
	(
		[ &value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->GetStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::GetStat( const String& name, Float& value )
{
	StatCommon
	(
		[ &value ]( Interface* userProfileInterface, const AnsiChar* name )
		{
			return userProfileInterface->GetStat( name, value );
		},
		name
	);
}

void CUserProfileManagerLinux::PrintUserStats( const String& name )
{
	StatCommon
	(
		[]( Interface* userProfileInterface, const AnsiChar* name )
		{
			Float fStat = 0.0f;

			if( userProfileInterface->GetStat( name, fStat ) )
			{
				RED_LOG( UserProfileManager, TXT( "Value for float stat \"%hs\": %f" ), name, fStat );
			}
			else
			{
				// Try int instead
				Int32 iStat = 0;
				if( userProfileInterface->GetStat( name, iStat ) )
				{
					RED_LOG( UserProfileManager, TXT( "Value for int stat \"%hs\": %i" ), name, iStat );
				}
				else
				{
					RED_LOG_WARNING( UserProfileManager, TXT( "Could not find stat with name \"%hs\"" ), name );
					return false;
				}
			}

			return true;
		},
		name
	);
}

Bool CUserProfileManagerLinux::GetSystemLanguageCommon( String& language, DDI::Language ( Interface::*interfaceFuncToCall )() const ) const
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		const InterfaceWrapper* wrapper = m_interfaces[ i ];

		DDI::Language ddiLang = ( wrapper->m_interface->*interfaceFuncToCall )();

		switch( ddiLang )
		{
		case DDI::Lang_Arabic:
			language = TXT( "AR" );
			break;

		case DDI::Lang_Chinese_Simplified:
			language = TXT( "CN" );
			break;

		case DDI::Lang_Chinese_Traditional:
			language = TXT( "ZH" );
			break;

		case DDI::Lang_Czech:
			language = TXT( "CZ" );
			break;

		case DDI::Lang_EnglishUK:
		case DDI::Lang_EnglishUS:
			language = TXT( "EN" );
			break;

		case DDI::Lang_FrenchFR:
		case DDI::Lang_FrenchCA:
			language = TXT( "FR" );
			break;

		case DDI::Lang_German:
			language = TXT( "DE" );
			break;

		case DDI::Lang_Hungarian:
			language = TXT( "HU" );
			break;

		case DDI::Lang_Italian:
			language = TXT( "IT" );
			break;

		case DDI::Lang_Japanese:
			language = TXT( "JP" );
			break;

		case DDI::Lang_Korean:
			language = TXT( "KR" );
			break;

		case DDI::Lang_Polish:
			language = TXT( "PL" );
			break;

		case DDI::Lang_PortugueseBR:
		case DDI::Lang_PortuguesePT:
			language = TXT( "BR" );
			break;

		case DDI::Lang_Russian:
			language = TXT( "RU" );
			break;

		case DDI::Lang_SpanishES:
			language = TXT( "ES" );
			break;

		case DDI::Lang_SpanishMX:
			language = TXT( "ESMX" );
			break;

		case DDI::Lang_Turkish:
			language = TXT( "TR" );
			break;
		}

		if( !language.Empty() )
		{
			return true;
		}
	}

	return false;
}

Bool CUserProfileManagerLinux::GetSystemLanguageStrings( String& language ) const
{
	return GetSystemLanguageCommon( language, &Interface::GetTextLanguage );
}

Bool CUserProfileManagerLinux::GetSystemLanguageSpeech( String& language ) const
{
	return GetSystemLanguageCommon( language, &Interface::GetSpeechLanguage );
}

void CUserProfileManagerLinux::UnlockAchievement( const CName& name )
{
	AchievementLookupCommon( name, &Interface::UnlockAchievement );
}

void CUserProfileManagerLinux::LockAchievement( const CName& name )
{
	AchievementLookupCommon( name, &Interface::LockAchievement );
}

Bool CUserProfileManagerLinux::IsAchievementLocked( const CName& name ) const
{
	return AchievementLookupCommon( name, &Interface::IsAchievementUnlocked );
}

Bool CUserProfileManagerLinux::AchievementLookupCommon( const CName& name, Bool (Interface::*interfaceFuncToCall)( const char* ) ) const
{
	Bool success = true;

	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		const InterfaceWrapper* wrapper = m_interfaces[ i ];

		String platformAchievementId;

		if( wrapper->m_achievementMap.Find( name, platformAchievementId ) )
		{
			const Uint32 maxAchievementIdSize = 64;
			char convertedPlatformAchievementId[ maxAchievementIdSize ];
			Red::System::StringConvert( convertedPlatformAchievementId, platformAchievementId.AsChar(), ARRAY_COUNT( convertedPlatformAchievementId ) );

			success &= ( wrapper->m_interface->*interfaceFuncToCall )( convertedPlatformAchievementId );
		}
		else
		{
			success = false;
			RED_LOG_ERROR( UserProfileManager, TXT( "Achievement '%ls' is not mapped" ), name.AsChar() );
		}
	}

	return success;
}

Uint32 CUserProfileManagerLinux::GetNumberOfWitcher2CloudSavePaths() const
{
	Uint32 count = 0;

	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		count += wrapper->m_interface->GetNumberOfCloudStoragePaths();
	}

	return count;
}

String CUserProfileManagerLinux::GetWitcher2CloudSavePath( Uint32 index ) const
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		const Uint32 numForWrapper = wrapper->m_interface->GetNumberOfCloudStoragePaths();

		if( index < numForWrapper )
		{
			const Uint32 maxSize = 1024;
			AnsiChar path[ maxSize ];

			wrapper->m_interface->GetCloudStoragePath( index, path, maxSize );

			return String::Printf( TXT( "%hs" ), path );
		}

		index -= numForWrapper;
	}

	return String::EMPTY;
}

void CUserProfileManagerLinux::MapAchievementInit( Uint32 numAchievements )
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		const Uint32 totalNumberOfAchievements = numAchievements + wrapper->m_achievementMap.Capacity();
		wrapper->m_achievementMap.Reserve( totalNumberOfAchievements );
	}
}

void CUserProfileManagerLinux::MapAchievement( const CName& name, const String& platform, const String& id )
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( Red::System::StringCompare( wrapper->m_interface->GetPlatformIdentifier(), platform.AsChar() ) == 0 )
		{
			wrapper->m_achievementMap.Insert( name, id );
		}
	}
}

Bool CUserProfileManagerLinux::IsAchievementMapped( const CName& name ) const
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		const InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( !wrapper->m_achievementMap.KeyExist( name ) )
		{
			return false;
		}
	}

	return true;
}

void* CUserProfileManagerLinux::Realloc( void* ptr, size_t size )
{
	return RED_MEMORY_REALLOCATE( MemoryPool_Default, ptr, MC_PlatformIntegrationDLL, size );
}

void CUserProfileManagerLinux::Free( const void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_PlatformIntegrationDLL, ptr );
}

void CUserProfileManagerLinux::DoUserPresence( const CName& presenceName )
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( wrapper->m_interface->SupportsPresence() )
		{
			PresenceMap& map = wrapper->m_presenceMap;

			Uint32 stringId = 0;
			if( map.Find( presenceName, stringId ) )
			{
				// Grab the presence string from the string database
				LocalizedString localizedPresenceString;
				localizedPresenceString.SetIndex( stringId );
				String presenceString = localizedPresenceString.GetString();

				const size_t maximumPresenceLength = 1024;
				AnsiChar convertedPresenceString[ maximumPresenceLength ];

				Red::System::StringConvert( convertedPresenceString, presenceString.AsChar(), maximumPresenceLength );

				wrapper->m_interface->SetPresence( convertedPresenceString );
			}
		}
	}
}

void CUserProfileManagerLinux::MapPresenceInit( Uint32 numEntries )
{
	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( wrapper->m_interface->SupportsPresence() )
		{
			PresenceMap& map = wrapper->m_presenceMap;

			Uint32 newSize = map.Capacity() + numEntries;
			map.Reserve( newSize );
		}
	}
}

void CUserProfileManagerLinux::MapPresence( const CName& name, const String& platform, const String& id )
{
	Uint32 localizedStringId = 0;

	for( Uint32 i = 0; i < m_interfaces.Size(); ++i )
	{
		InterfaceWrapper* wrapper = m_interfaces[ i ];

		if( wrapper->m_interface->SupportsPresence() )
		{
			if( platform == wrapper->m_interface->GetPlatformIdentifier() )
			{
				if( localizedStringId == 0 )
				{
					if( !FromString( id, localizedStringId ) )
					{
						RED_LOG_ERROR( UserProfileManager, TXT( "'%ls' Is not a valid localised string id!" ), id.AsChar() );
						return;
					}
				}

				PresenceMap& map = wrapper->m_presenceMap;

				map.Set( name, localizedStringId );
			}
		}
	}
}
