/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityGraphEditor.h"

#include "entityEditor.h"
#include "undoManager.h"
#include "undoCreate.h"

#include "../../common/game/actor.h"
#include "../../common/game/itemEntity.h"
#include "../../common/game/storyScene.h"

#include "../../common/game/storySceneAbstractLine.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/bgNpcItem.h"
#include "../../common/engine/pathlibNavmesh.h"
#include "../../common/engine/effectDummyPoint.h"
#include "../../common/core/scriptSnapshot.h"
#include "itemSelectionDialogs/componentSelectorDialog.h"
#include "itemSelectionDialogs/mappedClassSelectorDialog.h"
// fx
#include "../../common/engine/fxTrackGroup.h"
#include "../../common/engine/fxTrack.h"
#include "../../common/engine/fxTrackItem.h"
// attachemnts
#include "../../common/engine/normalBlendAttachment.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/meshSkinningAttachment.h"
// components
#include "../../common/engine/dressMeshComponent.h"
#include "../../common/engine/pathlibNavmeshComponent.h"
#include "../../common/engine/morphedMeshComponent.h"
#include "../../common/engine/dismembermentComponent.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/game/storySceneComponent.h"
#include "../../common/engine/normalBlendComponent.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/furComponent.h"
#include "../../common/engine/slotComponent.h"
#include "../../common/engine/externalProxy.h"
#include "../../common/engine/rigidMeshComponent.h"
#include "../../common/engine/animDangleComponent.h"
// resources
#include "../../common/engine/mesh.h"
#include "../../common/engine/furMeshResource.h"
#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/animatedComponent.h"
#include "../../common/engine/ragdollPhysX.h"
#include "../../common/engine/dyngResource.h"
#include "../../common/core/feedback.h"


enum EEntityGraphEditorIDs
{
	ID_FIRST = wxID_HIGHEST,

	ID_SHOW_INCLUDES,
	ID_HIDE_SELECTION,
	ID_ISOLATE_SELECTION,
	ID_UNHIDE_HIDDEN,

	ID_COPY_COMPONENT,
	ID_CUT_COMPONENT,
	ID_PASTE_COMPONENT,
	ID_CHANGE_CLASS_COMPONENT,
	ID_MERGE_MESH,

	ID_SHOW_COMPONENT_OVERRIDES,
	ID_RESET_OVERRIDDEN_PROPERTIES,

	ID_CONVERT_MESH_COMPONENT,
	ID_CONVERT_STATIC_MESH_COMPONENT,
	ID_CONVERT_RIGID_MESH_COMPONENT_KINEMATIC,
	ID_CONVERT_RIGID_MESH_COMPONENT_DYNAMIC,
	ID_CONVERT_FUR_COMPONENT,
	ID_CONVERT_MOVING_AGENT_COMPONENT,
	ID_CONVERT_SLOT_COMPONENT,
	ID_CONVERT_TO_DRESS_COMPONENT,
	ID_CONVERT_TO_SLOT,
	ID_CONVERT_NPC_ITEM_TO_MESH,
	ID_GENERATE_NAVMESH,
	ID_TOGGLE_NAVMESH_EDDITION,

	ID_REMOVE_STREAMING_DATA,
	ID_RESET_ENTITY_DATA,
	ID_INSPECT_OBJECT,

	ID_SHOW_COMPONENT_SELECTOR,

	ID_ALIGN_COMPONENTS,
	ID_ALIGN_SELECTED_COMPONENTS,
	// Anything that uses this id must count and maintain the index
	ID_RANGE_COUNT
};

BEGIN_EVENT_TABLE( CEdEntityGraphEditor, CEdCanvas )
	EVT_MOUSEWHEEL( CEdEntityGraphEditor::OnMouseWheel )
	EVT_LEFT_DCLICK( CEdEntityGraphEditor::OnMouseLeftDblClick )
END_EVENT_TABLE()

// Return whether first element is greater than the second
namespace
{
	bool ComponentGreater( const CClass *elem1, const CClass *elem2 )
	{
		return elem1->GetName().AsString() <= elem2->GetName().AsString();
	}

	Bool GetInludePath( const CEntityTemplate& tmpl, const CComponent& block, String & path, Bool skipThisTmpl )
	{
		if ( !tmpl.GetEntityObject() )
		{
			path = String::EMPTY;
			return false;
		}

		const TDynArray< CComponent* > & components = tmpl.GetEntityObject()->GetComponents();
		for ( Uint32 i = 0; i < components.Size(); ++i )
		{
			if ( components[i]->GetGUID() == block.GetGUID() )
			{
				if ( components[i]->IsA< CExternalProxyComponent >() )
				{
					break; // This is an external proxy to our component -> we won't find this component here
				}

				if ( skipThisTmpl )
				{
					path = String::EMPTY;
					return true;
				}
				
				if ( tmpl.GetFile() != NULL )
				{
					path = tmpl.GetFile()->GetFileName();
					return true;
				}
				
				path = TXT("???");
				return true;
			}
		}

		const TDynArray< THandle< CEntityTemplate > > & includes = tmpl.GetIncludes();
		for ( Uint32 i = 0; i < includes.Size(); ++i )
		{
			if ( includes[i].Get() && GetInludePath( *includes[i].Get(), block, path, false ) )
			{
				if ( ! skipThisTmpl && tmpl.GetFile() != NULL )
				{
					path = tmpl.GetFile()->GetFileName() + String(TXT(" / ")) + path;
				}
				return true;
			}
		}

		return false;
	}
}

namespace
{
	Bool ManuallyCreatableObjectFilter( CClass *classField )
	{
		CObject *defaultObject = classField->GetDefaultObject< CObject >();
		return defaultObject && defaultObject->IsManualCreationAllowed();
	}
}

CEdEntityGraphEditor::CEdEntityGraphEditor( wxWindow* parent, CEntity* entity, CEntityTemplate* entityTemplate, CEdUndoManager* undoManager, EntityGraphEditorHook* hook, CEdEntityEditor* editor )
	: CEdCanvas( parent )
	, m_lastMousePos( 0,0 )
	, m_selectRectStart( 0,0 )
	, m_selectRectEnd( 0,0 )
	, m_autoScroll( 0,0 )
	, m_moveTotal( 0 )
	, m_action( MA_None )
	, m_hook( hook )
	, m_undoManager( undoManager )
	, m_srcComponent( NULL )
	, m_destComponent( NULL )
	, m_template( entityTemplate )
	, m_activeItem( NULL )
	, m_editor( editor )
	, m_componentClassHierarchyInitialized( false )
	, m_showIncludes( true )
{
	// Load icons
	m_importedIcon	 = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( _T("IMG_EE_IMPORTED") ) );
	m_appearanceIcon = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( _T("IMG_EE_APPEARANCE") ) );
	m_streamedIcon	 = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( _T("IMG_EE_STREAMED") ) );
	m_overridesIcon	 = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( _T("IMG_EE_OVERRIDES") ) );

	// Collect allowed attachment classes
	SRTTI::GetInstance().EnumClasses( ClassID< IAttachment >(), m_attachmentClasses, ::ManuallyCreatableObjectFilter );

	// Restore background offset
	SetBackgroundOffset( wxPoint( m_template->GetBackgroundOffset().X, m_template->GetBackgroundOffset().Y )); 

	LoadOptionsFromConfig();

	// Update layout
	ForceLayoutUpdate();
}

CEdEntityGraphEditor::~CEdEntityGraphEditor()
{
	SaveOptionsToConfig();
	delete m_importedIcon;
	delete m_appearanceIcon;
	delete m_streamedIcon;
	delete m_overridesIcon;
}

CEntity* CEdEntityGraphEditor::GetEntity() const
{
	ASSERT( m_editor, TXT("Editor missing") );
	return m_editor->GetPreviewPanel()->GetEntity();
}

String CEdEntityGraphEditor::GetComponentName( const CComponent & block )
{
	String path;
	GetInludePath( *m_template, block, path, true );

	if ( path.Empty() )
	{
		return block.GetName();
	}

	return TXT("[ ") + path + TXT(" ] ") + block.GetName();
}

String CEdEntityGraphEditor::GetFriendlyName(const CComponent &component)
{
	return component.GetName() + TXT( " {" ) + component.GetClass()->GetName().AsString() + TXT( "}" );
}

Bool CEdEntityGraphEditor::IsComponentVisible(const CComponent &component)
{
	// Check if the component is in the hidden set
	if ( m_hidden.Exist( component.GetName() ) )
	{
		return false;
	}

	// Check if we disallow included components
	if ( ( component.HasFlag( NF_IncludedFromTemplate ) || component.HasComponentFlag( CF_UsedInAppearance ) ) && !m_showIncludes )
	{
		return false;
	}

	return true;
}

void CEdEntityGraphEditor::ForceSelectionUpdate()
{
	// Collect components
	TDynArray< CComponent* > components;
	CollectEntityComponents( GetEntity(), components );

	// Generate new selection list
	m_selected.Clear();
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
        CComponent* tc = Cast< CComponent >( components[i] );
        if ( tc && !IsComponentVisible( *tc ) )
			continue;
		if ( tc && tc->IsSelected() )
		{
			m_selected.PushBack( tc );
		}
	}
	
	// Redraw
	Repaint();

	// Selection updated
	if ( m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

void CEdEntityGraphEditor::ForceLayoutUpdate()
{
	if ( !GetEntity() )
	{
		RED_HALT( "No entity" );
		return;
	}

	// Collect components
	TDynArray< CComponent* > components;
	CollectEntityComponents( GetEntity(), components );

	// Remove invalid entries from hidden components list so they wont remain
	// around after we delete a component with that name and cause issues when
	// we try to create a new component afterwards with the same name
	TDynArray< String > toRemoveFromHidden;
	for ( auto& it=m_hidden.Begin(); it != m_hidden.End(); ++it )
	{
		const String& name = *it;
		if ( !GetEntity()->FindComponent( name ) )
		{
			toRemoveFromHidden.PushBack( name );
		}
	}
	for ( const String& name : toRemoveFromHidden )
	{
		m_hidden.Erase( name );
	}

	// HACK : Dismemberment fill mesh is added to the entity dynamically as a regular CMeshComponent. We don't want to show it
	// in the graph editor...
	CMeshTypeComponent* hack_dismemberFillMesh = nullptr;
	{
		CDismembermentComponent* dismemberComponent = GetEntity()->FindComponent< CDismembermentComponent >();
		if ( dismemberComponent != nullptr )
		{
			hack_dismemberFillMesh = dismemberComponent->GetFillMeshComponent();
		}
	}


	// Update layout for components
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
		CComponent* comp = components[i];

		if ( comp == hack_dismemberFillMesh )
		{
			continue;
		}

		//if ( m_showIncludes || !comp->HasFlag( OF_Referenced ) )
			UpdateBlockLayout( comp );
	}

	Refresh();
}

void CEdEntityGraphEditor::DeleteSelection()
{
	DeleteSelectedObjects();
}

void CEdEntityGraphEditor::CopySelection()
{
	// Only if we have selection
	if ( m_selected.Size() )
	{
		GObjectClipboard.Copy( m_selected );
	}
}

void CEdEntityGraphEditor::CutSelection()
{
	// Only if we have selection
	if ( m_selected.Size() )
	{
		if ( GObjectClipboard.Copy( m_selected ) )
		{
			DeleteSelectedObjects( false );
		}
	}
}
void CEdEntityGraphEditor::AddComponent( CComponent* component, const Vector& origin )
{
	// Grab transform
	Matrix localToWorld = component->GetLocalToWorld();

	AddComponent( component, localToWorld, origin );
}

void CEdEntityGraphEditor::AddComponent( CComponent* component, const Matrix& localToWorld, const Vector& origin )
{
	// Change parent
	component->SetParent( GetEntity() );

	// Put component in the entity's components
	GetEntity()->m_components.PushBack( component );

	// Inform component that it has been spawned
	SComponentSpawnInfo dummySpawnInfo;
	dummySpawnInfo.m_spawnPosition = localToWorld.GetTranslation() + origin;
	dummySpawnInfo.m_spawnRotation = localToWorld.ToEulerAnglesFull();
	dummySpawnInfo.m_spawnScale = localToWorld.GetScale33();
	component->OnSpawned( dummySpawnInfo );

	// Reset component GUIDs
	component->SetGUID( CGUID::Create() );

	// Make a valid name
	GetEntity()->GenerateUniqueComponentName( component );

	// Initialize if entity is initialized
	if ( GetEntity()->IsInitialized() )
	{
		component->PerformInitialization();
	}

	// Attach to world if entity is attached
	if ( GetEntity()->IsAttached() )
	{
		CWorld* world = GetEntity()->GetLayer()->GetWorld();
		component->PerformAttachment( world );
	}

	// Update UI
	UpdateComponent( component, origin.X, origin.Y );

	// Update component transform
	extern void RecursiveImmediateTransformUpdate( CNode* node );
	RecursiveImmediateTransformUpdate( component );
}

void CEdEntityGraphEditor::PasteSelection()
{
	// Open clipboard
	if ( GObjectClipboard.HasObjects() )
	{
		// Paste objects
		TDynArray< CObject* > objects;
		if ( !GObjectClipboard.Paste( objects, NULL ) )
		{
			return;
		}

		// Array of useless objects to discard later
		TDynArray< CObject* > discardLater;

		// Grab all components
		TDynArray< CComponent* > components;
		THashSet< CComponent* > comesFromEntity;
		Vector originForEntityComponents = Vector::ZERO_3D_POINT;
		for ( auto it=objects.Begin(); it != objects.End(); ++it )
		{
			CObject* obj = *it;

			// Found a component, put it in the list
			if ( obj->IsA< CComponent >() )
			{
				components.PushBack( static_cast< CComponent* >( obj ) );
			}
			else if ( obj->IsA< CEntity >() ) // Found an entity, suck out the components out of it
			{
				CEntity* entity = static_cast< CEntity* >( obj );

				// Create streamed components for the entity
				entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
				
				// Collect all components in the entity
				TDynArray< CComponent* > entityComponents;
				CollectEntityComponents( entity, entityComponents );

				// Invalidate the entity
				entity->m_streamingComponents.ClearFast();
				entity->m_components.ClearFast(); // remove components from the entity itself
				discardLater.PushBack( entity );

				// Put the components in the list
				components.PushBack( entityComponents );
				
				// Set the origin for the first component(s), if any
				if ( comesFromEntity.Empty() )
				{
					originForEntityComponents = -entity->GetWorldPosition();
				}

				// Also put the components in the comesFromEntity set so that
				// they get proper transform below
				for ( auto it=entityComponents.Begin(); it != entityComponents.End(); ++it )
				{
					comesFromEntity.Insert( *it );
				}
			}
			else // Unknown object type, just discard it later
			{
				discardLater.PushBack( obj );
			}
		}

		// Reset selection
		DeselectAllObjects();

		// Add all collected components to the entity
		for ( auto it=components.Begin(); it != components.End(); ++it )
		{
			CComponent* component = *it;

			// Remove streaming flags (may be added later)
			component->ClearComponentFlag( CF_StreamedComponent );
			component->ClearFlag( NF_IncludedFromTemplate | OF_Transient | OF_Referenced );

			// Set proper transform matrix
			component->SetLocalToWorldFromTransformDirectly();

			AddComponent( component, originForEntityComponents );
		}

		// Discard objects
		for ( auto it=discardLater.Begin(); it != discardLater.End(); ++it )
		{
			(*it)->Discard();
		}

		SEvents::GetInstance().DispatchEvent( CNAME( PreviewEntityModified ), CreateEventData( m_template ) );
	}

	ForceLayoutUpdate();
}

void CEdEntityGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	wxColour back = GetCanvasColor();
	static wxColour rect( 0, 0, 200 );
	Clear( back );

	if ( !GetEntity() )
	{
		// Draw track name
		DrawText( wxPoint( width / 2, height / 2 ), 
			GetGdiDrawFont(), TXT( "Couldn't load entity" ),
			wxColour( 0, 0, 0 ), CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
		return;
	}

	// Collect components
	TDynArray< CComponent* > components;
	CollectEntityComponents( GetEntity(), components );

	// Update layout
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
		CComponent* component = components[i];

        if ( IsComponentVisible(*component) )
		{
			DrawBlockLayout( component );
		}
	}

	// Draw links
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
		CComponent* component = components[i];
		
        if ( IsComponentVisible(*component) )
		{
			DrawBlockLinks( component );
		}
	}

	// Draw selection rect
	if ( m_action == MA_SelectingWindows )
	{
		// Get size and pos
		wxRect selRect;
		selRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
		selRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
		selRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
		selRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

		// Selection rect
		DrawRect( selRect, rect );
	}

	// Link drawing
	if ( m_action == MA_DraggingLink )
	{
		DrawDraggedLink();
	}

	DrawStatistic();
}

void CEdEntityGraphEditor::MouseClick( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	// Pass to base class
	CEdCanvas::MouseClick( event );

	// Convert to graph coords
	wxPoint pos = ClientToCanvas( point );

	// Zooming
	if ( event.ControlDown() )
	{
		// Only when in no mode
		if ( m_action == MA_None )
		{
			if ( event.LeftDown() )
			{
				// Full zoom in
				m_desiredScale = 1.0f;

				// Don't do any more processing
				return;
			}
			else if ( event.RightDown() )
			{
				// Full zoom out
				m_desiredScale = 0.1f;

				// Don't do any more processing
				return;
			}
		}
	}

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
	{
		m_moveTotal	 = 0;
		m_action = MA_BackgroundScroll;
		CaptureMouse( true, true );
	}
	else if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, true );

		// Minimal movement, show menu
		if ( m_moveTotal < 5 )
		{
			OpenContextMenu();
		}
	}

	// Click
	if ( m_action == MA_None && event.LeftDown() )
	{
		Bool draggingConnection = false;

		// Connector drag
		{
			TDynArray< CComponent* > components;
			CollectEntityComponents( GetEntity(), components );

			// Update layout
			for ( Uint32 i=0; i<components.Size(); i++ )
			{
				CComponent* component = components[i];
				if ( !IsComponentVisible(*component) )	continue;

				LayoutInfo* layout = m_layout.FindPtr( component );
				if ( layout )
				{
					Int32 posX=0, posY=0;
					component->GetGraphPosition( posX, posY );
					wxRect connectorRect( layout->m_connectorRect.x + posX, layout->m_connectorRect.y + posY, layout->m_connectorRect.width, layout->m_connectorRect.height );
					if ( connectorRect.Contains( pos ) )
					{
						m_activeItem = component;
						m_srcComponent = component;
						m_destComponent = NULL;
						m_action = MA_DraggingLink;
						CaptureMouse( true, false );
					}
				}
			}
		}

		// Default actions
		if ( m_action == MA_None )
		{
			// Block clicked, select
			if ( m_activeItem )
			{
				// Block has been clicked, select it and move
				if ( ! m_selected.Exist( m_activeItem ) )
				{
					// No shift, deselect all blocks first
					if ( ! event.ShiftDown() )
						DeselectAllObjects();

					// Select clicked object
					SelectObject( m_activeItem, true );
				}

				// Movement
				m_action = MA_MovingWindows;
				CaptureMouse( true, false );
			}
			else
			{
				// Deselect
				DeselectAllObjects();

				// Initialize selection rect
				m_selectRectStart = ClientToCanvas( point );
				m_selectRectEnd = ClientToCanvas( point ); 

				// Start mode
				m_action = MA_SelectingWindows;
				CaptureMouse( true, false );
			}
		}

		// Repaint
		Repaint();
	}

	// Finished movement
	if ( m_action==MA_MovingWindows && event.LeftUp() )
	{
		m_action = MA_None;
		CaptureMouse( false, false );
	}

	// Finished link dragging
	if ( m_action==MA_DraggingLink && event.LeftUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, false );

		// Finalize link
		if ( m_srcComponent && m_destComponent )
		{
			FinalizeLink();
		}

		// Repaint
		Repaint();
	}

	// Finished window selection
	if ( m_action==MA_SelectingWindows && event.LeftUp() )
	{
		// End drag
		m_action = MA_None;
		CaptureMouse( false, false );

		// Assemble rect
		wxRect selectionRect;
		selectionRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
		selectionRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
		selectionRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
		selectionRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

		// No shift, deselect all blocks first
		if ( ! event.ShiftDown() )
		{
			DeselectAllObjects();
		}

		// Select blocks from area
		SelectObjectsFromArea( selectionRect );

		// Repaint
		Repaint();
	}
}

void CEdEntityGraphEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	wxPoint point = event.GetPosition();

	// Update active item;
	UpdateActiveItem( point );

	// Accumulate move
	m_moveTotal += Abs( delta.x ) + Abs( delta.y );

	// Reset auto scroll
	m_autoScroll = wxPoint(0,0);

	// Remember mouse position
	m_lastMousePos = ClientToCanvas( point );

	// Background drag
	if ( m_action == MA_BackgroundScroll )
	{
		// Update background offset
		Float scale = GetScale();
		ScrollBackgroundOffset( wxPoint( delta.x / scale, delta.y / scale ) );

		// Repaint
		Repaint();
	}

	// Block movement
	if ( m_action == MA_MovingWindows )
	{
		// Move blocks
		Float scale = GetScale();
		MoveSelectedBlocks( wxPoint( delta.x / scale, delta.y / scale ) );

		// Repaint
		Repaint();
	}

	// Selection rect
	if ( m_action == MA_SelectingWindows )
	{
		// Set selection rect
		m_selectRectEnd = ClientToCanvas( point );

		// Repaint
		Repaint();
	}

	// Setup auto scroll
	if ( m_action == MA_MovingWindows || m_action == MA_DraggingLink || m_action == MA_SelectingWindows )
	{
		// Get client size
		Int32 width, height;
		GetClientSize( &width, &height );

		// X edges
		m_autoScroll.x += (event.GetX() < 10) ? 5 : 0;
		m_autoScroll.x -= (event.GetX() > ( width - 10 )) ? 5 : 0;

		// Y edge
		m_autoScroll.y += (event.GetY() < 10) ? 5 : 0;
		m_autoScroll.y -= (event.GetY() > ( height - 10 )) ? 5 : 0;
	}

	// Link dragging
	if ( m_action == MA_DraggingLink )
	{
		// Determine if we can connect to highlighted component
		if ( m_activeItem && m_activeItem->IsA< CComponent >() && m_activeItem != m_srcComponent )
		{
			m_destComponent = SafeCast< CComponent >( m_activeItem );
		}
		else
		{
			m_destComponent = NULL;
		}

		// Repaint
		Repaint();
	}
}

void CEdEntityGraphEditor::OnMouseLeftDblClick( wxMouseEvent &event )
{
	if ( CComponent* component = Cast<CComponent>( m_activeItem ) )
	{
		TDynArray< const CResource* > componentsResources;
		component->GetResource( componentsResources );

		for( Uint32 i=0; i<componentsResources.Size(); ++i )
		{
			if( componentsResources[i] && componentsResources[i]->GetFile() )
			{
				String resourcePath = componentsResources[i]->GetFile()->GetDepotPath();
				SEvents::GetInstance().DispatchEvent( CNAME( OpenAsset ), CreateEventData( resourcePath ) );
			}
		}
	}
}

void CEdEntityGraphEditor::ScaleGraph( Float newScale )
{
	// Different ?
	if ( GetScale() != newScale )
	{
		// Apply new scale
		Float oldScale = GetScale();
		SetScale( newScale );

		// Get mouse cursor position
		wxPoint mouse;
		::GetCursorPos( (POINT*) &mouse );

		// Transform to client space
		mouse = ScreenToClient( mouse );

		// If we are outside the client area then use the center of client area as a zoom focus point
		wxRect clientRect = GetClientRect();
		if ( !clientRect.Contains( mouse ) )
		{
			// Calculate center of client area
			mouse.x = clientRect.x + clientRect.width / 2;
			mouse.y = clientRect.y + clientRect.height / 2;
		}

		// Repair background offset so we stay focused on the same space
		wxPoint offset;
		Float diff = 1.0f / newScale - 1.0f / oldScale;
		offset.x = mouse.x * diff;
		offset.y = mouse.y * diff;
		ScrollBackgroundOffset( offset );
	}
}

void CEdEntityGraphEditor::ZoomExtents( Bool extentsOfSelectionOnly /*= false*/ )
{
	Int32	minX =  NumericLimits<Int32>::Max(),
		minY =  NumericLimits<Int32>::Max(),
		maxX = -NumericLimits<Int32>::Max(),
		maxY = -NumericLimits<Int32>::Max();

	Uint32 numObjects = 0;

	if ( m_layout.Empty() )
	{
		wxSize windowSize = GetParent()->GetSize();
		minX = 0;
		minY = 0;
		maxX = windowSize.x;
		maxY = windowSize.y;
	}
	else
	{
		auto currBlock = m_layoutObjects.Begin();
		auto lastBlock = m_layoutObjects.End();
		for ( ; currBlock != lastBlock; ++currBlock )
		{
			CObject* cobject = (*currBlock).Get();
			if ( !cobject )
			{
				continue;
			}
			LayoutInfo * layout = m_layout.FindPtr( cobject );
			CComponent * component = Cast< CComponent >( cobject );
			ASSERT( component, TXT("Failed to get the component from the block") );
			if ( !component || !layout )
			{
				continue;
			}

			if ( extentsOfSelectionOnly && ! component->IsSelected() )
			{
				continue;
			}
			if ( ! IsComponentVisible( *component ) )
			{
				continue;
			}

			++numObjects;

			Int32 x, y;
			component->GetGraphPosition( x, y );

			minX = Min( minX, x );
			minY = Min( minY, y );
			maxX = Max( maxX, x+layout->m_windowSize.x );
			maxY = Max( maxY, y+layout->m_windowSize.y );
		}
	}

	if ( numObjects == 0 )
	{
		return;
	}

	wxSize  windowSize = GetParent()->GetSize();
	wxPoint graphCenter( ( maxX + minX ) / 2, ( maxY + minY ) / 2 );
	wxSize  graphSize( (Int32)( ( maxX - minX ) * 1.2f ) , (Int32)( ( maxY - minY ) * 1.2f ) );

	Float xScale = (Float)windowSize.GetWidth() / graphSize.GetWidth();
	Float yScale = (Float)windowSize.GetHeight() / graphSize.GetHeight();

	Float scale = Min( xScale, yScale );
	scale = Clamp( scale, 0.1f, 2.0f );

	wxPoint graphCorner( graphCenter.x - ( (windowSize.x / 2) / scale ), graphCenter.y - ( (windowSize.y / 2) / scale ) );	
	wxPoint newOffset( -graphCorner.x , -graphCorner.y );

	SetScale( scale );
	m_desiredScale = scale;
	SetBackgroundOffset( newOffset );
}

void CEdEntityGraphEditor::SetBackgroundOffset( wxPoint offset )
{
	m_template->SetBackgroundOffset( Vector( offset.x, offset.y, 0, 0 ) );
	SetOffset( wxPoint( m_template->GetBackgroundOffset().X, m_template->GetBackgroundOffset().Y )); 
}

void CEdEntityGraphEditor::ScrollBackgroundOffset( wxPoint delta )
{
	m_template->SetBackgroundOffset( m_template->GetBackgroundOffset() + Vector( delta.x, delta.y, 0, 0 ) );
	SetOffset( wxPoint( m_template->GetBackgroundOffset().X, m_template->GetBackgroundOffset().Y )); 
}

void CEdEntityGraphEditor::DeleteSelectedObjects( bool askUser /* = true */ )
{
	// Ask the question
	if ( askUser && m_selected.Size() )
	{
		if ( !YesNo( TXT("Sure to delete selection ?") ) )
		{
			return;
		}
	}

	// Reset selection
	TDynArray< THandle< CObject > > selection = m_selected;

	// Delete selected objects
	for ( Uint32 i=0; i<selection.Size(); i++ )
	{
		if ( selection[i].IsValid() )
		{
			CObject* object = selection[i].Get();
			if ( object->IsA< IAttachment >() )
			{
				IAttachment* attachment = SafeCast< IAttachment >( object );

				if ( attachment->IsA< CHardAttachment >() )
				{
					if ( YesNo( TXT("Preserve world position of component %s?"), attachment->GetChild()->GetName().AsChar() ) )
					{
						attachment->GetChild()->SetPosition( attachment->GetChild()->GetWorldPosition() );
						attachment->GetChild()->SetRotation( attachment->GetChild()->GetWorldRotation() );
					}
				}

				// Inform the appearance component that the attachment was explicitly deleted so it will remove any references to it
				CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( GetEntity() );
				if ( appearanceComponent != nullptr )
				{
					appearanceComponent->RemoveAppearanceAttachment( attachment );
				}

				if ( m_undoManager )
				{
					CUndoCreateDestroy::CreateStep( m_undoManager, attachment, false );
				}
				else
				{
					attachment->Break();
				}
			}
			else if ( object->IsA< CComponent >() )
			{
				CComponent* component = SafeCast< CComponent >( object );

				// If this is undone, 
				RemoveFromLayout( component );

				if ( m_undoManager )
				{
					CUndoCreateDestroy::CreateStep( m_undoManager, component, false, false );
				}
				else
				{
					component->Destroy();
				}
			}
		}
	}
	if ( m_undoManager )
	{
		CUndoCreateDestroy::FinishStep( m_undoManager );
	}

	// selection is actually reset after destroying the component, which prevents deleted objects from being included in the properties pages
	DeselectAllObjects();

	// Update layout
	ForceLayoutUpdate();

	if ( m_hook )
	{
		m_hook->OnGraphSelectionDeleted();
	}

	SEvents::GetInstance().DispatchEvent( CNAME( PreviewEntityModified ), CreateEventData( m_template ) );
}

void CEdEntityGraphEditor::DeselectAllObjects()
{
	// Deselect
	for ( Uint32 i=0; i<m_selected.Size(); i++ )
	{
		CComponent* component = Cast< CComponent >( m_selected[i].Get() );
		if ( component != nullptr )
		{
			component->Select( false );
		}
	}

	// Clear selection list
	m_selected.Clear();

	// Send event
	if ( m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

Bool CEdEntityGraphEditor::IsObjectSelected( CObject* object )
{
	ASSERT( object );
	return m_selected.Exist( object );
}

void CEdEntityGraphEditor::GetSelectedObjects( TDynArray< CObject* >& objects )
{
	objects.ClearFast();
	for ( THandle< CObject >& handle : m_selected )
	{
		if ( handle.IsValid() )
		{
			objects.PushBack( handle.Get() );
		}
	}
}

void CEdEntityGraphEditor::GetAllObjects( TDynArray< CObject* >& objects )
{
	Int32 num = m_layoutObjects.Size();
	Int32 i;
	objects.Reserve( num );
	for( i=0;i<num;++i)
	{
		objects.PushBack( m_layoutObjects[i] );
	}
}

void CEdEntityGraphEditor::SelectObject( CObject* object, Bool select )
{
	// Generic case
	ASSERT( object );
	if ( select )
	{
		m_selected.PushBackUnique( object );
	}
	else
	{
		m_selected.Remove( object );
	}

	// Components
	if ( object->IsA< CComponent >() )
	{
		CComponent* tc = SafeCast< CComponent >( object );
		tc->Select( select );
	}

	// Send event
	if ( m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

void CEdEntityGraphEditor::MoveSelectedBlocks( wxPoint totalOffset )
{
	// Move selected components
	for ( Uint32 i=0; i<m_selected.Size(); i++ )
	{
		if ( m_selected[i].IsValid() )
		{
			CObject* object = m_selected[i].Get();
			if ( object->IsA< CComponent >() )
			{
				CComponent* comp = SafeCast< CComponent >( object );
				Int32 posX=0, posY=0;
				comp->GetGraphPosition( posX, posY );
				posX += totalOffset.x;
				posY += totalOffset.y;
				comp->SetGraphPosition( posX, posY );
			}
		}
	}
}

void CEdEntityGraphEditor::SelectObjectsFromArea( wxRect area )
{
	// Send event
	if ( m_hook )
	{
		TDynArray< CComponent* > components;
		CollectEntityComponents( GetEntity(), components );

		// Linear search
		for ( Uint32 i=0; i < components.Size(); i++ )
		{
			// Get layout
			CComponent* block = components[i];
			const LayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info )
			{
				// Get block rect
				wxRect blockRect;
				Int32 posX=0, posY=0;
				block->GetGraphPosition( posX, posY );
				blockRect.x = posX;
				blockRect.y = posY;
				blockRect.width = info->m_windowSize.x;
				blockRect.height = info->m_windowSize.y;

				// Selected ?				
				if ( area.Intersects( blockRect ) && !IsObjectSelected( block ) )
				{
					SelectObject( block, true );
				}
			}
		}

		m_hook->OnGraphSelectionChanged();
	}
}

void CollectEffectsThatUsesComponent( CComponent* component, CEntityTemplate* entityTemplate, TDynArray< CFXDefinition* >& usedEffects )
{
	CName componentName( component->GetName() );

	// Scan effects
	const TDynArray< CFXDefinition* >& effects = entityTemplate->GetEffects();
	for ( Uint32 i=0; i<effects.Size(); ++i )
	{
		CFXDefinition* fx = effects[i];

		// Scan track groups
		Bool isUsed = false;
		const TDynArray< CFXTrackGroup* >& trackGroups = fx->GetEffectTrackGroup();
		for ( Uint32 j=0; j<trackGroups.Size(); ++j )
		{
			CFXTrackGroup* group = trackGroups[j];

			// Used directly
			if ( group->GetComponentName() == componentName )
			{
				isUsed = true;
				break;
			}

			// Check tracks
			if ( !isUsed )
			{
				const TDynArray< CFXTrack* >& tracks = group->GetTracks();
				for ( Uint32 k=0; k<tracks.Size(); ++k )
				{
					CFXTrack* track = tracks[k];

					// Check track items
					const TDynArray< CFXTrackItem* >& trackItems = tracks[k]->GetTrackItems();
					for ( Uint32 l=0; l<trackItems.Size(); ++l )
					{
						CFXTrackItem* item = trackItems[l];
						if ( item->UsesComponent( componentName ) )
						{
							isUsed = true;
							break;
						}
					}
				}
			}
		}

		// Remember
		if ( isUsed	)
		{
			usedEffects.PushBackUnique( fx );
		}
	}

	// Recurse
	const TDynArray< THandle< CEntityTemplate > >& includes = entityTemplate->GetIncludes();
	for ( Uint32 i=0; i<includes.Size(); ++i )
	{
		CEntityTemplate* includedTemplate = includes[i].Get();
		if ( includedTemplate )
		{
			CollectEffectsThatUsesComponent( component, includedTemplate, usedEffects );
		}
	}
}

class wxEffectDataWrapper : public wxObject
{
public:
	THandle< CFXDefinition >		m_effect;

public:
	wxEffectDataWrapper( CFXDefinition* def )
		: m_effect( def )
	{};
};

void CEdEntityGraphEditor::OpenContextMenu()
{
	wxMenu* menu = new wxMenu();

	Int32 rangeMenuIdCount = ID_RANGE_COUNT;

	// Default menu
	if ( !m_activeItem )
	{
		m_toggleShowIncludesStartingId = rangeMenuIdCount;

		// Add "show all" item
		wxMenuItem* itemSA = menu->AppendCheckItem( ID_SHOW_INCLUDES, TXT("Show includes") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnToggleShowIncludes, this, ID_SHOW_INCLUDES );
		itemSA->Check( m_showIncludes );
		menu->AppendCheckItem( ID_HIDE_SELECTION, TXT("Hide selection") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnHideSelection, this, ID_HIDE_SELECTION );
		menu->AppendCheckItem( ID_ISOLATE_SELECTION, TXT("Isolate selection") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnIsolateSelection, this, ID_ISOLATE_SELECTION );
		menu->AppendCheckItem( ID_UNHIDE_HIDDEN, TXT("Unhide hidden components") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnUnhideHiddenComponents, this, ID_UNHIDE_HIDDEN );
		menu->AppendSeparator();

		menu->Append(ID_PASTE_COMPONENT, TXT("Paste"));
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnEditPaste, this, ID_PASTE_COMPONENT );
		menu->AppendSeparator();

		menu->Append( ID_SHOW_COMPONENT_SELECTOR, TXT( "Select Component..." ) );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnSelectComponent, this, ID_SHOW_COMPONENT_SELECTOR );

		menu->Append( ID_ALIGN_SELECTED_COMPONENTS, TXT( "Realign Selected Components." ) );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnAlignComponents, this, ID_ALIGN_SELECTED_COMPONENTS );

		menu->Append( ID_ALIGN_COMPONENTS, TXT( "Realign All Components." ) );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnAlignComponentsAll, this, ID_ALIGN_COMPONENTS );

		wxMenu* recentItemsMenu = new wxMenu();

		m_recentlyUsedBlockClassesStartingId = rangeMenuIdCount;

		for( Uint32 i = 0; i < m_recentlyUsedBlockClasses.Size(); ++i )
		{
			CClass* blockClass = m_recentlyUsedBlockClasses[ i ];
			if( blockClass ) 
				recentItemsMenu->Append( rangeMenuIdCount++, blockClass->GetName().AsString().AsChar() );
		}

		if( rangeMenuIdCount > m_recentlyUsedBlockClassesStartingId )
		{
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnSpawnRecentBlock, this, m_recentlyUsedBlockClassesStartingId, rangeMenuIdCount - 1 );
		}

		menu->Append( wxID_ANY, TXT( "Recent Components" ), recentItemsMenu );
		
		menu->AppendSeparator();
		menu->Append( ID_REMOVE_STREAMING_DATA, wxT("Remove streaming data") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnRemoveStreamingData, this, ID_REMOVE_STREAMING_DATA );
		menu->Append( ID_RESET_ENTITY_DATA, wxT("Reset all entity data") );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnResetEntityData, this, ID_RESET_ENTITY_DATA );
	}
	else
	{
		// Menu for component
		CComponent *component = Cast<CComponent>( m_activeItem );
		if ( !component )
		{
			return;
		}

		menu->Append(ID_COPY_COMPONENT, TXT("Copy"));
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnEditCopy, this, ID_COPY_COMPONENT );
		menu->Append(ID_CUT_COMPONENT, TXT("Cut"));
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnEditCut, this, ID_CUT_COMPONENT );
		menu->Append(ID_CHANGE_CLASS_COMPONENT, TXT("Change component class"));
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnChangeComponentClass, this, ID_CHANGE_CLASS_COMPONENT );
		menu->Append(ID_MERGE_MESH, TXT("Merge into one MeshComponent"));
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnMergeMesh, this, ID_MERGE_MESH );

		if ( m_template->HasOverridesForComponent( component ) )
		{
			menu->AppendSeparator();
			menu->Append( ID_SHOW_COMPONENT_OVERRIDES, TXT("Show overrides") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnShowComponentOverrides, this, ID_SHOW_COMPONENT_OVERRIDES );
			menu->Append( ID_RESET_OVERRIDDEN_PROPERTIES, TXT("Reset overridden properties") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnResetOverriddenProperties, this, ID_RESET_OVERRIDDEN_PROPERTIES );
		}

		if ( component->IsA< CMeshComponent >() ) 
		{
			menu->AppendSeparator();
			menu->Append(ID_CONVERT_MESH_COMPONENT, TXT("Convert to mesh"));
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMesh, this, ID_CONVERT_MESH_COMPONENT );
			menu->Append(ID_CONVERT_FUR_COMPONENT, TXT("Convert to fur"));
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMesh, this, ID_CONVERT_FUR_COMPONENT );
			menu->Append(ID_CONVERT_STATIC_MESH_COMPONENT, TXT("Convert to static mesh"));
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMesh, this, ID_CONVERT_STATIC_MESH_COMPONENT );
			menu->Append(ID_CONVERT_RIGID_MESH_COMPONENT_KINEMATIC, TXT("Convert to rigid mesh kinematic"));
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMesh, this, ID_CONVERT_RIGID_MESH_COMPONENT_KINEMATIC );
			menu->Append(ID_CONVERT_RIGID_MESH_COMPONENT_DYNAMIC, TXT("Convert to rigid mesh dynamic"));
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMesh, this, ID_CONVERT_RIGID_MESH_COMPONENT_DYNAMIC );
		}
		else if ( component->IsA< CAnimatedComponent >() )
		{
			menu->AppendSeparator();

			if ( component->IsExactlyA< CMovingAgentComponent >() )
			{
				menu->Append(ID_CONVERT_MOVING_AGENT_COMPONENT, TXT("Convert to moving physical agent component"));
			}
			else
			{
				menu->Append(ID_CONVERT_MOVING_AGENT_COMPONENT, TXT("Convert to moving agent component"));
			}
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertMovingAgent, this, ID_CONVERT_MOVING_AGENT_COMPONENT );
			menu->Append( ID_CONVERT_SLOT_COMPONENT, TXT("Convert to slot component") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertAnimComponentToSlotComponent, this, ID_CONVERT_SLOT_COMPONENT );
		}
		else if ( component->IsA< CSlotComponent >() )
		{
			menu->AppendSeparator();

			menu->Append( ID_CONVERT_SLOT_COMPONENT, TXT("Convert to animated component") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertSlotComponentToAnimComponent, this, ID_CONVERT_SLOT_COMPONENT );
		}
		else if ( component->IsA< CNavmeshComponent >() )
		{
			CNavmeshComponent* naviComponent = static_cast< CNavmeshComponent* >( component );
			menu->AppendSeparator();
			if ( naviComponent->IsNavmeshGenerationRunning() )
			{
				menu->Append( ID_GENERATE_NAVMESH, TXT("Generation is running...") );
			}
			else
			{
				menu->Append( ID_GENERATE_NAVMESH, TXT("Generate navmesh") );
				menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnGenerateNavmesh, this, ID_GENERATE_NAVMESH );
				menu->Append( ID_TOGGLE_NAVMESH_EDDITION, m_editor->GetPreviewPanel()->IsNavmeshEditorActive() ? TXT("Disable navmesh editor") : TXT("Edit navmesh") );
				menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnToggleNavmeshEdition, this, ID_TOGGLE_NAVMESH_EDDITION );
			}
			
		}

		if ( m_selected.Size() == 2 )
		{
			CAnimatedComponent* animatedComponent = NULL;
			CMeshComponent* meshComponent = NULL;
			
			for ( Uint32 i=0; i<m_selected.Size(); ++i )
			{
				if ( m_selected[i].IsValid() )
				{
					CObject* obj = m_selected[i].Get();
					if ( obj->IsA< CAnimatedComponent >() )
					{
						animatedComponent = Cast< CAnimatedComponent >( obj );
					}
					else if ( obj->IsA< CMeshComponent >() )
					{
						meshComponent = Cast< CMeshComponent >( obj );
					}
				}
			}
		}

		// Slots
		if ( component->IsA<CSpriteComponent>() || component->IsA<CSlotComponent>() )
		{
			menu->AppendSeparator();

			menu->Append( ID_CONVERT_TO_SLOT, TXT("Convert to entity slot(s)") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnConvertToSlot, this, ID_CONVERT_TO_SLOT );
		}

		// Find effects that uses this component
		TDynArray< CFXDefinition* > effects;
		CollectEffectsThatUsesComponent( component, m_template, effects );

		// Create sub menu for the used effects
		if ( !effects.Empty() )
		{
			wxMenu* usedEffectsMenu = new wxMenu();

			m_usedEffectsStartingId = rangeMenuIdCount;

			for ( Uint32 i=0; i<effects.Size(); ++i )
			{
				CFXDefinition* def = effects[i];
				usedEffectsMenu->Append( rangeMenuIdCount++, def->GetName().AsString().AsChar() );
				menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnEditComponentRelatedEffect, this, rangeMenuIdCount, wxID_ANY, new wxEffectDataWrapper( def ) );
			}

			menu->AppendSeparator();
			menu->Append( wxID_ANY, wxT("Used by Effects"), usedEffectsMenu );
		}

		// Add inspector
		if ( IsObjectInspectorAvailable() )
		{
			menu->AppendSeparator();
			menu->Append( ID_INSPECT_OBJECT, wxT("Inspect object...") );
			menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnInspectObject, this, ID_INSPECT_OBJECT );
		}
    }

	// Show menu
	PopupMenu( menu );
}

void CEdEntityGraphEditor::OnSelectComponent( wxCommandEvent& event )
{
	CEdComponentSelectorDialog selector( this );
	if ( CClass* blockClass = selector.Execute() )
	{
		SpawnBlock( blockClass );
	}
}

void CEdEntityGraphEditor::OnAlignComponents( wxCommandEvent& event )
{
	m_editor->RealignOverlappingComponents( FALSE );
}
void CEdEntityGraphEditor::OnAlignComponentsAll( wxCommandEvent& event )
{
	m_editor->RealignOverlappingComponents( TRUE );
}


void CEdEntityGraphEditor::UpdateActiveItem( wxPoint mousePoint )
{
	// Reset active item
	CObject* activeItem = NULL;

	// Convert to graph coords
	wxPoint pos = ClientToCanvas( mousePoint );

	// Collect components
	TDynArray< CComponent* > components;
	CollectEntityComponents( GetEntity(), components );

	// Hit test components
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
        CComponent* component = components[i];
        if ( !IsComponentVisible(*component) )
			continue;

		LayoutInfo* layout = m_layout.FindPtr( component );
		if ( layout )
		{
			Int32 posX=0, posY=0;
			component->GetGraphPosition( posX, posY );
			wxRect rect( posX, posY, layout->m_windowSize.x, layout->m_windowSize.y );
			if ( rect.Contains( pos ))
			{
				activeItem = component;
				break;
			}
		}
	}

	// Hit test attachments
	if ( !activeItem )
	{
		// Check attachments of all components
		for ( Uint32 i=0; i<components.Size(); i++ )
		{
			// Skip components that are not visible
            if ( !IsComponentVisible(*components[i]) )
			{
				continue;
			}

			TList< IAttachment* > attachments = components[i]->GetChildAttachments();
			for ( TList< IAttachment* >::iterator it=attachments.Begin(); it!=attachments.End(); ++it )
			{
				IAttachment* attachment = *it;
				CComponent* component = Cast< CComponent >( (*it)->GetChild() );
				if ( component )
				{
					// Ignore component from item entities shown in preview
					if ( GetEntity() && !GetEntity()->IsA<CItemEntity>() && component->GetEntity()->IsA< CItemEntity >() )
					{
						continue;
					}

					// This component is not visible in editor
					if ( !IsComponentVisible( *component ) )
					{
						continue;
					}

					if ( !( attachment->GetParent() && attachment->GetChild() ) )
					{

						continue;
					}

					if ( !m_layout.FindPtr( attachment->GetParent() ) )
					{
						continue;
					}

					if ( !m_layout.FindPtr( attachment->GetChild() ) )
					{
						continue;
					}

					// Calculate curve
					wxPoint points[4];
					CalculateBezierPoints( Cast< CComponent >( (*it)->GetParent() ), Cast< CComponent >( (*it)->GetChild() ), points[0], points[1], points[2], points[3] );

					// Sample curve
					if ( HitTestCurve( points, pos, 15.0f ) )
					{
						activeItem = *it;
						break;
					}
				}
			}
		}
	}

	// New active object
	if ( activeItem != m_activeItem )
	{
		m_activeItem = activeItem;
		Repaint();
	}
}

void CEdEntityGraphEditor::UpdateComponent( CComponent* component, Int32 x, Int32 y )
{
	// Select it
	if ( component )
	{
		// Update pos
		component->SetGraphPosition( x, y );

		// Update layout
		UpdateBlockLayout( component );

		// Select
		SelectObject( component, true );

		// Redraw
		Repaint();
	}
}

void CEdEntityGraphEditor::UpdateBlockLayout( CComponent* block )
{
	// Find or create layout info
	LayoutInfo* layout = m_layout.FindPtr( block );

	// Not defined, create
	if ( !layout )
	{
		m_layout.Insert( block, LayoutInfo() );
		layout = m_layout.FindPtr( block );
		m_layoutObjects.PushBackUnique( block );
	}

	// Calculate text size
	wxPoint size = TextExtents( GetGdiBoldFont(), GetComponentName(*block) );
	Int32 pos = ( layout->m_windowSize.x - size.x ) / 2;

	// Setup size to some default size
	layout->m_windowSize.x = 20 + size.x;
	layout->m_windowSize.y = 30;

	// Create connector
	layout->m_connectorSrc.x = layout->m_windowSize.x / 2;
	layout->m_connectorSrc.y = layout->m_windowSize.y;
	layout->m_connectorDest.x = layout->m_windowSize.x / 2;
	layout->m_connectorDest.y = 0;
	layout->m_connectorRect = wxRect( layout->m_connectorSrc.x-5, layout->m_connectorSrc.y-5, 10, 10 );
}

void CEdEntityGraphEditor::GetBlockWidthHeight( CComponent* block, Int32 & outX, Int32 & outY )
{
	LayoutInfo* layout = m_layout.FindPtr( block );
	if( layout )
	{
		outX = layout->m_windowSize.GetX();
		outY = layout->m_windowSize.GetY();
	}
}

void CEdEntityGraphEditor::DrawBlockLayout( CComponent* block )
{
	// Find layout info
	LayoutInfo* layout = m_layout.FindPtr( block );
	if ( !layout )
	{
		return;
	}

	// Calculate window offset
	Int32 posX=0, posY=0;
	block->GetGraphPosition( posX, posY );
	wxPoint location( posX, posY );

    // Get colors
    Color clientColor = Color( 142, 142, 142 );
	Color borderColor = Color( 0, 0, 0 );
	Bool hasOverrides = false;

	if ( m_template->GetEntityObject() && m_template->GetEntityObject()->FindComponent( block->GetGUID() ) )
	{
		if ( m_template->HasOverridesForComponent( block ) )
		{
			clientColor = Color( 155, 120, 163 );
			hasOverrides = true;
		}
		else if ( !block->HasFlag( NF_IncludedFromTemplate ) )
		{
			clientColor = Color( 148, 124, 82 );
		}
	}

	// Border highlights
	if ( m_activeItem == block )
	{
		borderColor = Color( 255, 255, 255 );
	}
	else if ( IsObjectSelected( block ) ) 
	{
		borderColor = Color( 255, 255, 0 );
	}

	// Setup wx colors
	wxColour border( borderColor.R, borderColor.G, borderColor.B );
	wxColour client( clientColor.R, clientColor.G, clientColor.B );
	wxColour shadow( 40, 40, 40 );
    wxColour text = wxColor( 255, 255, 255 );
	
	if ( block->HasFlag( OF_Referenced ) )
	{
		if ( block->IsStreamed() )
		{
			text = wxColor( 0, 255, 255 );
		}
		else
		{
			text = wxColor( 255, 64, 32 );
		}
    }
	else if ( block->HasFlag( NF_IncludedFromTemplate ) && !block->IsStreamed() )
	{
		text = wxColor( 255, 192, 121 );
	}

	wxColour connector( 255, 128, 64 );

	// Draw window area
	FillRoundedRect( location.x, location.y, layout->m_windowSize.x, layout->m_windowSize.y, client, 8 );
	DrawRoundedRect( location.x, location.y, layout->m_windowSize.x, layout->m_windowSize.y, border, 8 );

	// Draw icons
	Int32 iconX = location.x + layout->m_windowSize.x - 16;
	if ( block->HasFlag( NF_IncludedFromTemplate ) )
	{
		DrawImage( m_importedIcon, iconX, location.y - 9, wxColour( 255, 0, 255 ) );
		iconX -= 18;
	}
	if ( block->HasComponentFlag( CF_UsedInAppearance ) )
	{
		DrawImage( m_appearanceIcon, iconX, location.y - 9, wxColour( 255, 0, 255 ) );
		iconX -= 18;
	}
	if ( hasOverrides )
	{
		DrawImage( m_overridesIcon, iconX, location.y - 9, wxColour( 255, 0, 255 ) );
		iconX -= 18;
	}
	if ( block->HasComponentFlag( CF_StreamedComponent ) )
	{
		DrawImage( m_streamedIcon, iconX, location.y - 9, wxColour( 255, 0, 255 ) );
		iconX -= 18;
	}

	// Calculate caption position
	wxPoint size = TextExtents( GetGdiBoldFont(), GetComponentName(*block) );
	Int32 pos = ( layout->m_windowSize.x - size.x ) / 2;

	// Draw text	
	DrawText( wxPoint( location.x + pos + 0, location.y + 10 ), GetGdiBoldFont(), GetComponentName(*block), shadow );
	DrawText( wxPoint( location.x + pos - 1, location.y + 9 ), GetGdiBoldFont(), GetComponentName(*block), text );

	// Draw connector
	wxRect connectorRect( layout->m_connectorRect.x + location.x, layout->m_connectorRect.y + location.y, layout->m_connectorRect.width, layout->m_connectorRect.height );
	DrawConnector( connectorRect, border, connector );

	String comment;
	if ( m_editor->FindComponentComment( block, comment ) )
	{
		DrawText( wxPoint( location.x + 0 + 8, location.y + 1 - 20 ), GetGdiDrawFont(), comment, shadow );
		DrawText( wxPoint( location.x - 1 + 8, location.y + 0 - 20 ), GetGdiDrawFont(), comment, wxColor( 255, 128, 128 ) );
	}
}

void CEdEntityGraphEditor::DrawConnector( const wxRect& rect, const wxColour& borderColor, const wxColour &baseColor )
{
	wxColour shadowColor( baseColor.Red() * 3 / 4, baseColor.Green() * 3 / 4, baseColor.Blue() * 3 / 4 );

	// Draw connector
	DrawCircle( rect.x+0, rect.y+0, rect.width, borderColor );
	FillCircle( rect.x+1, rect.y+1, rect.width-2, baseColor );
	DrawCircle( rect.x+1, rect.y+1, rect.width-2, shadowColor );
}

void CEdEntityGraphEditor::DrawBlockLinks( CComponent* block )
{
	// Draw child attachments
	const TList< IAttachment* >& attachments = block->GetChildAttachments();
	for ( TList< IAttachment* >::const_iterator it=attachments.Begin(); it!=attachments.End(); ++it )
	{
		IAttachment* attachment = *it;
		ASSERT( attachment );
		ASSERT( attachment->GetParent() == block );

        CComponent *component = Cast< CComponent >( attachment->GetChild() );
		if ( component )
		{
			// Skip hidden components
			if ( !IsComponentVisible(*component) )
			{
				continue;
			}

			// Don't draw unwanted links for item entities spawned in preview
			if ( GetEntity() && !GetEntity()->IsA<CItemEntity>() && component->GetEntity()->IsA< CItemEntity >() )
			{
				continue;
			}

			if ( !( attachment->GetParent() && attachment->GetChild() ) )
			{
				
				continue;
			}

			if ( !m_layout.FindPtr( attachment->GetParent() ) )
			{
				continue;
			}

			if ( !m_layout.FindPtr( attachment->GetChild() ) )
			{
				continue;
			}

			// Calculate curve
			wxPoint points[4];
			CalculateBezierPoints( Cast< CComponent >( attachment->GetParent() ), Cast< CComponent >( attachment->GetChild() ), points[0], points[1], points[2], points[3] );

			// Determine color
			wxColour linkColor( 180,180,180 );
			if ( m_activeItem == attachment )
			{
				linkColor = wxColour( 255, 255, 255 );
			}
			else if ( IsObjectSelected( attachment ) ) 
			{
				linkColor = wxColour( 255, 255, 0 );
			}

			// Draw link curve with arrow !
			DrawCurve( points, linkColor, 2.0f );

			if ( m_activeItem == attachment )
			{
				wxPoint& a = points[0];
				wxPoint& b = points[3];
				wxPoint c = wxPoint( ( b.x + a.x )/2, ( b.y + a.y )/2 );
				DrawText( wxPoint( c.x - 1, c.y ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x + 1, c.y ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x, c.y - 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x, c.y + 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x - 1, c.y - 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x + 1, c.y - 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x - 1, c.y + 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( wxPoint( c.x + 1, c.y + 1 ), GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxBLACK, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
				DrawText( c, GetGdiDrawFont(), attachment->GetClass()->GetName().AsChar(), *wxYELLOW, CEdCanvas::CVA_Center, CEdCanvas::CHA_Center );
			}
		}
	}
}

void CEdEntityGraphEditor::CalculateBezierPoints( CComponent* srcComponent, CComponent* destComponent, wxPoint& a, wxPoint& b, wxPoint& c, wxPoint& d )
{
	wxPoint src, srcDir, dest, destDir;

	// Source
	LayoutInfo* srcLayout = m_layout.FindPtr( srcComponent );
	if ( srcLayout && srcComponent )
	{
		Int32 posX=0, posY=0;
		srcComponent->GetGraphPosition( posX, posY );
        src = srcLayout->m_connectorSrc + wxPoint( posX, posY );
		srcDir = wxPoint( 0,1 );
	}
	else
	{
		src = m_lastClickPoint;
		srcDir = wxPoint( 0,0 );
	}

	// Destination
	LayoutInfo* destLayout = m_layout.FindPtr( destComponent );
	if ( destComponent && destLayout )
	{
		Int32 posX=0, posY=0;
		destComponent->GetGraphPosition( posX, posY );
        dest = destLayout->m_connectorDest + wxPoint( posX, posY );
		destDir = wxPoint( 0,-1 );
	}
	else
	{
		dest = m_lastMousePos;
		destDir = wxPoint( 0,0 );
	}

	// Calculate distance between end points
	Int32 dist = Max( Abs( src.x-dest.x ), Abs( src.y-dest.y ) );
	Int32 offset = 5.0f + Min( dist / 3.0f, 200.0f );

	// Setup link
	a.x = src.x;
	a.y = src.y;
	b.x = src.x + srcDir.x * offset;
	b.y = src.y + srcDir.y * offset;
	c.x = dest.x + destDir.x * offset;
	c.y = dest.y + destDir.y * offset;
	d.x = dest.x;
	d.y = dest.y;
}

void CEdEntityGraphEditor::DrawDraggedLink()
{
	// Calculate curve
	wxPoint points[4];
	CalculateBezierPoints( m_srcComponent, m_destComponent, points[0], points[1], points[2], points[3] );

	// Draw link curve with arrow !
	wxColour linkColor( 180,180,180 );
	DrawCurve( points, linkColor, 1.0f );
}

void CEdEntityGraphEditor::FinalizeLink()
{
	ASSERT( m_srcComponent );
	ASSERT( m_destComponent );

	// Create menu with list of available attachment classes
	wxMenu* menu = new wxMenu();
	for ( Uint32 i=0; i<m_attachmentClasses.Size(); i++ )
	{
		// Create menu
		CClass* blockClass = m_attachmentClasses[ i ];

		// Ignore external attachment proxies
		if ( blockClass->IsA< CExternalProxyAttachment >() )
		{
			continue;
		}

		if ( IsAttachmentAllowed( m_srcComponent, m_destComponent, blockClass ) == false )
		{
			continue;
		}

		String caption = String::Printf( TXT("Link using '%s'"), blockClass->GetName().AsString().AsChar() );
		menu->Append( ID_RANGE_COUNT + i, caption.AsChar() );
		menu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdEntityGraphEditor::OnSpawnAttachment, this );
	}

	// Show menu
	PopupMenu( menu );
}

void CEdEntityGraphEditor::OnMouseWheel( wxMouseEvent& event )
{
	// Scale
	Float scale = GetScale();
	Float delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	scale = Clamp< Float >( scale + delta * 0.10f, 0.10f, 2.0f );

	// Set new desired scale
	m_desiredScale = scale;

	// Automatic scaling
	if ( GetScale() != m_desiredScale )
	{
		// Set new scale
		ScaleGraph( m_desiredScale );

		// Repaint
		Repaint( true );
	}
}

void CEdEntityGraphEditor::OnSpawnRecentBlock( wxCommandEvent& event )
{
	Uint32 index = static_cast< Uint32 >( event.GetId() ) - m_recentlyUsedBlockClassesStartingId;
	CClass* blockClass =  m_recentlyUsedBlockClasses[ index ];

	SpawnBlock( blockClass );
}

void CEdEntityGraphEditor::SpawnBlock( CClass* blockClass )
{
	ASSERT( blockClass );

	Int32 existingIndex = m_recentlyUsedBlockClasses.GetIndex( blockClass );

	if( existingIndex != -1 )
	{
		m_recentlyUsedBlockClasses.RemoveAt( existingIndex );
	}
	else if( m_recentlyUsedBlockClasses.Size() == m_recentlyUsedBlockClasses.Capacity() )
	{
		m_recentlyUsedBlockClasses.PopBack();
	}

	m_recentlyUsedBlockClasses.Insert( 0, blockClass );

	// Create block
	CComponent* component = NULL;
	component = GetEntity()->CreateComponent( blockClass, SComponentSpawnInfo() );
	GetEntity()->UpdateStreamedComponentDataBuffers();
	component = GetEntity()->FindComponent( component->GetGUID() );

	// Deselect all
	DeselectAllObjects();

	// Move to spawn position
	wxPoint graphPoint = ClientToCanvas( m_lastClickPoint );
	component->SetGraphPosition( graphPoint.x, graphPoint.y );

	// Update creation, select component
	UpdateComponent( component, 0 ,0 );

	if ( m_undoManager )
	{
		CUndoCreateDestroy::CreateStep( m_undoManager, component, true, false );
		CUndoCreateDestroy::FinishStep( m_undoManager );
	}

	SEvents::GetInstance().DispatchEvent( CNAME( PreviewEntityModified ), CreateEventData( m_template ) );
}

void CEdEntityGraphEditor::OnSpawnAttachment( wxCommandEvent& event )
{
	ASSERT( m_srcComponent );
	ASSERT( m_destComponent );

	// dex_fix: Removed CCharacterComponent
/*    if ( m_destComponent->IsA< CCharacterComponent >() )
    {
        if ( ::wxMessageBox( TXT("You are trying to set a CCharacterComponent as a child of other component.\nProbably the link should go the other way round.\n\nContinue?"),
                             TXT("Unusual child of a link"), wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION, this) == wxNO )
            return;
    }*/

	// Get info
	CClass* attachmentClass = m_attachmentClasses[ event.GetId() - ID_RANGE_COUNT ];
	ASSERT( attachmentClass );

	// Create attachment
	IAttachment* attachment = m_srcComponent->Attach( m_destComponent, attachmentClass );
	if ( attachment )
	{
		// Update appearance component
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( GetEntity() );
		if ( appearanceComponent != nullptr )
		{
			appearanceComponent->UpdateCurrentAppearanceAttachments();
		}

		// Select
		DeselectAllObjects();
		SelectObject( attachment, true );

		// Redraw
		Repaint();

		if ( m_undoManager )
		{
			CUndoCreateDestroy::CreateStep( m_undoManager, attachment, true );
			CUndoCreateDestroy::FinishStep( m_undoManager );
		}

		OnAttachmentCreated( attachment );
		SEvents::GetInstance().DispatchEvent( CNAME( PreviewEntityModified ), CreateEventData( m_template ) );
	}
}

void CEdEntityGraphEditor::OnToggleShowIncludes( wxCommandEvent& event )
{
    DeselectAllObjects();

	m_showIncludes = !m_showIncludes;

    // Send selection event
    if ( m_hook )
        m_hook->OnGraphSelectionChanged();
    
    ForceLayoutUpdate();

    Refresh();
}

void CEdEntityGraphEditor::OnHideSelection( wxCommandEvent& event )
{
	TDynArray< CComponent* > components;
	for ( THandle< CObject >& handle : m_selected )
	{
		if ( handle.IsValid() )
		{
			CObject* obj = handle.Get();
			if ( obj->IsA< CComponent >() )
			{
				components.PushBack( static_cast< CComponent* >( obj ) );
			}
		}
	}
	DeselectAllObjects();
	HideComponents( components );
}

void CEdEntityGraphEditor::OnIsolateSelection( wxCommandEvent& event )
{
	TDynArray< CComponent* > components;
	for ( THandle< CObject >& handle : m_selected )
	{
		if ( handle.IsValid() )
		{
			CObject* obj = handle.Get();
			if ( obj->IsA< CComponent >() )
			{
				components.PushBack( static_cast< CComponent* >( obj ) );
			}
		}
	}
	DeselectAllObjects();
	IsolateComponents( components );
}

void CEdEntityGraphEditor::OnUnhideHiddenComponents( wxCommandEvent& event )
{
	UnhideComponents();
}

void CEdEntityGraphEditor::OnEditCopy( wxCommandEvent& event )
{
	CopySelection();
}

void CEdEntityGraphEditor::OnConvertMesh( wxCommandEvent& event )
{
	// quick return
	if ( m_selected.Empty() ) return;

	CMeshComponent* oldMeshComponent = nullptr;
	CEntity* entity = nullptr;

	const Uint32 size = m_selected.Size();

	for ( Uint32 i=0; i<size; ++i )
	{
		if ( oldMeshComponent = Cast< CMeshComponent >( m_selected[i].Get() ) )
		{
			TDynArray< CComponent* > attachedParents;
			TDynArray< CClass* > attachmentClass;
			TDynArray< CName > parentSlots;
			CMeshComponent* newMeshComponent = nullptr;
			entity = oldMeshComponent->GetEntity();

			if ( !oldMeshComponent->GetParentAttachments().Empty() )
			{
				IAttachment* attachment;
				TList< IAttachment* >::const_iterator it = oldMeshComponent->GetParentAttachments().Begin();
				for ( ; it != oldMeshComponent->GetParentAttachments().End(); ++it )
				{
					attachment = *it;
					if ( attachment )
					{
						CComponent* parentComponent = Cast< CComponent >( attachment->GetParent() );
						if ( parentComponent )
						{
							attachedParents.PushBack( parentComponent );
							attachmentClass.PushBack( attachment->GetClass() );
							if ( attachment->IsA< CHardAttachment >() )
							{
								CHardAttachment* ha = Cast< CHardAttachment >( attachment );
								parentSlots.PushBack( ha->GetParentSlotName() );
							}
						}
					}
				}
			}

			SComponentSpawnInfo spawnInfo;
			spawnInfo.m_spawnPosition = oldMeshComponent->GetPosition();
			spawnInfo.m_spawnRotation = oldMeshComponent->GetRotation();

			// use previos casted component instead of casting twice
			if ( oldMeshComponent )
			{
				// cache drawable flags from ui
				const Uint32 drawableFlags = oldMeshComponent->GetDrawableFlags();
			
				// cache light channel flags
				const Uint8 lightChannelFlags = oldMeshComponent->GetLightChannels();
				const String& oldComponentName = oldMeshComponent->GetName();

				// cache tags on conversion
				const TagList& oldTags = oldMeshComponent->GetTags();

				if ( event.GetId() == ID_CONVERT_FUR_COMPONENT )
				{
					// convert to fur component
					CFurComponent* component = Cast< CFurComponent > ( entity->CreateComponent( CFurComponent::GetStaticClass(), spawnInfo ) );
					newMeshComponent = component;
				}

				if ( event.GetId() == ID_CONVERT_MESH_COMPONENT )
				{
					// convert to mesh component
					CMeshComponent* component = Cast< CMeshComponent > ( entity->CreateComponent( CMeshComponent::GetStaticClass(), spawnInfo ) );
					newMeshComponent = component;
				}
				if ( event.GetId() == ID_CONVERT_STATIC_MESH_COMPONENT )
				{
					// convert to static mesh component
					CStaticMeshComponent* component = Cast< CStaticMeshComponent > ( entity->CreateComponent( CStaticMeshComponent::GetStaticClass(), spawnInfo ) );
					newMeshComponent = component;
				}
				else if ( event.GetId() == ID_CONVERT_RIGID_MESH_COMPONENT_DYNAMIC )
				{
					// convert to rigid mesh component with motion type dynamic
					CRigidMeshComponent* component = Cast< CRigidMeshComponent > ( entity->CreateComponent( CRigidMeshComponent::GetStaticClass(), spawnInfo ) );
					newMeshComponent = component;
				}
				else if ( event.GetId() == ID_CONVERT_RIGID_MESH_COMPONENT_KINEMATIC )
				{
					// convert to rigid mesh component with motion type kinematic
					CRigidMeshComponent* component = Cast< CRigidMeshComponent > ( entity->CreateComponent( CRigidMeshComponent::GetStaticClass(), spawnInfo ) );
					newMeshComponent = component;
					SetPropertyValue( newMeshComponent, TXT("motionType"), MT_KeyFramed, false );
				}
				// restore drawable flags
				newMeshComponent->SetDrawableFlags( drawableFlags );

				// restore light channels flags
				newMeshComponent->SetLightChannels( lightChannelFlags );

				// restore old name
				newMeshComponent->SetName( oldComponentName );

				// restoring tags
				newMeshComponent->SetTags( oldTags );
			}

			newMeshComponent->SetAsCloneOf( oldMeshComponent );
			entity->DestroyComponent( oldMeshComponent );

			// re-attach to parents
			const Uint32 n = attachedParents.Size();
			for ( Uint32 i=0, k=0; i < n; ++i )
			{
				IAttachment* attachment = attachedParents[ i ]->Attach( newMeshComponent, attachmentClass[ i ] ); 
				if ( attachment && attachment->IsA< CHardAttachment >() )
				{
					CHardAttachment* ha = Cast< CHardAttachment >( attachment );
					ha->SetupSlot( attachedParents[ i ], parentSlots[ k++ ] );
				}
			}
		}
	}

	if ( entity )
	{
		entity->ForceUpdateTransformNodeAndCommitChanges();
	}

	ForceLayoutUpdate();
	Refresh();
}

void CEdEntityGraphEditor::OnConvertMovingAgent( wxCommandEvent& event )
{
	if ( ! m_activeItem || ! m_activeItem->IsA< CAnimatedComponent >() )
	{
		return;
	}

	CAnimatedComponent* oldAnimComponent = Cast< CAnimatedComponent >( m_activeItem );
	CAnimatedComponent* newAnimComponent = NULL;
	CEntity* entity = oldAnimComponent->GetEntity();

	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = oldAnimComponent->GetName();
	spawnInfo.m_spawnPosition = oldAnimComponent->GetPosition();
	spawnInfo.m_spawnRotation = oldAnimComponent->GetRotation();
	if ( oldAnimComponent->IsExactlyA<CMovingAgentComponent>() )
	{
		newAnimComponent = Cast<CAnimatedComponent>( entity->CreateComponent( CMovingPhysicalAgentComponent::GetStaticClass(), spawnInfo ) );
	}
	else
	{
		newAnimComponent = Cast<CAnimatedComponent>( entity->CreateComponent( CMovingAgentComponent::GetStaticClass(), spawnInfo ) );
	}

	newAnimComponent->SetName( oldAnimComponent->GetName() );
	newAnimComponent->SetAsCloneOf( oldAnimComponent );

	if ( !oldAnimComponent->GetParentAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > parents = oldAnimComponent->GetParentAttachments();
		TList< IAttachment* >::iterator it = parents.Begin();
		for ( ; it != parents.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetParent() )
			{
				CComponent * parent = SafeCast< CComponent >( attachment->GetParent() );
				attachment->Break();
				attachment->Init( parent, newAnimComponent, NULL );
			}
		}
	}
	if ( !oldAnimComponent->GetChildAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > children = oldAnimComponent->GetChildAttachments();
		TList< IAttachment* >::iterator it = children.Begin();
		for ( ; it != children.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetChild() )
			{
				CComponent * child = SafeCast< CComponent >( attachment->GetChild() );
				attachment->Break();
				attachment->Init( newAnimComponent, child, NULL );
				attachment->SetParent( newAnimComponent );
			}
		}
	}

	m_activeItem = NULL;
	entity->DestroyComponent( oldAnimComponent );

	CActor* actor = Cast< CActor > ( entity );
	if ( actor )
	{
		// Call this to update cached pointer to root animated component
		actor->FindMovingAgentComponent();
	}

	entity->ForceUpdateTransformNodeAndCommitChanges();
	ForceLayoutUpdate();
	Refresh();
}

void CEdEntityGraphEditor::OnConvertAnimComponentToSlotComponent( wxCommandEvent& event )
{
	if ( ! m_activeItem || ! m_activeItem->IsA< CAnimatedComponent >() )
	{
		return;
	}

	CAnimatedComponent* oldAnimComponent = Cast< CAnimatedComponent >( m_activeItem );
	CSlotComponent* newSlotComponent = NULL;
	CEntity* entity = oldAnimComponent->GetEntity();

	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = oldAnimComponent->GetName();
	spawnInfo.m_spawnPosition = oldAnimComponent->GetPosition();
	spawnInfo.m_spawnRotation = oldAnimComponent->GetRotation();

	newSlotComponent = Cast<CSlotComponent>( entity->CreateComponent( CSlotComponent::GetStaticClass(), spawnInfo ) );

	newSlotComponent->BaseOn( oldAnimComponent );

	if ( !oldAnimComponent->GetParentAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > parents = oldAnimComponent->GetParentAttachments();
		TList< IAttachment* >::iterator it = parents.Begin();
		for ( ; it != parents.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetParent() )
			{
				CComponent * parent = SafeCast< CComponent >( attachment->GetParent() );
				attachment->Break();
				attachment->Init( parent, newSlotComponent, NULL );
			}
		}
	}
	if ( !oldAnimComponent->GetChildAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > children = oldAnimComponent->GetChildAttachments();
		TList< IAttachment* >::iterator it = children.Begin();
		for ( ; it != children.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetChild() )
			{
				CComponent * child = SafeCast< CComponent >( attachment->GetChild() );
				attachment->Break();
				attachment->Init( newSlotComponent, child, NULL );
				attachment->SetParent( newSlotComponent );
			}
		}
	}

	m_activeItem = NULL;
	entity->DestroyComponent( oldAnimComponent );
	entity->ForceUpdateTransformNodeAndCommitChanges();
	ForceLayoutUpdate();
	Refresh();
}

void CEdEntityGraphEditor::OnConvertSlotComponentToAnimComponent( wxCommandEvent& event )
{
	if ( ! m_activeItem || ! m_activeItem->IsA< CSlotComponent >() )
	{
		return;
	}

	CSlotComponent* oldSlotComponent = Cast< CSlotComponent >( m_activeItem );
	CAnimatedComponent* newAnimComponent = NULL;
	CEntity* entity = oldSlotComponent->GetEntity();

	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = oldSlotComponent->GetName();
	spawnInfo.m_spawnPosition = oldSlotComponent->GetPosition();
	spawnInfo.m_spawnRotation = oldSlotComponent->GetRotation();

	newAnimComponent = Cast<CAnimatedComponent>( entity->CreateComponent( CAnimatedComponent::GetStaticClass(), spawnInfo ) );

	newAnimComponent->BaseOnSlotComponent( oldSlotComponent );
	newAnimComponent->SetGUID( oldSlotComponent->GetGUID() );

	if ( !oldSlotComponent->GetParentAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > parents = oldSlotComponent->GetParentAttachments();
		TList< IAttachment* >::iterator it = parents.Begin();
		for ( ; it != parents.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetParent() )
			{
				CComponent * parent = SafeCast< CComponent >( attachment->GetParent() );
				attachment->Break();
				attachment->Init( parent, newAnimComponent, NULL );
			}
		}
	}
	if ( !oldSlotComponent->GetChildAttachments().Empty() )
	{
		IAttachment* attachment;
		TList< IAttachment* > children = oldSlotComponent->GetChildAttachments();
		TList< IAttachment* >::iterator it = children.Begin();
		for ( ; it != children.End(); ++it )
		{
			attachment = *it;
			if ( attachment && attachment->GetChild() )
			{
				CComponent * child = SafeCast< CComponent >( attachment->GetChild() );
				attachment->Break();
				attachment->Init( newAnimComponent, child, NULL );
				attachment->SetParent( newAnimComponent );
			}
		}
	}

	m_activeItem = NULL;
	entity->DestroyComponent( oldSlotComponent );
	entity->ForceUpdateTransformNodeAndCommitChanges();
	ForceLayoutUpdate();
	Refresh();
}

void CEdEntityGraphEditor::OnConvertToSlot( wxCommandEvent& event )
{
	// Special case for slot component
	CSlotComponent* slots = Cast< CSlotComponent >( m_activeItem );
	if ( slots )
	{
		// Check slots, cannot duplicate
		Bool hasDuplicates = false;
		const TDynArray< SSlotInfo >& realSlots = slots->GetSlots();
		for ( Uint32 i=0; i<realSlots.Size(); ++i )
		{
			const SSlotInfo& realSlotInfo = realSlots[i];
			const CName slotName( realSlotInfo.m_slotName );
			if ( m_template->FindSlotByName( slotName, true ) && !m_template->FindSlotByName( slotName, true ) )
			{
				WARN_EDITOR( TXT("Slot '%s' is already defined in included template."), slotName.AsString().AsChar() );
				hasDuplicates = true;
			}
		}

		// We have some duplicated slots
		if ( hasDuplicates )
		{
			if ( wxNO == wxMessageBox( wxT("Some slots are already defined in some includes, they should not be redefine here. Are you sure ?"), wxT("Convert component to slot"), wxYES_NO | wxICON_QUESTION ) )
			{
				return;
			}
		}

		// Get the parent attachment
		CName parentComponentName;
		if ( slots->GetTransformParent() )
		{
			CComponent* parentComponent = Cast< CComponent >( slots->GetTransformParent()->GetParent() );
			if ( parentComponent )
			{
				parentComponentName = CName( parentComponent->GetName() );
			}
		}

		// Checkout
		if ( m_template->MarkModified() )
		{
			// Convert slots
			Bool failedToAddSlot = false;
			for ( Uint32 i=0; i<realSlots.Size(); ++i )
			{
				const SSlotInfo& realSlotInfo = realSlots[i];
				const CName slotName( realSlotInfo.m_slotName );

				// Initialize slot info
				EntitySlotInitInfo slotInfo;
				slotInfo.m_componentName = parentComponentName;
				slotInfo.m_boneName = realSlotInfo.m_parentSlotName;
				slotInfo.m_transform = EngineTransform( realSlotInfo.m_relativePosition, realSlotInfo.m_relativeRotation );

				// Create slot
				const Bool state = m_template->AddSlot( slotName, &slotInfo );

				// Added ?
				if ( !state )
				{
					failedToAddSlot = true;
					continue;
				}
			}

			// Failed to add all slots
			if ( failedToAddSlot )
			{
				wxMessageBox( wxT("Failed to add some slots!"), wxT("Convert component to slot"), wxOK | wxICON_ERROR );
				return;
			}

			// Delete the component
			m_activeItem = NULL;
			slots->Destroy();

			// Redraw the slot list
			m_editor->UpdateSlotList();

			// Redraw
			Refresh();
		}

		// Done
		return;
	}

	// Get selected component
	CComponent* component = Cast< CComponent >( m_activeItem );
	if ( component )
	{
		// Find slot named like that
		const CName slotName( component->GetName() );
		if ( m_template->FindSlotByName( slotName, true ) && !m_template->FindSlotByName( slotName, true ) )
		{
			if ( wxNO == wxMessageBox( wxT("Slot with that name is already defined in some includes, it should not be redefine here. Are you sure ?"), wxT("Convert component to slot"), wxYES_NO | wxICON_QUESTION ) )
			{
				return;
			}
		}

		// Get attachment
		CHardAttachment* attachment = component->GetTransformParent();
		if ( !attachment )
		{
			if ( wxNO == wxMessageBox( wxT("Component is not attached using hard attachment, are you sure you want to convert this component to slot ?"), wxT("Convert component to slot"), wxYES_NO | wxICON_QUESTION ) )
			{
				return;
			}
		}

		// Checkout
		if ( m_template->MarkModified() )
		{
			// Initialize slot info
			EntitySlotInitInfo slotInfo;

			// Copy shit from attachment
			if ( attachment )
			{
				// Copy slot information from attachment
				slotInfo.m_freeRotation = 0 != ( attachment->GetAttachmentFlags() & HAF_FreeRotation );
				slotInfo.m_freePositionAxisX = 0 != ( attachment->GetAttachmentFlags() & HAF_FreePositionAxisX );
				slotInfo.m_freePositionAxisY = 0 != ( attachment->GetAttachmentFlags() & HAF_FreePositionAxisY );
				slotInfo.m_freePositionAxisZ = 0 != ( attachment->GetAttachmentFlags() & HAF_FreePositionAxisZ );
				slotInfo.m_componentName = CName( attachment->GetParent()->GetName().AsChar() );
				slotInfo.m_boneName = attachment->GetParentSlotName();
				slotInfo.m_transform = attachment->GetRelativeTransform();
			}
			else
			{
				// Simple slot, not attached to anything
				slotInfo.m_transform = component->GetLocalToWorld();
			}
		
			// Create slot
			const Bool state = m_template->AddSlot( slotName, &slotInfo );

			// Added ?
			if ( !state )
			{
				wxMessageBox( wxT("Slot could not be created!"), wxT("Convert component to slot"), wxOK | wxICON_ERROR );
				return;
			}

			// Delete the component
			m_activeItem = NULL;
			component->Destroy();

			// Redraw the slot list
			m_editor->UpdateSlotList( slotName );

			// Redraw
			Refresh();
		}

		// Done
		return;
	}
}

void CEdEntityGraphEditor::OnEditComponentRelatedEffect( wxCommandEvent& event )
{
	wxEffectDataWrapper* data = static_cast< wxEffectDataWrapper* >( event.GetEventObject() );
	if ( data )
	{
		CFXDefinition* fx = data->m_effect.Get();
		if ( fx )
		{
			
		}
	}
}

void CEdEntityGraphEditor::OnGenerateNavmesh( wxCommandEvent& event )
{
	CNavmeshComponent* navmeshComponent = Cast< CNavmeshComponent >( m_activeItem );
	if ( !navmeshComponent )
	{
		return;
	}
	struct InputIteratorAttachments : public CNavmeshComponent::InputIterator
	{
		InputIteratorAttachments( const TList< IAttachment* >& attachments )
			: m_it( attachments.Begin() )
			, m_end( attachments.End() )
		{}
		void Next() override
		{
			++m_it;
		}
		CNode* Get() override
		{
			for ( ; m_it != m_end; ++m_it )
			{
				CNavmeshInputAttachment* attachment = Cast< CNavmeshInputAttachment >( *m_it );
				if ( attachment )
				{
					return attachment->GetParent();
				}
			}
			return NULL;
		}
		TList< IAttachment* >::const_iterator		m_it;
		TList< IAttachment* >::const_iterator		m_end;
	} inputAttachments( navmeshComponent->GetParentAttachments() );
	struct InputIteratorComponents : public CNavmeshComponent::InputIterator
	{
		InputIteratorComponents( CEntity* entity )
			: m_it( entity->GetComponents().Begin() )
			, m_end( entity->GetComponents().End() )
		{}
		void Next() override
		{
			++m_it;
		}
		CNode* Get() override
		{
			return ( m_it == m_end ) ? NULL : *m_it;
		}
		TDynArray< CComponent* >::const_iterator	m_it;
		TDynArray< CComponent* >::const_iterator	m_end;
	} inputComponents( navmeshComponent->GetEntity() );

	Bool isUsingAttachments = false;
	for ( auto it = navmeshComponent->GetParentAttachments().Begin(), end = navmeshComponent->GetParentAttachments().End(); it != end; ++it )
	{
		if ( ( *it )->IsA< CNavmeshInputAttachment >() )
		{
			isUsingAttachments = true;
			break;
		}
	}

	CNavmeshComponent::InputIterator* input;
	if ( isUsingAttachments )
	{
		input = new InputIteratorAttachments( navmeshComponent->GetParentAttachments() );
	}
	else
	{
		input = new InputIteratorComponents( navmeshComponent->GetEntity() );
	}

	navmeshComponent->GenerateNavmesh( input, true );
}

void CEdEntityGraphEditor::OnToggleNavmeshEdition( wxCommandEvent& event )
{
	CEdEntityPreviewPanel* previewPanel = m_editor->GetPreviewPanel();
	previewPanel->SetNavmeshEditorActive( !previewPanel->IsNavmeshEditorActive() );
}

void CEdEntityGraphEditor::OnRemoveStreamingData( wxCommandEvent& event )
{
	// Confirm
	if ( !YesNo( TXT("Are you sure? Any streaming data will be lost.\r\nIMPORTANT: This is *NOT* undoable!") ) )
	{
		return;
	}

	// Remove all the data
	GetEntity()->RemoveAllStreamingData();
	m_template->GetEntityObject()->RemoveAllStreamingData();
	Refresh();
}

void CEdEntityGraphEditor::OnResetEntityData( wxCommandEvent& event )
{
	// Confirm
	if ( !YesNo( TXT("Are you sure? All streaming data will be lost.\r\nIMPORTANT: This is *NOT* undoable!") ) )
	{
		return;
	}

	// Remove all the data
	m_template->ResetEntityTemplateData();
	Refresh();
}

void CEdEntityGraphEditor::OnInspectObject( wxCommandEvent& event )
{
	if ( m_activeItem != nullptr )
	{
		InspectObject( m_activeItem );
	}
}

void CEdEntityGraphEditor::OnEditCut( wxCommandEvent& event )
{
	CutSelection();
}

void CEdEntityGraphEditor::OnEditPaste( wxCommandEvent& event )
{
	PasteSelection();
}

void CEdEntityGraphEditor::OnChangeComponentClass( wxCommandEvent& event )
{
	if ( m_selected.Size() != 1 )
	{
		return;
	}
	CComponent* component = Cast< CComponent >( m_activeItem );
	if ( !component )
	{
		return;
	}

	if ( !m_componentClassHierarchyInitialized )
	{
		CClassHierarchyMapper::MapHierarchy( CComponent::GetStaticClass(), m_componentClassHierarchy );
	}

	CEdMappedClassSelectorDialog selector( this, m_componentClassHierarchy, TXT(""), CComponent::GetStaticClass(), component->GetClass() );
	if ( CClass* newClass = selector.Execute() )
	{
		if ( m_selected.Size() != 1 )
		{
			return;
		}

		CComponent* component = Cast< CComponent >( m_selected[ 0 ].Get() );
		if ( !component )
		{
			return;
		}

		DeselectAllObjects();

		// Snapshot properties
		CScriptSnapshot snapshot;
		CScriptSnapshot::ScriptableSnapshot* snap = snapshot.BuildEditorObjectSnapshot( component );
	
		// Create new component object
		CComponent* newComponent = CreateObject< CComponent >( newClass );

		// Restore properties
		snapshot.RestoreEditorObjectSnapshot( newComponent, snap );
		delete snap;

		Matrix localToWorld;
		component->GetLocalToWorld( localToWorld );
		RemoveFromLayout( component );
		component->Destroy();
		AddComponent( newComponent, localToWorld );

		SEvents::GetInstance().DispatchEvent( CNAME( PreviewEntityModified ), CreateEventData( m_template ) );
	}
}

CDirectory* DoResourceSaveDialog( const CFileFormat& format, String& outFileName )
{
	CEdFileDialog dialog;
	dialog.AddFormat( format );

	if ( !dialog.DoSave( static_cast< ::HWND >( wxTheFrame->GetHandle() ) ) )
	{
		return nullptr;
	}

	String ext = TXT(".") + format.GetExtension();
	String path( dialog.GetFile() );
	if( ! path.EndsWith( ext ) )
	{
		path += ext;
	}

	size_t pos; 
	path.FindSubstring( GDepot->GetRootDataPath(), pos );
	if( pos != 0 )
	{
		wxMessageBox( TXT( "You can create a resource only inside depot" ) );
		return nullptr;
	}

	String relativePath( path.RightString( path.Size() - GDepot->GetRootDataPath().Size() ) );
	String relativeDir( relativePath.StringBefore( TXT( "\\" ), true ) );
	outFileName = relativeDir.Empty() ? relativePath : relativePath.StringAfter( TXT( "\\" ), true );

	if( ! relativeDir.Empty() )
	{
		relativeDir += TXT( "\\" );
	}

	CDirectory* dir = GDepot->FindPath( relativeDir.AsChar() );
	ASSERT( dir != nullptr );

	return dir;
}

void CEdEntityGraphEditor::OnMergeMesh( wxCommandEvent& event )
{
	if ( m_selected.Size() <= 1 )
	{
		return;
	}

	CSelectionManager* selMan = GetEntity()->GetLayer()->GetWorld()->GetSelectionManager();
	Vector pivotPos = selMan->GetPivotPosition();

	String fileName;
	CDirectory* dir = DoResourceSaveDialog( CFileFormat( TXT( "w2mesh" ), TXT( "Mesh resource" ) ), fileName );

	if ( !dir )
	{
		return;
	}


	CMesh* newMesh = CreateObject< CMesh >();

	const Uint32 size = m_selected.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if( CRigidMeshComponent* rigidMeshComp = Cast< CRigidMeshComponent >( m_selected[i].Get() ) )
		{
			continue;
		}
		if( CMeshComponent* meshComp = Cast< CMeshComponent >( m_selected[i].Get() ) )
		{
			Matrix m = meshComp->GetLocalToWorld();
			m.SetTranslation( m.GetTranslation() - pivotPos );
			CMesh* mesh = meshComp->GetMeshNow();
			newMesh->AddMesh( *mesh, m );
		}
	}

	// remove skinning info from all chunks
	newMesh->RemoveSkinning();

	if ( !newMesh->SaveAs( dir, fileName ) )
	{
		GFeedback->ShowError( TXT("Unable to save mesh") );
		return;
	}

	for ( Uint32 i=0; i<size; ++i )
	{
		if( CRigidMeshComponent* rigidMeshComp = Cast< CRigidMeshComponent >( m_selected[i].Get() ) )
		{
			continue;
		}
		if ( CMeshComponent* meshComp = Cast< CMeshComponent >( m_selected[i].Get() ) )
		{
			RemoveFromLayout( meshComp );
			meshComp->Destroy();
		}
	}

	CComponent* meshComp = CreateObject< CMeshComponent >();
	meshComp->SetResource( newMesh );

	Matrix m = Matrix::IDENTITY;
	m.SetTranslation( pivotPos );
	AddComponent( meshComp, m );
}

void CEdEntityGraphEditor::GetUniqueOverriddenPropertiesForSelection( TDynArray< CName >& propertyNames )
{
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		CComponent* component = Cast< CComponent >( (*it).Get() );
		if ( component )
		{
			TDynArray< CName > propertiesForThisComponent;
			m_template->GetOverridenPropertiesForComponent( component, propertiesForThisComponent );
			propertyNames.PushBackUnique( propertiesForThisComponent );
		}
	}
}

void CEdEntityGraphEditor::OnShowComponentOverrides( wxCommandEvent& event )
{
	// Get unique properties for the selection
	TDynArray< CName > overriddenProperties;
	GetUniqueOverriddenPropertiesForSelection( overriddenProperties );

	// Display dialog box with them
	String overridesList = CEdFormattedDialog::ArrayToList( overriddenProperties, []( const CName& name ) { return name.AsString(); } );
	String code = TXT("'Overridden Properties:'L") + overridesList + TXT("|H{~B@'&OK'~}");
	Int32 unused = 0;
	FormattedDialogBox( wxT("Overrides"), code.AsChar(), &unused );
}

void CEdEntityGraphEditor::OnResetOverriddenProperties( wxCommandEvent& event )
{
	// Get unique properties for the selection
	TDynArray< CName > overriddenProperties;
	GetUniqueOverriddenPropertiesForSelection( overriddenProperties );

	// Display dialog box with them
	TDynArray< Bool > resetPropertyCheck;
	resetPropertyCheck.Grow( overriddenProperties.Size() );
	String overridesList = CEdFormattedDialog::ArrayToList( overriddenProperties, []( const CName& name ) { return name.AsString(); } );
	String code = TXT("H{V{H{'Overridden Properties:'|||||||}M") + overridesList + TXT("=160}|V{B@'&Reset Checked'|B'Reset All';~B'&Cancel';}}");
	Int32 button = FormattedDialogBox( wxT("Overrides"), code.AsChar(), resetPropertyCheck.TypedData() );

	// Cancel if button 0 or 1 was not clicked
	if ( button != 0 && button != 1 )
	{
		return;
	}

	// "Check" everything if the Reset All button was clicked
	if ( button == 1 )
	{
		// "Check" everything
		for ( Uint32 i=0; i < resetPropertyCheck.Size(); ++i )
		{
			resetPropertyCheck[i] = true;
		}
	}

	// If everything is checked, show a confirmation box
	Uint32 resetCount = 0;
	for ( Uint32 i=0; i < resetPropertyCheck.Size(); ++i )
	{
		resetCount += resetPropertyCheck[i] ? 1 : 0;
	}
	if ( resetCount == resetPropertyCheck.Size() && wxMessageBox( wxT("Are you sure that you want to reset ALL property overrides?"), wxT("Reset Overrides"), wxYES_NO|wxICON_WARNING|wxCENTRE ) != wxYES )
	{
		return;
	}

	// Build name list from selection
	TDynArray< CName > propertiesToReset;
	for ( Uint32 i=0; i < overriddenProperties.Size(); ++i )
	{
		if ( resetPropertyCheck[i] )
		{
			propertiesToReset.PushBack( overriddenProperties[i] );
		}
	}

	// Remove overrides from the selection and reset their values
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		CComponent* component = Cast< CComponent >( (*it).Get() );
		if ( component )
		{
			m_template->RemoveOverrideForComponentProperties( component, propertiesToReset, true );
		}
	}

	// Update the editor
	RefreshLater( this );
}

wxDragResult CEdEntityGraphEditor::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
	const TDynArray< CResource* >& resources = GetDraggedResources();

	for ( Uint32 i = 0; i < resources.Size(); ++i )
	{
		if ( CResource* resource = Cast< CResource>( resources[ i ] ) )
		{
			// override implicit Move with implicit Copy ( we want to show '+' by default here )
			return wxGetKeyState( WXK_SHIFT ) ? wxDragMove : wxDragCopy;
		}
	}
	return wxDragNone;
}

// This funcion tries to generate a suffix for the passed name if there are other
// components with the same name in the passed entity
static void DuplicateAvoidalSuffixGenerator( String& name, CEntity* entity )
{
	String originalName = name;
	Uint32 attempt = 1;

	while ( entity->FindComponent( name, false ) )
	{
		name = String::Printf( TXT("%s %d"), originalName.AsChar(), ++attempt );
	}
}

Bool CEdEntityGraphEditor::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	if ( resources.Empty() ) return false;

	// Deselect all
	DeselectAllObjects();
	Bool res = false;
	wxPoint graphPoint = ClientToCanvas( wxPoint(x, y) );

	Uint32 resCount = resources.Size();

	// empty component
	CComponent* component = nullptr;

	if	( resCount == 2 )
	{
		// handle canimatedcomponent case skeleton and ragdoll
		Bool part1 = resources[0]->IsA(ClassID<CSkeleton>()) && resources[1]->IsA(ClassID<CRagdoll>());
		Bool part2 = resources[0]->IsA(ClassID<CRagdoll>()) && resources[1]->IsA(ClassID<CSkeleton>());
		if ( part1 || part2 )
		{
			// we have canimatedcomponent so set resource. otherwise create and set
			component = CreateComponent( resources[0], ClassID<CAnimatedComponent>() );
			component->SetResource( resources[1] );
			// Update creation, select component
			UpdateComponent( component, graphPoint.x, graphPoint.y );
			graphPoint.y += 40;
			SetFocus();
			return true;
		}
	}

	for ( Uint32 i = 0; i < resCount; ++i )
	{
		// clean component
		component = nullptr;

		CResource* resource = resources[i];
		// Dropped a mesh resource
		if ( CMesh *mesh = Cast<CMesh>( resource ) )
		{
			Int32 meshTypeSelection = 0;
			Int32 result = FormattedDialogBox( this, wxT("Create Mesh Component"), wxT("R('Mesh' 'Static Mesh' 'Static Mesh (PLC_Disabled)' 'Rigid Mesh Kinematic' 'Rigid Mesh Dynamic')|H{B@'OK' B'Cancel'}"), &meshTypeSelection );
			if ( result == -1 || result == 1 ) // X or Cancel
			{
				return false;
			}
			switch ( meshTypeSelection )
			{
			case 0: // mesh
				component = CreateComponent( mesh, ClassID<CMeshComponent>() );
				break;
			case 1: // static mesh
				component = CreateComponent( mesh, ClassID<CStaticMeshComponent>() );
				break;
			case 2: // static mesh with PLC_Disabled
				component = CreateComponent( mesh, ClassID<CStaticMeshComponent>() );
				SetPropertyValue( component, TXT("pathLibCollisionType"), PLC_Disabled, false );
				break;
			case 3: // rigid mesh kinematic
				component = CreateComponent( mesh, ClassID<CRigidMeshComponent>() );
				SetPropertyValue( component, TXT("motionType"), MT_KeyFramed, false );
				break;
			case 4: // rigid mesh dynamic
				component = CreateComponent( mesh, ClassID<CRigidMeshComponent>() );
				break;
			}
			res = true;
		}
#ifdef USE_APEX
		// Dropped an Apex Cloth Resource
		else if ( CApexClothResource* apexRes = Cast<CApexClothResource>( resource ) )
		{
			component = CreateComponent( apexRes, ClassID<CClothComponent>() );
			res = true;
		}
		// Dropped an Apex destruction Resource
		else if ( CApexDestructionResource* apexRes = Cast<CApexDestructionResource>( resource ))
		{
			component = CreateComponent( apexRes, ClassID<CDestructionSystemComponent>() );
			res = true;
		}
#endif
#ifdef USE_NVIDIA_FUR
		// Dropped a fur resource
		else if ( CFurMeshResource* fur = Cast<CFurMeshResource>( resource ) )
		{
			component = CreateComponent( fur, ClassID<CFurComponent>() );
			res = true;
		}
#endif
		// Dropped a skeleton resource
		else if ( CSkeleton* skeleton = Cast<CSkeleton>( resource ) )
		{
			component = CreateComponent( skeleton, ClassID<CAnimatedComponent>() );
			res = true;
		}
		// Dropped a ragdoll resource
		else if ( CRagdoll* ragdoll = Cast<CRagdoll>( resource ) )
		{
			component = CreateComponent( ragdoll, ClassID<CAnimatedComponent>() );
			res = true;
		}
		// Dropped a dyng resource
		else if ( CDyngResource* dyng = Cast<CDyngResource>( resource ) )
		{
			component = CreateComponent( dyng, ClassID<CAnimDangleComponent>() );
			res = true;
		}
		//update component
		UpdateComponent( component, graphPoint.x, graphPoint.y );
		graphPoint.y += 40;
	}
	SetFocus();
	return res;
}

Bool CEdEntityGraphEditor::IsAttachmentAllowed( const CComponent* parent, const CComponent* child, const CClass* attachmentClass ) const
{
	// static meshes shouldnt be skinned so disable ay attachement for static meshes
	if ( child->IsExactlyA< CStaticMeshComponent >() )
	{
		return false;
	}
	if ( child->IsA< CNavmeshComponent >() )
	{
		return attachmentClass == CNavmeshInputAttachment::GetStaticClass();
	}
	else if ( parent->IsA< CNormalBlendComponent >() )
	{
		return child->QueryRenderProxyInterface() && attachmentClass == CNormalBlendAttachment::GetStaticClass();
	}
	else if ( parent->IsA< CAnimatedComponent >() && child->IsA< CAnimatedComponent >() )
	{
		return attachmentClass == CAnimatedAttachment::GetStaticClass() || attachmentClass == CHardAttachment::GetStaticClass();
	}
	return true;
}

void CEdEntityGraphEditor::OnAttachmentCreated( IAttachment* attachment )
{
}

wxString CEdEntityGraphEditor::ConvertActorDialogLine( String& line ) const
{
	const Uint32 lineMaxSize = 45;

	if ( line.Size() > lineMaxSize )
	{
		String temp = line.LeftString( lineMaxSize - 3 ) + TXT("...");
		return temp.AsChar();
	}
	else
	{
		return line.AsChar();
	}
}

void CEdEntityGraphEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/EntityGraphEditor") );

	String showIncludesPath = m_template->GetFile()->GetFileName() + TXT("/ShowIncludes");
	config.Write( showIncludesPath, m_showIncludes );

	// instead of just write out the size of the array, check if items inside are NULL
	Int32 recentlyUsedBlockClassesCount = 0;
	for( Uint32 i = 0; i < m_recentlyUsedBlockClasses.Size(); ++i )
	{
		if( m_recentlyUsedBlockClasses[ i ] )
			recentlyUsedBlockClassesCount++;
	}

	config.Write( TXT( "RecentlyUsedComponentCount" ), recentlyUsedBlockClassesCount );

	for( Uint32 i = 0; i < m_recentlyUsedBlockClasses.Size(); ++i )
	{
		if( !m_recentlyUsedBlockClasses[ i ] )
			continue;

		String blockConfig = String::Printf( TXT( "RecentlyUsedComponent%u" ), i );
		config.Write( blockConfig.AsChar(), m_recentlyUsedBlockClasses[ i ]->GetName().AsString().AsChar() );
	}
}

void CEdEntityGraphEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/EntityGraphEditor") );

	String showIncludesPath = m_template->GetFile()->GetFileName() + TXT("/ShowIncludes");
	m_showIncludes = config.Read( showIncludesPath, 1 ) != 0;

	Uint32 size = config.Read( TXT( "RecentlyUsedComponentCount" ), 0 );
	ASSERT( size <= m_recentlyUsedBlockClasses.Capacity() );

	for( Uint32 i = 0; i < size; ++i )
	{
		String blockConfig = String::Printf( TXT( "RecentlyUsedComponent%u" ), i );

		String blockNameStr = config.Read( blockConfig.AsChar(), String::EMPTY );
		ASSERT( !blockNameStr.Empty() );

		CName blockName( blockNameStr );

		CClass* block = SRTTI::GetInstance().FindClass( blockName );
		ASSERT( block != NULL );

		m_recentlyUsedBlockClasses.PushBack( block );
	}
}

void CEdEntityGraphEditor::RemoveFromLayout( CComponent* component )
{
	m_layout.Erase( component );
	m_layoutObjects.Remove( component );
}

void CEdEntityGraphEditor::ClearLayout()
{
	m_layout.Clear();
	m_layoutObjects.Clear();
}

void CEdEntityGraphEditor::SelectComponents( const TDynArray< CComponent* >& components )
{
	DeselectAllObjects();
	for ( CComponent* component : components )
	{
		SelectObject( component, true );
	}
}

void CEdEntityGraphEditor::HideComponents( const TDynArray< CComponent* >& components )
{
	for ( CComponent* component : components )
	{
		m_hidden.Insert( component->GetName() );
	}
	RunLaterOnce( [&]{ ForceLayoutUpdate(); Refresh(); } );
}

void CEdEntityGraphEditor::IsolateComponents( const TDynArray< CComponent* >& components )
{
	// Collect components
	TDynArray< CComponent* > allComponents;
	CollectEntityComponents( GetEntity(), allComponents );

	for ( CComponent* component : allComponents )
	{
		if ( !components.Exist( component ) )
		{
			m_hidden.Insert( component->GetName() );
		}
	}

	RunLaterOnce( [&]{ ForceLayoutUpdate(); Refresh(); } );
}

void CEdEntityGraphEditor::UnhideComponents()
{
	m_hidden.Clear();
	RunLaterOnce( [&]{ ForceLayoutUpdate(); Refresh(); } );
}

void CEdEntityGraphEditor::DrawStatistic()
{
	if( GetEntity() != nullptr )
	{
		Uint32 streamedCount = 0;
		Uint32 notStreamedCount = 0;
		Uint32 duplicateResourcePaths = 0;
		Uint32 uniqueComponentsCountedByResourcePaths = 0;

		const Uint32 allComponentCount = GetEntity()->GetComponents().Size();

		TDynArray< String > componentResourcePaths;
		componentResourcePaths.Reserve( allComponentCount );

		if( GetEntity()->ShouldBeStreamed() )
		{
			for ( Uint32 i=0; i<allComponentCount; ++i )
			{
				CComponent* comp = GetEntity()->GetComponents()[i];
				if ( comp->IsStreamed() )
				{
					++streamedCount;
				}
				else
				{
					++notStreamedCount;
				}

				TDynArray< const CResource* > componentResources;
				comp->GetResource( componentResources );

				Bool duplicateResourceFound = false;

				for( Uint32 iResource = 0; iResource < componentResources.Size(); ++iResource )
				{
					if ( const CResource* resource = componentResources[ iResource ] )
					{
						String depotPath = resource->GetDepotPath();

						if( !componentResourcePaths.PushBackUnique( std::move( depotPath ) ) )
						{
							duplicateResourceFound = true;
						}
					}
				}

				if( duplicateResourceFound )
				{
					++duplicateResourcePaths;
				}
				else
				{
					++uniqueComponentsCountedByResourcePaths;
				}
			}
		}

		DrawText( ClientToCanvas( wxPoint( 5, 5 ) ), GetGdiBoldFont(), String::Printf( TXT("Streamed components count: %i"), streamedCount ), *wxWHITE );
		DrawText( ClientToCanvas( wxPoint( 5, 20 ) ), GetGdiBoldFont(), String::Printf( TXT("Not streamed components count: %i"), notStreamedCount ), *wxWHITE );
		DrawText( ClientToCanvas( wxPoint( 5, 35 ) ), GetGdiBoldFont(), String::Printf( TXT("All components count: %i"), allComponentCount ), *wxWHITE );
		DrawText( ClientToCanvas( wxPoint( 5, 50 ) ), GetGdiBoldFont(), String::Printf( TXT("Streaming distance: %i Saved: %i"), (int)GetEntity()->GetStreamingDistance(), (int)m_template->GetEntityObject()->GetStreamingDistance() ), *wxWHITE );
		DrawText( ClientToCanvas( wxPoint( 5, 65 ) ), GetGdiBoldFont(), String::Printf( TXT("Unique components (by resource path): %u"), uniqueComponentsCountedByResourcePaths ), *wxWHITE );
		DrawText( ClientToCanvas( wxPoint( 5, 80 ) ), GetGdiBoldFont(), String::Printf( TXT("Duplicate resources: %u (Unique: %u)"), duplicateResourcePaths, componentResourcePaths.Size() ), *wxWHITE );
	}
}
CComponent* CEdEntityGraphEditor::CreateComponent( CResource* r, CClass* c )
{
	SComponentSpawnInfo spawnInfo;
	CComponent* component = nullptr;
	if( r )
	{
		String fileName = r->GetFile()->GetFileName();
		size_t delimiterIndex;
		fileName.FindSubstring( TXT("."), delimiterIndex, true );
		fileName = fileName.LeftString( delimiterIndex );
		spawnInfo.m_name = fileName;
		DuplicateAvoidalSuffixGenerator( spawnInfo.m_name, GetEntity() );
		component = GetEntity()->CreateComponent( c, spawnInfo );
		component->SetResource( r );
	}
	else
	{
		component = GetEntity()->CreateComponent( c, spawnInfo );
	}
	return component;
}
