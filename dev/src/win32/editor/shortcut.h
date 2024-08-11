/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef __SEDSHORTCUT_h__
#define __SEDSHORTCUT_h__

#include <wx/msw/tooltip.h>
#include <wx/layout.h>

struct SEdShortcut
{
    wxString            m_id;
    wxAcceleratorEntry  m_acceleratorEntry;
    wxToolBarToolBase  *m_tool;
    wxWindowBase       *m_ctrl;

    SEdShortcut()
        : m_id(wxString())
        , m_acceleratorEntry()
        , m_tool(NULL)
        , m_ctrl(NULL)
    {}
    
    SEdShortcut(const wxString &id, const wxAcceleratorEntry &acceleratorEntry)
        : m_id(id)
        , m_acceleratorEntry(acceleratorEntry)
        , m_tool(NULL)
        , m_ctrl(NULL)
    {}

    SEdShortcut(const wxString &id, wxToolBarToolBase &tool)
        : m_id(id)
        , m_acceleratorEntry()
        , m_tool(&tool)
        , m_ctrl(NULL)
    {
        wxString sShCut = GetToolTipShortcut( tool.GetShortHelp() );
        if (!sShCut.empty())
            m_acceleratorEntry.FromString(wxT('\t') + sShCut);
        m_acceleratorEntry.Set(m_acceleratorEntry.GetFlags(), m_acceleratorEntry.GetKeyCode(), m_tool->GetId());
    }

    SEdShortcut(const wxString &id, wxWindowBase &ctrl)
        : m_id(id)
        , m_acceleratorEntry()
        , m_tool(NULL)
        , m_ctrl(&ctrl)
    {
        wxString sShCut = GetToolTipShortcut( ctrl.GetToolTip()->GetTip() );
        if (sShCut.empty())
            m_acceleratorEntry.FromString(wxT('\t') + sShCut);
        m_acceleratorEntry.Set(m_acceleratorEntry.GetFlags(), m_acceleratorEntry.GetKeyCode(), m_ctrl->GetId());
    }

    static wxString StripToolTipShortcut(const wxString &sToolTip)
    {
        wxString sLabel = sToolTip.BeforeFirst(wxT('('));
        if (sLabel.EndsWith(wxT(" ")))
            sLabel.RemoveLast();
        return sLabel;
    }

    static wxString GetToolTipShortcut(const wxString &sToolTip)
    {
        return sToolTip.AfterFirst(wxT('(')).BeforeLast(wxT(')'));
    }

    static wxString AppendToolTipShortcut(const wxString &sToolTip, const wxString &sShortcut)
    {
        wxString sLabel = StripToolTipShortcut(sToolTip);
        if (sShortcut.empty())
            return sLabel;
        
        return sLabel + wxT(" (") + sShortcut + wxT(')');
    }
};
typedef TDynArray<SEdShortcut> TEdShortcutArray;

struct SEdShortcutUtils
{
    class wxToolBarUglyHackToGetToolsBack : public wxToolBar
    {
    public:
        wxToolBarToolsList &GetTools() { return m_tools; }
    };  

    static void AddToolBarItems(TEdShortcutArray &shortcutArray, wxToolBar &toolBar, wxString sPreName = wxString())
    {
        wxToolBarUglyHackToGetToolsBack *toolBarHacked =
            static_cast<wxToolBarUglyHackToGetToolsBack *>( &toolBar );

        wxToolBarToolsList &tools = toolBarHacked->GetTools();
        for ( wxToolBarToolsList::compatibility_iterator node = tools.GetFirst();
              node;
              node = node->GetNext() )
        {
            wxToolBarToolBase *tool = node->GetData();
            wxString label = SEdShortcut::StripToolTipShortcut(tool->GetShortHelp());
            if (!label.empty())
                if (sPreName.empty())
                    shortcutArray.PushBack(SEdShortcut(TXT("Toolbar\\") + label, *tool));
                else
                    shortcutArray.PushBack(SEdShortcut(sPreName + TXT("\\") + label, *tool));
        }
    }
};

#endif
