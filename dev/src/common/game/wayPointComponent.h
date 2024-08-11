/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/spriteComponent.h"

/////////////////////////////////////////////////////////////////////////////////////////

#define WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY

#ifdef NO_EDITOR
#define WAYPOINT_COMPONENT_NO_VALIDITY_DATA
#endif

/// WayPoint - navigation component
class CWayPointComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CWayPointComponent, CSpriteComponent, 0 )

protected:
	
	static IRenderResource*		m_markerValid;
	static IRenderResource*		m_markerInvalid;
	static IRenderResource*		m_markerWarning;
	static IRenderResource*		m_communityMarkerValid;
	static IRenderResource*		m_communityMarkerInvalid;
	static IRenderResource*		m_communityMarkerWarning;
	static IRenderResource*		m_markerNoMesh; 
	static IRenderResource*		m_markerSelection;

#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	Bool						m_usedByPathLib;			//!< This waypoint is used by path engine
	enum EValidState
	{
		VALIDITY_INVALID		= false,
		VALIDITY_VALID			= true,
		VALIDITY_WARRING,
		VALIDITY_UNCALCULATED
	};
	Int8						m_isValid;					//!< Is position valid for actors
#endif

	Bool						m_onCommunityLayer;
	virtual void WaypointGenerateEditorFragments( CRenderFrame* frame );
	
public:

#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	//! True if this location is valid position for actors ( has a navigation mesh )
	Bool IsPositionValid() const;

	//! Can this waypoint not used by path engine
	RED_INLINE Bool IsUsedByPathLib() const { return m_usedByPathLib; }
#else
	RED_INLINE Bool IsUsedByPathLib() const { return true; }
#endif

	//! Refresh status of all PathLib waypoints
	static void RefreshWaypoints();

	//! Check waypoint location - determine if the placement is valid or not
	void CheckLocation( CWorld * world, Bool isInitialAttach );

	CWayPointComponent();

	// Component was attached to world
	virtual void OnAttached( CWorld* world );

	// Component was detached from world
	virtual void OnDetached( CWorld* world );

#ifndef NO_EDITOR
	virtual void OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context ) override;
#endif

#ifdef USE_UMBRA
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif
	
#if !defined( WAYPOINT_COMPONENT_NO_VALIDITY_DATA ) && !defined( NO_EDITOR )
	// Update component transform
	virtual void EditorOnTransformChanged();
#endif

	// Get attachment group for this component - it determines the order
	virtual EAttachmentGroup GetAttachGroup() const;

	// Generate editor side rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

protected:
	//! Get sprite rendering color
	virtual Color CalcSpriteColor() const;	
	void InitializeMarkers();

	virtual IRenderResource* GetMarkerValid();
	virtual IRenderResource* GetMarkerInvalid();
	virtual IRenderResource* GetMarkerWarning();
	virtual IRenderResource* GetMarkerNoMesh();
	virtual IRenderResource* GetMarkerSelection();

public:
	//! Change the path engine flag
	void SetUsedByPathLib( Bool value );
	static IRenderResource* CreateAgentMesh( Float radius, Float height, const Color& color, Bool wireframe=false );

};

BEGIN_CLASS_RTTI( CWayPointComponent );
	PARENT_CLASS( CSpriteComponent );
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	PROPERTY_EDIT_NOT_COOKED( m_usedByPathLib, TXT("This waypoint is used by navigation system") );
#endif
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////////////////////

