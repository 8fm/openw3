//////////////////////////////////////
//        Inferno]|[ Engine         //
// Copyright (C) 2000-2003 by Dexio //
//////////////////////////////////////

#pragma once

#define GRAPH_BLOCK_RESIZE_RECT_SIZE	10

#include "canvas.h"
#include "graphConnectionClipboard.h"

class CEdGraphUndoManager;
class IGraphContainer;

struct SGraphBlockLayoutHelperData
{
	wxPoint location;
	wxPoint locationInScreen;
	wxSize canvasSize;
	Float scale;
	EGraphBlockShape shape;
	Color clientColor;
	Color borderColor;
	Color outerBorderColor;
	Color titleColor;
	wxRect selectionRect;
	wxRect blockRect;
	Float borderWidth;
	wxColour border;
	wxColour outerBorder;
	wxColour title;
	wxColour client;
	wxColour shadow;
	wxColour text;
};

class IGraphEditorClipboard
{
public:
	virtual void Copy( class CEdGraphEditor* editor ) = 0;
	virtual void Paste( class CEdGraphEditor* editor ) = 0;
};

/// Graph editor hook
class GraphEditorHook
{
public:
	virtual ~GraphEditorHook() {};

	virtual void OnGraphStructureWillBeModified( IGraphContainer* graph ) {}

	//! Structure of the graph was modified
	virtual void OnGraphStructureModified( IGraphContainer* graph )=0;

	//! Selection has changed
	virtual void OnGraphSelectionChanged()=0;
};

/// Linked graph editor
class CEdGraphEditor : public CEdCanvas, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

	friend class CEdGraphLayerEditor;
	friend class CUndoGraphStep;

protected:
	/// Mouse action mode
	enum EMouseAction
	{
		MA_None,				//!< Nothing, free mouse
		MA_BackgroundScroll,	//!< Scrolling background
		MA_MovingWindows,		//!< Moving selected windows
		MA_SelectingWindows,	//!< Selecting multiple windows
		MA_DraggingLink,		//!< Dragging link
		MA_BlockInternalDrag,	//!< Internal block event
		MA_RescalingWindows,	//!< Rescaling windows
	};

public:
	/// Socket layout info
	struct SocketLayoutInfo
	{
		wxRect				m_socketRect;	//!< Socket area in client space
		wxPoint				m_captionPos;	//!< Caption position in client space
		wxPoint				m_linkPos;		//!< Link point
		wxPoint				m_linkDir;		//!< Link tangent at link point
	};

	/// Block layout info
	struct BlockLayoutInfo
	{
		Bool						m_onScreen;		//!< Is block visible on screen
		Bool						m_visible;		//!< Is block 
		Bool						m_freeze;		//!< Is block freeze
		wxSize						m_windowSize;	//!< Absolute window size
		wxRect						m_titleRect;	//!< Block title rect
		wxRect						m_iconRect;		//!< Block icon rect
		wxRect						m_clientRect;	//!< Client area rect
		wxRect						m_innerRect;	//!< Inner area rect
		THashMap< THandle< CGraphSocket >, SocketLayoutInfo>	m_sockets;		//!< Visible sockets to draw
	};

protected:
	THashMap< THandle< CGraphBlock >, BlockLayoutInfo >	m_layout;	//!< Layout info for blocks

protected:
	wxPoint						m_lastMousePos;			//!< Last position of mouse
	IGraphContainer*			m_graph;				//!< Edited graph
	THandle< ISerializable >	m_activeItem;			//!< Item under cursor
	EMouseAction				m_action;				//!< Action performed
	Float						m_desiredScale;			//!< Desired graph scale	
	Bool						m_canBeModify;
	Bool						m_shouldRepaint;
	Bool						m_shouldZoom;
	CEdUndoManager*				m_undoManager;

private:
	wxPoint						  m_selectRectStart;	//!< Selection rect first corner
	wxPoint						  m_selectRectEnd;		//!< Selection rect second corner
	wxPoint						  m_autoScroll;			//!< Automatic background scroll
	THandle< ISerializable >	  m_contextMenuItem;	//!< Item for context menu
	CGraphSocket*				  m_sourceSocket;		//!< Source link socket
	CGraphSocket*				  m_destSocket;			//!< Destination link socket
	CGraphConnectionClipboard	  m_connectionClipboard;//!< Graph connection clipboard for copy/cut/paste connections
	THashSet<CGraphBlock*>		  m_selected;			//!< Selected blocks
	CGraphBlock*				  m_internalDragBlock;	//!< Special action block
	Int32						  m_moveTotal;			//!< Total move
	Bool						  m_mirrored;			//!< Blocks layout is mirrored
	Bool						  m_autoUnplugSingleConnections; //!< Automatically un-plug single connections when creating new ones
	TDynArray< CGraphSocket* >	  m_exposableSockets;	//!< Exposable sockets
	GraphEditorHook*			  m_hook;				//!< Event hook
	Bool						  m_windowsMoved;		//!< Were the blocks moved during last MA_MovingWindows action
	wxPoint						  m_rescalePressPoint;	//!< Cooridinates in block space of mouse click, when rescaling has started
	TDynArray < CGraphBlock* >	  m_blocksToMove;		//!< Blocks on top of currently moved block which IsMovingOverlayingBlocks method returns true
	THashMap< CGraphBlock*, wxPoint > m_movePressPoints;

public:
	CEdGraphEditor( wxWindow *parent, Bool mirrored, Bool autoUnplugSingleConnections = false );
	~CEdGraphEditor();

	//! Set graph editor event hook
	void SetHook( GraphEditorHook* hook );

	//! Set graph to edit
	void SetGraph( IGraphContainer *graph );

	//!
	void SetUndoManager( CEdUndoManager* undoManager );

	//! Scale to see the whole graph
	void ZoomExtents();

	//! Call ZoomExtents at a later time (in next editor tick)
	void ZoomExtentsLater();

	//! Focus graph on block
	void FocusOnBlock( CGraphBlock* block );

	//! Get selected blocks
	void GetSelectedBlocks( TDynArray< CGraphBlock* > &blocks );

	//! Delete selected blocks
	void DeleteSelection();

	//! Copy selected blocks
	void CopySelection( bool isCopy = true );

	//! Cut selected blocks
	void CutSelection();

	//! Paste blocks
	TDynArray< CGraphBlock* > Paste( const Vector* position, Bool atLeftUpper = false, Bool doNotCreateUndoSteps = false );

	//! Check blocks visibility
	void CheckBlocksVisibility();

	//! Reset blocks visibility
	void ResetBlocksVisibility( const TDynArray< CGraphBlock* >& blocks );

	//! returns blocks on given block
	void GetBlocksOnBlock( TDynArray< CGraphBlock* >& blocks, CGraphBlock* block ) const;

protected:
	//! Set background offset
	void SetBackgroundOffset( wxPoint offset );

	//! Scroll background offset
	void ScrollBackgroundOffset( wxPoint delta );

public:
	//! Select all blocks
	void SelectAllBlocks();

	//! Deselect all block
	void DeselectAllBlocks();

	//! Returns true if given block is selected
	Bool IsBlockSelected( CGraphBlock* block );

	//! Select block
	void SelectBlock( CGraphBlock* block, Bool select, Bool clear = false );

	//! Set selected blocks
	void SelectBlocks( const TDynArray< CGraphBlock* > &blocks, Bool clear = false );

	//! Is block activated
	virtual Bool IsBlockActivated( CGraphBlock* block ) const { return block->IsActivated(); }

	//! Get blocks activation alpha
	virtual Float GetBlockActivationAlpha( CGraphBlock* block ) const { return block->GetActivationAlpha(); }

	//! Get block's layout
	const BlockLayoutInfo* GetBlockLayout( CGraphBlock* block ) const { return m_layout.FindPtr( block ); }

private:
	//! Copy selected block to clipboard
	Bool CopyBlocksToClipboard( const TDynArray< CGraphBlock* >& blocks, bool isCopy = true );

	//! Move selected blocks
	void MoveSelectedBlocks( wxPoint graphMousePoint );

	//! Select blocks from area
	void SelectBlocksFromArea( wxRect area, Bool clear );

	//! Open context menu
	void OpenContextMenu();

	//! Update active item
	void UpdateActiveItem( wxPoint mousePoint );

	//! Scale graph rendering
	void ScaleGraph( Float newScale );

	//! Filter block class list leaving only classes supported by this graph
	void FilterBlockClassList( TDynArray< CClass* >& blockClasses );

	//! Perform selected blocks alignment
	void DoAlignSelection( Bool horz );

	//! Perform selected blocks distribution
	void DoDistributeSelection( Bool horz );

	//! Get resize rectangle
	wxRect GetResizeRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const;

	//! Get resize rectangle
	wxRect GetFreezeRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const;

	//! Get move without content rectangle
	wxRect GetMoveWithoutContentRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const;

	//! Delete block connections
	void DeleteBlockConnections( CGraphBlock* block );

	//! Delete socket connections
	void DeleteSocketConnections( CGraphSocket* socket );

	//! Delete connection
	void DeleteConnection( CGraphConnection* connection );

protected:
	//! Get socket link position and direction
	void GetSocketLinkParams( CGraphSocket* socket, wxPoint& pos, wxPoint &dir );

	//! Can be modify
	Bool CanBeModify() const;

	//! Get graph
	IGraphContainer* GetGraph() { return m_graph; }

	Bool ModifyGraphStructure();

	void OnGraphStructureWillBeModified();

	//! Graph structure was modified
	void GraphStructureModified();

protected: 
	//! Get active graph item
	virtual THandle< ISerializable > GetActiveItem( const wxPoint& graphPoint );

	//! Paint the shit
	virtual void PaintCanvas( Int32 width, Int32 height );

	//! Fill context menu for Linked Block
	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );

	//! Fill context menu for Linked socket
	virtual void InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu );

	//! Fill context menu for Empty Selection
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );

	//! Add menu option for showing hidden sockets from given list
	virtual Bool InitLinkedSockedShowExposableMenu( const TDynArray< CGraphSocket* > &sockets, wxMenu &menu, wxMenu &baseMenu );

	//! Mouse event
	virtual void MouseClick( wxMouseEvent& event );
	
	//! Mouse moved
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );

	//! Dispatch system event
	virtual void DispatchEditorEvent( const CName& systemEvent, IEdEventData* data );

	//! Calculate title icon size
	virtual void CalcTitleIconArea( CGraphBlock* block, wxSize &size );

	//! Draw icon area for given block, rect is given in client space, clip rect is set
	virtual void DrawTitleIconArea( CGraphBlock* block, const wxRect& rect );

	//! Calculate inner area size for given block
	virtual void CalcBlockInnerArea( CGraphBlock* block, wxSize& size );

	//! Draw inner area for given block, rect is given in client space, clip rect is set
	virtual void DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect );

	//! Create connection between two given sockets
	virtual void ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket );

	//! Adjust block drawing parameters
	virtual void AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth );

	//! Adjust link drawing parameters
	virtual void AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth );

	//! Adjust link caps
	virtual void AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow );

	//! Adjust socket caption
	virtual void AdjustSocketCaption( CGraphSocket* socket, String& caption, wxColor& color );

	//! Check if this block can be dragged by clicking in inner arrea
	virtual Bool IsDraggedByClickOnInnerArea( wxMouseEvent& event, CGraphBlock* block );

protected:
	//! Calculate title bar size
	virtual wxPoint CalcTitleBarSize( const String &caption );

	//! Calculate size of vertical socket group
	virtual wxPoint CalcSocketGroupSizeV( const TDynArray< CGraphSocket* > &sockets );

	//! Calculate size of horizontal socket group
	virtual wxPoint CalcSocketGroupSizeH( const TDynArray< CGraphSocket* > &sockets, bool& hasCaptions );

	//! Calculate sockets layout on left side
	virtual void CalcSocketLayoutVLeft( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout, bool center );

	//! Calculate sockets layout on right side
	virtual void CalcSocketLayoutVRight( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout, bool center );

	//! Calculate sockets layout on bottom side
	virtual void CalcSocketLayoutHBottom( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout );

	//! Calculate sockets layout in center
	virtual void CalcSocketLayoutCenter( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout );

	//! Calculate sockets layout on title bar
	virtual void CalcSocketLayoutTitle( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, const wxRect& titleRect, BlockLayoutInfo& layout );

	//! Update block layout
	virtual void UpdateBlockLayout( CGraphBlock* block );

	//! Draws additional information over a connection
	virtual void AnnotateConnection( const CGraphConnection* con, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width = 1.0f );

	//! A utility method for calculating the middle point of a link
	wxPoint CalculateLinkMidpoint( const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size );

	//! Draw link
	virtual void DrawLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width = 1.0f, Bool srcCapArrow = false, Bool destCapArrow = false );

	//! Draw cross on link
	void DrawCrossOnLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size, float width = 1.0f );

	//! Draw cross on link, place [0,1]
	void DrawCrossOnLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size, float width, float place );

	//! Draw block layout
	virtual void DrawBlockLayout( CGraphBlock* block );
	virtual void DrawBlockShadows( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockSockets( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockBackground( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockBorder( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockSocketsCaptions( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockButtons( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );

	//! Draw block links
	virtual void DrawBlockLinks( CGraphBlock* block );

	//! Get the block display value
	virtual String GetBlockDisplayValue( CGraphBlock* block );

	//! Draw socket connector
	virtual void DrawConnector( const wxRect& rect, const wxColour& borderColor, const wxColour &baseColor, ELinkedSocketDrawStyle style, ELinkedSocketDirection direction, float width = 1.0f );

	//! Draw title bar
	virtual void DrawTitleBar( const wxRect& rect, const wxRect& iconRect, const wxColour& borderColor, const wxColour& clientColor, const wxColour& titleColor, const String &caption, EGraphBlockShape shape, float width = 1.0f, Bool shouldCaptionBeInConstantSize = false );

	//! Draw dragged link
	virtual void DrawDraggedLink();

	//! Hit test for block link
	Bool HitTestLink( const wxPoint *points, const wxPoint& point, const Float minDistFromSocket, const Float range );

	//! AABB hit test for block link - faster then HitTestLink
	Bool HitTestLinkAABB( const wxPoint& source, const wxPoint& dest, const wxPoint& testPoint ) const;
	Bool HitTestLinkAABB( const wxPoint& source, const wxPoint& dest, const wxRect& testRect ) const;

	//! Add block to graph
	CGraphBlock* SpawnBlock( CClass* blockClass, wxPoint graphPoint );

	//! Is block visible
	Bool IsBlockVisible( CGraphBlock* block, const BlockLayoutInfo* layout = NULL ) const;

	//! Utility method to obtain the real size from the block layout, or the zero vector if the layout is invalid.
	Vector GetSizeFromLayout( CGraphBlock* block ) const;

	//! Convert color to freeze mode
	virtual wxColor ConvertBlockColorToFreezeMode( const wxColor &color );
	virtual wxColor ConvertLinkColorToFreezeMode( const wxColor &color );

	//! Collect connections blocking from creating a new one
	TDynArray< CGraphConnection* > CollectBlockingConnections( const CGraphSocket* srcSocked, const CGraphSocket* dstSocket ) const;

	//! Get area of graph
	virtual wxSize GetGraphArea() const;

	//unique channel name for copy paste
	virtual const Char * ClipboardChannelName() const = 0 ;

protected:
	//! Events
	virtual void OnMouseWheel( wxMouseEvent& event );
	virtual void OnSpawnBlock( wxCommandEvent& event );
	virtual void OnSetCursor( wxSetCursorEvent& event );
	virtual void OnBreakAllLinks( wxCommandEvent& event );
	virtual void OnBreakLink( wxCommandEvent& event );
	virtual void OnHideSocket( wxCommandEvent& event );
	virtual void OnHideSockets( wxCommandEvent& event );
	virtual void OnShowSockets( wxCommandEvent& event );
	virtual void OnShowSocket( wxCommandEvent& event );
	virtual void OnUpdateVersion( wxCommandEvent& event );
	virtual void OnAlignHorz( wxCommandEvent& event );
	virtual void OnAlignVert( wxCommandEvent& event );
	virtual void OnDistributeHorz( wxCommandEvent& event );
	virtual void OnDistributeVert( wxCommandEvent& event );
	virtual void OnSize( wxSizeEvent& event );
	virtual void OnCopyConnections( wxCommandEvent& event );
	virtual void OnCutConnections( wxCommandEvent& event );
	virtual void OnPasteConnections( wxCommandEvent& event );
	virtual void OnMouseMiddleEvent( wxMouseEvent& event );
	virtual void OnKeyDown( wxKeyEvent& event );

	void OnAlignConnectedNode( wxCommandEvent& event );
	Bool GetConnectedSocketLayouts( CGraphBlock* left, CGraphBlock* right, SocketLayoutInfo*& leftSocket, SocketLayoutInfo*& rightSocket );

protected:

	// This fields are for the hack that fixes block focusing on the debugger creation.
	// The graphEditors height is 0 at this time, so refocus the block on the first OnSize
	// event with non-zero height.
	bool         m_LastSizeEventHad0Height;
	CGraphBlock* m_LastFocusedBlock;
};
