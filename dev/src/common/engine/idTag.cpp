/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "idTag.h"
#include "baseEngine.h"

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SIMPLE_RTTI_TYPE( IdTag );

//////////////////////////////////////////////////////////////////////////////////////////////

void IdTag::Serialize( IFile& file )
{
	file << m_isDynamic;
	file << m_guid;
}

void IdTag::ToString( String& outString ) const
{
	if ( m_isDynamic )
	{
		outString += TXT("Dynamic: ");
	}
	else
	{
		outString += TXT("Static: ");
	}

	outString += ::ToString( m_guid );
}

const IdTag& IdTag::Empty()
{
	static IdTag emptyTag;
	return emptyTag;
}

IdTag IdTag::AllocateStaticTag()
{
#ifdef RED_FINAL_BUILD
	return IdTag();
#else
	// Allocate GUID
	IdTag tag;
	tag.m_isDynamic = false;
	tag.m_guid = CGUID::Create();
	return tag;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////
