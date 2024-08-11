/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storyPhaseLocation.h"
#include "communityErrorReport.h"

RED_DEFINE_STATIC_NAME( CommunityStoryPhase );

void CErrorReportSystem::BadData( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledDebug )
	{
		String formattedMsg;
		formattedMsg = TXT("CS Bad Data: ") + String(msg) + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation );
		COMMUNITY_ERROR( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::BadCode( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledDebug )
	{
		String formattedMsg;
		formattedMsg = TXT("CS Bad Code: ") + String(msg) + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation );
		COMMUNITY_ERROR( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::InfoMsg( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledDebug )
	{
		String formattedMsg;
		formattedMsg = TXT("CS Info: ") + String(msg) + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation );
		COMMUNITY_LOG( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::LogSpawn( const String &msg )
{
	if ( m_isLogEnabledSpawn )
	{
		String formattedMsg;
		formattedMsg = TXT("CS spawn problem: ") + msg;
		COMMUNITY_WARN( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::LogSpawn( const String &msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledSpawn )
	{
		LogSpawn( msg + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation ) );
	}
}

void CErrorReportSystem::LogActionPoint( const String &msg )
{
	if ( m_isLogEnabledAP )
	{
		String formattedMsg;
		formattedMsg = TXT("CS AP problem: ") + msg;
		COMMUNITY_WARN( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::LogActionPoint( const String &msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledAP )
	{
		LogActionPoint( msg + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation ) );
	}
}

void CErrorReportSystem::LogStub( const String &msg )
{
	if ( m_isLogEnabledStub )
	{
		String formattedMsg;
		formattedMsg = TXT("CS Agent Stub problem: ") + msg;
		COMMUNITY_WARN( formattedMsg.AsChar() );
	}
}

void CErrorReportSystem::LogStub( const String &msg, const SStoryPhaseLocation &storyPhaseLocation )
{
	if ( m_isLogEnabledStub )
	{
		LogStub( msg + TXT("\n") + GetInfoFromStoryPhaseLocation( storyPhaseLocation ) );
	}
}

void CErrorReportSystem::LogStoryPhaseChange( const String &msg )
{
	if ( m_isLogEnabledStoryPhase )
	{
		String formattedMsg;
		formattedMsg = TXT("CS StoryPhase: ") + msg;
		RED_LOG( CommunityStoryPhase, formattedMsg.AsChar() );
	}
}

String CErrorReportSystem::GetInfoFromStoryPhaseLocation( const SStoryPhaseLocation &storyPhaseLocation )
{
	String result( TXT("") );

	if ( storyPhaseLocation.m_community )
	{
		result += TXT("Community: ") + storyPhaseLocation.m_community->GetFriendlyName() + TXT("\n");
	}

	if ( storyPhaseLocation.m_communityEntry )
	{
		result += TXT("Community entry comment: ") + storyPhaseLocation.m_communityEntry->m_comment
			+ TXT(" entry ID: ") + storyPhaseLocation.m_communityEntry->m_entryID + TXT("\n");
	}
	if ( storyPhaseLocation.m_communityStoryPhaseEntry )
	{
		result += TXT("Story phase entry comment: ") + storyPhaseLocation.m_communityStoryPhaseEntry->m_comment + TXT("\n");
	}

	return result;
}
