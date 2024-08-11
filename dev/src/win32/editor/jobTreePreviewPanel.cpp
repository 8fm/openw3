/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/actor.h"
#include "../../common/game/definitionsManager.h"
#include "../../common/game/jobTreeNode.h"
#include "../../common/game/jobTreeLeaf.h"
#include "../../common/game/extAnimItemEvents.h"
#include "../../common/core/depot.h"

#include "editorExternalResources.h"
#include "jobTreeEditor.h"
#include "jobTreePreviewPanel.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"


#define ID_MENU_ENTITY			100
#define ID_PREVIEW_AP_MAN		101
#define ID_PREVIEW_AP_BIG_MAN	102
#define ID_PREVIEW_AP_WOMAN		103
#define ID_PREVIEW_AP_DWARF		104
#define ID_PREVIEW_AP_CHILD		105
#define ID_PREVIEW_AP_EP2_MAN	106
#define ID_PREVIEW_AP_EP2_WOMAN	107



BEGIN_EVENT_TABLE( CEdJobTreePreviewPanel, CEdPreviewPanel )
EVT_MENU( ID_MENU_ENTITY, CEdJobTreePreviewPanel::OnLoadEntity )
EVT_MENU( ID_PREVIEW_AP_EP2_MAN, CEdJobTreePreviewPanel::OnLoadEp2Man )
EVT_MENU( ID_PREVIEW_AP_EP2_WOMAN, CEdJobTreePreviewPanel::OnLoadEp2Woman )
EVT_MENU( ID_PREVIEW_AP_MAN, CEdJobTreePreviewPanel::OnLoadMan )
EVT_MENU( ID_PREVIEW_AP_BIG_MAN, CEdJobTreePreviewPanel::OnLoadBigMan )
EVT_MENU( ID_PREVIEW_AP_WOMAN, CEdJobTreePreviewPanel::OnLoadWoman )
EVT_MENU( ID_PREVIEW_AP_DWARF, CEdJobTreePreviewPanel::OnLoadDwarf )
EVT_MENU( ID_PREVIEW_AP_CHILD, CEdJobTreePreviewPanel::OnLoadChild )
END_EVENT_TABLE()

const String	CEdJobTreePreviewPanel::CONFIG_PATH( TXT( "/Frames/JobTreePreview/" ) );
const String	CEdJobTreePreviewPanel::CONFIG_ACTOR( TXT( "Actor" ) );
const CName		CEdJobTreePreviewPanel::JTP_SLOT_NAME( TXT("NPC_ANIM_SLOT") );

CEdJobTreePreviewPanel::CEdJobTreePreviewPanel( wxWindow* parent, CEdJobTreeEditor* editor )
: CEdPreviewPanel( parent )
, m_jobTreeEditor( editor )
, m_jobTreeNode( NULL )  
, m_jobContext( NULL )
, m_currentActor( NULL )
, m_animated( NULL )
, m_currentItemEntity( NULL )
, m_currentlyPreviewedAction( NULL )
{
	// Render with lighting
	GetViewport()->SetRenderingMode( RM_Shaded );
	SetCameraMode( RPCM_DefaultFlyby );
	SetCameraPosition( Vector::ZEROS );

	m_previewWorld->DelayedActions();

	Init();
}

CEdJobTreePreviewPanel::~CEdJobTreePreviewPanel()
{
	if ( m_currentActor )
	{
		UnregisterAnimEventHandler();
		m_currentActor = NULL;
	}
}

void CEdJobTreePreviewPanel::Init()
{
	if ( !m_currentActor )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, CONFIG_PATH );

		// Retrieve animset name
		String actorPath = config.Read( CONFIG_ACTOR, String::EMPTY );

		if ( actorPath != String::EMPTY )
		{
			LoadEntity( actorPath );
		}
	}
}

void CEdJobTreePreviewPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	__super::OnViewportTick( view, timeDelta );

	SItemEntityManager::GetInstance().OnTick( timeDelta );
}

void CEdJobTreePreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Generate normal fragments
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );
}

CEntity* CEdJobTreePreviewPanel::LoadEntity( const String& entityTemplatePath )
{
	CEntityTemplate *entTemplate = LoadResource< CEntityTemplate>( entityTemplatePath );
	if ( !entTemplate )
	{
		ERR_EDITOR( TXT("CEdJobTreePreviewPanel::OnLoadEntity - Failed to load %s entity"), entityTemplatePath.AsChar() );
		return NULL;
	}

	CEntity* ent = LoadEntity( entTemplate );
	if ( ent )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, CONFIG_PATH );

		// Save animset filename
		config.Write( CONFIG_ACTOR, entityTemplatePath );
	}
	return ent;
}

CEntity* CEdJobTreePreviewPanel::LoadEntity( CEntityTemplate* entityTemplate )
{
	EntitySpawnInfo einfo;
	einfo.m_template = entityTemplate;
	einfo.m_previewOnly = true;
	//einfo.m_detachTemplate = true;

	const TDynArray<CEntityAppearance> & appearances = entityTemplate->GetAppearances();

	if ( appearances.Size() )
	{
		einfo.m_appearances.PushBack( appearances[0].GetName() );
	}

	CEntity* entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );	
	if ( !entity )
	{
		//entityTemplate->GetEntityClassName()
		ERR_EDITOR( TXT("Failed to create actor") );
		return NULL;
	}
	if ( !entity->IsA< CActor >() )
	{
		ERR_EDITOR( TXT("Failed to create actor") );
		entity->Destroy();
		return NULL;
	}

	if ( m_currentActor )
	{
		m_currentActor->Destroy();
		m_currentActor = NULL;
	}

	m_currentActor = SafeCast<CActor>( entity );
	ASSERT( m_currentActor );
	

	// Setup camera look at
	m_cameraPosition.Set3( 3.0f, 3.0f, 3.0f );

	Vector camForward = Vector::ZERO_3D_POINT - m_cameraPosition;
	camForward.Normalize3();

	Matrix rotation;
	rotation.BuildFromDirectionVector( camForward );
	m_cameraRotation = rotation.ToEulerAngles();

	if ( m_jobTreeEditor != NULL )
	{
		m_jobTreeEditor->OnPreviewActorChanged();
	}

	return m_currentActor;
}

void CEdJobTreePreviewPanel::UnloadEntity()
{
	if ( m_jobTreeEditor != NULL )
	{
		m_jobTreeEditor->OnPreviewActorChanged();
	}
}

Bool CEdJobTreePreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{		
	return CEdPreviewPanel::OnViewportInput( view, key, action, data );
}
void CEdJobTreePreviewPanel::OnLoadEp2Man( wxCommandEvent& event )
{
	LoadEntity( TXT("dlc\\bob\\data\\gameplay\\community\\community_npcs\\citizens\\noble.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadEp2Woman( wxCommandEvent& event )
{
	LoadEntity( TXT("dlc\\bob\\data\\gameplay\\community\\community_npcs\\citizens\\noble_woman.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadMan( wxCommandEvent& event )
{
	LoadEntity( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_citizen_man.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadBigMan( wxCommandEvent& event )
{
	LoadEntity( TXT("gameplay\\community\\community_npcs\\novigrad\\guards\\novigrad_eternal_fire_guard.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadWoman( wxCommandEvent& event )
{
	LoadEntity( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_noblewoman.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadDwarf( wxCommandEvent& event )
{
	LoadEntity( TXT("gameplay\\community\\community_npcs\\novigrad\\nonhumans\\dwarf_man.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadChild( wxCommandEvent& event )
{
	LoadEntity( TXT("gameplay\\community\\community_npcs\\novigrad\\regular\\novigrad_boy.w2ent") );
}
void CEdJobTreePreviewPanel::OnLoadEntity( wxCommandEvent& event )
{
	String selectedResource;
	if ( GetActiveResource( selectedResource ) )
	{
		LoadEntity( selectedResource );
	}
}

void CEdJobTreePreviewPanel::HandleContextMenu( Int32 x, Int32 y )
{
	wxMenu menu;
	menu.Append( ID_MENU_ENTITY, TXT("Use selected entity") );
	menu.AppendSeparator();
	menu.Append( ID_PREVIEW_AP_MAN, TXT("Load Man") );
	menu.Append( ID_PREVIEW_AP_BIG_MAN, TXT("Load Big Man") );
	menu.Append( ID_PREVIEW_AP_WOMAN, TXT("Load Woman") );
	menu.Append( ID_PREVIEW_AP_DWARF, TXT("Load Dwarf") );
	menu.Append( ID_PREVIEW_AP_CHILD, TXT("Load Child") );
	menu.AppendSeparator();
	menu.Append( ID_PREVIEW_AP_EP2_MAN, TXT("Load Ep2 Man") );
	menu.Append( ID_PREVIEW_AP_EP2_WOMAN, TXT("Load Ep2 Woman") );
	PopupMenu( &menu, x, y );
}

void CEdJobTreePreviewPanel::SetJobTree( CJobTreeNode* jobTree )
{
	m_jobTreeNode = jobTree;
}

void CEdJobTreePreviewPanel::Play( CName category )
{
	if ( !m_currentActor )
	{
		wxMessageBox( TXT( "You need to select entity first" ) );
		return;
	}
	if ( !m_jobContext )
	{
		m_jobContext = new SJobTreeExecutionContext;
	}
	m_jobContext->Reset();
	m_jobContext->m_currentCategories.PushBackUnique( category );

	m_animated = m_currentActor->GetRootAnimatedComponent();
	m_animated->SetUseExtractedMotion( true );
	CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_animated );
	if ( mac )
	{
		mac->ForceEntityRepresentation( true );
		mac->GetRepresentationStack()->OnActivate( Vector::ZEROS, EulerAngles::ZEROS );
	}
	m_currentActor->Teleport(Vector::ZEROS, EulerAngles::ZEROS);

	RegisterAnimEventHandler();

	m_animated->SetDispSkeleton( ACDD_SkeletonBone, true );
	m_animated->SetDispSkeleton( ACDD_SkeletonAxis, true );
	m_animated->SetDispSkeleton( ACDD_SkeletonName, true );

	const CJobActionBase* action = m_jobTreeNode->GetNextAction( *m_jobContext );
	if ( action && m_currentActor->GetAgent() && m_currentActor->GetAgent()->GetBehaviorStack() )
	{
		// Setup slot
		SBehaviorSlotSetup slotSetup;
		slotSetup.m_blendIn = action->GetBlendIn();
		slotSetup.m_blendOut = action->GetBlendOut();
		slotSetup.m_listener = this;

		m_currentActionItemName = action->GetItemName();
		m_currentActor->GetAgent()->GetBehaviorStack()->PlaySlotAnimation( JTP_SLOT_NAME, action->GetAnimName(), &slotSetup );
		m_currentlyPreviewedAction = action;
		m_jobTreeEditor->OnActionPreviewStarted( m_currentlyPreviewedAction );
	}	
}

void CEdJobTreePreviewPanel::Stop()
{
	if ( m_currentActor && m_currentActor->GetAgent() && m_currentActor->GetAgent()->GetBehaviorStack() )
	{
		m_currentActor->GetAgent()->GetBehaviorStack()->StopSlotAnimation( JTP_SLOT_NAME );	
		UnregisterAnimEventHandler();

		if ( m_currentItemEntity )
		{
			m_currentActor->DetachEntityFromSkeleton( m_currentItemEntity );
			m_currentItemEntity->Destroy();
			GGame->GetActiveWorld()->DelayedActions();
			m_currentItemEntity = NULL;
		}

		if ( m_currentlyPreviewedAction )
		{
			m_jobTreeEditor->OnActionPreviewEnded( m_currentlyPreviewedAction );
			m_currentlyPreviewedAction = NULL;
		}
	}
}

void CEdJobTreePreviewPanel::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status )
{
	if ( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted )
	{
		if ( m_currentlyPreviewedAction )
		{
			m_jobTreeEditor->OnActionPreviewEnded( m_currentlyPreviewedAction );
			m_currentlyPreviewedAction = NULL;
		}
		
		const CJobActionBase* action = m_jobTreeNode->GetNextAction( *m_jobContext );
		if ( action )
		{
			m_currentActionItemName = action->GetItemName();

			SBehaviorSlotSetup slotSetup;
			slotSetup.m_blendIn = action->GetBlendIn();
			slotSetup.m_blendOut = action->GetBlendOut();
			slotSetup.m_listener = this;

			sender->PlayAnimation( instance, action->GetAnimName(), &slotSetup );

			m_currentlyPreviewedAction = action;
			m_jobTreeEditor->OnActionPreviewStarted( action );
		}
	}
}

void CEdJobTreePreviewPanel::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.GetEventName() == CNAME( take_item ) )
	{
		PerformPickUp();
	}
	else if ( event.GetEventName() == CNAME( leave_item ) )
	{
		PerformPut();
	}
	else if ( event.GetEventName() == CNAME( item_effect ) )
	{
		if ( m_currentItemEntity )
		{
			const CExtAnimItemEffectEvent* itemEffectEvent = (const CExtAnimItemEffectEvent*)( event.m_extEvent );
			if ( itemEffectEvent )
			{
				if ( itemEffectEvent->GetEffectAction() == IEA_Start )
				{
					m_currentItemEntity->PlayEffect( itemEffectEvent->GetEffectName() );
				}
				else
				{
					m_currentItemEntity->StopEffect( itemEffectEvent->GetEffectName() );
				}
			}
		}
	}
}

void CEdJobTreePreviewPanel::RegisterAnimEventHandler()
{
	if ( !m_animated )
	{
		return;
	}
	// Register this action as animation event handler
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->RegisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( item_effect ) )->RegisterHandler( this );

}

void CEdJobTreePreviewPanel::UnregisterAnimEventHandler()
{
	if ( !m_animated )
	{
		return;
	}
	// Unregister this action from anim event handlers
	m_animated->GetAnimationEventNotifier( CNAME( take_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( leave_item ) )->UnregisterHandler( this );
	m_animated->GetAnimationEventNotifier( CNAME( item_effect ) )->UnregisterHandler( this );
}

void CEdJobTreePreviewPanel::PerformPickUp()
{
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( m_currentActionItemName );
	if ( !itemDef )
	{
		return;
	}

	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_currentItemEntity );

	CInventoryComponent* inventoryComponent = nullptr;
	
	if( m_currentItemEntity )
	{
		inventoryComponent = m_currentItemEntity->FindComponent<CInventoryComponent>();
	}

	String templateName;
	if ( inventoryComponent )
	{
		templateName = inventoryComponent->GetTemplate( m_currentActionItemName );
	}
	else
	{
		templateName = itemDef->GetEquipTemplateName( false );
	}

	const String& str = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( templateName );
	if( !str.Empty() )
	{
		CEntityTemplate * entityTemplate = SafeCast< CEntityTemplate >( GDepot->LoadResource( str ) );
		if( entityTemplate )
		{			
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = entityTemplate;
			spawnInfo.m_spawnPosition = Vector::ZERO_3D_POINT;				
			m_currentItemEntity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );

			if( !appearanceComponent )
			{
				appearanceComponent = CAppearanceComponent::GetAppearanceComponent( m_currentItemEntity );
			}

			if( m_currentItemEntity )
			{			
				if ( appearanceComponent && itemDef->GetItemAppearanceName( false ) != CName::NONE )
				{
					appearanceComponent->ApplyAppearance( itemDef->GetItemAppearanceName( false ) );
				}			

				m_currentActor->AttachEntityToBone( m_currentItemEntity, itemDef->GetHoldSlot( false ) );

#ifndef NO_EDITOR_GRAPH_SUPPORT
				// Appending sync token for new item
				if ( const CAnimatedComponent* animatedComponent = m_currentActor->GetRootAnimatedComponent() )
				{
					if ( CBehaviorGraphStack* behaviorStack = animatedComponent->GetBehaviorStack() )
					{
						behaviorStack->AppendSyncTokenForEntityOnSlot( JTP_SLOT_NAME, m_currentItemEntity );
					}
				}
#endif
			}			
		}
	}
}

void CEdJobTreePreviewPanel::PerformPut()
{
	if ( m_currentItemEntity )
	{
		m_currentActor->DetachEntityFromSkeleton( m_currentItemEntity );
		m_currentItemEntity->Destroy();
		m_currentItemEntity = NULL;
	}
}

void CEdJobTreePreviewPanel::PlayAnimation( CName animationName, CName itemName /* = CName::NONE */ )
{
	if ( !m_currentActor )
	{
		wxMessageBox( TXT( "You need to select entity first" ) );
		return;
	}

	m_animated = m_currentActor->GetRootAnimatedComponent();
	m_animated->SetUseExtractedMotion( true );
	CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_animated );
	if ( mac )
	{
		mac->ForceEntityRepresentation( true );
	}

	RegisterAnimEventHandler();

	m_animated->SetDispSkeleton( ACDD_SkeletonBone, true );
	m_animated->SetDispSkeleton( ACDD_SkeletonAxis, true );
	m_animated->SetDispSkeleton( ACDD_SkeletonName, true );

	if ( m_currentActor->GetAgent() == NULL )
	{
		return;
	}

	CBehaviorGraphStack* behaviorStack = m_currentActor->GetAgent()->GetBehaviorStack();
	if ( behaviorStack != NULL )
	{
		if ( animationName != CName::NONE )
		{
			if ( itemName != CName::NONE )
			{
				m_currentActionItemName = itemName;
			}
			m_currentActor->GetAgent()->GetBehaviorStack()->PlaySlotAnimation( JTP_SLOT_NAME, animationName );
		}
		else
		{
			m_currentActor->GetAgent()->GetBehaviorStack()->StopSlotAnimation( JTP_SLOT_NAME );
		}
		
	}
}

#undef JTP_SLOT_NAME
