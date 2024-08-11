/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CGraphConnection );

CGraphConnection::CGraphConnection()
	: m_source( NULL )
	, m_destination( NULL )
	, m_inactive( false )
{
}

CGraphConnection::CGraphConnection( CGraphSocket* source, CGraphSocket* destination )
	: m_source( source )
	, m_destination( destination )
	, m_inactive( false )
{
	ASSERT( m_source );
	ASSERT( m_destination );
}
