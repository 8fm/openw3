/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resourcePath.h"

void CResourcePath::SetPath( const Char* path )
{
	if ( path && *path )
	{
		// Convert to ANSI
		m_path.Resize( Red::System::StringLength( path ) + 1 );
		Red::System::MemoryCopy( m_path.Data(), UNICODE_TO_ANSI( path ), m_path.Size() );

		// Convert case and slashes
		AnsiChar* ch = m_path.TypedData();
		for ( Uint32 i=0; i<m_path.Size(); i++, ch++ )
		{
			if ( *ch == '/' )
			{
				*ch = '\\';
			}
			else if ( *ch >= 'A' && *ch <= 'Z' )
			{
				*ch += (AnsiChar)( 'a' - 'A' );
			}
		}
	}
	else
	{
		// Empty resource path
		m_path.Clear();
	}
}

String CResourcePath::ToString() const
{
	if ( m_path.Size() )
	{
		return ANSI_TO_UNICODE( m_path.TypedData() );
	}
	else
	{
		return String::EMPTY;
	}
}
