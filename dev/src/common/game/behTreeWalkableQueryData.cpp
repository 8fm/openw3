/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeWalkableQueryData.h"

IMPLEMENT_ENGINE_CLASS( CBehTreePositioningRequest )




///////////////////////////////////////////////////////////////////////////////
// CBehTreePositioningRequest
///////////////////////////////////////////////////////////////////////////////
void CBehTreePositioningRequest::LazyCreate()
{
	if ( !m_request )
	{
		m_request.Assign( new CPositioningFilterRequest() );
	}
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreePositioningRequest::CInitializer
///////////////////////////////////////////////////////////////////////////////
void CBehTreePositioningRequest::CInitializer::InitializeItem( CAIStorageItem& item ) const
{

}
IRTTIType* CBehTreePositioningRequest::CInitializer::GetItemType() const
{
	return CBehTreePositioningRequest::GetStaticClass();
}