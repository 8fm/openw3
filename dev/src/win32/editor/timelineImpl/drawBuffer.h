// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

class CEdTimeline;

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

class CDrawBuffer
{
public:
	CDrawBuffer(CanvasType type, wxDC* dc);
	CDrawBuffer(CanvasType type, CEdTimeline* owner);
	~CDrawBuffer();

	Gdiplus::Bitmap* GetBitmap();
	Gdiplus::Graphics* GetGraphics();

	wxMemoryDC* GetMemoryDC();
	wxBitmap* GetWxBitmap();

	Bool IsValid() const;
	void SetValid(Bool state);

	void GetSize(Int32& width, Int32& height) const;
	void Resize(Int32 width, Int32 height);

	void GetTargetPos(wxPoint& targetPos) const;
	void SetTargetPos(wxPoint targetPos);

	Int32 GetVerticalOffset() const;
	void SetVerticalOffset(Int32 voff);

	void Clear( wxColour& color );
	void DrawRect(Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width = 1.0f);
	void DrawRect(const wxRect& rect, const wxColour& color, Float width = 1.0f);
	void FillRect(Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color);
	void FillRect(const wxRect& rect, const wxColour& color );
	void DrawTriangle(Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth = 1.0f);
	void FillTriangle(Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color);
	void DrawCircleCentered(const wxPoint &center, Int32 radius, const wxColour& color, Float width = 1.0f);
	void FillCircleCentered(const wxPoint &center, Int32 radius, const wxColour& color);
	void DrawLine(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width = 1.0f);
	
	void DrawText(const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color);
	void DrawText(const wxPoint& topLeft, Uint32 width, Uint32 height, const wxFont& font, const String &text, const wxColour &color);
	void DrawText(const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign);
	
	void DrawText(const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color);
	void DrawText(const wxPoint& topLeft, Uint32 width, Uint32 height, const Gdiplus::Font& font, const String &text, const wxColour &color);
	void DrawText(const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign);
	
	void DrawImage(const wxBitmap* bitmap, const wxRect& rect);
	void DrawImageWithMask(const wxBitmap* bitmap, const wxRect& rect);
	void DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h);
	void DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y);
	void DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor);

	void DrawImage(Gdiplus::Bitmap* bitmap, const wxRect& rect);
	void DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h);
	void DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y);
	void DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor);

	void DrawTooltip(const wxPoint& point, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial"));
	void DrawTooltip(const wxRect& rect, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial"));

	void DrawTooltip(const wxPoint& point, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font = Gdiplus::Font(L"Arial", 8));
	void DrawTooltip(const wxRect& rect, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font = Gdiplus::Font(L"Arial", 8));
	
	wxPoint TextExtents(const wxFont& font, const String &text);
	wxPoint TextExtents(const Gdiplus::Font& font, const String &text);

	CEdTimeline* GetOwner() const { return m_owner; }
	CanvasType GetType() const { return m_type; }

private:
	CDrawBuffer(const CDrawBuffer&);				// cctor - not defined
	CDrawBuffer& operator=(const CDrawBuffer&);		// op= - not defined

	void DestroyGraphicsAndBitmap();

	CanvasType			m_type;				// gdiplus or gdi

	// these are only for gdiplus type buffers
	Gdiplus::Graphics*	m_graphics;
	Gdiplus::Bitmap*	m_bitmap;

	// these are only for gdi type buffers
	wxMemoryDC*			m_memDC;
	wxBitmap*			m_wxBitmap;

	Int32				m_width;
	Int32				m_height;
	wxPoint				m_targetPos;
	Int32				m_vertOff;
	Bool				m_isValid;

	CEdTimeline*		m_owner; // if != nullptr then draw buffer is just a wrapper around memDC + bitmap (or gdiplus graphics + bitmap) of owner
};

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE Bool CDrawBuffer::IsValid() const
{
	return m_isValid;
}

RED_INLINE void CDrawBuffer::SetValid(Bool state)
{
	m_isValid = state;
}

RED_INLINE void CDrawBuffer::GetSize(Int32& width, Int32& height) const
{
	width = m_width;
	height = m_height;
}

RED_INLINE void CDrawBuffer::GetTargetPos(wxPoint& targetPos) const
{
	targetPos = m_targetPos;
}

RED_INLINE void CDrawBuffer::SetTargetPos(wxPoint targetPos)
{
	m_targetPos = targetPos;
}

RED_INLINE Int32 CDrawBuffer::GetVerticalOffset() const
{
	return m_vertOff;
}

RED_INLINE void CDrawBuffer::SetVerticalOffset(Int32 voff)
{
	m_vertOff = voff;
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
