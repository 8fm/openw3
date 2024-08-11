/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define ID_ACCEL_FILTER_FIRST   9200
#define ID_ACCEL_FILTER_LAST    9900

#include <wx/treebook.h>

enum EViewportFlagsType
{
	VFT_EDITOR,
	VFT_GAME,
	VFT_PREVIEW,
};

// Show flags filter panel
class CEdFilterPanel : public wxPanel, public ISavableToConfig
{
	struct ClassTemplatesFlags
	{
		wxCheckListBox* m_templatesListBox;
		wxCheckBox* m_selectAllCBox;

		ClassTemplatesFlags( wxCheckBox* selectAllCBox, wxCheckListBox* lBox )
			: m_selectAllCBox( selectAllCBox )
			, m_templatesListBox( lBox )
		{}

		void Enable( Bool enable )
		{
			if ( m_templatesListBox ){ m_templatesListBox->Enable( enable ); }
			if ( m_selectAllCBox ) { m_selectAllCBox->Enable( enable ); }
		}

		Uint32 GetTemplatesCount() { return m_templatesListBox ? m_templatesListBox->GetCount() : 0; }

		Bool IsTemplateChecked( Uint32 i ) { return ( i >= 0 && i < GetTemplatesCount() ) ? m_templatesListBox->IsChecked( i ) : false; }
		String GetTemplateName( Uint32 i ) { return ( i >= 0 && i < GetTemplatesCount() ) ? String( m_templatesListBox->GetString( i ).c_str() ) : String::EMPTY; }
		
		Bool IsAnyTemplateChecked()
		{
			for ( Uint32 i = 0; i < m_templatesListBox->GetCount(); ++i ) 
			{
				if ( m_templatesListBox->IsChecked( i ) )
				{
					return true;
				}
			}
			return false;
		}

		Bool IsAnyTemplateUnchecked()
		{
			for ( Uint32 i = 0; i < m_templatesListBox->GetCount(); ++i ) 
			{
				if ( !m_templatesListBox->IsChecked( i ) )
				{
					return true;
				}
			}
			return false;
		}

		void CheckTemplate( Uint32 i, Bool checked ) { if ( i >= 0 && i < GetTemplatesCount() ) { m_templatesListBox->Check( i, checked ); } }
		void CheckAllTemplates( Bool checked ) { for ( Uint32 i = 0; i < m_templatesListBox->GetCount(); ++i ) { m_templatesListBox->Check( i, checked ); } }
		void CheckSelectAllBox( wxCheckBoxState state ) { if ( m_selectAllCBox ) { m_selectAllCBox->Set3StateValue( state ); } }
	};

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
protected:
	IViewport*									m_viewport;
	THashMap< Uint32, wxCheckBox*	>			m_gameOnlyFlags;
	THashMap< Uint32, wxCheckBox*	>			m_editorOnlyFlags;
	THashMap< Uint32, wxCheckBox* >				m_gameSimulationFlags;
	THashMap< Uint32, wxCheckBox* >				m_editorSimulationFlags;
	THashMap< Uint32, wxCheckBox* >				m_previewOnlyFlags;
	THashMap< CClass*, wxCheckBox* >			m_gameClassFlags;
	THashMap< CClass*, wxCheckBox* >			m_editorClassFlags;
	THashMap< String, ClassTemplatesFlags >		m_gameClassesTemplatesFlags;
	THashMap< String, ClassTemplatesFlags >		m_editorClassesTemplatesFlags;

	TEdShortcutArray							m_shortcuts;
	TDynArray< Uint32 >							m_mapIndexToFlag;
	TDynArray< CEdPreviewPanel* >				m_previewPanels;
	wxTreebook*									m_book;

	wxSlider*									m_commonRenderDistanceSlider;
	wxSlider*									m_commonRenderLineThicknessSlider;
	Float										m_debugMaxDistance;
	Float										m_debugLinesThickness;

public:
	CEdFilterPanel( wxWindow* parent, IViewport* viewport );
	~CEdFilterPanel();
	const EShowFlags* GetViewportFlags( EViewportFlagsType type ) const;
	void SetViewportFlag( EViewportFlagsType type, EShowFlags flag, Bool b );
	void UpdateSimulationOptions( Bool game );

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	Bool GetSimulationEditorFlag( Uint32 flag );

	TEdShortcutArray *GetAccelerators();
	void OnAccelFilter( wxCommandEvent& event );

	void RegisterPreviewPanel( CEdPreviewPanel* panel );
	void UnregisterPreviewPanel( CEdPreviewPanel* panel );
	void UpdatePreviewPanel( CEdPreviewPanel* p );

	Float GetVisualDebugMaxRenderingDistance() { return m_debugMaxDistance; }
	Float GetDebugLinesThickness() { return m_debugLinesThickness; }

protected:
	void GetFlagsFromConfig( CConfigurationManager &config, THashMap< Uint32, wxCheckBox*	> &storage, const EShowFlags* defaultFlags );
	void GetFlagsFromConfig( CConfigurationManager &config, THashMap< CClass*, wxCheckBox*	> &storage );
	void GetFlagsFromConfig( CConfigurationManager &config, THashMap< String, ClassTemplatesFlags	> &storage );

	void WriteFlagsToConfig( CConfigurationManager &config, THashMap< Uint32, wxCheckBox*	> &storage );
	void WriteFlagsToConfig( CConfigurationManager &config, THashMap< CClass*, wxCheckBox*	> &storage );
	void WriteFlagsToConfig( CConfigurationManager &config, THashMap< String, ClassTemplatesFlags	> &storage );
	
	wxSizer* AddPage( const String &name, Bool mainPage = false );
	void AddOption( THashMap< Uint32, wxCheckBox*	> &storage, wxSizer* sizer, const String& name, EShowFlags flag );
	void AddOption( THashMap< CClass*, wxCheckBox*	> &storage, wxSizer* sizer, CClass* type );
	void AddOption( THashMap< String, ClassTemplatesFlags > &classesListBoxes, TDynArray< String >& templates, wxSizer* sizer, CClass* type );
	void UpdateFlags();

	void AddOptions( THashMap< Uint32, wxCheckBox* >& opts, wxSizer* sizer, const EShowFlags* filter );
	void AddClassOptions( THashMap< CClass*, wxCheckBox* >& opts, THashMap< String, ClassTemplatesFlags >& choices, wxSizer* sizer );
	void AddSimulationOption( THashMap< Uint32, wxCheckBox* > &storage, wxSizer* sizer, const String& name, Uint32 flag );

	void GatherAreaTemplates( CDirectory* dir, THashMap< CClass*, TDynArray< String > >& outEntitiesPerClasses ) const;

	void ExportFiltersForGame();

	void OnFlagUpdate( wxCommandEvent& event );
	void OnSelectAll( wxCommandEvent& event );
	void OnExportFiltersForGame( wxCommandEvent& event );
	void OnChangeSimulationOption( wxCommandEvent& event );

private:
	static Bool IsInFilter( const EShowFlags* filter, Int32 value );
	static void BuildFilterByAnd( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2 );
	static void BuildFilterByNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2 );
	static void BuildFilterByAndNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2, const EShowFlags* and3 );
	static void BuildFilterByNotNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2, const EShowFlags* and3 );
};
