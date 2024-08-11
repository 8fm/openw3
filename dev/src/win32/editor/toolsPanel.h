/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORYTOOLSPANEL_H
#define EDITORYTOOLSPANEL_H

/// Tools panel
class CEdToolsPanel : public wxPanel, public IEdEventListener, public ISavableToConfig
{
	wxDECLARE_CLASS( CEdToolsPanel );

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );

protected:
	struct ToolInfo
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Editor );
		CClass*				m_toolClass;		//!< Tool class
		CEdToggleButton*	m_button;			//!< Tool button

		ToolInfo( CClass* toolClass, wxWindow* parent, wxSizer* sizer, Uint32 index );
		~ToolInfo();
	};

	class ToolInfoWrapper : public wxObject
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	public:
		ToolInfo*		m_tool;

	public:
		ToolInfoWrapper ( ToolInfo* tool )
			: m_tool( tool )
		{};
	};

public:
	struct UndoEvent
	{
		CEdToolsPanel * m_toolsPanel;
		CWorld        * m_world;
		CClass        * m_toolClass;
	};

protected:
	CEdRenderingPanel*			m_viewport;					// Associated viewport
	wxGridSizer*				m_buttonSizer;				// Button sizer
	wxBoxSizer*					m_panelSizer;				// Tool panel sizer
	wxPanel*					m_panel;					// Custom shit panel
	TDynArray< ToolInfo* >		m_tools;					// Tools
	IEditorTool*				m_tool;						// Active editor tool
	CSelectionManager::
	ESelectionGranularity		m_originalSelectionMode;	// Original selection mode
	TDynArray< CEntity* >		m_originalSelectionEnt;		// Original entity selection
	TDynArray< CComponent* >	m_originalSelectionComp;	// Original component selection
	wxAcceleratorTable			m_prevTable;				//!< Accelerators before the tool has been activated

public:
	CEdToolsPanel( wxWindow* parent, CEdRenderingPanel* viewport );
	~CEdToolsPanel();

	// Start tool
	IEditorTool* StartTool( CClass* toolClass );

	// Cancel tool
	void CancelTool();

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );

    TEdShortcutArray GetAccelerators();

	TEdShortcutArray * GetCurrentAccelerators();
	TEdShortcutArray   GetAllAccelerators();
	void OnAccelerator( wxCommandEvent& event );

protected:
	void RestoreSelection();

	void OnSelectTool( wxCommandEvent& event );
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
    void ConnectAccelerators();
};

#endif
