/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "instanceVar.h"
#include "rttiSystem.h"

InstanceVar::InstanceVar( CName typeName )
	: m_offset( 0xFFFFFFFF )
#ifdef RED_ASSERTS_ENABLED
	, m_index( 0xFFFFFFFF )
#endif
	, m_type( NULL )
{
	// Find the RTTI type by name
	m_type = SRTTI::GetInstance().FindType( typeName );
	if ( !m_type )
	{
		HALT( "Instance var uses unregistered type '%ls'. Please debug.", typeName.AsString().AsChar() );
	}
}
