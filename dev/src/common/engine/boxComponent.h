/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "boundedComponent.h"

class CBoxComponent : public CBoundedComponent
{
	DECLARE_ENGINE_CLASS( CBoxComponent, CBoundedComponent, 0 );

public:
	CBoxComponent();
	virtual ~CBoxComponent();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );
	
	// Update bounds
	virtual void OnUpdateBounds();

	// Editor drawing
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

protected:
	static const Uint16 m_indices[];

	Color			m_drawingColor;
	Vector			m_vertices[ 8 ];

private:

	Float	m_width;
	Float	m_height;
	Float	m_depth;
};

BEGIN_CLASS_RTTI( CBoxComponent )
	PARENT_CLASS( CBoundedComponent )
	PROPERTY_EDIT( m_width, TXT( "Box width (X axis)") )
	PROPERTY_EDIT( m_height, TXT( "Box height (Z axis)") )
	PROPERTY_EDIT( m_depth, TXT( "Box depth (Y axis)") )
	PROPERTY_EDIT( m_drawingColor, TXT( "Color in editor" ) )
END_CLASS_RTTI()
