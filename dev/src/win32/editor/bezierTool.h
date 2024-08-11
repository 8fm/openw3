/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CVertexEditorEntity;

class CEdBezierEdit : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdBezierEdit, IEditorTool, 0 );

public:
	struct SEditedComponent
	{
		CComponent *						m_component;
		TDynArray< CVertexEditorEntity* >	m_vertices;
		Bool								m_isClosed;
		Float								m_height;
	};

	typedef THashMap< CComponent*, SEditedComponent* >	TEditedMap;

	CWorld*					m_world;						//!< World shortcut
	CEdRenderingPanel*		m_viewport;						//!< Viewport shortcut
	TEditedMap				m_editedComponents;				//!< Selected components that support vertex edit
	SEditedComponent*		m_closestTrackComponent;		//!< Component closest to cursor
	Int32						m_hoveredTrackVertex;			//!< Vertex currently under cursor
	Int32						m_closestTrackVertex;			//!< Vertex closest to cursor
	Vector					m_closestTrackPoint;			//!< Point on edge closest to cursor

public:
	CEdBezierEdit();
	virtual String GetCaption() const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleActionClick( Int32 x, Int32 y );
	virtual Bool OnDelete();
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportTrack( const CMousePacket& packet );

	virtual Bool UsableInActiveWorldOnly() const { return false; }

	void OnVertexAdded( CVertexEditorEntity * vertex );
	void OnVertexRemoved( CVertexEditorEntity * vertex );
	void UpdateCurve(SEditedComponent * ec);

private:
	void DeleteVertex( SEditedComponent * ec, Int32 index );
	void Reset();
};

BEGIN_CLASS_RTTI( CEdBezierEdit );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();