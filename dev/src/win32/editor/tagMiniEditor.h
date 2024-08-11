/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef __CEDTAGMINIEDITOR_h__
#define __CEDTAGMINIEDITOR_h__

#include "treeListCtrl.h"
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include "treelistctrl.h"

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_TAGEDITOR_SELECT, wxEVT_USER_FIRST + 3 )
END_DECLARE_EVENT_TYPES()

class  CTagListProvider;
struct STagNode;
typedef TDynArray< CTagListProvider* > TagProviderArray;

/// Editor of tags
class CEdTagMiniEditor : public wxDialog
{
private:
	DECLARE_EVENT_TABLE()

public:
	CEdTagMiniEditor( wxWindow* parent, const TDynArray<CName>& currentTagList, Bool forceTagTreeVisible = false );
	CEdTagMiniEditor( wxWindow* parent, const TDynArray<CName>& currentTagList, wxTextCtrl* tagEditBox );
	virtual ~CEdTagMiniEditor();

protected:
	TDynArray< CName >          m_tags;					//!< Tags returned on OK
	THashMap< String, Uint32 >    m_filteredTags;			//!< Tags visible on the hint list
	CEdTimer                    m_filterTimer;			//!< Filter update timer
    String                      m_filter;				//!< Filter used to filter the potential tag list

	Bool						m_providersWillBeDisposedExternally;
	TagProviderArray			m_availableTagsProviders;
	
	wxPoint						m_lastMousePosition;

	Bool						m_forceTagTreeVisible;
    Bool                        m_hintMode;
    Bool                        m_lockOnEditTagChanged;
    Bool                        m_lockTagCompletion;
    Bool                        m_clearEditTagSelection;
    
	Bool						m_blockKeyboardArrows;
	Bool						m_allowOnlyValidKeyPresses;
	Bool						m_saveOnClose;
	Bool						m_isSelectionFromKeyboard;
	

	
public:
	//! Get edited tag list
	RED_INLINE const TDynArray< CName >& GetTags() const { return m_tags; }

public:
	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	void SetTagListProviders( const TDynArray< CTagListProvider* > updaters, Bool providersWillBeDisposedExternally );

	RED_INLINE Bool GetBlockKeyboardArrows() const { return m_blockKeyboardArrows; }
	RED_INLINE void SetBlockKeyboardArrows( Bool newValue ) { m_blockKeyboardArrows = newValue; }
	RED_INLINE Bool GetAllowOnlyValidKeyPresses() const { return m_allowOnlyValidKeyPresses; }
	RED_INLINE void SetAllowOnlyValidKeyPresses( const Bool& newValue ) { m_allowOnlyValidKeyPresses = newValue; }
	RED_INLINE void SetSaveOnClose( const Bool& newValue ) { m_saveOnClose = newValue; }

protected:
    void ShowTagTreeIfNeeded();
	void UpdateTagTree();
	void BuildTree(const STagNode *tagNode);

	void OnCloseCleanup();

	String ExtractFilter();

// wxWidgets code goes below
private:
	//Do not add custom control declarations between
	//GUI Control Declaration Start and GUI Control Declaration End.
	//wxDev-C++ will remove them. Add custom code after the block.
	////GUI Control Declaration Start
		wxStaticText *m_wxStaticText3;
		wxStaticText *m_wxStaticText2;
		wxStaticText *m_wxStaticText1;
		wxTextCtrl *m_wxEditTag;
		wxButton *m_wxButtonOK;
		wxButton *m_wxButtonCancel;
		wxListBox *m_wxTagsList;
		//wxTreeListCtrl *m_wxTagsTree;
	////GUI Control Declaration End

    //Note: if you receive any error with these enum IDs, then you need to
	//change your old form code that are based on the #define control IDs.
	//#defines may replace a numeric value for the enum names.
	//Try copy and pasting the below block in your old form header files.
	enum
	{
		////GUI Enum Control ID Start
			ID_WXSTATICTEXT3 = 1010,
			ID_WXSTATICTEXT2 = 1009,
			ID_WXSTATICTEXT1 = 1002,
			ID_WXEDITTAG = 1001,
			ID_WXTAGSTREE = 1008,
		////GUI Enum Control ID End
		ID_DUMMY_VALUE_ //don't remove this value unless you have other enum values
	};

public:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

protected:
	void OnClose( wxCloseEvent& event );
	void OnTimer( wxTimerEvent& event );
    void OnEditTagSetFocus( wxFocusEvent& event );
    void OnEditTagKillFocus( wxFocusEvent& event );
	void OnEditTagChanged( wxCommandEvent& event );
    void OnEditTagKeyDown( wxKeyEvent& event );

	void ChangeTreeItemSelection( Bool shouldSelectNext );
	void OnEditTagMouseDown( wxMouseEvent& event );
    void OnTagContext( wxMouseEvent& event );
    void OnTagSelected( wxCommandEvent& event );
	void OnTagDoubleClicked( wxCommandEvent& event );
    void OnForgetTag( wxCommandEvent& event );
    void CreateGUIControls();
    void CreateGUIControlsHint();
};

#endif
