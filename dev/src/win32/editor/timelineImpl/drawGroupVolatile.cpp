// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawGroupVolatile.h"
#include "drawBuffer.h"
#include "drawing.h"
#include "../timeline.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================
namespace {
// =================================================================================================

/*
Computes tooltip rectangle that guarantees that tooltip doesn't cross specified borders.

\param text Tooltip text.
\param g Graphics object.
\param font Font used to display the tooltip.
\param pos Position at which we wanted to display the tooltip.
\param topBorder Coordinate of a top border that the tooltip is not allowed to cross.
\param rightBorder Coordinate of a right border that the tooltip is not allowed to cross.
\param maxWidth Max width of tooltip. 0 - any.
\param maxHeight Max height of tooltip. 0 - any.
\return Rectangle specifying position and size of tooltip so that it doesn't cross specified borders.
*/
wxRect GetTooltipRect(const String& text, Gdiplus::Graphics* g, const Gdiplus::Font& font, wxPoint pos, Int32 topBorder, Int32 rightBorder, Int32 maxWidth = 0, Int32 maxHeight = 0)
{
	Gdiplus::RectF layoutRectf(0, 0, maxWidth, maxHeight);
	Gdiplus::RectF rectf;
	g->MeasureString(text.AsChar(), text.GetLength(), &font, layoutRectf, &rectf);

	wxRect rect(pos.x, pos.y - rectf.Height, rectf.Width, rectf.Height);
	if(rect.y < topBorder)
	{
		rect.y = topBorder;
	}
	if(rect.x + rect.width - 1 > rightBorder)
	{
		rect.x = rightBorder - rect.width + 1;
	}

	return rect;
}

/*
Computes tooltip rectangle that guarantees that tooltip doesn't cross specified borders.

\param text Tooltip text.
\param dc Device context.
\param font Font used to display the tooltip.
\param pos Position at which we wanted to display the tooltip.
\param topBorder Coordinate of a top border that the tooltip is not allowed to cross.
\param rightBorder Coordinate of a right border that the tooltip is not allowed to cross.
\param maxWidth Max width of tooltip. 0 - any.
\param maxHeight Max height of tooltip. 0 - any.
\return Rectangle specifying position and size of tooltip so that it doesn't cross specified borders.
*/
wxRect GetTooltipRect(const String& text, wxDC* dc, const wxFont& font, wxPoint pos, Int32 topBorder, Int32 rightBorder, Int32 maxWidth = 0, Int32 maxHeight = 0)
{
	wxCoord w = 0;
	wxCoord h = 0;
	dc->GetMultiLineTextExtent(text.AsChar(), &w, &h, 0, &font);

	wxRect rect(pos.x, pos.y - h, w, h);
	if(rect.y < topBorder)
	{
		rect.y = topBorder;
	}
	if(rect.x + rect.width - 1 > rightBorder)
	{
		rect.x = rightBorder - rect.width + 1;
	}

	return rect;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

void CDrawGroupVolatile::Draw()
{
	Int32 timelineTrackBtnWidth = GetOwner().TIMELINE_TRACK_BTN_WIDTH;

	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	if(!GetDrawBuffer()->GetOwner())
	{
		GetDrawBuffer()->Clear(wxColour(0, 0, 0, 0));
	}
	else
	{
		// buffer is just a wrapper, not need to erase bg
	}

	Int32 animStart = GetOwner().CalculatePixelPos(0.0f);
	Int32 animEnd =   GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration());

	// Draw time position
	Int32 pixCoord = GetOwner().CalculatePixelPos(GetOwner().m_timeSelectorTimePos);
	if(pixCoord > timelineTrackBtnWidth)
	{
		GetDrawBuffer()->DrawLine(pixCoord, 1, pixCoord, height - 2, wxColour(255, 0, 0), 2.0f);
	}

	if(GetDrawBuffer()->GetType() == CanvasType::gdi)
	{
		// Show tooltip
		GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeSelectorTimePos), height),
			String::Printf(TXT("%.2f"), GetOwner().m_timeSelectorTimePos),
			wxColour(255, 0, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
	}
	else
	{
		GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeSelectorTimePos), height),
			String::Printf(TXT("%.2f"), GetOwner().m_timeSelectorTimePos),
			wxColour(255, 0, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
	}
	

	if(GetOwner().m_timeLimitsEnabled)
	{
		// Draw lower limit
		pixCoord = GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMin);
		if(pixCoord > timelineTrackBtnWidth)
		{
			GetDrawBuffer()->DrawLine(pixCoord, 1, pixCoord, height - 2, wxColour(0, 255, 0), 2.0f);
		}

		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			// Show tooltip
			GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMin), height),
				String::Printf(TXT("%.2f"), GetOwner().m_timeLimitMin),
				wxColour(0, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
		}
		else
		{
			// Show tooltip
			GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMin), height),
				String::Printf(TXT("%.2f"), GetOwner().m_timeLimitMin),
				wxColour(0, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
		}

		// Draw upper limit
		pixCoord = GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMax);
		if(pixCoord > timelineTrackBtnWidth)
		{
			GetDrawBuffer()->DrawLine(pixCoord, 1, pixCoord, height - 2, wxColour(0, 0, 255), 2.0f);
		}

		if(GetDrawBuffer()->GetType() == CanvasType::gdi)
		{
			// Show tooltip
			GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMax), height),
				String::Printf(TXT("%.2f"), GetOwner().m_timeLimitMax),
				wxColour(0, 0, 255), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
		}
		else
		{
			// Show tooltip
			GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_timeLimitMax), height),
				String::Printf(TXT("%.2f"), GetOwner().m_timeLimitMax),
				wxColour(0, 0, 255), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
		}
	}

	// Draw mouse position
	if(GetOwner().m_hovered)
	{
		pixCoord = GetOwner().CalculatePixelPos(GetOwner().m_cursorTimePos);

		// Skip lines which are not in animation range
		if(pixCoord >= animStart && pixCoord <= animEnd)
		{
			// Skip invisible lines and these under track buttons
			if(pixCoord > timelineTrackBtnWidth)
			{
				GetDrawBuffer()->DrawLine(pixCoord, 1, pixCoord, height - 2, wxColour(255, 255, 255), 2.0f);

				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_cursorTimePos), height),
						String::Printf(TXT("%.4f"), GetOwner().m_cursorTimePos),
						wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
				}
				else
				{
					// Show tooltip
					GetDrawBuffer()->DrawTooltip(wxPoint(GetOwner().CalculatePixelPos(GetOwner().m_cursorTimePos), height),
						String::Printf(TXT("%.4f"), GetOwner().m_cursorTimePos),
						wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
				}
			}
		}
	}

	// Draw type tooltip
	wxPoint globalPos(GetOwner().m_cursorPos.x, GetOwner().m_cursorPos.y);
	ITimelineItem* item = GetOwner().GetItemAt(globalPos);
	if(item != NULL && ! GetOwner().IsSelected(item))
	{
		String text;
		if( item->GetTooltip(text))
		{
			if(GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				wxRect rect = GetTooltipRect(text, GetDrawBuffer()->GetMemoryDC(), GetOwner().GetWxTooltipFont(), globalPos, 0, width - 1, 300, 0);
				GetDrawBuffer()->DrawTooltip(rect, text, wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetWxTooltipFont());
			}
			else
			{
				wxRect rect = GetTooltipRect(text, GetDrawBuffer()->GetGraphics(), GetOwner().GetTooltipFont(), globalPos, 0, width - 1, 300, 0);
				GetDrawBuffer()->DrawTooltip(rect, text, wxColour(255, 255, 0), wxColour(0, 0, 0), GetOwner().GetTooltipFont());
			}
		}
	}

	// Draw selection range
	if(GetOwner().m_state == CEdTimeline::STATE_SELECTING)
	{
		wxRect rect;

		if(GetOwner().m_selectionStartPoint.x < GetOwner().m_cursorPos.x)
		{
			rect.x = GetOwner().m_selectionStartPoint.x;
			rect.width = GetOwner().m_cursorPos.x - GetOwner().m_selectionStartPoint.x;
		}
		else
		{
			rect.x = GetOwner().m_cursorPos.x;
			rect.width = GetOwner().m_selectionStartPoint.x - GetOwner().m_cursorPos.x;
		}

		if(GetOwner().m_selectionStartPoint.y < GetOwner().m_cursorPos.y)
		{
			rect.y = GetOwner().m_selectionStartPoint.y;
			rect.height = GetOwner().m_cursorPos.y - GetOwner().m_selectionStartPoint.y;
		}
		else
		{
			rect.y = GetOwner().m_cursorPos.y;
			rect.height = GetOwner().m_selectionStartPoint.y - GetOwner().m_cursorPos.y;
		}

		GetDrawBuffer()->DrawRect(rect, wxColour(255, 255, 0, 200));
	}
}

Int32 CDrawGroupVolatile::GetPreferredHeight() const
{
	// volatile group wants to span whole timeline
	Int32 width, height;
	GetOwner().GetClientSize(&width, &height);

	return height;
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
