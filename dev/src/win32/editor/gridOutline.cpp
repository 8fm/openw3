#include "build.h"
#include "gridOutline.h"
#include "gridCellEditors.h"

#define HASH_COORDS(row, col)	(Uint32)((((row) & 0xffff) << 16) | ((col) & 0xffff))

COutlineGrid::COutlineGrid(wxWindow *parent)
: wxGrid(parent, wxID_ANY)
, m_highlightColor(0, 0, 0)
{
	SetDefaultRenderer(new CGridCellDefaultRenderer);
}

COutlineGrid::SGridCellOutline::SGridCellOutline(Int32 outline, wxColour color, Int32 width)
{
	SetStyle(outline, color, width);
}

void COutlineGrid::SGridCellOutline::AddStyle(Int32 outline, wxColour color, Int32 width)
{
	m_outline |= outline;

	if (outline & Outline_Top)
	{
		m_color[0] = color;
		m_width[0] = width;
	}
	if (outline & Outline_Bottom)
	{
		m_color[1] = color;
		m_width[1] = width;
	}
	if (outline & Outline_Left)
	{
		m_color[2] = color;
		m_width[2] = width;
	}
	if (outline & Outline_Right)
	{
		m_color[3] = color;
		m_width[3] = width;
	}
}

void COutlineGrid::SetOutline(Int32 row, Int32 col, Int32 rowsCount, Int32 colsCount, Int32 outline, wxColour color, Int32 width, Bool outerBorder)
{
	SGridCellOutline *cellOutline = NULL;
	if (outerBorder)
	{
		// Set border flag (border is an outer outline)
		if (outline & Outline_Top || outline & Outline_Bottom)
		{
			for (Int32 i = col; i < col + colsCount; ++i)
			{
				if (outline & Outline_Top)
				{
					Uint32 hashCoords = HASH_COORDS(row, i);
					if (cellOutline = m_outlineCells.FindPtr(hashCoords))
						cellOutline->AddStyle(Outline_Top, color, width);
					else
						m_outlineCells.Insert(hashCoords, SGridCellOutline(Outline_Top, color, width));
				}

				if (outline & Outline_Bottom)
				{
					Uint32 hashCoords = HASH_COORDS(row + rowsCount - 1, i);
					if (cellOutline = m_outlineCells.FindPtr(hashCoords))
						cellOutline->AddStyle(Outline_Bottom, color, width);
					else
						m_outlineCells.Insert(hashCoords, SGridCellOutline(Outline_Bottom, color, width));
				}
			}
		}

		if (outline & Outline_Left || outline & Outline_Right)
		{
			for (Int32 i = row; i < row + rowsCount; ++i)
			{
				if (outline & Outline_Left)
				{
					Uint32 hashCoords = HASH_COORDS(i, col);
					if (cellOutline = m_outlineCells.FindPtr(hashCoords))
						cellOutline->AddStyle(Outline_Left, color, width);
					else
						m_outlineCells.Insert(hashCoords, SGridCellOutline(Outline_Left, color, width));
				}

				if (outline & Outline_Right)
				{
					Uint32 hashCoords = HASH_COORDS(i, col + colsCount - 1);
					if (cellOutline = m_outlineCells.FindPtr(hashCoords))
						cellOutline->AddStyle(Outline_Right, color, width);
					else
						m_outlineCells.Insert(hashCoords, SGridCellOutline(Outline_Right, color, width));
				}
			}
		}
	}
	else
	{
		// Set internal outline (for all cells in given range)
		for (Int32 i = row; i < row + rowsCount; ++i)
		{
			for (Int32 j = col; j < col + colsCount; ++j)
			{
				Uint32 hashCoords = HASH_COORDS(i, j);
				if (cellOutline = m_outlineCells.FindPtr(hashCoords))
					cellOutline->AddStyle(outline, color, width);
				else
					m_outlineCells.Insert(hashCoords, SGridCellOutline(outline, color, width));
			}
		}
	}
}

Int32 COutlineGrid::GetCellOutline(Int32 row, Int32 col) const
{
	THashMap<Uint32, SGridCellOutline>::const_iterator it = m_outlineCells.Find(HASH_COORDS(row, col));
	if (it != m_outlineCells.End())
		return (*it).m_second.m_outline;

	return Outline_None;
}

Bool COutlineGrid::GetCellOutlineStyle(Int32 row, Int32 col, Int32 outline, wxColour &color, Int32 &width) const
{
	THashMap<Uint32, SGridCellOutline>::const_iterator it = m_outlineCells.Find(HASH_COORDS(row, col));
	if (it != m_outlineCells.End())
	{
		Int32 index;
		if (outline == Outline_Top) index = 0;
		else if (outline == Outline_Bottom) index = 1;
		else if (outline == Outline_Left) index = 2;
		else if (outline == Outline_Right) index = 3;
		else
			return false;

		color = (*it).m_second.m_color[index];
		width = (*it).m_second.m_width[index];
		return true;
	}

	return false;
}

void COutlineGrid::ClearOutlines(Int32 row, Int32 col, Int32 rowsCount, Int32 colsCount)
{
	if (row < 0 || col < 0)
	{
		// Clear all outlines
		m_outlineCells.Clear();
	}
	else
	{
		// Clear actual outline flag
		for (Int32 x = col; x < col + colsCount; ++x)
		{
			for (Int32 y = row; y < row + rowsCount; ++y)
			{
				m_outlineCells.Erase(HASH_COORDS(row, col));
			}
		}
	}
}

void COutlineGrid::SetHighlightRange(const wxRect &range)
{
	if (m_highlightRange != range)
	{
		m_highlightRange = range;
		ForceRefresh();
	}
}
