#pragma once


/// Preview Panel that allows interaction with certain types of objects. Interaction is done by pressing the "I" (interact)
/// key while the preview panel has focus. Apex Cloth can be dragged by holding the I key while moving the mouse. Apex
/// Destructibles can be broken by pressing I with the mouse over an object -- a force will be applied to all destructibles,
/// but should only affect those close to the mouse.
class CEdInteractivePreviewPanel : public CEdPreviewPanel
{
protected:
	CClothComponent*		m_draggingCloth;			//!< If a cloth is being dragged, this tracks that component.
	Int32					m_selectedClothVertex;		//!< The cloth vertex that is being dragged. Only relevant when m_draggingCloth is non-NULL.
	Float					m_clothHitDistance;			//!< The initial distance to where a cloth was dragged from.

	Int32					m_mouseX, m_mouseY;			//!< Mouse position in client-space, since OnViewportInput doesn't have mouse info.

public:
	CEdInteractivePreviewPanel( wxWindow* parent, Bool allowRenderOptionsChange = false );
	virtual ~CEdInteractivePreviewPanel();

protected:

	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	void FreezeSimulation( Bool state );

	void BeginInteraction( IViewport* view, Int32 x, Int32 y );
	void EndInteraction( IViewport* view, Int32 x, Int32 y );

	void UpdateDraggedCloth( IViewport* view );
};

