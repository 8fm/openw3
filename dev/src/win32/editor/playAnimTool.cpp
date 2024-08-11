/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/game/movingAgentComponent.h"

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"
#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/actor.h"

#include "../../common/core/depot.h"

#include "sceneExplorer.h"

#include "detachablePanel.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/dynamicLayer.h"


class CEdPlayAnimToolPanel;

/// Editor tool for playing animation in the world
class CEdPlayAnimTool : public IEditorTool, public ISlotAnimationListener
{
	DECLARE_ENGINE_CLASS( CEdPlayAnimTool, IEditorTool, 0 );

protected:
	CEdPlayAnimToolPanel*			m_panel;
	CEdRenderingPanel*				m_viewport;
	CWorld*							m_world;
	THandle< CEntityTemplate >		m_npcEntityTemplate;
	String							m_componentName;
	CName							m_animSlotName;
	CName							m_animName;

public:
	CEdPlayAnimTool() : m_actorEntity( NULL ), m_actor( NULL ), m_selectedComponent( NULL ), m_animSlotName( TXT("NPC_ANIM_SLOT") ) {}

	virtual String GetCaption() const { return TXT("Test Anim"); }
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();
	virtual Bool UsableInActiveWorldOnly() const { return false; }
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
	virtual String GetListenerName() const { return TXT("CEdPlayAnimTool"); }

	void Spawn();
	void Despawn();
	void PlayAnim();
	void PlayAnimAtTime( int animPos ); // 0 - 100
	void SaveProperties();
	void LoadProperties();

private:
	CEntity*	m_actorEntity;
	CActor*		m_actor;
	CComponent*	m_selectedComponent;
};

BEGIN_CLASS_RTTI( CEdPlayAnimTool );
	PARENT_CLASS( IEditorTool );
	PROPERTY_EDIT( m_npcEntityTemplate, TXT("Entity template") );
	PROPERTY_EDIT( m_componentName, TXT("Entity component name (optional)") );
	PROPERTY_EDIT( m_animSlotName, TXT("Anim slot name") );
	PROPERTY_EDIT( m_animName, TXT("Animation name") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdPlayAnimTool );

/// Play Anim tool panel
class CEdPlayAnimToolPanel : public CEdDraggablePanel
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdPlayAnimToolPanel );

protected:
	CEdPlayAnimTool*		m_tool;
	CEdSelectionProperties*	m_properties;
	wxSlider*				m_sliderAnim;
	CEdDetachablePanel		m_detachablePanel;

public:
	CEdPlayAnimToolPanel( CEdPlayAnimTool* tool, wxWindow* parent )
		: m_tool( tool )
	{
		// Load layout from XRC
		wxXmlResource::Get()->LoadPanel( this, parent, wxT("PlayAnimTool") );

		m_sliderAnim = XRCCTRL( *this, "sliderAnim", wxSlider );

		// Create properties
		{
			wxPanel* rp = XRCCTRL( *this, "panelProperties", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			settings.m_showEntityComponents = false;
			m_properties = new CEdSelectionProperties( rp, settings, NULL );
			m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdPlayAnimToolPanel::OnPropertiesChanged ), NULL, this );
			sizer1->Add( m_properties, 1, wxEXPAND, 0 );
			rp->SetSizer( sizer1 );
			rp->Layout();

			m_properties->Get().SetObject( m_tool );
		}

		m_detachablePanel.Initialize( this, TXT( "Test Anim" ) );
	}

protected:
	void OnPlayAnim( wxCommandEvent &event )
	{
		m_tool->PlayAnim();
	}

	void OnSpawn( wxCommandEvent &event )
	{
		m_tool->Spawn();
	}

	void OnDespawn( wxCommandEvent &event )
	{
		m_tool->Despawn();
	}

	void OnPropertiesChanged( wxCommandEvent &event )
	{
		//m_tool->SaveProperties();
	}

	void OnAnimScroll( wxScrollEvent &event )
	{
		m_tool->PlayAnimAtTime( m_sliderAnim->GetValue() );
	}
};

wxIMPLEMENT_CLASS( CEdPlayAnimToolPanel, CEdDraggablePanel );

BEGIN_EVENT_TABLE( CEdPlayAnimToolPanel, CEdDraggablePanel )
	EVT_BUTTON( XRCID("buttonPlayAnim"), CEdPlayAnimToolPanel::OnPlayAnim )
	EVT_BUTTON( XRCID("buttonSpawn"), CEdPlayAnimToolPanel::OnSpawn )
	EVT_BUTTON( XRCID("buttonDespawn"), CEdPlayAnimToolPanel::OnDespawn )
	EVT_COMMAND_SCROLL( XRCID("sliderAnim"), CEdPlayAnimToolPanel::OnAnimScroll )
END_EVENT_TABLE()

Bool CEdPlayAnimTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_world    = world;
	m_viewport = viewport;

	LoadProperties();

	if ( ! selection.Empty() )
	{
		m_selectedComponent = *selection.Begin();
	}

	// Create tool panel
	m_panel = new CEdPlayAnimToolPanel( this, panel );

	// Create panel for custom window
	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();

	return true;
}

void CEdPlayAnimTool::End()
{
	SaveProperties();
	Despawn();
}

Bool CEdPlayAnimTool::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager::CSelectionTransaction transaction( *m_world->GetSelectionManager() );

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_world->GetSelectionManager()->DeselectAll();
	}

	// Select only sprites
	for ( Uint32 i=0; i < objects.Size(); i++ )
	{
		CComponent *selObj = Cast< CComponent >( objects[i]->GetHitObject() );
		if ( selObj )
		{
			m_selectedComponent = selObj;
			m_world->GetSelectionManager()->Select( selObj );
			break;
		}
	}

	// Handled
	return true;
}

void CEdPlayAnimTool::Spawn()
{
	// Get world
	CWorld* world = m_world;
	if( world == NULL )
	{
		wxMessageBox( TXT( "You need to have world opened" ) );
		return;
	}

	if ( m_npcEntityTemplate.Get() == NULL  )
	{
		wxMessageBox( TXT( "You need to select actor template first" ) );
		return;
	}

	// Find entity for previewing
	TDynArray< CEntity* > selectedEntities;
	world->GetSelectionManager()->GetSelectedEntities( selectedEntities );

	if( selectedEntities.Size() == 0 && m_selectedComponent == NULL )
	{
		wxMessageBox( TXT( "You need to select at least one entity in the world" ) );
		return;
	}

	Vector		spawnPosition;
	EulerAngles	spawnRotation;
	if ( m_selectedComponent )
	{
		spawnPosition = m_selectedComponent->GetWorldPosition();
		spawnRotation = m_selectedComponent->GetWorldRotation();
	}
	else if ( m_componentName != String::EMPTY )
	{
		CComponent* component = NULL;
		// Find needed component
		for( TDynArray< CEntity* >::iterator entityIter = selectedEntities.Begin();
			entityIter != selectedEntities.End();
			++entityIter )
		{
			CEntity* entity = *entityIter;

			component = entity->FindComponent( m_componentName );
			if( component != NULL )
			{
				break;
			}
		}

		if( component == NULL )
		{
			wxMessageBox( String::Printf( TXT( "Selected entity does not have component '%s'" ),
				m_componentName.AsChar() ).AsChar() );
			return;
		}

		spawnPosition = component->GetWorldPosition();
		spawnRotation = component->GetWorldRotation();
	}
	else
	{
		spawnPosition = selectedEntities[0]->GetWorldPosition();
		spawnRotation = selectedEntities[0]->GetWorldRotation();
	}

	// Spawn entity
	if( m_actorEntity != NULL )
	{
		m_actorEntity->Destroy();
		world->DelayedActions();
		m_actorEntity = NULL;
	}

	EntitySpawnInfo info;
	info.m_template = m_npcEntityTemplate;
	info.m_spawnPosition = spawnPosition;
	info.m_spawnRotation = spawnRotation;
	// use pointer as randomizer in entity name
	info.m_name = String::Printf( TXT( "PlayAnimToolEntity_%d" ), ( Int32 ) this );

	m_actorEntity = world->GetDynamicLayer()->CreateEntitySync( info );

	if( m_actorEntity == NULL )
	{
		wxMessageBox( TXT( "Error in spawning entity" ) );
		return;
	}

	// Wait for attaching
	world->DelayedActions();

	m_actor = SafeCast< CActor >( m_actorEntity );
}

void CEdPlayAnimTool::Despawn()
{
	if( m_actorEntity != NULL )
	{
		m_actorEntity->Destroy();
		m_world->DelayedActions();
		m_actorEntity = NULL;
	}

	m_actor = NULL;
}

void CEdPlayAnimTool::PlayAnim()
{
	if ( m_actor && m_actor->GetAgent() && m_actor->GetAgent()->GetBehaviorStack() )
	{
		// Setup slot
		SBehaviorSlotSetup slotSetup;
		slotSetup.m_blendIn = 0; //action->GetBlendIn();
		slotSetup.m_blendOut = 0; //action->GetBlendOut();
		slotSetup.m_listener = this;

		m_actor->GetAgent()->GetBehaviorStack()->PlaySlotAnimation( m_animSlotName, m_animName, &slotSetup );
	}
}

void CEdPlayAnimTool::PlayAnimAtTime( int animPos )
{
	if ( m_actor )
	{
		CAnimatedComponent * ac = m_actor->GetRootAnimatedComponent();
		if ( ac )
		{
			Float animDuration = ac->GetAnimationDuration( m_animName );
			if ( animDuration > 0 )
			{
				Float animTime = animDuration * ((Float)animPos/100.0f);

				if ( ! ac->PlayAnimationAtTimeOnSkeleton( m_animName, animTime ) )
				{
					LOG_EDITOR( TXT("Cannot play animation %s"), m_animName.AsString().AsChar() );
				}
			}
		}
	}
}

void CEdPlayAnimTool::SaveProperties()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/PlayAnimTool") );

	config.Write( TXT("NpcEntityTemplate"), m_npcEntityTemplate.Get() != NULL ? m_npcEntityTemplate->GetFile()->GetDepotPath() : String::EMPTY );
}

void CEdPlayAnimTool::LoadProperties()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/PlayAnimTool") );

	String resPath;
	if ( config.Read( TXT("NpcEntityTemplate"), &resPath ) )
	{
		m_npcEntityTemplate = LoadResource< CEntityTemplate >( resPath );
	}
}

void CEdPlayAnimTool::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status )
{

}
