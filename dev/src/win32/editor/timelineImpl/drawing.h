// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

// GDI+
void Clear( Gdiplus::Graphics* g, wxColour& color );
void DrawRect(Gdiplus::Graphics* g, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width = 1.0f);
void DrawRect(Gdiplus::Graphics* g, const wxRect& rect, const wxColour& color, Float width = 1.0f);
void FillRect(Gdiplus::Graphics* g, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color);
void FillRect(Gdiplus::Graphics* g, const wxRect& rect, const wxColour& color );
void DrawTriangle(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth = 1.0f);
void FillTriangle(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color);
void DrawCircleCentered(Gdiplus::Graphics* g, const wxPoint &center, Int32 radius, const wxColour& color, Float width = 1.0f);
void FillCircleCentered(Gdiplus::Graphics* g, const wxPoint &center, Int32 radius, const wxColour& color);
void DrawLine(Gdiplus::Graphics* g, Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width = 1.0f);
void DrawText(Gdiplus::Graphics* g, const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color);
void DrawText(Gdiplus::Graphics* g, const wxPoint& topLeft, Uint32 width, Uint32 height, const Gdiplus::Font& font, const String &text, const wxColour &color);
void DrawText(Gdiplus::Graphics* g, const wxPoint& offset, const Gdiplus::Font& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign);
void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, const wxRect& rect);
void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h);
void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y);
void DrawImage(Gdiplus::Graphics* g, Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor);
void DrawTooltip(Gdiplus::Graphics* g, const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font = Gdiplus::Font(L"Arial", 8));
void DrawTooltip(Gdiplus::Graphics* g, const wxRect& rect, String text, const wxColour& colorBackground, const wxColour& colorForeground, const Gdiplus::Font& font = Gdiplus::Font(L"Arial", 8));
wxPoint TextExtents(Gdiplus::Graphics* g, const Gdiplus::Font& font, const String &text);

// GDI
void Clear( wxDC* dc, wxColour& color );
void DrawRect(wxDC* dc, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, Float width = 1.0f);
void DrawRect(wxDC* dc, const wxRect& rect, const wxColour& color, Float width = 1.0f);
void FillRect(wxDC* dc, Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color);
void FillRect(wxDC* dc, const wxRect& rect, const wxColour& color );
void DrawTriangle(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, Float borderWidth = 1.0f);
void FillTriangle(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color);
void DrawCircleCentered(wxDC* dc, const wxPoint &center, Int32 radius, const wxColour& color, Float width = 1.0f);
void FillCircleCentered(wxDC* dc, const wxPoint &center, Int32 radius, const wxColour& color);
void DrawLine(wxDC* dc, Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, Float width = 1.0f);
void DrawText(wxDC* dc, const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color);
void DrawText(wxDC* dc, const wxPoint& topLeft, Uint32 width, Uint32 height, const wxFont& font, const String &text, const wxColour &color);
void DrawText(wxDC* dc, const wxPoint& offset, const wxFont& font, const String &text, const wxColour &color, CEdCanvas::EVerticalAlign vAlign, CEdCanvas::EHorizontalAlign hAlign);
void DrawImage(wxDC* dc, const wxBitmap* bitmap, const wxRect& rect);
void DrawImageWithMask(wxDC* dc, const wxBitmap* bitmap, const wxRect& rect);
void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h);
void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y);
void DrawImage(wxDC* dc, const wxBitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor);
void DrawTooltip(wxDC* dc, const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial"));
void DrawTooltip(wxDC* dc, const wxRect& rect, String text, const wxColour& colorBackground, const wxColour& colorForeground, const wxFont& font = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial"));
wxPoint TextExtents(wxDC* dc, const wxFont& font, const String &text);

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
