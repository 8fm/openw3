/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/**
 * The class allows to pain an icon that represents the state 
 * a file in an active version control system.
 */
class CVersionControlIconsPainter
{
private:
	CEdCanvas&							m_canvas;
	Gdiplus::Bitmap*					m_checkOutIcon;
	Gdiplus::Bitmap*					m_deleteIcon;
	Gdiplus::Bitmap*					m_addIcon;
	Gdiplus::Bitmap*					m_localIcon;
	Gdiplus::Bitmap*					m_checkInIcon;
	Gdiplus::Bitmap*					m_notSyncedIcon;

public:
	CVersionControlIconsPainter( CEdCanvas& canvas );

	/**
	 * Paints the icon.
	 *
	 * @param file	file the state of which we want to represent
	 * @param rect	where should the icon be painted
	 */
	void PaintVersionControlIcon( const CDiskFile *file, const wxRect& rect );
};
