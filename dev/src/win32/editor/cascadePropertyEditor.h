/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdCascadeArrayEditor;

class CEdCascadePropertyEditor : public wxScrolledWindow
{
protected:
	CEdPropertiesPage*	m_propertyPage;

	wxSizer*	m_arraysSizer;
	TDynArray< CEdCascadeArrayEditor* >	m_arrayEditors;

	Bool	m_cascadeOnlySelectedProperties;

	STypedObject m_typedObject;

public:
	CEdCascadePropertyEditor( wxWindow* parent, Bool topLevel = true, Bool cascadeOnlySelectedProperties = false );
	~CEdCascadePropertyEditor();

protected:
	Bool FindPropertyOwner( const CProperty* propertyToLookFor, STypedObject& object, STypedObject& propertyOwner );

	void ProcessComplexArrayProperties( STypedObject typedObject );
	
	void CreateArrayEditor( CProperty* property, STypedObject typedObject );
	void ClearArrayEditors();

	void RefreshLayout();

public:
	void SetNoObject();
	void SetTypedObject( STypedObject typedObject );
	
	template< class T >
	void SetObject( T* object )
	{
		/*ClearArrayEditors();
		
		if ( m_propertyPage != NULL )
		{
			m_propertyPage->SetObject( object );
		}
		if ( object != NULL )
		{
			ProcessComplexArrayProperties( object->GetClass(), object );
		}

		RefreshLayout();*/
		SetTypedObject( STypedObject( object, object->GetClass() ) );
	}

	

protected:
	void OnPropertySelected( wxCommandEvent& event );
	void OnPropertyChanged( wxCommandEvent& event );

};

//////////////////////////////////////////////////////////////////////////

class CEdCascadeArrayEditor : public wxPanel
{
protected:
	wxStaticText*				m_arrayName;
	wxListBox*					m_arrayElementList;
	wxBitmapButton*				m_arrayAddButton;
	wxBitmapButton*				m_arrayDeleteButton;
	wxBitmapButton*				m_arrayClearButton;

	CEdCascadePropertyEditor*	m_elementEditor;

	void*						m_arrayObject;
	CRTTIArrayType*				m_arrayType;
	CProperty*					m_parentProperty;
	CObject*					m_parentObject;


public:
	CEdCascadeArrayEditor( wxWindow* parent, Bool cascadeOnlySelectedProperties = false );
	~CEdCascadeArrayEditor();

	void SetArrayData( CProperty* arrayProperty, STypedObject parentTypedObject );
	void AddArrayElement( CClass* objectClass );

	RED_INLINE CProperty*	GetParentProperty() const { return m_parentProperty; }

protected:
	void OnArrayElementSelected( wxCommandEvent& event );
	void OnArrayElementAdd( wxCommandEvent& event );
	void OnArrayElementAddDerivedClass( wxCommandEvent& event );
	void OnArrayElementDelete( wxCommandEvent& event );
	void OnArrayClear( wxCommandEvent& event );
};
