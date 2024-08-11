/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderObject.h"

/// Visibility result
enum ERenderVisibilityResult
{
	RVR_NotTested,				//!< We do not have valid results yet...
	RVR_NotVisible,				//!< Object is not visible in main view and in shadow view
	RVR_PartialyVisible,		//!< Object is not directly visible but it's shadow may be
	RVR_Visible,				//!< Object is directly visible in main view
};

/// ID of the query
/// Since we will have a MASIVE number of queries we can no longer use the IRenderObject approach
typedef Uint32 TRenderVisibilityQueryID;

// Rendering entity group
class IRenderEntityGroup : public IRenderObject
{
public:
	IRenderEntityGroup();
	virtual ~IRenderEntityGroup();
};
