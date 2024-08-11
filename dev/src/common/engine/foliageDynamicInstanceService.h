/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "foliageResource.h"
#include "../core/weakPtr.h"

class CFoliageResourceHandler;
class IFoliageRenderCommandDispatcher;

class CFoliageDynamicInstanceService
{
private:
	Red::TWeakPtr< CFoliageResourceHandler > m_resourceHandler;

public:
	CFoliageDynamicInstanceService();
	CFoliageDynamicInstanceService( Red::TWeakPtr< CFoliageResourceHandler > resourceHandler );
	~CFoliageDynamicInstanceService(void);

	void AddInstance( const CSRTBaseTree* baseTree, SFoliageInstance& instanceInfo );
	Bool RemoveInstance( const CSRTBaseTree* baseTree, const Vector& position );
	Bool IsValid();

};
