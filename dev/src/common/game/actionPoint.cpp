/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "actionPoint.h"
#include "actionPointComponent.h"

#include "../core/gatheredResource.h"
#include "../core/depot.h"
#include "../game/inventoryDefinition.h"
#include "../core/dataError.h"
#include "../engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CActionPoint );

CGatheredResource resActionPointCategoryGroups( ACTION_POINT_CATEGORY_TABLE, RGF_Startup );

CActionPoint::CActionPoint() 
	: m_actionBreakable( true )
{
}

void CActionPoint::OnPasted( CLayer* layer )
{
	TBaseClass::OnPasted( layer );
	
	MarkModified();
}

#ifndef NO_DATA_VALIDATION
void CActionPoint::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// Test animated components
	Uint32 acNum = 0;
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		acNum++;
	}
	if ( acNum > 0 )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("Community"), TXT("Action point entity has got '%d' animated component(s)"), acNum );
	}
}
#endif

Bool CActionPoint::IsSupportingCategory( const CName& category ) const
{
	for ( ComponentIterator< CActionPointComponent > it( this ); it; ++it )
	{
		if ( (*it)->GetActionCategories().Exist( category ) == true )
		{
			return true;
		}
	}
	return false;
}

Bool CActionPoint::CanExtractComponents( const Bool isOnStaticLayer ) const
{
	// yup, optimize away the action points
	return true;
}

CActionPointCategoriesResourcesManager::CActionPointCategoriesResourcesManager() : C2dArraysResourcesManager( resActionPointCategoryGroups )
{

}
