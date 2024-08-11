/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _SS_BOOKMARKS_CTRL_H_
#define _SS_BOOKMARKS_CTRL_H_

#include "widgets/markers.h"

/// Bookmarks panel
class CSSBookmarks: public CSSMarkers
{
	wxDECLARE_CLASS( CSSBookmarks );

public:
	CSSBookmarks( wxWindow* parent, Solution* solution );
	virtual ~CSSBookmarks();

	void GotoNext();
	void GotoPrev();

protected:
	unsigned int m_currBookmarkIdx;

private:
	void Goto( int direction );

	virtual CMarkerToggledEvent* CreateToggleEvent( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state ) const override final;
};

#endif // _SS_BOOKMARKS_CTRL_H_
