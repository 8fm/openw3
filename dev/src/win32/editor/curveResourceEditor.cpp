///////////////////////////////////////////////////////
// headers
#include "build.h"
#include "curveResourceEditor.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curve.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/staticMeshComponent.h"

BEGIN_EVENT_TABLE( CSelectPropertyEditorDialog, wxDialog )
	EVT_BUTTON( XRCID("OK"), CSelectPropertyEditorDialog::OnOk )
	EVT_BUTTON( XRCID("Cancel"), CSelectPropertyEditorDialog::OnCancel )
	EVT_TREE_ITEM_ACTIVATED( XRCID("groupTree"), CSelectPropertyEditorDialog::OnTreeItemActivated )
END_EVENT_TABLE()

CSelectPropertyEditorDialog::CSelectPropertyEditorDialog( wxWindow* parent, TDynArray<String>& properties, const String& selectedProp )
	: m_treelist()
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, NULL, TXT( "LayerGroupPicker" ) );
	wxDialog::SetLabel( TXT("Select available properties from list") );
	SetSize( 400, 600 );

	// get tree list
	m_treelist = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	ASSERT(m_treelist);
	if ( !m_treelist )
	{
		return;
	}

	// hide null button - we don't need them
	wxButton* nullButton = XRCCTRL( *this, "NULL", wxButton );
	if ( nullButton )
	{
		nullButton->Hide();
	}

	// setup props list
	m_treelist->SetWindowStyle( wxTR_HIDE_ROOT|wxTR_SINGLE );
	m_treelist->DeleteAllItems();

	// fill properties list
	wxTreeItemId root = m_treelist->AddRoot( TXT( "root" ) );
	wxTreeItemId baseObject = m_treelist->AppendItem( root, TXT( "Base Object" ) );
	const Uint32 propsCount = properties.Size();
	wxTreeItemId currentProp;
	String parentObject;
	Bool selected = false;
	for ( Uint32 no = 0; no < propsCount; ++no )
	{
		// base object props
		if ( !properties[no].ContainsCharacter( '/' ) )
		{
			currentProp = m_treelist->AppendItem( baseObject, properties[no].AsChar() );
		}

		// components props
		else
		{
			size_t index;
			if ( properties[no].FindCharacter( '/', index ) )
			{
				// get current component name
				String currentParentObject = properties[no].LeftString( index );

				// we found next component
				if ( currentParentObject != parentObject )
				{
					baseObject = m_treelist->AppendItem( root, currentParentObject.AsChar() );
					parentObject = currentParentObject;
				}

				// add prop to component
				currentProp = m_treelist->AppendItem( baseObject, properties[no].MidString( index+1 ).AsChar() );
			}
		}

		// select current property on tree view
		if ( !selected && properties[no].ContainsSubstring( selectedProp ) && currentProp.IsOk() )
		{
			m_treelist->SelectItem( currentProp );
			m_treelist->ScrollTo( currentProp );
			selected = true;
		}
	}

	// show
	Layout();
	Show();
}
//////////////////////////////////////////////////////////////////////////
//
// dtor
CSelectPropertyEditorDialog::~CSelectPropertyEditorDialog()
{
}
//////////////////////////////////////////////////////////////////////////
//
// close dialog
const String& CSelectPropertyEditorDialog::GetSelectedPropertyName()
{
	return m_selectedPropertyName;
}
//////////////////////////////////////////////////////////////////////////
//
// close dialog - ok
void CSelectPropertyEditorDialog::OnOk( wxCommandEvent& event )
{
	EndDialog(1);
}
//////////////////////////////////////////////////////////////////////////
//
// close dialog - cancel
void CSelectPropertyEditorDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog(0);
}
//////////////////////////////////////////////////////////////////////////
//
// tree item activated
void CSelectPropertyEditorDialog::OnTreeItemActivated( wxTreeEvent& event )
{
	// clear selected property
	m_selectedPropertyName.ClearFast();

	// Properties have [<type>] suffix
	String propName = m_treelist->GetItemText( event.GetItem() );
	size_t index;
	if ( propName.FindCharacter( '[', index ) )
	{
		// check does selected item has parent node
		wxTreeItemId parentId = m_treelist->GetItemParent( event.GetItem() );
		if ( parentId.IsOk() && m_treelist->GetItemText( parentId ) != TXT( "Base Object" ) )
		{
			// store parent node as component name
			m_selectedPropertyName = m_treelist->GetItemText( parentId ) + TXT( "/" );
		}

		// store item name
		m_selectedPropertyName += propName.LeftString( index-1 );
	}

	EndDialog(1);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// ctor
CAnimatedPropertyEditor::CAnimatedPropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlText()
{	
	ASSERT( m_propertyItem->GetPropertyType()->GetName() == TXT("CName") );
}

//////////////////////////////////////////////////////////////////////////

CAnimatedPropertyEditor::~CAnimatedPropertyEditor()
{
}

//////////////////////////////////////////////////////////////////////////
//
// add curve editor dialog on focus
void CAnimatedPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Calculate placement
	wxRect valueRect;
	valueRect.y = propertyRect.y + 3;
	valueRect.height = propertyRect.height - 3;
	valueRect.x = propertyRect.x + 2;
	valueRect.width = propertyRect.width - ( propertyRect.height + 1 ) * 1;

	// Create text editor
	m_ctrlText = new wxTextCtrlEx( m_propertyItem->GetPage(), wxID_ANY,	wxEmptyString, valueRect.GetTopLeft(), valueRect.GetSize(), wxNO_BORDER | wxTE_PROCESS_ENTER );
	m_ctrlText->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CAnimatedPropertyEditor::OnEditKeyDown ), NULL, this );
	m_ctrlText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CAnimatedPropertyEditor::OnEditTextEnter ), NULL, this );
	m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlText->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_ctrlText->SetSelection( -1, -1 );
	m_ctrlText->SetFocus();

	// Add select property editor button
	m_propertyItem->AddButton( SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_BROWSE") ), wxCommandEventHandler( CAnimatedPropertyEditor::OnSelectPropertyEditorDialog ), this );
}

//////////////////////////////////////////////////////////////////////////
//
// on name editor enter - store name
void CAnimatedPropertyEditor::OnEditTextEnter( wxCommandEvent& event )
{
	m_propertyItem->SavePropertyValue();
}
//////////////////////////////////////////////////////////////////////////
//
// on key down in name editor - copied from stringPropertyEditor
void CAnimatedPropertyEditor::OnEditKeyDown( wxKeyEvent& event )
{
	// Navigation
	if ( event.GetKeyCode() == WXK_UP )
	{
		PostMessage( (HWND) m_propertyItem->GetPage()->GetHandle(), WM_KEYDOWN, VK_UP, 0 );
		return;
	}
	else if ( event.GetKeyCode() == WXK_DOWN )
	{
		PostMessage( (HWND) m_propertyItem->GetPage()->GetHandle(), WM_KEYDOWN, VK_DOWN, 0 );
		return;
	}

	// Escape, restore original value
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		m_propertyItem->GrabPropertyValue();
		return;
	}

	// Allow text ctrl to process key
	event.Skip();
}
//////////////////////////////////////////////////////////////////////////
//
// get value
Bool CAnimatedPropertyEditor::GrabValue( String& displayValue )
{
	displayValue = String::EMPTY;

	// get name as string
	CName text;
	if ( m_propertyItem->Read( &text ) )
	{
		displayValue = text.AsString();
	}
	
	// set on control
	if ( m_ctrlText )
	{
		m_ctrlText->SetValue( displayValue.AsChar() );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// save value
Bool CAnimatedPropertyEditor::SaveValue()
{
	if ( m_ctrlText )
	{
		String text = m_ctrlText->GetValue().wc_str();

		CName tmpText( text );
		m_propertyItem->Write( &tmpText );

		m_propertyItem->GrabPropertyValue();
	}
	return true;
}

Bool CAnimatedPropertyEditor::IsModifiableProperty( CProperty* property )
{
	if ( property && property->IsEditable() )
	{
		const CName& propTypeName = property->GetType()->GetName();
		if ( propTypeName == CNAME( EngineTransform ) || propTypeName == CNAME( Vector ) || propTypeName == CNAME( Float ) || propTypeName == CNAME( EulerAngles ) )
		{
			return true;
		}
	}
	return false;
}

void CAnimatedPropertyEditor::OnSelectPropertyEditorDialog( wxCommandEvent& event )
{
	// get base object
	CGameplayEntity* parentObject = m_propertyItem->GetRootObject( 0 ).As< CGameplayEntity >();
	ASSERT( parentObject );
	if ( !parentObject )
	{
		return;
	}

	CPropertyAnimationSet* animationSet = parentObject->GetPropertyAnimationSet();

	// prepare list of all properties
	TDynArray<CProperty*> properties;

	// get all props from parent class
	animationSet->GetParent()->GetClass()->GetProperties( properties );

	// get all properties possible to animated
	TDynArray<String> propertyList;
	const Uint32 propCount = properties.Size();
	for ( Uint32 no = 0; no < propCount; ++no )
	{
		CProperty* prop = properties[no];
		if ( IsModifiableProperty( prop )  )
		{
			propertyList.PushBack( prop->GetName().AsString() + TXT(" [") + prop->GetType()->GetName().AsString() + TXT("]") );
		}
	}

	Bool hasStaticComponent = false;

	// attach properties from components
	if ( animationSet->GetParent()->IsA<CEntity>() )
	{
		// get all components
		const TDynArray<CComponent*> components = static_cast<CEntity*>( animationSet->GetParent() )->GetComponents();

		// get all props from components
		const Uint32 compCount = components.Size();
		for ( Uint32 no = 0; no < compCount; ++no )
		{
			CComponent* component = components[no];

			hasStaticComponent |= component->IsExactlyA<CStaticMeshComponent>();

			// get component props
			properties.ClearFast();
			component->GetClass()->GetProperties( properties );

			// add it to list when are modifiable
			const Uint32 propCount = properties.Size();
			for ( Uint32 nr = 0; nr < propCount; ++nr )
			{
				CProperty* prop = properties[nr];
				if ( IsModifiableProperty( prop )  )
				{
					propertyList.PushBack( component->GetName() + TXT("/") + prop->GetName().AsString() + TXT(" [") + prop->GetType()->GetName().AsString() + TXT("]") );
				}
			}
		}
	}

	// get current property name
	CName text;
	m_propertyItem->Read( &text );

	// show editor when property is assigned to curve
	CSelectPropertyEditorDialog* dlg = new CSelectPropertyEditorDialog( m_propertyItem->GetPage(), propertyList, text.AsString() );
	if ( dlg->ShowModal() )
	{
		// if property is selected then write to property cell
		if ( dlg->GetSelectedPropertyName().Size() )
		{
			const Bool isTransform = dlg->GetSelectedPropertyName().ContainsSubstring( TXT("transform") );
			Bool isInStaticLayer = false;
			if ( CLayer* layer = parentObject->GetLayer() )
			{
				if ( CLayerInfo* layerInfo = layer->GetLayerInfo() )
				{
					isInStaticLayer = layerInfo->GetLayerType() != LT_NonStatic;
				}
			}

			if ( isTransform && hasStaticComponent )
			{
				wxMessageBox( TXT("Cannot animate transform properties within entities with static components. Remove or change all static components (e.g. use CRigidMeshComponent instead of CStaticMeshComponent)."), TXT("Warning") );
			}
			else if ( isTransform && isInStaticLayer )
			{
				wxMessageBox( TXT("Cannot animate transform properties within entities placed in other than NonStatic layer. Change layer type and try again."), TXT("Warning") );
			}
			else
			{
				CName text( dlg->GetSelectedPropertyName() );
				m_propertyItem->Write( &text );
			}
		}

		m_propertyItem->GrabPropertyValue();
	}
}
//////////////////////////////////////////////////////////////////////////
//
// on closed editors
void CAnimatedPropertyEditor::CloseControls()
{
	if ( m_ctrlText )
	{
		delete m_ctrlText;
		m_ctrlText = NULL;
	}
}


//////////////////////////////////////////////////////////////////////////
// EOF