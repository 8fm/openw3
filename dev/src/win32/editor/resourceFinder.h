/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct SOccurenceData
{
	struct Info
	{
		THandle< CEntity >	m_entity;
		Int32				m_count;
		Info() : m_count( 1 ) {}
	};

	TDynArray< Info > m_occurences;
	CResource*		  m_resource;

	void CollectOccurences( CWorld* world, CResource* resource, const CName& componentClassName );

private:
	Info* FindOccurenceInfo( CEntity* entity );
	void IncludeOccurence( CEntity* entity );
};

/// A tool for finding specific resources in a loaded world
class CEdResourceFinder 
	: public wxSmartLayoutPanel
	, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:
	static void ShowForResource( CResource* resource, const CName& componentClassName = CName() );

	~CEdResourceFinder();

private:
	CEdResourceFinder( wxWindow* parent, CResource* resource, const CName& componentClassName ); // not to be created directly, use ShowForResource
	 
	static TSortedMap< String, CEdResourceFinder* >	s_resourceFinderWindows;

	wxListBox*		m_listBox;
	wxCheckBox*		m_selectInWorldCheckBox;
	CResource*		m_resource;
	SOccurenceData	m_occurenceData;
	Bool			m_pauseSelectionUpdate;
	const CName		m_componentClassName;

	void RefreshList();
	void UpdateWorldSelection();
	TDynArray< CEntity* > ExtractSelectedEntities();

	void OnGotoResource( wxCommandEvent &event );
	void OnDelete( wxCommandEvent &event );
	void OnDelete2( wxCommandEvent &event );
	void OnSelectAll( wxCommandEvent &event );
	void OnUnselectAll( wxCommandEvent &event );
	void OnRefresh( wxCommandEvent &event );
	void OnFocus( wxFocusEvent& event );
	void OnSelectionChanged( wxCommandEvent& event );
	void OnCopyPath( wxCommandEvent &event );
	void OnReplaceButton( wxCommandEvent &event );
	void OnReplace( wxCommandEvent &event );
	void OnUpdateUI( wxUpdateUIEvent& event );

	void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;
};
