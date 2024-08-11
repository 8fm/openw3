/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef __CEDSHORTCUTSEDITOR_h__
#define __CEDSHORTCUTSEDITOR_h__

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/dialog.h>
#else
	#include <wx/wxprec.h>
#endif

//Do not add custom headers between 
//Header Include Start and Header Include End.
//wxDev-C++ designer will remove them. Add custom headers after the block.
////Header Include Start
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
////Header Include End
#include <wx/menu.h>
#include "shortcut.h"

////Dialog Style Start
#undef CEdShortcutsEditor_STYLE
#define CEdShortcutsEditor_STYLE wxCAPTION | wxSYSTEM_MENU | wxDIALOG_NO_PARENT | wxCLOSE_BOX
////Dialog Style End

class CEdShortcutsEditor : public wxDialog
{
	private:
		DECLARE_EVENT_TABLE();
		
	public:
		CEdShortcutsEditor(wxWindow *parent, wxWindowID id = 1, const wxString &title = wxT("Shortcuts Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = CEdShortcutsEditor_STYLE);
		virtual ~CEdShortcutsEditor();
		
		void ListMenuEntriesItemFocused(wxListEvent& event);
        void ListMenuEntriesColLeftClick(wxListEvent& event);
		void BtnSetClick(wxCommandEvent& event);
		void BtnCloseClick(wxCommandEvent& event);
		void BtnClearClick(wxCommandEvent& event);
        void BtnResetFilterClick(wxCommandEvent& event);
		void FilterTextUpdated(wxCommandEvent& evet);
		
	public:
        void AddItems(wxWindow &window, TEdShortcutArray &accelerators, const wxString &family);
        void AddItems(wxWindow &window, SEdShortcut &accelerator,       const wxString &family);

        void AddItems(wxWindow &window, wxMenuBar  &cMenuBar,  const wxString &sFamily, const wxString &sPreName = wxT(""));
        void AddItems(wxWindow &window, wxMenu     &cMenu,     const wxString &sFamily, const wxString &sPreName = wxT(""));
        void AddItems(wxWindow &window, wxMenuItem &cMenuItem, const wxString &sFamily, const wxString &sPreName = wxT(""));
        void ClearItems();

        static void Load(wxWindow &window, TEdShortcutArray &accelerators, const wxString &sFamilyName, Bool first = true, Bool last = true);
        static void Load(wxMenuBar &cMenuBar, const wxString sFamilyName, const wxString &sPreName = wxT(""), Bool first = true, Bool last = true);
        
        void UpdateAcceleratorsForWindow(wxWindow *window);

		virtual int ShowModal();

    private:
        static void LoadItems(wxMenuBar  &cMenuBar,  const wxString &sPreName = wxT(""));
        static void LoadItems(wxMenu     &cMenu,     const wxString &sPreName = wxT(""));
        static void LoadItems(wxMenuItem &cMenuItem, const wxString &sPreName = wxT(""));
	
	private:
		//Do not add custom control declarations between 
		//GUI Control Declaration Start and GUI Control Declaration End.
		//wxDev-C++ will remove them. Add custom code after the block.
		////GUI Control Declaration Start
		wxButton *m_cBtnResetFilter;
		wxTextCtrl *m_cFilterId;
		wxStaticText *m_cStaticText3;
		wxTextCtrl *m_cFilterFamily;
		wxButton *m_cBtnClear;
		wxButton *m_cBtnClose;
		wxButton *m_cBtnSet;
		wxListCtrl *m_cListMenuEntries;
		wxTextCtrl *m_cEditShortcut;
		wxStaticText *m_cStaticText2;
		wxStaticText *m_cStaticText1;
		////GUI Control Declaration End

	private:
        struct SMenuEntry
        {
            wxString            m_family;
            wxString            m_id;
            wxWindow           *m_window;
            wxAcceleratorEntry  m_acceleratorEntry;
            wxMenuItem         *m_menuItem;
            wxToolBarToolBase  *m_tool;
            wxWindowBase       *m_ctrl;
            wxString            m_shortcut;
            Int32                 m_listIdx;
            Bool                m_isProcessed;

            SMenuEntry()
                : m_family(wxString())
                , m_id(wxString())
                , m_window(NULL)
                , m_acceleratorEntry()
                , m_menuItem(NULL)
                , m_tool(NULL)
                , m_ctrl(NULL)
                , m_shortcut(wxString())
                , m_listIdx(-1)
                , m_isProcessed(false)
            {}
            SMenuEntry(const wxString &family, wxWindow &window, const wxString &id, wxMenuItem &menuItem, const wxString shortcut, Int32 listIdx = -1)
                : m_family(family)
                , m_id(id)
                , m_menuItem(&menuItem)
                , m_window(&window)
                , m_acceleratorEntry()
                , m_tool(NULL)
                , m_ctrl(NULL)
                , m_shortcut(shortcut)
                , m_listIdx(listIdx)
                , m_isProcessed(false)
            {}
            SMenuEntry(const wxString &family, wxWindow &window, SEdShortcut &shortcut, Int32 listIdx = -1)
                : m_family(family)
                , m_id(shortcut.m_id)
                , m_menuItem(NULL)
                , m_window(&window)
                , m_acceleratorEntry(shortcut.m_acceleratorEntry)
                , m_tool(shortcut.m_tool)
                , m_ctrl(shortcut.m_ctrl)
                , m_shortcut(shortcut.m_acceleratorEntry.GetKeyCode() ? shortcut.m_acceleratorEntry.ToString() : wxString())
                , m_listIdx(listIdx)
                , m_isProcessed(false)
            {}

            Int32 GetCommand()
            {
                if (m_acceleratorEntry.GetCommand())
                    return m_acceleratorEntry.GetCommand();
                if (m_menuItem)
                    return m_menuItem->GetId();
                return 0;
            }

            Bool MatchesFilter(const wxString &familyFilter, const wxString &idFilter) const
            {
                return m_family.Lower().find(familyFilter.Lower()) != wxString::npos && m_id.Lower().find(idFilter.Lower()) != wxString::npos;
            }
        };

        typedef TDynArray<SMenuEntry> TVecMenuItems;
        TVecMenuItems                 m_configurableMenuItems;

        Bool SetShortcut(const wxString &shortcut, SMenuEntry &menuEntry, Int32 nDataIdx, Bool searchForDuplicates);

        typedef TSortedSet<String>    TSetUsedMenuKeys;
        static TSetUsedMenuKeys       sm_usedMenuKeys;

        Bool m_shouldIFocusShortcutEditBox;

        Int32  m_sortingColumn;
        Int32  m_sortingDirection;

        void ReapplySort();

		//Note: if you receive any error with these enum IDs, then you need to
		//change your old form code that are based on the #define control IDs.
		//#defines may replace a numeric value for the enum names.
		//Try copy and pasting the below block in your old form header files.
		enum
		{
			////GUI Enum Control ID Start
			ID_M_CBTNRESETFILTER = 1015,
			ID_M_CFILTERID = 1013,
			ID_M_CSTATICTEXT3 = 1012,
			ID_M_CFILTERFAMILY = 1011,
			ID_M_CBTNCLEAR = 1009,
			ID_M_CBTNCLOSE = 1008,
			ID_M_CBTNSET = 1007,
			ID_M_CLISTMENUENTRIES = 1006,
			ID_M_CEDITSHORTCUT = 1004,
			ID_M_CSTATICTEXT2 = 1003,
			ID_M_CSTATICTEXT1 = 1002,
			////GUI Enum Control ID End
			ID_DUMMY_VALUE_ //don't remove this value unless you have other enum values
		};
	
	private:
        void OnIdle (wxIdleEvent&  event);
		void OnClose(wxCloseEvent& event);
		void CreateGUIControls();

        friend int wxCALLBACK wxListCompareFunction(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData);
};

#endif
