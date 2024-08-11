/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/moveNavigationPath.h"


class CMaraudersMapItemBase;

class IMaraudersMapLogger
{
public:
	virtual void AddLogInfo( const String& msg ) = 0;
	virtual void AddLogInfo( const TDynArray<String> &msg ) = 0;
	virtual void ClearLog() = 0;
};

class CMaraudersMapCanvasDrawing : public CEdCanvas
{
public:
	CMaraudersMapCanvasDrawing( wxWindow* parent )
		: CEdCanvas( parent )
		, m_scaleWorld( 1.0f, 1.0f, 1.0f )
	{}

	// High level graphics methods
	void DrawTooltip( Float xPosTime, Float yPosTime, const String &text );
	
	// Low level graphics methods
	void DrawTextCanvas( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void DrawTextCanvas( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void DrawLineCanvas( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width = 1.0f );
	void DrawLineCanvas( const Vector& start, const Vector& end, const wxColour& color, Float width = 1.0f );
	void FillRectCanvas( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color );
	void DrawCircleCanvas( Float x, Float y, Float radius, const wxColour& color, Float width = 1.0f );
	void DrawCircleCanvas( const Vector& center, Float radius, const wxColour& color, Float width = 1.0f );
	void DrawCircleConstRadiusCanvas( const Vector& center, Int32 radius, const wxColour& color, Float width = 1.0f );
	void FillCircleCanvas( const Vector& center, Float radius, const wxColour& color );
	void FillCircleConstRadiusCanvas( const Vector& center, Int32 radius, const wxColour& color );
	void DrawCrossCavnas( const Vector &center, Float size, const wxColour& color, Float width = 1.0f );
	void DrawCrossCavnasConstSize( const Vector &center, Int32 size, const wxColour& color, Float width = 1.0f );
	void DrawPolyCanvas( const Vector points[], Int32 numPoints, const wxColour& color, Float width = 1.0f );
	void FillPolyCanvas( const Vector points[], Int32 numPoints, const wxColour& color );

	// Scale translate methods
	Vector  CanvasToWorld( wxPoint point ) const;
	wxPoint WorldToCanvas( const Vector&  point ) const;
	Vector  ClientToWorld( wxPoint point ) const;
	wxPoint WorldToClient( const Vector&  point ) const;

	// Regions : 'zoomed region' is the region where canvas is actually zoomed
	Vector GetZoomedRegion() const;
	void SetZoomedRegion( const Vector& corner1, const Vector& corner2 );
	void Center( const Vector &centerPos );

	// Utility methods
	Float SnapFloat( const Float& floatToSnap ) const;

protected:
	Vector m_scaleWorld;
};

class CMaraudersMapCanvas : public CMaraudersMapCanvasDrawing, public IMaraudersMapLogger, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

	enum EMouseAction
	{
		MA_None,
		MA_Zooming,
		MA_BackgroundScroll,
		MA_MovingActor,
	};
	EMouseAction m_action; // Action actually performed

public:
	CMaraudersMapCanvas( wxWindow* parent );
	~CMaraudersMapCanvas();

	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	// Events
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) {}

	// Options/features
	void TeleportPlayer();
	void TeleportCamera();
	void SetFollowingPlayer( Bool isFollowingPlayer );
	
	// Waypoints (for marking places)
	void SetWaypoint( const Vector& worldPos ); // only one waypoint can be marked
	void ClearWaypoint();
	void GotoWaypoint(); // goto current waypoint

	// Items
	void Reset();
	Bool ItemExist( const CMaraudersMapItemBase &item );
	Bool AddItem( CMaraudersMapItemBase *item, Bool checkUniqueness = true );	
	void RemoveInvalidItems();
	void Deselect();
	const CMaraudersMapItemBase *GetSelectedItem();
	void HideItem( Int32 id );
	void ShowItem( Int32 id );
	void ActivateVisibilityFilter( Int32 id );
	void DeactivateVisibilityFilter( Int32 id );
	void ActivateSelectingFilter( Int32 id );
	void DeactivateSelectingFilter( Int32 id );
	void ShowItemSelectionContextMenu( wxPoint menuPos );
	void ShowItemContextMenu( wxPoint menuPos );
	void OnItemSelectionContextMenu( wxCommandEvent &event );
	void OnItemContextMenu( wxCommandEvent &event );
	TDynArray< CMaraudersMapItemBase* >& GetItems() { return m_items; }
	void GotoItem( CMaraudersMapItemBase *item );
	void SelectItem( CMaraudersMapItemBase *item );
	Bool GotoActionPointItem( TActionPointID apID );

	// The Marauder's Map log
	virtual void AddLogInfo( const String& msg );
	virtual void AddLogInfo( const TDynArray<String> &msg );
	virtual void ClearLog();

	// Info methods (read-only accessors)
	const Vector &GetLastMouseWorldPos() const { return m_lastMousePosition; }

private:
	// Updates coordinates for all widgets
	void UpdateLayout();

	// Scrolling
	void ScrollBackgroundOffset( wxPoint delta );

	// Drawing methods
	virtual void PaintCanvas( Int32 width, Int32 height );
	void DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText );
	void DrawFreeCamera();

	// Mouse methods
	virtual void MouseClick( wxMouseEvent& event );
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );
	void OnMouseLeftDblClick( wxMouseEvent &event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnCharPressed( wxKeyEvent& event );
	void MouseClickCanvasPanel( wxMouseEvent& event );
	Bool IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const;

	// Items methods
	Bool IsItemVisible( const CMaraudersMapItemBase *item ) const;
	Bool IsItemSelectable( const CMaraudersMapItemBase *item ) const;

	// Fast update items
	void AddItemToFastUpdate( CMaraudersMapItemBase *item );
	void FastUpdateItemsTick();

	static Bool GetTerrainCollision( const Vector& pos, Vector& colisionPos );

	// Mouse data
	Int32    m_moveTotal;            // Distance moved when right mouse button was pressed
	Vector m_lastMousePosition;    // Used when dynamically scaling canvas via mouse move
	Vector m_clickedMousePosition; // Used in context menu

	// Items
	TDynArray< CMaraudersMapItemBase * >       m_items;
	CMaraudersMapItemBase                     *m_itemMouseOver;
	TDynArray< CMaraudersMapItemBase *>        m_itemsMouseOver;
	CMaraudersMapItemBase                     *m_itemSelected;
	TDynArray< CMaraudersMapItemBase * >       m_fastUpdateItems;
	TDynArray< Int32 >                           m_hiddenItemsIDs;
	TBitSet< 16 >                              m_activeFiltersVisibility;
	TBitSet< 16 >                              m_activeFiltersSelecting;

	// Tools
	void ExecuteTool();
	enum ELeftMouseOption { LMO_NONE, LMO_PLAYER_TELEPORT, LMO_CAMERA_TELEPORT };
	ELeftMouseOption m_leftMouseOption;
	Bool m_isFollowingPlayer;

	// Waypoints
	Vector m_waypoint;
	Bool m_isWaypointEnabled;

	// Log
	void DrawLog();
	struct SLogEntry
	{
		String m_msg;
	};
	TDynArray< SLogEntry > m_log;
	Float m_logTimer;
	Float m_logTimerValue;
	Float m_logLastTime;
};

///////////////////////////////////////////////////////////////////////////////

class CMaraudersNavigationCanvas : public IDebugFrame
{
private:
	CMaraudersMapCanvasDrawing*		m_canvas;

public:
	CMaraudersNavigationCanvas() : m_canvas( NULL ) {}

	RED_INLINE void SetCanvas( CMaraudersMapCanvasDrawing* canvas ) { m_canvas = canvas; }

	// ------------------------------------------------------------------------
	// CMovePathPlanner::IDebugFrame implementation
	// ------------------------------------------------------------------------
	virtual void AddSphere( const Vector& pos, Float radius, const Color& color );
	virtual void AddLine( const Vector& start, const Vector& end, const Color& color );
};
