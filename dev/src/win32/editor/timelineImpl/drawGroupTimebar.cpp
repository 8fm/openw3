// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "drawGroupTimebar.h"
#include "drawBuffer.h"
#include "drawing.h"
#include "../timeline.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

void CDrawGroupTimebar::Draw()
{
	Int32 width, height;
	GetDrawBuffer()->GetSize(width, height);

	Int32 animStart = GetOwner().CalculatePixelPos(0.0f);
	Int32 animEnd =   GetOwner().CalculatePixelPos(GetOwner().GetActiveRangeDuration());

	// Draw time description
	{
		// Draw text background
		GetDrawBuffer()->FillRect(1, height - 35, width - 2, 35, wxColor(20, 20, 20));

		Int32 num = 0;

		for(Float t = 0.0f; t < GetOwner().GetVisibleRangeDuration() - GetOwner().m_activeRangeTimeOffset; t += GetOwner().m_currentGrid / 16.0f, ++num)
		{
			Int32 i = GetOwner().CalculatePixelPos(t);
			
			// Skip lines which are not in animation range
			if(i < animStart || i > animEnd)
			{
				// Check to avoid lock
				if ( GetOwner().CalculatePixelPos(t) == GetOwner().CalculatePixelPos(t + GetOwner().m_currentGrid / 16.0f) )
				{
					break;
				}

				continue;
			}

			// Skip invisible lines and these under track buttons
			if(i < GetOwner().TIMELINE_TRACK_BTN_WIDTH)
			{
				continue;
			}

			if(num % 16 == 0)
			{
				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					GetDrawBuffer()->DrawText(wxPoint(i, height - 20), GetOwner().GetWxDrawFont(), String::Printf(TXT("%.2f" ), t), wxColour(255, 255, 255 ), CEdCanvas::CVA_Center, CEdCanvas::CHA_Center);
				}
				else
				{
					GetDrawBuffer()->DrawText(wxPoint(i, height - 20), GetOwner().GetGdiDrawFont(), String::Printf(TXT("%.2f" ), t), wxColour(255, 255, 255 ), CEdCanvas::CVA_Center, CEdCanvas::CHA_Center);
				}
			}
			else if(num % 4 == 0)
			{
				if(GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					// Draw text if there is enough space
					GetDrawBuffer()->DrawText(wxPoint(i, height - 20), GetOwner().GetWxDrawFont(), String::Printf(TXT( "%.2f"), t), wxColour(255, 255, 255 ), CEdCanvas::CVA_Center, CEdCanvas::CHA_Center);
				}
				else
				{
					// Draw text if there is enough space
					GetDrawBuffer()->DrawText(wxPoint(i, height - 20), GetOwner().GetGdiDrawFont(), String::Printf(TXT( "%.2f"), t), wxColour(255, 255, 255 ), CEdCanvas::CVA_Center, CEdCanvas::CHA_Center);
				}
			}
		}

		// Draw white horizontal line
		GetDrawBuffer()->DrawLine(1, height - 35, width - 2, height - 35, wxColour(255, 255, 255), 2.0f);
	}
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
