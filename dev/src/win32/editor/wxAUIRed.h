// Copyright © 2013 CD Projekt Red. All Rights Reserved.


#pragma once


#include <wx/wx.h>
#include <wx/aui/aui.h>
#include "../../common/core/dynarray.h"



/// @see wxAuiManager
class CEdAuiManager : public wxAuiManager
{
public:

	/// constructor same as in wxAuiManager
	CEdAuiManager(wxWindow* managedWnd = NULL, Uint32 flags = wxAUI_MGR_DEFAULT);


	/// @brief allows saving layout including docking of tabs
	wxString SaveWholeLayout();
	/// @brief allow loading layout including docking of tabs
	/// @return false if loading failed
	Bool LoadWholeLayout(const wxString& wholeLayoutString, Bool update = true);
	Bool LoadWholeLayout(const String& wholeLayoutString, Bool update = true);
};





class CEdAuiNotebook : public wxAuiNotebook
{
public:

	/// constructor same as in wxAuiNotebook
	CEdAuiNotebook(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxAUI_NB_DEFAULT_STYLE
	);


	/// @brief save tabs docking for this notebook
	wxString SavePerspective();
	/// @brief load tabs dockign for this notebook
	Bool LoadPerspective(const wxString& layout);




private:


	/// @brief Remove all tab ctrls (but still keep them in main index)
	/// @param windowsLeft - copy of all windows that are in the main index
	Bool RemoveAllTabCtrls( TDynArray<wxWindow*>& windowsLeft );

	/// create a new tab frame
	wxAuiTabCtrl* CreateTabFrameForLoad( const wxString& paneName );

};






inline Bool CEdAuiManager::LoadWholeLayout( const String& wholeLayoutString, Bool update /*= true*/ )
{
	return LoadWholeLayout(wxString(wholeLayoutString.AsChar()), update);
}

