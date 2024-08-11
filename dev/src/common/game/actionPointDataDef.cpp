/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "actionPointDataDef.h"

IMPLEMENT_ENGINE_CLASS( SActionPointId );

const TActionPointID ActionPointBadID = SActionPointId( CGUID::ZERO, CGUID::ZERO );				//!< Invalid action point identifier

SActionPointId::SActionPointId() 
{

}

SActionPointId::SActionPointId( const CGUID& component, const CGUID& entity ) 
	: m_component( component )
	, m_entity( entity )
{

}

String SActionPointId::ToString() const
{
	Char buffer[ ( RED_GUID_STRING_BUFFER_SIZE * 2 ) ];

	// Convert Entity (first half)
	m_entity.ToString( buffer, RED_GUID_STRING_BUFFER_SIZE );

	// Convert first null terminator to "-"
	buffer[ RED_GUID_STRING_BUFFER_SIZE - 1 ] = TXT( '-' );

	// Convert component (second half)
	m_component.ToString( &buffer[ RED_GUID_STRING_BUFFER_SIZE ], RED_GUID_STRING_BUFFER_SIZE );

	return String( buffer );
}

Bool SActionPointId::FromString( const String &text )
{
	if( m_entity.FromString( text.TypedData() ) )
	{
		if( m_component.FromString( &( text.TypedData()[ RED_GUID_STRING_BUFFER_SIZE ] ) ) )
		{
			return true;
		}
	}

	return false;
}
