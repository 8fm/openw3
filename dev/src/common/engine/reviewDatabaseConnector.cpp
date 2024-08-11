/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "ReviewDatabaseConnector.h"
#include "reviewSystem.h"
#include "entity.h"
#include "bitmapTexture.h"
#include "../core/configFileManager.h"
#include "../core/feedback.h"
#include "commonConfigs.h"

#ifndef NO_MARKER_SYSTEMS
#ifdef _WIN64
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x64.lib" )
#elif defined(W2_PLATFORM_WIN32)
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x86.lib" )
#endif
#endif

namespace Config
{
	TConfigVar<Bool> cvUseWindowsAuthentication( "ReviewSystem", "UseWindowsAuthentication", true,	eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String> cvSQLUser( "ReviewSystem", "SQLUser", TXT(""),	eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String> cvSQLPass( "ReviewSystem", "SQLPass", TXT(""),	eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
}

#ifndef NO_MARKER_SYSTEMS
Bool CReviewDatabaseConnector::Connect( String& errorMessage )
{
	Disconnect();

	String databaseAddress = Config::cvDatabaseAddress.Get();
	String user, password, passEnd;
	if ( !Config::cvUseWindowsAuthentication.Get() )
	{
		user = Config::cvSQLUser.Get();
		password = Config::cvSQLPass.Get();
		ProcessPassword( password, passEnd );
	}
	
	SDBInitInfo info = ReviewDBInit( databaseAddress.AsChar(), user.AsChar(), passEnd.AsChar() );
	if ( info.m_info != NULL )
	{
		errorMessage = info.m_info;
	}
	m_connection = info.m_connection;
	ReviewDBFreeInfoErrorMessage( info );

	return (m_connection != NULL);
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::Disconnect()
{
	if( m_connection != NULL )
	{
		ReviewDBClose( m_connection );
		m_connection = NULL;
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CReviewDatabaseConnector::IsConnected() const
{
	return ( m_connection != NULL );
}
#endif


#ifndef NO_MARKER_SYSTEMS
Bool CReviewDatabaseConnector::AddNewFlag( CReviewFlag& newFlag, const String& ttProject )
{
	Uint32 newId;

	if( ReviewDBNextIDForFlag( m_connection, &newId ) < 0)
	{
		GFeedback->ShowError( TXT( "Unable to insert flag into database") );
		ERR_ENGINE( TXT( "Unable to insert flag into database" ));
		return false;
	}

	if( ReviewDBInsertFlag( m_connection, newId, newFlag.m_testTrackNumber, newFlag.m_type, newFlag.m_summary.AsChar(),
							newFlag.m_mapName.AsChar(), newFlag.m_linkToVideo.AsChar(), ttProject.AsChar() ) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to insert flag into database") );
		ERR_ENGINE( TXT( "Unable to insert flag into database" ) );
		return false;
	}

	newFlag.m_databaseId = newId;
	newFlag.m_comments[0].m_flagId = newId;

	if( ReviewDBInsertComment( m_connection, newFlag.m_comments[0].m_flagId, newFlag.m_comments[0].m_author.AsChar(), 
								newFlag.m_comments[0].m_description.AsChar(), newFlag.m_comments[0].m_priority,
								SVec3(newFlag.m_comments[0].m_cameraPosition.X, newFlag.m_comments[0].m_cameraPosition.Y, newFlag.m_comments[0].m_cameraPosition.Z),
								SVec3(newFlag.m_comments[0].m_cameraOrientation.X, newFlag.m_comments[0].m_cameraOrientation.Y, newFlag.m_comments[0].m_cameraOrientation.Z),
								SVec3(newFlag.m_comments[0].m_flagPosition.X, newFlag.m_comments[0].m_flagPosition.Y, newFlag.m_comments[0].m_flagPosition.Z), 
								newFlag.m_comments[0].m_pathToScreen.AsChar(), newFlag.m_comments[0].m_state ) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to insert comment into database") );
		ERR_ENGINE( TXT( "Unable to insert comment into database" ) );
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::GetAllFlags(TDynArray<CReviewFlag>& flags, const String& mapName, const String& ttProject, Bool downloadClosedFlags)
{
	SSelectedFlags* readResult = NULL;

	if(downloadClosedFlags == true)
	{
		readResult = ReviewDBSelectAllFlags(m_connection, mapName.AsChar(), ttProject.AsChar() );
	}
	else
	{
		readResult = ReviewDBSelectFlagsWithoutClosed(m_connection, mapName.AsChar(), ttProject.AsChar() );
	}

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_flagCount; ++i )
		{
			CReviewFlag newFlag;
			ParseFlag(newFlag, readResult, i);
			flags.PushBack(newFlag);
		}

		ReviewDBFreeSeletedFlags( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::GetFlagStates(TDynArray<String>& states)
{
	states.Clear();

	SSelectedStates* readResult = NULL;

	readResult = ReviewDBSelectFlagStates(m_connection);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_stateCount; ++i )
		{
			states.PushBack(readResult->m_state[i]);
		}

		ReviewDBFreeSelectFlagStates( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::GetFlagTypes(TDynArray<String>& types)
{
	types.Clear();

	SSelectedTypes* readResult = NULL;

	readResult = ReviewDBSelectFlagTypes(m_connection);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_typeCount; ++i )
		{
			types.PushBack(readResult->m_type[i]);
		}

		ReviewDBFreeSelectFlagTypes( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::Synchronize(TDynArray<CReviewFlag>& flags, const String& mapName, const String& lastUpdate)
{
	// check flag which must be update
	TDynArray<CReviewFlag*> flagsToUpdate;
	TDynArray<Uint32> flagsToDownload;

	SIndexFlagToUpdate* indexFlagToUpdate = NULL;
	indexFlagToUpdate = ReviewDBSelectIndexFlagToUpdate(m_connection, lastUpdate.AsChar());

	if(indexFlagToUpdate != NULL)
	{
		for(Uint32 i=0; i<indexFlagToUpdate->m_flagCount; ++i)
		{
			bool inTheFlags = false;
			for(Uint32 j=0; j<flags.Size(); ++j)
			{
				if(indexFlagToUpdate->m_ids[i] == flags[j].m_databaseId)
				{
					flagsToUpdate.PushBack(&flags[j]);
					inTheFlags = true;
					break;
				}
			}

			if(inTheFlags == false)
			{
				flagsToDownload.PushBack(indexFlagToUpdate->m_ids[i]);
			}
		}
		ReviewDBFreeSelectIndexFlagToUpdates( indexFlagToUpdate );
	}

	// update flag in actually container
	for(Uint32 i=0; i<flagsToUpdate.Size(); ++i)
	{
		SSelectedComments* newCommentsResult = ReviewDBSelectNewCommentsForUpdatedFlag(m_connection, flagsToUpdate[i]->m_databaseId, lastUpdate.AsChar());

		for(Uint32 j=0; j<newCommentsResult->m_commentCount; ++j)
		{
			CReviewFlagComment newComment;
			ParseComment(newComment, newCommentsResult, j);
			flagsToUpdate[i]->m_comments.PushBack(newComment);
		}

		ReviewDBFreeSelectedComments(newCommentsResult);
	}

	// add new flag to container
	for(Uint32 i=0; i<flagsToDownload.Size(); ++i)
	{
		SSelectedFlags* newFlagResult = ReviewDBSelectFlag(m_connection, flagsToDownload[i]);

		if(newFlagResult != NULL)
		{
			CReviewFlag newFlag;
			ParseFlag(newFlag, newFlagResult, 0);
			flags.PushBack(newFlag);
		}

		ReviewDBFreeSeletedFlags( newFlagResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CReviewDatabaseConnector::ModifyFlag( CReviewFlag& flag, CReviewFlagComment& comment)
{
	if( ReviewDBInsertComment( m_connection, comment.m_flagId, comment.m_author.AsChar(), comment.m_description.AsChar(), comment.m_priority,
								SVec3(comment.m_cameraPosition.X, comment.m_cameraPosition.Y, comment.m_cameraPosition.Z),
								SVec3(comment.m_cameraOrientation.X, comment.m_cameraOrientation.Y, comment.m_cameraOrientation.Z),
								SVec3(comment.m_flagPosition.X, comment.m_flagPosition.Y, comment.m_flagPosition.Z),
								comment.m_pathToScreen.AsChar(), comment.m_state) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to insert comment into database") );
		ERR_ENGINE( TXT( "Unable to insert comment into database" ));
		return false;
	}

	if( ReviewDBUpdateFlag( m_connection, flag.m_databaseId) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to update flag in database") );
		ERR_ENGINE( TXT( "Unable to update flag in database" ));
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::ParseFlag(CReviewFlag& flag, SSelectedFlags* flagsFromDB, Uint32 currentFlag)
{
	flag.m_databaseId = flagsFromDB->m_ids[currentFlag];
	flag.m_testTrackNumber = flagsFromDB->m_testTrackIds[currentFlag];
	flag.m_type = flagsFromDB->m_types[currentFlag];

	if ( flagsFromDB->m_summaries[currentFlag] != NULL )
	{
		flag.m_summary = String( flagsFromDB->m_summaries[currentFlag] );
	}
	if ( flagsFromDB->m_mapNames[currentFlag] != NULL )
	{
		flag.m_mapName = String( flagsFromDB->m_mapNames[currentFlag] );
	}

	if ( flagsFromDB->m_linksToVideos[currentFlag] != NULL )
	{
		flag.m_linkToVideo = String( flagsFromDB->m_linksToVideos[currentFlag] );
		flag.m_linkToVideo.Trim();
	}

	flag.m_lastUpdate.tm_year = (Int32)flagsFromDB->m_lastUpdate[currentFlag][0];
	flag.m_lastUpdate.tm_mon  = (Int32)flagsFromDB->m_lastUpdate[currentFlag][1];
	flag.m_lastUpdate.tm_mday = (Int32)flagsFromDB->m_lastUpdate[currentFlag][2];
	flag.m_lastUpdate.tm_hour = (Int32)flagsFromDB->m_lastUpdate[currentFlag][3];
	flag.m_lastUpdate.tm_min  = (Int32)flagsFromDB->m_lastUpdate[currentFlag][4];
	flag.m_lastUpdate.tm_sec  = (Int32)flagsFromDB->m_lastUpdate[currentFlag][5];
	flag.m_lastUpdate.tm_wday  = 0;
	flag.m_lastUpdate.tm_yday  = 0;
	flag.m_lastUpdate.tm_isdst = 0;

	// get comments
	SSelectedComments* readComment = NULL;
	readComment = ReviewDBSelectCommentForFlag(m_connection, flag.m_databaseId);

	if(readComment != NULL)
	{
		for(Uint32 i=0; i<readComment->m_commentCount; ++i)
		{
			CReviewFlagComment newComment;
			ParseComment(newComment, readComment, i);
			flag.m_comments.PushBack(newComment);
		}
	}

	ReviewDBFreeSelectedComments(readComment);
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CReviewDatabaseConnector::ParseComment(CReviewFlagComment& comment, SSelectedComments* commentsFromDB, Uint32 currentComment)
{
	comment.m_id = commentsFromDB->m_ids[currentComment];
	comment.m_flagId = commentsFromDB->m_flagId[currentComment];
	comment.m_author = String(commentsFromDB->m_author[currentComment]);
	comment.m_description = String(commentsFromDB->m_comment[currentComment]);
	comment.m_description.TrimRight();
	comment.m_priority = commentsFromDB->m_priority[currentComment];
	comment.m_cameraPosition = Vector((float)commentsFromDB->m_cameraPositions[currentComment].m_x, (float)commentsFromDB->m_cameraPositions[currentComment].m_y, (float)commentsFromDB->m_cameraPositions[currentComment].m_z);
	comment.m_cameraOrientation = Vector((float)commentsFromDB->m_cameraOrientations[currentComment].m_x, (float)commentsFromDB->m_cameraOrientations[currentComment].m_y, (float)commentsFromDB->m_cameraOrientations[currentComment].m_z);
	comment.m_flagPosition = Vector((float)commentsFromDB->m_flagPositions[currentComment].m_x, (float)commentsFromDB->m_flagPositions[currentComment].m_y, (float)commentsFromDB->m_flagPositions[currentComment].m_z);
	comment.m_pathToScreen = String( commentsFromDB->m_screen[currentComment] );

	comment.m_state = commentsFromDB->m_state[currentComment];

	comment.m_creationDate.tm_year = (Int32)commentsFromDB->m_creationDate[currentComment][0];
	comment.m_creationDate.tm_mon  = (Int32)commentsFromDB->m_creationDate[currentComment][1];
	comment.m_creationDate.tm_mday = (Int32)commentsFromDB->m_creationDate[currentComment][2];
	comment.m_creationDate.tm_hour = (Int32)commentsFromDB->m_creationDate[currentComment][3];
	comment.m_creationDate.tm_min  = (Int32)commentsFromDB->m_creationDate[currentComment][4];
	comment.m_creationDate.tm_sec  = (Int32)commentsFromDB->m_creationDate[currentComment][5];
	comment.m_creationDate.tm_wday  = 0;
	comment.m_creationDate.tm_yday  = 0;
	comment.m_creationDate.tm_isdst = 0;
}

void CReviewDatabaseConnector::ProcessPassword( const String& passIn, String& passOut )
{
	String keys = TXT("xru");

	passOut = passIn;
	Uint32 keysNum = keys.Size();
	for ( Uint32 i = 0; i < passIn.GetLength(); ++i )
	{
		Char c = passIn[i];
		Char res = ( 128 + passIn[i] - keys[ i % keysNum ] ) % 128;
		passOut[i] = res;
	}
}
#endif
