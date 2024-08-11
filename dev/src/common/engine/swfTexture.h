/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "bitmapTexture.h"

//////////////////////////////////////////////////////////////////////////
// CSwfTexture
//////////////////////////////////////////////////////////////////////////
class CSwfTexture : public CBitmapTexture
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSwfTexture, CBitmapTexture, "redswfx", "Exported SWF texture" );

private:
	String						m_linkageName;

public:
	const String&				GetLinkageName() const { return m_linkageName; }

public:
#ifndef NO_RESOURCE_IMPORT
	struct FactoryInfo : public CResource::FactoryInfo< CSwfTexture >
	{	
		String					m_linkageName;
	};

	static CSwfTexture* Create( const FactoryInfo& data );
#endif

#ifndef NO_RESOURCE_COOKING
	Bool UncookData();
#endif
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CSwfTexture );
PARENT_CLASS( CBitmapTexture );
	PROPERTY_RO( m_linkageName, TXT("Linkage name") );
END_CLASS_RTTI();
