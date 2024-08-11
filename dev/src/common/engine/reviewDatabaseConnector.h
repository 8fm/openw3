/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include <time.h>
#include "../../../internal/MarkersSystemDB/MarkersSystemDB.h"

struct SVec3;
class CReviewFlag;
class CReviewSystem;
class CReviewFlagComment;
class CReviewDBConnection;

class CReviewDatabaseConnector
{
public:
	CReviewDatabaseConnector(void) { /*intentionally empty*/ };
	~CReviewDatabaseConnector(void) { /*intentionally empty*/ };

	// manage connection with database
	Bool Connect( String& errorMessage );
	void Disconnect();
	Bool IsConnected() const;
	
	// main operations on database
	Bool AddNewFlag	( CReviewFlag& newFlag, const String& ttProject );
	Bool ModifyFlag	( CReviewFlag& flag, CReviewFlagComment& comment );
	void Synchronize(TDynArray<CReviewFlag>& flags, const String& mapName, const String& lastUpdate);

	// 
	void GetAllFlags(TDynArray<CReviewFlag>& flags, const String& mapName, const String& ttProject, Bool downloadClosedFlags = false);
	void GetNewestComment(CReviewFlag& flag);
	void GetAllComments(CReviewFlag& flag);
	void GetFlagStates(TDynArray<String>& states);
	void GetFlagTypes(TDynArray<String>& types);

private:
	// translate database result to suitable format
	void ParseFlag(CReviewFlag& flag, SSelectedFlags* flagsFromDB, Uint32 currentFlag);
	void ParseComment(CReviewFlagComment& comment, SSelectedComments* commentsFromDB, Uint32 currentComment);
	void ProcessPassword( const String& passIn, String& passOut );

	CReviewDBConnection* m_connection;
};
#endif // NO_MARKER_SYSTEMS
