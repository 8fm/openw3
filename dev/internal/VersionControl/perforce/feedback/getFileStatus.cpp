/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "getFileStatus.h"

namespace VersionControl
{
	namespace Feedback
	{
		GetFileStatus::GetFileStatus()
		{

		}

		GetFileStatus::~GetFileStatus()
		{

		}

		void GetFileStatus::OutputStat( StrDict* varList )
		{
			// By virtue of reaching this function, we know it's in perforce
			m_status.SetFlag( VCSF_InDepot );

			// Find out if it's been marked for add or edit
			StrPtr* action = varList->GetVar( "action" );

			if( action )
			{
				if( *action == "edit" )
				{
					m_status.SetFlag( VCSF_CheckedOut );
				}
				else if( *action == "add" )
				{
					m_status.SetFlag( VCSF_Added );
					m_status.SetFlag( VCSF_CheckedOut );
				}
				else if( *action == "delete" )
				{
					m_status.SetFlag( VCSF_Deleted );
				}
			}

			// See if the local version is up to date
			m_status.SetFlag( VCSF_OutOfDate );

			StrPtr* haveRevision = varList->GetVar( "haveRev" );
			StrPtr* headRevision = varList->GetVar( "headRev" );

			if( haveRevision && headRevision )
			{
				if( *haveRevision == *headRevision )
				{
					m_status.UnSetFlag( VCSF_OutOfDate );
				}
			}

			// See if anyone else has this file checked out
			StrPtr* otherOpen = varList->GetVar( "otherOpen" );

			if( otherOpen )
			{
				int num = otherOpen->Atoi();
				if( num > 0 )
				{
					m_status.SetFlag( VCSF_CheckedOutByAnother );
				}
			}
		}
	}
}
