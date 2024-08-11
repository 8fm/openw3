#pragma once

#define Outline_None		0
#define Outline_Top			1
#define Outline_Bottom		2
#define Outline_Left		4
#define Outline_Right		8
#define Outline_Horizontal	(Outline_Top | Outline_Bottom)
#define Outline_Vertical	(Outline_Left | Outline_Right)
#define Outline_All			15

class COutlineGrid : public wxGrid
{
public:

	COutlineGrid(wxWindow *parent);

	void SetOutline(Int32 row, Int32 col, Int32 rowsCount = 1, Int32 colsCount = 1, Int32 outline = Outline_All, wxColour color = wxColour(0, 0, 0), Int32 width = 1, Bool outerBorder = false);
	Int32 GetCellOutline(Int32 row, Int32 col) const;
	Bool GetCellOutlineStyle(Int32 row, Int32 col, Int32 outline, wxColour &color, Int32 &width) const;
	void ClearOutlines(Int32 row=-1, Int32 col=-1, Int32 rowsCount = 1, Int32 colsCount = 1);
	wxRect GetHighlightRange() const { return m_highlightRange; }
	wxColour GetHighlightColor() const { return m_highlightColor; }
	void SetHighlightColor(wxColour color) { m_highlightColor = color; ForceRefresh(); }

protected:

	struct SGridCellOutline
	{
		SGridCellOutline() {}
		SGridCellOutline(Int32 outline, wxColour color, Int32 width);
		void SetStyle(Int32 outline, wxColour color, Int32 width) { m_outline = Outline_None; AddStyle(outline, color, width); }
		void AddStyle(Int32 outline, wxColour color, Int32 width);

		wxColour m_color[4];
		Int32 m_width[4];
		Int32 m_outline;
	};

	THashMap<Uint32, SGridCellOutline> m_outlineCells;
	wxRect m_highlightRange;
	wxColour m_highlightColor;

	void SetHighlightRange(Int32 row, Int32 col, Int32 numRows, Int32 numCols) { SetHighlightRange(wxRect(row, col, numRows, numCols)); }
	void SetHighlightRange(const wxRect &range);

};