/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "entity.h"

class CCookedMeshEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CCookedMeshEntity, CEntity, 0 );

public:
	CCookedMeshEntity();
};

BEGIN_CLASS_RTTI( CCookedMeshEntity );
	PARENT_CLASS( CEntity );
END_CLASS_RTTI();
