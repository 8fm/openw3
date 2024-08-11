/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "editorExternalResources.h"
#include "undoManager.h"
#include "undoCreate.h"
#include "undoSelection.h"
#include "undoGroupingObjects.h"
#include "encounterEditor.h"
#include "TemplateInsertion.h"
#include "jobTreeEditor.h"
#include "jobTreePreviewPanel.h"
#include "entityList.h"
#include "../../common/engine/envProbeComponent.h"
#include "../../common/engine/easyMeshBuilder.h"
#include "../../common/engine/reviewSystem.h"
#include "../../common/engine/characterEntityTemplate.h"
#include "../../common/engine/pathlibNavmeshComponent.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/game/actionAreaVertex.h"
#include "../../common/game/encounter.h"
#include "../../common/game/actionPointComponent.h"
#include "../../common/game/actionPoint.h"
#include "../../games/r4/r4Game.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h" //for grabbing envprobe faces
#include "../../common/engine/mesh.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/core/depot.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/streamingSectorData.h"
#include "toolsPanel.h"
#include "errorsListDlg.h"

#ifndef NO_RED_GUI
#include "../../common/engine/redGuiManager.h"
#ifndef NO_DEBUG_WINDOWS
#include "../../common/engine/debugWindowsManager.h"
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/physics/physicsWrapper.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/decalComponent.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/pathlibComponent.h"
#include "../../common/engine/rigidMeshComponent.h"
#include "nodeTransformManager.h"
#include "../../common/engine/fonts.h"
#include "../../common/engine/entityGroup.h"
#include "../../common/engine/worldIterators.h"
#include "markerTools.h"
#include "../../common/engine/helpTextComponent.h"
#include "..\..\common\engine\layersEntityCompatibilityChecker.h"
#include "worldEditPanel.h"
#include "screenshotEditor.h"
#include "../../common/engine/shadowMeshGenerator.h"
#include "undoTransform.h"

enum
{
	ID_EDIT_COPY_WP                     , 
	ID_EDIT_CUT_WP                      , 
	ID_EDIT_DELETE_WP                   , 
	ID_EDIT_PASTE_WP                    , 
	ID_EDIT_PASTE_ALIGNED_WP            , 
	ID_EDIT_ENTITY                      , 
	ID_SPAWN_RESOURCE                   , 
	ID_SPAWN_ENTITY_CLASS               , 
	ID_ATTACH_TO_BEHAVIOR               , 
	ID_ACTIVATE_LAYER                   , 
	ID_LOOK_AT_SELECTED                 , 
	ID_DETACH_TEMPLATES                 , 
	ID_REMOVE_COLLISIONS                , 
	ID_REBUILD_COLLISIONS               , 
	ID_REPLACE_BY_ENTITY                , 
	ID_GROUP_ITEMS                      , 
	ID_UNGROUP_ITEMS                    , 
	ID_REPLACE_BY_ENTITY_COPY_COMPONENTS, 
	ID_SHOW_IN_SCENE_EXPLORER           , 
	ID_CREATE_ENTITY_TEMPLATE           , 
	ID_CREATE_MESH_RESOURCE             , 
	ID_CREATE_SHADOW_MESH               , 
	ID_ALIGN_PIVOT                      , 
	ID_EXTRACT_MESHES_FROM_ENTITY       , 
	ID_EXTRACT_COMPONENTS_FROM_ENTITY	,
	ID_HIDE_SELECTED_ENTITIES			,
	ID_ISOLATE_SELECTED_ENTITIES		,
	ID_LIST_HIDDEN_ENTITIES				,
	ID_REVEAL_HIDDEN_ENTITIES			,
	ID_MESH_NAV_TO_STATIC_WALKABLE      , 
	ID_REPLACE_BY_MESH                  , 
	ID_REPLACE_BY_STATIC_MESH           , 
	ID_GEN_CUBEMAP                      , 
	ID_CONV_TO_RIGID_MESH               , 
	ID_NAVMESH_GENROOT_MOVE             , 
	ID_NAVMESH_GENROOT_ADD              , 
	ID_NAVMESH_GENROOT_DEL              , 
	ID_NAVMESH_GENERATE                 , 
	ID_OPEN_ENCOUNTER_EDITOR            , 
	ID_PREVIEW_AP_MAN		            , 
	ID_PREVIEW_AP_BIG_MAN	            , 
	ID_PREVIEW_AP_WOMAN		            , 
	ID_PREVIEW_AP_DWARF		            , 
	ID_PREVIEW_AP_CHILD		            , 
	ID_PREVIEW_AP_SELECTED	            , 
	ID_MESH_NAV_TO_STATIC				,
	ID_MESH_NAV_TO_WALKABLE				,
	ID_MESH_NAV_TO_DYNAMIC              , 
	ID_LOCK_GROUP						,
	ID_UNLOCK_GROUP						,
	ID_REMOVE_FROM_GROUP				,
	ID_IMPORT_ENTITIES_FROM_OLD_TILES	,
    ID_CHANGE_APPEARANCE                ,
	ID_FORCE_STREAM_IN_INSIDE_AREA		,
	ID_SPAWN_TEMPLATE_FIRST				,
	ID_SPAWN_TEMPLATE_LAST = ID_SPAWN_TEMPLATE_FIRST + 200,
	ID_DEBUG_MENU						,
	ID_LAYER_FROMSELOBJ					,
	ID_LAYER_CHANGE						,
	ID_SELECT_RESOURCE_FIRST			,
	ID_SELECT_RESOURCE_LAST = ID_SELECT_RESOURCE_FIRST + 100,
	ID_STICKER_MENU						,
	ID_SELECT_SELECT_BY_ENTITY			,
	ID_SELECT_SELECT_BY_RESOURCE		,
	ID_SELECT_SELECT_BY_TAG				,
	ID_SELECT_INVERT_SELECTION			,
	ID_SELECT_HIDE_SELECTION			,
	ID_SELECT_UNHIDE_ALL_LAYERS			,
	ID_SELECT_UNSELECT_ALL				,
	ID_SELECT_CENTER					,
	ID_SELECT_ALL						,
	ID_PICK_POINT_MENU_FIRST			,
	ID_PICK_POINT_MENU_LAST = ID_PICK_POINT_MENU_FIRST + 100,
	ID_LOOT_REPLACE_WITH_EXISTING		,
	ID_LOOT_REPLACE_WITH_NEW			,
	ID_LOOT_ADD_OPTIONS					,
	ID_LOOT_MERGE_WITH_ENTITY			,
};

#define BLACKBOX_NETWORK_CHANNEL	"blackbox"

BEGIN_EVENT_TABLE( CEdWorldEditPanel, CEdRenderingPanel )
END_EVENT_TABLE()

#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "behaviorEditor.h"
#include "sceneExplorer.h"
#include "callbackData.h"
#include "maraudersMap.h"
#include "localizedStringsEditor.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\..\common\engine\stickerComponent.h"
#include "..\..\common\physics\physicsDebugger.h"

/// This class is used to populate right click insertion menu 
class CMenuItem
{
public:
	typedef THashMap< String, CMenuItem > CSubMenuItemMap;
	/// String is the name of the sub menu, empty if the menuItem is not a subMenu
	CSubMenuItemMap		m_subMenuItemMap;
	Int32				m_index;		// -1 if sub menu

	CMenuItem( Int32 index = -1 )
		: m_index( index )	{}

	void Insert( const String & group, Int32 index )
	{
		const String separator = TXT(".");
		String leftSide, rightSide;
		Bool isSubMenu	= group.Split( separator, &leftSide, &rightSide );
		leftSide		= isSubMenu ? leftSide : group;
		// First check if the array of menu item (corresponding to that group) has not been inserted yet :
		CMenuItem* subMenuItem = m_subMenuItemMap.FindPtr( leftSide );
		const Int32 localIndex = isSubMenu ? -1 : index;
		if ( subMenuItem == nullptr )
		{
			VERIFY( m_subMenuItemMap.Insert( leftSide, CMenuItem( localIndex ) ) );
		}
		if ( isSubMenu == false )
		{
			return;
		}
		subMenuItem = m_subMenuItemMap.FindPtr( leftSide );
		subMenuItem->Insert( rightSide, index );				
	}

	void PopulateMenu( wxMenu*const rootMenu, wxMenu*const parentMenu, const TDynArray< TemplateInfo* >& templates, const String & resourcePath, CEdWorldEditPanel *const edWorldEditPanel )const
	{
		for( CSubMenuItemMap::const_iterator iter = m_subMenuItemMap.Begin(); iter != m_subMenuItemMap.End(); ++iter )
		{
			const String& menuName		= (*iter).m_first;
			const CMenuItem & menuItem	= (*iter).m_second;
			const Bool resourceOkay		= menuItem.m_index != -1 ? templates[ menuItem.m_index ]->SupportsResource( resourcePath ) : true;
			if ( resourceOkay )
			{
					
				String name		= menuItem.m_index != -1 ? String( TXT("Add ") ) + menuName : menuName;
				Int32 itemID	= menuItem.m_index != -1 ? ID_SPAWN_TEMPLATE_FIRST + menuItem.m_index : wxID_ANY;
						
						
				if ( menuItem.m_index != -1 )
				{
					parentMenu->Append( itemID, name.AsChar(), wxEmptyString, false );
					rootMenu->Connect( itemID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnSpawnHere ), NULL, edWorldEditPanel );
				}
				else
				{
					wxMenu* subMenu				= new wxMenu();
					menuItem.PopulateMenu( rootMenu, subMenu, templates, resourcePath, edWorldEditPanel );
					parentMenu->Append( itemID, name.AsChar(), subMenu );
				}
			}
					
		}
	}
};

extern EShowFlags GShowEditorMask[];
//extern Float GVisualDebugMaxRenderDistance;

extern CGatheredResource resOnScreenTextFont;

CEdWorldCameraBookmarks GWorldCameraBookmarks;
static int SLastUsedBookmark;
static int SSwappedBookmark;


static const Float NavmeshRootPointsSelectionDistance = 5.f;

RED_DEFINE_NAME( SetWorldEditorViewportCameraView );
RED_DEFINE_NAME( MoveWorldEditorViewportToOverviewPoint );

namespace Debug
{
	extern Int32			GRenderTargetZoomShift;
	extern Int32			GRenderTargetOffsetX;
	extern Int32			GRenderTargetOffsetY;
}

static Bool m_drawGuides = false;

static void AlignNewEntityToPositionAndNormal( CEntity* entity, const Vector& position, const Vector& normal, Bool applyRotation )
{
	Float pushUnits = 0;
	bool first = true;
	const TDynArray<CComponent*>& components = entity->GetComponents();
	EulerAngles savedRotation = entity->GetRotation();
	bool isDecal = false;
	SEntityStreamingState streamingState;
	entity->SetRotation( EulerAngles( 0, 0, 0 ) );
	entity->ForceUpdateTransformNodeAndCommitChanges();
	entity->PrepareStreamingComponentsEnumeration( streamingState, true );

	// Check if it is a decal (special case alignment)
	isDecal = components.Size() == 1 && components[0]->IsA<CDecalComponent>();

	for ( Uint32 i=0; i<components.Size(); ++i )
	{
		CDrawableComponent* dc = Cast<CDrawableComponent>( components[i] );
		if ( dc )
		{
			dc->OnUpdateBounds();
			Box box = dc->GetBoundingBox();
			if ( first || box.Min.Y < pushUnits ) pushUnits = box.Min.Y;
			first = false;
		}
	}

	entity->FinishStreamingComponentsEnumeration( streamingState );

	pushUnits = first ? 0 : position.Y - pushUnits;

	Matrix matrix;
	if ( isDecal )
	{
		Matrix tmp, t2;
		tmp.BuildFromDirectionVector( normal );
		t2.SetIdentity();
		t2.SetRotX33( -M_PI*0.5f );
		matrix = t2*tmp;
		pushUnits = 0;
	}
	else if ( fabs( normal.Z ) > fabs( normal.X ) && fabs( normal.Z ) > fabs( normal.Y ) )
	{
		Matrix tmp;
		tmp.BuildFromDirectionVector( normal );
		matrix.SetRows( tmp.GetRow( 0 ), tmp.GetRow( 2 ), tmp.GetRow( 1 ), tmp.GetRow( 3 ) );
		pushUnits = 0;
	}
	else
	{
		matrix.BuildFromDirectionVector( normal );
	}
	Vector pushVector = matrix.TransformVectorAsPoint( Vector( 0, pushUnits, 0 ) );
	if ( applyRotation )
	{
		entity->SetRotation( matrix.ToEulerAngles() );
	}
	else
	{
		entity->SetRotation( savedRotation );
	}
	entity->SetPosition( entity->GetPosition() + pushVector );
	entity->ForceUpdateTransformNodeAndCommitChanges();
	entity->ForceUpdateBoundsNode();
}

static void AlignEntitiesToPointAndNormal( const TDynArray<CEntity*>& entities, const Vector& mpoint, const Vector& mnormal, Bool applyRotation, Bool addUndoSteps )
{
	CEdWorldEditPanel* editPanel = wxTheFrame->GetWorldEditPanel();
	Vector pathPushedPoint = mpoint + mnormal*0.05f;

	if ( !editPanel->GetWorld() )
	{
		return;
	}

	// Align entities
	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		Bool isVertexEditorEntity = entity->IsA< CVertexEditorEntity >();
		Vector point = isVertexEditorEntity ? pathPushedPoint : mpoint;

		if ( addUndoSteps )
		{
			Vector previousPosition = entity->GetPosition();

			CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( entity ), RED_NAME( transform ) );
			SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPreChange ), CreateEventData( eventData ) );

			TDynArray< CNode* > nodes;
			nodes.PushBack( entity );
			editPanel->GetTransformManager()->Move( nodes, point - previousPosition );

			AlignNewEntityToPositionAndNormal( entity, point, mnormal, applyRotation );

			SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPostChange ), CreateEventData( eventData ) );
		}
		else
		{
			entity->SetPosition( point );
			AlignNewEntityToPositionAndNormal( entity, point, mnormal, applyRotation );
		}
	}
}

static Bool IsPolygonClockwise( const TDynArray< Vector >& points )
{
	Float area = 0.0f;
	for ( Int32 i=0; i < points.SizeInt() - 1; ++i )
	{
		area += ( points[i].X * points[i + 1].Y ) - ( points[i + 1].X * points[i].Y );
	}
	return area < 0.0f;
}

static void ApplyRandomAppearance( CEntity* entity )
{
	CEntityTemplate* tpl = entity->GetEntityTemplate();
	if ( tpl == nullptr ) return;
	const auto& appearances = tpl->GetEnabledAppearancesNames();
	if ( appearances.Empty() ) return;
	CAppearanceComponent* ac = CAppearanceComponent::GetAppearanceComponent( entity );
	if ( ac == nullptr || ac->GetForcedAppearance() ) return;
	ac->SetForcedAppearance( appearances[ ::rand()%appearances.SizeInt() ] );
	ac->ApplyAppearance( ac->GetForcedAppearance() );
}

// Exported camera view copy/paste 
class CCameraClipboard : public ICameraClipboard
{
public:
	CCameraClipboard()
	{
		GCameraClipboard = this;
	}

	virtual void Copy( const Vector& position, const EulerAngles& rotation )
	{
		String viewText = TXT("[[") + ToString( position ) + TXT("|") + ToString( rotation ) + TXT("]]");
		if ( wxTheClipboard->Open() )
		{
			wxTheClipboard->SetData( new wxTextDataObject( viewText.AsChar() ) );
			wxTheClipboard->Close();
			wxTheClipboard->Flush();
		}
		else
		{
			wxMessageBox( wxT("Failed to copy the camera view to the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
		}
	}

	virtual void Paste( Vector& position, EulerAngles& rotation )
	{
		if ( wxTheClipboard->Open() )
		{
			wxTextDataObject data;
			if ( wxTheClipboard->GetData( data ) )
			{
				if ( ParseCameraViewString( String( data.GetText().c_str() ), position, rotation ) )
				{
					GWorldCameraBookmarks.SetBookmark( 0, position, rotation );
					SSwappedBookmark = SLastUsedBookmark;
					SLastUsedBookmark = 0;
					wxTheClipboard->Close();
					return;
				}
			}
			wxTheClipboard->Close();
		}
		wxMessageBox( wxT("Failed to paste a camera view from the clipboard"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, wxTheFrame );
	}

	virtual void CopyToBookmark( Int32 bookmark, const Vector& position, const EulerAngles& rotation )
	{
		GWorldCameraBookmarks.SetBookmark( bookmark, position, rotation );
	}

	virtual Bool PasteFromBookmark( Int32 bookmark, Vector& position, EulerAngles& rotation )
	{
		return GWorldCameraBookmarks.GetBookmark( bookmark, position, rotation );
	}
};

static CCameraClipboard EditorCameraClipboard;


//////////////////////////////////////////////////////////////////////////
// TemplateInfo::CSpawnEventHandler
//////////////////////////////////////////////////////////////////////////
void TemplateInfo::CSpawnEventHandler::OnPostAttach( CEntity* entity )
{
	entity->EditorPostCreation();
}
//////////////////////////////////////////////////////////////////////////
// TemplateInfo
//////////////////////////////////////////////////////////////////////////

TemplateInfo::TemplateInfo( const String& name, const String& requiredExtension, const String& templateName, const String& group, const String& templateInsertionClass, Bool detachFromTemplate )
	: m_name( name )
	, m_requiredExtension( requiredExtension )
	, m_templateName( templateName )
	, m_group( group )
	, m_templateInsertionClass( templateInsertionClass )
	, m_detachFromTemplate( detachFromTemplate )
{
}

// Does this template given resource as a base
Bool TemplateInfo::SupportsResource( const String& resName )
{
	// Check extension
	if ( !m_requiredExtension.Empty() )
	{
		CFilePath path( resName );
		return m_requiredExtension.EqualsNC( path.GetExtension() );
	}

	// Valid
	return true;
}

CEntity* TemplateInfo::Create( CLayer* layer, const Vector& spawnPosition, CResource* baseResource )
{
	ASSERT( layer );

	// Spawn entity
	EntitySpawnInfo sinfo;
	sinfo.m_spawnPosition = spawnPosition;
	sinfo.m_resource = baseResource;
	sinfo.m_template = LoadResource< CEntityTemplate >( m_templateName.AsChar() );
	sinfo.m_detachTemplate = m_detachFromTemplate;
	sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );

	CEntity* newEntity = layer->CreateEntitySync( sinfo );

	// Note: have to check, because CreateEntitySync may fail for different reasons and this method is supposed to return nullptr without crashing.
	if ( newEntity )
	{
		// Create streamed components (they will be unloaded if not needed in the next camera move)
		newEntity->CreateStreamedComponents( SWN_NotifyWorld );
		ApplyRandomAppearance( newEntity );
	}

	return newEntity;
}

void TemplateInfo::GetName( String & outName )const
{
	const String separator			= TXT(".");	
	TDynArray< String > splitArray	= m_group.Split( separator );
	outName = m_group;
	if ( splitArray.Size() != 0 )
	{
		outName = splitArray[ splitArray.Size() - 1 ];
	}
}


// Spawnable template info
class TemplateInfoEntityTemplate : public TemplateInfo
{
public:
	RED_INLINE TemplateInfoEntityTemplate()
		: TemplateInfo( TXT("Entity"), TXT("w2ent"), String::EMPTY, String::EMPTY, String::EMPTY, false )
	{};

	virtual CEntity* Create( CLayer* layer, const Vector& spawnPosition, CResource* baseResource )
	{
		ASSERT( layer );

		// Load template
		CEntityTemplate* entityTemplate = SafeCast< CEntityTemplate >( baseResource );
		if ( entityTemplate )
		{
			// Spawn entity
			EntitySpawnInfo sinfo;
			sinfo.m_spawnPosition = spawnPosition;
			sinfo.m_template = entityTemplate;
			sinfo.m_detachTemplate = false;	// Use template
			sinfo.m_tags = entityTemplate->GetEntityObject() ? entityTemplate->GetEntityObject()->GetTags() : TagList::EMPTY;
			sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );
			CEntity* entity = layer->CreateEntitySync( sinfo );
			ApplyRandomAppearance( entity );
			return entity;
		}

		// Not created
		return NULL;
	}

};

class TemplateInfoCharacterEntityTemplate : public TemplateInfoEntityTemplate
{
public:
	RED_INLINE TemplateInfoCharacterEntityTemplate()
		: TemplateInfoEntityTemplate()
	{
		m_name = TXT("Character Entity");
		m_requiredExtension = TXT("w2cent");
	}
};

class LayerToSet : public wxObject
{
public:
	CLayerInfo*		m_layer;

public:
	LayerToSet ( CLayerInfo* layer )
		: m_layer( layer )
	{};
};

CEdWorldCameraBookmarks::CEdWorldCameraBookmarks()
{
	Red::System::MemorySet( &m_bookmark, 0, sizeof(m_bookmark) );
}

void CEdWorldCameraBookmarks::SetBookmark( int num, const Vector& position, const EulerAngles& rotation )
{
	ASSERT( num >= 0 && num < MAX_WORLD_CAMERA_BOOKMARKS, TXT("World camera bookmark out of range" ) );
	m_bookmark[ num ].position = position;
	m_bookmark[ num ].rotation = rotation;
	m_bookmark[ num ].used = true;
}

Bool CEdWorldCameraBookmarks::GetBookmark( int num, Vector& position, EulerAngles& rotation ) const
{
	ASSERT( num >= 0 && num < MAX_WORLD_CAMERA_BOOKMARKS, TXT("World camera bookmark out of range" ) );
	if ( m_bookmark[ num ].used )
	{
		position = m_bookmark[ num ].position;
		rotation = m_bookmark[ num ].rotation;
	}
	return m_bookmark[ num ].used;
}

Bool CEdWorldCameraBookmarks::IsBookmarkSet( int num ) const
{
	ASSERT( num >= 0 && num < MAX_WORLD_CAMERA_BOOKMARKS, TXT("World camera bookmark out of range" ) );
	return m_bookmark[ num ].used;
}

void CEdWorldCameraBookmarks::SaveSession( CConfigurationManager &config )
{
	for ( Uint32 i=0; i<MAX_WORLD_CAMERA_BOOKMARKS; ++i )
	{
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("Defined"), ToString( m_bookmark[i].used ) );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionX"), m_bookmark[i].position.X );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionY"), m_bookmark[i].position.Y );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionZ"), m_bookmark[i].position.Z );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationPitch"), m_bookmark[i].rotation.Pitch );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationYaw"), m_bookmark[i].rotation.Yaw );
		config.Write( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationRoll"), m_bookmark[i].rotation.Roll );
	}
}

void CEdWorldCameraBookmarks::RestoreSession( CConfigurationManager &config )
{
	for ( Uint32 i=0; i<MAX_WORLD_CAMERA_BOOKMARKS; ++i )
	{
		FromString( config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("Defined"), ToString( m_bookmark[i].used ) ), m_bookmark[i].used );
		m_bookmark[i].position.X = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionX"), m_bookmark[i].position.X );
		m_bookmark[i].position.Y = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionY"), m_bookmark[i].position.Y );
		m_bookmark[i].position.Z = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("PositionZ"), m_bookmark[i].position.Z );
		m_bookmark[i].rotation.Pitch = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationPitch"), m_bookmark[i].rotation.Pitch );
		m_bookmark[i].rotation.Yaw = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationYaw"), m_bookmark[i].rotation.Yaw );
		m_bookmark[i].rotation.Roll = config.Read( TXT("WorldCameraBookmarks/Bookmark") + ToString( i ) + TXT("RotationRoll"), m_bookmark[i].rotation.Roll );
	}
}

//////////////////////////////////////////////////////////////////////////

CEdWorldEditPanel::CEdWorldEditPanel( wxWindow* parent, CEdSceneExplorer *scene )
	: CEdRenderingPanel( parent )
	, m_forceOrthoCamera( false )
	, m_orthoZoom( 1000.0f )
	, m_scene( scene )
	, m_maraudersMap( NULL )
	, m_locStringsEd( NULL )
	, m_screenshotEditor( NULL )
	, m_cameraRotating( false )
	, m_lastClickTime( 0 )
	, m_previousLastClickTime( 0 )
	, m_enableSelectionUndo( true )
	, m_hasPickPoint( false )
	, m_pickPointClient( nullptr )
	, m_transformManager( nullptr )
	, m_selFreeSpaceShow( false )
	, m_selFreeSpaceLock( false )
{
	// Set render mode
	GetViewport()->SetRenderingMode( RM_Shaded );
	GetViewport()->SetRenderingMask( GShowEditorMask );
	//GetViewport()->SetRenderingDebugOptions( GVisualDebugMaxRenderDistance );

	// Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileSaved ), this );
	SEvents::GetInstance().RegisterListener( CNAME( Attached ), this );
	SEvents::GetInstance().RegisterListener( CNAME( Detached ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnding ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPostUndoStep ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

#ifdef RED_NETWORK_ENABLED
	// Register as network channel listener
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		network->RegisterListener( BLACKBOX_NETWORK_CHANNEL, this );
	}
#endif

	// Add widgets
	const Float rotateGismoSize = 0.7f;

	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 1, 0 ), Vector::EX, Color::RED, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 1, 0, 0 ), Vector::EY, Color::GREEN, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 0, 1 ), Vector::EZ, Color::BLUE, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( Color::GRAY, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleUniform( Vector::EX, Vector::EY, Vector::EZ, Color::WHITE ) );

	// Templates
	m_templates.PushBack( new TemplateInfoEntityTemplate() );
	m_templates.PushBack( new TemplateInfoCharacterEntityTemplate() );

	C2dArray *list = LoadResource< C2dArray >( EDITOR_TEMPLATES_CSV );

	for ( Uint32 i=0; i<list->GetNumberOfRows(); ++i )
	{
		String ext			= list->GetValue( TXT("Ext"), i );
		String resource		= list->GetValue( TXT("Resource"), i );
		String group		= list->GetValue( TXT("GroupPath"), i );
		String insertionClass	= list->GetValue( TXT("TemplateInsertionClass"), i );
		m_templates.PushBack( new TemplateInfo( String::EMPTY, ext, resource, group, insertionClass, true ) );
	}

	InitStickers(); // execute it before accelerators load

	// Accelerators
	CEdShortcutsEditor::Load( *this, *GetAccelerators(), TXT("Main Editor") );

	// for sound area testing without running game
	m_prevCameraPosition = m_cameraPosition;

	if ( m_copiesDirectory = GDepot->CreateNewDirectory( TXT("layers_copies") ) )
	{
		while (  m_copiesDirectory->GetFiles().Size() > 0 )
		{
			CDiskFile* currFile = *( m_copiesDirectory->GetFiles().Begin());
			currFile->GetStatus();
			currFile->Delete( false, false );
		}
	}
}

CEdWorldEditPanel::~CEdWorldEditPanel()
{
#ifdef RED_NETWORK_ENABLED
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		network->UnregisterListener( BLACKBOX_NETWORK_CHANNEL, this );
	}
#endif

	SEvents::GetInstance().UnregisterListener( this );
	m_templates.ClearPtr();
	ClearCopiedLayers();
	
	if ( m_shapesContainer ) 
	{
		delete m_shapesContainer;
	}

	if ( m_transformManager ) 
	{
		delete m_transformManager;
	}
}

CWorld* CEdWorldEditPanel::GetWorld()
{
	return GGame->GetActiveWorld();
}

CSelectionManager* CEdWorldEditPanel::GetSelectionManager()
{
	// TODO: change it to returning a local field when SelectionManager it's already created in place
	if ( GetWorld() ) 
	{
		return GetWorld()->GetSelectionManager();
	}
	else
	{
		return nullptr;
	}
}

CNodeTransformManager* CEdWorldEditPanel::GetTransformManager()
{
	// TODO: change it to returning a local field when SelectionManager it's already created in place
	if ( !m_transformManager && GetWorld() && GetSelectionManager() )
	{
		m_transformManager = new CNodeTransformManager( GetWorld(), GetSelectionManager() );
	}

	return m_transformManager;
}

void CEdWorldEditPanel::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	// Ask edit mode to handle event
	if ( m_tool && m_tool->HandleSelection( objects ) )
	{
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
        // Toggle selection
        CSelectionManager* selectionMgr = GetSelectionManager();
		CSelectionManager::CSelectionTransaction transaction(*selectionMgr);

		// Change active layer only (if ALT key is down)
		int altDown = RIM_IS_KEY_DOWN( IK_Alt );
		if ( altDown )
		{
			for ( Uint32 i=0; i<objects.Size(); i++ )
			{
				CComponent* tc = Cast< CComponent >( objects[i]->GetHitObject() );

				if ( !tc )
				{
					continue;
				}

				selectionMgr->DeselectAll();

				if ( tc->GetLayer() != m_scene->GetActiveLayer() )
					m_scene->ChangeActiveLayer( tc->GetLayer()->GetLayerInfo() );

				selectionMgr->Select( tc );
			}

			return;
		}


		// Deselect all selected object
		if ( !RIM_IS_KEY_DOWN( IK_LControl ) )
		{
			selectionMgr->DeselectAll();
			m_scene->SelectSceneObject( nullptr, true );
		}

		// Toggle selection
		for ( Uint32 i=0; i<objects.Size(); i++ )
		{
			CComponent* tc = Cast< CComponent >( objects[i]->GetHitObject() );

			if ( !tc )
			{
				continue;
			}

			CEntity* entity = tc->GetEntity();

			// Select mode set to select only from active layer?
			if( selectionMgr->GetSelectMode() == SM_ActiveLayer && tc->GetLayer() != m_scene->GetActiveLayer() )
			{
				continue;
			}

			if ( tc->IsSelected() && !RIM_IS_KEY_DOWN( IK_LShift ) && entity->GetPartOfAGroup() == false )
			{
				selectionMgr->Deselect( tc );
				m_scene->DeselectSceneObject( entity );
			}
			else
            if ( !tc->IsSelected() )
			{
				if( selectionMgr->GetSelectMode() == SM_ActiveLayer && tc->GetLayer() != m_scene->GetActiveLayer() )
					m_scene->ChangeActiveLayer( tc->GetLayer()->GetLayerInfo() );
				selectionMgr->Select( tc );
				m_scene->SelectSceneObject( entity );
			}
		}
	}
	if ( m_shapesContainer )
	{
		m_shapesContainer->HandleItemSelection( objects );
	}
}

void CEdWorldEditPanel::CopySelectedEntities()
{
	ClearCopiedLayers();

	// Copy entities
	CWorld* world = GetWorld();
	if ( world )
	{
		// Get selection
		TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();

		// Only if we have selection
		if ( entities.Size() )
		{
			if ( !GObjectClipboard.Copy( entities ) )
			{
				WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
			}
		}
	}
}

void CEdWorldEditPanel::CopyLayers( TDynArray< CLayerInfo* > layerInfos )
{
	ClearCopiedLayers();

	if ( layerInfos.Size() )
	{
		THashMap< String, Uint32 > namesCounter;
		for ( CLayerInfo* li : layerInfos )
		{
			String name = li->GetShortName();
			Uint32& nameCount = namesCounter.GetRef( name, 0 );
			if ( nameCount > 0 )
			{
				name += String::Printf( TXT("(%d)"), nameCount++ );
			}

			if ( li->GetLayer()->GetFile()->Copy( m_copiesDirectory, name + TXT(".w2l") ) )
			{
				if ( CDiskFile* copiedFile = m_copiesDirectory->FindLocalFile( name + TXT(".w2l") ) )
				{
					if ( CLayerInfo* copiedLayer = Cast< CLayerInfo >( li->Clone( nullptr ) ) )
					{
						m_copiedLayers.PushBack( copiedLayer );
						copiedLayer->InitNoWorld( copiedFile->GetDepotPath() );
					}
				}
			}
		}
	}
}

void CEdWorldEditPanel::OnCopy( wxCommandEvent& event )
{
	ClearCopiedLayers();

	// Ask edit mode to handle event
	if ( m_tool && m_tool->OnCopy() )
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnCopy() )
		{
			return;
		}
	}

	m_scene->OnCopy();
}

void CEdWorldEditPanel::OnCut( wxCommandEvent& event )
{
	ClearCopiedLayers();

	// Ask edit mode to handle event
	if ( m_tool && m_tool->OnCut() )
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnCut() )
		{
			return;
		}
	}
	m_scene->OnCopy( CM_Default, true );
}

Bool CEdWorldEditPanel::PasteLayers( CLayerGroup* parentGroup )
{
	if ( parentGroup )
	{
		if ( !m_copiedLayers.Empty() )
		{
			// Destroy current selection
			GetSelectionManager()->DeselectAll();

			// Keep a list of pasted layers
			TDynArray< CLayerInfo* > pastedLayers;

			// Add layers in the group
			m_scene->Freeze();
			for ( Uint32 i = 0; i < m_copiedLayers.Size(); ++i )
			{
				CLayerInfo* layerInfo = m_copiedLayers[i].Get();

				String counter, layerName = layerInfo->GetShortName();
				Uint32 namesIt = 0;
				while ( parentGroup->FindLayer( layerName + counter ) )
				{
					namesIt++;
					counter = String::Printf( TXT("_%d"), namesIt );
				}
				layerName += counter + TXT(".w2l");

				String newLayerPath = parentGroup->GetDepotPath() + layerName;
				if ( CDiskFile* newFile = GDepot->CreateNewFile( newLayerPath.AsChar() ) )
				{
					newFile->GetStatus();
					if ( newFile->IsDeleted() )
					{
						newFile->Revert( true );
						newFile->CheckOut();
					}
					else if ( newFile->IsLocal() )
					{
						newFile->Add();
					}
				}

				CLayerInfo* newLayerInfo = Cast< CLayerInfo >( layerInfo->Clone( parentGroup ) );
				newLayerInfo->Init( parentGroup->GetWorld(), parentGroup, newLayerPath );
				newLayerInfo->Create();
				newLayerInfo->SyncUnload();

				parentGroup->AddLayer( newLayerInfo );
				m_scene->ChangeActiveLayer( newLayerInfo );

				CLayer* tmpLayer = nullptr;
				if ( CDiskFile* tmpFile = GDepot->FindFile( layerInfo->GetDepotPath() ) )
				{
					tmpLayer = Cast< CLayer >( tmpFile->Load() );
					newLayerInfo->SyncLoad( LayerLoadingContext() );
					
					if ( tmpLayer )
					{
						for ( const CEntity* ent : tmpLayer->GetEntities() )
						{
							CEntity* newEntity = Cast< CEntity >( ent->Clone( nullptr ) );
							newLayerInfo->GetLayer()->AddEntity( newEntity );
						}
					}
					tmpFile->Unload();
				}

				// Add to the pasted layers
				pastedLayers.PushBack( newLayerInfo );
				GetSelectionManager()->SelectLayer( newLayerInfo );
			}

			// Finish
			m_scene->Thaw();

			// Did we actually paste anything?
			if ( !pastedLayers.Size() )
			{
				// Inform the user if we didn't paste anything
				wxMessageBox( wxT("Couldn't find any entities among the objects in the clipboard"), wxT("No entities to paste"), wxCENTRE|wxOK, this );
				return false;
			}

			return true;
		}
	}
	return false;
}

void CEdWorldEditPanel::OnPaste( wxCommandEvent& event )
{	
	// Ask edit mode to handle event
	if ( m_tool && m_tool->OnPaste() )
	{
		return;
	}

	// Paste entities
	m_scene->OnPaste();
}

void CEdWorldEditPanel::OnPasteHere( wxCommandEvent& event )
{
	// Paste only if no edit mode enabled
	if ( !m_tool )
	{
		PasteEntities( &m_clickedWorldPos, false );
	}
}

void CEdWorldEditPanel::OnPasteAlignedHere( wxCommandEvent& event )
{
	// Paste only if no edit mode enabled
	if ( !m_tool )
	{
		PasteEntities( &m_clickedWorldPos, true );
	}
}

void CEdWorldEditPanel::OnDelete( wxCommandEvent& event )
{
	if ( CWorld* world = GetWorld() )
	{
		TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();

		// special case for group of objects
		{
			Bool foundGrouppedObject = false;

			const Uint32 entityCount = entities.Size();
			for( Uint32 i=0; i<entityCount; ++i )
			{
				CEntityGroup* group = entities[i]->GetContainingGroup();
				if( group != nullptr && group->IsLocked() == false )
				{
					foundGrouppedObject = true;
				}
			}

			// if one or more objects are grouped
			if( foundGrouppedObject == true )
			{
				GFeedback->ShowWarn( TXT("One or more selected objects are in group and their group is unlocked. Please remove them from group or lock group before deleting.") );
				return;
			}
		}

		DeleteEntities( entities );
	}
}

void CEdWorldEditPanel::OnCopyCameraView( wxCommandEvent& event )
{
	EditorCameraClipboard.Copy( m_cameraPosition, m_cameraRotation );
}

void CEdWorldEditPanel::OnPasteCameraView( wxCommandEvent& event )
{
	EditorCameraClipboard.Paste( m_cameraPosition, m_cameraRotation );
	OnCameraMoved();
}

/*
void CEdWorldEditPanel::OnSpawnHere( wxCommandEvent& event )
{
	TemplateInfo* info = m_templates[ event.GetId() - ID_SPAWN_TEMPLATE_FIRST ];
	ASSERT( info );

	// Load resource
	CResource* baseResource = NULL;
	if ( !info->m_requiredExtension.Empty() )
	{
		String resourcePath;

		if( event.m_callbackUserData )
		{
			TCallbackData< String > *data = ( TCallbackData< String >* )event.m_callbackUserData;
			resourcePath = data->GetData();
		}
		else
		{
			GetActiveResource( resourcePath );
		}
		if ( !resourcePath.Empty() )
		{
			baseResource = LoadResource< CResource >( resourcePath );
		}

		// Resource not loaded although needed
		if ( resourcePath.Empty() )
		{
			MessageBox( (HWND)GetHandle(), TXT("Unable to load resource"), TXT("Error"), MB_ICONWARNING | MB_TASKMODAL | MB_TOPMOST );
			return;
		}
	}

	CEntityTemplate* entTemplate = Cast< CEntityTemplate >( baseResource );
	if ( !CanSpawnEntity( entTemplate ) )
	{
		wxMessageBox( TXT("Can't spawn this type of entity on a static layer. This may be the in-game only dynamically spawn entity."), TXT("Spawn denied") );
		return;
	}

	// Create entity
	CLayer* layer = CheckActiveLayer();
	if ( layer )
	{
		// if layer has file, then it must be able to be modified - otherwise it cannot be changed
		if ( layer->GetFile() && !layer->GetFile()->MarkModified() )
		{
			return;
		}

		CEntity *newEntity = info->Create( layer, m_clickedWorldPos, baseResource );
		

		if ( newEntity )

		{
			if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
			{
				AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal );
			}

			wxTheFrame->AddToIsolated( newEntity );

			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
			CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
		}
	}
}
*/

void CEdWorldEditPanel::OnSpawnHere( wxCommandEvent& event )
{
	// Bring the focus back to the panel whatever the case
	SetFocus();
	CLayer* layer = CheckActiveLayer();
	if ( layer )
	{
		// if layer has file, then it must be able to be modified - otherwise it cannot be changed
		if ( layer->GetFile() && !layer->GetFile()->MarkModified() )
		{
			return;
		}
		TemplateInfo* info = m_templates[ event.GetId() - ID_SPAWN_TEMPLATE_FIRST ];
		ASSERT( info );

		// Load resource
		CResource* baseResource = NULL;
		if ( !info->m_requiredExtension.Empty() )
		{
			String resourcePath;

			if( event.m_callbackUserData )
			{
				TCallbackData< String > *data = ( TCallbackData< String >* )event.m_callbackUserData;
				resourcePath = data->GetData();
			}
			else
			{
				GetActiveResource( resourcePath );
			}
			if ( !resourcePath.Empty() )
			{
				baseResource = LoadResource< CResource >( resourcePath );
			}

			// Resource not loaded although needed
			if ( resourcePath.Empty() )
			{
				MessageBox( (HWND)GetHandle(), TXT("Unable to load resource"), TXT("Error"), MB_ICONWARNING | MB_TASKMODAL | MB_TOPMOST );
				SetFocus();
				return;
			}
		}

		CEntityTemplate* entTemplate = Cast< CEntityTemplate >( baseResource );
		if ( !CanSpawnEntity( entTemplate ) )
		{
			wxMessageBox( TXT("Can't spawn this type of entity on a static layer. This may be the in-game only dynamically spawn entity."), TXT("Spawn denied") );
			SetFocus();
			return;
		}

		// Create entity
		if ( !layer->IsAttached() )
		{
			wxMessageBox( TXT("Can't spawn this type of entity on hiden layer."), TXT("Spawn denied") );
			SetFocus();
			return;
		}
		if ( SpawnHereWithTemplateInsertionClasss( info, layer, baseResource ) )
		{
			// Success !
			return;
		}
		
		// No insertion class just creating entity and insert it :
		CEntity *newEntity = info->Create( layer, m_clickedWorldPos, baseResource );
		if ( newEntity )
		{
			if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
			{
				AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal, true );
			}

			wxTheFrame->AddToIsolated( newEntity );

#ifdef USE_UMBRA
			GetWorld()->NotifyOcclusionSystem( newEntity );
#endif // USE_UMBRA

			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
			CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );

			m_scene->SelectSceneObject( newEntity, true );
		}
	}
}

Bool CEdWorldEditPanel::SpawnHereWithTemplateInsertionClasss( const TemplateInfo *const templateInfo, CLayer* layer, CResource* baseResource )
{
	if ( templateInfo->m_templateInsertionClass.Empty() )
	{
		return false;
	}
	CClass *const templateInsertionClass		= SRTTI::GetInstance().FindClass( CName( templateInfo->m_templateInsertionClass ) );
	if ( templateInsertionClass == nullptr )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Editor ), TXT("TemplateInsertionClass is invalid: %s"), templateInfo->m_templateInsertionClass.AsChar() );
		return true;
	}
	CTemplateInsertion *const templateInsertion	= templateInsertionClass->GetDefaultObject<CTemplateInsertion>();
	if ( templateInsertion == nullptr )
	{
		return true;
	}
	templateInsertion->OnTemplateInserted( templateInfo, m_clickedWorldPos, layer, baseResource );
	return true;
}

void CEdWorldEditPanel::ClearCopiedLayers()
{
	for ( Uint32 i = 0; i < m_copiedLayers.Size(); ++i )
	{
		CLayerInfo* li = m_copiedLayers[i];
		li->SyncUnload();
		if ( CDiskFile* copyFile = GDepot->FindFile( li->GetDepotPath() ) )
		{
			copyFile->GetStatus();
			copyFile->Delete();
			delete copyFile;
		}
		delete li;
	}
	m_copiedLayers.Clear();
}

void CEdWorldEditPanel::OnSpawnClass( wxCommandEvent& event )
{
	CLayer* layer = CheckActiveLayer();
	if ( layer )
	{
		EntitySpawnInfo info;
		info.m_spawnPosition = m_clickedWorldPos;
//		info.m_class = GetActiveClass();
		info.m_resource = NULL;
		info.m_template = NULL;
		info.AddHandler( new TemplateInfo::CSpawnEventHandler() );

		// Create entity
		CEntity *newEntity = layer->CreateEntitySync( info );
		if ( newEntity != nullptr )
		{
			ApplyRandomAppearance( newEntity );
		}

		CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
		CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
	}
}

void CEdWorldEditPanel::OnChangeLayer( wxCommandEvent& event )
{
	LayerToSet* layer = static_cast< LayerToSet* >( event.m_callbackUserData );
	
	if ( layer )
	{
		m_scene->ChangeActiveLayer( layer->m_layer );
	}
}

/*
Bool CEdWorldEditPanel::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	if( CalcClickedPosition( x, y ) )
	{
		// Filter templates, that can't be spawned on the static layer
		for ( Int32 i=resources.Size()-1; i>=0; --i )
		{
			CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( resources[i] );
			if ( entityTemplate && !CanSpawnEntity( entityTemplate ) )
			{
				String message = String::Printf( TXT("Template %s is only allowed to be spawned dynamically."), entityTemplate->GetFriendlyName().AsChar() );
				wxMessageBox( message.AsChar(), TXT("Dropping templates...") );
				resources.RemoveAt( i );
			}
		}

		CLayer* layer = CheckActiveLayer();
		if( resources.Size() == 1 )
		{
			wxMenu menu;
			String resourcePath = resources[ 0 ]->GetFile()->GetDepotPath();
			TemplateInfo* info = NULL;
			for ( Uint32 i=0; i<m_templates.Size(); i++ )
			{
				TemplateInfo* ti = m_templates[i];
				if ( ti->SupportsResource( resourcePath ) )
				{
					String name = String( TXT("Add ") ) + ti->m_name;
					if ( !ti->m_requiredExtension.Empty()  )
					{
						name += String::Printf( TXT(" using '%s'"), resourcePath.AsChar() );
						// Add menu option
						menu.Append( ID_SPAWN_TEMPLATE_FIRST + i, name.AsChar() );
						menu.Connect( ID_SPAWN_TEMPLATE_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnSpawnHere ), new TCallbackData< String >( resourcePath ), this );

						if( !info )
						{
							info = ti;
						}
					}
				}
			}

			if( menu.GetMenuItemCount() <= 1 )
			{
				if( info )
				{
					CEntity *newEntity = NULL;
						
					if ( layer )
					{
						newEntity = info->Create( layer, m_clickedWorldPos, resources[ 0 ] );
					}
					else
					{
						newEntity = info->Create( GetWorld(), m_clickedWorldPos, resources[ 0 ] );
					}

					if ( newEntity )
					{
						if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
						{
							AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal );
						}

						wxTheFrame->AddToIsolated( newEntity );

						CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
						CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
					}
				}
			}
			else
			{
				PopupMenu( &menu );
			}
		}
		else if ( !resources.Empty() )
		{
			for( TDynArray< CResource* >::iterator it=resources.Begin(); it!=resources.End(); it++ )
			{
				CResource *resource = *it;
				String resourcePath = resource->GetFile()->GetDepotPath();

				TemplateInfo* info = NULL;
				for ( Uint32 i=0; i<m_templates.Size(); i++ )
				{
					// Add templates options
					TemplateInfo* ti = m_templates[i];
					if ( ti->SupportsResource( resourcePath ) && !ti->m_requiredExtension.Empty() )
					{
						info = ti;
						break;
					}
				}

				if( !info )
					continue;

				CEntity* newEntity;
				if ( layer )
				{
					newEntity = info->Create( layer, m_clickedWorldPos, resource );
				}
				else
				{
					newEntity = info->Create( GetWorld(), m_clickedWorldPos, resource );
				}

				if(newEntity)
				{
					if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
					{
						AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal );
					}

					wxTheFrame->AddToIsolated( newEntity );
					CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
				}
				else
				{
					GFeedback->ShowWarn(TXT("Could not load selected entity template"));
					//couldnt load entity 
				}
					
			}
			CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
		}
		return true;
	}

	return false;
}
*/

Bool CEdWorldEditPanel::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	if( CalcClickedPosition( x, y ) )
	{
		#ifndef NO_MARKER_SYSTEMS
			if( GEngine->GetMarkerSystems()->WaitingForEntity() == true)
			{
				CEntity *newEntity = NULL;
				CEntityTemplate* entityTemplate = SafeCast< CEntityTemplate >( resources[0] );
				if ( entityTemplate )
				{
					EntitySpawnInfo einfo;
					einfo.m_spawnPosition = m_clickedWorldPos;
					einfo.m_detachTemplate = false;
					einfo.m_template = entityTemplate;
					einfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );
					newEntity = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
					if ( newEntity != nullptr )
					{
						ApplyRandomAppearance( newEntity );
					}
				}
				if(resources.Empty() == false)
				{
					GEngine->GetMarkerSystems()->SetNewEntity(newEntity);
				}

				return true;
			}
		#endif

		CLayer* layer = CheckActiveLayer();
		if ( layer )
		{
			if ( !layer->GetLayerInfo()->IsVisible() )
			{
				wxMessageBox( wxT("Dropping resources on hidden layers is not allowed."), wxT("Error"), wxOK | wxICON_ERROR, wxTheFrame );
				return false;
			}

			// Check if the layer can be modified
			if ( !layer->IsModified() && layer->GetFile() && !layer->GetFile()->Edit() )
			{
				return false;
			}

			// Filter templates, that can't be spawned on the static layer
			for ( Int32 i=resources.Size()-1; i>=0; --i )
			{
				CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( resources[i] );
				if ( entityTemplate && !CanSpawnEntity( entityTemplate ) )
				{
					String message = String::Printf( TXT("Template %s is only allowed to be spawned dynamically."), entityTemplate->GetFriendlyName().AsChar() );
					wxMessageBox( message.AsChar(), TXT("Dropping templates...") );
					resources.RemoveAt( i );
				}
			}

			if( resources.Size() == 1 )
			{
				wxMenu menu;
				String resourcePath = resources[ 0 ]->GetFile()->GetDepotPath();
				TemplateInfo* info = NULL;
				Bool qaFolder = false;
				// If the path is going into the qa folder we prompt the user saying ajjabajja
				// Make sure user knows what he's doing
				String msg = String::Printf( TEXT("%s file(s) is from the QA Folder. Do you really want to Create it since we do not support the QA FOLDER for the GAME?"), resourcePath.AsChar() );
				if ( resourcePath.BeginsWith( TXT("qa\\") ) )
				{
					if ( wxNO == wxMessageBox( msg.AsChar(), wxT("Place Resource"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT ) )
					{
						qaFolder = true;
					}
				}	

				if ( !qaFolder )
				{
					for ( Uint32 i=0; i<m_templates.Size(); i++ )
					{
						TemplateInfo* ti = m_templates[i];
						if ( ti->SupportsResource( resourcePath ) )
						{
							String templateName;
							ti->GetName( templateName );
							String name = String( TXT("Add ") ) + templateName;
							if ( !ti->m_requiredExtension.Empty()  )
							{
								name += String::Printf( TXT(" using '%s'"), resourcePath.AsChar() );
								// Add menu option
								menu.Append( ID_SPAWN_TEMPLATE_FIRST + i, name.AsChar() );
								menu.Connect( ID_SPAWN_TEMPLATE_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnSpawnHere ), new TCallbackData< String >( resourcePath ), this );

								if( !info )
								{
									info = ti;
								}
							}
						}
					}
				}

				if( menu.GetMenuItemCount() <= 1 )
				{
					if( info )
					{
						CEntity *newEntity = info->Create( layer, m_clickedWorldPos, resources[ 0 ] );

						Bool qaFolder = false;
						if( newEntity )
						{
							const CLayerInfo* layerInfo = layer->GetLayerInfo();
							String error;
							if ( !CLayersEntityCompatibilityChecker::IsEntityCompatible( newEntity, layerInfo, error ) )
							{
								wxMessageBox( error.AsChar(), wxT("Error"), wxOK | wxICON_ERROR, wxTheFrame );
							}

							SEntityStreamingState streamingState;
							TDynArray< CResource* > usedResources;
							
							newEntity->PrepareStreamingComponentsEnumeration( streamingState, true );
							newEntity->ForceFinishAsyncResourceLoads();

							// Get resources used by selected entities
							newEntity->CollectUsedResources( resources );
							newEntity->FinishStreamingComponentsEnumeration( streamingState );
							for ( Uint32 i=0; i<resources.Size(); ++i )
							{
								CResource* res = resources[i];
								if( res->GetDepotPath().BeginsWith( TXT("qa\\") ) )
								{
									// If the path is going into the qa folder we prompt the user saying ajjabajja
									// Make sure user knows what he's doing
									String msg = String::Printf( TEXT("%s file(s) is from the QA Folder. Do you really want to Create it since we do not support the QA FOLDER for the GAME?"), res->GetDepotPath().AsChar() );
									if ( wxNO == wxMessageBox( msg.AsChar(), wxT("Place Resource"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT ) )
									{
										qaFolder = true;
										newEntity->Destroy();
										newEntity = nullptr;
									}
								}
							}							
						}
						if(newEntity && !qaFolder)
						{
							if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
							{
								AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal, true );
							}

							wxTheFrame->AddToIsolated( newEntity );
							CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
							CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );

							m_scene->SelectSceneObject( newEntity, true );
						}
						else
						{
							GFeedback->ShowWarn(TXT("Could not load selected entity template"));
							//couldnt load entity 
						}
					}
				}
				else
				{
					PopupMenu( &menu );
				}
			}
			else if ( !resources.Empty() )
			{
				const CLayerInfo* layerInfo = layer->GetLayerInfo();

				for( TDynArray< CResource* >::iterator it=resources.Begin(); it!=resources.End(); it++ )
				{
					CResource *resource = *it;
					String resourcePath = resource->GetFile()->GetDepotPath();

					TemplateInfo* info = NULL;
					for ( Uint32 i=0; i<m_templates.Size(); i++ )
					{
						// Add templates options
						TemplateInfo* ti = m_templates[i];
						if ( ti->SupportsResource( resourcePath ) && !ti->m_requiredExtension.Empty() )
						{
							info = ti;
							break;
						}
					}

					if( !info )
						continue;

					CEntity *newEntity = info->Create( layer, m_clickedWorldPos, resource );
					if(newEntity)
					{
						String error;
						if ( !CLayersEntityCompatibilityChecker::IsEntityCompatible( newEntity, layerInfo, error ) )
						{
							wxMessageBox( error.AsChar(), wxT("Error"), wxOK | wxICON_ERROR, wxTheFrame );
						}

						if ( RIM_IS_KEY_DOWN( IK_Shift ) || wxGetKeyState( WXK_SHIFT ) )
						{
							AlignNewEntityToPositionAndNormal( newEntity, m_clickedWorldPos, m_clickedWorldNormal, true );
						}

						wxTheFrame->AddToIsolated( newEntity );
						CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
					}
					else
					{
						GFeedback->ShowWarn(TXT("Could not load selected entity template"));
						//couldnt load entity 
					}
					
				}
				CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
			}

			GGame->GetActiveWorld()->RequestStreamingUpdate();
		}
		return true;
	}

	return false;
}

bool CEdWorldEditPanel::CalcClickedPosition( Int32 x, Int32 y )
{
	if ( !GetWorld() )
		return false;

	return GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), x, y, m_clickedWorldPos, &m_clickedWorldNormal );
};

void CEdWorldEditPanel::HandleContextMenu( Int32 x, Int32 y )
{
	// Check if any level is even loaded
	if ( !GetWorld() )
	{
		return;
	}

	Bool hitDetected = CalcClickedPosition( x, y );

	// Ask edit mode to handle event
	if ( m_tool && m_tool->HandleContextMenu( x, y, m_clickedWorldPos ) )
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->HandleContextMenu( x, y, m_clickedWorldPos ) )
		{
			return;
		}
	}

	// Show menu
	wxMenuWithSeparators menu;

	// We have a pick point client, give it priority and put its actions there
	if ( HasPickPointClient() )
	{
		// Add "set pick point" command
		menu.Append( ID_PICK_POINT_MENU_FIRST, m_pickPointCommands[0].AsChar(), wxEmptyString, false );
		menu.Connect( ID_PICK_POINT_MENU_FIRST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPickPointMenu ), NULL, this );
		
		// If we already have a pick point, show its commands
		if ( HasPickPoint() )
		{
			for ( Uint32 i=1; i < m_pickPointCommands.Size(); ++i )
			{
				menu.Append( ID_PICK_POINT_MENU_FIRST + i, m_pickPointCommands[i].AsChar(), wxEmptyString, false );
				menu.Connect( ID_PICK_POINT_MENU_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPickPointMenu ), NULL, this );
			}
		}

		menu.BeginGroup();
	}

	// Get selected entities
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );

	if ( selectedEntities.Size() )
	{
		menu.BeginGroup();
		menu.Append( ID_SHOW_IN_SCENE_EXPLORER, TXT("Show in scene explorer"), wxEmptyString, false );
		menu.Connect( ID_SHOW_IN_SCENE_EXPLORER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnShowInSceneExplorer ), NULL, this );
		menu.Append( ID_LOOK_AT_SELECTED, TXT("Look at selected"), wxEmptyString, false );
		menu.Connect( ID_LOOK_AT_SELECTED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnLookAtSelected ), NULL, this );

		// Conversion
		{
			wxMenu* convertMenu = new wxMenu();

			convertMenu->Append( ID_REMOVE_COLLISIONS, TXT("static meshes to meshes"), wxEmptyString, false );
			menu.Connect( ID_REMOVE_COLLISIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnRemoveCollisions ), nullptr, this );
			convertMenu->Append( ID_REBUILD_COLLISIONS, TXT("meshes to static meshes"), wxEmptyString, false );
			menu.Connect( ID_REBUILD_COLLISIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnRebuildCollisions ), nullptr, this );
			convertMenu->Append( ID_CONV_TO_RIGID_MESH, TXT("meshes to rigid meshes"), wxEmptyString, false );
			menu.Connect( ID_CONV_TO_RIGID_MESH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnConvertToRigidMeshes ), nullptr, this );

			menu.Append( wxID_ANY, TXT("Convert"), convertMenu );
		}

		wxMenu* replaceMenu = new wxMenu();
		replaceMenu->Append( ID_REPLACE_BY_ENTITY, TXT("entity from AssetBrowser"), wxEmptyString, false );
		menu.Connect( ID_REPLACE_BY_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnReplaceWithEntity ), NULL, this );
		replaceMenu->Append( ID_REPLACE_BY_ENTITY_COPY_COMPONENTS, TXT("entity from AssetBrowser (copy components)"), wxEmptyString, false );
		menu.Connect( ID_REPLACE_BY_ENTITY_COPY_COMPONENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnReplaceWithEntityCopyComponents ), NULL, this );
		replaceMenu->Append( ID_REPLACE_BY_MESH, TXT("mesh from AssetBrowser"), wxEmptyString, false );
		menu.Connect( ID_REPLACE_BY_MESH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnReplaceWithMesh ), NULL, this );
		replaceMenu->Append( ID_REPLACE_BY_STATIC_MESH, TXT("mesh from AssetBrowser (static)"), wxEmptyString, false );
		menu.Connect( ID_REPLACE_BY_STATIC_MESH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnReplaceWithMeshStatic ), NULL, this );
		menu.Append( wxID_ANY, TXT("Replace selection with..."), replaceMenu );
	
		// For Action Points only
		{
			CActionPoint* ap = NULL;
			for ( auto entIt = selectedEntities.Begin(); entIt != selectedEntities.End(); ++entIt )
			{
				ap = Cast< CActionPoint >( *entIt );
				if ( ap )
				{
					break;
				}
			}

			if ( ap && selectedEntities.Size() == 1) //can only preview one AP at a time
			{
				wxMenu* apMenu = new wxMenu();
				apMenu->Append( ID_PREVIEW_AP_MAN, TXT("Man"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_MAN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPMan ), NULL, this );
				apMenu->Append( ID_PREVIEW_AP_BIG_MAN, TXT("Big man"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_BIG_MAN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPBigMan ), NULL, this );
				apMenu->Append( ID_PREVIEW_AP_WOMAN, TXT("Woman"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_WOMAN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPWoman ), NULL, this );
				apMenu->Append( ID_PREVIEW_AP_DWARF, TXT("Dwarf"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_DWARF, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPDwarf ), NULL, this );
				apMenu->Append( ID_PREVIEW_AP_CHILD, TXT("Child"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_CHILD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPChild ), NULL, this );
				apMenu->Append( ID_PREVIEW_AP_SELECTED, TXT("Selected entity"), wxEmptyString, false );
				menu.Connect( ID_PREVIEW_AP_SELECTED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPreviewAPSelectedEntity ), NULL, this );
				menu.Append( wxID_ANY, TXT("Preview Action Point with..."), apMenu );
			}
		}


		// For Static Meshes only
		{
			// Check if at least one static mesh component is selected
			Bool isNavobstacleMenuAvailable = false;
			Int32 dominantCollisionType = -1;
			CNavmeshComponent* navmeshComponent = NULL;
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );
			for ( Uint32 i=0; i < nodes.Size(); ++i )
			{
				CComponent* component = Cast< CComponent >( nodes[ i ] );
				if ( component )
				{
					if ( nodes[ i ]->IsA< CNavmeshComponent > () )
					{
						navmeshComponent = static_cast< CNavmeshComponent* >( nodes[ i ] );
						continue;
					}
					PathLib::IComponent* pathlibComponent = component->AsPathLibComponent();
					if ( pathlibComponent )
					{
						PathLib::IObstacleComponent* obstacleComponent = pathlibComponent->AsObstacleComponent();
						if ( obstacleComponent )
						{
							isNavobstacleMenuAvailable = true;
							if ( dominantCollisionType == -1 )
							{
								dominantCollisionType = obstacleComponent->GetPathLibCollisionGroup();
							}
							else if ( dominantCollisionType != obstacleComponent->GetPathLibCollisionGroup() )
							{
								dominantCollisionType = -2;
							}
						}
					}
				}
			}

			if ( isNavobstacleMenuAvailable )
			{
				if ( dominantCollisionType != PLC_StaticWalkable )
				{
					menu.Append( ID_MESH_NAV_TO_STATIC_WALKABLE, TXT("Navigation: Convert to Static Walkable"), wxEmptyString, false );
					menu.Connect( ID_MESH_NAV_TO_STATIC_WALKABLE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavigationToStaticWalkable ), NULL, this );
				}
				if ( dominantCollisionType != PLC_Walkable )
				{
					menu.Append( ID_MESH_NAV_TO_WALKABLE, TXT("Navigation: Convert to Walkable"), wxEmptyString, false );
					menu.Connect( ID_MESH_NAV_TO_WALKABLE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavigationToWalkable ), NULL, this );
				}
				if ( dominantCollisionType != PLC_Static )
				{
					menu.Append( ID_MESH_NAV_TO_STATIC, TXT("Navigation: Convert to Static"), wxEmptyString, false );
					menu.Connect( ID_MESH_NAV_TO_STATIC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavigationToStatic ), NULL, this );
				}
				if ( dominantCollisionType != PLC_Dynamic )
				{
					menu.Append( ID_MESH_NAV_TO_DYNAMIC, TXT("Navigation: Convert to Dynamic"), wxEmptyString, false );
					menu.Connect( ID_MESH_NAV_TO_DYNAMIC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavigationToDynamic ), NULL, this );
				}
			}

#ifndef NO_NAVMESH_GENERATION
			// Navmesh instance only
			if ( navmeshComponent )
			{
				auto& generationRoots = navmeshComponent->GetGenerationRoots();
				// move generation root
				if ( generationRoots.Size() == 1 )
				{
					menu.Append( ID_NAVMESH_GENROOT_MOVE, TXT("Navmesh: Move generation root here"), wxEmptyString, false );
					menu.Connect( ID_NAVMESH_GENROOT_MOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavmeshMoveGenerationRoot ), NULL, this );
				}

				// add generation root
				menu.Append( ID_NAVMESH_GENROOT_ADD, TXT("Navmesh: Add generation root here"), wxEmptyString, false );
				menu.Connect( ID_NAVMESH_GENROOT_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavmeshAddGenerationRoot ), NULL, this );

				for ( Uint32 i = 0, n = generationRoots.Size(); i != n; ++i )
				{
					Float distSq = (navmeshComponent->GetGenerationRootWorldPosition( i ) - m_clickedWorldPos).SquareMag3();
					if ( distSq < (NavmeshRootPointsSelectionDistance*NavmeshRootPointsSelectionDistance) )
					{
						menu.Append( ID_NAVMESH_GENROOT_DEL, TXT("Navmesh: Delete this generation root point"), wxEmptyString, false );
						menu.Connect( ID_NAVMESH_GENROOT_DEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavmeshDelGenerationRoot ), NULL, this );
						break;
					}
				}

				menu.Append( ID_NAVMESH_GENERATE, TXT("Navmesh: Generate"), wxEmptyString, false );
				menu.Connect( ID_NAVMESH_GENERATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnNavmeshGenerate ), NULL, this );
			}
		}
#endif
		
		// For encounters only
		{
			CEncounter* encounter = NULL;
			for ( auto entIt = selectedEntities.Begin(); entIt != selectedEntities.End(); ++entIt )
			{
				encounter = Cast< CEncounter >( *entIt );
				if ( encounter )
				{
					break;
				}
			}

			if ( encounter )
			{
				menu.Append( ID_OPEN_ENCOUNTER_EDITOR, TXT("Encounter: Open Encounter Editor"), wxEmptyString, false );
				menu.Connect( ID_OPEN_ENCOUNTER_EDITOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnEncounterEditor ), NULL, this );
			}

		}

		if ( selectedEntities.Size() == 1 && selectedEntities[0]->GetEntityTemplate() != nullptr )
		{
			CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( selectedEntities[0] );
			if ( appearanceComponent != nullptr )
			{
				menu.Append( ID_CHANGE_APPEARANCE, TXT("Change appearance"), wxEmptyString, false );
				menu.Connect( ID_CHANGE_APPEARANCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnChangeAppearance ), NULL, this );
			}
		}

		if( selectedEntities.Size() > 1 && m_scene->GetActiveLayer() != NULL )
		{
			menu.Append( ID_CREATE_ENTITY_TEMPLATE, TXT("Create entity template from selection"), wxEmptyString, false );
			menu.Connect( ID_CREATE_ENTITY_TEMPLATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCreateEntityTemplate ), NULL, this );
			menu.Append( ID_CREATE_MESH_RESOURCE, TXT("Create mesh resource from selection"), wxEmptyString, false );
			menu.Connect( ID_CREATE_MESH_RESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCreateMeshResource ), NULL, this );
		}
		else if( selectedEntities.Size() == 1 && m_scene->GetActiveLayer() != NULL )
		{
			menu.Append( ID_CREATE_ENTITY_TEMPLATE, TXT("Create entity template from selection"), wxEmptyString, false );
			menu.Connect( ID_CREATE_ENTITY_TEMPLATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCreateEntityTemplate ), NULL, this );
			menu.Append( ID_CREATE_MESH_RESOURCE, TXT("Create mesh resource from selection"), wxEmptyString, false );
			menu.Connect( ID_CREATE_MESH_RESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCreateMeshResource ), NULL, this );
			menu.Append( ID_GEN_CUBEMAP, TXT("Generate cube map"), wxEmptyString, false );
			menu.Connect( ID_GEN_CUBEMAP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnGenerateCubemap ), NULL, this );
		}
		else
		{
			menu.Append( ID_GEN_CUBEMAP, TXT("Generate cube map"), wxEmptyString, false );
			menu.Connect( ID_GEN_CUBEMAP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnGenerateCubemap ), NULL, this );
		}

		if ( selectedEntities.Size() > 1 )
		{
			menu.Append( ID_ALIGN_PIVOT, TXT("Align pivot"), wxEmptyString, false );
			menu.Connect( ID_ALIGN_PIVOT,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAlignPivot ), NULL, this );
		}

		menu.Append( ID_EXTRACT_MESHES_FROM_ENTITY, TXT("Extract meshes from entities"), wxEmptyString, false );
		menu.Connect( ID_EXTRACT_MESHES_FROM_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnExtractMeshesFromEntities ), NULL, this );
		menu.Append( ID_EXTRACT_COMPONENTS_FROM_ENTITY, TXT("Extract components from entities"), wxEmptyString, false );
		menu.Connect( ID_EXTRACT_COMPONENTS_FROM_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnExtractComponentsFromEntities ), NULL, this );
		menu.Append( ID_CREATE_SHADOW_MESH, TXT("Create shadow meshes for entities"), wxEmptyString, false );
		menu.Connect( ID_CREATE_SHADOW_MESH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCreateShadowMesh ), NULL, this );

		menu.AppendSeparator();

		menu.Append( ID_HIDE_SELECTED_ENTITIES, TXT("Hide selected entities"), wxEmptyString, false );
		menu.Connect( ID_HIDE_SELECTED_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnHideSelectedEntities ), NULL, this );
		menu.Append( ID_ISOLATE_SELECTED_ENTITIES, TXT("Isolate selected entities"), wxEmptyString, false );
		menu.Connect( ID_ISOLATE_SELECTED_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnIsolateSelectedEntities ), NULL, this );
		if ( !m_hiddenEntities.Empty() )
		{
			menu.Append( ID_LIST_HIDDEN_ENTITIES, TXT("List hidden entities..."), wxEmptyString, false );
			menu.Connect( ID_LIST_HIDDEN_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnListHiddenEntities ), NULL, this );
			menu.Append( ID_REVEAL_HIDDEN_ENTITIES, TXT("Reveal hidden entities"), wxEmptyString, false );
			menu.Connect( ID_REVEAL_HIDDEN_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnRevealHiddenEntities ), NULL, this );
		}

		menu.AppendSeparator();

		// Areas
		{
			CAreaComponent* area = nullptr;
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );
			for ( Uint32 i=0; !area && i < nodes.Size(); ++i )
			{
				area = Cast< CAreaComponent >( nodes[i] );
			}

			if ( area )
			{
				menu.AppendSeparator();
				menu.Append( ID_FORCE_STREAM_IN_INSIDE_AREA, TXT("Force stream in of entities inside the area"), wxEmptyString, false );
				menu.Connect( ID_FORCE_STREAM_IN_INSIDE_AREA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnForceStreamInInsideArea ), NULL, this );
				menu.AppendSeparator();
				menu.Append( ID_IMPORT_ENTITIES_FROM_OLD_TILES, TXT("Import entities from old streaming tiles"), wxEmptyString, false );
				menu.Connect( ID_IMPORT_ENTITIES_FROM_OLD_TILES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnImportEntitiesFromOldTiles ), NULL, this );
			}
		}
	}
	else // no selection
	{
		if ( !m_hiddenEntities.Empty() )
		{
			menu.Append( ID_LIST_HIDDEN_ENTITIES, TXT("List hidden entities..."), wxEmptyString, false );
			menu.Connect( ID_LIST_HIDDEN_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnListHiddenEntities ), NULL, this );
			menu.Append( ID_REVEAL_HIDDEN_ENTITIES, TXT("Reveal hidden entities"), wxEmptyString, false );
			menu.Connect( ID_REVEAL_HIDDEN_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnRevealHiddenEntities ), NULL, this );
		}
	}

	//
	{
		const Uint32 selectedEntityCount = selectedEntities.Size();

		// Selection tools from menu bar
		wxMenu* selectMenu = new wxMenu();
		wxMenu* layerVisibility = new wxMenu();
		wxMenu* selectByMenu = new wxMenu();

		selectMenu->Append( ID_SELECT_ALL, TXT("Select All") );
		menu.Connect( ID_SELECT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnSelectAll ), nullptr, wxTheFrame );

		if ( selectedEntityCount > 0 )
		{
			selectMenu->Append( ID_SELECT_UNSELECT_ALL, TXT("Unselect All") );
			menu.Connect( ID_SELECT_UNSELECT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnUnselectAll ), nullptr, wxTheFrame );
			selectMenu->Append( ID_SELECT_INVERT_SELECTION, TXT("Invert Selection") );
			menu.Connect( ID_SELECT_INVERT_SELECTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnInvertSelection ), nullptr, wxTheFrame );

			// select by menu
			{
				selectByMenu->Append( ID_SELECT_SELECT_BY_ENTITY, TXT("Entity Template") );
				menu.Connect( ID_SELECT_SELECT_BY_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnSelectByTheSameEntityTemplate ), nullptr, wxTheFrame );
			}

			// layer visibility
			{
				layerVisibility->Append( ID_SELECT_HIDE_SELECTION, TXT("Hide Layers With Selected Objects") );
				menu.Connect( ID_SELECT_HIDE_SELECTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnHideLayersWithSelectedObjects ), nullptr, wxTheFrame );
			}
		}

		layerVisibility->Append( ID_SELECT_UNHIDE_ALL_LAYERS, TXT("Unhide All Loaded Layers") );
		menu.Connect( ID_SELECT_UNHIDE_ALL_LAYERS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnUnhideAllLoadedLayers ), nullptr, wxTheFrame );

		//
		selectByMenu->Append( ID_SELECT_SELECT_BY_TAG, TXT("Tag") );
		menu.Connect( ID_SELECT_SELECT_BY_TAG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnSelectByTheSameTag ), nullptr, wxTheFrame );

		//
		selectMenu->Append( wxID_ANY, TXT("Select By The Same"), selectByMenu );
		menu.Append( wxID_ANY, TXT("Select"), selectMenu );
		if ( selectedEntityCount > 0 )
		{
			menu.Append( ID_SELECT_CENTER, TXT("Center On Selected") );
			menu.Connect( ID_SELECT_CENTER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnCenterOnSelected ), nullptr, wxTheFrame );
		}
		menu.Append( wxID_ANY, TXT("Layer Visibility"), layerVisibility );
	}

	{
		wxMenu* selectMenu = new wxMenu();

		selectMenu->Append( ID_LOOT_MERGE_WITH_ENTITY, TXT( "Combine with loot container entity" ) );
		menu.Connect( ID_LOOT_MERGE_WITH_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnMergeLootWithEntity ), NULL, wxTheFrame );

		selectMenu->Append( ID_LOOT_REPLACE_WITH_NEW, TXT("Copy and replace with lootable") );
		menu.Connect( ID_LOOT_REPLACE_WITH_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnReplaceWithLootable ), NULL, wxTheFrame );

		selectMenu->Append( ID_LOOT_ADD_OPTIONS, TXT("Setup loot params") );
		menu.Connect( ID_LOOT_ADD_OPTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnAddLootOptions ), NULL, wxTheFrame );

		menu.Append( wxID_ANY, TXT("Looting"), selectMenu  );
	}

	// Grouping options
	{
		if ( selectedEntities.Size() > 0 )
		{
			// Check if in selected entities are any groups
			Bool areGroups = false;
			Bool isInGroup = false;
			CEntityGroup* entityGroup = nullptr;
			for( TDynArray< CEntity* >::const_iterator entityIter = selectedEntities.Begin();
				entityIter != selectedEntities.End(); ++entityIter )
			{
				if( ( *entityIter )->IsA< CEntityGroup >() == true )
				{
					entityGroup = Cast< CEntityGroup >( *entityIter );
					areGroups = true;
					break;
				}
				if( ( *entityIter )->GetPartOfAGroup() == true )
				{
					isInGroup = true;
					entityGroup = ( *entityIter )->GetContainingGroup();
					break;
				}
			}

			if( areGroups == true || isInGroup == true )
			{
				wxMenu* groupMenu = new wxMenu();

				if( areGroups == true )
				{
					groupMenu->Append( ID_UNGROUP_ITEMS, TXT("Ungroup"), wxEmptyString, false );
					menu.Connect( ID_UNGROUP_ITEMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnUngroupItems ), nullptr, this );
				}

				if( entityGroup->IsLocked() == false || isInGroup == true )
				{
					groupMenu->Append( ID_LOCK_GROUP, TXT("Lock"), wxEmptyString, false );
					menu.Connect( ID_LOCK_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnLockGroup ), nullptr, this );

					if( isInGroup == true )
					{
						groupMenu->Append( ID_REMOVE_FROM_GROUP, TXT("Remove from group"), wxEmptyString, false );
						menu.Connect( ID_REMOVE_FROM_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnRemoveFromGroup ), nullptr, this );
					}
				}
				else
				{
					groupMenu->Append( ID_UNLOCK_GROUP, TXT("Unlock"), wxEmptyString, false );
					menu.Connect( ID_UNLOCK_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnUnlockGroup ), nullptr, this );
				}

				menu.Append( wxID_ANY, TXT("Group"), groupMenu  );
			}
			else
			{
				if( selectedEntities.Size() > 1 )
				{
					menu.Append( ID_GROUP_ITEMS, TXT("Group"), wxEmptyString, false );
					menu.Connect( ID_GROUP_ITEMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnGroupItems ), nullptr, this );
				}
			}
		}
	}

	// Edit
	Bool activeLayerOptionEnable = false;
	if( selectedEntities.Size() == 1 )
	{
		activeLayerOptionEnable = true;
	}
	else if( selectedEntities.Size() > 1 )	// this case is important when someone selected group
	{
		if( selectedEntities[0]->GetPartOfAGroup() == true )
		{
			activeLayerOptionEnable = true;
		}
		else if( selectedEntities[0]->IsA< CEntityGroup >() == true )
		{
			activeLayerOptionEnable = true;
		}
	}

	if ( activeLayerOptionEnable == true )
	{
		CEntity* entity = selectedEntities[ 0 ];
		menu.BeginGroup();
		menu.Append( ID_ACTIVATE_LAYER, String::Printf( TXT("Activate %s"), entity->GetLayer()->GetFriendlyName().AsChar() ).AsChar(), wxEmptyString, false );
		menu.Connect( ID_ACTIVATE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnActivateEntityLayer ), NULL, this );
	}

	// Adding object
	if ( m_scene->GetActiveLayer() )
	{
		// Creating objects
		String resourcePath;
		GetActiveResource( resourcePath );

		// Add templates options: selected resource
		menu.BeginGroup();		
		TemplateInfo* info = m_templates[0];
		if ( info->SupportsResource( resourcePath ))
		{
			// Determine name
			String name = String( TXT("Add ") ) + info->m_name;
			if ( !info->m_requiredExtension.Empty() && !resourcePath.Empty() )
			{
				name += String::Printf( TXT(" using '%s'"), resourcePath.AsChar() );
			}

			// Add menu option
			menu.Append( ID_SPAWN_TEMPLATE_FIRST + 0, name.AsChar() );
			menu.Connect( ID_SPAWN_TEMPLATE_FIRST + 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnSpawnHere ), NULL, this );
		}

		// Add templates options: standard resources
		menu.BeginGroup();
		
		CMenuItem rootMenuItem;
		TDynArray< Int32 > ungrouppedIndices;
		for ( Uint32 i = 1; i < m_templates.Size(); i++ )
		{
			TemplateInfo* info = m_templates[i];
			if( info->m_group != String::EMPTY )
			{
				rootMenuItem.Insert( info->m_group, i );
			}
			else
			{
				ungrouppedIndices.PushBack( i );
			}
		}

		rootMenuItem.PopulateMenu( &menu, &menu, m_templates, resourcePath, this );

		// Sticker menu
		{
			wxMenu* stickerMenu = new wxMenu();

			stickerMenu->Append( ID_STICKER_MENU, TXT("Add custom sticker...") );
			menu.Connect( ID_STICKER_MENU, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerFromContextMenu ),
				new TCallbackData< String >( String::EMPTY ) , this );

			for ( Uint32 i = 0; i < m_stickersTexts.Size(); i++ )
			{
				stickerMenu->Append( ID_STICKER_MENU + 1 + i, String(TXT("Add sticker: '") + m_stickersTexts[i] + TXT("'")).AsChar() );
				menu.Connect( ID_STICKER_MENU + 1 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerFromContextMenu ),
					new TCallbackData< String >( m_stickersTexts[i] ), this );
			}

			menu.Append( wxID_ANY, TXT("Sticker"), stickerMenu );
		}
		
		// Ungrouped
		menu.BeginGroup();
		for ( Uint32 c=0; c<ungrouppedIndices.Size(); c++ )
		{
			Int32 index = ungrouppedIndices[c];
			TemplateInfo* info = m_templates[index];
			if ( info->SupportsResource( resourcePath ))
			{
				// Add menu option
				String name = String( TXT("Add ") ) + info->m_name;
				menu.Append( ID_SPAWN_TEMPLATE_FIRST + index, name.AsChar() );
				menu.Connect( ID_SPAWN_TEMPLATE_FIRST + index, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnSpawnHere ), NULL, this );
			}
		}
	}

	// Selection - copy/cut/delete
	if ( selectedEntities.Size() )
	{
		menu.BeginGroup();

		// Clipboard
		menu.BeginGroup();
		menu.Append( ID_EDIT_COPY_WP, TXT("Copy") );
		menu.Connect( ID_EDIT_COPY_WP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCopy ), NULL, this );
		menu.Append( ID_EDIT_CUT_WP, TXT("Cut") );
		menu.Connect( ID_EDIT_CUT_WP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnCut ), NULL, this );
		menu.Append( ID_EDIT_DELETE_WP, TXT("Delete") );
		menu.Connect( ID_EDIT_DELETE_WP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnDelete ), NULL, this );
	}

	// Paste menu
	if ( hitDetected && GObjectClipboard.HasObjects() )
	{
		menu.BeginGroup();
		menu.Append( ID_EDIT_PASTE_WP, TXT("Paste here") );
		menu.Connect( ID_EDIT_PASTE_WP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPasteHere ), NULL, this );
		menu.Append( ID_EDIT_PASTE_ALIGNED_WP, TXT("Paste aligned here") );
		menu.Connect( ID_EDIT_PASTE_ALIGNED_WP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnPasteAlignedHere ), NULL, this );
	}

	// Layers  menu
#if 0
	{
		wxMenu* layersMenu = new wxMenu();
		CComponent* selected = NULL;

		// Selected object layer
		if ( selectedComponents.Size() )
		{
			selected = selectedComponents[0];
		}

		// World layers
		CWorld* world = GetWorld();
		if ( world )
		{
			TDynArray< CLayerInfo* > layers;
			world->GetLayers( layers );
			
			for ( Uint32 i=0; i < layers.Size(); i++ )
			{
				CLayerInfo* layer = layers[i];
				if ( layer->IsLoaded() )
				{
					wxMenuItem* wm = layersMenu->Append( ID_LAYER_CHANGE + i, layer->GetName().AsChar(), wxEmptyString, wxITEM_CHECK );
					menu.Connect( ID_LAYER_CHANGE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnChangeLayer ), new LayerToSet( layer ) , this );

					if ( m_scene->GetActiveLayer() == layer->GetLayer() )
					{
						wm->Check( true);
					}	

					// insert selected object's layer on top
					if ( selected && selected->GetLayer() == layer->GetLayer() )
					{
						layersMenu->Insert( 0, ID_LAYER_FROMSELOBJ, TXT("From Selected Object: [ ") + wxString( layer->GetName().AsChar()  ) + TXT(" ]") );
						menu.Connect( ID_LAYER_FROMSELOBJ, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnChangeLayer ), new LayerToSet( layer ) , this );
						layersMenu->InsertSeparator( 1 );
					}
				}
			}
		}

		menu.BeginGroup();
		menu.Append( wxID_ANY, TXT("Change Layer"), layersMenu  );
	}
#endif

	// Debug menu
	{
		wxMenu* debugMenu = new wxMenu();

		debugMenu->Append( ID_DEBUG_MENU + 4, TXT("Add shadows to all meshes") );
		menu.Connect( ID_DEBUG_MENU + 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::Debug_AddShadowsToAllMeshes ), NULL, this);	

		
		// DEBUG MIPMAPS DISABLES
		/*
		m_debugMipmaps.Clear();

		wxMenu* mipMapsMenu = new wxMenu();
		CollectMipmaps(mipMapsMenu, m_debugMipmaps);
		debugMenu->Append(wxID_ANY, TXT("Debug MipMaps"), mipMapsMenu);
		for ( Uint32 i = 0; i < m_debugMipmaps.Size(); i++)
		{
			menu.Connect(ID_DEBUG_MENU + 5 + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler ( CEdWorldEditPanel::DebugMipMaps ), NULL, this);
		}
		//there are m_debugMipmaps.Size() items added, so last used id = (ID_DEBUG_MENU + 4 + m_debugMipmaps.Size())
		debugMenu->Append(ID_DEBUG_MENU + 5 + m_debugMipmaps.Size(), TXT("Reset debug MipMaps"));
		menu.Connect( ID_DEBUG_MENU + 5 + m_debugMipmaps.Size(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::ResetDebugMipMaps ), NULL, this);
		*/

		menu.BeginGroup();
		menu.Append( wxID_ANY, TXT("Debug"), debugMenu  );
	}


	// Resources
	if ( selectedEntities.Size() )
	{
		Bool templatesInUse = false;

		// Get resources used by selected entities
		TDynArray< CResource* > usedResources;
		for ( Uint32 i=0; i<selectedEntities.Size(); i++ )
		{
			CEntity* entity = selectedEntities[i];
			SEntityStreamingState streamingState;
			entity->PrepareStreamingComponentsEnumeration( streamingState, true );
			entity->ForceFinishAsyncResourceLoads();
			entity->CollectUsedResources( usedResources );
			entity->FinishStreamingComponentsEnumeration( streamingState );

			if ( entity->GetTemplate() != NULL )
				templatesInUse = true;
		}

		// We have some resources, show menu
		if ( usedResources.Size() )
		{
			wxMenu* subMenu = new wxMenu();

			// Add items
			for ( Uint32 i=0; i<usedResources.Size(); i++ )
			{
				CResource* resource = usedResources[i];
				subMenu->Append( ID_SELECT_RESOURCE_FIRST + i, resource->GetFriendlyName().AsChar() );
				menu.Connect( ID_SELECT_RESOURCE_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnShowResource ), new ResourceWrapper( resource ), this);
			}

			// Resources
			menu.Append( wxID_ANY, TXT("Resources"), subMenu );
		}

		if (templatesInUse)
		{
			menu.Append( ID_DETACH_TEMPLATES, TXT("Detach templates") );
			menu.Connect( ID_DETACH_TEMPLATES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnDetachTemplates ), NULL, this );
		}
	}

	// Show the popup
	PopupMenu( &menu );
}

void CEdWorldEditPanel::OnEditEntity( wxCommandEvent& event )
{
	// Get selected entities
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );

	// Spawn editor
	SpawnEntityEditor( selectedEntities[0] );
}

void CEdWorldEditPanel::OnActivateEntityLayer( wxCommandEvent& event )
{
	// Get selected entities
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );

	// Change active layer
	m_scene->ChangeActiveLayer( selectedEntities[0]->GetLayer()->GetLayerInfo() );
	m_scene->UpdateSelected();
}

void CEdWorldEditPanel::OnLookAtSelected( wxCommandEvent& event )
{
	LookAtSelectedNodes();
}

void CEdWorldEditPanel::OnRemoveCollisions( wxCommandEvent& event )
{
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );

	const Uint32 numNodes = nodes.Size();
	for ( Uint32 i=0; i < numNodes; ++i )
	{
		CNode* node = nodes[ i ];
		if ( node->IsA< CStaticMeshComponent > () && node->GetLayer() )
		{
			if ( node->GetLayer()->MarkModified() && node->MarkModified() )
			{
				CEntity *entity = Cast< CComponent > ( node )->GetEntity();
				if ( entity )
					entity->ConvertAllStaticMeshesToMeshes();
			}
			else
			{
				GFeedback->ShowWarn( TXT("Unable to convert") );
				return;
			}
		}
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );
}

void CEdWorldEditPanel::OnRebuildCollisions( wxCommandEvent& event )
{
	TDynArray< CNode* > nodes;
	TDynArray< CEntity* > meshEntities;
	GetSelectionManager()->GetSelectedNodes( nodes );

	// Collect en(.)(.)
	const Uint32 numNodes = nodes.Size();
	for ( Uint32 i=0; i < numNodes; ++i )
	{
		CNode* node = nodes[ i ];
		if ( node->IsA< CMeshComponent > () )
		{
			CEntity *entity = Cast< CComponent > ( node )->GetEntity();
			if ( entity && entity->GetLayer() )
			{
				if ( entity->GetLayer()->MarkModified() && node->MarkModified() )
				{
					meshEntities.PushBackUnique( entity );
				}
				else
				{
					GFeedback->ShowWarn( TXT("Unable to convert") );
					return;
				}
			}
		}
	}

	// Rebuild collisions
	const Uint32 numEntities = meshEntities.Size();
	for ( Uint32 k=0; k < numEntities; ++k )
	{
		meshEntities[ k ]->ConvertAllStaticMeshesToMeshes();
		meshEntities[ k ]->ConvertAllMeshesToStatic();
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );
}

void CEdWorldEditPanel::OnConvertToRigidMeshes( wxCommandEvent& event )
{
	TDynArray< CNode* > nodes;
	TDynArray< CEntity* > meshEntities;
	GetSelectionManager()->GetSelectedNodes( nodes );

	// Collect en(.)(.)
	const Uint32 numNodes = nodes.Size();
	for ( Uint32 i=0; i < numNodes; ++i )
	{
		CNode* node = nodes[ i ];
		if ( node->IsA< CMeshComponent > () )
		{
			CEntity *entity = Cast< CComponent > ( node )->GetEntity();
			if ( entity && entity->GetLayer() )
			{
				if ( entity->GetLayer()->MarkModified() && node->MarkModified() )
				{
					meshEntities.PushBackUnique( entity );
				}
				else
				{
					GFeedback->ShowWarn( TXT("Unable to convert") );
					return;
				}
			}
		}
	}

	// Rebuild collisions
	const Uint32 numEntities = meshEntities.Size();
	for ( Uint32 k=0; k < numEntities; ++k )
	{
		meshEntities[ k ]->ConvertAllStaticMeshesToMeshes();
		meshEntities[ k ]->ConvertAllMeshesToRigidMeshes();
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );
}

void CEdWorldEditPanel::OnNavigationConvert( wxCommandEvent& event, EPathLibCollision collisionGroup )
{
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );

	// Collect entities
	const Uint32 numNodes = nodes.Size();
	for ( Uint32 i=0; i < numNodes; ++i )
	{
		CNode* node = nodes[ i ];

		CComponent* component = Cast< CComponent >( node );

		if ( component && component->GetLayer() )
		{
			PathLib::IComponent* pathlibComponent = component->AsPathLibComponent();

			if ( pathlibComponent )
			{
				PathLib::IObstacleComponent* obstacleComponent = pathlibComponent->AsObstacleComponent();

				if ( obstacleComponent )
				{
					if ( component->GetLayer()->MarkModified()  && component->MarkModified() )
					{
						obstacleComponent->SetPathLibCollisionGroup( collisionGroup );
					}
					else
					{
						GFeedback->ShowWarn( TXT("Unable to convert") );
						return;
					}
				}
			}
		}
	}
	CDrawableComponent::RenderingSelectionColorChangedInEditor();
}
void CEdWorldEditPanel::OnShadowCasting( Bool isLocal )
{
	TDynArray< CComponent* > nodes;
	GetSelectionManager()->GetSelectedComponentsFiltered( ClassID<CMeshComponent>(), nodes );
	
	// Collect entities
	const Uint32 numNodes = nodes.Size();
	for ( Uint32 i=0; i < numNodes; ++i )
	{
		CNode* node = nodes[ i ];
		
		CMeshComponent* component = Cast< CMeshComponent >( node );
		if ( component && component->GetLayer() )
		{
			if ( !isLocal )
			{
				if ( component->IsCastingShadows() )
				{
					component->EnableDrawableFlags( false, DF_CastShadows );
				} 
				else 
				{
					component->EnableDrawableFlags( true, DF_CastShadows );
				}		
			} 
			else 
			{
				if ( component->IsCastingShadowsFromLocalLightsOnly() )
				{
					component->EnableDrawableFlags( false, DF_CastShadowsFromLocalLightsOnly );
				} 
				else 
				{
					component->EnableDrawableFlags( true, DF_CastShadowsFromLocalLightsOnly );
				}				
			}
			component->RefreshRenderProxies();
		} 
		else 
		{
			GFeedback->ShowWarn( TXT("Unable to convert") );
			return;
		}
	}
	CDrawableComponent::RenderingSelectionColorChangedInEditor();
}

void CEdWorldEditPanel::OnNavigationToStaticWalkable( wxCommandEvent& event )
{
	OnNavigationConvert( event, PLC_StaticWalkable );
}
void CEdWorldEditPanel::OnNavigationToWalkable( wxCommandEvent& event )
{
	OnNavigationConvert( event, PLC_Walkable );
}
void CEdWorldEditPanel::OnNavigationToStatic( wxCommandEvent& event )
{
	OnNavigationConvert( event, PLC_Static );
}
void CEdWorldEditPanel::OnNavigationToDynamic( wxCommandEvent& event )
{
	OnNavigationConvert( event, PLC_Dynamic );
}

extern Uint32	GetEnvProbeDataSourceResolution();
extern Uint32	GetEnvProbeDataSourceFaceSize();
extern void		BuildEnvProbeCamera( CRenderCamera &camera, const Vector &origin, Uint32 faceIndex );
extern Bool		GrabEnvProbeFaceBuffers( const CRenderCamera &camera, const GpuApi::Rect* srcRect, Uint32 supersampleFactor, void *bufferData, Bool grabNonShadowData, Int32 shadowChannelIndex, Int32 shadowBitIndex, TDynArray<Float> &refSuperSamplingWeights );

void GenerateEnvProbe( CEnvProbeComponent* envProbe )
{
	if ( !envProbe )
	{
		return;
	}

	const Uint32 envProbeSize = GetEnvProbeDataSourceResolution();
	const Uint32 envProbeGrabSize = 8 * envProbeSize;
	ASSERT( envProbeGrabSize <= 1024 && "Will not fit in typical fullHD monitor, so expect some data corruption / crash" );

	const Uint32 oldSizeX = GGame->GetViewport()->GetWidth();
	const Uint32 oldSizeY = GGame->GetViewport()->GetHeight();
	
	const Uint32 faceEnvProbeBufferSize = GetEnvProbeDataSourceFaceSize();
	TDynArray<Uint8> facesData ( 6 * faceEnvProbeBufferSize );
	TDynArray<Float> supersamplingWeights;

	// Begin feedback task
	GFeedback->BeginTask( TXT("Generating Environment Probe"), false );

	const Vector probeGenOrigin = envProbe->GetProbeOrigin(); //< using probeOrigin instead of probeGenOrigin because we're interested in current probe position here!

	Bool success = true;
	for ( Uint32 face_i = 0; face_i < 6 && success; ++face_i )
	{
		for ( Uint32 timepoint_i=0, num_timepoints=4*8; timepoint_i<num_timepoints && success; ++timepoint_i )
		{
			// Update feedback task
			GFeedback->UpdateTaskProgress( face_i * num_timepoints + timepoint_i, 6 * num_timepoints );

			// Init frame info
			CRenderFrameInfo frameInfo( GGame->GetViewport() );
			{
				frameInfo.m_clearColor = Color::BLACK;

				frameInfo.m_allowPostSceneRender = false;
				frameInfo.m_forceGBufferClear = true;
				frameInfo.m_present = false;

				frameInfo.SetShadowConfig( GGame->GetActiveWorld()->GetShadowConfig() );

				frameInfo.m_width = envProbeGrabSize;
				frameInfo.m_height = envProbeGrabSize;
				GGame->GetViewport()->AdjustSize(envProbeGrabSize,envProbeGrabSize);

				frameInfo.SetShowFlag( SHOW_VisualDebug, false );
			}

			// Render the scene
			CRenderFrame *compiledFrame = GGame->GetActiveWorld()->GenerateFrame( GGame->GetViewport(), frameInfo );
			if ( !compiledFrame )
			{
				success = false;
				break;
			}
			CRenderFrameInfo& info = const_cast<CRenderFrameInfo&>( compiledFrame->GetFrameInfo() );

			// ace_hack: override some frameInfo params because they were initiated during GenerateFrame call
			{	
				// camera params
				BuildEnvProbeCamera( info.m_camera, probeGenOrigin, face_i );
				info.m_occlusionCamera = info.m_camera;

				// setup light direction - hacky, but sufficient for shadows generation
				info.m_baseLightingParameters.m_lightDirection = GGame->GetActiveWorld()->GetEnvironmentParameters().m_globalLightingTrajectory.GetLightDirection( 24 * 60 * 60 * (timepoint_i / (Float)num_timepoints) );

				// disable clouds rendering (we have a 0/1 shadowmask, so clouds shadows must be included at a different stage)
				info.m_envParametersGame.m_displaySettings.m_allowCloudsShadow = false;

				// disable global water. it's not rendered do gbuffers, but lays depth so we don't want it.
				info.m_envParametersGame.m_displaySettings.m_allowWaterShader = false; 
			}

			( new CRenderCommand_RenderScene( compiledFrame, GGame->GetActiveWorld()->GetRenderSceneEx() ) )->Commit();

			// Grab the frame
			{
				// Flush rendering
				GRender->Flush();

				GpuApi::Rect rect;
				rect.left = 0;
				rect.top = 0;
				rect.bottom = envProbeGrabSize;
				rect.right = envProbeGrabSize;

				ASSERT( 0 == envProbeGrabSize % envProbeSize );
				VERIFY( GrabEnvProbeFaceBuffers( info.m_camera, &rect, envProbeGrabSize / envProbeSize, facesData.TypedData() + face_i * faceEnvProbeBufferSize, 0 == timepoint_i, timepoint_i / 8, timepoint_i % 8, supersamplingWeights ) );
			}

			// Release frame
			SAFE_RELEASE( compiledFrame );
		}
	}

	// End feedback tack
	GFeedback->EndTask();

	// Generate envProbe
	if ( success )
	{
		envProbe->SetFacesBuffers( probeGenOrigin, facesData.Size(), facesData.Data() );
		envProbe->CreateRenderResource( true );
	}

	// Resore old viewport
	GGame->GetViewport()->AdjustSize( oldSizeX, oldSizeY );
}

void CEdWorldEditPanel::OnGenerateCubemap( wxCommandEvent& event )
{
	TDynArray< CEntity* > nodes;
	GetSelectionManager()->GetSelectedEntities( nodes );
	if ( nodes.Empty() )
	{
		return;
	}

	// Handle the env probe
	if ( nodes[0]->GetComponents().Size() == 1 && nodes[0]->GetComponents()[0]->IsA<CEnvProbeComponent>() )
	{
		CEnvProbeComponent *envProbe = static_cast< CEnvProbeComponent* >( nodes[0]->GetComponents()[0] );
		GenerateEnvProbe( envProbe );
		return;
	}
}

void CEdWorldEditPanel::OnNavmeshAddGenerationRoot( wxCommandEvent& event )
{
	CNavmeshComponent* navmeshComponent = NULL;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; i < nodes.Size(); ++i )
	{
		if ( nodes[ i ]->IsA< CNavmeshComponent > () )
		{
			navmeshComponent = static_cast< CNavmeshComponent* >( nodes[ i ] );
			continue;
		}
	}
	if ( navmeshComponent )
	{
		Vector localPos = m_clickedWorldPos - navmeshComponent->GetWorldPositionRef();
		navmeshComponent->GetGenerationRoots().PushBack( localPos );
	}
}
void CEdWorldEditPanel::OnNavmeshMoveGenerationRoot( wxCommandEvent& event )
{
	CNavmeshComponent* navmeshComponent = NULL;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; i < nodes.Size(); ++i )
	{
		if ( nodes[ i ]->IsA< CNavmeshComponent > () )
		{
			navmeshComponent = static_cast< CNavmeshComponent* >( nodes[ i ] );
			continue;
		}
	}
	if ( navmeshComponent )
	{
		auto& generationRoots = navmeshComponent->GetGenerationRoots();
		if ( generationRoots.Size() == 1 )
		{
			Vector localPos = m_clickedWorldPos - navmeshComponent->GetWorldPositionRef();
			generationRoots[ 0 ] = localPos;
		}
	}
}
void CEdWorldEditPanel::OnNavmeshDelGenerationRoot( wxCommandEvent& event )
{
	CNavmeshComponent* navmeshComponent = NULL;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; i < nodes.Size(); ++i )
	{
		if ( nodes[ i ]->IsA< CNavmeshComponent > () )
		{
			navmeshComponent = static_cast< CNavmeshComponent* >( nodes[ i ] );
			continue;
		}
	}
	if ( navmeshComponent )
	{
		auto& generationRoots = navmeshComponent->GetGenerationRoots();
		for ( Uint32 i = 0, n = generationRoots.Size(); i != n; ++i )
		{
			Float distSq = (navmeshComponent->GetGenerationRootWorldPosition( i ) - m_clickedWorldPos).SquareMag3();
			if ( distSq < (NavmeshRootPointsSelectionDistance*NavmeshRootPointsSelectionDistance) )
			{
				generationRoots.RemoveAt( i );
				break;
			}
		}
	}
}

void CEdWorldEditPanel::OnNavmeshGenerate( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = NULL;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; i < nodes.Size(); ++i )
	{
		if ( nodes[ i ]->IsA< CNavmeshComponent > () )
		{
			navmeshComponent = static_cast< CNavmeshComponent* >( nodes[ i ] );
			continue;
		}
	}
	if ( navmeshComponent )
	{
		navmeshComponent->GenerateNavmeshAsync();
	}
#endif
}

void CEdWorldEditPanel::OnPickPointMenu( wxCommandEvent& event )
{
	Int32 index = event.GetId() - ID_PICK_POINT_MENU_FIRST;

	switch ( index )
	{
	case 0:	// Set point
		m_pickPoint = m_clickedWorldPos;
		m_pickPointNormal = m_clickedWorldNormal;
		m_hasPickPoint = true;
		if ( HasPickPointClient() )
		{
			m_pickPointClient->OnWorldPickPointAction( WPPA_SET );
		}
		break;
	case 1: // Clear point
		m_hasPickPoint = false;
		if ( HasPickPointClient() )
		{
			m_pickPointClient->OnWorldPickPointAction( WPPA_CLEAR );
		}
		break;
	default:
		if ( index >= 2 && HasPickPointClient() ) // extra command
		{
			m_pickPointClient->OnWorldPickPointCommand( index - 2 );
		}
	}
}

void CEdWorldEditPanel::OnGroupItems( wxCommandEvent& event )
{
	CWorld* world = GetWorld();
	if( world == NULL )
	{
		return;
	}

	// Check if all selected entities belong to the same layer
	TDynArray< CEntity* > allEntities;
	TDynArray< CEntity* > skipEntities;
	CLayer* layer = NULL;
	GetSelectionManager()->GetSelectedEntities( allEntities );
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if ( ( layer && layer != (*entityIter)->GetLayer() ) || GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) )
		{
			skipEntities.PushBackUnique( *entityIter );
		}
		layer = (*entityIter)->GetLayer();
	}

	// Confirm with the user that there might be entities that will be skipped
	if ( skipEntities.Size() > 0 )
	{
		String msg = TXT("You cannot group entities that belong to existing groups or different layers. The following entities will not be included in the group: ");
		for ( Uint32 i=0; i<skipEntities.Size(); ++i )
		{
			if ( i > 0 )
			{
				msg += TXT(", ");
			}
			msg += skipEntities[i]->GetName();
		}
		msg += TXT(". Do you want to continue?");
		if ( !YesNo( msg.AsChar() ) ) return;
	}

	// Get selected entities
	TDynArray< CEntity* > entities;
	allEntities.Clear();
	GetSelectionManager()->GetSelectedEntities( allEntities );

	// Skip entities that belongs to other groups
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) == NULL )
		{
			entities.PushBack( *entityIter );
		}
	}

	// If nothing selected do nothing
	if( entities.Size() == 0 )
	{
		return;
	}

	// Ask for name
	String groupName = InputBox( this, TXT( "Group name" ), TXT( "Enter group name:" ), TXT( "NewGroup" ) );

	// Create new group
	EntitySpawnInfo info;
	info.m_entityClass = CEntityGroup::GetStaticClass();
	info.m_name = groupName;
	info.m_spawnPosition = entities[ 0 ]->GetPosition();
	info.m_spawnRotation = entities[ 0 ]->GetRotation();
	info.AddHandler( new TemplateInfo::CSpawnEventHandler() );

	// Add entities to the group
	CEntityGroup* newGroup = Cast< CEntityGroup >( layer->CreateEntitySync( info ) );
	if( newGroup == NULL )
	{
		wxMessageBox( TXT( "This name is already used" ), TXT( "Error" ) );
		return;
	}

	newGroup->AddEntities( entities );
	
	// create undo step
	TDynArray< CEntityGroup* > groups;
	groups.PushBack( newGroup );
	CUndoGroupObjects::CreateGroupStep( *wxTheFrame->GetUndoManager(), groups );

	// Add new group to the selection (so it can be copied/cloned with the selection)
	GetSelectionManager()->Select( newGroup, true );

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );
}

void CEdWorldEditPanel::OnUngroupItems( wxCommandEvent& event )
{
	CWorld* world = GetWorld();
	if( world == NULL )
	{
		return;
	}

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	TDynArray< CEntityGroup* > groupEntities;
	GetSelectionManager()->GetSelectedEntities( allEntities );

	// Skip non-group entities and groups that belongs to other groups
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ! ( *entityIter)->IsA< CEntityGroup >() )
		{
			continue;
		}

		if( GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) == nullptr )
		{
			groupEntities.PushBack( Cast< CEntityGroup >( *entityIter ) );
		}
	}

	// If nothing selected do nothing
	if( groupEntities.Size() == 0 )
	{
		return;
	}

	// create undo step
	CUndoGroupObjects::CreateUngroupStep( *wxTheFrame->GetUndoManager(), groupEntities );

	// Delete selected groups
	for( TDynArray< CEntityGroup* >::iterator entityIter = groupEntities.Begin();
		entityIter != groupEntities.End(); ++entityIter )
	{
		CEntityGroup* group = ( *entityIter );
		group->Destroy();
	}

	GetSelectionManager()->DeselectAll();

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

void CEdWorldEditPanel::OnLockGroup( wxCommandEvent& event )
{
	CWorld* world = GetWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	GetSelectionManager()->GetSelectedEntities( allEntities );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->IsA< CEntityGroup >() )
		{
			entityGroup = Cast< CEntityGroup >( *entityIter );
			break;
		}
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			break;
		}
	}

	if( entityGroup != nullptr )
	{
		entityGroup->Lock();
		// create undo step
		CUndoLockGroupObjects::CreateStep( *wxTheFrame->GetUndoManager(), entityGroup, false );
	}

	GetSelectionManager()->DeselectAll();
}

void CEdWorldEditPanel::OnUnlockGroup( wxCommandEvent& event )
{
	CWorld* world = GetWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	GetSelectionManager()->GetSelectedEntities( allEntities );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->IsA< CEntityGroup >() )
		{
			entityGroup = Cast< CEntityGroup >( *entityIter );
			break;
		}
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			break;
		}
	}

	if( entityGroup != nullptr )
	{
		entityGroup->Unlock();
		// create undo step
		CUndoLockGroupObjects::CreateStep( *wxTheFrame->GetUndoManager(), entityGroup, true );
	}

	GetSelectionManager()->DeselectAll();
}

void CEdWorldEditPanel::OnRemoveFromGroup( wxCommandEvent& event )
{
	CWorld* world = GetWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	GetSelectionManager()->GetSelectedEntities( allEntities );

	// create undo step
	CUndoRemoveObjectFromGroup::CreateStep( *wxTheFrame->GetUndoManager(), allEntities, true );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			entityGroup->DeleteEntity( ( *entityIter ) );

			// group cannot be empty - if is empty should be removed
			if( entityGroup->IsEmpty() == true )
			{
				entityGroup->Destroy();
			}

			entityGroup = nullptr;
		}
	}

	GetSelectionManager()->DeselectAll();

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

void CEdWorldEditPanel::ReplaceEntitiesWithMesh( const TDynArray< CEntity* >& entities, Bool createStaticMesh )
{
	if ( !GetWorld() || entities.Empty() )
	{
		return;
	}

	String resourcePath;
	if ( !GetActiveResource( resourcePath ) )
	{
		GFeedback->ShowError( TXT("Please select valid mesh in asset browser first.") );
		return;
	}

	CResource *resource = GDepot->LoadResource( resourcePath );
	if ( !resource || !resource->IsA< CMesh >() )
	{
		GFeedback->ShowError( TXT("Couldn't load '%s'. Please select valid mesh in asset browser first."), resourcePath.AsChar() );
		return;
	}

	// Use the same template, as when spawning meshes manually
	C2dArray *list = LoadResource< C2dArray >( EDITOR_TEMPLATES_CSV );
	String typeName = createStaticMesh ? TXT("Graphics.Static Mesh") : TXT("Graphics.Mesh");
	String templatePath = list->GetValue( TXT("GroupPath"), typeName, TXT("Resource") );
	if ( templatePath.Empty() )
	{
		GFeedback->ShowError( TXT("Replace by mesh: couln't find the proper template path."), resourcePath.AsChar() );
		return;
	}

	// Deselect all entities to create a proper selection undo step BEFORE any destroy undo action is done
	GetSelectionManager()->Deselect( CastArray< CNode >( entities ) );

	TSet< CLayer* > lockedLayers;

	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		CLayer* layer = entity->GetLayer();
		ASSERT( layer );

		// if layer has file, then it must be able to be modified - otherwise it cannot be changed

		if ( lockedLayers.Find( layer ) != lockedLayers.End() )
		{
			continue;
		}

		if ( layer->GetFile() && !layer->GetFile()->MarkModified() )
		{
			lockedLayers.Insert( layer );
			continue;
		}

		EntitySpawnInfo sinfo;
		sinfo.m_spawnPosition = entity->GetWorldPosition();
		sinfo.m_resource = resource;
		sinfo.m_template = LoadResource< CEntityTemplate >( templatePath );
		sinfo.m_detachTemplate = true;
		sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );

		if ( CEntity *created = layer->CreateEntitySync( sinfo ) )
		{
			created->SetRotation( entity->GetRotation() );
			created->SetScale( entity->GetScale() );
			created->ForceUpdateTransformNodeAndCommitChanges();
			created->ForceUpdateBoundsNode();

			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), created, true );
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, false );
		}
	}

	CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager(), TXT("replace with mesh") );
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );

	GetWorld()->RequestStreamingUpdate();

	if ( !lockedLayers.Empty() )
	{
		String layers;
		for ( auto layerIt = lockedLayers.Begin(); layerIt != lockedLayers.End(); ++layerIt )
		{
			layers += (*layerIt)->GetFriendlyName() + TXT("\n");
		}

		GFeedback->ShowMsg( TXT("Replace by mesh"), TXT("Couldn't replace some entities due to following layers being checked in:\n\n%s"), layers.AsChar() );
	}
}

void CEdWorldEditPanel::ReplaceEntitiesWithEntity( const TDynArray< CEntity* >& entities, Bool copyComponents )
{
	if ( !GetWorld() || entities.Empty() )
	{
		return;
	}

	String resourcePath;
	if ( !GetActiveResource( resourcePath ) )
	{
		GFeedback->ShowError( TXT("Please select valid entity template in asset browser first.") );
		return;
	}

	TDynArray< CEntity* > createdEntities;
	ReplaceEntitiesWithEntity( resourcePath, entities, copyComponents, createdEntities );
}


void CEdWorldEditPanel::ReplaceEntitiesWithEntity( const String& resourcePath, const TDynArray< CEntity* >& entities, Bool copyComponents, TDynArray< CEntity* >& createdEntities )
{
	TemplateInfo* ti = m_templates[ 0 ];
	if ( !ti->SupportsResource( resourcePath ) )
	{
		GFeedback->ShowError( TXT("Couldn't load '%s'. Please select valid entity template in asset browser first."), resourcePath.AsChar() );
		return;
	}

	CResource *resource = GDepot->LoadResource( resourcePath );
	if ( !resource )
	{
		GFeedback->ShowError( TXT("Couldn't load '%s'. Please select valid entity template in asset browser first."), resourcePath.AsChar() );
		return;
	}

	// Deselect all entities to create a proper selection undo step BEFORE any destroy undo action is done
	GetSelectionManager()->Deselect( CastArray< CNode >( entities ) );

	TSet< CLayer* > lockedLayers;

	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		CLayer* layer = entity->GetLayer();

		if ( lockedLayers.Find( layer ) != lockedLayers.End() )
		{
			continue;
		}

		if ( layer->GetFile() && !layer->GetFile()->MarkModified() )
		{
			lockedLayers.Insert( layer );
			continue;
		}

		if ( CEntity* created = ti->Create( layer, entity->GetPosition(), resource ) )
		{
			createdEntities.PushBack( created );
			created->SetRotation( entity->GetRotation() );
			created->SetScale( entity->GetScale() );
			created->SetTags( entity->GetTags() );
			created->ForceUpdateTransformNodeAndCommitChanges();
			created->ForceUpdateBoundsNode();

			if ( copyComponents )
			{
				// Remove all components from created entity
				TDynArray< CComponent* > oldComponents;
				CollectEntityComponents( created, oldComponents );
						
				for( TDynArray< CComponent* >::const_iterator componentIter = oldComponents.Begin();
					componentIter != oldComponents.End(); ++componentIter )
				{
					CComponent* component = *componentIter;
					ASSERT( component != NULL );

					component->Destroy();
				}

				// Copy components from old entity to created one
				TDynArray< CComponent* > components;
				CollectEntityComponents( entities[ i ], components );

				for( TDynArray< CComponent* >::const_iterator componentIter = components.Begin();
					componentIter != components.End(); ++componentIter )
				{
					CComponent* component = *componentIter;
					ASSERT( component != NULL );

					entity->RemoveComponent( component );
					GetWorld()->DelayedActions();
					created->AddComponent( component );
				}
			}

			// special case for grouping entities
			CEntityGroup* group = entity->GetContainingGroup();
			if( group != nullptr )
			{
				group->AddEntity( created );
			}

			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), created, true );
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, false );
		}
	}

	CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager(), TXT("replace with entity") );
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );

	GetWorld()->RequestStreamingUpdate();

	if ( !lockedLayers.Empty() )
	{
		String layers;
		for ( auto layerIt = lockedLayers.Begin(); layerIt != lockedLayers.End(); ++layerIt )
		{
			layers += (*layerIt)->GetFriendlyName() + TXT("\n");
		}

		GFeedback->ShowMsg( TXT("Replace by mesh"), TXT("Couldn't replace some entities due to following layers being checked in:\n\n%s"), layers.AsChar() );
	}
}

void CEdWorldEditPanel::OnReplaceWithEntity( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();
	ReplaceEntitiesWithEntity( entities, false );
}

void CEdWorldEditPanel::OnReplaceWithEntityCopyComponents( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();
	ReplaceEntitiesWithEntity( entities, true );
}

void CEdWorldEditPanel::OnReplaceWithMesh( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();
	ReplaceEntitiesWithMesh( entities, false );
}

void CEdWorldEditPanel::OnReplaceWithMeshStatic( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities = GetSelectionManager()->GetSelectedEntities();
	ReplaceEntitiesWithMesh( entities, true );
}

CLayer* CEdWorldEditPanel::CheckActiveLayer()
{
	// Check that there is active layer
	CLayer* layer = m_scene->GetActiveLayer();
	if ( !layer )
	{
		return NULL;
	}

	// Valid layer
	return layer;
}

void CEdWorldEditPanel::HideEntities( const TDynArray< CEntity* >& entities )
{
	for ( CEntity* entity : entities )
	{
		entity->DetachFromWorld( GetWorld() );
		m_hiddenEntities.Insert( entity );
	}
}

void CEdWorldEditPanel::IsolateEntities( const TDynArray< CEntity* >& entities )
{
	// Make a hash set out of them for fast checking
	THashSet< CEntity* > entitiesSet = DynArrayToHashSet( entities );

	// Hide all attached entities except the selected
	for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CEntity* entity = *it;
		if ( !entitiesSet.Exist( entity ) )
		{
			//entity->SuspendRendering( true );
			entity->DetachFromWorld( GetWorld() );
			m_hiddenEntities.Insert( entity );
		}
	}
}

void CEdWorldEditPanel::RevealEntities( const TDynArray< CEntity* >& entities )
{
	for ( CEntity* entity : entities )
	{
		entity->AttachToWorld( GetWorld() );
		m_hiddenEntities.Erase( entity );
	}
}

void CEdWorldEditPanel::RevealEntities()
{
	// Collect valid entities to array
	TDynArray< CEntity* > entities;
	for ( THandle< CEntity > entity : m_hiddenEntities )
	{
		if ( entity.IsValid() )
		{
			entities.PushBack( entity );
		}
	}
	m_hiddenEntities.Clear();

	// Reveal them
	RevealEntities( entities );
}

void CEdWorldEditPanel::PasteEntities( const Vector* relativePosition, Bool align )
{
	// We need a layer to paste into
	CLayer* layer = CheckActiveLayer();
	if ( !layer )
	{
		wxMessageBox( wxT("Please activate a layer first"), wxT("No layer to paste"), wxCENTRE|wxOK, this );
		return;
	}

	// Check if the layer is part of the world
    if ( !layer->IsAttached() )
    {
		wxMessageBox( wxT("Cannot paste entities on a layer which is not attached to the world."), wxT("Wrong layer"), wxCENTRE|wxOK, this );
        return;
    }

	// Check if the layer can be modified
	if ( !layer->IsModified() && layer->GetFile() && !layer->GetFile()->Edit() )
	{
		return;
	}

	// Get objects from the clipboard
	TDynArray< CObject* > clipboardObjects;
	if ( !GObjectClipboard.Paste( clipboardObjects ) ) return;
	if ( clipboardObjects.Size() == 0 ) return;

	// Calculate spawn position and rotation
	CEntity* firstEntity = NULL;

	// Destroy current selection
	GetSelectionManager()->DeselectAll();

	// Keep a list of pasted entities
	TDynArray< CEntity* > pastedEntities;

	// Add entities in the layer
	m_scene->Freeze();
	for ( Uint32 i=0; i<clipboardObjects.Size(); ++i )
	{
		// The clipboard may contain non CEntity objects - discard them
		CEntity* entity = Cast<CEntity>( clipboardObjects[i] );
		if ( !entity ) {
			clipboardObjects[i]->Discard();
			continue;
		}
		if ( !firstEntity ) firstEntity = entity;

		// Make sure the local to world matrix is set
		entity->SetLocalToWorldFromTransformDirectly();

		// Move and rotate the entity
		if ( relativePosition )
		{
			entity->SetPosition( (*relativePosition) + entity->GetWorldPosition() - firstEntity->GetWorldPosition() );
		}

		// Generate unique name
		entity->SetName( layer->GenerateUniqueEntityName( entity->GetName() ) );

		// Inform the entity that it was pasted
		entity->OnPasted( layer );

		// Add to layer
		entity->ClearFlag( NF_Attached ); // needed because the pasted item contains the original flags
		layer->AddEntity( entity );

		// Update streaming
		entity->CreateStreamedComponents( SWN_NotifyWorld );
		entity->UpdateStreamedComponentDataBuffers();
		entity->UpdateStreamingDistance();

		// Update transform
		entity->ForceUpdateTransformNodeAndCommitChanges();
		entity->ForceUpdateBoundsNode();

		// Select the entity
		GetSelectionManager()->Select( entity );

		// Add to the pasted entities
		pastedEntities.PushBack( entity );

		// Add undo step
		CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, true );
	}

	// Did we actually paste anything?
	if ( !pastedEntities.Size() )
	{
		// Inform the user if we didn't paste anything
		wxMessageBox( wxT("Couldn't find any entities among the objects in the clipboard"), wxT("No entities to paste"), wxCENTRE|wxOK, this );
	}
	else
	{
		// otherwise, finish undo stuff and mark the layer as modified
		CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
		layer->MarkModified();
		m_scene->UpdateSelected( false );
	}

	// Align entities if needed
	if ( align )
	{
		AlignEntitiesToPointAndNormal( pastedEntities, m_clickedWorldPos, m_clickedWorldNormal, true, false );
	}

	// Finish
	m_scene->Thaw();
}

void CEdWorldEditPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Selection has changed
	if ( name == RED_NAME( SelectionChanged ) || name == RED_NAME( LayerSelectionChanged ) )
	{
        const auto& eventData = GetEventData< CSelectionManager::SSelectionEventData >( data );

		if ( eventData.m_world == GetWorld() )
		{
			if ( m_enableSelectionUndo )
			{
				if ( !m_undoManager->IsUndoInProgress() 
					// Do not create selection step if there is some other awaiting undo step, pushing the step here would drop it
					&& m_undoManager->GetStepToAdd() == nullptr 
					)
				{
					CUndoSelection::CreateStep( *m_undoManager, *GetSelectionManager(), eventData.m_transactionId );
				}
			}

			UpdateViewportWidgets();

			if ( name == RED_NAME( SelectionChanged ) )
			{
				RunLaterOnce( [&]() { RefreshSelectionFreeSpaceMesh(); } );
			}
		}
	}
	else if ( name == RED_NAME( EditorPostUndoStep ) )
	{
		const CEdUndoManager* undoManager = GetEventData< CEdUndoManager* >( data );
		if ( undoManager == m_undoManager )
		{
			GetSelectionManager()->RefreshPivot();
		}
	}
	else if ( name == RED_NAME( EditorPropertyPostChange ) || name == RED_NAME( EditorPropertyChanging ) )
	{
		if( GGame->GetActiveWorld() == GetWorld() && GetWorld() != nullptr )
		{
			const CEdPropertiesPage::SPropertyEventData& propertyEventData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
			if( propertyEventData.m_propertyName == CName( TXT("transform") ) )
			{
				GetSelectionManager()->RefreshPivot();
				if ( m_selFreeSpaceShow )
				{
					RefreshSelectionFreeSpaceMesh();
				}
			}
		}
	}
	else if ( name == RED_NAME( FileSaved ) )
	{
		String path = GetEventData< String >( data );
		if ( path == STICKERS_CSV )
		{
			InitStickers();
		}
	}
	else if ( !GGame->IsActive() && name == RED_NAME( Attached ) )
	{
		CObject* obj = GetEventData< CObject* >( data );
		if ( CEntity * entity = Cast< CEntity >( obj ) )
		{
			m_scene->AddSceneObject( entity );
		}
	}
	else if ( !GGame->IsActive() && name == RED_NAME( Detached ) )
	{
		CObject * obj = GetEventData< CObject* >( data );
		if ( CEntity * entity = Cast< CEntity >( obj ) )
		{
			m_scene->RemoveSceneObject( entity );
		}
	}
	else if ( name == RED_NAME( ActiveWorldChanging ) || name == RED_NAME( GameEnding ) )
	{
		if ( m_shapesContainer )
		{
			delete m_shapesContainer;
			m_shapesContainer = nullptr;
		}

		if ( m_transformManager )
		{
			delete m_transformManager;
			m_transformManager = nullptr;
		}
	}
}

void CEdWorldEditPanel::UpdateSelectionSensitiveTools()
{

}

void CEdWorldEditPanel::UpdateViewportWidgets()
{
	// Get selected entities
	Bool hasSelection = false;
	if ( GetWorld() )
	{
		TDynArray< CNode* > nodes = GetSelectionManager()->GetSelectedNodes();
		hasSelection = nodes.Size() > 0;
	}

	// Toggle widgets
	m_widgetManager->EnableWidgets( hasSelection );
}

void CEdWorldEditPanel::SpawnEntityEditor( CEntity* entity )
{
	ASSERT( entity );

	// Editor already exists, show it
	wxWindow* editor = NULL;
	if ( m_entityEditors.Find( entity, editor ) )
	{
		ASSERT( editor );
		editor->Show( true );
		editor->SetFocus();
		return;
	}

	// Attach
	if ( editor )
	{
		m_entityEditors.Insert( entity, editor );
		editor->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdWorldEditPanel::OnEntityEditorClosed ), NULL, this );
	}
}

void CEdWorldEditPanel::CloseEntityEditors( CEntity* entity )
{
	for ( THashMap< CEntity*, wxWindow* >::iterator i = m_entityEditors.Begin(); i != m_entityEditors.End(); ++i )
	{
		if ( i->m_first == entity )
		{
			wxWindow* window = i->m_second;
			m_entityEditors.Erase( entity );
			delete window;
			break;
		}
	}
}

void CEdWorldEditPanel::OnEntityEditorClosed( wxCloseEvent& event )
{
	wxWindow* window = ( wxWindow* ) event.GetEventObject();
	for ( THashMap< CEntity*, wxWindow* >::iterator i = m_entityEditors.Begin(); i != m_entityEditors.End(); ++i )
	{
		if ( i->m_second == window )
		{
			wxWindow* window = i->m_second;
			m_entityEditors.Erase( i->m_first );
			event.Skip();
			break;
		}
	}
}


void CEdWorldEditPanel::Debug_AddShadowsToAllMeshes( wxCommandEvent& event )
{
	if( GFeedback->AskYesNo(TXT("This will add shadow casting to ALL THE MESHES. Are you sure?")) )
	{
		for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			CDrawableComponent *drawable = Cast< CDrawableComponent > ( *it );
			if ( drawable )
			{
				drawable->SetCastingShadows( true );
			}
		}
	}	
}

void CEdWorldEditPanel::OnShowResource( wxCommandEvent& event )
{
	ResourceWrapper* wrapper = static_cast< ResourceWrapper* >( event.m_callbackUserData );
	if ( wrapper )
	{
		CResource* res = wrapper->m_resource;
		if ( res && res->GetFile() )
		{
			String resPath = res->GetFile()->GetDepotPath();
			SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( resPath ) );
		}
	}
}

void CEdWorldEditPanel::OnDetachTemplates( wxCommandEvent& event )
{
	// Confirm with the user
	if ( !YesNo( wxT("Are you sure that you want to detach the selected entities from their templates? This cannot be undone!") ) )
	{
		return;
	}

	// Detatch
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );
	for ( Uint32 i=0; i<selectedEntities.Size(); i++ )
	{
		if ( selectedEntities[i]->MarkModified() )
		{
			// The following lines will move streamed components from the entity
			// template's streaming buffers to the entity's own streaming buffers
			selectedEntities[i]->SetStreamingLock( false );
			selectedEntities[i]->DestroyStreamedComponents( SWN_NotifyWorld );				// The world will think we're gone
			selectedEntities[i]->CreateStreamedComponents( SWN_DoNotNotifyWorld );			// Bring all components in memory
			selectedEntities[i]->DetachTemplate();
			selectedEntities[i]->UpdateStreamedComponentDataBuffers();						// Rebuild buffers
			selectedEntities[i]->UpdateStreamingDistance();
			selectedEntities[i]->DestroyStreamedComponents( SWN_DoNotNotifyWorld );			// Kill streamed components
			selectedEntities[i]->CreateStreamedComponents( SWN_NotifyWorld );				// Bring them back with the world's blessing
		}
	}
}

void CEdWorldEditPanel::OnAddStickerFromContextMenu( wxCommandEvent& event )
{
	TCallbackData< String >* data = dynamic_cast< TCallbackData< String >* >( event.m_callbackUserData );
	CLayer* layer = CheckActiveLayer();
	if ( data && 
		( !layer || ( !layer->GetFile() || layer->GetFile()->MarkModified() )))// if layer has file, then it must be able to be modified - otherwise it cannot be changed)
	{
		// Load resource
		if ( CResource* resource = LoadResource< CResource >( STICKER_TEMPLATE_PATH ) )
		{
			String stickerText;
			if( data->GetData() == String::EMPTY )
			{
				if ( !InputBox( this, TXT("Sticker message"), TXT("Write the sticker message"), stickerText, true ) )
					return;
			}
			else
			{
				stickerText = data->GetData();
			}

			// Create entity
			CEntity *newEntity;
			if ( layer )
			{
				newEntity = TemplateInfo( TXT("Sticker"), String::EMPTY, STICKER_TEMPLATE_PATH, String::EMPTY, String::EMPTY, true ).Create( layer, m_clickedWorldPos, resource );
			}
			else
			{
				wxMessageBox( wxT("Please select a layer"), wxT("Error"), wxOK|wxCENTRE );
			}

			ASSERT( newEntity && TXT("Could not create Sticker entity") );

			TDynArray< CComponent* > components;
			CollectEntityComponents( newEntity, components );
			CStickerComponent *sticker = NULL;
			if ( components.Size() == 1 && (sticker = Cast< CStickerComponent >( components[0] )) )
			{
				sticker->SetText( stickerText );
				wxTheFrame->AddToIsolated( newEntity );

				CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
				CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
			}
		}
	}
}

void CEdWorldEditPanel::OnAddStickerInCurrentMousePos( wxCommandEvent& event )
{
	// Calculate cursor position
	POINT cursorPoint;
	::GetCursorPos( &cursorPoint );
	::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
	
	if ( CalcClickedPosition( cursorPoint.x, cursorPoint.y ) )
	{
		OnAddStickerFromContextMenu( event );
	}
}

TEdShortcutArray* CEdWorldEditPanel::GetAccelerators()
{
	if ( m_shortcuts.Empty() )
	{
		m_shortcuts.PushBack( SEdShortcut(TXT("Stickers\\Add custom sticker"), wxAcceleratorEntry( 0, 0, ID_STICKER_MENU ) ) );

		for ( Uint32 i = 0; i < m_stickersTexts.Size(); i++ )
		{
			m_shortcuts.PushBack( SEdShortcut( 
									String( TXT("Stickers\\Add sticker: '") + m_stickersTexts[i] + TXT("'")).AsChar(),
									wxAcceleratorEntry( 0, 0, ID_STICKER_MENU + 1 + i ))
								);
		}
	}

	return &m_shortcuts;
}

void CEdWorldEditPanel::InitStickers()
{
	m_stickersTexts.Clear();

	Connect( ID_STICKER_MENU, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( String::EMPTY ), this );

	if ( C2dArray *stickersData = Cast< C2dArray >( GDepot->LoadResource( STICKERS_CSV ) ) )
	{
		Uint32 colSize, rowSize;
		stickersData->GetSize( colSize, rowSize );
		if ( colSize > 0 )
		{
			for ( Uint32 row = 0; row < rowSize; row++ )
			{
				String stickerText = stickersData->GetValue( 0, row );
				m_stickersTexts.PushBack( stickerText );
				Connect( ID_STICKER_MENU + 1 + row, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( stickerText ), this );
			}
		}
	}
	else
	{
		// Use default stickers if resource is not available
		WARN_EDITOR( TXT("Stickers resource not available - using default values.") );
		Connect( ID_STICKER_MENU+1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( TXT("TODO") ), this );
		Connect( ID_STICKER_MENU+2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( TXT("Error 1") ), this );
		Connect( ID_STICKER_MENU+3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( TXT("Error 2") ), this );
		Connect( ID_STICKER_MENU+4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnAddStickerInCurrentMousePos ), new TCallbackData< String >( TXT("Error 3") ), this );
		m_stickersTexts.PushBack( TXT("TODO") );
		m_stickersTexts.PushBack( TXT("Error 1") );
		m_stickersTexts.PushBack( TXT("Error 2") );
		m_stickersTexts.PushBack( TXT("Error 3") );
	}
}

void CEdWorldEditPanel::DeleteEntities( const TDynArray< CEntity* >& entities, Bool requireUserConfirmation /* = true */ )
{
	// Ask edit mode to handle event
	if ( m_tool && m_tool->OnDelete() ) 
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnDelete() )
		{
			return;
		}
	}

	// Delete selected entities
	CWorld* world = GetWorld();
	if ( world )
	{
		// Count entities, but not groups
		Uint32 count = 0;
		for ( auto entityIter = entities.Begin(); entityIter != entities.End(); ++entityIter )
		{
			CEntityGroup* group = Cast< CEntityGroup >( ( *entityIter ) );
			if( group != nullptr && group->IsLocked() == false && group->IsEmpty() == false )
			{
				continue;
			}
			else
			{
				++count;
			}
		}

		if ( count )
		{
			// Ask user
			if ( requireUserConfirmation && !YesNo( TXT("Sure to delete %i entit%s ?"), count, count == 1 ? TXT("y") : TXT("ies") ) )
			{
				// Not sure
				LOG_EDITOR( TXT("Don't bother me again dude !") );
				return;
			}

			// Deselect all entities to create a proper selection undo step BEFORE any destroy undo action is done
			GetSelectionManager()->Deselect( CastArray< CNode >( entities ) );

			{
				for ( CEntity* entity : entities )
				{
					if ( entity->MarkModified() )
					{
#ifdef USE_UMBRA
						world->NotifyOcclusionSystem( entity );
#endif // USE_UMBRA
						CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, false );
					}
				}

				CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
			}
		}
	}
}

Bool CEdWorldEditPanel::IsSpawnTreeBeingEdited( CObject* ownerObject )
{
	CEdEncounterEditor* spawnTreeEditor;

	return m_spawnTreeEditors.Find( ownerObject, spawnTreeEditor );
}

void CEdWorldEditPanel::OpenSpawnTreeEditor( CObject* ownerObject )
{
	CEdEncounterEditor* spawnTreeEditor;
	Bool isEncounter = ownerObject->IsA< CEncounter >();
	Bool isSpawnTree = !isEncounter &&  ownerObject->IsA< CSpawnTree >();
	if ( !isEncounter && !isSpawnTree )
	{
		return;
	}
	if ( !m_spawnTreeEditors.Find( ownerObject, spawnTreeEditor ) )
	{
		spawnTreeEditor = isEncounter ? new CEdEncounterEditor( NULL, static_cast< CEncounter* >( ownerObject ) ) : new CEdEncounterEditor( NULL, static_cast< CSpawnTree* >( ownerObject ) );
		m_spawnTreeEditors.Insert( ownerObject, spawnTreeEditor );
		spawnTreeEditor->Bind( wxEVT_DESTROY, &CEdWorldEditPanel::OnEncounterEditorClosed, this );
	}

	ASSERT ( spawnTreeEditor != NULL );
	spawnTreeEditor->Show();
	spawnTreeEditor->SetFocus();
	spawnTreeEditor->Raise();
}

void CEdWorldEditPanel::RefreshSelectionFreeSpaceMesh()
{
	// If the visualization is locked or we're not showing the free space, do nothing
	if ( m_selFreeSpaceLock || !m_selFreeSpaceShow )
	{
		return;
	}

	// Remove previous mesh
	m_selFreeSpaceMesh.Clear();
	m_selFreeSpaceMeshIndices.Clear();
	
	// Collect selected entities
	TDynArray< CEntity* > entities;
	if ( GetWorld() )
	{
		GetSelectionManager()->GetSelectedEntities( entities );
	}
	if ( entities.Empty() )
	{
		return;
	}

	// Collect points from the meshes
	TDynArray< Vector > points;
	for ( CEntity* entity : entities )
	{
		for ( CComponent* component : entity->GetComponents() )
		{
			CMeshComponent* meshComponent;
			if ( ( meshComponent = Cast< CMeshComponent >( component ) ) != nullptr )
			{
				TDynArray< DebugVertex > vertices;
				TDynArray< Uint32 > indices;

				// Collect vertex data from the collision mesh
				const CCollisionMesh* collisionMesh = meshComponent->GetMeshNow() ? meshComponent->GetMeshNow()->GetCollisionMesh() : nullptr;
				if ( collisionMesh )
				{
					const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
					for ( ICollisionShape* shape : shapes )
					{
						shape->GenerateDebugMesh( vertices, indices );
					}
				}

				// Insert new vertices into the points
				for ( Uint32 index : indices )
				{
					Vector v( vertices[index].x, vertices[index].y, vertices[index].z );
					points.PushBack( meshComponent->GetLocalToWorld().TransformPoint( v /* + v.Normalized3()*1.6 */ ) );
				}
			}
		}
	}

	// No points
	if ( points.Empty() )
	{
		return;
	}

	// Calculate tight 2D convex hull
	TDynArray< Uint32 > hullIndices;
	if ( !ComputeQuickHull2D( points, hullIndices ) )
	{
		return;
	}

	// Expand convex hull area to cover 1.4 meters from each surface
	// (this is done by duplicating the hull points for every edge normal
	// of the hull - it is a bit shitty way to do it, but these hulls wont
	// have many points and the other way, calculating the planes, pushing
	// them towards their normals 1.4 meters and recalculating the convex hull
	// from the common area behind the planes, is a bit more complicated)
	TDynArray< Vector > expandedPoints;
	for ( Uint32 i=1; i <= hullIndices.Size(); ++i )
	{
		Plane edgePlane( 
			Vector( points[hullIndices[i - 1]].X, points[hullIndices[i - 1]].Y, 0 ),
			Vector( points[hullIndices[i%hullIndices.Size()]].X, points[hullIndices[i%hullIndices.Size()]].Y, 0 ),
			Vector( points[hullIndices[i - 1]].X, points[hullIndices[i - 1]].Y, 1 )
		);
		Vector normal = edgePlane.NormalDistance.Normalized3();
		for ( Uint32 index : hullIndices )
		{
			expandedPoints.PushBack( points[index] + normal*1.4f );
		}
	}
	points.SwapWith( expandedPoints );

	// Calculate convex hull of expanded points
	if ( !ComputeQuickHull2D( points, hullIndices ) )
	{
		return;
	}
	
	// Set Z to selection pivot's Z
	Float selectionZ = GetSelectionManager()->GetPivotPosition().Z + 0.12f;
	for ( Vector& p : points )
	{
		p.Z = selectionZ;
	}

	// Build the mesh
	TEasyMeshBuilder< DebugVertex, Uint16 > meshBuilder;
	meshBuilder.Begin();
	for ( Uint32 i=2; i < hullIndices.Size(); ++i )
	{
		static const Color theColor( 126, 207, 205, 80 );
		meshBuilder.Triangle(
			DebugVertex( points[hullIndices[0]], theColor ),
			DebugVertex( points[hullIndices[i]], theColor ),
			DebugVertex( points[hullIndices[i - 1]], theColor )
			);
	}
	meshBuilder.End();

	// Store the new mesh
	m_selFreeSpaceMesh.Grow( meshBuilder.GetVertexCount() );
	Red::System::MemoryCopy( m_selFreeSpaceMesh.Data(), meshBuilder.GetVertexArray(), sizeof(DebugVertex)*meshBuilder.GetVertexCount() );
	m_selFreeSpaceMeshIndices.Grow( meshBuilder.GetIndexCount() );
	Red::System::MemoryCopy( m_selFreeSpaceMeshIndices.Data(), meshBuilder.GetIndexArray(), sizeof(Uint16)*meshBuilder.GetIndexCount() );
}

void CEdWorldEditPanel::ShowFreeSpaceVisualization( Bool show )
{
	if ( show != m_selFreeSpaceShow )
	{
		m_selFreeSpaceShow = show;
		RefreshSelectionFreeSpaceMesh();
	}
}

void CEdWorldEditPanel::LockFreeSpaceVisualization( Bool lock )
{
	if ( lock != m_selFreeSpaceLock )
	{
		m_selFreeSpaceLock = lock;
		RefreshSelectionFreeSpaceMesh();
	}
}

void CEdWorldEditPanel::SetPickPointClient( CEdWorldPickPointClient* client, const String& addCommand, const String& removeCommand, const TDynArray<String>* extraCommands )
{
	ClearPickPointClient();
	m_pickPointClient = client;
	m_pickPointCommands.PushBack( addCommand );
	m_pickPointCommands.PushBack( removeCommand );
	if ( extraCommands != nullptr )
	{
		m_pickPointCommands.PushBack( *extraCommands );
	}
}

void CEdWorldEditPanel::ClearPickPointClient( CEdWorldPickPointClient* client /* = nullptr */ )
{
	if ( client && client != m_pickPointClient )
	{
		return;
	}

	if ( m_pickPointClient && m_hasPickPoint )
	{
		m_pickPointClient->OnWorldPickPointAction( WPPA_CLEAR );
	}
	m_pickPointClient = nullptr;
	m_pickPointCommands.Clear();
	m_hasPickPoint = false;
}

void CEdWorldEditPanel::OnCameraMoved()
{
	struct Local
	{
		static Float ReduceAngle( Float degrees )
		{
			while ( degrees > 180.f )
				degrees -= 360.f;
			while ( degrees < -180.f )
				degrees += 360.f;
			return degrees;
		};
	};

	wxTheFrame->DisplayCameraTransform( String::Printf( TXT("Camera  x: %.2f  y: %.2f  z: %.2f  yaw: %.2f  pitch: %.2f  roll: %.2f"),
			m_cameraPosition.X, m_cameraPosition.Y, m_cameraPosition.Z,
			Local::ReduceAngle( m_cameraRotation.Yaw ), 
			Local::ReduceAngle( m_cameraRotation.Pitch ), 
			Local::ReduceAngle( m_cameraRotation.Roll ) ).AsChar() );
	
	if ( GetWorld() )
	{
		// Process area triggers
		/*{
			GetWorld()->GetTriggerSystem()->FireEditorEvents( m_prevCameraPosition, m_cameraPosition, 1.0f );
		}*/

		// Update camera
		m_prevCameraPosition = m_cameraPosition;
		GetWorld()->UpdateCameraPosition( m_cameraPosition );
		GetWorld()->UpdateCameraForward( m_cameraRotation.TransformVector( Vector(0,1,0,0) ) );
		GetWorld()->UpdateCameraUp( m_cameraRotation.TransformVector( Vector(0,0,1,0) ) );
		
		// Refresh mesh coloring if the current coloring scheme requires it
		if ( GEngine->GetMeshColoringRefreshOnCameraMove() )
		{
			CDrawableComponent::RenderingSelectionColorChangedInEditor();
		}
	}

	// Send to connected console
	wxTheFrame->OnCameraMoved( m_cameraPosition, m_cameraRotation );
}

void CEdWorldEditPanel::OnPacketReceived( const AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, BLACKBOX_NETWORK_CHANNEL ) == 0 )
	{
		UniChar instruction[ 32 ];
		RED_VERIFY( packet.ReadString( instruction, ARRAY_COUNT_U32( instruction ) ) );

		if( Red::System::StringCompare( instruction, TXT( "CameraToOverviewPoint" ) ) == 0 )
		{
			CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				Float normalizedX;
				Float normalizedY;

				packet.Read( normalizedX );
				packet.Read( normalizedY );

				CClipMap* clipMap = world->GetTerrain();
				if ( clipMap )
				{
					SClipmapParameters params;
					clipMap->GetClipmapParameters( &params );

					Vector terrainCorner = clipMap->GetTerrainCorner();
					Vector position( params.terrainSize*normalizedX + terrainCorner.X, params.terrainSize*normalizedY + terrainCorner.Y, 0 );
					clipMap->GetHeightForWorldPositionSync( position, 0, position.Z );
					position.Z += 16.0f;

					m_cameraPosition = position;
					m_cameraRotation.Pitch = 270.0f;
					m_cameraRotation.Yaw = -90.0f;
					m_cameraRotation.Roll = 0.0f;
					OnCameraMoved();
				}
			}
		}

		if( Red::System::StringCompare( instruction, TXT( "CameraToCoordinates" ) ) == 0 )
		{
			CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				Float X;
				Float Y;

				packet.Read( X );
				packet.Read( Y );

				CClipMap* clipMap = world->GetTerrain();
				if ( clipMap )
				{
					SClipmapParameters params;
					clipMap->GetClipmapParameters( &params );

					Vector terrainCorner = clipMap->GetTerrainCorner();
					Vector position( X,Y, 0 );
					clipMap->GetHeightForWorldPositionSync( position, 0, position.Z );
					position.Z += 16.0f;

					m_cameraPosition = position;
					m_cameraRotation.Pitch = 270.0f;
					m_cameraRotation.Yaw = 0.0f;
					m_cameraRotation.Roll = 0.0f;
					OnCameraMoved();
				}
			}
		}
	}
}

// Bool CEdWorldEditPanel::ProcessDebugRequest( CName requestType, const CNetworkPacket& packet, CNetworkPacket& response )
// {
// 	if ( requestType == CNAME( SetWorldEditorViewportCameraView ) )
// 	{
// 		if ( ParseCameraViewString( packet.ReadString(), m_cameraPosition, m_cameraRotation ) )
// 		{
// 			GWorldCameraBookmarks.SetBookmark( 0, m_cameraPosition, m_cameraRotation );
// 			SSwappedBookmark = SLastUsedBookmark;
// 			SLastUsedBookmark = 0;
// 			OnCameraMoved();
// 		}
// 		return true;
// 	}
// 	else if ( requestType == CNAME( MoveWorldEditorViewportToOverviewPoint ) )
// 	{
// 		String str = packet.ReadString();
// 		TDynArray<String> parts = str.Split( TXT(" "), true );

// 		return true;
// 	}
// 	return false;
// }

void CEdWorldEditPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	//#ifndef NO_RED_GUI
	//	if ( GGame->IsActive() ==  false )
	//	{
	//		if ( GRedGui::GetInstance().GetEnabled() == true )
	//		{
	//#ifndef NO_DEBUG_WINDOWS
	//			if( GDebugWin::GetInstance().GetVisible() == true )
	//			{
	//				// Update debug windows
	//				GRedGui::GetInstance().OnTick( timeDelta );

	//				if( GRedGui::GetInstance().GetInputManager()->CursorIsInViewport() == true )
	//				{
	//					view->SetMouseMode( MM_Capture );
	//					GRedGui::GetInstance().GetInputManager()->ShowCursor();
	//				}
	//			}	
	//#endif	// NO_DEBUG_WINDOWS
	//		}
	//	}
	//#endif	// NO_RED_GUI	
	
	CEdRenderingPanel::OnViewportTick( view, timeDelta );
}

Bool CEdWorldEditPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef NO_RED_GUI
	if ( GGame->IsActive() ==  false )
	{
#ifndef NO_DEBUG_WINDOWS
		if( key == IK_F8 && action == IACT_Press )
		{
			if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
			{
				GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );
				GDebugWin::GetInstance().SetVisible(!GRedGui::GetInstance().GetEnabled());

				if( GDebugWin::GetInstance().GetVisible() == false )
				{
					view->SetMouseMode( MM_Normal );
				}
				else
				{
					GRedGui::GetInstance().SetBudgetMode( false );
				}
			}
			else
			{
				Bool turningOn = !GDebugWin::GetInstance().IsDebugWindowVisible( DebugWindows::DW_SceneStats );
				GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );

				GRedGui::GetInstance().SetBudgetMode( turningOn );

				if (  turningOn )
				{
					GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_SceneStats );
				}
				else
				{
					GDebugWin::GetInstance().HideDebugWindow( DebugWindows::DW_SceneStats );
				}
			}
		}
#endif	// NO_DEBUG_WINDOWS
		if( GRedGui::GetInstance().GetEnabled() == true )
		{
			if( GRedGui::GetInstance().OnViewportInput( view, key, action, data ) == true )
			{
				return true;
			}
		}
	}
#endif	// NO_RED_GUI

	if ( GCommonGame ->OnViewportInputDebugInEditor( view, key, action, data ) )
	{
		return true;
	}

	// Toggle guides
	if ( key == IK_G && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Alt ) && !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_drawGuides = !m_drawGuides;
	}

	// Merge or whatever
	if ( key == IK_M && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
	{
		Int32 target = 2;
		Bool usePosition = true, useRotation = true, useScale	= false, 
			useXTrans	= true, useYTrans	= true, useZTrans	= true, 
			useXRotate	= true, useYRotate	= true, useZRotate	= true,
			useXScale	= true, useYScale	= true, useZScale	= true;
		
		TDynArray< CEntity* > entities;
		TDynArray< CNode* > entsAsNodes;
		if ( GetWorld() )
		{
			GetSelectionManager()->GetSelectedEntities( entities );
		}

		for ( Uint32 entIt = 0; entIt < entities.Size(); )
		{
			entsAsNodes.PushBack( entities[ entIt ] );
			if ( entities[ entIt ]->GetContainingGroup() != nullptr )
			{
				entities.RemoveAt(entIt);
			}
			else
			{
				++entIt;
			}
		}

		if ( !entities.Empty() )
		{
			CEntity* alignTarget = nullptr;
			Int32 button = FormattedDialogBox( wxT("Align Selection"), wxT("H{R'Align to'('First''Last''Center')*1|V'Use'{X'Position'X'Rotation'X'Scale'}*1}V'Apply to...'{T{'Position:'H{|||X'X'|X'Y'|X'Z'}'Rotation:'H{|||X'X'|X'Y'|X'Z'}'Scale:'H{|||X'X'|X'Y'|X'Z'}}}|H{~B@'OK'B'Cancel'}"), 
												&target, &usePosition, &useRotation, &useScale, &useXTrans, &useYTrans, &useZTrans, &useXRotate, &useYRotate, &useZRotate, &useXScale, &useYScale, &useZScale );
				
			if ( button == 0 )
			{
				// Create undo step
				CUndoTransform::CreateStep( *(wxTheFrame->GetUndoManager()), entsAsNodes );

				// Pre-change events
				for ( Int32 i=entsAsNodes.SizeInt() - 1; i >= 0; --i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( entsAsNodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPreChange ), CreateEventData( eventData ) );
				}

				// Establish target transform
				EngineTransform et;
				Vector pos( 0, 0, 0, 0 ), scale( 0, 0, 0, 0 );
				EulerAngles rot( 0, 0, 0 );

				// Calc point
				switch ( target )
				{
				case 0: // First
					{
						alignTarget = Cast< CEntity >( entsAsNodes[0] );
						Uint32 ind = 1;
						while ( alignTarget->IsA< CEntityGroup >() && ind < entities.Size() )
						{
							alignTarget = Cast< CEntity >( entsAsNodes[ind++] );
						}
						et = alignTarget->GetTransform();
					}
					break;
				case 1: // Last
					{
						alignTarget = Cast< CEntity >( entsAsNodes.Back() );
						et = alignTarget->GetTransform();
					}
					break;
				case 2: // Center
					{
						for ( auto it=entities.Begin(); it != entities.End(); ++it )
						{
							const EngineTransform& es = (*it)->GetTransform();
							pos += es.GetPosition();
							scale += es.GetScale();
							rot += es.GetRotation();
						}
						pos /= (Float)entities.Size();
						scale /= (Float)entities.Size();
						rot /= (Float)entities.Size();
						et = EngineTransform( pos, rot, scale );
					}
					break;
				}

				// Apply transform
				for ( CEntity* ent : entities )
				{
					if ( ent->MarkModified() && ent != alignTarget )
					{
						CEntityGroup* eg = nullptr;
						if ( ent->IsA< CEntityGroup >() )
						{
							eg = Cast< CEntityGroup >( ent );
							ent = eg->GetEntities().Front();
						}

						if ( usePosition )
						{
							Float xPos = ent->GetPosition().X;
							Float yPos = ent->GetPosition().Y;
							Float zPos = ent->GetPosition().Z;
							if ( useXTrans )
							{
								xPos = et.GetPosition().X;
							}
							if ( useYTrans )
							{
								yPos = et.GetPosition().Y;
							}
							if ( useZTrans )
							{
								zPos = et.GetPosition().Z;
							}
							Vector newPos( xPos, yPos, zPos );
							Vector posDiff = newPos - ent->GetPosition();
							ent->SetPosition( newPos );

							if ( eg )
							{
								for ( Uint32 i = 1; i < eg->GetEntities().Size(); ++i )
								{
									eg->GetEntities()[i]->SetPosition( eg->GetEntities()[i]->GetPosition() + posDiff );
								}
							}
						}
						if ( useRotation )
						{
							const EulerAngles rot = ent->GetRotation();

							Float xRot = rot.Roll;			//!< Rotation around the X axis
							Float yRot = rot.Pitch;			//!< Rotation around the Y axis
							Float zRot = rot.Yaw;			//!< Rotation around the Z axis

							if ( useXRotate )
							{
								xRot = et.GetRotation().Roll;
							}
							if ( useYRotate )
							{
								yRot = et.GetRotation().Pitch;
							}
							if ( useZRotate )
							{
								zRot = et.GetRotation().Yaw;
							}
							EulerAngles newRot( xRot, yRot, zRot );
							EulerAngles rotDiff = newRot - ent->GetRotation();
							ent->SetRotation( newRot );

							if ( eg )
							{
								for ( Uint32 i = 1; i < eg->GetEntities().Size(); ++i )
								{
									eg->GetEntities()[i]->SetRotation( eg->GetEntities()[i]->GetRotation() + rotDiff );
									Vector d = eg->GetEntities()[i]->GetPosition() - ent->GetPosition();
									eg->GetEntities()[i]->SetPosition( ent->GetPosition() + rotDiff.TransformVector( d ) );
								}
							}
						}
						if ( useScale )
						{
							Float xScale = ent->GetScale().X;
							Float yScale = ent->GetScale().Y;
							Float zScale = ent->GetScale().Z;
							if ( useXScale )
							{
								xScale = et.GetScale().X;
							}
							if ( useYScale )
							{
								yScale = et.GetScale().Y;
							}
							if ( useZScale )
							{
								zScale = et.GetScale().Z;
							}
							Vector newScale( xScale, yScale, zScale );
							ent->SetScale( newScale );
						}
					}
				}

				// Post-change events
				for ( Int32 i=0; i < entsAsNodes.SizeInt(); ++i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( entsAsNodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPostChange ), CreateEventData( eventData ) );
				}
			}
		}
	}
	// changing mesh collsion type
	if ( wxTheFrame && wxTheFrame->GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->IsChecked() )
	{
		if ( key == IK_Z && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnNavigationConvert( wxCommandEvent(), PLC_Static );
			return true;
		}
		if ( key == IK_X && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{		
			OnNavigationConvert( wxCommandEvent(), PLC_StaticWalkable );
			return true;
		}
		if ( key == IK_C && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnNavigationConvert( wxCommandEvent(), PLC_Walkable );
			return true;
		}
		if ( key == IK_V && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnNavigationConvert( wxCommandEvent(), PLC_Dynamic );
			return true;
		}
		if ( key == IK_B && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnNavigationConvert( wxCommandEvent(), PLC_Disabled );
			return true;
		}
		if ( key == IK_N && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnNavigationConvert( wxCommandEvent(), PLC_StaticMetaobstacle );
			return true;
		}
	}
	if ( wxTheFrame && wxTheFrame->GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->IsChecked() )
	{
		if ( key == IK_Z && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnShadowCasting(true);
		}
	}
	if ( wxTheFrame && wxTheFrame->GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->IsChecked() )
	{
		if ( key == IK_X && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Alt ) && GetWorld() )
		{	
			OnShadowCasting(false);
		}
	}

	// Whole map screenshot taking rotation :)
	if ( key == IK_M && action == IACT_Release && RIM_IS_KEY_DOWN( IK_Alt ) ) 
	{
		static Float yaw = -90.0f;
		yaw += 90.0f;
		if ( yaw == 360.0f ) yaw = 0.0f;
		m_cameraRotation.Pitch = 270.0f;
		m_cameraRotation.Yaw = yaw;
		m_forceOrthoCamera = true;
		OnCameraMoved();
		return true;
	}
	else if ( m_forceOrthoCamera && key == IK_NumPlus && action == IACT_Release )
	{
		m_orthoZoom += 50.0f;
		return true;
	}
	else if ( m_forceOrthoCamera && key == IK_NumMinus && action == IACT_Release )
	{
		m_orthoZoom -= 50.0f;
		return true;
	}
	else if ( m_forceOrthoCamera && key == IK_N )
	{
		m_forceOrthoCamera = false;
	}

	// Process debug pages stuff
#ifndef NO_DEBUG_PAGES
	if ( !GGame->IsActive() )
	{
		if( IDebugPageManagerBase::GetInstance()->OnViewportInput( view, key, action, data ) )
		{
			return true;
		}
	}
#endif

#ifndef RED_FINAL_BUILD
	if ( action == IACT_Press )
	{
		// Calculate zoom shift
		Int32 zoomShiftAdd = 0;
		{	
			// Toggle render target size up
			if ( key == IK_Equals )
			{
				zoomShiftAdd = 2;			
			}

			// Toggle render target size down
			if ( key == IK_Minus )
			{
				zoomShiftAdd = -2;
			}
		}

		// Apply zoom shift
		if ( 0 != zoomShiftAdd )
		{
			const Int32 newZoomShift = Max<Int32>( 0, (Int32)Debug::GRenderTargetZoomShift + zoomShiftAdd );
			if ( 0 == newZoomShift )
			{
				Debug::GRenderTargetZoomShift = 0;
				Debug::GRenderTargetOffsetX = 0;
				Debug::GRenderTargetOffsetY = 0;
			}
			else
			{
				POINT cursorPoint;
				::GetCursorPos( &cursorPoint );
				::ScreenToClient( (HWND)GetHandle(), &cursorPoint );

				Int32 x = cursorPoint.x;
				Int32 y = cursorPoint.y;
				Int32 screenWidth = view->GetWidth();
				Int32 screenHeight = view->GetHeight();

				Int32 clickPosX = (x >> Debug::GRenderTargetZoomShift) + Debug::GRenderTargetOffsetX;
				Int32 clickPosY = (y >> Debug::GRenderTargetZoomShift) + Debug::GRenderTargetOffsetY;

				Debug::GRenderTargetZoomShift = newZoomShift;
				Debug::GRenderTargetOffsetX = clickPosX - (x >> Debug::GRenderTargetZoomShift);
				Debug::GRenderTargetOffsetY = clickPosY - (y >> Debug::GRenderTargetZoomShift);
			}
		}
	}
#endif

	// Camera jump
	if ( action == IACT_Press && key == IK_J )
	{
		POINT cursorPoint;
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );

		if ( CalcClickedPosition( cursorPoint.x, cursorPoint.y ) )
		{
			Vector back = m_cameraPosition - m_clickedWorldPos;
			back.Normalize3();
			back.Mul3( 2 );
			m_cameraPosition = m_clickedWorldPos + back + Vector(0, 0, 2);
			OnCameraMoved();
			return true;
		}
	}

	// Camera rotation
	{
		if ( key == IK_Z && !RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			if ( action == IACT_Press )
			{
				POINT cursorPoint;
				::GetCursorPos( &cursorPoint );
				::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
				if ( CalcClickedPosition( cursorPoint.x, cursorPoint.y ) )
				{
					m_cameraRotationCenter3D = m_clickedWorldPos;
					m_cameraRotationInitialPosition = m_cameraPosition;
					m_cameraRotationInitialAngles = m_cameraRotation;
					m_cameraRotationDistance = m_cameraRotationCenter3D.DistanceTo( m_cameraRotationInitialPosition );
					m_cameraRotating = true;
					view->SetMouseMode( MM_Clip );
					::GetCursorPos( &m_cameraRotationInitialCursorPosition );

					RECT rect;
					::GetWindowRect( (HWND)GetHandle(), &rect );
					::SetCursorPos( ( rect.left + rect.right ) / 2, ( rect.top + rect.bottom ) / 2 );
					m_cameraRotationCenter2D.x = ( rect.left + rect.right ) / 2;
					m_cameraRotationCenter2D.y = ( rect.top + rect.bottom ) / 2;
					::ScreenToClient( (HWND)GetHandle(), &m_cameraRotationCenter2D );

					GetViewport()->SetCursorVisibility( false );

					return true;
				}
			}
			else if ( action == IACT_Release )
			{
				if ( m_cameraRotating )
				{
					m_cameraRotating = false;
					view->SetMouseMode( MM_Normal );
				}
				return true;
			}
		}

		if ( m_cameraRotating )
		{
			if ( key == IK_MiddleMouse && action == IACT_Press )
			{
				Vector back = m_cameraPosition - m_cameraRotationCenter3D;
				back.Normalize3();
				back.Mul3( 2 );
				m_cameraPosition = m_clickedWorldPos + back + Vector(0, 0, 2);
				OnCameraMoved();
				m_cameraRotating = false;
				view->SetMouseMode( MM_Normal );
				::SetCursorPos( m_cameraRotationInitialCursorPosition.x, m_cameraRotationInitialCursorPosition.y );
				::ClipCursor( NULL );
				return true;
			}

			if ( key == IK_MouseZ )
				m_cameraRotationDistance += data*2.5f;

			POINT cursorPoint, delta;
			::GetCursorPos( &cursorPoint );
			::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
			delta.x = cursorPoint.x - m_cameraRotationCenter2D.x;
			delta.y = cursorPoint.y - m_cameraRotationCenter2D.y;
			Matrix preCamMatrix = m_cameraRotationInitialAngles.ToMatrix();
			m_cameraRotation = m_cameraRotationInitialAngles;
			m_cameraRotation.Yaw += delta.x*0.2f;
			m_cameraRotation.Pitch += delta.y*0.2f;
			Matrix postCamMatrix = m_cameraRotation.ToMatrix();
			Vector dir = ( m_cameraRotationCenter3D - m_cameraRotationInitialPosition ).Normalized3();
			dir = postCamMatrix.TransformVector( preCamMatrix.Inverted().TransformVector( dir ) );
			Vector endPoint = m_cameraRotationInitialPosition + dir*m_cameraRotationDistance;
			Vector deltaVec = endPoint - m_cameraRotationCenter3D;
			m_cameraPosition = m_cameraRotationInitialPosition - deltaVec;
			
			OnCameraMoved();

			return true;
		}	
	}

	// Camera bookmarks
	if ( action == IACT_Press )
	{
		// Cycle between set bookmarks
		if ( key == IK_Tilde && RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Alt ) )
		{
			int currentBookmark = SLastUsedBookmark;
			while ( true )
			{
				SLastUsedBookmark++;
				if ( SLastUsedBookmark == MAX_WORLD_CAMERA_BOOKMARKS ) SLastUsedBookmark = 0;

				if ( SLastUsedBookmark == currentBookmark ) break;

				if ( GWorldCameraBookmarks.GetBookmark( SLastUsedBookmark, m_cameraPosition, m_cameraRotation ) )
				{
					OnCameraMoved();
					break;
				}
			}

			return true;
		}

		// Swap between last two bookmarks
		if ( key == IK_Tilde && RIM_IS_KEY_DOWN( IK_Alt ) && !RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			if ( GWorldCameraBookmarks.GetBookmark( SSwappedBookmark, m_cameraPosition, m_cameraRotation ) )
			{
				int current = SLastUsedBookmark;
				SLastUsedBookmark = SSwappedBookmark;
				SSwappedBookmark = current;
				OnCameraMoved();
			}
			return true;
		}

		// Navigate to or set a bookmark
		if ( key >= IK_0 && key <= IK_9 && RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			// set bookmark
			if ( RIM_IS_KEY_DOWN( IK_Alt ) )
			{
				GWorldCameraBookmarks.SetBookmark( key - IK_0, m_cameraPosition, m_cameraRotation );
				SLastUsedBookmark = key - IK_0;
			}
			else // navigate to bookmark
			{
				if ( GWorldCameraBookmarks.GetBookmark( key - IK_0, m_cameraPosition, m_cameraRotation ) )
				{
					SSwappedBookmark = SLastUsedBookmark;
					SLastUsedBookmark = key - IK_0;
					OnCameraMoved();

// 					GGame->GetActiveWorld()->UnloadStreamingLayers();
// 					GGame->GetActiveWorld()->RefreshStreaming();
// 					SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), NULL );
				}
			}
			return true;
		}
	}

	// Create resource at position under cursor
	if ( RIM_IS_KEY_DOWN( IK_C ) && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Alt ) && !RIM_IS_KEY_DOWN( IK_Ctrl ) )
	{
		const TDynArray<String>& resourcePaths = GetActiveResources();
		if ( resourcePaths.Size() > 0 )
		{
			Uint32 index = GEngine->GetRandomNumberGenerator().Get< Uint32 >( resourcePaths.Size() );
			CResource* resource = GDepot->LoadResource( resourcePaths[ index ] );
			if ( resource )
			{
				TDynArray<CResource*> resources;
				resources.PushBack( resource );
				POINT cursorPoint;
				::GetCursorPos( &cursorPoint );
				::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
				OnDropResources( cursorPoint.x, cursorPoint.y, resources );
			}
		}
		return true;
	}

	// Start creating area using polygon drawing mode with Ctrl+Alt+P
	if ( key == IK_P && action == IACT_Press && RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		if ( !m_scene->GetActiveLayer() )
		{
			wxMessageBox( wxT("Please activate a layer before drawing a polygon"), wxT("No active layer"), wxICON_ERROR|wxOK );
			m_polygonDrawing = false;
		}
		else
		{
			if ( m_polygonDrawing )
			{
				CreateAreaFromPolygon();
			}
			else
			{
				POINT cursorPoint;
				::GetCursorPos( &cursorPoint );
				::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
				CalcClickedPosition( cursorPoint.x, cursorPoint.y );
				m_polygonPoints.Clear();
				m_polygonPoints.PushBack( m_clickedWorldPos );
				m_polygonPoints.PushBack( m_clickedWorldPos );
				m_polygonDrawing = true;
			}
		}
	}
	if ( m_polygonDrawing && key == IK_P && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ) && !RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		POINT cursorPoint;
		Uint32 nearest = m_polygonPoints.Size();
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
		CalcClickedPosition( cursorPoint.x, cursorPoint.y );
		for ( Uint32 i=0; i<m_polygonPoints.Size() - 1; ++i )
		{
			if ( m_clickedWorldPos.DistanceTo( m_polygonPoints[i] ) < 1 )
			{
				nearest = i;
				break;
			}
		}

		if ( nearest == m_polygonPoints.Size() )
		{
			m_polygonPoints[m_polygonPoints.Size() - 1] = m_clickedWorldPos;
			m_polygonPoints.PushBack( m_clickedWorldPos );
		}
		else if ( nearest == 0 && m_polygonPoints.Size() > 2 )
		{
			CreateAreaFromPolygon();
		}
	}
	if ( m_polygonDrawing && key == IK_Backspace && action == IACT_Press )
	{
		m_polygonPoints.PopBack();
		if ( m_polygonPoints.Size() < 2 )
		{
			m_polygonDrawing = false;
		}
	}

	// Align selection to geometry under cursor
	if ( RIM_IS_KEY_DOWN( IK_Y ) && action == IACT_Press && !RIM_IS_KEY_DOWN( IK_Ctrl ) )
	{
		POINT cursorPoint;
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );

		TDynArray<CEntity*> entities = GetSelectionManager()->GetSelectedEntities();

		if ( !entities.Empty() )
		{
			if ( RIM_IS_KEY_DOWN( IK_Shift ) )
			{
				TDynArray< CEntity* > clones = GetSelectionManager()->DuplicateSelectedEntities();
				
				if ( !clones.Empty() )
				{
					for ( CEntity* clone : clones )
					{
						CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), clone, true );
					}

					CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
				}
			}

			if ( AlignEntitiesToPositionFromViewportXY( cursorPoint.x, cursorPoint.y, entities, RIM_IS_KEY_DOWN( IK_Alt ), !RIM_IS_KEY_DOWN( IK_Shift ) ) )
			{
				return true;
			}
		}
	}

	if ( m_shapesContainer && action == IACT_Press && key == IK_R )
	{
		TDynArray< CEntity* > selectedEntities;
		if ( GetWorld() )
		{
			GetSelectionManager()->GetSelectedEntities( selectedEntities );

			const Uint32 size = selectedEntities.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				m_shapesContainer->AddToEdit( selectedEntities[ i ] );
			}
		}
	}

	// Debug time scaling
	if ( key == IK_NumPlus && action == IACT_Release )
	{
		GGame->SetOrRemoveTimeScale( ::Min( GGame->GetTimeScale() * 2.f, 128.0f ), CNAME( UserInputTimeScale ), 0x7FFFFFFF );
		return true;
	}
	else if ( key == IK_NumMinus && action == IACT_Release )
	{
		GGame->SetOrRemoveTimeScale( ::Max( GGame->GetTimeScale() * 0.5f, 0.03125f / 16.f ),  CNAME( UserInputTimeScale ), 0x7FFFFFFF );
		return true;
	}

	// Pass to base shit
	return CEdRenderingPanel::OnViewportInput( view, key, action, data );
}

Bool CEdWorldEditPanel::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
#ifndef NO_RED_GUI
	if( GGame->IsActive() == false )
	{
		if(GRedGui::GetInstance().GetEnabled() == true && m_mouseButtonFlags == 0 )
		{
			if( GRedGui::GetInstance().OnViewportClick( view, button, state, x, y ) == true )
			{
					return true;
				}
			}
		}		
#endif	// NO_RED_GUI

	// Process debug pages stuff
#ifndef NO_DEBUG_PAGES
	if ( !GGame->IsActive() )
	{
		if( IDebugPageManagerBase::GetInstance()->OnViewportClick( view, button, state, x, y ) )
		{
			return true;
		}
	}
#endif

	if ( button == 0 && state )
	{
		wxMilliClock_t millis = wxGetLocalTimeMillis();
		if ( millis - m_lastClickTime < 400 && millis - m_previousLastClickTime < 800 && Vector( x, y, 0 ).DistanceTo( Vector( m_lastClickX, m_lastClickY, 0 ) ) < 3 ) // triple click
		{
			TDynArray< CEntity* > selectedEntities;
			if ( GetWorld() )
			{
				GetSelectionManager()->GetSelectedEntities( selectedEntities );
			}

			if ( selectedEntities.Size() > 0 )
			{
				if ( selectedEntities.Size() > 5 )
				{
					if ( wxMessageBox( wxT("You have more than 5 entities selected. Are you sure you want to open all these editors?"), wxT("Wait a minute there"), wxCENTRE|wxYES_NO, this ) != wxYES )
					{
						return true;
					}
				}

				for ( Uint32 i=0; i<selectedEntities.Size(); ++i )
				{
					CEntity* ent = selectedEntities[i];
					CDiskFile* file = NULL;

					const TDynArray<CComponent*> components = ent->GetComponents();
					if ( components.Size() == 1 ) 
					{
						CComponent* comp = components[0];
						if ( comp->IsA< CAreaEnvironmentComponent >() )
						{
							wxTheFrame->OpenWorldEnvironmentEditor();
							wxTheFrame->EditEnvironmentParams( comp );
							return true;
						}
					}

#ifndef NO_MARKER_SYSTEMS
					if ( ent->GetName().BeginsWith( TXT("poi_") ) )
					{
						TDynArray<CComponent*> comps = ent->GetComponents();
						String name;
						for ( Uint32 i = 0; i < comps.Size(); ++i )
						{
							if( comps[i] && comps[i]->IsA<CHelpTextComponent>() )
							{
								name = Cast< CHelpTextComponent >( comps[i] )->GetText();
								CEdMarkersEditor* tool = Cast<CEdMarkersEditor>(wxTheFrame->GetToolsPanel()->StartTool( ClassID< CEdMarkersEditor >() ));
								tool->OpenPOIsEditor( name );
								return true;
							}
						}
					}
#endif // NO_MARKER_SYSTEMS

					CEntityTemplate* tpl = ent->GetEntityTemplate();
					if ( tpl )
					{
						file = tpl->GetFile();
					} 
					else
					{
						TDynArray<CResource*> resources;
						SEntityStreamingState streamingState;
						ent->PrepareStreamingComponentsEnumeration( streamingState, true );
						ent->ForceFinishAsyncResourceLoads();
						ent->CollectUsedResources( resources );
						ent->FinishStreamingComponentsEnumeration( streamingState );
						if ( resources.Size() == 1 )
						{
							file = resources[0]->GetFile();
						}
					}

					if ( !file ) continue;

					wxTheFrame->GetAssetBrowser()->OpenFile( file->GetDepotPath() );
				}
			}

			return true;
		}
		m_previousLastClickTime = m_lastClickTime;
		m_lastClickTime = millis;
		m_lastClickX = x;
		m_lastClickY = y;
	}

	return CEdRenderingPanel::OnViewportClick( view, button, state, x, y );
}

static inline void AddBoxExtentLines( CRenderFrame* frame, const Box& box, const Matrix& mtx, const Color& color )
{
	Vector lines[ 24 ];
	Vector dir = box.Max - box.Min;
	dir.Normalize3();

	lines[ 0 ] = Vector( box.Min.X - dir.X * 10000, box.Min.Y, box.Min.Z );  lines[ 1 ] = Vector( box.Min.X + dir.X * 10000, box.Min.Y, box.Min.Z );
	lines[ 2 ] = Vector( box.Min.X - dir.X * 10000, box.Max.Y, box.Min.Z );  lines[ 3 ] = Vector( box.Min.X + dir.X * 10000, box.Max.Y, box.Min.Z );
	lines[ 4 ] = Vector( box.Min.X, box.Min.Y - dir.Y * 10000, box.Min.Z );  lines[ 5 ] = Vector( box.Min.X, box.Min.Y + dir.Y * 10000, box.Min.Z );
	lines[ 6 ] = Vector( box.Max.X, box.Min.Y - dir.Y * 10000, box.Min.Z );  lines[ 7 ] = Vector( box.Max.X, box.Min.Y + dir.Y * 10000, box.Min.Z );
	lines[ 8 ] = Vector( box.Min.X - dir.X * 10000, box.Min.Y, box.Max.Z );  lines[ 9 ] = Vector( box.Min.X + dir.X * 10000, box.Min.Y, box.Max.Z );
	lines[ 10 ] = Vector( box.Min.X - dir.X * 10000, box.Max.Y, box.Max.Z ); lines[ 11 ] = Vector( box.Min.X + dir.X * 10000, box.Max.Y, box.Max.Z );
	lines[ 12 ] = Vector( box.Min.X, box.Min.Y - dir.Y * 10000, box.Max.Z ); lines[ 13 ] = Vector( box.Min.X, box.Min.Y + dir.Y * 10000, box.Max.Z );
	lines[ 14 ] = Vector( box.Max.X, box.Min.Y - dir.Y * 10000, box.Max.Z ); lines[ 15 ] = Vector( box.Max.X, box.Min.Y + dir.Y * 10000, box.Max.Z );
	lines[ 16 ] = Vector( box.Min.X, box.Min.Y, box.Min.Z - dir.Z * 10000 ); lines[ 17 ] = Vector( box.Min.X, box.Min.Y, box.Min.Z + dir.Z * 10000 );
	lines[ 18 ] = Vector( box.Max.X, box.Min.Y, box.Min.Z - dir.Z * 10000 ); lines[ 19 ] = Vector( box.Max.X, box.Min.Y, box.Min.Z + dir.Z * 10000 );
	lines[ 20 ] = Vector( box.Min.X, box.Max.Y, box.Min.Z - dir.Z * 10000 ); lines[ 21 ] = Vector( box.Min.X, box.Max.Y, box.Min.Z + dir.Z * 10000 );
	lines[ 22 ] = Vector( box.Max.X, box.Max.Y, box.Min.Z - dir.Z * 10000 ); lines[ 23 ] = Vector( box.Max.X, box.Max.Y, box.Min.Z + dir.Z * 10000 );

	for ( Uint32 i=0; i<24; ++i )
	{
		lines[ i ] = mtx.TransformVectorAsPoint( lines[ i ] );
	}

	for ( Uint32 i=0; i<24; i += 2 )
	{
		frame->AddDebugLine( lines[ i ], lines[ i + 1 ], color, false );
	}
}

void CEdWorldEditPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();

	// Allow sequential capture from main viewport window
	frame->GetFrameInfo().m_allowSequentialCapture = true;

	if ( m_forceOrthoCamera )
	{
		// hack for making orthogonal level screenshots
		Float aspect = view->GetHeight() / (Float) view->GetWidth();
		CRenderCamera& renderCam = const_cast<CRenderFrameInfo&>( frame->GetFrameInfo() ).m_camera;
		renderCam.Set( renderCam.GetPosition(), renderCam.GetRotation(), 0.0f, aspect, renderCam.GetNearPlane(), renderCam.GetFarPlane(), m_orthoZoom );
	}

	CEdRenderingPanel::OnViewportGenerateFragments( view, frame );

	// Draw selection free space visualization mesh
	if ( m_selFreeSpaceShow && !m_selFreeSpaceMesh.Empty() )
	{
		static const Color theColor( 32, 100, 200, 60 );
		frame->AddDebugTriangles( Matrix::IDENTITY,
			m_selFreeSpaceMesh.TypedData(), m_selFreeSpaceMesh.Size(),
			m_selFreeSpaceMeshIndices.TypedData(), m_selFreeSpaceMeshIndices.Size(), 
			theColor, false, true );
	}

	// Draw world pick point
	if ( HasPickPoint() && HasPickPointClient() )
	{
		frame->AddDebugSphere( GetPickPoint(), 0.1f, Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}

	// Draw entity list markers
	if ( GetSceneExplorer()->GetEntityListManager() != nullptr )
	{
		for ( CEntityList* list : GetSceneExplorer()->GetEntityListManager()->GetAllManagedLists() )
		{
			if ( list->IsVisible() )
			{
				const TDynArray<Vector>& markers = list->GetWorldMarkers();
				for ( const Vector& v : markers )
				{
					frame->AddDebugSphere( v, 0.1f, Matrix::IDENTITY, Color::LIGHT_CYAN, true );
					frame->AddDebugSolidTube( v, v + Vector( 0, 0, 10000 ), 0.06f, 0.03f, Matrix::IDENTITY, Color::LIGHT_MAGENTA, Color::LIGHT_CYAN );
					frame->AddDebugText( v, TXT("T"), true, Color::RED, Color::BLACK );
				}
			}
		}
	}

	// Can we draw debug shit
#ifndef NO_DEBUG_PAGES
	if ( !GGame->IsActive() )
	{
		IDebugPageManagerBase::GetInstance()->OnViewportGenerateFragments( view, frame );
	}
#endif

#ifndef NO_RED_GUI
	if ( GGame->IsActive() == false )
	{
		if ( GDebugWin::GetInstance().GetVisible() )
		{
			GRedGui::GetInstance().OnViewportGenerateFragments( view, frame );
		}

		if ( GRedGui::GetInstance().GetEnabled() )
		{
			// help text
			frame->AddDebugScreenText( view->GetWidth() - 165, 35, String::Printf( TXT("F8 key closes Debug Windows") ), 0, false, Color::LIGHT_YELLOW );
			frame->AddDebugScreenText( view->GetWidth() - 245, 50, String::Printf( TXT("Free camera works when left control is pressed") ), 0, false, Color::LIGHT_YELLOW );
		}
		else
		{
			// help text
			frame->AddDebugScreenText( view->GetWidth() - 165, 35, String::Printf( TXT("F8 key opens Debug Windows") ), 0, false, Color::LIGHT_YELLOW );
		}
	}
#endif	// NO_RED_GUI

	// maybe it would be a good idea to create an IFragmentsGenerator interface and store maraudersMap and screenshotEditor in a list 
	if ( m_maraudersMap )
	{
		m_maraudersMap->OnViewportGenerateFragments( view, frame );
	}

	if ( m_screenshotEditor )
	{
		m_screenshotEditor->OnViewportGenerateFragments( view, frame );
	}

	// Draw a small frame with the tool's name, if there is a tool
	if ( m_tool )
	{
		Color color( 182, 73, 73 );
		frame->AddDebugRect( 0, 0, GetSize().GetWidth() - 2, 2, color );
		frame->AddDebugRect( 0, 0, 2, GetSize().GetHeight() - 2, color );
		frame->AddDebugRect( 0, GetSize().GetHeight() - 2, GetSize().GetWidth() - 1, GetSize().GetHeight() - 1, color );
		frame->AddDebugRect( GetSize().GetWidth() - 2, 0, GetSize().GetWidth() - 1, GetSize().GetHeight() - 1, color );

		if ( font )
		{
			Int32 tx, ty;
			Uint32 tw, th;
			font->GetTextRectangle( m_tool->GetFriendlyName(), tx, ty, tw, th );
			frame->AddDebugScreenText( GetSize().GetWidth() - tw, 1 + font->GetLineDist(), m_tool->GetFriendlyName(), Color::WHITE, font, true, color );
		}
	}
	
	// Draw the camera position in world and tile coordinates
	if ( font )
	{
		Int32 tileX = 0, tileY = 0;
		CWorld* world = NULL;
		const CRenderFrameInfo& frameInfo = frame->GetFrameInfo();
		const Vector cameraPosition = frameInfo.m_camera.GetPosition();
		const Float cameraFov = frameInfo.m_camera.GetFOV();
		const Float nearPlane = frameInfo.m_camera.GetNearPlane();
		const Float farPlane = frameInfo.m_camera.GetFarPlane();
		if ( GGame->GetActiveWorld() )
		{
			world = GGame->GetActiveWorld();
			if ( world->GetTerrain() )
			{
				world->GetTerrain()->GetTileFromPosition( cameraPosition, tileX, tileY, true );
			}
		}

		Color color( 73, 255, 80 );
		String text;

		// Hidden entities
		if ( !m_hiddenEntities.Empty() )
		{
			text += String::Printf( TXT("  %d hidden entities"), (int)m_hiddenEntities.Size() );
		}

		// Distance from selection
		if ( world && GetSelectionManager() && GetSelectionManager()->GetSelectionCount() > 0 )
		{
			text += String::Printf( TXT("  selection (%i nodes) distance %0.2f from"), GetSelectionManager()->GetSelectionCount(), cameraPosition.DistanceTo( GetSelectionManager()->GetPivotPosition() ) );
		}

		// Camera coordinates
		text += String::Printf( TXT(" camera at position %0.2f, %0.2f, %0.2f | fov %0.2f | np %0.2f | fp %0.2f | speed %0.2f | mouse sensitivity %d | terrain tile %d, %d"), 
			cameraPosition.X, cameraPosition.Y, cameraPosition.Z, cameraFov, nearPlane, farPlane, GetCameraSpeedMultiplier(), GetCameraPosSensitivity(), tileX, tileY );

		Int32 tx, ty;
		Uint32 tw, th;
		font->GetTextRectangle( text, tx, ty, tw, th );
		frame->AddDebugScreenText( frame->GetFrameOverlayInfo().m_width - tw - 2, frame->GetFrameOverlayInfo().m_height - 2, text, color, font, true, Color( 0, 0, 0, 0 ) );
	}

	// Draw selected entities bounding box extents (helpful for aligning stuff)
	if ( m_drawGuides && GGame->GetActiveWorld() != NULL && GetSelectionManager()->GetEntitiesSelectionCount() > 0 )
	{
		TDynArray< CEntity* > entities;
		Box box;
		Bool first = true;
		GetSelectionManager()->GetSelectedEntities( entities );
		
		for ( Uint32 i=0; i<entities.Size(); ++i )
		{
			CEntity* entity = entities[i];
			const TDynArray< CComponent* >& components = entity->GetComponents();
			for ( Uint32 i=0; i<components.Size(); ++i )
			{
				CBoundedComponent* bc = Cast<CBoundedComponent>( components[i] );
				if ( bc )
				{
					if ( m_widgetManager->GetWidgetSpace() != RPWS_Global )
					{
						Box localBox;
						if ( bc->IsA<CMeshTypeComponent>() )
						{
							CMeshTypeResource* mesh = static_cast<CMeshTypeComponent*>( bc )->GetMeshTypeResource();
							if ( mesh )
							{
								localBox = mesh->GetBoundingBox();
							}
						}
						else
						{
							localBox = bc->GetBoundingBox();
							Matrix mtx;
							bc->GetWorldToLocal( mtx );
							localBox.Min = mtx.TransformVector( localBox.Min );
							localBox.Max = mtx.TransformVector( localBox.Max );
						}

						AddBoxExtentLines( frame, localBox, bc->GetLocalToWorld(), Color::CYAN );
					}
					else
					{
						if ( first )
						{
							box = bc->GetBoundingBox();
							first = false;
						}
						else
						{
							box.AddBox( bc->GetBoundingBox() );
						}
					}
				}
			}
		}

		if ( m_widgetManager->GetWidgetSpace() == RPWS_Global )
		{
			AddBoxExtentLines( frame, box, Matrix::IDENTITY, Color::CYAN );
		}
		else if ( m_widgetManager->GetWidgetSpace() == RPWS_Foreign )
		{
			Matrix space = GetTransformManager()->CalculatePivotSpace( RPWS_Foreign );
			AddBoxExtentLines( frame, Box( Vector(0, 0, 0), 0.001 ), space, Color::GRAY );
		}
	}

	// When in polygon drawing mode, draw the polygon surfaces
	if ( m_polygonDrawing )
	{
		Float minZ, maxZ;

		// Update last polygon point using the mouse cursor
		POINT cursorPoint;
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
		CalcClickedPosition( cursorPoint.x, cursorPoint.y );
		m_polygonPoints[m_polygonPoints.Size() - 1] = m_clickedWorldPos;

		// Find minimum and maximum Z
		for ( Uint32 i=0; i<m_polygonPoints.Size(); ++i )
		{
			if ( i == 0 || m_polygonPoints[i].Z < minZ ) minZ = m_polygonPoints[i].Z;
			if ( i == 0 || m_polygonPoints[i].Z > maxZ ) maxZ = m_polygonPoints[i].Z;
		}

		// Add a couple of meters to maxZ
		maxZ += 2;

		// Draw the polygon area boundary lines
		for ( Uint32 i=0; i<m_polygonPoints.Size() - 1; ++i )
		{
			// Side points
			Vector a = m_polygonPoints[i];
			Vector b = m_polygonPoints[i + 1];

			// Add lines
			frame->AddDebugLine( Vector( a.X, a.Y, minZ ), Vector( b.X, b.Y, minZ ), Color::CYAN );
			frame->AddDebugLine( Vector( a.X, a.Y, maxZ ), Vector( b.X, b.Y, maxZ ), Color::CYAN );
			frame->AddDebugLine( Vector( a.X, a.Y, minZ ), Vector( a.X, a.Y, maxZ ), Color::LIGHT_BLUE );
			frame->AddDebugLine( Vector( a.X, a.Y, minZ ), Vector( a.X, a.Y, maxZ ), Color::LIGHT_BLUE );
			frame->AddDebugLine( Vector( a.X, a.Y, minZ ), Vector( b.X, b.Y, maxZ ), Color::BLUE );
			frame->AddDebugLine( Vector( a.X, a.Y, maxZ ), Vector( b.X, b.Y, minZ ), Color::BLUE );
		}
	}

	// MG: All stats moved to IDebugPageManagerBase, so it is available in game executable

	//dex--
	if ( GetWorld() && ( m_shapesContainer == NULL || m_shapesContainer->GetPreviewItemWorld() != GetWorld() ) )
	{
		if ( m_shapesContainer ) delete m_shapesContainer;
		m_shapesContainer = new ShapesPreviewContainer( GetWorld(), false );
	}

	if ( m_shapesContainer && !GetWorld() )
	{
		delete m_shapesContainer;
		m_shapesContainer = NULL;
	}
	if ( m_shapesContainer )
	{
		m_shapesContainer->OnGenerateFragments( view, frame );
	}
}

void CEdWorldEditPanel::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	// Calculate base game camera
	CEdRenderingPanel::OnViewportCalculateCamera( view, camera );

#ifndef NO_DEBUG_PAGES
	if ( !GGame->IsActive() )
	{
		IDebugPageManagerBase::GetInstance()->OnViewportCalculateCamera( view, camera );
	}
#endif

#ifndef NO_RED_GUI
	if ( GGame->IsActive() == false )
	{
		if(GRedGui::GetInstance().GetEnabled() == true)
		{
			GRedGui::GetInstance().OnViewportCalculateCamera( view, camera );
		}
	}
#endif	// NO_RED_GUI
}

void CEdWorldEditPanel::OnShowInSceneExplorer( wxCommandEvent& event )
{
	m_scene->UpdateSelected();
}

void CEdWorldEditPanel::OnAlignPivot( wxCommandEvent& event )
{
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );
	ASSERT( selectedEntities.Size() > 1 );

	CEntity* destEntity = selectedEntities.Back();

	Vector      destPosition = destEntity->GetPosition();
	EulerAngles destRotation = destEntity->GetRotation();

	THashSet< CEntityTemplate* > processedTemplates;

	for ( Uint32 i=0; i<selectedEntities.Size()-1; ++i )
	{
		CEntity* entity = selectedEntities[i];
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		entity->ForceFinishAsyncResourceLoads();

		CEntityTemplate* templ = entity->GetEntityTemplate();

		if ( templ )
		{
			if ( processedTemplates.Exist( templ ) )
			{
				continue;
			}
			else
			{
				if ( !GFeedback->AskYesNo( TXT("This will modify the entity template, affecting all its instances in the world. Are you sure?") ) 
					|| !templ->MarkModified()
					)
				{
					continue;
				}

				processedTemplates.Insert( templ );
			}
		}

		Vector      entityPos = entity->GetPosition();
		EulerAngles entityRot = entity->GetRotation();

		Vector      diffPos = entityPos - destPosition;
		EulerAngles diffRot = entityRot - destRotation;

		Matrix m  = entity->GetLocalToWorld();
		Matrix mi = entity->GetLocalToWorld().FullInverted();
		
		CLayer* layer = entity->GetLayer();

		if ( !layer || !entity->MarkModified() || !layer->MarkModified() )
		{
			continue;
		}

		TDynArray< Vector > worldPos;
		for ( CComponent* c : entity->GetComponents() )
		{
			Vector worldPos = m.TransformPoint( c->GetPosition() ) + diffPos;
			c->SetPosition( mi.TransformPoint( worldPos ) );
		}

		TDynArray< CNode* > componentsAsNodes = CastArray< CNode >( entity->GetComponents() );
		Vector pivotOffset = Vector::ZEROS;

		//m_transformManager->Move( componentsAsNodes, diffPos );
		// Rotate around 0 (thus no pivot node given), after aligning the position
		m_transformManager->Rotate( componentsAsNodes, diffRot, false, nullptr, pivotOffset );

		entity->UpdateStreamedComponentDataBuffers();
		entity->UpdateStreamingDistance();

		if ( templ )
		{
			entity->PrepareEntityForTemplateSaving();
			entity->DetachTemplate();
			templ->CaptureData( entity );
			templ->Save();

			EntitySpawnInfo info;
			info.m_template = templ;
			info.m_spawnPosition = destPosition;
			info.m_spawnRotation = destRotation;
			info.m_spawnScale    = entity->GetScale();

			// replace the entity with the one based on the updated template
 			layer->RemoveEntity( entity );
 			entity->Discard();
			CEntity* entity = layer->CreateEntitySync( info );
			if ( entity != nullptr )
			{
				ApplyRandomAppearance( entity );
			}
		}
		else
		{
			entity->SetPosition( destPosition );
			entity->SetRotation( destRotation );
		}
	}

	GetSelectionManager()->RefreshPivot();
}

namespace
{
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
}

void CEdWorldEditPanel::OnCreateEntityTemplate( wxCommandEvent& event )
{
	// Get selected entities
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );

	ASSERT( selectedEntities.Size() > 0 );

	// Get active layer
	CLayer* activeLayer = m_scene->GetActiveLayer();
	ASSERT( activeLayer != NULL );

	// Mark modified
	for( CEntity* entity : selectedEntities )
	{
		if ( !entity->GetLayer()->MarkModified() )
		{
			return;
		}
	}
	if ( !activeLayer->MarkModified() )
	{
		return;
	}

	Bool createTemplate = true;
	// Checking if all selected is mesh components
	for ( CEntity* entity : selectedEntities )
	{
		// Get components
		TDynArray< CComponent* > components;
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		CollectEntityComponents( entity, components );

		for( TDynArray< CComponent* >::iterator componentIter = components.Begin();
			componentIter != components.End(); ++componentIter )
		{
			CComponent* component = *componentIter;
			RED_ASSERT( component != nullptr );
			if ( component->IsExactlyA< CStaticMeshComponent >() || component->IsExactlyA< CMeshComponent >() || component->IsExactlyA< CDecalComponent >() )
			{
				createTemplate = true;
			}
			else
			{
				createTemplate = false;
				break;
			}
		}
	}

	if( createTemplate )
	{
		String fileName;
		CDirectory* dir = DoResourceSaveDialog( CFileFormat( TXT( "w2ent" ), TXT( "Entity Template" ) ), fileName );

		if ( !dir )
		{
			return;
		}

		// Create temp entity
		EntitySpawnInfo tempInfo;
		tempInfo.m_entityClass = CEntity::GetStaticClass();
		tempInfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );
		CEntity* entityObject = activeLayer->CreateEntitySync( tempInfo );
		ASSERT( entityObject != NULL );
		GetWorld()->DelayedActions();

		// Add components from selected entities
		for ( CEntity* entity : selectedEntities )
		{
			// Get components
			TDynArray< CComponent* > components;
			entity->DestroyStreamedComponents( SWN_NotifyWorld );
			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			CollectEntityComponents( entity, components );

			// Move components to created entity
			for( TDynArray< CComponent* >::iterator componentIter = components.Begin();
				componentIter != components.End(); ++componentIter )
			{
				CComponent* component = *componentIter;
				ASSERT( component != NULL );

				// Skip components which shouldn't be saved
				if ( !component->ShouldWriteToDisk() ||
					// Special case for sound emitter component since we don't want to include
						// SECs even if they come from a template and have default values
							( component->IsA< CSoundEmitterComponent >() && 
							static_cast< CSoundEmitterComponent* >( component )->IsDefault() ) )
				{
					continue;
				}

				// Skip components which shouldn't be saved
				if ( !component->ShouldWriteToDisk() )
				{
					continue;
				}

				// Get component world matrix
				Matrix world;
				component->GetLocalToWorld( world );

				entity->RemoveComponent( component );
				GetWorld()->DelayedActions();
				entityObject->AddComponent( component );

				// Set component transform
				component->SetPosition( world.GetTranslation() - GetSelectionManager()->GetPivotPosition() );
				component->SetRotation( world.ToEulerAnglesFull() );
				component->SetScale( world.GetScale33() );
				component->ForceUpdateTransformNodeAndCommitChanges();
			}
		}

		// Build streaming buffers
		entityObject->UpdateStreamedComponentDataBuffers();

		// Create entity template
		CEntityTemplate* entityTemplate = CreateObject< CEntityTemplate >();
		entityTemplate->CaptureData( entityObject );

		// Save template
		Bool saveOk = entityTemplate->SaveAs( dir, fileName );
		ASSERT( saveOk );

		// Destroy temp entity
		entityObject->Destroy();
		GetWorld()->DelayedActions();

		// Set spawning info
		EntitySpawnInfo info;
		info.m_template = entityTemplate;
		info.m_spawnPosition = GetSelectionManager()->GetPivotPosition();
		info.AddHandler( new TemplateInfo::CSpawnEventHandler() );

		// Delete selected entities
		for( CEntity* entity : selectedEntities )
		{
			entity->Destroy();
		}

		GetWorld()->DelayedActions();

		// Spawn new entity	
		activeLayer->CreateEntitySync( info );
		GetWorld()->DelayedActions();
		GetWorld()->RequestStreamingUpdate();
	}
	else
	{
		String msg = TXT("You can only create an entity template from meshes / static meshes!");
		wxMessageBox( msg.AsChar(), wxT("Important Information"), wxOK ) ;	
	}
}

void CEdWorldEditPanel::OnCreateMeshResource( wxCommandEvent& event )
{
	Vector pivotPos = GetSelectionManager()->GetPivotPosition();

	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );
	ASSERT( selectedEntities.Size() > 0 );

	CLayer* activeLayer = m_scene->GetActiveLayer();
	ASSERT( activeLayer != NULL );

	if ( !GetSelectionManager()->ModifySelection() || !activeLayer->MarkModified() )
	{
		return;
	}

	String fileName;
	CDirectory* dir = DoResourceSaveDialog( CFileFormat( TXT( "w2mesh" ), TXT( "Mesh resource" ) ), fileName );

	if ( !dir )
	{
		return;
	}

	CEdUndoManager::Transaction undoTransaction( *m_undoManager, TXT("create mesh resource from selection") );

	CMesh* newMesh = CreateObject< CMesh >();

	for ( CEntity* entity : selectedEntities )
	{
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

		for ( ComponentIterator< CMeshComponent > it( entity ); it; ++it )
		{
			Matrix m = (*it)->GetLocalToWorld();
			m.SetTranslation( m.GetTranslation() - pivotPos );
			CMesh* mesh = (*it)->GetMeshNow();
			newMesh->AddMesh( *mesh, m );
		}
	}

	if ( !newMesh->SaveAs( dir, fileName ) )
	{
		GFeedback->ShowError( TXT("Unable to save mesh") );
		return;
	}
	
	// = Delete selection =

	for ( CEntity* entity : selectedEntities )
	{
		CUndoCreateDestroy::CreateStep( m_undoManager, entity, false );
	}

	// = Spawn entity using new mesh =

	EntitySpawnInfo info;
	info.m_template = nullptr;
	info.m_spawnPosition = pivotPos;
	CEntity* newEntity = activeLayer->CreateEntitySync( info );

	CComponent* meshComp = newEntity->CreateComponent( ClassID< CStaticMeshComponent >(), SComponentSpawnInfo() );
	meshComp->SetResource( newMesh );

	CUndoCreateDestroy::CreateStep( m_undoManager, newEntity, true );

	CUndoCreateDestroy::FinishStep( m_undoManager );

	GetWorld()->RequestStreamingUpdate();
}

void CEdWorldEditPanel::OnCreateShadowMesh( wxCommandEvent& event )
{
	Vector pivotPos = GetSelectionManager()->GetPivotPosition();

	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );
//	ASSERT( selectedEntities.Size() > 1 );

// 	CLayer* activeLayer = m_scene->GetActiveLayer();
// 	ASSERT( activeLayer != NULL );
// 
// 	if ( !activeLayer->MarkModified() )
// 	{
// 		return;
// 	}

	CLayerInfo* layerInfo = GetWorld()->GetWorldLayers()->FindLayerByPath( TXT("shadow_meshes") );

	if ( layerInfo == nullptr )
	{
		layerInfo = GetWorld()->GetWorldLayers()->CreateLayer( TXT("shadow_meshes") );
	}

	if ( !layerInfo->IsLoaded() )
	{
		LayerLoadingContext loadingContext;
		layerInfo->SyncLoad( loadingContext );
	}

	CLayer* layer = layerInfo->GetLayer();
	ASSERT ( layer != nullptr );

	if ( !layer->MarkModified() )
	{
		return;
	}

	CEntity* shadowMeshEnt = MeshUtilities::ExtractShadowMesh( selectedEntities, layer, pivotPos );

	CUndoCreateDestroy::CreateStep( m_undoManager, shadowMeshEnt, true );
	CUndoCreateDestroy::FinishStep( m_undoManager );

	{
		CSelectionManager::CSelectionTransaction transaction( *GetSelectionManager() );
		GetSelectionManager()->DeselectAll();
		GetSelectionManager()->Select( shadowMeshEnt );
	}

//	m_scene->MarkTreeItemsForUpdate();
	GetWorld()->RequestStreamingUpdate();
}

void CEdWorldEditPanel::OnExtractMeshesFromEntities( wxCommandEvent& event )
{
	// Get selected entities
	TDynArray< CEntity* > selectedEntities = GetSelectionManager()->GetSelectedEntities();

	// Get active layer
	CLayer* activeLayer = m_scene->GetActiveLayer();
	if ( activeLayer == nullptr )
	{
		wxMessageBox( wxT("You need to activate a layer where the new meshes will be extracted into"), wxT("No active layer"), wxICON_ERROR|wxOK|wxCENTRE, wxTheFrame );
		return;
	}

	// Create undo transaction
	CEdUndoManager::Transaction undoTransaction( *wxTheFrame->GetUndoManager(), TXT("extract meshes from entities") );

	// Get a list of all mesh and non-mesh components
	TDynArray< CMeshComponent* > meshComponents;
	TDynArray< CComponent* > nonMeshComponents;
	for ( auto it=selectedEntities.Begin(); it != selectedEntities.End(); ++it )
	{
		CEntity* entity = *it;
		entity->CreateStreamedComponents( SWN_NotifyWorld );
		const TDynArray< CComponent* >& entityComponents = entity->GetComponents();
		for ( auto it=entityComponents.Begin(); it != entityComponents.End(); ++it )
		{
			CComponent* component = *it;
			if ( component->IsA< CMeshComponent >() )
			{
				meshComponents.PushBackUnique( static_cast< CMeshComponent* >( component ) );
			}
			else
			{
				nonMeshComponents.PushBackUnique( component );
			}
		}
	}

	TDynArray< String > warnings;

	// Create new entities
	for ( auto it=meshComponents.Begin(); it != meshComponents.End(); ++it )
	{
		CMeshComponent* meshComponent = *it;
		CMesh* mesh = meshComponent->GetMeshNow();

		// Skip meshless components
		if ( !mesh )
		{
			continue;
		}

		// Create a new entity for the mesh
		EntitySpawnInfo sinfo;
		sinfo.m_spawnPosition = meshComponent->GetWorldPositionRef();
		sinfo.m_spawnRotation = meshComponent->GetWorldRotation();
		sinfo.m_spawnScale = meshComponent->GetLocalToWorld().GetScale33();
		sinfo.m_resource = nullptr;
		sinfo.m_template = nullptr;
		sinfo.m_detachTemplate = true;
		sinfo.m_name = meshComponent->GetEntity()->GetName() + TXT("_") + meshComponent->GetName();
		sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );
		CEntity* entity = activeLayer->CreateEntitySync( sinfo );
		if ( !entity )
		{
			warnings.PushBack( String::Printf( TXT("Couldn't create entity %s"), sinfo.m_name.AsChar() ) );
			continue;
		}

		// Add undo step
		CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, true );

		// Create a new static mesh component for the mesh
		SComponentSpawnInfo cinfo;
		cinfo.m_name = meshComponent->GetName();
		CStaticMeshComponent* newMeshComponent = static_cast< CStaticMeshComponent* >( entity->CreateComponent( CStaticMeshComponent::GetStaticClass(), cinfo ) );
		if ( !newMeshComponent )
		{
			warnings.PushBack( String::Printf( TXT("Couldn't create mesh component %s"), cinfo.m_name.AsChar() ) );
			entity->Discard();
			continue;
		}

		// Set mesh
		newMeshComponent->SetResource( mesh );

		// Setup streaming
		newMeshComponent->SetStreamed( true );
		entity->SetStreamed( true );
		entity->UpdateStreamedComponentDataBuffers();
		entity->UpdateStreamingDistance();
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
		entity->CreateStreamedComponents( SWN_NotifyWorld );
	}

	if ( !warnings.Empty() )
	{
		CEdErrorsListDlg* errorsDlg = new CEdErrorsListDlg( wxTheFrame, false );
		errorsDlg->SetHeader( TXT("Couldn't create following items:") );
		errorsDlg->SetTitle( TXT("Warnings") );
		Uint32 style = errorsDlg->GetWindowStyle();
		errorsDlg->SetWindowStyle( style & ~wxSTAY_ON_TOP );
		errorsDlg->Execute( warnings );
	}

	// Finish creation undo steps
	CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );

	// Inform the user about non-component meshes and ask if he wants to delete the original entities
	TDynArray< CEntity* > entitiesToDelete;
	if ( nonMeshComponents.Empty() )
	{
		if ( wxMessageBox( wxT("Do you want to delete the original entities?"), wxT("Delete Source Entities"), wxICON_QUESTION|wxYES_NO|wxCENTRE, wxTheFrame ) == wxYES )
		{
			entitiesToDelete.PushBack( selectedEntities );
		}
	}
	else
	{
		TDynArray< Bool > checked;
		TDynArray< String > names;
		for ( auto it=nonMeshComponents.Begin(); it != nonMeshComponents.End(); ++it )
		{
			checked.PushBack( true );
			names.PushBack( (*it)->GetName() );
		}
		Int32 button = FormattedDialogBox( wxTheFrame, wxT("Delete Source Entities"), wxString::Format( wxT("'The entities had the following non-mesh components:'H{V{H{L%s*1=300}=200}|V{B@'Delete'|B'Keep'}}"), CEdFormattedDialog::ArrayToList( names ).AsChar() ), checked.TypedData() );
		if ( button == 0 ) // Delete
		{
			entitiesToDelete.PushBack( selectedEntities );
		}
	}

	// Delete the entities that the user requested
	Bool finishUndo = false;
	for ( auto it=entitiesToDelete.Begin(); it != entitiesToDelete.End(); ++it )
	{
		CEntity* entity = *it;
		// Make sure we can modify the layer
		if ( entity->GetLayer()->MarkModified() )
		{
			// Add undo step
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, false );
			finishUndo = true;
		}
	}

	// Finish destruction undo steps
	if ( finishUndo )
	{
		CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
	}
}

void CEdWorldEditPanel::OnExtractComponentsFromEntities( wxCommandEvent& event )
{
	// Get selected entities
	TDynArray< CEntity* > selectedEntities = GetSelectionManager()->GetSelectedEntities();

	// Get active layer
	CLayer* activeLayer = m_scene->GetActiveLayer();
	if ( activeLayer == nullptr )
	{
		wxMessageBox( wxT("You need to activate a layer where the components will be extracted into"), wxT("No active layer"), wxICON_ERROR|wxOK|wxCENTRE, wxTheFrame );
		return;
	}

	// Get a list of all components
	TDynArray< CComponent* > components;
	for ( auto it=selectedEntities.Begin(); it != selectedEntities.End(); ++it )
	{
		CEntity* entity = *it;
		entity->CreateStreamedComponents( SWN_NotifyWorld );
		components.PushBack( entity->GetComponents() );
	}

	// Create a list of components
	TDynArray<Bool> checked;
	static Bool deleteEntities = false;
	checked.Grow( components.Size() );
	int button = FormattedDialogBox( wxT("Extract Components"),
		wxString::Format( wxT("H{V'Selection Components:'{H{M%s=200}*1}|V{|B@'&Extract'|B'Extract &All'|B'&Cancel'V{}=100}}|X'Delete the original entities'"),
			CEdFormattedDialog::ArrayToList( components, []( CComponent* cmp ){ return cmp->GetName(); } ).AsChar() ),
			checked.Data(), &deleteEntities );
	if ( button == -1 || button == 2 ) return; // cancelled

	// Check if there are any checked components
	Bool any = false;
	for ( auto it=checked.Begin(); it != checked.End(); ++it)
	{
		if ( (*it) )
		{
			any = true;
			break;
		}
	}
	if ( !any )
	{
		return;
	}
	
	// Create undo transaction
	CEdUndoManager::Transaction undoTransaction( *wxTheFrame->GetUndoManager(), TXT("extract components from entities") );

	// Create new entities
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{
		CComponent* component = *it;

		// Skip components that weren't checked
		if ( !checked[ it - components.Begin() ] )
		{
			continue;
		}
		
		// Create a new entity for the component
		EntitySpawnInfo sinfo;
		sinfo.m_spawnPosition = component->GetWorldPositionRef() + Vector( 0.1, 0.1, 0.1 ); // This is done to avoid umbra caching, but let's 
		sinfo.m_spawnRotation = component->GetWorldRotation();								// present it as a feature: "with the offset you can
		sinfo.m_spawnScale = component->GetLocalToWorld().GetScale33();						// make sure you aren't making accidental duplicates"
		sinfo.m_resource = nullptr;															//                              Yes, that will work!
		sinfo.m_template = nullptr;
		sinfo.m_detachTemplate = true;
		sinfo.m_name = component->GetEntity()->GetName() + TXT("_") + component->GetName();
		sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );
		CEntity* entity = activeLayer->CreateEntitySync( sinfo );
		ASSERT( entity, TXT("Failed to create entity" ) );
		entity->SetStreamed( component->GetEntity()->ShouldBeStreamed() );

		// Add undo steps
		CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, true );

		// Create a new component
		CComponent* clone = entity->CreateComponent( component->GetClass(), SComponentSpawnInfo() );

		// Copy the properties
		TDynArray< CProperty* > properties;
		component->GetClass()->GetProperties( properties );
		for ( CProperty* prop : properties )
		{
			if ( !prop->IsEditable() || !prop->IsSerializable() )
			{
				continue;
			}
			clone->OnPropertyPreChange( prop );
			prop->GetType()->Copy( prop->GetOffsetPtr( clone ), prop->GetOffsetPtr( component ) );
			clone->OnPropertyPostChange( prop );
		}
		
		// Reset transform (we already transformed the entity)
		clone->SetPosition( Vector::ZERO_3D_POINT );
		clone->SetScale( Vector( 1, 1, 1 ) );
		clone->SetRotation( EulerAngles::ZEROS );
		
		// Update transform
		entity->ForceUpdateTransformNodeAndCommitChanges();
		entity->ForceUpdateBoundsNode();

		// Setup streaming
		if ( entity->ShouldBeStreamed() )
		{
			entity->UpdateStreamedComponentDataBuffers();
			entity->UpdateStreamingDistance();
			entity->DestroyStreamedComponents( SWN_NotifyWorld );
			entity->CreateStreamedComponents( SWN_NotifyWorld );
		}
	}

	// Delete the entities if requested
	if ( deleteEntities )
	{
		for ( auto it=selectedEntities.Begin(); it != selectedEntities.End(); ++it )
		{
			CEntity* entity = *it;
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), entity, false );
		}
	}

	// Finish creation undo steps
	CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
}

void CEdWorldEditPanel::OnHideSelectedEntities( wxCommandEvent& event )
{
	HideEntities( GetSelectionManager()->GetSelectedEntities() );
}

void CEdWorldEditPanel::OnIsolateSelectedEntities( wxCommandEvent& event )
{
	IsolateEntities( GetSelectionManager()->GetSelectedEntities() );
}

void CEdWorldEditPanel::OnListHiddenEntities( wxCommandEvent& event )
{
	// Create list of hidden entities
	TDynArray< CEntity* > hiddenEntities;
	hiddenEntities.Reserve( m_hiddenEntities.Size() );
	for ( THandle< CEntity > entity : m_hiddenEntities )
	{
		if ( entity.IsValid() )
		{
			hiddenEntities.PushBack( entity.Get() );
		}
	}

	// Add them to the hidden entities list
	ClearEntityList( TXT("Hidden Entities" ) );
	AddEntitiesToEntityList( TXT("Hidden Entities"), hiddenEntities );
}

void CEdWorldEditPanel::OnRevealHiddenEntities( wxCommandEvent& event )
{
	RevealEntities();
}

void CEdWorldEditPanel::OnEncounterEditor( wxCommandEvent& event )
{
	CEncounter* encounter = NULL;
	TDynArray< CNode* > selection;
	GetSelectionManager()->GetSelectedNodes( selection );
	for ( auto nodeIt = selection.Begin(); nodeIt != selection.End(); ++nodeIt )
	{
		encounter = Cast< CEncounter >( *nodeIt );
		if ( encounter )
		{
			break;
		}
	}
	
	if ( encounter )
	{
		OpenSpawnTreeEditor( encounter );
	}
}

void CEdWorldEditPanel::OnChangeAppearance( wxCommandEvent& event )
{
	TDynArray< CEntity* > selectedEntities;
	GetWorld()->GetSelectionManager()->GetSelectedEntities( selectedEntities );

	if ( selectedEntities.Size() == 1 && selectedEntities[0]->GetEntityTemplate() != nullptr )
	{
		CEntity* entity = selectedEntities[0];
		CEntityTemplate* tpl = selectedEntities[0]->GetEntityTemplate();
		CAppearanceComponent* ac = CAppearanceComponent::GetAppearanceComponent( entity );
		TDynArray< CEntityAppearance* > appearances;
		tpl->GetAllAppearances( appearances );
		if ( ac == nullptr ) return;

		TDynArray< String > names;
		for ( auto it=appearances.Begin(); it != appearances.End(); ++it )
		{
			names.PushBack( (*it)->GetName().AsString() );
		}

		String name = InputComboBox( this, TXT("Select Appearance"), TXT("Select the appearance to use"), ac->GetAppearance().AsString(), names );
		name.Trim();
		if ( name.Empty() ) return;

		if ( entity->MarkModified() )
		{
			ac->SetForcedAppearance( CName( name ) );
			ac->ApplyAppearance( CName( name ) );
		}
	}
}

void CEdWorldEditPanel::OnImportEntitiesFromOldTiles( wxCommandEvent& event )
{
	TDynArray< void* > allocates;

	// Find area
	CAreaComponent* area = nullptr;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; !area && i < nodes.Size(); ++i )
	{
		area = Cast< CAreaComponent >( nodes[i] );
	}
	if ( !area ) return;

	// Find old data file
	CFilePath path( GetWorld()->GetFile()->GetDepotPath() );
	path.SetExtension( TXT("dat") );
	path.SetFileName( TXT("old_streaming") );
	IFile* data = GFileManager->CreateFileReader( path.ToString(), FOF_Buffered );
	if ( !data )
	{
		wxMessageBox( wxT("Old streaming data file (old_streaming.dat) not found"), wxT("Cannot import"), wxICON_ERROR|wxOK, wxTheFrame );
		return;
	}

	// Feedback
	GFeedback->BeginTask( wxT("Loading GUID map"), false );

	// Load the GUID map
	struct EntityInfo {
		CGUID guid, layerGUID;
		Vector position;
		EulerAngles rotation;
		Vector scale;
		TDynArray< void* > data;
		TDynArray< size_t > size;
	};
	THashMap< CGUID, EntityInfo > guidInfoMap;
	THashMap< Vector, CGUID > positionGuidMap;
	TDynArray< Vector > importedPositions;
	Uint32 guidInfoMapSize = 0;
	*data << guidInfoMapSize;
	for ( Uint32 i=0; i < guidInfoMapSize; ++i )
	{
		EntityInfo ei;
		*data << ei.guid;
		*data << ei.layerGUID;
		*data << ei.position;
		*data << ei.rotation;
		*data << ei.scale;
		guidInfoMap[ei.guid] = ei;
		positionGuidMap[ei.position] = ei.guid;
		importedPositions.PushBack( ei.position );
	}

	// Scan the imported data for entities inside the area
	TDynArray< CGUID > insideTheArea;
	GFeedback->UpdateTaskName( wxT("Scanning the old streaming data (1/4)") );
	for ( Uint32 i=0; i < importedPositions.Size(); ++i )
	{
		if ( area->TestPointOverlap( importedPositions[i] ) )
		{
			insideTheArea.PushBackUnique( positionGuidMap[importedPositions[i]] );
		}
		GFeedback->UpdateTaskProgress( i, importedPositions.Size() );
	}

	// Scan the world for entities inside the area
	GFeedback->UpdateTaskName( wxT("Scanning the world (2/4)") );
	TDynArray< CEntity* > entities;
	THashMap< CGUID, CEntity* > guidEntityMap;
	for ( WorldAttachedEntitiesIterator it( GetWorld() ); it; ++it )
	{
		CEntity* entity = *it;
		if ( area->TestPointOverlap( entity->GetWorldPosition() ) )
		{
			entities.PushBack( entity );
			guidEntityMap[entity->GetGUID()] = entity;
		}
	}

	// Scan the buffers
	THashMap< CEntity*,Uint32 > dataSizes;
	Uint32 buffers = 0;
	String report = TXT("<html><head></head><body><h1>Report</h1>");
	*data << buffers;
	GFeedback->UpdateTaskName( wxT("Scanning the buffers (3/4)") );
	for ( Uint32 i=0; i < buffers; ++i )
	{
		// Load buffer
		CGUID bufferGUID;
		Uint32 bufferSize = 0;
		void* bufferData = nullptr;
		*data << bufferGUID;
		*data << bufferSize;
		bufferData = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, bufferSize );
		allocates.PushBack( bufferData );
		data->Serialize( bufferData, bufferSize );

		// Entity information for this buffer
		EntityInfo& ei = guidInfoMap[bufferGUID];
		ei.data.PushBack( bufferData );
		ei.size.PushBack( bufferSize );
		
		GFeedback->UpdateTaskProgress( i, buffers );
	}

	// Try to recreate lost entities
	CLayer* activeLayer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();
	if ( activeLayer )
	{
		int count = 0;

		// Scan the imported data for entities inside the area
		report += TXT("</ul><b>The following entities were processed</b>:<ul>");
		GFeedback->UpdateTaskName( wxT("Recreating lost entities (4/4)") );
		for ( Uint32 i=0; i < insideTheArea.Size(); ++i )
		{
			EntityInfo& ei = guidInfoMap[insideTheArea[i]];
			CEntity* entity = nullptr;
			Bool transMatch = false;
			Bool ignoreEntry = false;
			String reportLine;
			if ( ei.data.Empty() ) continue;
			
			reportLine += TXT("<li>");

			// Check if the entry is for an entity that already exists
			entity = guidEntityMap[positionGuidMap[ei.position]];

			// No entity found, check if the transformation matches exactly any other existing entity
			for ( Uint32 j=0; j < entities.Size(); ++j )
			{
				CEntity* other = entities[j];
				if ( other->GetWorldPosition() == ei.position &&
					 other->GetWorldRotation() == ei.rotation &&
					 other->GetScale() == ei.scale )
				{
					entity = other;
					transMatch = true;
					break;
				}
			}

			// Check if the entity wasn't lost
			if ( entity != nullptr )
			{
				// Check if the entity is not empty
				const TDynArray< CComponent* >& components = entity->GetComponents();
				if ( !components.Empty() )
				{
					// Scan the components - if we find meshes ignore this entity
					Bool hasMeshes = false;
					for ( auto it=components.Begin(); it != components.End(); ++it )
					{
						CComponent* component = *it;
						if ( component->IsA< CMeshTypeComponent >() )
						{
							entity = nullptr;
							transMatch = false;
							ignoreEntry = true;
							break;
						}
					}

					// Also check if the layers match
					if ( entity != nullptr )
					{
						CLayer* entityLayer = GetWorld()->FindLayer( ei.layerGUID );
						if ( entityLayer != nullptr && entityLayer != entity->GetLayer() )
						{
							entity = nullptr;
							transMatch = false;
						}
					}

					// If we still have an entity, remove all of its components
					if ( entity )
					{
						entity->DestroyAllComponents();
						reportLine += TXT("<i>(cleaned)</i>");
					}
				}
				
				if ( entity && components.Empty() )
				{
					reportLine += TXT("<i>(was empty)</i>");
				}
			}

			// If we decided to ignore this entry (because a proper entity already exists), skip this
			if ( ignoreEntry )
			{
				continue;
			}
			count++;
			
			if ( entity == nullptr ) // new entity
			{
				EntitySpawnInfo sinfo;
				sinfo.m_spawnPosition = ei.position;
				sinfo.m_spawnRotation = ei.rotation;
				sinfo.m_spawnScale = ei.scale;

				CLayer* entityLayer = GetWorld()->FindLayer( ei.layerGUID );
				if ( entityLayer == nullptr )
				{
					entityLayer = activeLayer;
					reportLine += TXT("<i>(lost layer)</i> ");
				}
				if ( !entityLayer->MarkModified() ) return;

				entity = entityLayer->CreateEntitySync( sinfo );
				reportLine += TXT("<i>(recreated)</i>");
			}
			
			if ( transMatch )
			{
				reportLine += TXT(" <i>(matched by transform)</i> ");
			}

			if ( !entity->MarkModified() ) return;

			TDynArray< CComponent* > streamingComponents;
			for ( Uint32 j=0; j < ei.data.Size(); ++j )
			{
				CEntityTemplate::IncludeComponents(
					entity->GetLayer(),
					entity,
					(Uint8*)ei.data[j],
					ei.size[j],
					streamingComponents,
					NULL,
					false
					);
			}
			for ( auto it=streamingComponents.Begin(); it != streamingComponents.End(); ++it )
			{
				(*it)->SetStreamed( false );
			}
			entity->UpdateStreamedComponentDataBuffers();
			entity->UpdateStreamingDistance();

			reportLine += String::Printf( TXT("%s <i>%i new components</i></li>"), entity->GetFriendlyName().AsChar(), (int)streamingComponents.Size() );
			report += reportLine;

			GFeedback->UpdateTaskProgress( i, insideTheArea.Size() );
		}
		report += String::Printf( TXT("</ul>%i entities"), count );
	}

	// Release memory
	for ( auto it=allocates.Begin(); it != allocates.End(); ++it )
	{
		void* data = *it;
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, data );
	}

	GFeedback->EndTask();
	delete data;

	// Show report
	report += TXT("</body></html>");
	HtmlBox( wxTheFrame, TXT("Import from old streaming tiles"), report );
}

void CEdWorldEditPanel::OnForceStreamInInsideArea( wxCommandEvent& event )
{
	// Find area
	CAreaComponent* area = nullptr;
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );
	for ( Uint32 i=0; !area && i < nodes.Size(); ++i )
	{
		area = Cast< CAreaComponent >( nodes[i] );
	}
	if ( !area ) return;

	Bool addToIgnore = YesNo( TXT("Do you want to add the entities to the streaming ignore list so they are not streamed out? You can remove them from the scene explorer later.") );

	// Stream in stuff
	GetWorld()->GetStreamingSectorData()->ForceStreamForArea( area->GetBoundingBox() );//, addToIgnore );
}

void CEdWorldEditPanel::OnPreviewAPMan( wxCommandEvent& event )
{
	PreviewActionPoint( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_citizen_man.w2ent"), event );
}
void CEdWorldEditPanel::OnPreviewAPBigMan( wxCommandEvent& event )
{
	PreviewActionPoint( TXT("gameplay\\community\\community_npcs\\novigrad\\guards\\novigrad_eternal_fire_guard.w2ent"), event );
}
void CEdWorldEditPanel::OnPreviewAPWoman( wxCommandEvent& event )
{
	PreviewActionPoint( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_noblewoman.w2ent"), event );
}
void CEdWorldEditPanel::OnPreviewAPDwarf( wxCommandEvent& event )
{
	PreviewActionPoint( TXT("gameplay\\community\\community_npcs\\novigrad\\nonhumans\\dwarf_man.w2ent"), event );
}
void CEdWorldEditPanel::OnPreviewAPChild( wxCommandEvent& event )
{
	PreviewActionPoint( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_boy.w2ent"), event );
}

void CEdWorldEditPanel::OnPreviewAPSelectedEntity( wxCommandEvent& event )
{
	String selectedResource;
	if ( GetActiveResource( selectedResource ) )
	{
		PreviewActionPoint( selectedResource.AsChar(), event );
	}
}

void CEdWorldEditPanel::PreviewActionPoint( const String& entityTemplatePath, wxCommandEvent& event )
{
	// Get editor selected entity (only one is allowed for this func to work) 
	TDynArray< CEntity* > selectedEntities;
	GetSelectionManager()->GetSelectedEntities( selectedEntities );

	CEntity* entity = *selectedEntities.Begin();
	if ( !entity )
		return;

	const CActionPointComponent* apComponent = entity->FindComponent< CActionPointComponent >();;
	if ( !apComponent )
		return;

	// Initialize tree & launch editor window
	CJobTree* tree = apComponent->GetJobTreeRes().Get();
	if ( !tree )
		return;

	//now that we have the editor, pass in the entity template path to load an NPC then run the tree
	CEdJobTreeEditor* editor = new CEdJobTreeEditor( NULL, tree );
	editor->GetPreviewPanel()->LoadEntity(entityTemplatePath);

	editor->OnPlayWorld( event );
}

void CEdWorldEditPanel::OnEncounterEditorClosed( wxWindowDestroyEvent& event )
{
	if ( CEdEncounterEditor* editor = dynamic_cast< CEdEncounterEditor* >( event.GetEventObject() ) )
	{
		m_spawnTreeEditors.EraseByValue( editor );
	}
}

Bool CEdWorldEditPanel::CanSpawnEntity( CEntityTemplate* entityTemplate )
{
	if ( entityTemplate )
	{
		if ( entityTemplate->GetEntityClassName() == CNAME( CItemEntity ) )
		{
			// Not this type
			return false;
		}
	}

	// Probably can :)
	return true;
}

void CEdWorldEditPanel::CreateAreaFromPolygon()
{
	CLayer* activeLayer = m_scene->GetActiveLayer();
	m_polygonDrawing = false;

	if ( !activeLayer || !activeLayer->GetFile()->MarkModified() )
	{
		wxMessageBox( wxT("Please activate a layer that you can modify before drawing a polygon"), wxT("No active layer"), wxICON_ERROR|wxOK );
	}
	else
	{
		// Find all area templates
		TDynArray<String> choices;
		THashMap<String, TemplateInfo*> templates;
		for ( Uint32 i = 0; i < m_templates.Size(); ++i )
		{
			String name;
			m_templates[ i ]->GetName( name );
			
			if ( name.ToLower().ContainsSubstring( TXT("area") ) )
			{
				choices.PushBack( name );
				templates[ name ] = m_templates[i];
			}
		}

		// Ask the user which one he wants
		String className = InputComboBox( this, TXT("Create area from polygon"), wxT("What kind of area do you want?"), wxT(""), choices );
		TemplateInfo* ti;
		if ( templates.Find( className, ti ) )
		{
			// Shorten the polygon point array
			m_polygonPoints.Resize( m_polygonPoints.Size() - 1 );

			// Find the entity's position (should be the center of all points)
			Box box;
			box.Clear();
			for ( Uint32 i=0; i<m_polygonPoints.Size(); ++i )
			{
				box.AddPoint( m_polygonPoints[i] );
			}
			Vector spawnPosition = box.CalcCenter();
			spawnPosition.Z = box.Min.Z;
			box.Max.Z += 2;

			// Bring the polygon points to local space and set the same height
			for ( Uint32 i=0; i<m_polygonPoints.Size(); ++i )
			{
				m_polygonPoints[i].X -= spawnPosition.X;
				m_polygonPoints[i].Y -= spawnPosition.Y;
				m_polygonPoints[i].Z = 0;
			}

			// Flip the points if they are not in counter-clockwise order
			if ( IsPolygonClockwise( m_polygonPoints ) )
			{
				TDynArray< Vector > flipped;
				flipped.PushBack( m_polygonPoints[0] );
				for ( Uint32 i=1; i<m_polygonPoints.Size(); ++i )
				{
					flipped.PushBack( m_polygonPoints[m_polygonPoints.Size() - i] );
				}
				m_polygonPoints.Clear();
				m_polygonPoints.PushBack( flipped );
			}

			// Create the area entity
			CEntity *newEntity = ti->Create( activeLayer, spawnPosition, NULL );
			if ( newEntity )
			{
				wxTheFrame->AddToIsolated( newEntity );

				CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), newEntity, true );
				CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );

				// Find the area component
				CAreaComponent* area = NULL;
				const TDynArray<CComponent*> components = newEntity->GetComponents() ;
				for ( Uint32 i=0; i<components.Size(); ++i )
				{
					if ( components[i]->IsA<CAreaComponent>() )
					{
						area = Cast<CAreaComponent>( components[i] );
						break;
					}
				}

				// Setup area
				if ( area )
				{
					CAreaComponent::TAreaPoints polygonPoints;
					polygonPoints = (const CAreaComponent::TAreaPoints&) m_polygonPoints;

					area->SetLocalPoints( polygonPoints, true );
					area->SetScale( Vector( 1, 1, ( box.Max.Z - box.Min.Z )/area->m_height ) );
					area->ForceUpdateBoundsNode();
					area->ForceUpdateTransformNodeAndCommitChanges();
				}
				else
				{
					newEntity->Destroy();
					wxMessageBox( wxT("Failed to create the area - the CAreaComponent was not found in the new entity"), wxT("Error"), wxICON_ERROR|wxOK );
				}
			}
			else
			{
				wxMessageBox( wxT("Failed to create the area entity"), wxT("Error"), wxICON_ERROR|wxOK );
			}			
		}
	}
}

Bool CEdWorldEditPanel::AlignEntitiesToPositionFromViewportXY( Int32 x, Int32 y, const TDynArray< CEntity* >& entities, Bool applyRotation, Bool addUndoSteps )
{
	if ( CalcClickedPosition( x, y ) )
	{
		AlignEntitiesToPointAndNormal( entities, m_clickedWorldPos, m_clickedWorldNormal, applyRotation, addUndoSteps );
		return true;
	}
	return false;
}

Bool CEdWorldEditPanel::GetVisibleTerrainBounds( Float& minX, Float& minY, Float& maxX, Float& maxY )
{
	Vector p[4];

	if ( !GetWorld() ) return false;

	if ( !GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), 0, 0, p[0], NULL, true ) ) return false;
	if ( !GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), GetSize().GetWidth() - 1, 0, p[1], NULL, true ) ) return false;
	if ( !GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), GetSize().GetWidth() - 1, GetSize().GetHeight() - 1, p[2], NULL, true ) ) return false;
	if ( !GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), 0, GetSize().GetHeight() - 1, p[3], NULL, true ) ) return false;

	for ( Uint32 i=0; i<4; ++i )
	{
		if ( i == 0 || minX > p[i].X ) minX = p[i].X;
		if ( i == 0 || minY > p[i].Y ) minY = p[i].Y;
		if ( i == 0 || maxX < p[i].X ) maxX = p[i].X;
		if ( i == 0 || maxY < p[i].Y ) maxY = p[i].Y;
	}

	return true;
}

void CEdWorldEditPanel::OnViewportSetDimensions( IViewport* view )
{
#ifndef NO_RED_GUI
	GRedGui::GetInstance().OnViewportSetDimensions( view, view->GetWidth(), view->GetHeight() );
#endif
}

void CalcRenderCameraFromEditorPreview( CRenderCamera& camera )
{
	Vector camPos = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
	EulerAngles camRot = wxTheFrame->GetWorldEditPanel()->GetCameraRotation();
	Float camFov = wxTheFrame->GetWorldEditPanel()->GetCameraFov();
	Uint32 h = wxTheFrame->GetWorldEditPanel()->GetViewport()->GetHeight();
	Uint32 w = wxTheFrame->GetWorldEditPanel()->GetViewport()->GetWidth();

	camera = CRenderCamera( camPos, camRot, camFov, w / (Float)h, 0.2f, 1900.0 );
}

Vector CalcCameraPosFromEditorPreview()
{
	return wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
}
