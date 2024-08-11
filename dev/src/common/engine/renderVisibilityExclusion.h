/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/
#pragma once

#include "../renderer/renderHelpers.h"

/// Visibility exclusion handler for rendering
class IRenderVisibilityExclusion : public IRenderObject
{
public:
	IRenderVisibilityExclusion();
	virtual ~IRenderVisibilityExclusion();

public:
	//!< Get number of object in the list
	virtual const Uint32 GetNumObjects() const = 0;
};

