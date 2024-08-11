/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CUndoTreeBlockMove;

/// Basic tree editor
class CEdTreeEditor : public CEdCanvas
{
	DECLARE_EVENT_TABLE()

	friend class CUndoTreeBlockMove;

public:
	/// Graph editor hook
	class IHook
	{
	public:
		virtual ~IHook() {};

		//! Selection has changed
		virtual void OnGraphSelectionChanged()=0;
	};

public:
	CEdTreeEditor( wxWindow * parent, IHook * hook, Bool blockPosRelativeToParent = false, Bool allowAddByDragging = false, Bool allowDecorateByDragging = false );

	~CEdTreeEditor();

	//! Get selected objects
	const TDynArray< IScriptable* >& GetSelectedObjects() const { return m_selected; }

	//! Set the undo manager
	void SetUndoManager( CEdUndoManager* undoManager ) { m_undoManager = undoManager; }

	//! Delete selection
	virtual void DeleteSelection() {}

	//! Force layout update
	virtual void ForceLayoutUpdate() = 0;

	//! Zoom extents
	void ZoomExtents();

	//! Automatically layout the tree starting from the given block (NULL == whole tree)
	void AutoLayout( CObject* block = NULL );

protected:
	/// Block layout info
	struct LayoutInfo
	{
		enum Gizmo
		{
			NONE,
			CONNECTOR,
			ELLIPSIS
		};

		wxPoint		m_windowPos;		//!< Block position
		wxSize		m_windowSize;		//!< Absolute window size
		wxPoint		m_connectorSrc;		//!< Connector source
		wxPoint		m_connectorDest;	//!< Connector destination
		wxColour	m_bgColor;			//!< Background color
		wxColour	m_textColor;		//!< Text color
		Gdiplus::Bitmap* m_bitmap;		//!< Icon shown in block
		Gizmo		m_topGizmo;			//!< Top gizmo type
		Gizmo		m_bottomGizmo;		//!< Bottom gizmo type

		LayoutInfo*	m_parent;
		IScriptable* m_owner;

		static const wxColour DEFAULT_TEXT_COLOR;
		static const wxColour DEFAULT_BG_COLOR;
		static const wxColour INVALID_BG_COLOR;

		LayoutInfo() : m_parent( NULL ), m_owner( NULL ), m_bitmap( NULL ), m_topGizmo( NONE ), m_bottomGizmo( NONE ) {}
		
		~LayoutInfo()
		{
			if ( m_bitmap ) delete m_bitmap;
		}
	};

	enum GizmoLocation
	{
		GL_None,
		GL_Top,
		GL_Bottom
	};

	//! Clear layout
	void ClearLayout();
	//! Update block layout
	LayoutInfo* UpdateBlockLayout( IScriptable* block, LayoutInfo * parentLayout );
	//! Get layout for object
	LayoutInfo* GetLayout( IScriptable* block );
	//! Change graph scale
	void ScaleGraph( Float newScale );

	// CEdCanvas:
	virtual void PaintCanvas( Int32 width, Int32 height ) override;
	virtual void MouseClick( wxMouseEvent& event ) override;
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta ) override;

	//! Load block position from somewhere
	virtual Bool LoadBlockPosition( IScriptable * block, wxPoint & pos ) { return false; }
	//! Save block position to somewhere
	virtual Bool SaveBlockPosition( IScriptable * block, const wxPoint & pos ) { return false; }
	//! Get block friendly name
	virtual String GetBlockName( IScriptable & block ) const { return block.GetFriendlyName(); }
	//! Get block comment
	virtual String GetBlockComment( IScriptable & block ) const { return String::EMPTY; }
	//! Get the resource name of the bitmap
	virtual String GetBitmapName( IScriptable & block ) const { return TXT(""); }
	//! Can this block have children?
	virtual Bool CanHaveChildren( IScriptable & block ) const { return false; }
	//! Is this block locked?
	virtual Bool IsLocked( IScriptable & block ) const { return false; }
	//! Is branch under this block hidden?
	virtual Bool IsHiddenBranch( IScriptable & block ) const { return false; }

	//! Draw block layout
	virtual void DrawBlockLayout( LayoutInfo & layout );	
	//! Draw block links
	virtual void DrawBlockLinks( LayoutInfo & layout );
	//! Draw dragged link
	virtual void DrawDraggedLink();

	//! Deselect all objects
	void DeselectAllObjects( Bool notifyHook = true );
	//! Returns true if given object is selected
	Bool IsObjectSelected( IScriptable* object );
	//! Select object
	void SelectObject( IScriptable* block, Bool select, Bool notifyHook = true );
	//! Select with children
	void SelectRecursive( LayoutInfo* rootLayout, Bool select, Bool notifyHook = true );
	//! Select objects from area
	void SelectObjectsFromArea( wxRect area, Bool sendEvent = true );
	//! Removes children and/or locked items from selection
	void CleanUpSelection( Bool removeChildren, Bool removeLocked );

	//! Get active item
	LayoutInfo* GetActiveItem() { return m_activeItem; }
	//! Get active item
	const LayoutInfo* GetActiveItem() const { return m_activeItem; }
	//! Set active item (NULL == none)
	void SetActiveItem( LayoutInfo* item ) { m_activeItem = item; }

	//! Finalize link
	virtual void OnAddByDragging( wxPoint pos, GizmoLocation location ) {}
	//! Gizmo of the node was clicked
	virtual void OnGizmoClicked( LayoutInfo* layout, GizmoLocation location ) {}
	//! Called when move ended
	virtual void OnMoveEnded() {}
	//! Open context menu
	virtual void OnOpenContextMenu() {}

	CEdUndoManager*	GetUndoManager() { return m_undoManager; }

private:
	typedef TDynArray< LayoutInfo* > TreeLayerInfo;

	Int32 MinDistanceBetween( LayoutInfo* layout1, LayoutInfo* layout2 ) const;
	void BuildLevels( LayoutInfo* parent, TDynArray< TreeLayerInfo >& layers, Uint32 level ) const;
	void MoveOnLevel( const TreeLayerInfo& layer, Uint32 index, Int32 nePos );
	Bool CalculateChildrenExtend( const LayoutInfo* parent, const TreeLayerInfo & childLayer, Int32& start, Int32& end ) const;

	void DoSavePosition( const LayoutInfo& layout );
	void UpdateActiveItem( wxPoint mousePoint );
	void CalculateBezierPoints( const wxPoint& from, const wxPoint& to, wxPoint (&outPoints)[4] );
	wxRect GetGizmoRect( const LayoutInfo& layout, GizmoLocation location ) const;
	void DrawGizmo( const LayoutInfo& layout, GizmoLocation location, Bool active, Bool selected );

	void MoveSelectedBlocks( wxPoint offset, Bool alternate );
	void MoveBlock( LayoutInfo* layout, wxPoint totalOffset, Bool alternate );
	void RecursivelyMoveBlocks( const LayoutInfo& parent, wxPoint offset );
	void RecursivelySavePosition( const LayoutInfo& parent );
	void UpdateDraggedSelection( const wxPoint& pos, Bool sendEvent );
	void OnMouseWheel( wxMouseEvent& event );

	/// Mouse action mode
	enum EMouseAction
	{
		MA_None,				//!< Nothing, free mouse
		MA_BackgroundScroll,	//!< Scrolling background
		MA_MovingWindows,		//!< Moving selected windows
		MA_MovingWindowsAlt,	//!< Moving selected windows in an alternate way
		MA_SelectingWindows,	//!< Selecting multiple windows
		MA_DraggingLink,		//!< Dragging link
	};

	THashMap< IScriptable*, LayoutInfo* >	m_layout;				//!< Layout info for blocks
	CEdUndoManager*						m_undoManager;

	LayoutInfo*							m_activeItem;			//!< Active item
	GizmoLocation						m_activeItemGizmo;		//!< Which item gizmo is active
	Bool								m_blockMoved;

	Bool								m_allowAddByDragging;	//!< Allow adding block by dragging from the connector
	Bool								m_allowDecorateByDragging;
	Bool								m_blockPosRelativeToParent;

	wxPoint								m_lastMousePos;			//!< Last position of mouse
	wxPoint								m_selectRectStart;		//!< Selection rect first corner
	wxPoint								m_selectRectEnd;		//!< Selection rect second corner
	wxPoint								m_autoScroll;			//!< Automatic background scroll
	Int32								m_moveTotal;			//!< Total move
	EMouseAction						m_action;				//!< Action performed
	Float								m_desiredScale;			//!< Desired scale

	TDynArray< IScriptable* >			m_selected;				//!< Selected objects
	IHook*								m_hook;					//!< Event hook
	Gdiplus::Bitmap*					m_lockBitmap;
};
