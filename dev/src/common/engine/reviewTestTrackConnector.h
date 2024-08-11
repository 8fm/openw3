/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#pragma warning( push )
#pragma warning( disable:4530 ) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#include "../../../internal/TestTrackConnection/TestTrackConnection/TestTrackConnection.h"
#pragma warning( pop )

class CReviewFlag;
class CReviewSystem;
class CReviewFlagComment;
class CReviewDBConnection;

class CReviewTestTrackConnector
{	
public:
	CReviewTestTrackConnector();
	~CReviewTestTrackConnector();

	Bool Initialize( const String& defaultProjectMilestone, const String& defaultProjectName );

	Bool SwitchProject( const String& projectName );
	void SetProjectMilestone( const String& projectMilestone );
	Bool ConnectToTTP( const String& user, const String& password, const String& project );

	Bool AddNewFlag( CReviewFlag& newFlag );
	Bool ModifyFlag( CReviewFlag& flag, CReviewFlagComment& comment );
	Bool AddComment( CReviewFlag& flag, CReviewFlagComment& comment );

	void GetProjectList( TDynArray< String >& projects );
	void GetMilestoneList( TDynArray< String >& milestones );
	void GetBugTypeList( TDynArray< String >& projects );
	void GetPriorityList( TDynArray< String >& priorities );

	String GetDefaultProjectName() const;
	String GetDefaultMilestoneName() const;

private:
	String ConvertUsernameToTestTrackConvention( const String& username );
	String ConvertUsernameToTestTrackConvention( const String& name, const String& surname );

	void PrepareDescription( const CReviewFlag& flag, const CReviewFlagComment &comment, String& description );

private:
	String m_defaultProjectMilestone;
	String m_defaultProjectName;

	TTPConnection::CTestTrackConnection m_ttpConnection;
};

#endif // NO_MARKER_SYSTEMS
