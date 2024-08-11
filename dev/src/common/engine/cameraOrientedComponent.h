
#pragma once

#include "component.h"

class CCameraOrientedComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CCameraOrientedComponent, CComponent, 0 );
public:
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	virtual void OnTick( Float timeDelta );
	//virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
};

BEGIN_CLASS_RTTI( CCameraOrientedComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()