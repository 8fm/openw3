/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_LOG

#define COMMUNITY_LOG( format, ... )	RED_LOG( Community, format, ## __VA_ARGS__ )
#define COMMUNITY_WARN( format, ... )	RED_LOG( Community, format, ## __VA_ARGS__ )
#define COMMUNITY_ERROR( format, ... )	RED_LOG( Community, format, ## __VA_ARGS__ )

#else

#define COMMUNITY_LOG( format, ... )	
#define COMMUNITY_WARN( format, ... )	
#define COMMUNITY_ERROR( format, ... )	

#endif


class CErrorReportSystem
{
public:
	CErrorReportSystem()
		: m_isLogEnabledStub( false )
		, m_isLogEnabledAP( false )
		, m_isLogEnabledSpawn( false )
		, m_isLogEnabledDebug( false ) 
		, m_isLogEnabledStoryPhase( false )
	{}

	~CErrorReportSystem() {}

	void BadData( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation );
	void BadCode( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation );
	void InfoMsg( const Char *msg, const SStoryPhaseLocation &storyPhaseLocation );

	void LogSpawn( const String &msg ); // Log spawn problem, why NPC cannot spawn
	void LogSpawn( const String &msg, const SStoryPhaseLocation &storyPhaseLocation );
	void LogActionPoint( const String &msg ); // Log action point problem, why cannot find AP
	void LogActionPoint( const String &msg, const SStoryPhaseLocation &storyPhaseLocation );
	void LogStub( const String &msg ); // Log agents stub problem
	void LogStub( const String &msg, const SStoryPhaseLocation &storyPhaseLocation );
	void LogStoryPhaseChange( const String &msg );

	// Enables/disables log
	void SwitchLogSpawn( Bool enable ) { m_isLogEnabledSpawn = enable; }
	void SwitchLogActionPoint( Bool enable ) { m_isLogEnabledAP = enable; }
	void SwitchLogStub( Bool enable ) { m_isLogEnabledStub = enable; }
	void SwitchLogDebug( Bool enable ) { m_isLogEnabledDebug = enable; }
	void SwitchLogStoryPhase( Bool enable ) { m_isLogEnabledStoryPhase = enable; }
	Bool IsLogEnabledSpawn() const { return m_isLogEnabledSpawn; }
	Bool IsLogEnabledActionPoint() const { return m_isLogEnabledAP; }
	Bool IsLogEnabledStub() const { return m_isLogEnabledStub; }
	Bool IsLogEnabledStoryPhase() const { return m_isLogEnabledStoryPhase; }
	Bool IsLogEnabledDebug() const { return m_isLogEnabledDebug; }
private:
	// returns friendly debug info base on story phase location
	String GetInfoFromStoryPhaseLocation( const SStoryPhaseLocation &storyPhaseLocation );
	Bool m_isLogEnabledSpawn;
	Bool m_isLogEnabledAP;
	Bool m_isLogEnabledStub;
	Bool m_isLogEnabledDebug;
	Bool m_isLogEnabledStoryPhase;
};

typedef TSingleton< CErrorReportSystem > SCSErrRep; // Community System Error Report
