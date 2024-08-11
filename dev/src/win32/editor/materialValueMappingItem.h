/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CEdMaterialValueMappingEditor;
class IEdMaterialValueMappingMaterialsInterface;
class IEdMaterialValueMappingValuesInterface;


/// Allows matching each material used by a MeshTypeResource to some other value. The property must be a
/// TDynArray<>, and must be on something that uses a MeshTypeResource. Each such "something" will need to
/// be handled by GetMeshTypeResource().
class CEdMaterialValueMappingPropertyItem : public ICustomPropertyEditor, public wxEvtHandler
{
protected:
	Bool										m_inlined;

	IEdMaterialValueMappingMaterialsInterface*	m_materialsInterface;
	IEdMaterialValueMappingValuesInterface*		m_valuesInterface;

public:
	CEdMaterialValueMappingPropertyItem( CPropertyItem* propertyItem, Bool inlined );
	~CEdMaterialValueMappingPropertyItem();

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;

protected:
	void OnOpenEditor( wxCommandEvent& event );
	void OnEditorClosed( wxCloseEvent& event );
};

