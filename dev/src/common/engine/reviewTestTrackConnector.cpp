/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "reviewTestTrackConnector.h"
#include "reviewSystem.h"
#include "bitmapTexture.h"
#include "entity.h"

#ifndef NO_MARKER_SYSTEMS

#if _MSC_VER == 1700
	#ifdef _DEBUG
		#ifdef _WIN64
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Debug/vc110/x64/TestTrackConnection_x64.lib" )
		#elif defined(W2_PLATFORM_WIN32)
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Debug/vc110/x86/TestTrackConnection_x86.lib" )
		#endif
		#endif
	#ifdef NDEBUG
		#ifdef _WIN64
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Release/vc110/x64/TestTrackConnection_x64.lib" )
		#elif defined(W2_PLATFORM_WIN32)
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Release/vc110/x86/TestTrackConnection_x86.lib" )
		#endif
	#endif
#elif _MSC_VER == 1600
	#ifdef _DEBUG
		#ifdef _WIN64
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Debug/TestTrackConnection_x64.lib" )
		#elif defined(W2_PLATFORM_WIN32)
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Debug/TestTrackConnection_x86.lib" )
		#endif
		#endif
	#ifdef NDEBUG
		#ifdef _WIN64
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Release/TestTrackConnection_x64.lib" )
		#elif defined(W2_PLATFORM_WIN32)
			#pragma comment ( lib, "../../../internal/TestTrackConnection/Release/TestTrackConnection_x86.lib" )
		#endif
	#endif
#else
#error Unsupported compiler
#endif


CReviewTestTrackConnector::CReviewTestTrackConnector()
{
	/* intentionally empty */
}

CReviewTestTrackConnector::~CReviewTestTrackConnector()
{
	m_ttpConnection.Logout();
}

Bool CReviewTestTrackConnector::Initialize( const String& defaultProjectMilestone, const String& defaultProjectName )
{
	m_defaultProjectMilestone = defaultProjectMilestone;
	m_defaultProjectName = defaultProjectName;

	Bool result = SwitchProject( m_defaultProjectName );
	if( result == false )
	{
		return false;
	}
	SetProjectMilestone( m_defaultProjectMilestone );

	return true;
}

Bool CReviewTestTrackConnector::AddNewFlag( CReviewFlag& newFlag )
{
	CReviewFlagComment& comment = newFlag.m_comments[0];

	String description = String::EMPTY;
	PrepareDescription( newFlag, comment, description );

	newFlag.m_testTrackNumber = m_ttpConnection.SendIssue( newFlag.m_summary.AsChar(), description.AsChar(), ConvertUsernameToTestTrackConvention( comment.m_author ).AsChar(),
		newFlag.m_type, comment.m_priority, m_defaultProjectMilestone.AsChar() );
	if( newFlag.m_testTrackNumber != 0 )
	{
		return true;
	}

	return false;
}

Bool CReviewTestTrackConnector::ModifyFlag( CReviewFlag& flag, CReviewFlagComment& comment )
{
	String description = String::EMPTY;
	PrepareDescription( flag, comment, description );

	Bool result = m_ttpConnection.ModifyIssue( flag.m_testTrackNumber, description.AsChar(), ConvertUsernameToTestTrackConvention(comment.m_author).AsChar(), 
		comment.m_state, comment.m_priority );

	return result;
}


Bool CReviewTestTrackConnector::AddComment( CReviewFlag& flag, CReviewFlagComment& comment )
{
	String description = String::EMPTY;
	PrepareDescription( flag, comment, description );

	Bool result = m_ttpConnection.AddCommentToIssue( flag.m_testTrackNumber, description.AsChar(), ConvertUsernameToTestTrackConvention(comment.m_author).AsChar(), comment.m_priority );

	return result;
}

String CReviewTestTrackConnector::ConvertUsernameToTestTrackConvention( const String& username)
{
	size_t dotPosition;
	if( username.FindCharacter(TXT('.'), dotPosition, false) == true )
	{
		String name = username.MidString(0, dotPosition);
		String surname = username.MidString(dotPosition+1, username.Size() - (name.Size() +  1));

		if(m_ttpConnection.UserExist(name.AsChar(), surname.AsChar()) == true)
		{
			return ConvertUsernameToTestTrackConvention( name, surname );
		}
	}
	return TXT("Reporter, Crash");	// default user
}

String CReviewTestTrackConnector::ConvertUsernameToTestTrackConvention( const String& name, const String& surname)
{
	return surname + TXT(", ") + name;
}

void CReviewTestTrackConnector::GetProjectList( TDynArray< String >& projects )
{
	projects.ClearFast();

	TTPConnection::CTextContainer projectList;
	m_ttpConnection.GetProjectList( projectList );

	Uint32 projectCount = projectList.GetCount();
	if( projectCount != 0 )
	{
		for( Uint32 i=0; i<projectCount; ++i )
		{
			projects.PushBack( projectList.GetText( i ) );
		}
	}
}

void CReviewTestTrackConnector::GetMilestoneList( TDynArray< String >& milestones )
{
	milestones.ClearFast();

	TTPConnection::CTextContainer miestoneList;
	m_ttpConnection.GetProjectMilestones( miestoneList );

	Uint32 projectCount = miestoneList.GetCount();
	if( projectCount != 0 )
	{
		for( Uint32 i=0; i<projectCount; ++i )
		{
			milestones.PushBack( miestoneList.GetText( i ) );
		}
	}
}

void CReviewTestTrackConnector::GetBugTypeList( TDynArray< String >& bugTypes )
{
	bugTypes.ClearFast();

	TTPConnection::CTextContainer typeList;
	m_ttpConnection.GetProjectBugTypes( typeList );

	Uint32 projectCount = typeList.GetCount();
	if( projectCount != 0 )
	{
		for( Uint32 i=0; i<projectCount; ++i )
		{
			bugTypes.PushBack( typeList.GetText( i ) );
		}
	}
}

void CReviewTestTrackConnector::GetPriorityList( TDynArray< String >& priorities )
{
	priorities.ClearFast();

	TTPConnection::CTextContainer priorityList;
	m_ttpConnection.GetProjectPriorities( priorityList );

	Uint32 projectCount = priorityList.GetCount();
	if( projectCount != 0 )
	{
		for( Uint32 i=0; i<projectCount; ++i )
		{
			priorities.PushBack( priorityList.GetText( i ) );
		}
	}
}

Bool CReviewTestTrackConnector::SwitchProject( const String& projectName )
{
	Bool result = m_ttpConnection.LoginToProject( projectName.AsChar() );
	m_defaultProjectName = projectName;

	return result;
}

void CReviewTestTrackConnector::SetProjectMilestone( const String& projectMilestone )
{
	m_defaultProjectMilestone = projectMilestone;
}

String CReviewTestTrackConnector::GetDefaultProjectName() const
{
	return m_defaultProjectName;
}

String CReviewTestTrackConnector::GetDefaultMilestoneName() const
{
	return m_defaultProjectMilestone;
}

Bool CReviewTestTrackConnector::ConnectToTTP( const String& user, const String& password, const String& project )
{
	return m_ttpConnection.LoginToProject( user.AsChar(), password.AsChar(), project.AsChar() );
}

void CReviewTestTrackConnector::PrepareDescription( const CReviewFlag& flag, const CReviewFlagComment &comment, String& description )
{
	description += comment.m_description;
	description += TXT("\n\n");
	description += TXT("\nPath to screen: ");
	description += comment.m_pathToScreen;
	description += TXT("\nActive world: ");
	description += flag.m_mapName;
	description += TXT("\n\n");
}

#endif	// NO_MARKER_SYSTEMS
