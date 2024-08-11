/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "cascadePropertyEditor.h"

CEdCascadePropertyEditor::CEdCascadePropertyEditor( wxWindow* parent, Bool topLevel, Bool cascadeOnlySelectedProperties )
	: wxScrolledWindow( parent )
	, m_cascadeOnlySelectedProperties( cascadeOnlySelectedProperties )
	, m_typedObject()
{
	wxBoxSizer* horizontalSizer = new wxBoxSizer( wxHORIZONTAL );
	
	PropertiesPageSettings propertiesSettings;
	propertiesSettings.m_autoExpandGroups = false;
	propertiesSettings.m_showEntityComponents = false;
	//TODO:		propertiesSettings.m_ignoreInlinedArrays = true;

	m_propertyPage = new CEdPropertiesPage( this, propertiesSettings, nullptr );
	m_propertyPage->SetMinSize( wxSize( 400, 200 ) );
	m_propertyPage->SetMaxSize( wxSize( 400, -1 ) );

	if ( m_cascadeOnlySelectedProperties == true )
	{
		m_propertyPage->Bind( wxEVT_COMMAND_PROPERTY_SELECTED, &CEdCascadePropertyEditor::OnPropertySelected, this );
	}
	m_propertyPage->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdCascadePropertyEditor::OnPropertyChanged, this );

	horizontalSizer->Add( m_propertyPage, 0, wxEXPAND );

	m_arraysSizer = new wxBoxSizer( wxVERTICAL );

	horizontalSizer->Add( m_arraysSizer );

	if ( topLevel == true )
	{
		SetScrollRate( 5, 5 );
	}
	
	SetSizer( horizontalSizer );
	Layout();
}

CEdCascadePropertyEditor::~CEdCascadePropertyEditor()
{
	if ( m_propertyPage != NULL )
	{
		delete m_propertyPage;
		m_propertyPage = NULL;
	}
	
}

void CEdCascadePropertyEditor::ProcessComplexArrayProperties( STypedObject typedObject )
{
	if ( typedObject.m_class == NULL )
	{
		return;
	}

	if ( m_cascadeOnlySelectedProperties == true )
	{
		return;
	}

	TDynArray< CProperty* > properties;
	typedObject.m_class->GetProperties( properties );

	for ( TDynArray< CProperty* >::iterator propertyIter = properties.Begin(); propertyIter != properties.End(); ++propertyIter )
	{
		CProperty* property = *propertyIter;
		if ( property->IsInlined() == true )
		{
			if ( property->GetType()->GetType() == RT_Array )
			{
				CreateArrayEditor( property, typedObject );
			}
			else if ( property->GetType()->GetType() == RT_Pointer )
			{
				CObject* pointedObject( NULL );
				property->Get( typedObject.m_object, &pointedObject );
				if ( pointedObject != NULL )
				{
					ProcessComplexArrayProperties( STypedObject( pointedObject ) );
				}
			}
			else if ( property->GetType()->GetType() == RT_Handle )
			{
				THandle< CObject > pointedHandle;
				property->Get( typedObject.m_object, &pointedHandle );
				CObject* pointedObject = pointedHandle.Get();
				if ( pointedObject != NULL )
				{
					ProcessComplexArrayProperties( STypedObject( pointedObject ) );
				}
			}
			else if ( property->GetType()->GetType() == RT_Class )
			{
				void* innerObject;
				property->Get( typedObject.m_object, &innerObject );
				CClass* innerClass = static_cast< CClass* >( property->GetType() );
				if ( innerObject != NULL )
				{
					ProcessComplexArrayProperties( STypedObject( innerObject, innerClass ) );
				}
			}
		}
	}
}

Bool CEdCascadePropertyEditor::FindPropertyOwner( const CProperty* propertyToLookFor, STypedObject& object, STypedObject& propertyOwner )
{
	TDynArray< CProperty* > properties;
	object.m_class->GetProperties( properties );

	for ( TDynArray< CProperty* >::iterator propertyIter = properties.Begin(); propertyIter != properties.End(); ++propertyIter )
	{
		CProperty* property = *propertyIter;

		if ( property == propertyToLookFor )
		{
			propertyOwner = object;
			return true;
		}

		if ( property->IsInlined() == true )
		{
			if ( property->GetType()->GetType() == RT_Pointer )
			{
				CObject* pointedObject( NULL );
				property->Get( object.m_object, &pointedObject );
				if ( pointedObject != NULL )
				{
					if ( FindPropertyOwner( propertyToLookFor, STypedObject( pointedObject, pointedObject->GetClass() ), propertyOwner ) == true )
					{
						return true;
					}
				}
			}
			else if ( property->GetType()->GetType() == RT_Handle )
			{
				THandle< CObject > pointedHandle;
				property->Get( object.m_object, &pointedHandle );
				CObject* pointedObject = pointedHandle.Get();
				if ( pointedObject != NULL )
				{
					if ( FindPropertyOwner( propertyToLookFor, STypedObject( pointedObject, pointedObject->GetClass() ), propertyOwner ) == true )
					{
						return true;
					}
				}
			}
			else if ( property->GetType()->GetType() == RT_Class )
			{
				void* innerObject;
				property->Get( object.m_object, &innerObject );
				CClass* innerClass = static_cast< CClass* >( property->GetType() );
				if ( innerObject != NULL )
				{
					if ( FindPropertyOwner( propertyToLookFor, STypedObject( innerObject, innerClass ), propertyOwner ) == true )
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

void CEdCascadePropertyEditor::CreateArrayEditor( CProperty* property, STypedObject typedObject )
{
	CEdCascadeArrayEditor* arrayEditor = new CEdCascadeArrayEditor( m_arraysSizer->GetContainingWindow(), m_cascadeOnlySelectedProperties );
	m_arraysSizer->Add( arrayEditor, ( m_cascadeOnlySelectedProperties == true ) ? 1 : 0 , wxEXPAND );
	m_arrayEditors.PushBack( arrayEditor );

	arrayEditor->SetArrayData( property, typedObject );

	//arrayEditor->Show( m_cascadeOnlySelectedProperties == false );
	arrayEditor->Show();
}

void CEdCascadePropertyEditor::ClearArrayEditors()
{
	m_arraysSizer->Clear( true );
	m_arrayEditors.Clear();
	
}

void CEdCascadePropertyEditor::SetTypedObject( STypedObject typedObject )
{
	ClearArrayEditors();

	m_typedObject = typedObject;

	if ( m_propertyPage != NULL )
	{
		m_propertyPage->SetTypedObject( typedObject );
	}
	if ( typedObject.m_object != NULL && typedObject.m_class != NULL )
	{
		ProcessComplexArrayProperties( typedObject );
	}

	RefreshLayout();
}

void CEdCascadePropertyEditor::RefreshLayout()
{
	wxWindow* window = this;
	while ( window->IsTopLevel() == false && window->GetParent() != NULL )
	{
		window = window->GetParent();
	}
	window->Freeze();
	window->Layout();
	window->Thaw();
}

void CEdCascadePropertyEditor::SetNoObject()
{
	ClearArrayEditors();
	m_typedObject = STypedObject();
	if ( m_propertyPage != NULL )
	{
		m_propertyPage->SetNoObject();
	}
	RefreshLayout();
}

void CEdCascadePropertyEditor::OnPropertySelected( wxCommandEvent& event )
{
	//ASSERT( m_arrayEditors.Size() <= 1 );
	if ( event.GetClientData() != NULL )
	{
		CProperty* property = static_cast< CProperty* >( event.GetClientData() ); 

		/*Bool isLayoutValid = true;

		for ( TDynArray< CEdCascadeArrayEditor* >::iterator arrayEditorIter = m_arrayEditors.Begin(); 
			arrayEditorIter != m_arrayEditors.End(); ++arrayEditorIter )
		{
			CEdCascadeArrayEditor* arrayEditor = *arrayEditorIter;

			isLayoutValid &= arrayEditor->Show( arrayEditor->GetParentProperty() == property ) == false;
			arrayEditor->SetMinSize( wxSize( -1, ( arrayEditor->GetParentProperty() == property ) ? GetSize().y : -1 ) );
		}

		if ( isLayoutValid == false )
		{
			RefreshLayout();
		}*/

		if ( m_arrayEditors.Size() == 1 && m_arrayEditors[ 0 ]->GetParentProperty() == property )
		{
			return;
		}

		Bool isLayoutValid = true;

		if ( m_arrayEditors.Empty() == false )
		{
			ClearArrayEditors();
			isLayoutValid = false;
		}
		
		if ( property->IsInlined() == true && property->GetType()->GetType() == RT_Array )
		{
			STypedObject propertyParent;
			if ( FindPropertyOwner( property, m_typedObject, propertyParent ) == true )
			{
				CreateArrayEditor( property, propertyParent );
			}
			
			isLayoutValid = false;
		}

		if ( isLayoutValid == false )
		{
			RefreshLayout();
		}
	}
	
}

void CEdCascadePropertyEditor::OnPropertyChanged( wxCommandEvent& event )
{
	//if ( event.GetClientData() != NULL )
	//{
	//	CProperty* property = static_cast< CProperty* >( event.GetClientData() ); 

	//	TDynArray< Uint32 > invalidatedArraysIndices;
	//	for( Uint32 i = 0; i < m_arrayEditors.Size(); ++i )
	//	{
	//		//m_arrayEditors[ 0 ]->GetParentProperty()->
	//	}

	//}
}

//////////////////////////////////////////////////////////////////////////

CEdCascadeArrayEditor::CEdCascadeArrayEditor( wxWindow* parent, Bool cascadeOnlySelectedProperties )
	: wxPanel( parent )
	, m_parentObject( NULL )
{
	if ( cascadeOnlySelectedProperties == true )
	{
		SetMinSize( wxSize( -1, parent->GetSize().y ) );
	}

	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
	//wxBoxSizer* mainSizer = new wxStaticBoxSizer( wxVERTICAL );
	wxBoxSizer* contentSizer = new wxBoxSizer( wxHORIZONTAL );
	wxBoxSizer* arraySizer = new wxBoxSizer( wxVERTICAL );
	wxBoxSizer* arrayButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_arrayName = new wxStaticText( this, wxID_ANY, "No array" );

	mainSizer->Add( m_arrayName, 0, wxEXPAND );

	m_arrayElementList = new wxListBox( this, wxID_ANY );
	m_arrayElementList->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdCascadeArrayEditor::OnArrayElementSelected, this );
	m_arrayElementList->SetMinSize( wxSize( 200, 200 ) );

	arraySizer->Add( m_arrayElementList, 1, wxEXPAND );

	wxBitmap addBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_ADD") );
	m_arrayAddButton = new wxBitmapButton( this, wxID_ANY, addBitmap );
	m_arrayAddButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdCascadeArrayEditor::OnArrayElementAdd, this );

	wxBitmap deleteBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_arrayDeleteButton = new wxBitmapButton( this, wxID_ANY, deleteBitmap );
	m_arrayDeleteButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdCascadeArrayEditor::OnArrayElementDelete, this );

	wxBitmap clearBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
	m_arrayClearButton = new wxBitmapButton( this, wxID_ANY, clearBitmap );
	m_arrayClearButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdCascadeArrayEditor::OnArrayClear, this );

	arrayButtonSizer->Add( m_arrayAddButton, 2 );
	arrayButtonSizer->Add( m_arrayDeleteButton, 1 );
	arrayButtonSizer->Add( m_arrayClearButton, 1 );

	arraySizer->Add( arrayButtonSizer, 0, wxEXPAND );

	contentSizer->Add( arraySizer, 0, wxEXPAND );

	m_elementEditor = new CEdCascadePropertyEditor( this, false, cascadeOnlySelectedProperties );
	m_elementEditor->Hide();

	contentSizer->Add( m_elementEditor, 0, wxEXPAND );

	mainSizer->Add( contentSizer, 1, wxEXPAND );

	SetSizer( mainSizer );
	Layout();
}

CEdCascadeArrayEditor::~CEdCascadeArrayEditor()
{

}

void CEdCascadeArrayEditor::SetArrayData( CProperty* arrayProperty, STypedObject parentTypedObject )
{
	m_arrayName->SetLabel( wxString::Format( "%s::%s", arrayProperty->GetParent()->GetName().AsString().AsChar(), arrayProperty->GetName().AsString().AsChar() ) );
	m_arrayElementList->Clear();
	
	m_parentProperty = arrayProperty;
	m_arrayObject = arrayProperty->GetOffsetPtr( parentTypedObject.m_object );
	m_arrayType = static_cast< CRTTIArrayType* >( arrayProperty->GetType() );

	if ( parentTypedObject.m_class != NULL && parentTypedObject.m_class->IsObject() == true )
	{
		m_parentObject = static_cast< CObject* >( parentTypedObject.m_object );
	}
	else
	{
		m_parentObject = NULL;
	}
	
	Uint32 arraySize = m_arrayType->GetArraySize( m_arrayObject );
	
	for ( Uint32 i = 0; i < arraySize; ++i )
	{
		String elementDescription;
		m_arrayType->GetInnerType()->ToString( m_arrayType->GetArrayElement( m_arrayObject, i ), elementDescription );

		m_arrayElementList->Append( elementDescription.AsChar() );
	}
	
}

void CEdCascadeArrayEditor::AddArrayElement( CClass* objectClass )
{
	ASSERT( objectClass );
	ASSERT( !objectClass->IsAbstract() );

	m_arrayType->AddArrayElement( m_arrayObject );
	
	Uint32 createdElementIndex = m_arrayType->GetArraySize( m_arrayObject ) - 1;
	void* createdElement = m_arrayType->GetArrayElement( m_arrayObject, createdElementIndex );

	if ( m_arrayType->GetInnerType()->GetType() == RT_Pointer )
	{
		CObject** arrayElement = static_cast< CObject** >( createdElement );
		*arrayElement = CreateObject< CObject >( objectClass, m_parentObject, OF_Inlined );
	}
	else if ( m_arrayType->GetInnerType()->GetType() == RT_Handle )
	{
		THandle< CObject >* arrayElement = static_cast< THandle< CObject >* >( createdElement );
		*arrayElement = ( CreateObject< CObject >( objectClass, m_parentObject, OF_Inlined ) );
	}

	String elementDescription;
	m_arrayType->GetInnerType()->ToString( createdElement, elementDescription );

	m_arrayElementList->Append( elementDescription.AsChar() );

	if ( m_parentObject != NULL )
	{
		m_parentObject->MarkModified();
	}
}

void CEdCascadeArrayEditor::OnArrayElementSelected( wxCommandEvent& event )
{
	m_elementEditor->Show();
	if ( m_arrayType->GetInnerType()->GetType() == RT_Pointer )
	{
		CObject** selectedObject = static_cast< CObject** >( m_arrayType->GetArrayElement( m_arrayObject, (Uint32) event.GetSelection() ) );

		m_elementEditor->SetObject( *selectedObject );
	}
	else if ( m_arrayType->GetInnerType()->GetType() == RT_Handle )
	{
		THandle< CObject >* selectedHandle = static_cast< THandle< CObject >* >( m_arrayType->GetArrayElement( m_arrayObject, (Uint32) event.GetSelection() ) );
		if ( selectedHandle != NULL )
		{
			m_elementEditor->SetObject( selectedHandle->Get() );
		}
	}
	else if ( m_arrayType->GetInnerType()->GetType() == RT_Class )
	{
		void* objectData = m_arrayType->GetArrayElement( m_arrayObject, (Uint32) event.GetSelection() );
		CClass* objectClass = static_cast< CClass* >( m_arrayType->GetInnerType() );

		if ( objectData != NULL )
		{
			m_elementEditor->SetTypedObject( STypedObject( objectData, objectClass ) );
		}
	}
	
	
}

void CEdCascadeArrayEditor::OnArrayElementAdd( wxCommandEvent& event )
{
	CClass* objectClass = NULL;
	if ( m_arrayType->GetInnerType()->GetType() == RT_Class )
	{
		objectClass = static_cast< CClass* >( m_arrayType->GetInnerType() );
	}
	else if ( m_arrayType->GetInnerType()->GetType() == RT_Pointer )
	{
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( m_arrayType->GetInnerType() );
		ASSERT( pointerType->GetPointedType()->GetType() == RT_Class );
		objectClass = static_cast< CClass* >( pointerType->GetPointedType() );
	}
	else if ( m_arrayType->GetInnerType()->GetType() == RT_Handle )
	{
		CRTTIHandleType* handleType = static_cast< CRTTIHandleType* >( m_arrayType->GetInnerType() );
		ASSERT( handleType->GetPointedType()->GetType() == RT_Class );
		objectClass = static_cast< CClass* >( handleType->GetPointedType() );
	}

	if ( objectClass == NULL )
	{
		return;
	}

	TDynArray< CClass* > derivedClasses;
	SRTTI::GetInstance().EnumDerivedClasses( objectClass, derivedClasses );

	if ( derivedClasses.Empty() == true )
	{
		AddArrayElement( objectClass );
	}
	else
	{
		wxMenu popup;

		for ( Uint32 i=0; i<derivedClasses.Size(); i++ )
		{
			popup.Append( i, derivedClasses[i]->GetName().AsString().AsChar(), wxEmptyString, false );
			popup.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCascadeArrayEditor::OnArrayElementAddDerivedClass, this, i, -1, new PopupClassWrapper( derivedClasses[i] ) );
			//popup.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdCascadeArrayEditor::OnArrayElementAddDerivedClass ), new PopupClassWrapper( derivedClasses[i] ), this ); 
		}

		m_arrayAddButton->PopupMenu( &popup );
	}

}

void CEdCascadeArrayEditor::OnArrayElementAddDerivedClass( wxCommandEvent& event )
{
	PopupClassWrapper* wrapper = ( PopupClassWrapper* ) event.m_callbackUserData;
	CClass* objectClass = wrapper->m_objectClass;

	AddArrayElement( objectClass );
}

void CEdCascadeArrayEditor::OnArrayElementDelete( wxCommandEvent& event )
{
	m_elementEditor->SetNoObject();
	Uint32 elementIndex = (Uint32) m_arrayElementList->GetSelection();
	m_arrayType->DeleteArrayElement( m_arrayObject, elementIndex );
	m_arrayElementList->Delete( elementIndex );

	if ( m_parentObject != NULL )
	{
		m_parentObject->MarkModified();
	}
}

void CEdCascadeArrayEditor::OnArrayClear( wxCommandEvent& event )
{
	m_elementEditor->SetNoObject();
	m_arrayType->Clean( m_arrayObject );
	m_arrayElementList->Clear();

	if ( m_parentObject != NULL )
	{
		m_parentObject->MarkModified();
	}
}
