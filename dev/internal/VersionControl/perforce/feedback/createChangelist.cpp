/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "createChangelist.h"

namespace VersionControl
{
	namespace Feedback
	{
		CreateChangelist::CreateChangelist()
		:	m_description( "<Created by RedP4>" )
		{

		}

		CreateChangelist::~CreateChangelist()
		{

		}

		void CreateChangelist::AddFile( StrPtr* file )
		{
			m_files.Append( "\t" );
			m_files.Append( file );
			m_files.Append( "\n" );
		}

		void CreateChangelist::AddFile( const char* file )
		{
			m_files.Append( "\t" );
			m_files.Append( file );
			m_files.Append( "\n" );
		}

		void CreateChangelist::SetDescription( const char* description )
		{
			m_description.Set( description );
		}

		void CreateChangelist::InputData( StrBuf* strbuf, ::Error* )
		{
			strbuf->Set( "Change: new\nStatus: new\nDescription: " );
			strbuf->Append( &m_description );
			strbuf->Append( "\n" );

			if( m_files.Length() > 0 )
			{
				strbuf->Append( "Files: \n" );
				strbuf->Append( &m_files );
			}
		}
	}
}
