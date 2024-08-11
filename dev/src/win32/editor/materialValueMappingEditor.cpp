/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "materialValueMappingEditor.h"
#include "materialPreviewPanel.h"
#include "../../common/engine/material.h"


wxDEFINE_EVENT( wxEVT_COMMAND_MATERIALVALUEMAPPING_CHANGED, wxCommandEvent );


CEdMaterialValueMappingEditor::CEdMaterialValueMappingEditor( wxWindow* parent, IEdMaterialValueMappingMaterialsInterface* materials, IEdMaterialValueMappingValuesInterface* values, const String& valueHint, Bool inlined )
	: m_materialsInterface( materials )
	, m_valuesInterface( values )
	, m_rootParent( nullptr )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "MaterialValueMapping" ) );
	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_materialPreview = new CEdMaterialPreviewPanel( rp );
		sizer1->Add( m_materialPreview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Material properties for selected material. Read only, just for artist's reference.
	{
		wxPanel* rp = XRCCTRL( *this, "MaterialProperties", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		settings.m_readOnly = true;
		m_materialProperties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		sizer1->Add( m_materialProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create texture selection. It's a single properties page, with a custom RTTI class containing a property
	// for each material.
	{
		wxPanel* rp = XRCCTRL( *this, "TextureSelect", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_valueEditProperties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_valueEditProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdMaterialValueMappingEditor::OnValueChanged ), nullptr, this );
		m_valueEditProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_SELECTED, wxCommandEventHandler( CEdMaterialValueMappingEditor::OnMaterialSelected ), nullptr, this );
		sizer1->Add( m_valueEditProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdMaterialValueMappingEditor::OnClose ), nullptr, this );
	Connect( wxEVT_SHOW, wxShowEventHandler( CEdMaterialValueMappingEditor::OnShow ), nullptr, this );

	m_numMaterials = m_materialsInterface->GetNumMaterials();
	IRTTIType* valueType = m_valuesInterface->GetValueType();
	Uint32 valueSize = valueType->GetSize();

	// CAbstractClass works fine. Don't need it to be able to instantiate anything, and it has everything already implemented.
	m_mappingClass = new CAbstractClass( CName(TXT("MaterialMapping")), m_numMaterials * valueSize, 0 );

	Uint32 propertyFlags = PF_Editable;
	if ( inlined )
	{
		propertyFlags |= PF_Inlined;
	}

	for ( Uint32 i = 0; i < m_numMaterials; ++i )
	{
		CName name( m_materialsInterface->GetMaterialName( i ) );
		m_mappingClass->AddProperty( new CProperty( valueType, m_mappingClass, i * valueSize, name, valueHint, propertyFlags ) );
	}

	m_values = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, m_numMaterials * valueSize );

	SetSize( 700, 500 );
	UpdatePosition( parent );

	GetParent()->Layout();
	Layout();
}


CEdMaterialValueMappingEditor::~CEdMaterialValueMappingEditor()
{
	if ( m_values )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_values );
		m_values = nullptr;
	}
}


void CEdMaterialValueMappingEditor::Initialize()
{
	IRTTIType* valueType = m_valuesInterface->GetValueType();
	Uint32 valueSize = valueType->GetSize();

	Uint8* valuePtr = static_cast< Uint8* >( m_values );

	// Copy current values from value interface.
	for ( Uint32 i = 0; i < m_valuesInterface->GetNumValues(); ++i )
	{
		m_valuesInterface->GetValue( i, valuePtr + i * valueSize );
	}

	// m_mappingClass is created so that we can just use the array data directly as the object.
	STypedObject typedObject;
	typedObject.m_class = m_mappingClass;
	typedObject.m_object = m_values;
	m_valueEditProperties->Get().SetTypedObject( typedObject );
}

void CEdMaterialValueMappingEditor::Execute()
{	
	// 1. changed the smartLaoutFrame to Dialog, as all custom editors should be modal
	// 2. cannot use ShowModal as materialValueMappingEditor contains material preview, which needs the events loop to be running 
	//    (showing modal dialog with its own loop here will block the main loop as we'd get stuck in the event handler till editor closing)

	Initialize();
	Show();
}


Int32 CEdMaterialValueMappingEditor::FindMaterialIndex( const String& name ) const
{
	for ( Uint32 i = 0; i < m_numMaterials; ++i )
	{
		if ( m_materialsInterface->GetMaterialName( i ) == name )
		{
			return i;
		}
	}
	return -1;
}

void CEdMaterialValueMappingEditor::UpdatePosition( wxWindow* parent )
{
	wxPoint end = GetScreenPosition() + GetSize();
	wxPoint parentEnd = parent->GetScreenPosition() + parent->GetSize();
	wxPoint diff = end - parentEnd;
	wxPoint newPosition = GetScreenPosition();
	newPosition.x = Max( 0, newPosition.x - Max( 0, diff.x ) - 50 );
	newPosition.y = Max( 0, newPosition.y - Max( 0, diff.y ) - 50 );
	SetPosition( newPosition );
}

void CEdMaterialValueMappingEditor::OnMaterialSelected( wxCommandEvent& event )
{
	// Find which material is selected. Can't just get an index, so we grab the property name (which
	// was created as the material name) and look for the material that way.
	CBasePropItem* item = m_valueEditProperties->Get().GetActiveItem();
	String propName = item->GetName();

	Int32 foundIndex = FindMaterialIndex( propName );
	IMaterial* selectedMaterial = nullptr;

	RED_ASSERT( foundIndex != -1, TXT("Couldn't find selected material: %s"), propName );
	if ( foundIndex == -1 )
	{
		m_materialProperties->Get().SetNoObject();
		m_materialPreview->SetMaterial( nullptr );
	}
	else
	{
		// Show this material in the preview and material properties.
		selectedMaterial = m_materialsInterface->GetMaterial( foundIndex );
		m_materialProperties->Get().SetObject( selectedMaterial );
		m_materialPreview->SetMaterial( selectedMaterial );
	}
	m_materialPreview->RefreshPreviewVisibility( selectedMaterial != nullptr );
}


void CEdMaterialValueMappingEditor::OnValueChanged( wxCommandEvent& event )
{
	if ( m_valuesInterface->GetNumValues() != m_numMaterials )
	{
		m_valuesInterface->SetNumValues( m_numMaterials );
	}

	CBasePropItem* item = m_valueEditProperties->Get().GetActiveItem();
	String propName = item->GetName();
	Int32 foundIndex = FindMaterialIndex( propName );

	RED_ASSERT( foundIndex != -1, TXT("Couldn't find selected material: %s"), propName );
	if ( foundIndex == -1 )
	{
		return;
	}

	Uint8* valuePtr = static_cast< Uint8* >( m_values );
	m_valuesInterface->SetValue( foundIndex, valuePtr + foundIndex * m_valuesInterface->GetValueType()->GetSize() );

	wxCommandEvent newEvent( wxEVT_COMMAND_MATERIALVALUEMAPPING_CHANGED );
	newEvent.SetEventObject( this );

	// Since we're already in an event, process immediately
	GetEventHandler()->ProcessEvent( newEvent );
}

void CEdMaterialValueMappingEditor::OnClose( wxCloseEvent& event )
{
	m_valueEditProperties->Get().SetNoObject();
	m_mappingClass->ClearProperties();

	delete m_mappingClass;
	m_mappingClass = nullptr;

	wxWindow* parent = GetParent();
	while ( parent->GetParent() )
	{
		parent = parent->GetParent();
	}

	if ( m_rootParent )
	{
		m_rootParent->Enable();
	}

	event.Skip();
}

void CEdMaterialValueMappingEditor::OnShow( wxShowEvent& event )
{
	if ( event.IsShown() )
	{
		m_rootParent = GetParent();
		while ( m_rootParent->GetParent() )
		{
			m_rootParent = m_rootParent->GetParent();
		}

		if ( m_rootParent )
		{
			m_rootParent->Disable();
		}
	}
	event.Skip();
}
