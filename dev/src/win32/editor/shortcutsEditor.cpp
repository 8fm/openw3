/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "shortcutsEditor.h"

class wxShortcutTextCtrl : public wxTextCtrl
{
public:
    wxShortcutTextCtrl() : wxTextCtrl() {}

    wxShortcutTextCtrl(wxWindow *parent, wxWindowID id,
               const wxString& value = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator,
               const wxString& name = wxTextCtrlNameStr)
        : wxTextCtrl(parent, id, value, pos, size, style, validator, name)
    {}

protected:
    virtual bool MSWShouldPreProcessMessage(WXMSG* pMsg)
    {
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
        {
            return true;
        }

        return wxTextCtrl::MSWShouldPreProcessMessage(pMsg);
    }

    // return true if the message was preprocessed and shouldn't be dispatched
    virtual bool MSWProcessMessage(WXMSG* pMsg)
    {
#if 0
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
        {
            if (pMsg->wParam != VK_CONTROL && pMsg->wParam != VK_MENU && pMsg->wParam != VK_SHIFT)
            {
                Int32 flags = wxACCEL_NORMAL;
                if (GetKeyState(VK_CONTROL) < 0)
                    flags |= wxACCEL_CTRL;
                if (GetKeyState(VK_SHIFT) < 0)
                    flags |= wxACCEL_SHIFT;
                if (GetKeyState(VK_MENU) < 0)
                    flags |= wxACCEL_ALT;

                Int32 nKeyId = wxCharCodeMSWToWX(pMsg->wParam, pMsg->lParam);
                if (!nKeyId)
                    nKeyId = pMsg->wParam;

                wxAcceleratorEntry entry(flags, nKeyId);
                SetValue(entry.ToString());
            }
            return true;
        }
#endif
        return wxTextCtrl::MSWProcessMessage(pMsg);
    }
};

//Do not add custom headers
//wxDev-C++ designer will remove them
////Header Include Start
////Header Include End

//----------------------------------------------------------------------------
// CEdShortcutsEditor
//----------------------------------------------------------------------------

CEdShortcutsEditor::TSetUsedMenuKeys CEdShortcutsEditor::sm_usedMenuKeys;

//Add Custom Events only in the appropriate block.
//Code added in other places will be removed by wxDev-C++
////Event Table Start
BEGIN_EVENT_TABLE(CEdShortcutsEditor,wxDialog)
	////Manual Code Start
	EVT_IDLE(CEdShortcutsEditor::OnIdle)
	////Manual Code End
	
	EVT_CLOSE(CEdShortcutsEditor::OnClose)
	EVT_BUTTON(ID_M_CBTNRESETFILTER,CEdShortcutsEditor::BtnResetFilterClick)
	
	EVT_TEXT(ID_M_CFILTERID,CEdShortcutsEditor::FilterTextUpdated)
	
	EVT_TEXT(ID_M_CFILTERFAMILY,CEdShortcutsEditor::FilterTextUpdated)
	EVT_BUTTON(ID_M_CBTNCLEAR,CEdShortcutsEditor::BtnClearClick)
	EVT_BUTTON(ID_M_CBTNCLOSE,CEdShortcutsEditor::BtnCloseClick)
	EVT_BUTTON(ID_M_CBTNSET,CEdShortcutsEditor::BtnSetClick)
	
	EVT_LIST_ITEM_FOCUSED(ID_M_CLISTMENUENTRIES,CEdShortcutsEditor::ListMenuEntriesItemFocused)
	EVT_LIST_COL_CLICK(ID_M_CLISTMENUENTRIES,CEdShortcutsEditor::ListMenuEntriesColLeftClick)
END_EVENT_TABLE()
////Event Table End

CEdShortcutsEditor::CEdShortcutsEditor(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &position, const wxSize& size, long style)
: wxDialog(parent, id, title, position, size, style)
, m_sortingColumn(-1)
, m_sortingDirection(1)
{
    m_shouldIFocusShortcutEditBox = false;
	CreateGUIControls();
}

CEdShortcutsEditor::~CEdShortcutsEditor()
{
}

int CEdShortcutsEditor::ShowModal()
{
	return wxDialog::ShowModal();
}

void CEdShortcutsEditor::CreateGUIControls()
{
	//Do not add custom code between
	//GUI Items Creation Start and GUI Items Creation End.
	//wxDev-C++ designer will remove them.
	//Add the custom code before or after the blocks
	////GUI Items Creation Start

	SetTitle(wxT("Shortcuts Editor"));
	SetIcon(wxNullIcon);
	SetSize(8,8,553,490);
	Center();
	

	m_cBtnResetFilter = new wxButton(this, ID_M_CBTNRESETFILTER, wxT("Reset filter"), wxPoint(413,43), wxSize(96,21), 0, wxDefaultValidator, wxT("m_cBtnResetFilter"));
	m_cBtnResetFilter->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cFilterId = new wxTextCtrlEx(this, ID_M_CFILTERID, wxT(""), wxPoint(112,44), wxSize(297,20), 0, wxDefaultValidator, wxT("m_cFilterId"));
	m_cFilterId->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cStaticText3 = new wxStaticText(this, ID_M_CSTATICTEXT3, wxT("Filter entries:"), wxPoint(8,25), wxDefaultSize, 0, wxT("m_cStaticText3"));
	m_cStaticText3->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cFilterFamily = new wxTextCtrlEx(this, ID_M_CFILTERFAMILY, wxT(""), wxPoint(8,44), wxSize(100,20), 0, wxDefaultValidator, wxT("m_cFilterFamily"));
	m_cFilterFamily->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cBtnClear = new wxButton(this, ID_M_CBTNCLEAR, wxT("Clear"), wxPoint(476,398), wxSize(62,21), 0, wxDefaultValidator, wxT("m_cBtnClear"));
	m_cBtnClear->Enable(false);
	m_cBtnClear->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cBtnClose = new wxButton(this, ID_M_CBTNCLOSE, wxT("Close"), wxPoint(234,428), wxSize(75,25), 0, wxDefaultValidator, wxT("m_cBtnClose"));
	m_cBtnClose->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cBtnSet = new wxButton(this, ID_M_CBTNSET, wxT("Set"), wxPoint(410,398), wxSize(62,21), 0, wxDefaultValidator, wxT("m_cBtnSet"));
	m_cBtnSet->Enable(false);
	m_cBtnSet->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cListMenuEntries = new wxListCtrl(this, ID_M_CLISTMENUENTRIES, wxPoint(8,72), wxSize(529,305), wxLC_REPORT);
	m_cListMenuEntries->InsertColumn(0,wxT("Shortcut"),wxLIST_FORMAT_LEFT,100 );
	m_cListMenuEntries->InsertColumn(0,wxT("Menu entry"),wxLIST_FORMAT_LEFT,300 );
	m_cListMenuEntries->InsertColumn(0,wxT("Family"),wxLIST_FORMAT_LEFT,100 );
	m_cListMenuEntries->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cEditShortcut = new wxShortcutTextCtrl(this, ID_M_CEDITSHORTCUT, wxT(""), wxPoint(7,399), wxSize(398,20), 0, wxDefaultValidator, wxT("m_cEditShortcut"));
	m_cEditShortcut->Enable(false);
	m_cEditShortcut->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cStaticText2 = new wxStaticText(this, ID_M_CSTATICTEXT2, wxT("Press new shortcut:"), wxPoint(8,383), wxDefaultSize, 0, wxT("m_cStaticText2"));
	m_cStaticText2->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	m_cStaticText1 = new wxStaticText(this, ID_M_CSTATICTEXT1, wxT("Select menu item for which you would like to redefine shortcut:"), wxPoint(8,5), wxDefaultSize, 0, wxT("m_cStaticText1"));
	m_cStaticText1->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));
	////GUI Items Creation End

    m_cFilterId        ->MoveAfterInTabOrder(m_cFilterFamily);
    m_cBtnResetFilter  ->MoveAfterInTabOrder(m_cFilterId);
    m_cListMenuEntries ->MoveAfterInTabOrder(m_cBtnResetFilter);
    m_cEditShortcut    ->MoveAfterInTabOrder(m_cListMenuEntries);
    m_cBtnSet          ->MoveAfterInTabOrder(m_cEditShortcut);
    m_cBtnClear        ->MoveAfterInTabOrder(m_cBtnSet);
    m_cBtnClose        ->MoveAfterInTabOrder(m_cBtnClear);
    m_cFilterId->SetFocus();
}

void CEdShortcutsEditor::OnClose(wxCloseEvent& /*event*/)
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
 	config.DeleteDirectory(TXT("/Shortcuts"));
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Shortcuts") );

    sm_usedMenuKeys.Clear();

    TVecMenuItems::const_iterator ITM_curr = m_configurableMenuItems.Begin(),
                                  ITM_last = m_configurableMenuItems.End();

    for (Int32 nData = 0; ITM_curr != ITM_last; ++ITM_curr, ++nData)
    {
        String sMenuEntry = (ITM_curr->m_family + wxT(':') + ITM_curr->m_id).wc_str();
        if (!sm_usedMenuKeys.Insert(sMenuEntry))
        {
            Int32 i = 0;
            while (!sm_usedMenuKeys.Insert(sMenuEntry + ToString(i)))
                ++i;

            sMenuEntry += ToString(i);
        }

        if (ITM_curr->m_shortcut.Length())
            config.Write(sMenuEntry, ITM_curr->m_shortcut.wc_str());
    }

    sm_usedMenuKeys.Clear();

	Destroy();
}

void CEdShortcutsEditor::OnIdle(wxIdleEvent& event)
{
    if (m_shouldIFocusShortcutEditBox)
    {
        m_cEditShortcut->SetFocus();
        m_shouldIFocusShortcutEditBox = false;
    }
}

void CEdShortcutsEditor::ClearItems()
{
    m_cEditShortcut->SetValue(wxString());
    m_cEditShortcut->Enable(false);
    m_cBtnSet->Enable(false);
    m_cBtnClear->Enable(false);
    m_cListMenuEntries->DeleteAllItems();
    m_configurableMenuItems.Clear();
}

void CEdShortcutsEditor::AddItems(wxWindow &window, TEdShortcutArray &accelerators, const wxString &family)
{
    TEdShortcutArray::iterator ACC_curr = accelerators.Begin(),
                               ACC_last = accelerators.End();

    for (; ACC_curr != ACC_last; ++ACC_curr)
        AddItems(window, *ACC_curr, family);
}

void CEdShortcutsEditor::AddItems(wxWindow &window, SEdShortcut &accelerator, const wxString &family)
{
    m_configurableMenuItems.PushBack(SMenuEntry(family, window, accelerator));
        
    SMenuEntry &menuEntry = m_configurableMenuItems[m_configurableMenuItems.Size()-1];
        
    if (menuEntry.MatchesFilter(m_cFilterFamily->GetValue(), m_cFilterId->GetValue()))
    {
        long nIdx = m_cListMenuEntries->InsertItem(m_cListMenuEntries->GetItemCount(), menuEntry.m_family);
        m_cListMenuEntries->SetItem(nIdx, 1, menuEntry.m_id);
        m_cListMenuEntries->SetItem(nIdx, 2, menuEntry.m_shortcut);
        m_cListMenuEntries->SetItemData(nIdx, m_configurableMenuItems.Size() - 1);

        menuEntry.m_listIdx = nIdx;

        ReapplySort();
    }
}

void CEdShortcutsEditor::AddItems(wxWindow &window, wxMenuBar &cMenuBar, const wxString &sFamily, const wxString &sPreName /* = wxT("") */)
{
    for (Uint32 i = 0; i < cMenuBar.GetMenuCount(); ++i)
    {
        AddItems(window, *cMenuBar.GetMenu(i), sFamily, sPreName + cMenuBar.GetMenuLabelText(i) + wxT("\\"));
    }
}

void CEdShortcutsEditor::AddItems(wxWindow &window, wxMenu &cMenu, const wxString &sFamily, const wxString &sPreName /* = wxT("") */)
{
    for (Uint32 i = 0; i < cMenu.GetMenuItemCount(); ++i)
    {
        AddItems(window, *cMenu.GetMenuItems().Item(i)->GetData(), sFamily, sPreName);
    }
}

void CEdShortcutsEditor::AddItems(wxWindow &window, wxMenuItem &cMenuItem, const wxString &sFamily, const wxString &sPreName /* = wxT("") */)
{
    wxMenu *pSubMenu = cMenuItem.GetSubMenu();
    if (pSubMenu)
    {
        AddItems(window, *pSubMenu, sFamily, sPreName + cMenuItem.GetItemLabelText() + wxT("\\"));
    }
    else
    {
        wxString sLabel = cMenuItem.GetItemLabelText();
        if (!sLabel.Length())
            return;
        
        wxAcceleratorEntry *pAccel = cMenuItem.GetAccel();
        if (pAccel)
        {
            m_configurableMenuItems.PushBack(SMenuEntry(sFamily, window, sPreName + sLabel, cMenuItem, pAccel->ToString()));
            delete pAccel;
        }
        else
            m_configurableMenuItems.PushBack(SMenuEntry(sFamily, window, sPreName + sLabel, cMenuItem, wxString()));

        SMenuEntry &menuEntry = m_configurableMenuItems[m_configurableMenuItems.Size()-1];
        
        if (menuEntry.MatchesFilter(m_cFilterFamily->GetValue(), m_cFilterId->GetValue()))
        {
            long nIdx = m_cListMenuEntries->InsertItem(m_cListMenuEntries->GetItemCount(), menuEntry.m_family);
            m_cListMenuEntries->SetItem(nIdx, 1, menuEntry.m_id);
            m_cListMenuEntries->SetItem(nIdx, 2, menuEntry.m_shortcut);
            m_cListMenuEntries->SetItemData(nIdx, m_configurableMenuItems.Size() - 1);

            menuEntry.m_listIdx = nIdx;

            ReapplySort();
        }
    }
}
    
// Load settings
    
void CEdShortcutsEditor::Load(wxWindow &window, TEdShortcutArray &accelerators, const wxString &sFamilyName, Bool first /*= true*/, Bool last /*= true*/)
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Shortcuts") );

    TDynArray< TPair<String, String > > settings;
    config.AllSettings(settings);
    
    if (first)
        sm_usedMenuKeys.Clear();

    TDynArray<wxAcceleratorEntry> entries;

    wxString sKeyPrefix = sFamilyName + wxT(':');

    TEdShortcutArray::iterator ACC_curr = accelerators.Begin(),
                               ACC_last = accelerators.End();
    for (; ACC_curr != ACC_last; ++ACC_curr)
    {
        String sMenuEntry = String((sKeyPrefix + ACC_curr->m_id).wc_str()),
               sShortcut;

        if (!sm_usedMenuKeys.Insert(sMenuEntry))
        {
            Int32 i = 0;
            while (!sm_usedMenuKeys.Insert(sMenuEntry + ToString(i)))
                ++i;

            sMenuEntry += ToString(i);
        }

		if ( ! settings.Empty() && config.Read(sMenuEntry, &sShortcut) )
		{
			wxAcceleratorEntry *pAccel = wxAcceleratorEntry::Create(wxT('\t') + wxString(sShortcut.AsChar()));
			if ( pAccel )
				ACC_curr->m_acceleratorEntry.Set(pAccel->GetFlags(), pAccel->GetKeyCode(), ACC_curr->m_acceleratorEntry.GetCommand(), ACC_curr->m_acceleratorEntry.GetMenuItem());
			else
				ACC_curr->m_acceleratorEntry.Set(0,0,ACC_curr->m_acceleratorEntry.GetCommand(),ACC_curr->m_acceleratorEntry.GetMenuItem());
		}
			
		if ( ACC_curr->m_acceleratorEntry.GetKeyCode() != 0 )
		{
			if (ACC_curr->m_tool)
			{
				ACC_curr->m_tool->SetShortHelp(SEdShortcut::AppendToolTipShortcut(ACC_curr->m_tool->GetShortHelp(), wxString(ACC_curr->m_acceleratorEntry.ToString())));
			}
			if (ACC_curr->m_ctrl)
			{
				ACC_curr->m_ctrl->SetToolTip(SEdShortcut::AppendToolTipShortcut(ACC_curr->m_ctrl->GetToolTip()->GetTip(), wxString(ACC_curr->m_acceleratorEntry.ToString())));
			}
			entries.PushBack(ACC_curr->m_acceleratorEntry);
		}
		else
        {
            if (ACC_curr->m_tool)
            {
                ACC_curr->m_tool->SetShortHelp( SEdShortcut::StripToolTipShortcut( ACC_curr->m_tool->GetShortHelp() ) );
            }
            if (ACC_curr->m_ctrl)
            {
                ACC_curr->m_ctrl->SetToolTip( SEdShortcut::StripToolTipShortcut( ACC_curr->m_ctrl->GetToolTip()->GetTip() ) );
            }
        }
    }

    wxAcceleratorEntry *entriesArr = new wxAcceleratorEntry[entries.Size()];
    for (size_t i = 0; i < entries.Size(); ++i)
        entriesArr[i] = entries[i];

    wxAcceleratorTable accelTbl(entries.Size(), entriesArr);
    window.SetAcceleratorTable(accelTbl);

    delete[] entriesArr;

    if (last)
        sm_usedMenuKeys.Clear();
}

void CEdShortcutsEditor::Load(wxMenuBar &cMenuBar, const wxString sFamilyName, const wxString &sPreName /* = wxT("") */, Bool first /*= true*/, Bool last /*= true*/)
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Shortcuts") );

    TDynArray< TPair<String, String > > settings;
    config.AllSettings(settings);
    if (!settings.Size()) // no shortcuts stored - use default configuration
    {
        return;
    }
    
    if (first)
        sm_usedMenuKeys.Clear();

    LoadItems(cMenuBar, sFamilyName + wxT(':') + sPreName);

    if (last)
        sm_usedMenuKeys.Clear();
}

void CEdShortcutsEditor::LoadItems(wxMenuBar &cMenuBar, const wxString &sPreName /* = wxT("") */)
{
    for (Uint32 i = 0; i < cMenuBar.GetMenuCount(); ++i)
    {
        LoadItems(*cMenuBar.GetMenu(i), sPreName + cMenuBar.GetMenuLabelText(i) + wxT("\\"));
    }
}

void CEdShortcutsEditor::LoadItems(wxMenu &cMenu, const wxString &sPreName /* = wxT("") */)
{
    for (Uint32 i = 0; i < cMenu.GetMenuItemCount(); ++i)
    {
        LoadItems(*cMenu.GetMenuItems().Item(i)->GetData(), sPreName);
    }
}

void CEdShortcutsEditor::LoadItems(wxMenuItem &cMenuItem, const wxString &sPreName /* = wxT("") */)
{
    wxMenu *pSubMenu = cMenuItem.GetSubMenu();
    if (pSubMenu)
    {
        LoadItems(*pSubMenu, sPreName + cMenuItem.GetItemLabelText() + wxT("\\"));
    }
    else
    {
        wxString sLabel = cMenuItem.GetItemLabelText();
        if (!sLabel.Length())
            return;

        CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

        String sMenuEntry = String(sPreName.wc_str()) + sLabel.wc_str(),
               sShortcut;

        if (!sm_usedMenuKeys.Insert(sMenuEntry))
        {
            Int32 i = 0;
            while (!sm_usedMenuKeys.Insert(sMenuEntry + ToString(i)))
                ++i;

            sMenuEntry += ToString(i);
        }

        if (config.Read(sMenuEntry, &sShortcut))
        {
            wxAcceleratorEntry *pAccel = wxAcceleratorEntry::Create(wxT("\t") + wxString(sShortcut.AsChar()));
            cMenuItem.SetAccel(pAccel);
			if ( pAccel )
				delete pAccel;
        }
        else
            cMenuItem.SetAccel(NULL);
    }
}
    
// Controls events
    
/*
 * m_cListMenuEntriesItemFocused
 */
void CEdShortcutsEditor::ListMenuEntriesItemFocused(wxListEvent& event)
{
    long nData = event.GetData();
    if (nData >= 0)
    {
        SMenuEntry &menuEntry = m_configurableMenuItems[nData];
        
        m_cEditShortcut->SetValue(menuEntry.m_shortcut);

        m_cEditShortcut->Enable(true);
        m_cBtnSet->Enable(true);
        m_cBtnClear->Enable(true);
        
        m_shouldIFocusShortcutEditBox = true;
    }
    else
    {
        m_cEditShortcut->SetValue(wxString());
        m_cEditShortcut->Enable(false);
        m_cBtnSet->Enable(false);
        m_cBtnClear->Enable(false);
    }
}

int wxCALLBACK wxListCompareFunction(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData)
{
    CEdShortcutsEditor *shortcutEditorBeingSorted = reinterpret_cast<CEdShortcutsEditor *>( sortData );

    int direction = - (shortcutEditorBeingSorted->m_sortingDirection * 2 - 1);

    switch (shortcutEditorBeingSorted->m_sortingColumn)
    {
        case 0:
            {
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return 1 * direction;
            }
            break;
        case 1:
            {
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return 1 * direction;
            }
            break;
        case 2:
            {
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_shortcut > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_shortcut)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_family > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_family)
                    return 1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id < shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return -1 * direction;
                if (shortcutEditorBeingSorted->m_configurableMenuItems[item1].m_id > shortcutEditorBeingSorted->m_configurableMenuItems[item2].m_id)
                    return 1 * direction;
            }
            break;
    }

    return 0;
}

void CEdShortcutsEditor::ReapplySort()
{
    if (m_sortingColumn < 0) return;

    m_cListMenuEntries->SortItems(wxListCompareFunction, reinterpret_cast<wxUIntPtr>(this));

    // Update indices
    for (int idx = 0; idx < m_cListMenuEntries->GetItemCount(); ++idx)
    {
        long nDataIdx = m_cListMenuEntries->GetItemData(idx);
        if (nDataIdx >= 0)
            m_configurableMenuItems[nDataIdx].m_listIdx = idx;
    }

    // Scroll to selected item
    long nSelItemIdx = m_cListMenuEntries->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if ( nSelItemIdx >= 0 )
    {
        wxRect rect;
        m_cListMenuEntries->GetItemRect(nSelItemIdx, rect, wxLIST_RECT_BOUNDS);
        m_cListMenuEntries->ScrollList(rect.x, rect.y - 100);
    }
}

/*
 * m_cListMenuEntriesColLeftClick
 */
void CEdShortcutsEditor::ListMenuEntriesColLeftClick(wxListEvent& event)
{
    if (m_sortingColumn == event.m_col)
    {
        m_sortingDirection = (m_sortingDirection + 1) % 2;
    }
    else
    {
        m_sortingColumn    = event.m_col;
        m_sortingDirection = 0;
    }

    ReapplySort();
}

Bool CEdShortcutsEditor::SetShortcut(const wxString &shortcut, SMenuEntry &menuEntry, Int32 nDataIdx, Bool searchForDuplicates)
{
    menuEntry.m_isProcessed = true;

    if (shortcut.Length())
    {
        // Search for shortcut duplicates
        if (searchForDuplicates)
        {
            TVecMenuItems::iterator ITM_curr = m_configurableMenuItems.Begin(),
                                    ITM_last = m_configurableMenuItems.End();

            for (Int32 nItrData = 0; ITM_curr != ITM_last; ++ITM_curr, ++nItrData)
            {
                if (nDataIdx == nItrData)
                    continue;

                if (ITM_curr->GetCommand() == menuEntry.GetCommand())
                    continue;

                if (ITM_curr->m_window != menuEntry.m_window)
                    continue;

                if (ITM_curr->m_shortcut == shortcut)
                {
                    if (wxNO ==
                        ::wxMessageBox(wxT("The shortcut is already used by:\n") +
                                       ITM_curr->m_family + wxT(':') + ITM_curr->m_id +
                                       wxT("\nWould you like to clear it?"),
                                       wxT("Conflicting shortcuts!"),
                                       wxYES_NO, this) )
                        return false;

                    // Remove duplicated shortcut
                    if (ITM_curr->m_menuItem)
                        ITM_curr->m_menuItem->SetAccel(NULL);
                    else
                    if (ITM_curr->m_acceleratorEntry.GetCommand())
                    {
                        ITM_curr->m_acceleratorEntry.Set(0,0,ITM_curr->m_acceleratorEntry.GetCommand(),ITM_curr->m_acceleratorEntry.GetMenuItem());
                        UpdateAcceleratorsForWindow(ITM_curr->m_window);
                    }
                    if (ITM_curr->m_tool)
                    {
                        ITM_curr->m_tool->SetShortHelp( SEdShortcut::StripToolTipShortcut( ITM_curr->m_tool->GetShortHelp() ) );
                    }
                    if (ITM_curr->m_ctrl)
                    {
                        ITM_curr->m_ctrl->SetToolTip( SEdShortcut::StripToolTipShortcut( ITM_curr->m_ctrl->GetToolTip()->GetTip() ) );
                    }
                    ITM_curr->m_shortcut.clear();
                    if (ITM_curr->m_listIdx >= 0)
                        m_cListMenuEntries->SetItem(ITM_curr->m_listIdx, 2, wxString());
                }
            }
        }
    }

    // Search for similiar command that could share the shortcut
    TVecMenuItems::iterator ITM_curr = m_configurableMenuItems.Begin(),
                            ITM_last = m_configurableMenuItems.End();

    for (Int32 nItrData = 0; ITM_curr != ITM_last; ++ITM_curr, ++nItrData)
    {
        if (ITM_curr->m_isProcessed)
            continue;

        if (nDataIdx == nItrData)
            continue;

        if (ITM_curr->m_window == menuEntry.m_window &&
            ITM_curr->GetCommand() == menuEntry.GetCommand())
        {
            if (ITM_curr->m_shortcut == m_cEditShortcut->GetValue() ||
                wxYES ==
                ::wxMessageBox(wxT("Another instance of this command exists:\n") +
                               ITM_curr->m_family + wxT(':') + ITM_curr->m_id +
                               wxT("\nWould you like to set this shortcut for that command too?"),
                               wxT("There are multiple instances of the same command!"),
                               wxYES_NO, this) )
            {
                SetShortcut(m_cEditShortcut->GetValue(), *ITM_curr, nItrData, false);
            }
        }
        else
        if (ITM_curr->m_window != menuEntry.m_window &&
            ITM_curr->m_id == menuEntry.m_id)
        {
            if (ITM_curr->m_shortcut == m_cEditShortcut->GetValue() ||
                wxYES ==
                ::wxMessageBox(wxT("Command with the same name (") +
                               ITM_curr->m_id +
                               wxT(") exists for family:\n") +
                               ITM_curr->m_family +
                               wxT("\nWould you like to set this shortcut for that command too?"),
                               wxT("More commands with such name found!"),
                               wxYES_NO, this) )
            {
                SetShortcut(m_cEditShortcut->GetValue(), *ITM_curr, nItrData, true);
            }
        }
    }

    // Set the new shortcut
    wxAcceleratorEntry *pAccel = wxAcceleratorEntry::Create(wxT("\t") + shortcut);
    if (menuEntry.m_menuItem)
        menuEntry.m_menuItem->SetAccel(pAccel);
    else
    if (menuEntry.m_acceleratorEntry.GetCommand())
    {
        if (pAccel)
            menuEntry.m_acceleratorEntry.Set(pAccel->GetFlags(),pAccel->GetKeyCode(),menuEntry.m_acceleratorEntry.GetCommand(),menuEntry.m_acceleratorEntry.GetMenuItem());
        else
            menuEntry.m_acceleratorEntry.Set(0,0,menuEntry.m_acceleratorEntry.GetCommand(),menuEntry.m_acceleratorEntry.GetMenuItem());
        UpdateAcceleratorsForWindow(menuEntry.m_window);
    }

    if (pAccel)
    {
        menuEntry.m_shortcut = pAccel->ToString();
        if (menuEntry.m_listIdx >= 0)
        {
            m_cListMenuEntries->SetItem(menuEntry.m_listIdx, 2, menuEntry.m_shortcut);
            m_cEditShortcut->SetValue(menuEntry.m_shortcut);
        }
    }
    else
    {
        menuEntry.m_shortcut.clear();
        if (menuEntry.m_listIdx >= 0)
        {
            m_cListMenuEntries->SetItem(menuEntry.m_listIdx, 2, wxString());
            m_cEditShortcut->SetValue(wxString());
        }
    }
    delete pAccel;
    
    if (menuEntry.m_tool)
    {
        menuEntry.m_tool->SetShortHelp( SEdShortcut::AppendToolTipShortcut( menuEntry.m_tool->GetShortHelp(), menuEntry.m_shortcut ) );
    }
    if (menuEntry.m_ctrl)
    {
        menuEntry.m_ctrl->SetToolTip( SEdShortcut::AppendToolTipShortcut( menuEntry.m_ctrl->GetToolTip()->GetTip(), menuEntry.m_shortcut ) );
    }
    return true;
}
/*
 * m_cBtnSetClick
 */
void CEdShortcutsEditor::BtnSetClick(wxCommandEvent& event)
{
    long nSelItemIdx = m_cListMenuEntries->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if ( nSelItemIdx == -1 )
        return;

    long nData = m_cListMenuEntries->GetItemData(nSelItemIdx);
    if (nData < 0)
        return;

    SMenuEntry &selMenuEntry = m_configurableMenuItems[nData];

    TVecMenuItems::iterator ITM_curr = m_configurableMenuItems.Begin(),
                            ITM_last = m_configurableMenuItems.End();

    for (; ITM_curr != ITM_last; ++ITM_curr)
        ITM_curr->m_isProcessed = false;

    SetShortcut(m_cEditShortcut->GetValue(), selMenuEntry, nData, true);

    ReapplySort();
}

void CEdShortcutsEditor::UpdateAcceleratorsForWindow(wxWindow *window)
{
    if (!window) return;

    TDynArray<wxAcceleratorEntry> entries;

    TVecMenuItems::iterator ITM_curr = m_configurableMenuItems.Begin(),
                            ITM_last = m_configurableMenuItems.End();
    for (; ITM_curr != ITM_last; ++ITM_curr)
    {
        if (ITM_curr->m_window == window && ITM_curr->m_acceleratorEntry.GetCommand())
            if (ITM_curr->m_acceleratorEntry.GetKeyCode())
                entries.PushBack(ITM_curr->m_acceleratorEntry);
    }

    wxAcceleratorEntry *entriesArr = new wxAcceleratorEntry[entries.Size()];
    for (size_t i = 0; i < entries.Size(); ++i)
        entriesArr[i] = entries[i];

    wxAcceleratorTable accelTbl(entries.Size(), entriesArr);
    window->SetAcceleratorTable(accelTbl);

    delete[] entriesArr;
}

/*
 * m_cBtnClearClick
 */
void CEdShortcutsEditor::BtnClearClick(wxCommandEvent& event)
{
    m_cEditShortcut->SetValue(wxString());
    BtnSetClick(event);
}

/*
 * m_cBtnCloseClick
 */
void CEdShortcutsEditor::BtnCloseClick(wxCommandEvent& event)
{
    Close();
}

/*
 * m_cFilterTextUpdated
 */
void CEdShortcutsEditor::FilterTextUpdated(wxCommandEvent& event)
{
    m_cEditShortcut->SetValue(wxString());
    m_cEditShortcut->Enable(false);
    m_cBtnSet->Enable(false);
    m_cBtnClear->Enable(false);
    m_cListMenuEntries->DeleteAllItems();

    wxString sFilterFamily = m_cFilterFamily->GetValue();
    wxString sFilterId     = m_cFilterId->GetValue();

    TVecMenuItems::iterator ITM_curr = m_configurableMenuItems.Begin(),
                            ITM_last = m_configurableMenuItems.End();

    for (Int32 nData = 0; ITM_curr != ITM_last; ++ITM_curr, ++nData)
    {
        if (ITM_curr->MatchesFilter(sFilterFamily, sFilterId))
        {
            long nIdx = m_cListMenuEntries->InsertItem(m_cListMenuEntries->GetItemCount(), ITM_curr->m_family);
            m_cListMenuEntries->SetItem(nIdx, 1, ITM_curr->m_id);
            m_cListMenuEntries->SetItem(nIdx, 2, ITM_curr->m_shortcut);
            m_cListMenuEntries->SetItemData(nIdx, nData);

            ITM_curr->m_listIdx = nIdx;
        }
        else
            ITM_curr->m_listIdx = -1;
    }

    ReapplySort();
}

/*
 * m_cBtnResetFilterClick
 */
void CEdShortcutsEditor::BtnResetFilterClick(wxCommandEvent& event)
{
    m_cFilterFamily->Clear();
    m_cFilterId->Clear();
}

