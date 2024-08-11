/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/meshComponent.h"

class CBgMeshComponent : public CMeshComponent
{
	DECLARE_ENGINE_CLASS( CBgMeshComponent, CMeshComponent, 0 );

public:
	CBgMeshComponent();
};

BEGIN_CLASS_RTTI( CBgMeshComponent )
	PARENT_CLASS( CMeshComponent );
END_CLASS_RTTI();
