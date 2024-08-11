/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "context.h"

SSFileContext::SSFileContext()
:	m_line( 0 )
{

}

SSFileContext::SSFileContext( const wstring& file, int line )
:	m_file( file )
,	m_line( line )
{

}
