// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawing.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

void DrawPlaceholder( wxDC* dc )
{
	wxDCBrushChanger brushChanger(*dc, *wxBLUE_BRUSH);
	dc->DrawRectangle(0, 0, 50, 50);
}

void Clear( Gdiplus::Graphics* g, wxColour& color )
{
	g->Clear(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
}

void DrawRect(Gdiplus::Graphics* g, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width /*= 1.0f */)
{
	Gdiplus::Pen drawPen(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()), width);
	// note we have to adjust args slightly since in GDI+ (x, y) is a point in the center of pixel and not whole pixel
	g->DrawRectangle(&drawPen, x, y, w - 1, h - 1);
}

void DrawRect(Gdiplus::Graphics* g, const wxRect& rect, const wxColour& color, Float width /*= 1.0f */)
{
	Gdiplus::Pen drawPen(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()), width);
	// note we have to adjust args slightly since in GDI+ (x, y) is a point in the center of pixel and not whole pixel
	g->DrawRectangle(&drawPen, rect.x, rect.y, rect.width - 1, rect.height - 1);
}

void FillRect(Gdiplus::Graphics* g, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color)
{
	Gdiplus::SolidBrush drawBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	// note we have to adjust args slightly since in GDI+ (x, y) is a point in the center of pixel and not whole pixel
	g->FillRectangle(&drawBrush, x - 0.5f, y - 0.5f, static_cast<Float>(w), static_cast<Float>(h));
}

void FillRect(Gdiplus::Graphics* g, const wxRect& rect, const wxColour& color)
{
	Gdiplus::SolidBrush drawBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	// note we have to adjust args slightly since in GDI+ (x, y) is a point in the center of pixel and not whole pixel
	g->FillRectangle(&drawBrush, rect.x - 0.5f, rect.y - 0.5f, static_cast<Float>(rect.width), static_cast<Float>(rect.height));
}

void DrawLine(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width /*= 1.0f */)
{
	Gdiplus::Pen drawPen(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()), width);
	g->DrawLine(&drawPen, x1, y1, x2, y2);
}

void DrawText(Gdiplus::Graphics* g, const wxPoint& offset, const Gdiplus::Font& font, const String& text, const wxColour& color)
{
	Gdiplus::SolidBrush textBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	g->DrawString(text.AsChar(), -1, &font, Gdiplus::PointF(offset.x, offset.y), &textBrush);
}

void DrawText(Gdiplus::Graphics* g, const wxPoint& topLeft, Uint32 width, Uint32 height, const Gdiplus::Font& font, const String &text, const wxColour &color)
{
	Gdiplus::SolidBrush textBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	g->DrawString(text.AsChar(), -1, &font,
		Gdiplus::RectF(topLeft.x, topLeft.y, width, height) , NULL, &textBrush);
}

void DrawText(Gdiplus::Graphics* g, const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign)
{
	wxPoint alignedOffset = offset;
	wxPoint stringSize = TextExtents(g, font, text);

	// Calc vertical position
	if(vAlign == CEdCanvas::CVA_Center)
	{
		alignedOffset.y -= stringSize.y / 2;
	}
	else if(vAlign == CEdCanvas::CVA_Bottom)
	{
		alignedOffset.y -= stringSize.y;
	}

	// Calc horizontal position
	if(hAlign == CEdCanvas::CHA_Center)
	{
		alignedOffset.x -= stringSize.x / 2;
	}
	else if(hAlign == CEdCanvas::CHA_Right)
	{
		alignedOffset.x -= stringSize.x;
	}

	// Draw
	Gdiplus::SolidBrush textBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	g->DrawString(text.AsChar(), -1, &font, Gdiplus::PointF(alignedOffset.x, alignedOffset.y), &textBrush);
}

void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, const wxRect& rect)
{
	Gdiplus::Rect drawRect(rect.x, rect.y, rect.width, rect.height);
	g->DrawImage( bitmap, drawRect );
}

void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h)
{
	Gdiplus::Rect rect(x, y, w, h);
	g->DrawImage(bitmap, rect);
}

void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y)
{
	g->DrawImage(bitmap, x, y);
}

void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor)
{
	if(!bitmap)
	{
		return;
	}

	Gdiplus::Rect drawRect(x, y, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::ImageAttributes attributes;
	attributes.SetColorKey(Gdiplus::Color(transparentColor.Alpha(), transparentColor.Red(), transparentColor.Green(), transparentColor.Blue()),
						   Gdiplus::Color(transparentColor.Alpha(), transparentColor.Red(), transparentColor.Green(), transparentColor.Blue()));

	g->DrawImage(bitmap, drawRect, 0, 0, bitmap->GetWidth(), bitmap->GetHeight(), Gdiplus::UnitPixel, &attributes);
}

void FillTriangle(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color)
{
	Gdiplus::GraphicsPath gp;
	gp.AddLine(x1, y1, x2, y2);
	gp.AddLine(x2, y2, x3, y3);
	gp.AddLine(x3, y3, x1, y1);
	gp.CloseFigure();

	Gdiplus::SolidBrush drawBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue())); 
	g->FillPath(&drawBrush, &gp);
}

void DrawTriangle(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth /*= 1.0f */)
{
	Gdiplus::GraphicsPath gp;
	gp.AddLine(x1, y1, x2, y2);
	gp.AddLine(x2, y2, x3, y3);
	gp.AddLine(x3, y3, x1, y1);
	gp.CloseFigure();

	Gdiplus::Pen drawPen(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()), borderWidth); 
	g->DrawPath(&drawPen, &gp);
}

void DrawCircleCentered(Gdiplus::Graphics* g, const wxPoint &center, Int32 radius, const wxColour& color, Float width)
{
	Gdiplus::Pen drawPen(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()), width); 
	g->DrawEllipse(&drawPen, center.x - radius, center.y - radius, radius * 2, radius * 2);
}

void FillCircleCentered(Gdiplus::Graphics* g, const wxPoint &center, Int32 radius, const wxColour& color)
{
	Gdiplus::SolidBrush drawBrush(Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue()));
	g->FillEllipse(&drawBrush, center.x - radius, center.y - radius, radius * 2, radius * 2);
}

/*
Draws tooltip at specified point.

\param point Bottom left corner of tooltip.
*/
void DrawTooltip(Gdiplus::Graphics* g, const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font)
{
	if(text.Empty())
	{
		return;
	}

	Gdiplus::RectF rectf;
	g->MeasureString(text.AsChar(), text.GetLength(), &font, Gdiplus::PointF(0, 0), &rectf);

	wxRect rect(point.x, point.y - rectf.Height, rectf.Width, rectf.Height);
	FillRect(g, rect, colorBackground);
	DrawRect(g, rect, colorForeground);
	DrawText(g, rect.GetTopLeft(), font, text, colorForeground);
}

/*
Draws tooltip in specified rect.

\param rect Tooltip rectangle.
*/
void DrawTooltip(Gdiplus::Graphics* g, const wxRect& rect, String text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font)
{
	if(text.Empty())
	{
		return;
	}

	FillRect(g, rect, colorBackground);
	DrawRect(g, rect, colorForeground);
	DrawText(g, rect.GetTopLeft(), rect.GetWidth(), rect.GetHeight(), font, text, colorForeground);
}

wxPoint TextExtents(Gdiplus::Graphics* g, const Gdiplus::Font& font, const String &text)
{
	if(!text.Empty())
	{
		Gdiplus::RectF boundingRect(0,0,0,0);
		g->MeasureString(text.AsChar(), -1, &font, Gdiplus::PointF(0,0), &boundingRect);
		return wxPoint(boundingRect.Width, boundingRect.Height);
	}
	else
	{
		Gdiplus::RectF boundingRect(0,0,0,0);
		g->MeasureString(TXT("A"), -1, &font, Gdiplus::PointF(0,0), &boundingRect);
		return wxPoint(0, boundingRect.Height);
	}
}

void Clear( wxDC* dc, wxColour& color )
{
	const wxBrush* brush = wxTheBrushList->FindOrCreateBrush(color);
	dc->SetBackground(*brush);
	dc->Clear();
}

void DrawRect(wxDC* dc, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width /* = 1.0f */)
{
	wxPen pen(color, width);
	wxDCPenChanger penChanger(*dc, pen);
	wxDCBrushChanger brushChanger(*dc, *wxTRANSPARENT_BRUSH);

	dc->DrawRectangle(x, y, w, h);
}

void DrawRect(wxDC* dc, const wxRect& rect, const wxColour& color, Float width /* = 1.0f */)
{
	wxPen pen(color, width);
	wxDCPenChanger penChanger(*dc, pen);
	wxDCBrushChanger brushChanger(*dc, *wxTRANSPARENT_BRUSH);

	dc->DrawRectangle(rect);
}

void FillRect(wxDC* dc, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color)
{
	wxDCPenChanger penChanger(*dc, *wxTRANSPARENT_PEN);
	wxBrush* brush = wxTheBrushList->FindOrCreateBrush(color);
	wxDCBrushChanger brushChanger(*dc, *brush);

	dc->DrawRectangle(x, y, w, h);
}
void FillRect(wxDC* dc, const wxRect& rect, const wxColour& color )
{
	wxDCPenChanger penChanger(*dc, *wxTRANSPARENT_PEN);
	wxBrush* brush = wxTheBrushList->FindOrCreateBrush(color);
	wxDCBrushChanger brushChanger(*dc, *brush);
	
	dc->DrawRectangle(rect);
}

void DrawTriangle(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth /* = 1.0f */)
{
	wxDCPenChanger penChanger(*dc, color);
	wxDCBrushChanger brushChanger(*dc, *wxTRANSPARENT_BRUSH);

	wxPoint points[3] = { wxPoint(x1, y1), wxPoint(x2, y2), wxPoint(x3, y3) };
	dc->DrawPolygon(3, points);
}

void FillTriangle(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color)
{
	wxDCPenChanger penChanger(*dc, *wxTRANSPARENT_PEN);
	wxDCBrushChanger brushChanger(*dc, color);

	wxPoint points[3] = { wxPoint(x1, y1), wxPoint(x2, y2), wxPoint(x3, y3) };
	dc->DrawPolygon(3, points);
}

void DrawCircleCentered(wxDC* dc, const wxPoint &center, Int32 radius, const wxColour& color, Float width /* = 1.0f */)
{
	wxDCPenChanger penChanger(*dc, color);
	wxDCBrushChanger brushChanger(*dc, *wxTRANSPARENT_BRUSH);

	dc->DrawCircle(center, radius);
}

void FillCircleCentered(wxDC* dc, const wxPoint &center, Int32 radius, const wxColour& color)
{
	wxDCPenChanger penChanger(*dc, *wxTRANSPARENT_PEN);
	wxDCBrushChanger brushChanger(*dc, color);

	dc->DrawCircle(center, radius);
}

void DrawLine(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width /* = 1.0f */)
{
	wxPen pen(color, width);
	wxDCPenChanger penChanger(*dc, pen);
	
	dc->DrawLine(x1, y1, x2, y2);
}

void DrawText(wxDC* dc, const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color)
{
	wxDCFontChanger fontChanger(*dc, font);
	wxDCTextColourChanger textColorChanger(*dc, color);

	dc->DrawText(text.AsChar(), offset);
}

void DrawText(wxDC* dc, const wxPoint& topLeft, Uint32 width, Uint32 height, const wxFont& font, const String &text, const wxColour &color)
{
	wxDCFontChanger fontChanger(*dc, font);
	wxDCTextColourChanger textColorChanger(*dc, color);

	RECT rect = { topLeft.x, topLeft.y, topLeft.x + width, topLeft.y + height };
	DrawTextW(dc->GetHDC(), text.AsChar(), text.GetLength(), &rect, DT_WORDBREAK);
}

void DrawText(wxDC* dc, const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign)
{
	wxPoint alignedOffset = offset;
	wxPoint stringSize = TextExtents(dc, font, text);

	// Calc vertical position
	if(vAlign == CEdCanvas::CVA_Center)
	{
		alignedOffset.y -= stringSize.y / 2;
	}
	else if(vAlign == CEdCanvas::CVA_Bottom)
	{
		alignedOffset.y -= stringSize.y;
	}

	// Calc horizontal position
	if(hAlign == CEdCanvas::CHA_Center)
	{
		alignedOffset.x -= stringSize.x / 2;
	}
	else if(hAlign == CEdCanvas::CHA_Right)
	{
		alignedOffset.x -= stringSize.x;
	}

	DrawText(dc, alignedOffset, font, text, color);
}

void DrawImage(wxDC* dc, const wxBitmap* bitmap, const wxRect& rect)
{
	// TODO: what if bitmap is already selected into other dc? this should not normally be a problem
	// but it could be if we call DrawImage for one of our buffers
	wxMemoryDC tempDC(dc);
	tempDC.SelectObjectAsSource(*bitmap);
	ASSERT(tempDC.IsOk());

	dc->StretchBlit( rect.x, rect.y, rect.width, rect.height, &tempDC, 0, 0, bitmap->GetWidth(), bitmap->GetHeight());
}

void DrawImageWithMask(wxDC* dc, const wxBitmap* bitmap, const wxRect& rect)
{
	// TODO: what if bitmap is already selected into other dc? this should not normally be a problem
	// but it could be if we call DrawImage for one of our buffers
	wxMemoryDC tempDC(dc);
	tempDC.SelectObjectAsSource(*bitmap);
	ASSERT(tempDC.IsOk());

	dc->StretchBlit( rect.x, rect.y, rect.width, rect.height, &tempDC, 0, 0, bitmap->GetWidth(), bitmap->GetHeight(), wxCOPY, true );
}

void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h)
{
	// TODO: what if bitmap is already selected into other dc? this should not normally be a problem
	// but it could be if we call DrawImage for one of our buffers
	wxMemoryDC tempDC(dc);
	tempDC.SelectObjectAsSource(*bitmap);
	ASSERT(tempDC.IsOk());

	dc->StretchBlit( x, y, w, h, &tempDC, 0, 0, bitmap->GetWidth(), bitmap->GetHeight());
}

void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y)
{
	dc->DrawBitmap(*bitmap, x, y);
}

void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor)
{
	dc->DrawBitmap(*bitmap, x, y);
}

/*
Draws tooltip at specified point.

\param point Bottom left corner of tooltip.
*/
void DrawTooltip(wxDC* dc, const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font /* = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial") */ )
{
	if(text.Empty())
	{
		return;
	}

	wxCoord w = 0;
	wxCoord h = 0;

	dc->GetMultiLineTextExtent(text.AsChar(), &w, &h, 0, &font);

	wxRect rect(point.x, point.y - h, w, h);
	FillRect(dc, rect, colorBackground);
	DrawRect(dc, rect, colorForeground);
	DrawText(dc, rect.GetTopLeft(), font, text, colorForeground);
}

/*
Draws tooltip in specified rect.

\param rect Tooltip rectangle.
*/
void DrawTooltip(wxDC* dc, const wxRect& rect, String text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font /* = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial") */)
{
	if(text.Empty())
	{
		return;
	}

	FillRect(dc, rect, colorBackground);
	DrawRect(dc, rect, colorForeground);
	DrawText(dc, rect.GetTopLeft(), rect.GetWidth(), rect.GetHeight(), font, text, colorForeground);
}

wxPoint TextExtents(wxDC* dc, const wxFont& font, const String& text)
{
	wxCoord w = 0;
	wxCoord h = 0;

	if(!text.Empty())
	{
		dc->GetMultiLineTextExtent(text.AsChar(), &w, &h, 0, &font);
	}
	else
	{
		dc->GetMultiLineTextExtent(TXT("A"), &w, &h, 0, &font);
	}

	return wxPoint(w, h);
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
