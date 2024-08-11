//////////////////////////////////////
//          Inferno Engine          //
// Copyright (C) 2002-2007 by Team  //
//////////////////////////////////////

#pragma once

// background colorn definitions for various editor windows
static wxColor DEFAULT_EDITOR_BACKGROUND = wxColor(80, 80, 80);
static wxColor EXPRESSION_EDITOR_BACKGROUND = wxColor(64,33,39);
static wxColor DIALOG_EDITOR_BACKGROUND = wxColor(72,67,51);
static wxColor BEHAVIOR_EDITOR_BACKGROUND = wxColor(68,61,35);
static wxColor ENTITY_EDITOR_BACKGROUND = wxColor(39,37,50);
static wxColor MATERIAL_EDITOR_BACKGROUND = wxColor(109,86,127);
static wxColor REACTION_EDITOR_BACKGROUND = wxColor(39,37,50);
static wxColor BEHTREE_EDITOR_BACKGROUND = wxColor(67,48,51);
static wxColor STEERING_EDITOR_BACKGROUND = wxColor(67,48,51);

enum class CanvasType
{
	gdi,
	gdiplus
};

/// GDI+ based buffered canvas
class CEdCanvas : public wxWindow, public CDropTarget
{
	DECLARE_EVENT_TABLE()

public:
	enum EVerticalAlign
	{
		CVA_Top,
		CVA_Center,
		CVA_Bottom,
	};
	enum EHorizontalAlign
	{
		CHA_Left,
		CHA_Center,
		CHA_Right,
	};

private:
	Bool				m_paintCached;
	Float				m_scale;
	Float				m_invScale;
	wxPoint				m_offset;
	wxRect				m_clip;

protected:
	Gdiplus::Bitmap*	m_bitmapBuffer;
	Gdiplus::Graphics*	m_bufferDevice;
	Int32					m_lastWidth;
	Int32					m_lastHeight;
	wxFont				m_wxDrawFont;
	Gdiplus::Font*		m_drawFont;
	wxFont				m_wxBoldFont;
	Gdiplus::Font*		m_boldFont;
	wxFont				m_wxTooltipFont;
	Gdiplus::Font*		m_tooltipFont;
	Bool				m_isMouseCaptured;
	Bool				m_isHardCaptured;
	wxPoint				m_preCapturePosition;
	wxPoint				m_lastClickPoint;

public:
	CEdCanvas( wxWindow *parent, long style = 0, CanvasType canvasType = CanvasType::gdiplus );
	~CEdCanvas();

	RED_INLINE Float GetScale() const { return m_scale; }
	RED_INLINE wxPoint GetOffset() const { return m_offset; }
	RED_INLINE wxSize GetCanvasSize() const { return wxSize(m_lastWidth, m_lastHeight); }

	void Repaint( Bool immediate = false );
	void CaptureMouse( Bool state, Bool hard );
	void SetScale( Float scale, Bool repaint = true );
	void SetOffset( wxPoint offset, Bool repaint = true );

public:
	// Space conversion
	wxPoint CanvasToClient( wxPoint point ) const;
	wxPoint ClientToCanvas( wxPoint point ) const;
	wxRect CanvasToClient( wxRect rect ) const;
	wxRect ClientToCanvas( wxRect rect ) const;

	// Calculate text extents
	wxPoint TextExtents( Gdiplus::Font& font, const String &text );
	wxRect TextRect( Gdiplus::Font& font, const String &text );

	// Clipping
	void SetClip( const wxRect &rect );
	void ResetClip();
	const wxRect& GetClip() const ;

	// Drawing
	void Clear( wxColour& color );
	void DrawText( const wxPoint& offset, Gdiplus::Font& font, const String &text, const wxColour &color );
	void DrawText( const wxPoint& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void DrawText( const wxPoint& topLeft, Uint32 width, Uint32 height, Gdiplus::Font& font, const String &text, const wxColour &color );
	void DrawTooltip(const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground);
	void DrawRect( const wxRect& rect, const wxColour& color, Float width = 1.0f );
	void DrawRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width = 1.0f );
	void FillRect( const wxRect& rect, const wxColour& color );
	void FillRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color );
	void FillGradientHorizontalRect( const wxRect& rect, const wxColour& leftColor, const wxColour& rightColor );
	void FillGradientVerticalRect( const wxRect& rect, const wxColour& upColor, const wxColour& downColor );
	void FillGradientVerticalRect( const wxRect& rect, const wxColour& upColor, const wxColour& downColor, const wxColour& middleColor );
	void DrawLine( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width = 1.0f );
	void DrawLine( const wxPoint& start, const wxPoint &end, const wxColour& color, Float width = 1.0f );
	void DrawLines( const wxPoint* points, const Int32 numPoints, const wxColour& color, Float width = 1.0f );
	void DrawArc( const wxRect &rect, Float startAngle, Float sweepAngle, const wxColour& color, Float width = 1.0f );
	void DrawCircle( Int32 x, Int32 y, Int32 radius, const wxColour& color, Float width = 1.0f );
	void DrawCircle( const wxPoint &center, Int32 radius, const wxColour& color, Float width = 1.0f );
	void DrawCircleCentered( const wxPoint &center, Int32 radius, const wxColour& color, Float width = 1.0f );
	void FillCircle( Int32 x, Int32 y, Int32 radius, const wxColour& color );
	void FillCircle( const wxPoint &topLeft, Int32 radius, const wxColour& color );
	void FillCircleCentered( const wxPoint &center, Int32 radius, const wxColour& color );
	void DrawCircle( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width = 1.0f );
	void DrawCircle( const wxRect& rect, const wxColour& color, Float width = 1.0f );
	void FillCircle( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color );
	void FillCircle( const wxRect& rect, const wxColour& color );
	void DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y );
	void DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor );
	void DrawImage( Gdiplus::Bitmap* bitmap, const wxPoint& point );
	void DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h );
	void DrawImage( Gdiplus::Bitmap* bitmap, const wxRect& rect );
	void DrawCurve( const wxPoint *points, const wxColour& color, Float width = 1.0f );
	void DrawCardinalCurve( const wxPoint *points, Int32 numPoints, const wxColour& color, Float width = 1.0f );
	void DrawRoundedRect( const wxRect& rect, const wxColour& color, Int32 radius, Float borderWidth = 1.0f );
	void DrawRoundedRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Int32 radius, Float borderWidth = 1.0f );
	void FillRoundedRect( const wxRect& rect, const wxColour& color, Int32 radius );
	void FillRoundedRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Int32 radius );
	void DrawCuttedRect( const wxRect& rect, const wxColour& color, Int32 radius, Float borderWidth = 1.0f );
	void DrawCuttedRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Int32 radius, Float borderWidth = 1.0f );
	void FillCuttedRect( const wxRect& rect, const wxColour& color, Int32 radius );
	void FillCuttedRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Int32 radius );
	void DrawDownArrow( const wxRect& rect, const wxColour& backColor, const wxColour& borderColor );
	void DrawUpArrow( const wxRect& rect, const wxColour& backColor, const wxColour& borderColor );
	void DrawTriangle( Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth = 1.0f );
	void FillTriangle( Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color );
	void DrawTipRect( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& borderColor, const wxColour& interiorColor, Uint32 size );
	void DrawCross( const wxRect& rect, const wxColour& color, Float width = 1.0f );
	void DrawPoly( const wxPoint *points, Uint32 numPoints, const wxColour& borderColor, Float borderWidth = 1.0f );
    void DrawPie( const wxRect &rect, Float startAngle, Float sweepAngle, const wxColour& color, FLOAT width );
	void FillPoly( const wxPoint *points, Uint32 numPoints, const wxColour& color );

	// Fonts
	RED_INLINE wxFont& GetWxDrawFont() { return m_wxDrawFont; }
	RED_INLINE Gdiplus::Font& GetGdiDrawFont() { return *m_drawFont; }
	RED_INLINE wxFont& GetWxBoldFont() { return m_wxBoldFont; }
	RED_INLINE Gdiplus::Font& GetGdiBoldFont() { return *m_boldFont; }
	RED_INLINE wxFont& GetWxTooltipFont() { return m_wxTooltipFont; }
	RED_INLINE Gdiplus::Font& GetTooltipFont() { return *m_tooltipFont; }

	// Hit tests
	Bool HitTestCurve( const wxPoint *points, const wxPoint& point, const Float range = 1.0f );

	// Conversion from wxBitmap into a GDI+ bitmap
	static Gdiplus::Bitmap* ConvertToGDI(const wxBitmap &bmp);
	static Gdiplus::Bitmap *ChangeImageColor( Gdiplus::Bitmap *bitmap, const wxColor &color );

	// Color conversion
	static wxColor ConvertToGrayscale(const wxColor &color);
	static wxColor HSVToRGB(Float hue, Float sat, Float val);
	static void RGBToHSV(const wxColor &color, Float &hue, Float &sat, Float &val);

public:
	virtual void PaintCanvas( Int32 width, Int32 height );
	virtual void MouseClick( wxMouseEvent& event );
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );

protected:
	void UpdateTransformMatrix();
	virtual void RecreateDevice( Int32 newWidth, Int32 newHeight );

protected:
	//! Events
	void OnPaint( wxPaintEvent &event );
	void OnSize( wxSizeEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnMouseEvent( wxMouseEvent& event );
	void OnMouseMove( wxMouseEvent& event );

	//! Returns background color
	virtual wxColor GetCanvasColor() const { return wxColor(80, 80, 80); }

	CanvasType GetCanvasType() const { return m_canvasType; }
	CanvasType m_canvasType;

	wxBitmap* m_bitmap;									// buffer for drawing
	wxMemoryDC* m_memDC;								// memory device context
};
