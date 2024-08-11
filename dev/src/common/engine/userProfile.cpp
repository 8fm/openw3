/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "userProfile.h"
#include "gameSaveManager.h"

#include "../core/xmlFileReader.h"
#include "../core/dataError.h"

IMPLEMENT_RTTI_ENUM( ELoadGameResult )
IMPLEMENT_RTTI_ENUM( ESaveListUpdateState )
IMPLEMENT_RTTI_ENUM( ESaveGameResult )

CUserProfileManager* GUserProfileManager = nullptr;

const String CUserProfileManager::ROOT_NODE( TXT( "redxml" ) );

const String CUserProfileManager::ACHIEVEMENTS_NODE( TXT( "achievements" ) );
const String CUserProfileManager::ACHIEVEMENT_NODE( TXT( "achievement" ) );

const String CUserProfileManager::PRESENCE_GROUP_NODE( TXT( "presencegroup" ) );
const String CUserProfileManager::PRESENCE_ITEM_NODE( TXT( "presence" ) );

const String CUserProfileManager::PLATFORM_NODE( TXT( "platform" ) );
const String CUserProfileManager::NAME_ATTRIBUTE( TXT( "name" ) );

// Sensible default for all platforms - ensure a minimum of 1 second passes before sending a new rich presence update
#define DEFAULT_MIN_TIME_BETWEEN_PRESENCE_UPDATES 1.0f

CUserProfileManager::CUserProfileManager()
:	m_disableInput( FLAG( CUserProfileManager::eAPDR_Startup ) )
,	m_minTimeBetweenRichPresenceUpdates( DEFAULT_MIN_TIME_BETWEEN_PRESENCE_UPDATES )
{

}

CUserProfileManager::~CUserProfileManager()
{

}

void CUserProfileManager::Update()
{
	SetUserPresence( m_queuedUserPresence );

	UpdateEventQueue();
}

Bool CUserProfileManager::LoadMap( CXMLReader& mapFile, const String& filepath )
{
	RED_FATAL_ASSERT( mapFile.IsReader(), "Need a file reader (not a writer), if you expect to be able to load in the achievements map" );
	CLEAR_XML_ERRORS( filepath.AsChar() );
	
	return ReadXMLNodeRoot( mapFile, filepath );
}

Bool CUserProfileManager::ReadXMLNodeRoot( CXMLReader& xmlReader, const String& filepath )
{
	Bool retval = false;

	if( xmlReader.BeginNode( ROOT_NODE ) )
	{
		retval = ReadXMLNodeGroup( xmlReader, filepath );

		xmlReader.EndNode();
	}
	else
	{
		ReportParsingError( filepath, TXT( "Missing redxml node" ) );
	}

	return retval;
}

Bool CUserProfileManager::ReadXMLNodeGroup( CXMLReader& xmlReader, const String& filepath )
{
	Bool retval = true;

	const Uint32 numGroups = xmlReader.GetChildCount();

	for( Uint32 i = 0; i < numGroups; ++i )
	{
		xmlReader.BeginNextNode();

		String groupName;
		if( xmlReader.GetNodeName( groupName ) )
		{
			const Uint32 numItemsInGroup = xmlReader.GetChildCount();

			if( groupName == ACHIEVEMENTS_NODE )
			{
				MapAchievementInit( numItemsInGroup );
				retval = ReadXMLNodeItems( xmlReader, ACHIEVEMENT_NODE, &CUserProfileManager::MapAchievement, filepath );
			}
			else if( groupName == PRESENCE_GROUP_NODE )
			{
				MapPresenceInit( numItemsInGroup );
				retval = ReadXMLNodeItems( xmlReader, PRESENCE_ITEM_NODE, &CUserProfileManager::MapPresence, filepath );
			}
			else
			{
				ReportParsingError( filepath, String::Printf( TXT( "Unrecognised node name: %ls" ), groupName.AsChar() ), true );
				retval = false;
			}
		}

		xmlReader.EndNode();
	}

	return retval;
}

Bool CUserProfileManager::ReadXMLNodeItems( CXMLReader& xmlReader, const String& itemNode, MapItemCallback callback, const String& filepath )
{
	Bool retval = true;

	const Uint32 numItems = xmlReader.GetChildCount();

	for( Uint32 i = 0; i < numItems; ++i )
	{
		retval &= ReadXMLNodeItem( xmlReader, itemNode, callback, filepath );
	}

	return retval;
}

Bool CUserProfileManager::ReadXMLNodeItem( CXMLReader& xmlReader, const String& nodeName, MapItemCallback callback, const String& filepath )
{
	Bool retval = true;

	if( xmlReader.BeginNode( nodeName ) )
	{
		const Uint32 numPlatforms = xmlReader.GetChildCount();

		String itemName;
		xmlReader.Attribute( NAME_ATTRIBUTE, itemName );

		for( Uint32 iPlatform = 0; iPlatform < numPlatforms; ++iPlatform )
		{
			xmlReader.BeginNode( PLATFORM_NODE );

			String platform;
			xmlReader.Attribute( NAME_ATTRIBUTE, platform );

			String value;
			xmlReader.Value( value );

			( this->*callback )( CName( itemName ), platform, value );

			xmlReader.EndNode();
		}

		xmlReader.EndNode();
	}
	else
	{
		ReportParsingError( filepath, TXT( "Missing map nodes" ), true );
		retval = false;
	}

	return retval;
}

void CUserProfileManager::ReportParsingError( const String& filepath, const String& error, Bool warning /* = false */ )
{
	XML_ERROR( filepath.AsChar(), error.AsChar() );
	if ( warning )
	{
		RED_LOG( CUserProfileManager, error.AsChar() );
	}
	else
	{
		RED_LOG_ERROR( CUserProfileManager, error.AsChar() );
	}
}

void CUserProfileManager::MapAchievementInit( Uint32 )
{

}

Bool CUserProfileManager::IsAchievementMapped( const CName& ) const
{
	return false;
}

void CUserProfileManager::MapAchievement( const CName&, const String&, const String& )
{

}

void CUserProfileManager::UnlockAchievement( const CName& )
{
}

void CUserProfileManager::LockAchievement( const CName& )
{
}

Bool CUserProfileManager::IsAchievementLocked( const CName& ) const
{
	return false;
}

void CUserProfileManager::GetAllAchievements( TDynArray< CName >& ) const
{
}

void CUserProfileManager::MapPresenceInit( Uint32 )
{

}

void CUserProfileManager::MapPresence( const CName&, const String&, const String& )
{

}

void CUserProfileManager::SetUserPresence( const CName& presenceName )
{
	//Clear this just in case DoUserPresence() does something funky
	m_queuedUserPresence = CName::NONE;

	if( presenceName != CName::NONE )
	{
		if( m_timeSinceLastPresenceUpdate.GetDelta() >= m_minTimeBetweenRichPresenceUpdates )
		{
			LOG_ENGINE( TXT( "Setting rich presence: %hs" ), presenceName.AsAnsiChar() );

			DoUserPresence( presenceName );
			m_timeSinceLastPresenceUpdate.Reset();
			m_queuedUserPresence = CName::NONE;
		}
		else
		{
			//LOG_ENGINE( TXT( "Too soon since last rich presence update (%.2lf), queuing update for later: %hs" ), m_timeSinceLastPresenceUpdate.GetDelta(), presenceName.AsAnsiChar() );

			m_queuedUserPresence = presenceName;
		}
	}
}

String CUserProfileManager::GetActiveUserDisplayName( Uint32 maxVisibleCharacters ) const
{
	String displayName = GetActiveUserDisplayNameRaw();

	// Includes Null terminator
	const Uint32 maxSize = maxVisibleCharacters + 1;

	if( displayName.GetLength() > maxVisibleCharacters )
	{
		RED_FATAL_ASSERT( maxVisibleCharacters > 3, "Too small a buffer" );

		displayName.ResizeFast( maxVisibleCharacters + 1 );

		for( Uint32 i = maxVisibleCharacters - 3; i < maxVisibleCharacters; ++i )
		{
			displayName[ i ] = TXT( '.' );
		}

		displayName[ maxVisibleCharacters ] = TXT( '\0' );
	}

	return displayName;
}

#ifdef RED_LOGGING_ENABLED
static Bool recursionCheck;
#endif

void CUserProfileManager::UpdateEventQueue()
{
#ifdef RED_LOGGING_ENABLED
	if ( recursionCheck )
	{
		ERR_ENGINE(TXT("UpdateEventQueue recursing!"));
	}
	Red::System::ScopedFlag< Bool > flag( recursionCheck = true, false );
#endif

	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_eventQueueMutex );

	for( EUserEvent event : m_eventQueue )
	{
		Events::CNotifier< EUserEvent >::Send( event );
	}

	m_eventQueue.ClearFast();
}

#ifdef RED_LOGGING_ENABLED
const Char* GetUserEventTxtForLog( EUserEvent event )
{
	switch (event)
	{
	case EUserEvent::UE_SignInStarted:			return TXT("UE_SignInStarted");
	case EUserEvent::UE_SignInCancelled:		return TXT("UE_SignInCancelled");
	case EUserEvent::UE_SignedIn:				return TXT("UE_SignedIn");
	case EUserEvent::UE_SignedOut:				return TXT("UE_SignedOut");
	case EUserEvent::UE_LoadSaveReady:			return TXT("UE_LoadSaveReady");
	case EUserEvent::UE_GameSaved:				return TXT("UE_GameSaved");
	case EUserEvent::UE_GameSaveFailed:			return TXT("UE_GameSaveFailed");
	case EUserEvent::UE_AccountPickerOpened:	return TXT("UE_AccountPickerOpened");
	case EUserEvent::UE_AccountPickerClosed:	return TXT("UE_AccountPickerClosed");
	default:
		return TXT("<Unknown>");
	}
}
#endif // RED_LOGGING_ENABLED

void CUserProfileManager::QueueEvent( EUserEvent event )
{
	LOG_ENGINE(TXT("CUserProfileManager::QueueEvent: %ls"), GetUserEventTxtForLog(event));

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_eventQueueMutex );

	m_eventQueue.PushBack( event );
}

void CUserProfileManager::ToggleInputProcessing( Bool enabled, EInputProcessingDisabledReason reason )
{
	if( enabled )
	{
		m_disableInput &= ~FLAG( reason );
	}
	else
	{
		m_disableInput |= FLAG( reason );
	}
}

void CUserProfileManager::SetStat( const String&, Int32 )
{
}

void CUserProfileManager::SetStat( const String&, Float )
{
}

void CUserProfileManager::SetAverageStat( const String&, Float, Double )
{
}

void CUserProfileManager::GetStat( const String&, Int32& )
{
}

void CUserProfileManager::GetStat( const String&, Float& )
{
}

void CUserProfileManager::IncrementStat( const String& name, Int32 value )
{
}

void CUserProfileManager::IncrementStat( const String& name, Float value )
{
}

Bool CUserProfileManager::GetSystemLanguageStrings( String& ) const
{
	return false;
}

Bool CUserProfileManager::GetSystemLanguageSpeech( String& ) const
{
	return false;
}

Uint32 CUserProfileManager::GetNumberOfWitcher2CloudSavePaths() const
{
	return 0;
}

String CUserProfileManager::GetWitcher2CloudSavePath( Uint32 ) const
{
	return String::EMPTY;
}

Bool IUserProfileSavegameInterface::GetSaveInSlot( ESaveGameType, Int16, SSavegameInfo& ) const
{
	return false;
}

String IUserProfileSavegameInterface::BuildFullDisplayNameForSave( const SSavegameInfo& info ) const
{
	if ( info.IsCustomFilename() )
	{
		return info.GetFileName();
	}

	// fallback impl
	return String::Printf
	(
		TXT("%ls - %02ld.%02ld.%ld %02ld:%02ld:%02ld"),
		info.GetDisplayName().AsChar(),
		info.m_timeStamp.GetDay() + 1,
		info.m_timeStamp.GetMonth() + 1,
		info.m_timeStamp.GetYear(),
		info.m_timeStamp.GetHour(),
		info.m_timeStamp.GetMinute(),
		info.m_timeStamp.GetSecond()
	);
}
