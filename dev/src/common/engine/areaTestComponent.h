/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "component.h"

//---------------------------------------------------------------------------

// General test component for easier testing the of the area/trigger system
class CAreaTestComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CAreaTestComponent, CComponent, 0 );

public:
	// Trace distance (for sweep test)
	Float m_traceDistance;

	// Box extents
	Vector m_extents;

	// Search radius (for closest point)
	Float m_searchRadius;

public:
	CAreaTestComponent();

	// Attach to world
	virtual void OnAttached( CWorld* world );

	// Detach from world
	virtual void OnDetached( CWorld* world );

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
#endif
};

BEGIN_CLASS_RTTI( CAreaTestComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_traceDistance, TXT("Distance of the trace - zero for overlap test") );
	PROPERTY_EDIT( m_extents, TXT("Box extents, zero for point test") );
	PROPERTY_EDIT( m_searchRadius, TXT("Closest point search radius") );
END_CLASS_RTTI();

//---------------------------------------------------------------------------
