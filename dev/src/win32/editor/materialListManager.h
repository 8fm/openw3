/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdUndoManager;
class CEdAutosizeListCtrl;

/// Manager for material list
class CEdMaterialListManager : public wxPanel, public CDropTarget, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

public:
	//! Get selected material
	RED_INLINE Int32 GetSelectedMaterial() const { return m_selected; }

public:
	CEdMaterialListManager( wxWindow* parent, CObject* owner, const String& rootPath, CEdUndoManager* undoManager );

	~CEdMaterialListManager();

	// Update listbox with materials
	void UpdateMaterialList();

	//! Select material by name
	Bool SelectByName( const String& materialName );

	//! Enable/disable material highlighting
	void EnableMaterialHighlighting( Bool state );

	//!
	void EnableListModification( Bool state );
	void EnableMaterialRemapping( Bool state );

public:
	// Get number of materials
	virtual Int32 GetNumMaterials() const=0;

	//! Get n-th material
	virtual IMaterial* GetMaterial( Int32 index ) const=0;

	//! Get n-th material name
	virtual String GetMaterialName( Int32 index ) const=0;

	//! Set n-th material
	virtual void SetMaterial( Int32 index, IMaterial* material )=0;

	//! Highlight n-th material
	virtual void HighlightMaterial( Int32 index, Bool state )=0;

	//! Rename n-th material. Return false if the name was not changed.
	virtual Bool RenameMaterial( Int32 index, const String& newName ) { return false; }

	//! Add new material. Returns an index of newly added material or -1 on failure.
	virtual Int32 AddMaterial( IMaterial* material, const String& name ) { return -1; }

	//! Remove all unused materials
	virtual Bool RemoveUnusedMaterials() { return false; }

	//! Show remapping dialog
	virtual Bool RemapMaterials() { return false; }

	//! Called after material property has changed
	virtual void MaterialPropertyChanged( CName propertyName, Bool finished ) {}

protected:
	CEdUndoManager* m_undoManager;

	//! Events
	void OnMaterialSelected( wxListEvent& event );
	void OnMaterialDeselected( wxListEvent& event );
	void OnMaterialRenamed( wxListEvent& event );
	void OnItemDoubleClicked( wxListEvent& event );
	void OnUseShader( wxCommandEvent& event );
	void OnUseAllShader( wxCommandEvent& event );
	void OnAddShader( wxCommandEvent& event );
	void OnCreateInstance( wxCommandEvent& event );
	void OnDestroyInstance( wxCommandEvent& event );
	void OnCopyInstance( wxCommandEvent& event );
	void OnRemoveUnused( wxCommandEvent& event );
	void OnRemap( wxCommandEvent& event );
	void OnStdShaderListSelection( wxCommandEvent& event );	
	void OnHighlight( wxCommandEvent& event );
	void OnLocalInstanceChecked( wxCommandEvent& event );
	void OnSaveMaterialInstanceToFile( wxCommandEvent& event );
	void OnUpdateUI( wxUpdateUIEvent& event );
	void OnMaterialPropertiesChanged( wxCommandEvent& event );

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override;
    virtual Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources ) override;

    void ExpandRootItems();

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

private:
	CDropTarget*						m_dropTarget;
	CObject*							m_owner;				//!< Material instance owner	
	Int32								m_selected;				//!< Index of selected material
	CEdPropertiesBrowserWithStatusbar*	m_properties;			//!< Properties
	CEdAutosizeListCtrl*				m_materialList;

	wxChoice*							m_stdShadersList;
	wxButton*							m_applyToAll;
	wxButton*							m_copyBtn;
	wxButton*							m_removeBtn;
	wxButton*							m_remapBtn;
	wxTextCtrl*							m_shaderDescription;
	wxCheckBox*							m_highlightCheck;
	wxCheckBox*							m_instanceCheck;
	wxButton*							m_saveInstanceBtn;
	TDynArray< CDiskFile* >				m_stdShadersFiles;

	Bool IsDirectlyBasedOnGraph( IMaterial* mat ) const;
	void ApplyShader( Uint32 chunkSelection );
	IMaterial* AcquireSelectedStdShader( String* outName = nullptr );
	Bool IsNameUnique( const String& name ) const;
	void UpdateSelectedShaderDesc();
	void ValidateAndFixMaterialInstanceParameters( CMaterialInstance* matInstance );
	CMaterialInstance* CreateNewMaterialInstance( CObject* parent );
};
