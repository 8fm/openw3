/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CLootDefinitions;
class CLootDefinitionBase;

class CEdLootEditor : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

public:

	CEdLootEditor( wxWindow* parent );
	~CEdLootEditor();

	void OnSave( wxCommandEvent &event );
	void OnSubmit( wxCommandEvent &event );
	void OnExit( wxCommandEvent &event );
	void OnReloadDefinitions( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );

	void OnFilenameSelected( wxCommandEvent &event );
	void OnDefinitionSelected( wxCommandEvent &event );
	void OnAddDefinition( wxCommandEvent &event );
	void OnDeleteDefinition( wxCommandEvent &event );
	void OnPropertiesChanged( wxCommandEvent &event );

private:

	void InitData();
	void InitPropertiesControl();
	void UpdateDefinitionsListBox();
	CName FindUniqueDefinitionName() const;

	Bool Load( const String& filename );
	Bool Save( const String& filename );

	Bool CheckoutFile( const String& filename );
	Bool SaveChangedData();
	Bool SaveData();
	Bool SubmitData();

	CLootDefinitions					*m_lootDefinitions;
	TDynArray< String >					m_filenames;
	String								m_currentFilename;
	TDynArray< CName >					m_definitionsNames;
	CEdPropertiesBrowserWithStatusbar*	m_properties;

	wxComboBox*							m_comboFilenames;
	wxListBox*							m_listDefinitions;

	Bool								m_dataChangedNotSaved;

	Red::System::GUID					m_creatorTag;
};