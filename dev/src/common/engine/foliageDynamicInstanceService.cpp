/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageDynamicInstanceService.h"
#include "foliageRenderCommandDispatcher.h"
#include "foliageResourceHandler.h"
#include "baseTree.h"

namespace
{
	const Float CREATION_RADIUS = 0.1f;
}

CFoliageDynamicInstanceService::CFoliageDynamicInstanceService()
{

}

CFoliageDynamicInstanceService::CFoliageDynamicInstanceService( Red::TWeakPtr< CFoliageResourceHandler > resourceHandler )
	: m_resourceHandler( resourceHandler )
{
}

CFoliageDynamicInstanceService::~CFoliageDynamicInstanceService(void)
{
}

void CFoliageDynamicInstanceService::AddInstance( const CSRTBaseTree* baseTree, SFoliageInstance& instanceInfo )
{
	Red::TSharedPtr< CFoliageResourceHandler > handler = m_resourceHandler.Lock();

	if( handler && baseTree )
	{
		FoliageInstanceContainer instanceContainer;
		instanceContainer.PushBack( instanceInfo );
		Box rect = baseTree->GetBBox();
		rect += instanceInfo.GetPosition();

		handler->AddDynamicInstances( baseTree, instanceContainer, rect );
	}
}

Bool CFoliageDynamicInstanceService::RemoveInstance( const CSRTBaseTree* baseTree, const Vector& position )
{
	Red::TSharedPtr< CFoliageResourceHandler > handler = m_resourceHandler.Lock();

	if( handler && baseTree )
	{
		handler->RemoveDynamicInstances( baseTree, position, CREATION_RADIUS );
	}

	return true;
}

Bool CFoliageDynamicInstanceService::IsValid()
{
	return !m_resourceHandler.Expired();
}
