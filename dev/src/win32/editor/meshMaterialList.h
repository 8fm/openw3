/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdMeshEditor;

/// List of materials with manager for static mesh
class CEdMeshMaterialList : public CEdMaterialListManager
{
protected:
	CMeshTypeResource*	m_mesh;		//!< Owning mesh
	CEdMeshEditor*		m_editor;

public:
	CEdMeshMaterialList( wxWindow* parent, CEdMeshEditor* editor, CMeshTypeResource* mesh, CEdUndoManager* undoManager );

	// Get number of materials
	virtual Int32 GetNumMaterials() const override;

	//! Get n-th material
	virtual IMaterial* GetMaterial( Int32 index ) const override;

	//! Get n-th material name
	virtual String GetMaterialName( Int32 index ) const override;

	//! Set n-th material
	virtual void SetMaterial( Int32 index, IMaterial* material ) override;

	//! Highlight n-th material
	virtual void HighlightMaterial( Int32 index, Bool state ) override;

	//!
	virtual Bool RenameMaterial( Int32 index, const String& newName ) override;

	//! Add new material
	virtual Int32 AddMaterial( IMaterial* material, const String& name ) override;

	//!
	virtual Bool RemoveUnusedMaterials() override;

	//!
	virtual Bool RemapMaterials() override;

	//! Called after material property has changed
	virtual void MaterialPropertyChanged( CName propertyName, Bool finished ) override;

};


