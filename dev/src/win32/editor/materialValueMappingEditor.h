/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

wxDECLARE_EVENT( wxEVT_COMMAND_MATERIALVALUEMAPPING_CHANGED, wxCommandEvent );

class CEdMaterialPreviewPanel;
class CEdPropertiesBrowserWithStatusbar;


class IEdMaterialValueMappingMaterialsInterface
{
public:
	virtual ~IEdMaterialValueMappingMaterialsInterface() {}

	// NumMaterials is assumed to be constant for the lifetime of this interface.
	virtual Uint32		GetNumMaterials() const = 0;
	virtual IMaterial*	GetMaterial( Uint32 index ) const = 0;
	virtual String		GetMaterialName( Uint32 index ) const = 0;
};

class IEdMaterialValueMappingValuesInterface
{
public:
	virtual ~IEdMaterialValueMappingValuesInterface() {}

	// Gives the implementation a chance to reserve space for how many values are needed. Get/SetValue
	// will provide indices within this range.
	virtual void SetNumValues( Uint32 num ) = 0;

	virtual Uint32 GetNumValues() const = 0;

	// The type of value being edited. This is used to allocate space to hold all values while
	// editing.
	virtual IRTTIType* GetValueType() const = 0;

	// Get current value for the given index. outData is already allocated to fit the value.
	virtual void GetValue( Uint32 index, void* outData ) const = 0;

	// Set new value for the given index. data is non-NULL and points to the new value.
	virtual void SetValue( Uint32 index, const void* data ) = 0;
};


/// Allows editing of a collection of values, where there is one value for each of a number of materials.
/// It is assumed that the materials and values have a consistent ordering, and the number/order does not change
/// during the lifetime of this editor panel.
///
/// The interface classes above provide a general way for the editor to handle materials and values coming
/// from an unknown source.
class CEdMaterialValueMappingEditor : private wxDialog
{
protected:
	CEdMaterialPreviewPanel*					m_materialPreview;			// Preview of currently selected material
	CEdPropertiesBrowserWithStatusbar*			m_materialProperties;		// Properties page for selected material, for artist's reference.
	CEdPropertiesBrowserWithStatusbar*			m_valueEditProperties;		// Properties page for editing per-material values.

	CClass*										m_mappingClass;				// A temporary CClass containing a property for each material.

	void*										m_values;					// Buffer holding local copy of values. Byte-compatible with the
																			// temporary class defined by m_mappingClass, so we can show it
																			// in a properties panel!
	Uint32										m_numMaterials;

	IEdMaterialValueMappingMaterialsInterface*	m_materialsInterface;
	IEdMaterialValueMappingValuesInterface*		m_valuesInterface;

	wxWindow*									m_rootParent;

public:
	CEdMaterialValueMappingEditor( wxWindow* parent, IEdMaterialValueMappingMaterialsInterface* materials, IEdMaterialValueMappingValuesInterface* values, const String& valueHint, Bool inlined );
	~CEdMaterialValueMappingEditor();

	void Execute();

protected:

	// Grab all current values and set them up in the editor window.
	void Initialize();

	Int32 FindMaterialIndex( const String& name ) const;

	void UpdatePosition( wxWindow* parent );
	void OnMaterialSelected( wxCommandEvent& event );
	void OnValueChanged( wxCommandEvent& event );

	// property editors should be modal, but this one contains material preview, which means it needs the main events loop to be running.
	// We cannot show it using ShowModal as it's shown in he event handler. Instead it will disable and enable its parent on showing and closing.
	void OnClose( wxCloseEvent& event );
	void OnShow( wxShowEvent& event );
};

