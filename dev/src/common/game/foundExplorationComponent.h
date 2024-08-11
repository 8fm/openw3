/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

/// WayPoint - navigation component
class CFoundExplorationComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CFoundExplorationComponent, CSpriteComponent, 0 )

public:
	CFoundExplorationComponent();

	// Generate editor side rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Component was attached to world
	virtual void OnAttached( CWorld* world );

	// Component was detached from world
	virtual void OnDetached( CWorld* world );

	void InitializeMarkers();

	//! Get sprite rendering color
	virtual Color CalcSpriteColor() const;	

	Bool ShouldBeIgnored() const { return m_ignore; }
	void ToggleIgnored( Bool ignore ) { m_ignore = ignore; }

protected:
	static IRenderResource* m_markerIgnored;
	static IRenderResource* m_markerInvalid;

private:
	Bool m_ignore;

	static IRenderResource* CreateAgentMesh( const Color& color );
};

BEGIN_CLASS_RTTI( CFoundExplorationComponent );
	PARENT_CLASS( CSpriteComponent );
	PROPERTY_EDIT( m_ignore, TXT("") );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////////////////////

