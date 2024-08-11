/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdSceneEditor;
class CStoryScene;
class CEdUndoManager;
class CUndoDialogSetExistance;

class CEdSceneEditorDialogsetPanel : public wxPanel
{
	friend class CUndoDialogSetExistance;

	DECLARE_EVENT_TABLE();

public:
	CEdSceneEditorDialogsetPanel( CEdSceneEditor* parentEditor );

	void UpdateDialogsetList();
	void UpdateDialogsetListNames();
	void UpdateSlotList( CStorySceneDialogsetInstance* instance );
	void UpdateSlotListNames( CStorySceneDialogsetInstance* instance );

	void MarkCurrentDialogsetInstance( const CName& dialogsetInstanceName );

	void SetUndoManager( CEdUndoManager* undoManager );

protected:
	void OnDialogsetInstanceSelected( wxListEvent& event );
	void OnDialogsetSlotSelected( wxCommandEvent& event );
	void OnNewDialogset( wxCommandEvent& event );
	void OnImportDialogset( wxCommandEvent& event );
	void OnRemoveDialogset( wxCommandEvent& event );
	void OnReloadDialogset( wxCommandEvent& event );
	void OnAddSlot( wxCommandEvent& event );
	void OnRemoveSlot( wxCommandEvent& event );
	void OnDialogsetPropertyChanged( wxCommandEvent& event );
	void OnSlotPropertyChanged( wxCommandEvent& event );
	void OnDuplicateDialogset( wxCommandEvent& event );
	void OnExportDialogset( wxCommandEvent& event );
	void OnDialogsetInstanceNameChangeBegin( wxListEvent& event );
	void OnDialogsetInstanceNameChange( wxListEvent& event );

private:
	CName GetSelectedDialogsetInstanceName() const;

protected:
	CEdSceneEditor*			m_sceneEditor;
	CEdSceneEditor*			m_mediator;

	wxListView*				m_dialogsetList;
	wxListBox*				m_slotList;
	CEdPropertiesPage*		m_dialogsetPropertiesPage;
	CEdPropertiesPage*		m_slotPropertiesPage;
	wxImageList				m_imageList;

	CEdUndoManager*			m_undoManager;
};
