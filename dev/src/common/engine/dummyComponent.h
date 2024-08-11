/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "component.h"

/// Dummy transformable component
class CDummyComponent: public CComponent
{
	DECLARE_ENGINE_CLASS( CDummyComponent, CComponent, 0 );

public:
	CDummyComponent();

    virtual Bool IsManualCreationAllowed() const { return false; }
};

DEFINE_SIMPLE_RTTI_CLASS( CDummyComponent, CComponent );
