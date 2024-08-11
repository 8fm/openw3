/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "getDeletedFiles.h"

namespace VersionControl
{
	namespace Feedback
	{
		GetDeletedFiles::GetDeletedFiles( Filelist* list )
		:	m_list( list )
		{

		}

		GetDeletedFiles::~GetDeletedFiles()
		{

		}

		void GetDeletedFiles::OutputStat( StrDict* varList )
		{
			m_status.OutputStat( varList );

			if( m_status.Get().HasFlag( VCSF_Deleted ) )
			{
				StrPtr* localPath = varList->GetVar( "depotFile" );

				m_list->Add( localPath->Text() );
			}

			m_status = GetFileStatus();
		}
	}
}
