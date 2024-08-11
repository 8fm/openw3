/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../../common/engine/areaComponent.h"

/// Editable area component
class CCrowdAreaComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CCrowdAreaComponent, CAreaComponent, 0 );

public:
	CCrowdAreaComponent();
	virtual ~CCrowdAreaComponent();

	// Get contour rendering color
	virtual Color CalcLineColor() const;

private:
	Bool Validate() const;
};

BEGIN_CLASS_RTTI( CCrowdAreaComponent );
	PARENT_CLASS( CAreaComponent );
END_CLASS_RTTI();
