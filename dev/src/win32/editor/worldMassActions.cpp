
#include "build.h"
#include "worldMassActions.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/core/scriptSnapshot.h"
#include "sceneExplorer.h"

CEdMassActionChangeComponentClass::Action::Action( IEdMassActionType* type, CEdMassActionContext* context )
	: IEdMassAction( context )
	, m_classHierarchyInitialized( false )
	, m_panel( nullptr )
	, m_classesChoice( nullptr )
	, m_typesChoice( nullptr )
{
	m_type = type;
}

void CEdMassActionChangeComponentClass::Action::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	m_panel->GetSizer()->Add( new wxStaticText( m_panel, wxID_ANY, wxT("from") ), 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_typesChoice = new wxChoice( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT );
	FillComponentsChoice();
	m_typesChoice->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &Action::OnComponentsChoiceChanged, this );
	m_panel->GetSizer()->Add( m_typesChoice, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );

	m_panel->GetSizer()->Add( new wxStaticText( m_panel, wxID_ANY, wxT("to") ), 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_classesChoice = new wxChoice( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT );
	FillClassesChoice();
	m_panel->GetSizer()->Add( m_classesChoice, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );

	m_panel->Layout();
}

void CEdMassActionChangeComponentClass::Action::OnComponentsChoiceChanged( wxCommandEvent& event )
{
	FillClassesChoice();
}

void CEdMassActionChangeComponentClass::Action::FillComponentsChoice()
{
	if ( !m_typesChoice )
	{
		return;
	}

	m_context->RewindIterator();

	m_typesChoice->Freeze();
	m_typesChoice->Clear();
	m_oldClasses.Clear();

	while ( m_context->HasMoreObjects() )
	{
		CClass* cls = m_context->GetNextObject()->GetClass();
		if ( !cls ) continue;

		if ( cls->IsBasedOn( ClassID< CMeshComponent >() ) && m_oldClasses.Insert( cls->GetName().AsString(), cls ) )
		{
			m_typesChoice->Append( cls->GetName().AsString().AsChar() );
		}
	}
	m_typesChoice->Thaw();
}

void CEdMassActionChangeComponentClass::Action::FillClassesChoice()
{
	if ( !m_classesChoice )
	{
		return;
	}

	if ( !m_classHierarchyInitialized )
	{
		CClassHierarchyMapper::MapHierarchy( CMeshComponent::GetStaticClass(), m_componentClassHierarchy, CClassHierarchyMapper::CClassNaming(), true );
		m_classHierarchyInitialized = true;
	}

	m_classesChoice->Freeze();
	m_classesChoice->Clear();
	m_classes.Clear();

	for ( Uint32 i = 0; i < m_componentClassHierarchy.Size(); ++i )
	{
		CClass* currClass = m_componentClassHierarchy[ i ].m_class;
		if ( currClass->GetName().AsString() != m_typesChoice->GetStringSelection().c_str() && !currClass->IsAbstract() )
		{
			if ( m_classes.Insert( currClass->GetName().AsString(), currClass ) )
			{
				m_classesChoice->Append( currClass->GetName().AsChar() );
			}
		}
	}
	m_classesChoice->Thaw();
}

void CEdMassActionChangeComponentClass::Action::ExportToXML( wxXmlNode* node )
{
	node->AddAttribute( wxT("from"), m_typesChoice->GetStringSelection() );
	node->AddAttribute( wxT("to"), m_classesChoice->GetStringSelection() );
}

void CEdMassActionChangeComponentClass::Action::ImportFromXML( wxXmlNode* node )
{
	Int32 ind = m_typesChoice->FindString( node->GetAttribute( wxT("from") ) );
	if ( ind == -1 )
	{
		wxMessageBox( wxString::Format( wxT("Type not found in scene explorer: %s"), node->GetAttribute( wxT("from") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
		ind = 0;
	}
	m_typesChoice->SetSelection( ind );

	ind = m_classesChoice->FindString( node->GetAttribute( wxT("to") ) );
	if ( ind == -1 )
	{
		wxMessageBox( wxString::Format( wxT("Unknown class: %s"), node->GetAttribute( wxT("to") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
		ind = 0;
	}
	m_classesChoice->SetSelection( ind );
}

wxWindow* CEdMassActionChangeComponentClass::Action::GetWindow( wxWindow* parent )
{
	if ( !m_panel )
	{
		CreatePanel( parent );
	}
	return m_panel;
}

wxString CEdMassActionChangeComponentClass::Action::GetDescription() const
{
	return wxString::Format( wxT("Change from '%s' to '%s' for '%s'"), m_typesChoice->GetStringSelection(), m_classesChoice->GetStringSelection(), m_lastProcessed.AsChar() );
}

bool CEdMassActionChangeComponentClass::Action::Perform( IEdMassActionObject* object )
{
	String newClassName = m_classesChoice->GetStringSelection().wc_str();
	String oldClassName = m_typesChoice->GetStringSelection().wc_str();
	CClass* newClass = m_classes.FindPtr( newClassName ) ? *( m_classes.FindPtr( newClassName ) ) : nullptr;
	CClass* oldClass = m_oldClasses.FindPtr( oldClassName ) ? *( m_oldClasses.FindPtr( oldClassName ) ) : nullptr;

	// Check if the object is a mesh type component
	if ( newClass == nullptr || oldClass == nullptr || ( object->GetClass() != oldClass && !object->GetClass()->IsA< CEntity >() ) )
	{
		m_lastProcessed = object->GetClass()->GetName().AsString();
		return false;
	}

	CEdMassActionRTTIProxy* proxy = static_cast< CEdMassActionRTTIProxy* >( object );
	CMeshComponent* component = nullptr;
	CEntity* entity = nullptr;

	if ( object->GetClass()->IsA< CMeshComponent >() )
	{
		component = static_cast< CMeshComponent* >( proxy->GetObject() );
		entity = component->GetEntity();
	}
	else 
	{
		entity = static_cast< CEntity* >( proxy->GetObject() );
		for ( Uint32 i = 0; i < entity->GetComponents().Size(); ++i )
		{
			CComponent* currCmp = entity->GetComponents()[i];
			if ( currCmp->GetClass() == oldClass )
			{
				component = Cast< CMeshComponent >( currCmp );
				break;
			}
		}
	}

	m_lastProcessed = entity->GetLayer()->GetDepotPath() + entity->GetName();
	if ( !entity || !component || entity->GetEntityTemplate() || !entity->MarkModified() )
	{
		return false;
	}

	// Snapshot properties
	CScriptSnapshot snapshot;
	CScriptSnapshot::ScriptableSnapshot* snap = snapshot.BuildEditorObjectSnapshot( component );
	snap->m_properties.Erase( RemoveIf( snap->m_properties.Begin(), snap->m_properties.End(), []( CScriptSnapshot::PropertySnapshot* prop ) { return prop->m_name == TXT("name"); } ) );

	SComponentSpawnInfo dummySpawnInfo;
	dummySpawnInfo.m_spawnPosition = component->GetPosition();
	dummySpawnInfo.m_spawnRotation = component->GetRotation();
	dummySpawnInfo.m_spawnScale = component->GetScale();

	// Create new component object and restore properties
	CMeshComponent* newComponent = Cast< CMeshComponent >( entity->CreateComponent( newClass, dummySpawnInfo ) );
	newComponent->SetAsCloneOf( component );
	snapshot.RestoreEditorObjectSnapshot( newComponent, snap );

	component->EditorPreDeletion();
	entity->RemoveComponent( component );
	component->Destroy();
	delete snap;

	return true;
}

void CEdMassActionChangeComponentClass::Action::OnActionSelect( IEdMassAction* previousSelection )
{
	FillComponentsChoice();
	FillClassesChoice();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CEdMassActionSelectNode::Action::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	wxStaticText* text = new wxStaticText( m_panel, wxID_ANY, wxT("the") );
	m_panel->GetSizer()->Add( text, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );

	m_selection = new wxChoice( m_panel, wxID_ANY );
	m_selection->Append( "node" );
	m_selection->Append( "entity" );
	m_selection->Append( "layer" );
	m_selection->SetSelection( m_previousSelection );
	m_panel->GetSizer()->Add( m_selection, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_panel->Layout();
}

void CEdMassActionSelectNode::Action::OnActionSelect( IEdMassAction* previousSelection )
{
	Action* prevSel;
	if ( ( prevSel = dynamic_cast<Action*>(previousSelection) ) )
	{
		m_previousSelection = prevSel->m_selection->GetSelection();
	}
}

CEdMassActionSelectNode::Action::Action( IEdMassActionType* type, CEdMassActionContext* context, THashSet< CNode* >& nodes )
	: IEdMassAction( context )
	, m_panel( nullptr )
	, m_selection( nullptr )
	, m_previousSelection( 0 )
	, m_nodes( nodes )
{
	m_type = type;
}

void CEdMassActionSelectNode::Action::ExportToXML( wxXmlNode* node )
{
	node->AddAttribute( wxT("selection"), m_selection->GetStringSelection() );
}

void CEdMassActionSelectNode::Action::ImportFromXML( wxXmlNode* node )
{
	int index;

	index = m_selection->FindString( node->GetAttribute( wxT("selection") ) );
	if ( index == -1 )
	{
		wxMessageBox( wxString::Format( wxT("Unknown selection: %s"), node->GetAttribute( wxT("selection") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
		index = 0;
	}
	m_selection->SetSelection( index );
}

wxWindow* CEdMassActionSelectNode::Action::GetWindow( wxWindow* parent )
{
	if ( m_panel == nullptr )
	{
		CreatePanel( parent );
	}

	return m_panel;
}

wxString CEdMassActionSelectNode::Action::GetDescription() const
{
	switch ( m_selection->GetSelection() )
	{
		case 0: return wxT("Select the node");
		case 1: return wxT("Select the entity");
		case 2: return wxT("Select the layer");
		default: return wxEmptyString;
	}
}

bool CEdMassActionSelectNode::Action::Perform( IEdMassActionObject* object )
{
	CEdMassActionRTTIProxy* proxy = static_cast< CEdMassActionRTTIProxy* >( object );
	CNode* node = static_cast< CNode* >( proxy->GetObject() );

	switch ( m_selection->GetSelection() )
	{
	case 0:
		m_nodes.Insert( node );
		break;
	case 1:
		if ( node->IsA< CComponent >() )
		{
			m_nodes.Insert( static_cast< CComponent* >( node )->GetEntity() );
		}
		else
		{
			m_nodes.Insert( node  );
		}
		break;
	case 2:
		node->GetLayer()->GetWorld()->GetSelectionManager()->SelectLayer( node->GetLayer() );
		break;
	default:
		return false;
	}

	return true;
}

void CEdMassActionShowHideEntity::Action::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	m_panel->Layout();
}

CEdMassActionShowHideEntity::Action::Action( IEdMassActionType* type, CEdMassActionContext* context, Bool hide, THashSet< CEntity* >& entities )
	: IEdMassAction( context )
	, m_panel( nullptr )
	, m_hide( hide )
	, m_entities( entities )
{
	m_type = type;
}

wxWindow* CEdMassActionShowHideEntity::Action::GetWindow( wxWindow* parent )
{
	if ( m_panel == nullptr )
	{
		CreatePanel( parent );
	}

	return m_panel;
}

wxString CEdMassActionShowHideEntity::Action::GetDescription() const
{
	return m_hide ? wxT("Hide the entity") : wxT("Reveal the entity");
}

bool CEdMassActionShowHideEntity::Action::Perform( IEdMassActionObject* object )
{
	CEdMassActionRTTIProxy* proxy = static_cast< CEdMassActionRTTIProxy* >( object );
	CNode* node = static_cast< CNode* >( proxy->GetObject() );

	if ( node->IsA< CEntity >() )
	{
		m_entities.Insert( static_cast< CEntity* >( node ) );
	}

	return true;
}

void CEdMassActionNodeSpecific::Action::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	m_text = new wxTextCtrl( m_panel, wxID_ANY );
	m_panel->GetSizer()->Add( m_text, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_panel->Layout();
}

wxWindow* CEdMassActionNodeSpecific::Action::GetWindow( wxWindow* parent )
{
	if ( !m_panel )
	{
		CreatePanel( parent );
	}
	return m_panel;
}

wxString CEdMassActionNodeSpecific::Action::GetDescription() const
{
	return wxString::Format( wxT("Perform action '%s'"), m_text->GetValue() );
}

bool CEdMassActionNodeSpecific::Action::Perform( IEdMassActionObject* object )
{
	// Check if the object is an entity
	if ( !object->GetClass()->IsA< CNode >() )
	{
		return false;
	}

	return static_cast< CNode* >( static_cast< CEdMassActionRTTIProxy* >( object )->GetObject() )->PerformMassAction( m_text->GetValue().wc_str() );
}

wxString CEdMassActionAddToSceneFilter::Action::GetDescription() const
{
	return wxT("Add the node to the scene tree filter");
}

bool CEdMassActionAddToSceneFilter::Action::Perform( IEdMassActionObject* object )
{
	CEdMassActionRTTIProxy* proxy = static_cast< CEdMassActionRTTIProxy* >( object );
	if ( wxTheFrame && wxTheFrame->GetSceneExplorer() )
	{
		wxTheFrame->GetSceneExplorer()->AddToGlobalFilter( static_cast<ISerializable*>( proxy->GetObject() ) );
	}
	return true;
}

void CEdMassActionEntityListModifierBase::Action::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	wxStaticText* text = new wxStaticText( m_panel, wxID_ANY, wxT("named") );
	m_panel->GetSizer()->Add( text, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );

	m_listName = new wxTextCtrl( m_panel, wxID_ANY );
	m_panel->GetSizer()->Add( m_listName, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_panel->Layout();
}

void CEdMassActionEntityListModifierBase::Action::ExportToXML( wxXmlNode* node )
{
	node->AddAttribute( wxT("listname"), m_listName->GetValue() );
}

void CEdMassActionEntityListModifierBase::Action::ImportFromXML( wxXmlNode* node )
{
	m_listName->SetValue( node->GetAttribute( wxT("listname") ) );
}

wxWindow* CEdMassActionEntityListModifierBase::Action::GetWindow( wxWindow* parent )
{
	if ( m_panel == nullptr )
	{
		CreatePanel( parent );
	}

	return m_panel;
}

wxString CEdMassActionEntityListModifierBase::Action::GetDescription() const
{
	return static_cast< CEdMassActionEntityListModifierBase* >( m_type )->GetDescription( m_listName->GetValue() );
}

bool CEdMassActionEntityListModifierBase::Action::Perform( IEdMassActionObject* object )
{
	CEdMassActionRTTIProxy* proxy = static_cast< CEdMassActionRTTIProxy* >( object );
	CNode* node = static_cast< CNode* >( proxy->GetObject() );

	CEntity* entity = nullptr;
	if ( node->IsA< CComponent >() )
	{
		entity = static_cast< CComponent* >( node )->GetEntity();
	}
	else
	{
		entity = static_cast< CEntity* >( node );
	}

	return static_cast< CEdMassActionEntityListModifierBase* >( m_type )->PerformAction( m_listName->GetValue(), entity );
}

Bool CEdMassActionAddEntityToList::PerformAction( const wxString& listName, CEntity* entity )
{
	AddEntityToEntityList( listName.wc_str(), entity );
	return true;
}

wxString CEdMassActionAddEntityToList::GetDescription( const wxString& listName )
{
	return wxString::Format( wxT("Add the entity to the entity list named '%s'"), listName );
}

Bool CEdMassActionRemoveEntityFromList::PerformAction( const wxString& listName, CEntity* entity )
{
	RemoveEntityFromEntityList( listName.wc_str(), entity );
	return true;
}

wxString CEdMassActionRemoveEntityFromList::GetDescription( const wxString& listName )
{
	return wxString::Format( wxT("Remove the entity from the entity list named '%s'"), listName );
}

Bool CEdMassActionClearEntityList::PerformAction( const wxString& listName, CEntity* entity )
{
	ClearEntityList( listName.wc_str() );
	return true;
}

wxString CEdMassActionClearEntityList::GetDescription( const wxString& listName )
{
	return wxString::Format( wxT("Clear the entity list named '%s'"), listName );
}
