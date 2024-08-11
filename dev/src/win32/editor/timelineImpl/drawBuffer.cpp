// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawBuffer.h"
#include "drawing.h"
#include "../timeline.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

/*
\param dc Used only when type is CanvasType::gdi to create memory dc compatible with given dc.
Argument dc must not be nullptr in such case. 
*/
CDrawBuffer::CDrawBuffer(CanvasType type, wxDC* dc)
: m_type(type)
, m_graphics(0)
, m_bitmap(0)
, m_memDC(nullptr)
, m_wxBitmap(nullptr)
, m_width(0)
, m_height(0)
, m_targetPos(0, 0)
, m_vertOff(0)
, m_isValid(false)
, m_owner(nullptr)
{
	if(m_type == CanvasType::gdi)
	{
		ASSERT(dc);
		m_memDC = new wxMemoryDC(dc);
	}
}

CDrawBuffer::CDrawBuffer(CanvasType type, CEdTimeline* owner)
: m_type(type)
, m_graphics(0)
, m_bitmap(0)
, m_memDC(nullptr)
, m_wxBitmap(nullptr)
, m_width(0)
, m_height(0)
, m_targetPos(0, 0)
, m_vertOff(0)
, m_isValid(false)
, m_owner(owner)
{
	ASSERT(owner);
	// no need to do anything
}

/*
TODO: change name - if type is gdi then this destroys memory dc and wxBitmap
*/
void CDrawBuffer::DestroyGraphicsAndBitmap()
{
	if(m_owner)
	{
		// we have an owner - nothing to do for us
		return;
	}

	if( m_type == CanvasType::gdiplus)
	{
		delete m_graphics;
		m_graphics = 0;

		delete m_bitmap;
		m_bitmap = 0;
	}
	else
	{
		// don't delete memory dc - we can reuse it
		m_memDC->SelectObject(wxNullBitmap);

		delete m_wxBitmap;
		m_wxBitmap = 0;
	}

	m_width = 0;
	m_height = 0;
}

void CDrawBuffer::Resize(Int32 width, Int32 height)
{
	if(m_owner)
	{
		// we have an owner - there only so much to do
		m_width = width;
		m_height = height;
		SetValid(false);

		return;
	}

	if(m_type == CanvasType::gdiplus)
	{
		// draw device and draw buffer exist or they don't exist because they were not yet created which means current size is (0, 0)
		ASSERT((m_bitmap && m_graphics) || (m_bitmap == 0 && m_graphics == 0 && m_width == 0 && m_height == 0));
	}
	else
	{
		ASSERT((m_wxBitmap) || (m_wxBitmap == 0 && m_width == 0 && m_height == 0));
	}
	
	if(width != m_width || height != m_height)
	{
		// destroy draw buffer and draw device before resizing
		DestroyGraphicsAndBitmap();

		// draw device and draw buffer mustn't exist before recreating them
		if(m_type == CanvasType::gdiplus)
		{
			ASSERT(m_graphics == 0 && m_bitmap == 0);

			m_bitmap = new Gdiplus::Bitmap(width, height);
			m_graphics = new Gdiplus::Graphics(m_bitmap);

			// setup rendering options, highest quality
			m_graphics->SetInterpolationMode(Gdiplus::InterpolationModeDefault);
			m_graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
			m_graphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
			m_graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		}
		else
		{
			ASSERT(m_wxBitmap == 0); // don't check m_memDC as it can exist

			m_wxBitmap = new wxBitmap( width, height );
			m_memDC->SelectObject(*m_wxBitmap);
		}

		m_width = width;
		m_height = height;

		SetValid(false);
	}
}

void CDrawBuffer::Clear( wxColour& color )
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::Clear(GetGraphics(), color);
	}
	else
	{
		TimelineImpl::Clear(GetMemoryDC(), color);
	}
}

void CDrawBuffer::DrawRect(Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width /* = 1.0f */)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::DrawRect(GetGraphics(), x, y, w, h, color, width);
	}
	else
	{
		TimelineImpl::DrawRect(GetMemoryDC(), x, y, w, h, color, width);
	}
}

void CDrawBuffer::DrawRect(const wxRect& rect, const wxColour& color, Float width /* = 1.0f */)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::DrawRect(GetGraphics(), rect, color, width);
	}
	else
	{
		TimelineImpl::DrawRect(GetMemoryDC(), rect, color, width);
	}
}

void CDrawBuffer::FillRect(Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::FillRect(GetGraphics(), x, y, w, h, color);
	}
	else
	{
		TimelineImpl::FillRect(GetMemoryDC(), x, y, w, h, color);
	}
}

void CDrawBuffer::FillRect(const wxRect& rect, const wxColour& color )
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::FillRect(GetGraphics(), rect, color );
	}
	else
	{
		TimelineImpl::FillRect(GetMemoryDC(), rect, color );
	}
}

void CDrawBuffer::DrawTriangle(Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth /* = 1.0f */)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::DrawTriangle(GetGraphics(), x1, y1, x2, y2, x3, y3, color, borderWidth);
	}
	else
	{
		TimelineImpl::DrawTriangle(GetMemoryDC(), x1, y1, x2, y2, x3, y3, color, borderWidth);
	}
}

void CDrawBuffer::FillTriangle(Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::FillTriangle(GetGraphics(), x1, y1, x2, y2, x3, y3, color);
	}
	else
	{
		TimelineImpl::FillTriangle(GetMemoryDC(), x1, y1, x2, y2, x3, y3, color);
	}
}

void CDrawBuffer::DrawCircleCentered(const wxPoint &center, Int32 radius, const wxColour& color, Float width /* = 1.0f */)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::DrawCircleCentered(GetGraphics(), center, radius, color, width);
	}
	else
	{
		TimelineImpl::DrawCircleCentered(GetMemoryDC(), center, radius, color, width);
	}
}

void CDrawBuffer::FillCircleCentered(const wxPoint &center, Int32 radius, const wxColour& color)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::FillCircleCentered(GetGraphics(), center, radius, color);
	}
	else
	{
		TimelineImpl::FillCircleCentered(GetMemoryDC(), center, radius, color);
	}
}

void CDrawBuffer::DrawLine(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width /* = 1.0f */)
{
	if(m_type == CanvasType::gdiplus)
	{
		TimelineImpl::DrawLine(GetGraphics(), x1, y1, x2, y2, color, width);
	}
	else
	{
		TimelineImpl::DrawLine(GetMemoryDC(), x1, y1, x2, y2, color, width);
	}
}

void CDrawBuffer::DrawText(const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color)
{
	ASSERT(m_type == CanvasType::gdi);
	TimelineImpl::DrawText(GetMemoryDC(), offset, font, text, color);
}

void CDrawBuffer::DrawText(const wxPoint& topLeft, Uint32 width, Uint32 height, const wxFont& font, const String &text, const wxColour &color)
{
	ASSERT(m_type == CanvasType::gdi);
	TimelineImpl::DrawText(GetMemoryDC(), topLeft, width, height, font, text, color);
}

void CDrawBuffer::DrawText(const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign)
{
	ASSERT(m_type == CanvasType::gdi);
	TimelineImpl::DrawText(GetMemoryDC(), offset, font, text, color, vAlign, hAlign);
}

void CDrawBuffer::DrawText(const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color)
{
	ASSERT(m_type == CanvasType::gdiplus);
	TimelineImpl::DrawText(GetGraphics(), offset, font, text, color);
}

void CDrawBuffer::DrawText(const wxPoint& topLeft, Uint32 width, Uint32 height, const Gdiplus::Font& font, const String &text, const wxColour &color)
{
	ASSERT(m_type == CanvasType::gdiplus);
	TimelineImpl::DrawText(GetGraphics(), topLeft, width, height, font, text, color);
}

void CDrawBuffer::DrawText(const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign)
{
	ASSERT(m_type == CanvasType::gdiplus);
	TimelineImpl::DrawText(GetGraphics(), offset, font, text, color, vAlign, hAlign);
}

void CDrawBuffer::DrawImage(const wxBitmap* bitmap, const wxRect& rect)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawImage(GetMemoryDC(), bitmap, rect);
}

void CDrawBuffer::DrawImageWithMask(const wxBitmap* bitmap, const wxRect& rect)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawImageWithMask(GetMemoryDC(), bitmap, rect);
}

void CDrawBuffer::DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawImage(GetMemoryDC(), bitmap, x, y, w, h);
}

void CDrawBuffer::DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawImage(GetMemoryDC(), bitmap, x, y);
}

void CDrawBuffer::DrawImage(const wxBitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawImage(GetMemoryDC(), bitmap, x, y, transparentColor);
}

void CDrawBuffer::DrawImage(Gdiplus::Bitmap* bitmap, const wxRect& rect)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawImage(GetGraphics(), bitmap, rect);
}

void CDrawBuffer::DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawImage(GetGraphics(), bitmap, x, y, w, h);
}

void CDrawBuffer::DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawImage(GetGraphics(), bitmap, x, y);
}

void CDrawBuffer::DrawImage(Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawImage(GetGraphics(), bitmap, x, y, transparentColor);
}

void CDrawBuffer::DrawTooltip(const wxPoint& point, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font /* = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial") */)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawTooltip(GetMemoryDC(), point, text, colorBackground, colorForeground, font);
}

void CDrawBuffer::DrawTooltip(const wxRect& rect, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font /* = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial") */)
{
	ASSERT(GetType() == CanvasType::gdi);
	TimelineImpl::DrawTooltip(GetMemoryDC(), rect, text, colorBackground, colorForeground, font);
}

void CDrawBuffer::DrawTooltip(const wxPoint& point, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font /* = Gdiplus::Font(L"Arial", 8) */)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawTooltip(GetGraphics(), point, text, colorBackground, colorForeground, font);
}

void CDrawBuffer::DrawTooltip(const wxRect& rect, const String& text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font /* = Gdiplus::Font(L"Arial", 8) */)
{
	ASSERT(GetType() == CanvasType::gdiplus);
	TimelineImpl::DrawTooltip(GetGraphics(), rect, text, colorBackground, colorForeground, font);
}

wxPoint CDrawBuffer::TextExtents(const wxFont& font, const String &text)
{
	ASSERT(m_type == CanvasType::gdi);
	return TimelineImpl::TextExtents(GetMemoryDC(), font, text);
}

wxPoint CDrawBuffer::TextExtents(const Gdiplus::Font& font, const String &text)
{
	ASSERT(m_type == CanvasType::gdiplus);
	return TimelineImpl::TextExtents(GetGraphics(), font, text);
}

CDrawBuffer::~CDrawBuffer()
{
	DestroyGraphicsAndBitmap();

	if(!m_owner)
	{
		// delete memory dc as it's not deleted by DestroyGraphicsAndBitmap()
		delete m_memDC;
	}
	// else
		// don't delete memory dc as we don't own it
}

Gdiplus::Bitmap* CDrawBuffer::GetBitmap()
{
	if(m_owner)
	{
		return m_owner->m_bitmapBuffer;
	}
	else
	{
		return m_bitmap;
	}
}

Gdiplus::Graphics* CDrawBuffer::GetGraphics()
{
	if(m_owner)
	{
		return m_owner->m_bufferDevice;
	}
	else
	{
		return m_graphics;
	}
}

wxMemoryDC* CDrawBuffer::GetMemoryDC()
{
	if(m_owner)
	{
		return m_owner->m_memDC;
	}
	else
	{
		return m_memDC;
	}
}

wxBitmap* CDrawBuffer::GetWxBitmap()
{
	if(m_owner)
	{
		return m_owner->m_bitmap;
	}
	else
	{
		return m_wxBitmap;
	}
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
