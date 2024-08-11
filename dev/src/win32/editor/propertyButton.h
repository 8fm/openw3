//////////////////////////////////////
//          Inferno Engine          //
// Copyright (C) 2002-2007 by Team  //
//////////////////////////////////////

#pragma once

class CBasePropItem;

/// Property grid item button
class CPropertyButton : public wxEvtHandler
{
protected:
	CBasePropItem*			m_item;
	wxBitmap				m_bitmap;
	wxRect					m_rect;
	Uint32					m_width;

public:
	CPropertyButton( CBasePropItem* owner, const wxBitmap& bitmap, Uint32 width );

	RED_INLINE CBasePropItem* GetItem() const { return m_item; }
	RED_INLINE const wxRect& GetRect() const { return m_rect; }
	RED_INLINE const wxBitmap& GetBitmap() const { return m_bitmap; }
	RED_INLINE Int32 GetWidth() const { return m_width; }

	bool IsSelected() const;
	virtual void UpdateLayout( Int32 &xOffset, Int32 y, Int32 height );
	virtual void DrawLayout( wxDC& dc );
	virtual void Clicked();
};