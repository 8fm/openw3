/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "TestTrackConnection.h"
#include "TTSOAPCGI\ttsoapcgi.nsmap"

namespace TTPConnection
{

//////////////////////////////////////////////////////////////////////////
//
CTextContainer::CTextContainer()
	: m_count( 0 )
	, m_array( nullptr )
{
	/* intentionally empty */
}

CTextContainer::~CTextContainer()
{
	FreeMemory();
}

Uint CTextContainer::GetCount() const
{
	return m_count;
}

Char* CTextContainer::GetText( Uint index )
{
	if( index < m_count )
	{
		return m_array[index];
	}
	return nullptr;
}

void CTextContainer::FreeMemory()
{
	for( Uint i=0; i<m_count; ++i )
	{
		delete m_array[i];
	}
	delete[] m_array;
}

//////////////////////////////////////////////////////////////////////////
//
namespace
{
	const char* GDefaultUser = "crash.reporter";
	const char* GDefaultPassword = "GPtu04quE9W0";
}

CTestTrackConnection::CTestTrackConnection() 
	: m_username( new char[strlen( GDefaultUser ) + 1] )
	, m_password( new char[strlen( GDefaultPassword ) + 1] )
{
	strcpy_s( m_username.get(), strlen( GDefaultUser ) + 1, GDefaultUser );
	strcpy_s( m_password.get(), strlen( GDefaultPassword ) + 1, GDefaultPassword );
}

CTestTrackConnection::~CTestTrackConnection()
{
	Logout();
}

charPtr CTestTrackConnection::ToAscii( CChar* text )
{
	charPtr ascii( new char[wcslen( text ) + 1] );
	wcstombs_s( nullptr, ascii.get(), wcslen( text ) + 1, text, wcslen( text ) + 1 );

	return ascii;
}

bool CTestTrackConnection::UserExist( CChar* name, CChar* surname )
{
	charPtr localName = ToAscii( name );
	charPtr localSurname = ToAscii( surname );

	ns1__getUserResponse user;
	m_ttEngine.getUser( m_session, localName.get(), "", localSurname.get(), user );
	if( user.pUser != nullptr )
	{
		return true;
	}

	return false;
}

Uint CTestTrackConnection::SendIssue( CChar* summary, CChar* description, CChar* author, Uint type, Uint priority, CChar* projectState )
{
	if( m_session == 0 )
	{
		return 0;
	}

	__int64 issueId = 0;
	int result = 0;

	charPtr localSummary = ToAscii( summary );
	charPtr localDescription = ToAscii( description );
	charPtr localAuthor = ToAscii( author );
	charPtr localProjectState = ToAscii( projectState );

	if( CreateNewIssue( localSummary, localDescription, localAuthor, type, priority, localProjectState, issueId ) == false )
	{
		return 0;
	}

	return static_cast<Uint>( issueId );
}

bool CTestTrackConnection::ModifyIssue( Uint issueNumber, CChar* description, CChar* author, Uint state, Uint priority )
{
	charPtr localDescription = ToAscii( description );
	charPtr localAuthor = ToAscii( author );

	ns1__editDefectResponse defectByRecord;
	if( m_ttEngine.editDefect( m_session, issueNumber, "", false, defectByRecord ) != SOAP_OK )
	{
		return false;
	}
	ns1__CDefect* issue = defectByRecord.pDefect;
	SetDefaultValues( *issue );

	// we can't modify custom fields
	issue->customFieldList = nullptr;

	// reported by
	ns1__CReportedByRecord** reportedByPointer = new ns1__CReportedByRecord*[issue->reportedbylist->__size+1];
	for( int i=0; i<issue->reportedbylist->__size; ++i )
	{
		reportedByPointer[i] = issue->reportedbylist->__ptritem[i];
	}

	ns1__CReportedByRecord newReportBy;
	SetDefaultValues ( newReportBy, localAuthor );
	newReportBy.comments = localDescription.get();
	reportedByPointer[issue->reportedbylist->__size] = &newReportBy;

	issue->reportedbylist->__size = issue->reportedbylist->__size+1;
	issue->reportedbylist->__ptritem = reportedByPointer;

	// priority
	CTextContainer priorityList;
	GetProjectPriorities( priorityList );
	if( priority >= priorityList.GetCount() )
	{
		return false;
	}
	charPtr localPriority = ToAscii( priorityList.GetText( priority ) );
	issue->priority = localPriority.get();

	// state
	AddNewEvent( *issue, state, localAuthor );

	// send modification
	int newNum = 0;
	int result = m_ttEngine.saveDefect( m_session, issue, newNum );
	if( result != SOAP_OK )
	{
		int result = m_ttEngine.cancelSaveDefect( m_session, issueNumber, newNum );
		return false;
	}

	return true;
}

bool CTestTrackConnection::AddCommentToIssue( Uint issueNumber, CChar* description, CChar* author, Uint priority )
{
	charPtr localDescription = ToAscii( description );
	charPtr localAuthor = ToAscii( author );

	ns1__editDefectResponse defectByRecord;
	if( m_ttEngine.editDefect( m_session, issueNumber, "", false, defectByRecord ) != SOAP_OK )
	{
		return false;
	}
	ns1__CDefect* issue = defectByRecord.pDefect;
	SetDefaultValues( *issue );

	// we can't modify custom fields
	issue->customFieldList = nullptr;

// 	// reported by
// 	ns1__CReportedByRecord** reportedByPointer = new ns1__CReportedByRecord*[issue->reportedbylist->__size+1];
// 	for( int i=0; i<issue->reportedbylist->__size; ++i )
// 	{
// 		reportedByPointer[i] = issue->reportedbylist->__ptritem[i];
// 	}
// 
// 	ns1__CReportedByRecord newReportBy;
// 	SetDefaultValues ( newReportBy, ToAscii( author ) );
// 	newReportBy.comments = ToAscii( description );
// 	reportedByPointer[issue->reportedbylist->__size] = &newReportBy;
// 
// 	issue->reportedbylist->__size = issue->reportedbylist->__size+1;
// 	issue->reportedbylist->__ptritem = reportedByPointer;

	// priority
	CTextContainer priorityList;
	GetProjectPriorities( priorityList );
	if( priority >= priorityList.GetCount() )
	{
		return false;
	}
	charPtr localPriority = ToAscii( priorityList.GetText( priority ) );
	issue->priority = localPriority.get();

	// state
	AddNewCommentEvent( *issue, localAuthor, localDescription );

	// send modification
	int newNum = 0;
	int result = m_ttEngine.saveDefect( m_session, issue, newNum );
	if( result != SOAP_OK )
	{
		int result = m_ttEngine.cancelSaveDefect( m_session, issueNumber, newNum );
		return false;
	}

	return true;
}

bool CTestTrackConnection::LoginToProject( CChar* projectName )
{
	Logout();

	if( m_session == 0 )
	{
		ns1__getProjectListResponse projects;
		if( m_ttEngine.getProjectList( m_username.get(), m_password.get(), projects ) != SOAP_OK )
		{
			return false;
		}

		charPtr localProjectName = ToAscii( projectName );
		for(int i=0; i<projects.pProjList->__size; ++i)
		{
			if( strcmp( projects.pProjList->__ptritem[i]->database->name, localProjectName.get() ) == 0 )
			{
				m_project = projects.pProjList->__ptritem[i];
				break;
			}
		}

		if( m_ttEngine.ProjectLogon( m_project, m_username.get(), m_password.get(), m_session) != SOAP_OK )
		{
			return false;
		}
	}

	return (m_session != 0);
}

bool CTestTrackConnection::LoginToProject( CChar* user, CChar* password, CChar* projectName )
{
	Logout();

	if( m_session == 0 )
	{
		charPtr tempUser = ToAscii( user );
		charPtr tempPassword = ToAscii( password );

		ns1__getProjectListResponse projects;
		if( m_ttEngine.getProjectList( tempUser.get(), tempPassword.get(), projects ) != SOAP_OK )
		{
			return false;
		}

		charPtr localProjectName = ToAscii( projectName );
		for(int i=0; i<projects.pProjList->__size; ++i)
		{
			if( strcmp( projects.pProjList->__ptritem[i]->database->name, localProjectName.get() ) == 0 )
			{
				m_project = projects.pProjList->__ptritem[i];
				break;
			}
		}

		if( m_ttEngine.ProjectLogon( m_project, tempUser.get(), tempPassword.get(), m_session) != SOAP_OK )
		{
			return false;
		}

		m_username = tempUser;
		m_password = tempPassword;
	}

	return (m_session != 0);
}

void CTestTrackConnection::Logout()
{
	if( m_session != 0 )
	{
		int result = 0;
		m_ttEngine.DatabaseLogoff( m_session, result );
		m_session = 0;
	}
}

bool CTestTrackConnection::CreateNewIssue( charPtr summary, charPtr description, charPtr author, Uint type, Uint priority, charPtr projectState, __int64& issueId )
{
	ns1__CDefect issue;
	SetDefaultValues( issue );

	issue.state = "Open";
	issue.summary = summary.get();
	issue.disposition = projectState.get();
	
	// priority
	CTextContainer priorityList;
	GetProjectPriorities( priorityList );
	if( priority >= priorityList.GetCount() )
	{
		return false;
	}
	charPtr localPriority = ToAscii( priorityList.GetText( priority ) );
	issue.priority = localPriority.get();

	// type
	CTextContainer bugTypeList;
	GetProjectBugTypes( bugTypeList );
	if( type >= bugTypeList.GetCount() )
	{
		return false;
	}
	charPtr localBugType = ToAscii( bugTypeList.GetText( type ) );
	issue.product = localBugType.get();

	// username
	issue.createdbyuser = author.get();
	issue.enteredby = author.get();
	issue.modifiedbyuser = author.get();

	// Custom fields - doesn't work  - I don't know why...
	//ns1__getDefectCustomFieldsDefinitionListResponse customFields;
	//if( m_ttEngine.getDefectCustomFieldsDefinitionList(m_session, customFields) != SOAP_OK )
	//{
	//	return false;
	//}

	//for( int i=0; i<customFields.customFields->__size; ++i)
	//{
	//	if( strcmp(customFields.customFields->__ptritem[i]->name, "Team") == 0 )
	//	{
	//		ns1__CDropdownField* team = reinterpret_cast<ns1__CDropdownField*>(customFields.customFields->__ptritem[i]);
	//		team->value = "QA";
	//		continue;
	//	}
	//	if( strcmp(customFields.customFields->__ptritem[i]->name, "Requested By") == 0 )
	//	{
	//		ns1__CMultiSelectDropdownField* requestBy = reinterpret_cast<ns1__CMultiSelectDropdownField*>(customFields.customFields->__ptritem[i]);

	//		ns1__CFieldValue fieldValue;
	//		fieldValue.value = author;

	//		ns1__CFieldValue* values[1];
	//		values[0] = &fieldValue;

	//		ArrayOfCFieldValue valueArray;
	//		valueArray.__size = 1;
	//		valueArray.__ptritem = values;
	//		requestBy->dropdownValues = &valueArray;
	//        continue;
	//	}
	//}
	//issue.customFieldList = customFields.customFields;
	
	// Reported by
	ns1__CReportedByRecord reportBy;
	SetDefaultValues ( reportBy, author );
	reportBy.comments = description.get(); 

	ns1__CReportedByRecord* reportedByPointer[1];
	reportedByPointer[0] = &reportBy;

	ArrayOfCReportedByRecord reportedByList;
	reportedByList.__size = 1;
	reportedByList.__ptritem = reportedByPointer;

	issue.reportedbylist = &reportedByList;

	if( m_ttEngine.addDefect( m_session, &issue, issueId ) != SOAP_OK )
	{
		return false;
	}

	return true;
}

void CTestTrackConnection::SetDefaultValues( ns1__CDefect& issue )
{
	issue.type = "Bug";						// required
	issue.component = "N\\A";				// required - in TTP 'Bug Frequency'
}

void CTestTrackConnection::SetDefaultValues( ns1__CReportedByRecord& report, charPtr userName )
{
	report.foundby = userName.get();// optional
	report.recordid = 0;			// required
	report.showorder = 1;			// required
}

void CTestTrackConnection::AddNewEvent( ns1__CDefect& issue, Uint state, charPtr author )
{
	ns1__CEvent* newEvent = nullptr;

	// state
	switch( state )
	{
	case 1:	// Open
		{
			newEvent = new ns1__CEvent();
			newEvent->name = "Open";
			newEvent->resultingstate = "Open";
			newEvent->notes = "Open from ReviewFlag system";
		}
		break;
	case 2:	// Fix
		{
			newEvent = new ns1__CEvent();
			newEvent->name = "Fix";
			newEvent->resultingstate = "Fixed";
			newEvent->notes = "Fixed from ReviewFlag system";
		}
		break;
	case 3:	// Close
		{
			newEvent = new ns1__CEvent();
			newEvent->name = "Force Close";
			newEvent->resultingstate = "Closed";
			newEvent->notes = "Closed from ReviewFlag system";
		}
		break;
	case 4:	// Re-Opened
		{
			newEvent = new ns1__CEvent();
			newEvent->name = "Re-Open";
			newEvent->resultingstate = "Open (Re-Opened)";
			newEvent->notes = "Reopen from ReviewFlag system";
		}
		break;
	}

	if( newEvent != nullptr )
	{
		newEvent->generatedbyname = author.get();
		newEvent->generatedeventtype = "User";
		newEvent->user = author.get();
		time( &newEvent->date );
		newEvent->eventaddorder = 0;

		// Add the event to defect's eventlist.
		if( issue.eventlist == nullptr || issue.eventlist->__size == 0 )
		{  
			// No events, so we can just create a new list.
			ns1__CEvent** events = new ns1__CEvent*[1];
			events[0] = newEvent;

			issue.eventlist = new ArrayOfCEvent();
			issue.eventlist->__size = 1;
			issue.eventlist->__ptritem = events;
		}
		else
		{  
			// Append new event to end of existing list.
			ns1__CEvent** events = reinterpret_cast< ns1__CEvent ** >( soap_malloc( &m_ttEngine, sizeof( ns1__CEvent* )*( issue.eventlist->__size + 1 ) ) );
			for(int i=0; i<issue.eventlist->__size; ++i)
			{
				events[i] = new ns1__CEvent();
				CopyEvent( issue.eventlist->__ptritem[i], events[i] );
			}
			events[issue.eventlist->__size] = newEvent;

			issue.eventlist->__size = issue.eventlist->__size + 1;
			issue.eventlist->__ptritem = events;
		}
	}
}

void CTestTrackConnection::AddNewCommentEvent( ns1__CDefect& issue, charPtr author, charPtr comment )
{
	ns1__CEvent* newEvent = nullptr;

	newEvent = new ns1__CEvent();
	newEvent->name = "Comment";
	newEvent->notes = comment.get();

	if( newEvent != nullptr )
	{
		newEvent->generatedbyname = author.get();
		newEvent->generatedeventtype = "User";
		newEvent->user = author.get();
		time( &newEvent->date );
		newEvent->eventaddorder = 0;

		// Append new event to end of existing list.
		ns1__CEvent** events = reinterpret_cast< ns1__CEvent ** >( soap_malloc( &m_ttEngine, sizeof( ns1__CEvent* )*( issue.eventlist->__size + 1 ) ) );
		for(int i=0; i<issue.eventlist->__size; ++i)
		{
			events[i] = new ns1__CEvent();
			events[i]->notes = issue.eventlist->__ptritem[i]->notes;
			CopyEvent( issue.eventlist->__ptritem[i], events[i] );
		}
		events[issue.eventlist->__size] = newEvent;

		issue.eventlist->__size = issue.eventlist->__size + 1;
		issue.eventlist->__ptritem = events;
	}
}

void CTestTrackConnection::CopyEvent( ns1__CEvent* src, ns1__CEvent* dst )
{
	dst->date = src->date;
	dst->eventaddorder = src->eventaddorder;
	dst->generatedbyname = src->generatedbyname;
	dst->generatedeventtype = src->generatedeventtype;
	dst->recordid = src->recordid;
	dst->user = src->user;
	dst->name = src->name;
}

void CTestTrackConnection::GetProjectList( CTextContainer& textContainer )
{
	ns1__getProjectListResponse projects;
	if( m_ttEngine.getProjectList( m_username.get(), m_password.get(), projects) != SOAP_OK )
	{
		return;
	}

	int projectCount = projects.pProjList->__size;
	textContainer.m_count = projectCount;

	textContainer.m_array = new CharPtr[projectCount];

	for(int i=0; i<projectCount; ++i)
	{
		Uint textLength = (Uint)strlen( projects.pProjList->__ptritem[i]->database->name );
		textContainer.m_array[i] = new Char[ textLength + 1];
		int needed = MultiByteToWideChar( 0, 0, projects.pProjList->__ptritem[i]->database->name, textLength + 1, textContainer.m_array[i], textLength + 1 );
	}

}

void CTestTrackConnection::GetProjectPriorities( CTextContainer& textContainer )
{
	GetFieldValues( textContainer, "Priority" );
}

void CTestTrackConnection::GetProjectMilestones( CTextContainer& textContainer )
{
	GetFieldValues( textContainer, "Milestone" );
}

void CTestTrackConnection::GetProjectBugTypes( CTextContainer& textContainer )
{
	GetFieldValues( textContainer, "Bug Category" );
}

void CTestTrackConnection::GetFieldValues( CTextContainer& textContainer, char* fieldName )
{
	if( m_session == 0 )
	{
		return;
	}

	ns1__getDropdownFieldValuesForTableResponse response;
	m_ttEngine.getDropdownFieldValuesForTable( m_session, "Defect", fieldName, response );
	if( response.pValueList != nullptr )
	{
		Uint count = response.pValueList->__size;
		textContainer.m_count = count;

		textContainer.m_array = new CharPtr[count];

		ns1__CFieldValue** fieldArray = response.pValueList->__ptritem;
		for( Uint i=0; i<count; ++i )
		{
			Uint textLength = (Uint)strlen( fieldArray[i]->value );
			textContainer.m_array[i] = new Char[ textLength + 1];
			int needed = MultiByteToWideChar( 0, 0, fieldArray[i]->value, textLength + 1, textContainer.m_array[i], textLength + 1 );
		}
	}
}

}	// namespace TTPConnection
