#include "build.h"

#include "../../common/game/behTreeNode.h"
#include "../../common/game/edSpawnTreeNode.h"
#include "../../common/game/spawnTreeNode.h"

#include "behTreePropertyEditor.h"
#include "behTreeEditor.h"


////////////////////////////////////////////////////////////////////////////////
// CBehTreePropertyEditor
////////////////////////////////////////////////////////////////////////////////
COpenTreePropertyEditor::COpenTreePropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
	m_iconEdit = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_VEGETATION") );
}
COpenTreePropertyEditor::~COpenTreePropertyEditor()
{
}
Bool COpenTreePropertyEditor::IsEditing()
{
	IRTTIType* propType = m_propertyItem->GetPropertyType();
	if ( !propType->IsPointerType() )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non-pointer type property!" ) );
		return false;
	}

	CBasePropItem* propBase = m_propertyItem->GetParent();
	CObject* object = propBase->GetParentObject( 0 ).AsObject();
	if ( !object )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non CObject owned property!" ) );
		return false;
	}

	CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( propType );
	IRTTIType* pointedType = pointerType->GetPointedType();
	if ( pointedType->GetType() != RT_Class )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non instance pointer property!" ) );
		return false;
	}

	CClass* pointedClass = static_cast< CClass* >( pointedType );

	if ( pointedClass->IsA< ISpawnTreeBaseNode > () )
	{
		return wxTheFrame->GetWorldEditPanel()->IsSpawnTreeBeingEdited( object );
	}
	else if ( pointedClass->IsA< IBehTreeNodeDefinition > () )
	{
		return false;
	}
	return false;
}
void COpenTreePropertyEditor::OnEditRequest( wxCommandEvent &event )
{
	IRTTIType* propType = m_propertyItem->GetPropertyType();
	if ( !propType->IsPointerType() )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non-pointer type property!" ) );
		return;
	}

	CBasePropItem* propBase = m_propertyItem->GetParent();
	CObject* object = propBase->GetParentObject( 0 ).AsObject();
	if ( !object )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non CObject owned property!" ) );
		return;
	}

	CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( propType );
	IRTTIType* pointedType = pointerType->GetPointedType();
	if ( pointedType->GetType() != RT_Class )
	{
		ASSERT( false, TXT( "TreeEditorButton used for non instance pointer property!" ) );
		return;
	}

	CClass* pointedClass = static_cast< CClass* >( pointedType );

	if ( pointedClass->IsA< ISpawnTreeBaseNode > () )
	{
		wxTheFrame->GetWorldEditPanel()->OpenSpawnTreeEditor( object );
	}
	else if ( pointedClass->IsA< IBehTreeNodeDefinition > () )
	{
		CBehTreeEditedProperty* editedItem = new CBehTreeEditedProperty( object, m_propertyItem->GetProperty() );
		CEdBehTreeEditor* editor = new CEdBehTreeEditor( NULL, editedItem );
		editor->Show();
		editor->SetFocus();
		editor->Raise();
	}

	//if ( m_propertyItem->GetNumObjects() != 1 )
	// return;
	//// get root "c"-object
	//void* rootObjectPtr = m_propertyItem->GetRootObject( 0 );
	//IRTTIType* rootObjectType = m_propertyItem->GetRootObjectType( 0 );
	//if ( rootObjectType->GetType() != RT_Class )
	// return;
	//CObject *root = Cast< CObject >( static_cast< CClass *>( rootObjectType ), rootObjectPtr );

	//// get parent object
	//void* parentObject = m_propertyItem->GetParent()->GetParentObject( 0 );
	//IRTTIType* parentObjectType = m_propertyItem->GetParent()->GetParentObjectType( 0 );

	//CBehTreeEditedProperty* propertyDescription = new CBehTreeEditedProperty( root, this );
	//m_editor = new CEdBehTreeEditor( NULL, propertyDescription );
	//m_isEdited = true;

	//m_propertyItem->GrabPropertyValue();

	//// Show the editor
	//m_editor->Show();
	//m_editor->SetFocus();
	//m_editor->Raise();
}
//! Property item got selected
void COpenTreePropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconEdit, wxCommandEventHandler( COpenTreePropertyEditor::OnEditRequest ), this );
}

Bool COpenTreePropertyEditor::GrabValue( String& displayValue )
{
	if ( IsEditing() )
	{
		displayValue = TXT("Editing...");
		return true;
	}
	
	CObject* objectPtr;
	if ( m_propertyItem->Read( &objectPtr ) )
	{
		if ( objectPtr )
		{
			if ( objectPtr->IsA< ISpawnTreeBaseNode >() )
			{
				displayValue = TXT("Spawn tree");
			}
			else if ( objectPtr->IsA< IBehTreeNodeDefinition >() )
			{
				displayValue = TXT("AI tree");
			}
			else
			{
				displayValue = objectPtr->GetFriendlyName();
			}
		}
		else
		{
			displayValue = TXT("Empty node");
		}
		return true;
	}

	return false;
}

