/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "effectEditor.h"
#include "../../common/engine/fxSystem.h"

class CEdEffectTracksEditorBar //: public CFXBase	//ab fix. let's see what happens
{
public:
	CEdEffectTracksEditorBar( int trackBarWidth, int trackBarHeight, CEdEffectTracksEditor *editor )
		: m_position( 0.0f )
		, m_editor( editor )
		, m_trackBarWidth( trackBarWidth )
		, m_trackBarHeight( trackBarHeight )
		, m_hightlight( false )
	{ }
	~CEdEffectTracksEditorBar() {}

	// Position in time space
	Float GetPos() const { return m_position; }
	void SetPos( Float position ) { m_position = position; }

	Int32 GetTrackBarWidth() const { return m_trackBarWidth; }
	Int32 GetTrackBarHeight() const { return m_trackBarHeight; }

	void Hightlight( Bool switchHightlight = true ) { m_hightlight = switchHightlight; }
	Bool IsHightlighted() const { return m_hightlight; }

	virtual wxRect GetRect() const = 0; // get mouse clickable rect
	virtual void UpdateLayout() = 0;    // calculate position
	virtual void Draw() = 0;            // draw bar
	virtual void DrawTooltip( Float yPos );

protected:
	Float m_position;
	CEdEffectTracksEditor *m_editor;
	const int m_trackBarWidth;
	const int m_trackBarHeight;
	Bool m_hightlight;

	// Display position in Canvas coordinates
	int m_x;
	int m_y;
};

class CEdEffectTracksEditorTimeBar : public CEdEffectTracksEditorBar
{
public:
	CEdEffectTracksEditorTimeBar( int trackBarWidth, int trackBarHeight, CEdEffectTracksEditor *editor )
		: CEdEffectTracksEditorBar( trackBarWidth, trackBarHeight, editor )
	{}
	~CEdEffectTracksEditorTimeBar() {}

	virtual wxRect GetRect() const;
	virtual void UpdateLayout();
	virtual void Draw();

private:
};

//////////////////////////////////////////////////////////////////////////

class CEdEffectTracksEditorIntervalTimeBeginBar : public CEdEffectTracksEditorBar
{
public:
	CEdEffectTracksEditorIntervalTimeBeginBar( int trackBarWidth, int trackBarHeight, int verticalPosition, CEdEffectTracksEditor *editor )
		: CEdEffectTracksEditorBar( trackBarWidth, trackBarHeight, editor )
		, m_verticalPosition( verticalPosition )
	{}
	~CEdEffectTracksEditorIntervalTimeBeginBar() {}

	virtual wxRect GetRect() const;
	virtual void UpdateLayout();
	virtual void Draw();

private:
	const int m_verticalPosition;
};

//////////////////////////////////////////////////////////////////////////

class CEdEffectTracksEditorIntervalTimeEndBar : public CEdEffectTracksEditorBar
{
public:
	CEdEffectTracksEditorIntervalTimeEndBar( int trackBarWidth, int trackBarHeight, int verticalPosition, CEdEffectTracksEditor *editor )
		: CEdEffectTracksEditorBar( trackBarWidth, trackBarHeight, editor )
		, m_verticalPosition( verticalPosition )
	{
		m_position = 20.0f;
	}
	~CEdEffectTracksEditorIntervalTimeEndBar() {}

	virtual wxRect GetRect() const;
	virtual void UpdateLayout();
	virtual void Draw();

private:
	const int m_verticalPosition;
};

//////////////////////////////////////////////////////////////////////////

class CEdEffectTracksEditorTimeEndBar : public CEdEffectTracksEditorBar
{
public:
	CEdEffectTracksEditorTimeEndBar( int trackBarWidth, int trackBarHeight, int verticalPosition, CEdEffectTracksEditor *editor )
		: CEdEffectTracksEditorBar( trackBarWidth, trackBarHeight, editor )
		, m_verticalPosition( verticalPosition )
	{
		m_position = 40.0f;
	}
	~CEdEffectTracksEditorTimeEndBar() {}

	virtual wxRect GetRect() const;
	virtual void UpdateLayout();
	virtual void Draw();

private:
	const int m_verticalPosition;
};

//////////////////////////////////////////////////////////////////////////

class CEdEffectTracksEditor : public CEdCanvas, public CEffectEditorSynchronizable
{
	friend CEdEffectTracksEditorBar;
	friend CEdEffectTracksEditorTimeBar;
	friend CEdEffectTracksEditorIntervalTimeBeginBar;
	friend CEdEffectTracksEditorIntervalTimeEndBar;
	friend CEdEffectTracksEditorTimeEndBar;

	DECLARE_EVENT_TABLE();

	// client rect coordinates
	struct LayoutInfo
	{
		wxRect m_windowRect;
		wxRect m_expandWindowRect;
	};

	enum EMouseAction
	{
		MA_None,
		MA_Zooming,
		MA_BackgroundScroll,
		MA_MovingTrackTimeBar,
		MA_MovingTrackIntervalTimeBeginBar,
		MA_MovingTrackIntervalTimeEndBar,
		MA_MovingTrackTimeEndBar,
		MA_MovingTrackItem,
		MA_ResizingTrackItem,
	};
	EMouseAction m_action; // Action actually performed

public:
	CEdEffectTracksEditor( wxWindow* parent, CFXDefinition *effectDefinition, Int32 sidePanelWidth, CEdEffectCurveEditor *effectCurveEditor, CEdEffectEditor *effectEditor );
	~CEdEffectTracksEditor();

	virtual void PaintCanvas( Int32 width, Int32 height );
	virtual void MouseClick( wxMouseEvent& event );
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );

	virtual void Synchronize( wxPoint offset, const Vector& scale );

	void UpdateBars();

	void SetEffectTime( Float time );
	void SetAnimationLength( Float time );

	Vector GetScale() const { return m_scaleTime; }

	void CopySelectedTrackItem();
	void PasteCopiedTrackItem();

private:
	void UpdateLayout(); // updates coordinates for all widgets

	void OnMouseLeftDblClick( wxMouseEvent &event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnCharPressed( wxKeyEvent& event );

	Vector  CanvasToTime( wxPoint point ) const;
	wxPoint TimeToCanvas( const Vector&  point ) const;
	Vector  ClientToTime( wxPoint point ) const;
	wxPoint TimeToClient( const Vector&  point ) const;

	// GUI elements
	CEdEffectTracksEditorBar *m_trackTimeBar;
	CEdEffectTracksEditorBar *m_trackIntervalTimeBeginBar;
	CEdEffectTracksEditorBar *m_trackIntervalTimeEndBar;
	CEdEffectTracksEditorBar *m_trackTimeEndBar;
	TDynArray< CEdEffectTracksEditorBar * > m_trackBars; // holds all bars

	// GUI data
	THashMap< CFXBase*, LayoutInfo > m_layout; // layout info for blocks
	Vector                           m_scaleTime;
	Int32                              m_sidePanelWidth;

	// Data
	CFXDefinition*					m_fxDefinition;

	// Editors
	CEdGradientEditor    *m_gradientEditor;
	CEdEffectCurveEditor *m_effectCurveEditor;
	CEdEffectEditor      *m_effectEditor;

	// Currently displayed time
	Float m_timeBegin;
	Float m_timeEnd;
	Float m_animationLength;

	// Mouse data
	Int32       m_moveTotal;            // Distance moved when right mouse button was pressed
	wxPoint   m_autoScroll;           // Used for auto scroll when mouse is near edge of canvas panel
	Vector    m_lastMousePosition;    // Used when dynamically scaling canvas via mouse move
	Vector    m_clickedMousePosition; // Used in context menu
	Bool	  m_mouseMovedSinceLast;  // Needs to be manually set to false

	// Mouse methods
	Bool IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const;
	virtual void MouseClickSidePanel( wxMouseEvent& event );
	virtual void MouseClickCanvasPanel( wxMouseEvent& event );

	// Drawing methods
	void DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText );
	void DrawDownPanel();
	void DrawSidePanel();
	void DrawTrackItems();
	void DrawTooltip( Float xPosTime, Float yPosTime, const String& customString = String::EMPTY );

	// Track bar
	Bool IsOverTrackBar( wxPoint point, CEdEffectTracksEditorBar *trackBar );

	// Zoomed region is the region where Curve Editor Canvas is actually zoomed
	Vector	GetZoomedRegion() const;
	void	SetZoomedRegion( const Vector& corner1, const Vector& corner2 );
	//void	SetZoomedRegionToFit();

	void ScrollBackgroundOffset( wxPoint delta );

	Int32 GetSidePanelWidth() const;
	Int32 GetSidePanelHeight() const;
	Int32 GetDownPanelHeight() const;

	// Tools methods
	Float SnapFloat( const Float& floatToSnap ) const;
	Float SnapToBar( Float timePos );

	// Low level graphics methods
	void DrawText( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void DrawText( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void DrawLine( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width = 1.0f );
	void DrawLine( const Vector& start, const Vector& end, const wxColour& color, Float width = 1.0f );

	// Buttons
	Gdiplus::Bitmap *m_iconTreeExpand;
	Gdiplus::Bitmap *m_iconTreeCollapse;
	const int m_buttonHeight;
	const int m_buttonWidth;
	const int m_buttonExpandRectSize;
	CFXBase *m_buttonMouseOver;
	CFXBase *m_buttonMouseOverClicked; // for use in context menu only
	Bool m_buttonIsMouseOverTreeExpander;
	CFXTrackGroup *m_buttonMouseOverTrackGroup; // use when 'm_buttonIsMouseOverTreeExpander' is true
	void OnButtonRemove( wxCommandEvent &event );
	void OnButtonRename( wxCommandEvent &event );
	void OnButtonAppendTrackGroup( wxCommandEvent &event );
	void OnButtonInsertTrackGroup( wxCommandEvent &event );
	void OnButtonAppendTrack( wxCommandEvent &event );
	Int32 m_buttonsScroolPos;
	void ScrollButtons( Int32 shift );

	Float GetAnimationLength();

	void BeginAnimationDrag();
	void EndAnimationDrag();
	void UpdateAnimationDrag( Float time );

	// Track items
	CFXTrackItem *m_trackItemMouseOver;
	CFXTrackItem *m_trackItemMouseOverClicked;
	TDynArray<CFXTrackItem*> m_selectedTrackItems;

	Bool m_trackItemIsMouseOverSizer;

	void PasteTrackItem( CFXTrack *parentTrack, Float position );

	void OnRemoveTrackItem( wxCommandEvent &event );
	void OnRenameTrackItem( wxCommandEvent &event );
	void OnCopyTrackItem( wxCommandEvent &event );
	void OnPasteTrackItem( wxCommandEvent &event );
	void OnAddTrackItem( wxCommandEvent &event );
	void OnAddTrackItemCurve( wxCommandEvent &event );
	void OnAddTrackItemCurves( wxCommandEvent &event ); // adds all available curves
	void OnRemoveTrackItemCurve( wxCommandEvent &event );
	void OnEditTrackItemColor( wxCommandEvent &event );
	TDynArray< CClass* > m_trackItemClasses;

	// Tracks
	CFXTrack *m_trackMouseOver;
	CFXTrack *m_trackMouseOverClicked;

	// Track groups
	CFXTrackGroup *m_trackGroupMouseOver;
	CFXTrackGroup *m_trackGroupMouseOverClicked;
};
