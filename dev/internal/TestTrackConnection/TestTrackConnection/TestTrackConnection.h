/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "TTSOAPCGI\soapH.h"
#include "TTSOAPCGI\soapttsoapcgiProxy.h"

#include <memory>
using std::shared_ptr;

namespace TTPConnection
{

	typedef unsigned int Uint;
	typedef wchar_t Char;
	typedef const Char CChar;
	typedef Char* CharPtr;
	typedef CharPtr* CharArray;

	typedef shared_ptr< char > charPtr;

	class CTestTrackConnection;

	class CTextContainer
	{
		friend class CTestTrackConnection;

	public:
		CTextContainer();
		~CTextContainer();

		Uint GetCount() const;
		Char* GetText( Uint index );

	private:
		void FreeMemory();

	private:
		Uint		m_count;
		CharArray	m_array;
	};

	class CTestTrackConnection
	{
	public:
		CTestTrackConnection();
		~CTestTrackConnection();

		bool LoginToProject( CChar* projectName );
		bool LoginToProject( CChar* user, CChar* password, CChar* projectName );
		void Logout();

		Uint SendIssue( CChar* summary, CChar* descriptioin, CChar* author, Uint type, Uint priority, CChar* projectState );
		bool ModifyIssue( Uint issueNumber, CChar* description, CChar* author, Uint state, Uint priority );
		bool AddCommentToIssue( Uint issueNumber, CChar* description, CChar* author, Uint priority );

		bool UserExist( CChar* name, CChar* surname );

		void GetProjectList( CTextContainer& textContainer );
		void GetProjectPriorities( CTextContainer& textContainer );
		void GetProjectMilestones( CTextContainer& textContainer );
		void GetProjectBugTypes( CTextContainer& textContainer );

	private:
		charPtr ToAscii( CChar* text);
		bool CreateNewIssue( charPtr summary, charPtr descriptioin, charPtr author, Uint type, Uint priority, charPtr projectState, __int64& issueId);
		void SetDefaultValues( ns1__CDefect& issue );
		void SetDefaultValues( ns1__CReportedByRecord& report, charPtr userName );
		void AddNewEvent( ns1__CDefect& issue, Uint state, charPtr author);
		void AddNewCommentEvent( ns1__CDefect& issue, charPtr author, charPtr comment );
		void CopyEvent( ns1__CEvent* src, ns1__CEvent* dst );

		void GetFieldValues( CTextContainer& textContainer, char* fieldName );

	private:
		charPtr			m_username;
		charPtr			m_password;

		LONG64			m_session;
		ns1__CProject*	m_project;
		ttsoapcgiProxy	m_ttEngine;
	};

}
