/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "maraudersMapItems.h"

#include "../../common/engine/deniedAreaComponent.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/stickerComponent.h"
#include "../../common/engine/triggerAreaComponent.h"
#include "../../common/engine/meshTypeComponent.h"

#include "../../common/game/actionPointComponent.h"
#include "../../common/game/behTreeGuardAreaData.h"
#include "../../common/game/behTreeMachineListener.h"
#include "../../common/game/behTreeMachine.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/communityArea.h"
#include "../../common/game/communityUtility.h"
#include "../../common/game/communityAgentStub.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/game/moveLocomotion.h"
#include "../../common/game/moveGlobalPathPlanner.h"
#include "../../common/game/movePathIterator.h"
#include "../../common/game/newNpcSchedule.h"
#include "../../common/game/nodeStorage.h"
#include "../../common/game/player.h"
#include "../../common/game/attackRange.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/game/movableRepresentationPhysicalCharacter.h"
#include "../../common/game/doorComponent.h"

#include "reactionsDebugger.h"



//////////////////////////////////////////////////////////////////////////
// names
RED_DEFINE_STATIC_NAME( GetStat );
RED_DEFINE_STATIC_NAME( GetStatMax );
RED_DEFINE_STATIC_NAME( UsesVitality );
RED_DEFINE_STATIC_NAME( IsAlive );
RED_DEFINE_STATIC_NAME( IsInvulnerable );
RED_DEFINE_STATIC_NAME( IsImmortal );


namespace // anonymous
{
	class HtmlCommunityDebugPage : public ICommunityDebugPage
	{
	private:
		String		m_msg;

	public:
		RED_INLINE const String& GetMessage() const { return m_msg; }

		virtual void AddText( const String& str, const Color& color )
		{
			m_msg += str + TXT( "<br>" );
		}

		virtual void FocusOn( const Vector& pos ) {}
	};
} // anonymous

const String CMaraudersMapItemNPC::OPTION_BEHEAVIORDEBUG = TXT("Behavior Graph Debug");
const String CMaraudersMapItemNPC::OPTION_BEHTREEDEBUG = TXT("AI Tree Debug");
const String CMaraudersMapItemNPC::OPTION_STEERINGDEBUG = TXT("Steering Tree Debug");
const String CMaraudersMapItemNPC::OPTION_REACTIONSDEBUG = TXT("Reactions Debug");
const String CMaraudersMapItemNPC::OPTION_DESTROY = TXT("Destroy");
const String CMaraudersMapItemNPC::OPTION_KILL = TXT("Kill");
const String CMaraudersMapItemNPC::OPTION_STUN = TXT("Stun");
const String CMaraudersMapItemNPC::OPTION_LIST_APS = TXT("Available action points");
const String CMaraudersMapItemNPC::OPTION_PAUSE_GAME_ON_AP_CHANGE = TXT("Pause on AP change");
const String CMaraudersMapItemNPC::OPTION_PAUSE_GAME_ON_NPC_STATE_CHANGE = TXT("Pause on state change");
const String CMaraudersMapItemNPC::OPTION_PAUSE_GAME_ON_NPC_SCHEDULE_CHANGE = TXT("Pause on schedule change");
const String CMaraudersMapItemNPC::OPTION_PAUSE_GAME_ON_BEHAVIOR_CHANGE = TXT("Pause on behavior change");
const String CMaraudersMapItemNPC::OPTION_SKELETON = TXT("Show/Hide skeleton");
const String CMaraudersMapItemNPC::OPTION_SET_ATTITUDE_HOSTILE = TXT("Set attitude hostile");

TDynArray< String > CMaraudersMapItemNPC::s_optionsNames;
TDynArray< String > CMaraudersMapItemBase::s_noTracks;
TDynArray< String > CMaraudersMapItemNPC::s_npcAITracks;
TDynArray< SAIEvent > CMaraudersMapItemBase::s_noHistory;
CMaraudersNavigationCanvas CMaraudersMapItemNPC::s_navigationCanvas;

CMaraudersMapItemNPC::CMaraudersMapItemNPC( CNewNPC *actor )
	: m_npc( actor )
	, m_isPauseApEnabled( false )
	, m_isPauseNpcStateEnabled( false )
	, m_isPauseNpcScheduleEnabled( false )
	, m_isPauseBehaviorEnabled( false )
	, m_isFastUpdateRequired( false )
	, m_debugAI( false )
{
	if( s_optionsNames.Size() == 0 )
	{
		s_optionsNames.PushBack( OPTION_BEHEAVIORDEBUG );
		s_optionsNames.PushBack( OPTION_BEHTREEDEBUG );
		s_optionsNames.PushBack( OPTION_STEERINGDEBUG );
		s_optionsNames.PushBack( OPTION_REACTIONSDEBUG );
		s_optionsNames.PushBack( OPTION_DESTROY );
		s_optionsNames.PushBack( OPTION_KILL );
		s_optionsNames.PushBack( OPTION_STUN );
		s_optionsNames.PushBack( OPTION_LIST_APS );
		s_optionsNames.PushBack( OPTION_PAUSE_GAME_ON_AP_CHANGE );
		s_optionsNames.PushBack( OPTION_PAUSE_GAME_ON_NPC_STATE_CHANGE );
		s_optionsNames.PushBack( OPTION_PAUSE_GAME_ON_NPC_SCHEDULE_CHANGE );
		s_optionsNames.PushBack( OPTION_PAUSE_GAME_ON_BEHAVIOR_CHANGE );
		s_optionsNames.PushBack( OPTION_SKELETON );
		s_optionsNames.PushBack( OPTION_SET_ATTITUDE_HOSTILE );
	}

	if ( s_npcAITracks.Empty() )
	{
		CEnum* aiTracksEnum = SRTTI::GetInstance().FindEnum( CNAME( EAIEventType ) );
		const TDynArray< CName >& options = aiTracksEnum->GetOptions();
		for ( TDynArray< CName >::const_iterator it = options.Begin(); it != options.End(); ++it )
		{
			s_npcAITracks.PushBack( it->AsString() );
		}
	}

	actor->AttachAIDebugListener( *this );
}

CMaraudersMapItemNPC::~CMaraudersMapItemNPC()
{
	CNewNPC *actor = m_npc.Get();
	if ( actor )
	{
		actor->DetachAIDebugListener( *this );
	}
}

String CMaraudersMapItemNPC::GetShortDescription() const
{
	String txt(TXT("Description"));
	if ( m_npc.Get() )
	{
		txt = TXT("NPC : ") + m_npc.Get()->GetFriendlyName();
	}
	return txt;
}

String CMaraudersMapItemNPC::GetFullDescription() const
{
	String result(TXT("Description"));
	CNewNPC* actor = m_npc.Get();
	if ( actor )
	{
		result.Clear();
		const CFunction* topFunction = actor->GetTopLevelFunction();
		CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();

		// monster type
		Bool vitMonster = false;
		{
			CallFunctionRet< Bool >( actor, CNAME( UsesVitality ), vitMonster );
		}

		// get hp (vitality or essence)
		Float statHPMax = 0, statHP = 0;
		{
			const Int32 statIndex = vitMonster ? 0 /*BCS_Vitality*/ : 1 /*BCS_Essence*/;
			CallFunctionRet< Float >( actor, CNAME( GetStatMax ), statIndex , statHPMax );
			CallFunctionRet< Float >( actor, CNAME( GetStat ), statIndex, statHP );
		}

		// immortality
		Bool invulnerable = false, immortal = false;
		{
			CallFunctionRet< Bool >( actor, CNAME( IsImmortal ), immortal );
			CallFunctionRet< Bool >( actor, CNAME( IsInvulnerable ), invulnerable );
		}

		// is alive - from scripts because is overridden
		Bool isAlive = false;
		{
			CallFunctionRet< Bool >( actor, CNAME( IsAlive ), isAlive );
		}

		Bool isHiddenInGame = actor->IsHiddenInGame();
		Bool isVisible = false;
		Bool shouldBeVisible = false;
		ComponentIterator< CDrawableComponent > it( actor );
		while ( it )
		{
			CDrawableComponent* component = *it;
			if ( component->CanAttachToRenderScene() )
			{
				shouldBeVisible = true;
			}
			if ( component->GetNumberOfRenderProxies() > 0 )
			{
				isVisible = true;
			}
			++it;
		}


		result += TXT("Display name: ") + actor->GetDisplayName() + TXT("<br>");
		result += TXT("Friendly name: ") + actor->GetFriendlyName() + TXT("<br>");
		result += String::Printf( TXT("Pointer: 0x%p <br>"), actor );
		result += TXT("Template friendly name: ") + actor->GetTemplate()->GetFriendlyName() + TXT("<br>");
		result += TXT( "HP: " ) + ToString( statHP ) + TXT( "/" ) + ToString( statHPMax ) + (vitMonster ? TXT(" (vitality)") : TXT(" (essence)")) + TXT("<br>");
		result += TXT( "Invulnerable: " ) + ToString( invulnerable ) + TXT("<br>");
		result += TXT( "Immortal: " ) + ToString( immortal ) + TXT("<br>");
		result += String( TXT("Script function: ") ) + (topFunction ? topFunction->GetName().AsString() : TXT("None")) + TXT("<br>");
		result += TXT("Voicetag: ") + actor->GetVoiceTag().AsString() + TXT("<br>");
		result += TXT("Alive: ") + ToString( isAlive ) + TXT("<br>");
		result += TXT("IsHiddenInGame: ") + ToString( isHiddenInGame ) + TXT(" renders: ") + ToString( isVisible ) + TXT(" should render: ") + ToString( shouldBeVisible ) + TXT("<br>");
		result += TXT("Senses: ") + actor->GetSensesInfo() + TXT("<br>");
		result += TXT("Externaly controlled: ") + (actor->IsExternalyControlled() ? String(TXT("<font color=\"blue\">TRUE</font>")) : String(TXT("FALSE"))) + TXT("<br>");
		result += TXT("Is AI enabled: ") + ToString( actor->GetBehTreeMachine() && !actor->GetBehTreeMachine()->IsStopped() ) + TXT("<br>");
		result += TXT("Entity template: ") + m_html.MakeLinkResourceGoto(actor->GetEntityTemplate()) + TXT("<br>");
		result += TXT("Is NPC in Community: ") + (cs ? ToString( cs->IsNPCInCommunity(actor) ) : String(TXT("n/a")) ) + TXT("<br>");
		result += TXT("Attitude group: ") + actor->GetAttitudeGroup().AsString() + TXT("<br>");
		result += TXT("Entity tags: ") + actor->GetTags().ToString() + TXT("<br>");
		
		CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
		if ( ac && ac->GetBehaviorStack() )
		{
			result += TXT("Behavior: ") + ac->GetBehaviorStack()->GetInstancesAsString() + TXT("<br>");
		}

		// Behavior tree
		CBehTreeMachine* behTreeMachine = actor->GetBehTreeMachine();
		CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : nullptr;
		//if( behTreeMachine )
		//{
		//	result += behTreeMachine->GetInfo() + TXT("<br>");
		//}

		// Error state
#ifndef NO_ERROR_STATE
		if ( actor->IsInErrorState() )
		{
			result += TXT("<h3>Error state</h3>");
			result += TXT("<font color=\"red\"><b>") + actor->GetErrorState() + TXT("</font><br>");
		}
#endif

		// Moving agent component
		CMovingAgentComponent *movingAgentComp = actor->GetMovingAgentComponent();
		if ( movingAgentComp )
		{
			result += TXT("<h3>Moving agent component</h3>");
			result += TXT("Is enabled = ") + ToString( movingAgentComp->IsEnabled() ) + TXT("<br>");
			result += String::Printf( TXT("Flags   MotionDisabled: %04x   ForceEntityRepr: %04x   EnablePhysics: %04x   DisablePhysics: %04x<br>")
				, movingAgentComp->GetMotionDisableFlags(), movingAgentComp->GetEntityRepresentationForceFlags(), movingAgentComp->GetPhysicalRepresentationEnableFlags(), movingAgentComp->GetPhysicalRepresentationDisableFlags() );
			if ( movingAgentComp->GetPathAgent() )
			{
				result += String::Printf( TXT("Is snapped to navigation: %s, is on navigation: %s<br>")
					, movingAgentComp->IsSnapedToNavigableSpace() ? TXT("true") : TXT("false")
					, movingAgentComp->GetPathAgent()->IsOnNavdata() ? TXT("true") : TXT("false") );
			}
			
		}

		if( actor )
		{
			// NPC Schedule
			result += TXT("<h3>NPC schedule</h3>");
			GetTimetableDescription( actor, result );
			result += TXT("<br>");
		}

		// NPC find last AP status
		result += TXT("<h3>Find last AP status</h3>");
		result += actor->GetLastFindAPFriendlyResult() + TXT("<br>");

		// NPC Vector Status
		result += TXT("<h3>NPC state vector status</h3><br>");

		if( actor )
		{
			const NewNPCScheduleProxy& npcSchedule = actor->GetActionSchedule();
			if ( npcSchedule.GetActiveAP() != ActionPointBadID )
			{
				result += TXT("Active AP: ") + GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( npcSchedule.GetActiveAP() ) + TXT("<br>");
			}
			else
			{
				result += TXT("Active AP is EMPTY<br>");
			}
			result += TXT("Is using last AP: ") + String(npcSchedule.UsesLastAP() ? TXT("true") : TXT("false")) + TXT("<br>");
		}

		// Combat
		result += TXT("<h3>Combat Info</h3><br>");
		result += TXT("Is attackable by player: ") + ToString( actor->IsAttackableByPlayer() ) + TXT("<br>");
		result += TXT("Attitude to player: ") +  ToString(actor->GetAttitude( GCommonGame->GetPlayer() )) + TXT("<br>");
		result += String::Printf( TXT("Target friendly name: %s<br>"), actor->GetTarget() ? actor->GetTarget()->GetFriendlyName().AsChar() : TXT("NULL") );

		// Guard area debug
		if ( ai )
		{
			CBehTreeGuardAreaData* guardAreaData = CBehTreeGuardAreaData::Find( ai );
			if ( guardAreaData )
			{
				CAreaComponent* guardArea = guardAreaData->GetGuardArea();
				CAreaComponent* pursuitArea = guardAreaData->GetPursuitArea();
				Float pursuitRange = guardAreaData->GetPursuitRange();

				result +=
					String::Printf( TXT("Guard area: %s<br>") , guardArea ? guardArea->GetEntity()->GetFriendlyName().AsChar() : TXT("NULL") );
				result += pursuitArea
					? String::Printf( TXT("pursuit area: %s<br>"), pursuitArea->GetEntity()->GetFriendlyName().AsChar() )
					: String::Printf( TXT("pursuit range %0.2f<br>"), pursuitRange );
			}
			else
			{
				result += TXT("No guard area used.<br>");
			}
		}
		

		// Noticed
		if( actor )
		{
			result += TXT("<h3>Noticed objects</h3><br>");
			const TDynArray< NewNPCNoticedObject >& noticedObjects = actor->GetNoticedObjects();
			for ( TDynArray< NewNPCNoticedObject >::const_iterator noticedObj = noticedObjects.Begin();
				noticedObj != noticedObjects.End();
				++noticedObj )
			{
				result += noticedObj->ToString();
				result.Append( TXT("<br>"), 4 );
			}

			// Attitude
			result += TXT("<h3>Attitudes</h3><br>");
			TActorAttitudeMap attMap;
			actor->GetAttitudeMap( attMap );
			TActorAttitudeMap::const_iterator iter = attMap.Begin();
			String name, attitude;
			CEnum* attEnum = SRTTI::GetInstance().FindEnum( CNAME( EAIAttitude ) );
			ASSERT( attEnum );
			for( ; iter!= attMap.End(); ++iter )
			{
				if ( iter->m_first != NULL )
					name = iter->m_first->GetName();
				else
					name = TXT("NULL");

				attitude.Clear();
				Bool res = attEnum->ToString( &(iter->m_second), attitude );
				ASSERT( res );

				result += String::Printf( TXT("Actor: %s, attitude: %s<br>"), name.AsChar(), attitude.AsChar() );
			}
		}

		DescriptionInventory( result, actor->GetInventoryComponent() );

		// Moving agent component info
		result += TXT("<h3>Moving Agent Component</h3><br>");
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		if ( mac )
		{
			result += String::Printf( TXT("Moving Agent Component available<br>") );
		}
		else
		{
			result += String::Printf( TXT("Moving Agent Component NOT available<br>") );
		}

		// Encounters info
		if ( actor )
		{
			TDynArray< IActorTerminationListener* >& terminationListenrs = actor->GetTerminationListeners();

			CEncounter* encounter = nullptr;
			for( Uint32 i=0; i < terminationListenrs.Size(); ++i  )
			{
				encounter = terminationListenrs[ i ]->AsEncounter();
				if( encounter )
				{
					break;
				}
			}
			if( encounter )
			{
				result += TXT("<h3>Encounter info</h3>");
				
				result += TXT("Encounter layer: ") + encounter->GetLayer()->GetFriendlyName() + TXT("<br>");
			}
		}

	}
	return result;
}

void CMaraudersMapItemNPC::GetLookAtInfo( String& info ) const
{
	const CNewNPC* actor = m_npc.Get();
	if ( actor )
	{
		CActorLookAtDesc desc;
		actor->GetLookAtDesc( desc );

		info += String::Printf( TXT("<i>Mode: %s</i><br>"), desc.GetMode().AsChar() );

		while ( desc.NextIndex() )
		{
			ASSERT( desc.IsValidIndex() );

			if ( !desc.IsActiveLookAt() )
			{
				info += TXT("<i>");
			}

			info += String::Printf( TXT("Name: %s<br>Level: %s<br>Speed: %s<br>Range: %s<br>Auto limit deactivation: %s<br>Desc: %s<br><br>"),
				desc.GetName().AsChar(),
				desc.GetLevel().AsChar(),
				desc.GetSpeed().AsChar(),
				desc.GetRange().AsChar(),
				desc.IsAutoLimitDeact().AsChar(),
				desc.GetExtraDesc().AsChar() );

			if ( !desc.IsActiveLookAt() )
			{
				info += TXT("</i>");
			}
		}
	}
}

void CMaraudersMapItemNPC::OnAIEvent( EAIEventType type, EAIEventResult result, const String& name, const String& description, Float time )
{
	m_aiHistory.PushBack( SAIEvent( type, name, description, time ) );
	m_aiHistory.Back().m_result = result;
	m_aiHistory.Back().m_endTime = time;
}

void CMaraudersMapItemNPC::OnAIEventStart( EAIEventType type, const String& name, const String& description, Float startTime )
{
	m_aiHistory.PushBack( SAIEvent( type, name, description, startTime ) );
}

void CMaraudersMapItemNPC::OnAIEventEnd( Float endTime, EAIEventType type, EAIEventResult result )
{
	// find the event with the specified handle
	Bool found = false;
	for ( TDynArray< SAIEvent >::iterator it = m_aiHistory.Begin(); it != m_aiHistory.End(); ++it )
	{
		if ( it->IsInRange( endTime, type ) )
		{
			it->m_endTime = endTime;
			it->m_result = result;

			found = true;
			break;
		}
	}

	if ( !found )
	{
		m_aiHistory.PushBack( SAIEvent( EAIE_Unknown, TXT( "Unsymmetry" ), TXT( "Invalid event closed" ), endTime ) );
		m_aiHistory.Back().m_result = EAIR_Failure;
		m_aiHistory.Back().m_endTime = endTime;
	}
}

void CMaraudersMapItemNPC::DescriptionInventory( String& result, CInventoryComponent* inventory )
{
	// Attitude
	result += TXT("<h3>Inventory</h3><br>");
	if( inventory )
	{
		if( inventory->GetItemCount() == 0 )
		{
			result += TXT("Empty<br>");
		}
		else
		{
			for( Uint32 i=0; i<inventory->GetItemCount(); i++ )
			{
				const SInventoryItem* item = inventory->GetItem( (SItemUniqueId)i );
				if( item )
					result += String::Printf( TXT("%u. %s<br>"), i, item->GetInfo().AsChar() );
				else
					result += String::Printf( TXT("%u. NULL<br>"), i );
			};
		}
	}
	else
	{
		result += TXT("No inventory<br>");
	}
}

Bool CMaraudersMapItemNPC::Update( CMaraudersMapCanvasDrawing *canvas )
{
	// Pause AP
	CNewNPC* actor = m_npc.Get();
	if ( actor )
	{
		CNewNPC* npc = actor;

		// Pause AP change
		if ( m_isPauseApEnabled && npc )
		{
			const NewNPCScheduleProxy &npcSchedule = npc->GetActionSchedule();
			m_pauseCurrentApID = npcSchedule.GetActiveAP();
			if ( m_pauseLastApID != m_pauseCurrentApID )
			{
				m_isPauseApEnabled = false;
				m_log->AddLogInfo( String::Printf( TXT("Game paused: NPC %s has changed AP."), actor->GetFriendlyName().AsChar() ) );
				return true;
			}
		}

		// Pause state change
		if ( m_isPauseNpcStateEnabled )
		{
			m_pauseCurrentNpcState = actor->GetCurrentStateName();
			if ( m_pauseCurrentNpcState != m_pauseLastNpcState )
			{
				m_isPauseNpcStateEnabled = false;
				m_log->AddLogInfo( String::Printf( TXT("Game paused: NPC %s has changed state (%s -> %s)."),
					actor->GetFriendlyName().AsChar(), m_pauseLastNpcState.AsString().AsChar(), m_pauseCurrentNpcState.AsString().AsChar() ) );
				return true;
			}
		}

		// Pause schedule change
		if ( m_isPauseNpcScheduleEnabled && npc )
		{
			m_pauseCurrentNpcSchedule = String::EMPTY;
			GetTimetableDescription( npc, m_pauseCurrentNpcSchedule );
			if ( m_pauseLastNpcSchedule != m_pauseCurrentNpcSchedule )
			{
				m_isPauseNpcScheduleEnabled = false;
				m_log->AddLogInfo( String::Printf( TXT("Game paused: NPC %s has changed schedule."), actor->GetFriendlyName().AsChar() ) );
				return true;
			}
		}

		// Pause behavior change
		if ( m_isPauseBehaviorEnabled )
		{
			CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
			if ( ac && ac->GetBehaviorStack() )
			{
				m_pauseCurrentBehavior = ac->GetBehaviorStack()->GetInstancesAsString();
			}
			else
			{
				m_pauseCurrentBehavior = String::EMPTY;
			}
			if ( m_pauseLastBehavior != m_pauseCurrentBehavior )
			{
				m_isPauseBehaviorEnabled = false;
				m_log->AddLogInfo( String::Printf( TXT("Game paused: NPC %s has changed behavior (%s -> %s)."),
					actor->GetFriendlyName().AsChar(), m_pauseLastBehavior.AsChar(), m_pauseCurrentBehavior.AsChar() ) );
				return true;
			}
		}

		if ( !m_isPauseApEnabled && !m_isPauseNpcStateEnabled && !m_isPauseNpcScheduleEnabled && !m_isPauseBehaviorEnabled )
		{
			m_isFastUpdateRequired = false;
		}
	}

	return false;
}

Vector CMaraudersMapItemNPC::GetWorldPosition() const
{
	Vector vector(0,0,0);
	if ( m_npc.Get() )
	{
		vector = m_npc.Get()->GetWorldPosition();
	}

	return vector;
}

wxRect CMaraudersMapItemNPC::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( m_npc.Get() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );

		/*
		rect.x = point.x - 10;
		rect.y = point.y - 10;
		rect.width = 20;
		rect.height = 20;*/

		// for circle with scaled radius
		Int32 radius = canvas->WorldToCanvas( Vector( 1.0f, 0, 0 ) ).x;
		rect.x = point.x - radius;
		rect.y = point.y - radius;
		rect.width = radius * 2;
		rect.height = radius * 2;
	}
	return rect;
}

void CMaraudersMapItemNPC::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) const
{
	if ( m_debugAI )
	{
		CNewNPC* actor = m_npc.Get();
		if ( !actor )
		{
			return;
		}

		Uint32 line = 0;
		actor->GenerateAIDebugInfo( frame, line );
		CBehTreeMachine* behTreeMachine = actor->GetBehTreeMachine();
		CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : NULL;
		if ( ai )
		{
			IBehTreeNodeInstance* node = ai->GetInstanceRootNode();
			if ( node )
			{
				node->PropagateDebugFragmentsGeneration( frame );
			}

		}
	}
}

void CMaraudersMapItemNPC::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	if ( m_npc.Get() == NULL ) return;
	CNewNPC * actor = m_npc.Get();
	wxColour drawColor = wxColor( 255, 0, 0 );
	CMovingPhysicalAgentComponent * mpac = Cast<CMovingPhysicalAgentComponent>( actor->GetMovingAgentComponent() );
	if( mpac && mpac->GetPhysicalCharacter() &&  mpac->GetPhysicalCharacter()->GetCharacterController() )
	{
		const CMRPhysicalCharacter* mphys =  mpac->GetPhysicalCharacter();
		Float radius = mphys->GetCombatRadius();
		canvas->DrawCircleCanvas( mphys->GetRepresentationPosition(0) , radius , drawColor );

		if( mphys->GetNumOfRepresentations() == 2 )
		{
			canvas->DrawCircleCanvas( mphys->GetRepresentationPosition(1) , radius, drawColor );
			canvas->DrawLineCanvas( mphys->GetRepresentationPosition(0), mphys->GetRepresentationPosition(1), drawColor, 4.f  );
		}
	}
	else
	{
		canvas->DrawCircleCanvas( GetWorldPosition(), 0.7f, drawColor );
	}

#ifndef NO_EDITOR
	if( actor->m_debugAttackRange )
	{
		if( actor->m_debugAttackRange->IsA<CConeAttackRange>() )
		{
			const CConeAttackRange* ar = static_cast<const CConeAttackRange*>(actor->m_debugAttackRange);
			Float sweepAngle	= ar->m_rangeAngle;
			Float startAng		= 270 - actor->GetWorldYaw() - ar->m_angleOffset - ar->m_rangeAngle/2.f ;
			Vector pos			= actor->GetWorldPosition() + actor->GetLocalToWorld().TransformVector( ar->m_position );
			wxRect circle( canvas->WorldToCanvas( pos-ar->m_rangeMax ), canvas->WorldToCanvas( pos+ar->m_rangeMax ) );
			canvas->DrawPie( circle, startAng, sweepAngle, wxColour(255, 255, 0), 1.f );
		}
		else if( actor->m_debugAttackRange->IsA<CBoxAttackRange>() )
		{
			const CBoxAttackRange* ar = static_cast<const CBoxAttackRange*>( actor->m_debugAttackRange );
			const Matrix& localToWorld = actor->GetLocalToWorld();
			Matrix rangePlacement( Matrix::IDENTITY );
			rangePlacement.SetTranslation( ar->m_position );
			rangePlacement.SetRotZ33( DEG2RAD( ar->m_angleOffset ) );
			rangePlacement = Matrix::Mul( localToWorld, rangePlacement );

			wxPoint rect[4];
			Vector point = rangePlacement.TransformPoint( Vector( - ar->m_rangeWidth*0.5f, 0.f ,0.f ));
			rect[0] = canvas->WorldToCanvas(point);
			point = rangePlacement.TransformPoint( Vector( - ar->m_rangeWidth*0.5f, ar->m_rangeMax, 0.f ));
			rect[1] = canvas->WorldToCanvas(point);
			point = rangePlacement.TransformPoint( Vector( ar->m_rangeWidth*0.5f, ar->m_rangeMax, 0.f ));
			rect[2] = canvas->WorldToCanvas(point);
			point = rangePlacement.TransformPoint( Vector( ar->m_rangeWidth*0.5f, 0.f, 0.f ));
			rect[3] = canvas->WorldToCanvas(point);

			canvas->DrawPoly( rect, 4, wxColour(255, 255, 0) );
			}
	}
#endif
}

void CMaraudersMapItemNPC::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	// Draw NPC position
	static wxColor color( 0, 255, 0 );
	canvas->FillCircleCanvas( GetWorldPosition(), 0.8f, color );

	// Draw moving destination
	Vector destWorldPos = m_npc.Get()->GetPositionOrMoveDestination();
	wxPoint destPos = canvas->WorldToCanvas( destWorldPos );
	static wxColor colorDest( 0, 255, 255 );

	canvas->DrawCircleCanvas( destWorldPos, 0.7f, color );

	// Draw action point position
	if( m_npc.Get()->IsA<CNewNPC>() )
	{
		TActionPointID apID = static_cast<CNewNPC*>(m_npc.Get())->GetActionSchedule().GetActiveAP();
		if ( apID != ActionPointBadID )
		{
			Vector worldAPPos;
			GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetActionExecutionPosition( apID, &worldAPPos, NULL );
			canvas->DrawCrossCavnas( worldAPPos, 0.8f, color, 2.0f );

			GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetGoToPosition( apID, &worldAPPos, NULL );
			canvas->DrawCrossCavnas( worldAPPos, 0.8f, wxColor( 0, 0, 255 ), 2.0f );
		}
	}

	DrawNavigation( canvas );
	DrawAvailableActionPoints( canvas );
}

void CMaraudersMapItemNPC::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemNPC::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemNPC *a = dynamic_cast< const CMaraudersMapItemNPC* >( &item );
	if ( a )
	{
		return a->m_npc.Get() == m_npc.Get();
	}

	return false;
}

Bool CMaraudersMapItemNPC::IsValid() const
{
	return m_npc.Get() != NULL ? true : false;;
}

void CMaraudersMapItemNPC::SetDraggedPos( const Vector& worldPos )
{
	CNewNPC* npc = m_npc.Get();
	npc->Teleport( worldPos, npc->GetRotation() );
}

void CMaraudersMapItemNPC::DrawNavigation( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 255, 255, 255 );

	if ( !GGame || !GGame->IsActive() )
	{
		return;
	}

	if ( m_npc.Get() )
	{
		s_navigationCanvas.SetCanvas( canvas );
		m_npc.Get()->DebugDraw( s_navigationCanvas );
		s_navigationCanvas.SetCanvas( NULL );
	}
}

void CMaraudersMapItemNPC::DrawAvailableActionPoints( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor colorApFree( 255, 255, 255 );
	static wxColor colorApOccupied( 255, 255, 255 );

	for ( TDynArray< SActionPointDebugData >::const_iterator apI = m_availableAPs.Begin();
		  apI != m_availableAPs.End();
		  ++apI )
	{
		canvas->DrawCircleCanvas( apI->m_pos, 0.4f, apI->m_isFree ? colorApFree : colorApOccupied, 3.0f );
	}
}

Int32 CMaraudersMapItemNPC::CollectAvailableActionPoints()
{
	CNewNPC* npc = m_npc.Get()->IsA<CNewNPC>() ? static_cast<CNewNPC*>(m_npc.Get()) : NULL;

	if( npc )
	{
		const NewNPCScheduleProxy& schedule = npc->GetActionSchedule();
		CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry =
			CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >(
				schedule.GetTimetable(), GGame->GetTimeManager()->GetTime() );

		m_availableAPs.Clear();

		if ( apMan == NULL ) return 0;
		if ( timetabEntry == NULL ) return 0;

		for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
			  action != timetabEntry->m_actions.End();
			  ++action )
		{
			CLayerInfo *layerInfo = NULL;
			if ( layerInfo == NULL ) continue;
			CGUID apLayer = layerInfo->GetGUID();
			for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
				  actionCategory != action->m_actionCategories.End();
				  ++actionCategory )
			{
				SActionPointFilter apFilter;
				apFilter.m_actionPointTags = actionCategory->m_apTags;
				apFilter.m_category = actionCategory->m_name;
				apFilter.m_layerGuid = apLayer;

				TDynArray< TActionPointID > actionPoints;
				apMan->GetActionPoints( apFilter, actionPoints );

				for ( TDynArray< TActionPointID >::const_iterator apIdCI = actionPoints.Begin();
					apIdCI != actionPoints.End();
					++apIdCI )
				{
					Vector apPos;
					Bool isApFree = apMan->IsFree( *apIdCI );
					apMan->GetActionExecutionPosition( *apIdCI, &apPos, NULL );
					m_availableAPs.PushBack( SActionPointDebugData(apPos, isApFree) );
				}
			}
		}

		return m_availableAPs.Size();
	}

	return 0;
}

#ifdef EDITOR_AI_DEBUG
extern IBehTreeDebugInterface* GBehTreeDebugInterface;
#endif
extern void StartBehaviorGraphDebug( CEntity* entity );
extern void StartSteeringTreeDebug( CActor* actor );

Bool CMaraudersMapItemNPC::ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ )
{
	CNewNPC* actor = m_npc.Get();
	if ( actor )
	{
		CNewNPC* npc = actor;
		if ( s_optionsNames[ optionNum ] == OPTION_BEHEAVIORDEBUG )
		{
			StartBehaviorGraphDebug( actor );
			return true;
		}
#ifdef EDITOR_AI_DEBUG
		else if ( s_optionsNames[ optionNum ] == OPTION_BEHTREEDEBUG )
		{
			GBehTreeDebugInterface->DebugBehTreeStart( actor->GetBehTreeMachine() );
			m_debugAI = true;
			return true;
		}
#endif	//EDITOR_AI_DEBUG
		else if ( s_optionsNames[ optionNum ] ==  OPTION_STEERINGDEBUG )
		{
			StartSteeringTreeDebug( actor );
		}
		else if ( s_optionsNames[ optionNum ] ==  OPTION_REACTIONSDEBUG && npc )
		{
			StartReactionsDebugger( npc );
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_DESTROY && npc )
		{

			npc->EnterForcedDespawn();
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_KILL )
		{
			actor->Kill( true );
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_STUN )
		{
			actor->Stun( true );
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_LIST_APS )
		{
			Int32 availApFoundNum = CollectAvailableActionPoints();
			logOutput.PushBack( String::Printf( TXT("%d action points found"), availApFoundNum ) );
			//m_showAvailableActionPoints = !m_showAvailableActionPoints;
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_PAUSE_GAME_ON_AP_CHANGE )
		{
			m_pauseLastApID = m_pauseCurrentApID;
			m_isPauseApEnabled = true;
			m_isFastUpdateRequired = true;
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_PAUSE_GAME_ON_NPC_STATE_CHANGE )
		{
			m_pauseLastNpcState = actor->GetCurrentStateName();
			m_isPauseNpcStateEnabled = true;
			m_isFastUpdateRequired = true;
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_PAUSE_GAME_ON_NPC_SCHEDULE_CHANGE && npc )
		{
			m_pauseLastNpcSchedule = String::EMPTY;
			GetTimetableDescription( npc, m_pauseLastNpcSchedule );
			m_isPauseNpcScheduleEnabled = true;
			m_isFastUpdateRequired = true;
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_PAUSE_GAME_ON_BEHAVIOR_CHANGE )
		{
			CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
			if ( ac && ac->GetBehaviorStack() )
			{
				m_pauseLastBehavior = ac->GetBehaviorStack()->GetInstancesAsString();
			}
			else
			{
				m_pauseLastBehavior = String::EMPTY;
			}
			m_isPauseBehaviorEnabled = true;
			m_isFastUpdateRequired = true;
			return true;
		}
		else if ( s_optionsNames[ optionNum ] ==  OPTION_SKELETON )
		{
			CAnimatedComponent* root = actor->GetRootAnimatedComponent();
			if ( root )
			{
				root->SetDispSkeleton( ACDD_SkeletonBone, !root->IsDispSkeleton( ACDD_SkeletonBone ) );
			}
		}
		else if ( s_optionsNames[ optionNum ] ==  OPTION_SET_ATTITUDE_HOSTILE && npc )
		{
			npc->SetAttitude( GCommonGame->GetPlayer(), AIA_Hostile );
		}
	}
	return false;
}

void CMaraudersMapItemNPC::GetTimetableDescription( const CNewNPC * npc, String &result ) const
{
	const NewNPCScheduleProxy& sched = npc->GetActionSchedule();
	const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >& timetable = sched.GetTimetable();

	const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry =
		CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( timetable, GGame->GetTimeManager()->GetTime() );

	for ( TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >::const_iterator timetableI = timetable.Begin(); timetableI != timetable.End(); ++timetableI )
	{
		String ttEntryText( TXT("<b>") + timetableI->m_time.ToString() + TXT("</b>") );

		if ( &(*timetableI) == timetabEntry )
		{
			ttEntryText += TXT(" ACTIVE<br>");
		}
		else
		{
			ttEntryText += TXT("<br>");
		}

		for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetableI->m_actions.Begin();
			action != timetableI->m_actions.End();
			++action )
		{
			ttEntryText += TXT("Layer: ") + action->m_layerName.m_layerName.AsString() + TXT("<br>");
			for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
				actionCategory != action->m_actionCategories.End();
				++actionCategory )
			{
				ttEntryText += actionCategory->m_name.AsString() + String::Printf( TXT(" : %f ;"), actionCategory->m_weight );
			}
			ttEntryText += TXT("<br>");
		}

		result += ttEntryText;
	}
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemPlayer::CMaraudersMapItemPlayer()
	: m_debugAI( false )
{
	m_optionsNames.PushBack( CMaraudersMapItemNPC::OPTION_BEHEAVIORDEBUG );
	m_optionsNames.PushBack( CMaraudersMapItemNPC::OPTION_BEHTREEDEBUG );
	m_optionsNames.PushBack( CMaraudersMapItemNPC::OPTION_STEERINGDEBUG );
	m_optionsNames.PushBack( CMaraudersMapItemNPC::OPTION_SKELETON );
}

CPlayer* CMaraudersMapItemPlayer::GetPlayer() const
{
	return GCommonGame->GetPlayer();
}

Bool CMaraudersMapItemPlayer::ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ )
{
	if ( GetPlayer() )
	{
		if( m_optionsNames[ optionNum ] == CMaraudersMapItemNPC::OPTION_BEHEAVIORDEBUG )
		{
			StartBehaviorGraphDebug( GetPlayer() );
			return true;
		}
		else if ( m_optionsNames[ optionNum ] == CMaraudersMapItemNPC::OPTION_BEHTREEDEBUG )
		{
			GBehTreeDebugInterface->DebugBehTreeStart( GetPlayer()->GetBehTreeMachine() );
			m_debugAI = true;
			return true;
		}
		else if( m_optionsNames[ optionNum ] == CMaraudersMapItemNPC::OPTION_STEERINGDEBUG )
		{
			StartSteeringTreeDebug( GetPlayer() );
			return true;
		}
		else if( m_optionsNames[ optionNum ] == CMaraudersMapItemNPC::OPTION_SKELETON )
		{
			CAnimatedComponent* root = GetPlayer()->GetRootAnimatedComponent();
			if ( root )
			{
				root->SetDispSkeleton( ACDD_SkeletonBone, !root->IsDispSkeleton( ACDD_SkeletonBone ) );
			}
		}
	}
	return false;
}

String CMaraudersMapItemPlayer::GetShortDescription() const
{
	return TXT("Player");
}

String CMaraudersMapItemPlayer::GetFullDescription() const
{
	CPlayer* player = GetPlayer();
	if( !player )
	{
		return String::EMPTY;
	}

	String result = TXT("<b>Player</b><br>");

	Bool isInCommunityArea = GCommonGame->GetSystem< CCommunitySystem >()->IsVisibilityAreaEnabled();
	CAreaComponent* visibilityArea = GCommonGame->GetSystem< CCommunitySystem >()->GetVisibilityArea();
	const CFunction* topFunction = player->GetTopLevelFunction();

	// get hp (vitality)
	Float statHPMax = 0, statHP = 0;
	{
		CallFunctionRet< Float >( player, CNAME( GetStatMax ), 0 /*BCS_Vitality*/, statHPMax );
		CallFunctionRet< Float >( player, CNAME( GetStat ), 0 /*BCS_Vitality*/, statHP );
	}

	// immortality
	Bool invulnerable = false, immortal = false;
	{
		CallFunctionRet< Bool >( player, CNAME( IsImmortal ), immortal );
		CallFunctionRet< Bool >( player, CNAME( IsInvulnerable ), invulnerable );
	}

	// is alive - from scripts because is overridden
	Bool isAlive = false;
	{
		CallFunctionRet< Bool >( player, CNAME( IsAlive ), isAlive );
	}

	result += TXT("Display name: ") + player->GetDisplayName() + TXT("<br>");
	result += TXT("Friendly name: ") + player->GetFriendlyName() + TXT("<br>");
	result += TXT("Template friendly name: ") + player->GetTemplate()->GetFriendlyName() + TXT("<br>");
	result += TXT("State: <b>") + player->GetCurrentStateName().AsString() + TXT("</b><br>");
	result += TXT( "HP: " ) + ToString( statHP ) + TXT( "/" ) + ToString( statHPMax ) + TXT("<br>");
	result += TXT( "Invulnerable: " ) + ToString( invulnerable ) + TXT("<br>");
	result += TXT( "Immortal: " ) + ToString( immortal ) + TXT("<br>");
	result += String( TXT("Script function: ") ) + (topFunction ? topFunction->GetName().AsString() : TXT("None")) + TXT("<br>");
	result += TXT("Voicetag: ") + player->GetVoiceTag().AsString() + TXT("<br>");
	result += TXT("Alive: ") + ToString( isAlive ) + TXT("<br>");
	result += TXT("Is in community area: ") + ToString( isInCommunityArea ) + TXT("<br>");
	if ( visibilityArea )
	{
		result += TXT("Visibility area friendly name: ") + visibilityArea->GetFriendlyName() + TXT("<br>");
	}
	result += TXT("Player world pos: ") + ToString( player->GetWorldPosition() ) + TXT("<br>");

	// spawn radiuses
	{
		Float spawnRadius = 0, despawnRadius = 0, areaSpawnRadius = 0, areaDespawnRadius = 0;
		GCommonGame->GetSystem< CCommunitySystem >()->GetVisibilityRadius( spawnRadius, despawnRadius );
		result += String::Printf( TXT("Spawn radius: %f<br>Despawn radius: %f<br>"), spawnRadius, despawnRadius );

		if ( isInCommunityArea )
		{
			GCommonGame->GetSystem< CCommunitySystem >()->GetVisibilityAreaRadius( areaSpawnRadius, areaDespawnRadius );
			result += String::Printf( TXT("Visibility area spawn radius: %f<br>Visibility area despawn radius: %f<br>"), areaSpawnRadius, areaDespawnRadius );
		}
	}

	CMaraudersMapItemNPC::DescriptionInventory( result, player->GetInventoryComponent() );

	return result;
}

Vector CMaraudersMapItemPlayer::GetWorldPosition() const
{
	if( GCommonGame->GetPlayer() )
		return GCommonGame->GetPlayer()->GetWorldPosition();
	else
		return Vector::ZEROS;
}

wxRect CMaraudersMapItemPlayer::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( GCommonGame->GetPlayer() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );

		// for circle with scaled radius
		Int32 radius = canvas->WorldToCanvas( Vector( 1.0f, 0, 0 ) ).x;
		rect.x = point.x - radius;
		rect.y = point.y - radius;
		rect.width = radius * 2;
		rect.height = radius * 2;
	}
	return rect;
}

void CMaraudersMapItemPlayer::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	DrawPlayer( canvas, false );
}

void CMaraudersMapItemPlayer::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	DrawPlayer( canvas, true );
}

void CMaraudersMapItemPlayer::DrawPlayer( CMaraudersMapCanvasDrawing *canvas, Bool selected )
{
	if ( const CPlayer *player = GCommonGame->GetPlayer() )
	{
		Vector playerPos = player->GetWorldPosition();
		static wxColor playerColor( 200, 0, 0 );
		static wxColor cameraColor( 255, 0, 0 );
		const Float size = 1.0f;

		Vector vertices[3] =
		{
			Vector( -size, -size, 0 ),	// left
			Vector( size, -size, 0 ),	// right
			Vector( 0, size*2.0f, 0 ),	// up
		};

		vertices[0] = player->GetWorldRotation().TransformVector( vertices[0] );
		vertices[1] = player->GetWorldRotation().TransformVector( vertices[1] );
		vertices[2] = player->GetWorldRotation().TransformVector( vertices[2] );

		vertices[0] += playerPos;
		vertices[1] += playerPos;
		vertices[2] += playerPos;

		if( selected )
		{
			canvas->FillPolyCanvas( vertices, 3, playerColor );
		}
		else
		{
			canvas->DrawPolyCanvas( vertices, 3, playerColor, 2.0 );
		}

		const Vector cameraDir = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();
		canvas->DrawLineCanvas( playerPos, playerPos + cameraDir*3.0f, cameraColor, 2.5f );

		// Draw community spawn/despawn radius
		static wxColor spawnRadiusColor( 150, 150, 235 );
		static wxColor despawnRadiusColor( 150, 15, 235 );
		static wxColor areaDespawnRadiusColor( 0, 0, 200 );
		static wxColor areaSpawnRadiusColor( 0, 0, 100 );

		CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( cs )
		{
			// PAKSAS TODO: debug strategii spawn'owania
			/*
			const Vector visibilityCenter = cs->GetVisibilityRadiusCenter();
			canvas->DrawCircleCanvas( visibilityCenter, CCommunityConstants::VISIBILITY_SPAWN_RADIUS, spawnRadiusColor );
			canvas->DrawCircleCanvas( visibilityCenter, CCommunityConstants::VISIBILITY_DESPAWN_RADIUS, despawnRadiusColor );

			if ( GCommonGame->GetSystem< CCommunitySystem >()->IsVisibilityAreaEnabled() )
			{
				canvas->DrawCircleCanvas( visibilityCenter, CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS, areaDespawnRadiusColor );
				canvas->DrawCircleCanvas( visibilityCenter, CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS, areaSpawnRadiusColor );
			}*/
		}
	}
}

void CMaraudersMapItemPlayer::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemPlayer::operator==( const CMaraudersMapItemBase& item  ) const
{
	const CMaraudersMapItemPlayer *a = dynamic_cast< const CMaraudersMapItemPlayer* >( &item );
	if ( a )
	{
		return true;
	}

	return false;
}
Bool CMaraudersMapItemPlayer::IsValid() const
{
	return ( GCommonGame->GetPlayer() != NULL );
}


void CMaraudersMapItemPlayer::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) const
{
	if ( m_debugAI )
	{
		CActor* actor = GetPlayer();
		if ( !actor )
		{
			return;
		}

		Uint32 line = 0;
		actor->GenerateAIDebugInfo( frame, line );
		CBehTreeMachine* behTreeMachine = actor->GetBehTreeMachine();
		CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : NULL;
		if ( ai )
		{
			IBehTreeNodeInstance* node = ai->GetInstanceRootNode();
			if ( node )
			{
				node->PropagateDebugFragmentsGeneration( frame );
			}

		}
	}
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemMesh::CMaraudersMapItemMesh( const CEntity *entity )
	: m_entity( entity )
{
	m_shortDescription = TXT("Mesh: ") + entity->GetFriendlyName();

	for ( ComponentIterator< CMeshTypeComponent > meshComponent( entity ); meshComponent; ++meshComponent )
	{
		const Box &box = (*meshComponent)->GetBoundingBox();
		if ( box.CalcSize().SquareMag2() < 4.0f * 4.0f )
		{
			m_bboxes.PushBack( box );
		}
	}

	// GenerateFrame();
}

CMaraudersMapItemMesh::~CMaraudersMapItemMesh()
{
}

String CMaraudersMapItemMesh::GetShortDescription() const
{
	return m_shortDescription;
}

String CMaraudersMapItemMesh::GetFullDescription() const
{
	String txt(TXT("Description"));
	return txt;
}

Vector CMaraudersMapItemMesh::GetWorldPosition() const
{
	Vector vector( 0, 0, 0 );
	if ( m_entity.Get() )
	{
		vector = m_entity.Get()->GetWorldPosition();
	}

	return vector;
}

void CMaraudersMapItemMesh::BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const
{
	const Float &xBeg = box.Min.X;
	const Float &yBeg = box.Min.Y;
	const Float &xEnd = box.Max.X;
	const Float &yEnd = box.Max.Y;
	Vector vBeg( xBeg, yBeg, 0 );
	Vector vEnd( xEnd, yEnd, 0 );
	pointBeg = canvas->WorldToClient( vBeg );
	pointEnd = canvas->WorldToClient( vEnd );
}

wxRect CMaraudersMapItemMesh::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	m_border = wxRect( 0, 0, 0, 0 );

	wxPoint pointBeg, pointEnd;
	if ( m_bboxes.Size() > 0 )
	{
		BoxToPointsClient( canvas, m_bboxes[ 0 ], pointBeg, pointEnd );

		m_border.SetLeft( pointBeg.x );
		m_border.SetRight( pointEnd.x );
		m_border.SetTop( pointEnd.y );
		m_border.SetBottom( pointBeg.y );
	}

	for ( Uint32 i = 1; i < m_bboxes.Size(); ++i )
	{
		BoxToPointsClient( canvas, m_bboxes[ i ], pointBeg, pointEnd );

		if ( pointBeg.x < m_border.GetLeft()   ) m_border.SetLeft  ( pointBeg.x );
		if ( pointEnd.x > m_border.GetRight()  ) m_border.SetRight ( pointEnd.x );
		if ( pointEnd.y < m_border.GetTop()    ) m_border.SetTop   ( pointEnd.y );
		if ( pointBeg.y > m_border.GetBottom() ) m_border.SetBottom( pointBeg.y );
	}

	return m_border;
}

void CMaraudersMapItemMesh::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 255, 255 );
	static wxColor color1( 255, 255, 0 );

	if ( m_entity.Get() == NULL ) return;

	for ( TDynArray< Box >::const_iterator bboxI = m_bboxes.Begin();
		  bboxI != m_bboxes.End();
		  ++bboxI )
	{
		wxPoint pointBeg = canvas->WorldToCanvas( Vector( bboxI->Min.X, bboxI->Min.Y, 0 ) );
		wxPoint pointEnd = canvas->WorldToCanvas( Vector( bboxI->Max.X, bboxI->Max.Y, 0 ) );

		wxRect rect( pointBeg, pointEnd );

		canvas->DrawRect( rect, color );
	}

	// Do not remove below code
	/*const Uint32 size = m_framePoints.Size();
	for ( Uint32 i = 0; i < size; ++i )
	{
		const Vector &a = m_framePoints[ i ];
		const Vector &b = m_framePoints[ (i+1) % size ];

		canvas->DrawLineCanvas( a, b, color );
	}*/
}

void CMaraudersMapItemMesh::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 255, 255 );

	if ( m_entity.Get() == NULL ) return;

	for ( TDynArray< Box >::const_iterator bboxI = m_bboxes.Begin();
		bboxI != m_bboxes.End();
		++bboxI )
	{
		wxPoint pointBeg = canvas->WorldToCanvas( Vector( bboxI->Min.X, bboxI->Min.Y, 0 ) );
		wxPoint pointEnd = canvas->WorldToCanvas( Vector( bboxI->Max.X, bboxI->Max.Y, 0 ) );

		wxRect rect( pointBeg, pointEnd );

		canvas->FillRect( rect, color );
	}
}

void CMaraudersMapItemMesh::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemMesh::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemMesh *a = dynamic_cast< const CMaraudersMapItemMesh* >( &item );
	if ( a )
	{
		return a->m_entity.Get() == m_entity.Get();
	}

	return false;
}

Bool CMaraudersMapItemMesh::IsValid() const
{
	return m_entity.Get() != NULL ? true : false;
}

Bool CMaraudersMapItemMesh::Update( CMaraudersMapCanvasDrawing *canvas )
{
	return false;
}

void CMaraudersMapItemMesh::GenerateFrame()
{
	// Generate raw points
	if ( !m_framePoints.Empty() ) return;

	Int32 maxLeftIndex = 0;

	Int32 i = 0;
	for ( ComponentIterator< CMeshTypeComponent > meshComponent( m_entity.Get() ); meshComponent; ++meshComponent, ++i )
	{
		const Box &box = (*meshComponent)->GetBoundingBox();

		ASSERT( box.Min.X <= box.Max.X );
		ASSERT( box.Min.Y <= box.Max.Y );

		SPoints p;
		p[0] = Vector( box.Min.X, box.Max.Y, 0 );
		p[1] = Vector( box.Min.X, box.Min.Y, 0 );
		p[2] = Vector( box.Max.X, box.Min.Y, 0 );
		p[3] = Vector( box.Max.X, box.Max.Y, 0 );
		m_bboxesPoints.PushBack( p );

		// find max left rectangle
		if ( p[0].X < m_bboxesPoints[maxLeftIndex][0].X )
		{
			maxLeftIndex = i;
		}
	}

	if ( m_bboxesPoints.Empty() ) return;

	Int32 currRectI = maxLeftIndex;
	Int32 currEdgeI = 0; // 0..3
	Vector firstPoint = m_bboxesPoints[ currRectI ][ 0 ];
	do
	{
		Vector &a = m_bboxesPoints[ currRectI ][ currEdgeI ];
		Vector &b = m_bboxesPoints[ currRectI ][ (currEdgeI+1) % 4 ];
		m_framePoints.PushBack( a );
		Vector crossPoint;
		if ( IsCross( a, b, crossPoint, currRectI, currEdgeI ) )
		{
			m_framePoints.PushBack( crossPoint );
		}
		else
		{
			currEdgeI = (currEdgeI+1) % 4;
		}
	} while ( firstPoint != m_bboxesPoints[ currRectI ][ currEdgeI ] );

	m_bboxesPoints.Clear();
}

Bool CMaraudersMapItemMesh::IsCross( const Vector &a, const Vector &b, Vector &crossPoint /* out */, Int32 &currRectI /* in out */, Int32 &currEdgeI /* out */ )
{
	// Generate cross points
	TDynArray< Vector > crossPoints;
	TDynArray< Int32 > crossRectIndexes;
	TDynArray< Int32 > crossEdgeIndexes;
	for ( Uint32 j = 0; j < m_bboxesPoints.Size(); ++j )
	{
		if ( j == currRectI ) continue; // don't compare with owner

		SPoints &points1 = m_bboxesPoints[ j ];
		for ( Uint32 l=0; l < points1.Size(); ++l )
		{
			Vector& a1 = points1[l];
			Vector& b1 = points1[(l+1) % points1.Size()];

			if ( DoSectionsIntersect( a, b, a1, b1 ) )
			{
				Vector nearestPoint = a1.NearestPointOnEdge( a, b );
				crossPoints.PushBack( nearestPoint );
				crossRectIndexes.PushBack( j );
				crossEdgeIndexes.PushBack( (l+1) % points1.Size() );
			}
		}
	}

	if ( crossPoints.Empty() )
	{
		return false;
	}
	else
	{
		// Find cross point closest to the 'a' point
		Float shortestDist = a.DistanceSquaredTo( crossPoints[0] );
		Int32 shortestIndex = 0;
		for ( Uint32 i = 1; i < crossPoints.Size(); ++i )
		{
			Float dist = a.DistanceSquaredTo( crossPoints[i] );
			if ( dist < shortestDist )
			{
				shortestDist = dist;
				shortestIndex = i;
			}
			else if ( dist == shortestDist )
			{
				// choose the most extended line
				/*if ( crossEdgeIndexes[ i ] == 0 )
				{
					if ( m_bboxesPoints[ crossRectIndexes[ i ] ][ 0 ].X < m_bboxesPoints[ crossRectIndexes[ shortestIndex ] ][ 0 ].X )
					{
						shortestDist = dist;
						shortestIndex = i;
					}
				}
				else if ( crossEdgeIndexes[ i ] == 1 )
				{
					if ( m_bboxesPoints[ crossRectIndexes[ i ] ][ 1 ].Y < m_bboxesPoints[ crossRectIndexes[ shortestIndex ] ][ 1 ].Y )
					{
						shortestDist = dist;
						shortestIndex = i;
					}
				}
				else if ( crossEdgeIndexes[ i ] == 2 )
				{
					if ( m_bboxesPoints[ crossRectIndexes[ i ] ][ 2 ].X > m_bboxesPoints[ crossRectIndexes[ shortestIndex ] ][ 2 ].X )
					{
						shortestDist = dist;
						shortestIndex = i;
					}
				}
				else if ( crossEdgeIndexes[ i ] == 3 )
				{
					if ( m_bboxesPoints[ crossRectIndexes[ i ] ][ 3 ].Y > m_bboxesPoints[ crossRectIndexes[ shortestIndex ] ][ 3 ].Y )
					{
						shortestDist = dist;
						shortestIndex = i;
					}
				}*/
			}
		}

		// return value - shortest to the 'a' cross point on ('a','b') edge
		crossPoint = crossPoints[ shortestIndex ];
		currRectI = crossRectIndexes[ shortestIndex ];
		currEdgeI = crossEdgeIndexes[ shortestIndex ];
		return true;
	}
}

Bool CMaraudersMapItemMesh::DoSectionsIntersect( const Vector &aBeg, const Vector &aEnd, const Vector &bBeg, const Vector &bEnd )
{
	Bool isAVertical;
	Bool isBVertical;

	if ( aBeg.X == aEnd.X ) isAVertical = true;
	else if ( aBeg.Y == aEnd.Y ) isAVertical = false;
	else
	{
		ASSERT( !TXT("Not vertical or horizontal point") );
	}

	if ( bBeg.X == bEnd.X ) isBVertical = true;
	else if ( bBeg.Y == bEnd.Y ) isBVertical = false;
	else
	{
		ASSERT( !TXT("Not vertical or horizontal point") );
	}

	if ( aEnd == bBeg && isAVertical == isBVertical )
	{
		return true;
	}

	if ( isAVertical == isBVertical )
	{
		return false;
	}

	if ( !isAVertical )
	{
		Bool xAxis;
		if ( aBeg.X <= aEnd.X ) xAxis = ( aBeg.X < bBeg.X && bBeg.X < aEnd.X );
		else xAxis = ( aEnd.X < bBeg.X && bBeg.X < aBeg.X );

		Bool yAxis;
		if ( bBeg.Y <= bEnd.Y ) yAxis = ( bBeg.Y < aBeg.Y && aBeg.Y < bEnd.Y );
		else yAxis = ( bEnd.Y < aBeg.Y && aBeg.Y < bBeg.Y );

		if ( xAxis && yAxis ) return true;
	}
	else
	{
		ASSERT( !isBVertical );

		Bool xAxis;
		if ( bBeg.X <= bEnd.X ) xAxis = ( bBeg.X < aBeg.X && aBeg.X < bEnd.X );
		else xAxis = ( bEnd.X < aBeg.X && aBeg.X < bBeg.X);

		Bool yAxis;
		if ( aBeg.Y <= aEnd.Y ) yAxis = ( aBeg.Y < bBeg.Y && bBeg.Y < aEnd.Y );
		else yAxis = ( aEnd.Y < bBeg.Y && bBeg.Y < aBeg.Y );

		if ( xAxis && yAxis ) return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemActionPoint::CMaraudersMapItemActionPoint( const CEntity *entity )
	: m_entity( entity )
{
	CollectEntityComponents( (CEntity*)entity, m_apComponents );
}

String CMaraudersMapItemActionPoint::GetShortDescription() const
{
	String txt(TXT("Description"));
	if ( m_entity.Get() )
	{
		txt = TXT("AP: ") + m_entity.Get()->GetFriendlyName();
	}
	return txt;
}

String CMaraudersMapItemActionPoint::GetFullDescription() const
{
	String result;

	if ( m_entity.Get() )
	{
		CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		if ( apMan == NULL ) return TXT("No ap manager available");

		result.Clear();
		result = TXT("<h2>Action Point</h2><br>");

		CEntityTemplate* apEntTemplate = Cast< CEntityTemplate >( m_entity.Get() ? m_entity.Get()->GetTemplate() : NULL );
		if ( apEntTemplate )
		{
			result += TXT("Action Point resource: ") + m_html.MakeLinkResourceGoto( apEntTemplate ) + TXT("<br>");
		}

		for ( TDynArray< CActionPointComponent* >::const_iterator actionPointI = m_apComponents.Begin();
			  actionPointI != m_apComponents.End();
			  ++actionPointI )
		{
			CActionPointComponent *actionPoint = *actionPointI;
			TActionPointID apID = actionPoint->GetID();

			result += TXT("Action Point name: ") + actionPoint->GetName() + TXT("<br>");
			result += TXT("Action Point friendly name: ") + actionPoint->GetFriendlyName() + TXT("<br>");

			result += TXT("AP Categories: ");
			for ( TDynArray< TAPCategory >::const_iterator apCategory = actionPoint->GetActionCategories().Begin();
				apCategory != actionPoint->GetActionCategories().End(); )
			{
				result += apCategory->AsString();
				if ( ++apCategory != actionPoint->GetActionCategories().End() )
				{
					result += TXT(", ");
				}
			}
			result += TXT("<br>");

			if ( apMan->IsFree( apID ) )
			{
				result += TXT("Action point is <font color=\"green\"><b>free</b></font>.<br>");
			}
			else
			{
				result += TXT("Action point is <font color=\"green\"><b>occupied</b></font>.<br>");
			}

			if ( actionPoint->HasPreferredNextAPs() )
			{
				result += TXT("Preferred next APs: ") + GetStringFromArray( actionPoint->GetPreferredNextAPsTagList().GetTags() );
				TActionPointID nextPrefApID = apMan->FindNextFreeActionPointInSequence( apID );
				if ( nextPrefApID != ActionPointBadID )
				{
					result += TXT(" - ") + m_html.MakeLinkMaraudersItemGotoActionPoint( TXT("link"), nextPrefApID );
				}
				result += TXT("<br>");
			}
			else
			{
				result += TXT("No preferred next APs.<br>");
			}

			result += TXT("AP Entity Tags: ") + actionPoint->GetEntity()->GetTags().ToString() + TXT("<br>");
			result += TXT("AP Component Tags: ") + actionPoint->GetTags().ToString() + TXT("<br>");

			result += TXT("<h2>Programmers info</h2>");
			result += TXT("AP ID: ") + apID.ToString() + TXT("<br>");
			result += TXT("AP component layer GUID: ") + ToString( actionPoint->GetLayer()->GetGUID() ) + TXT("<br>");
			Uint32 problemReportsCount = 0;
			Bool flagInvalidWpPos = false, flagMissingWp = false;
			apMan->GetDebugInfo( apID, problemReportsCount, flagInvalidWpPos, flagMissingWp );
			String problemReportsCountFont = problemReportsCount ? TXT("<font color=\"red\">") : TXT("<font color=\"black\">");
			result += TXT("AP problem reports counter: ") + problemReportsCountFont + ToString( problemReportsCount ) + TXT("</font><br>");
			String flagInvalidWpPosFont = flagInvalidWpPos ? TXT("<font color=\"red\">") : TXT("<font color=\"black\">");
			result += TXT("AP invalid WP pos flag: ") + flagInvalidWpPosFont + ToString( flagInvalidWpPos ) + TXT("</font><br>");
			String flagMissingWpFont = flagMissingWp ? TXT("<font color=\"red\">") : TXT("<font color=\"black\">");
			result += TXT("AP missing WP flag: ") + flagMissingWpFont + ToString( flagMissingWp ) + TXT("</font><br>");
		}
	}

	return result;
}

Vector CMaraudersMapItemActionPoint::GetWorldPosition() const
{
	Vector vector(0,0,0);
	if ( m_entity.Get() )
	{
		vector = m_entity.Get()->GetWorldPosition();
	}

	return vector;
}

wxRect CMaraudersMapItemActionPoint::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( m_entity.Get() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );

		// for circle with scaled radius
		Int32 radius = canvas->WorldToCanvas( Vector( 1.0f, 0, 0 ) ).x;
		rect.x = point.x - radius;
		rect.y = point.y - radius;
		rect.width = radius * 2;
		rect.height = radius * 2;

		// for circle with const radius
		/*rect.x = point.x - 10;
		rect.y = point.y - 10;
		rect.width = 20;
		rect.height = 20;*/
	}
	return rect;
}

void CMaraudersMapItemActionPoint::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	if ( m_entity.Get() == NULL ) return;

	static wxColor color( 255, 0, 255 );
	static wxColor componentColor( 155, 0, 155 );

	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( apMan == NULL ) return;

	//canvas->DrawCircleConstRadiusCanvas( GetWorldPosition(), 10, color, 0.5f );
	canvas->DrawCircleCanvas( GetWorldPosition(), 0.8f, color, 0.5f );

	for ( TDynArray< CActionPointComponent* >::const_iterator actionPointI = m_apComponents.Begin();
		  actionPointI != m_apComponents.End();
		  ++actionPointI )
	{
		TActionPointID apID = (*actionPointI)->GetID();
		Uint32 problemReportsCount = 0;
		Bool flagInvalidWpPos = false, flagMissingWp = false;
		apMan->GetDebugInfo( apID, problemReportsCount, flagInvalidWpPos, flagMissingWp );
		wxColor apColor = componentColor;

		if ( problemReportsCount > 0 || flagInvalidWpPos || flagMissingWp )
		{
			apColor = wxColor( 255, 0, 0 );
		}

		const Bool isApFree = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager() ?
			GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->IsFree( (*actionPointI)->GetID() ) : true;
		const Float width =  isApFree ? 1.0f : 3.0f;
		//canvas->DrawCircleConstRadiusCanvas( (*actionPointI)->GetWorldPosition(), 13, componentColor, width );
		canvas->DrawCircleCanvas( (*actionPointI)->GetWorldPosition(), 1.0f, apColor, width );
	}
}

void CMaraudersMapItemActionPoint::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	//wxPoint pos = canvas->WorldToCanvas( GetWorldPosition() );
	static wxColor color( 255, 0, 255 );
	static wxColor componentColor( 155, 0, 155 );

	canvas->FillCircleCanvas( GetWorldPosition(), 0.8f, color );
	//canvas->FillCircleConstRadiusCanvas( GetWorldPosition(), 10, color );

	for ( TDynArray< CActionPointComponent* >::const_iterator actionPointI = m_apComponents.Begin();
		actionPointI != m_apComponents.End();
		++actionPointI )
	{
		canvas->FillCircleCanvas( GetWorldPosition(), 1.0f, componentColor );
		//canvas->FillCircleConstRadiusCanvas( (*actionPointI)->GetWorldPosition(), 13, componentColor );
	}
}

void CMaraudersMapItemActionPoint::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemActionPoint::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemActionPoint *a = dynamic_cast< const CMaraudersMapItemActionPoint* >( &item );
	if ( a )
	{
		return a->m_entity.Get() == m_entity.Get();
	}

	return false;
}

Bool CMaraudersMapItemActionPoint::IsValid() const
{
	return m_entity.Get() != NULL ? true : false;
}

String CMaraudersMapItemActionPoint::GetStringFromArray( const TDynArray< CName > &names ) const
{
	String result;

	for ( TDynArray< CName >::const_iterator name = names.Begin();
		name != names.End(); )
	{
		result += name->AsString();
		if ( ++name != names.End() )
		{
			result += TXT(", ");
		}
	}

	return result;
}

const TagList *CMaraudersMapItemActionPoint::GetTags()
{
	if ( m_tagList.Empty() )
	{
		m_tagList.AddTags( m_entity.Get()->GetTags() );

		for ( TDynArray< CActionPointComponent* >::const_iterator apI = m_apComponents.Begin();
			  apI != m_apComponents.End();
			  ++apI )
		{
			m_tagList.AddTags( (*apI)->GetTags() );
		}
	}

	return &m_tagList;
}

Bool CMaraudersMapItemActionPoint::DoesMatchApID( TActionPointID apID ) const
{
	if ( apID == ActionPointBadID ) return false;

	for ( TDynArray< CActionPointComponent* >::const_iterator ci = m_apComponents.Begin();
		  ci != m_apComponents.End();
		  ++ci )
	{
		if ( (*ci)->GetID() == apID )
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

String CMaraudersMapItemAgentStub::GetShortDescription() const
{
	String txt( TXT("Agent Stub") );
	if ( m_agentStub->GetEntitiesEntry() )
	{
		CEntityTemplate *entityTemplate = m_agentStub->GetEntitiesEntry()->m_entityTemplate.Get();
		if ( entityTemplate )
		{
			txt += TXT(" : ") + entityTemplate->GetFile()->GetFileName();
		}
	}
	return txt;
}

String CMaraudersMapItemAgentStub::GetFullDescription() const
{
	String result( TXT("<h2>Agent Stub</h2><br>") );

	if ( !IsValid() )
	{
	    return result;
	}
	if ( !GGame->IsActive() )
	{
		return TXT("");
	}
	CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
	Int32 stubIdx = -1;
	if ( cs )
	{
		stubIdx = cs->GetStubIndex( m_agentStub );
	}

	result += TXT("Current state: ") + CCommunityUtility::GetFriendlyAgentStateName( m_agentStub->m_state ) + TXT("<br><br>");
	const NewNPCSchedule& schedule = m_agentStub->GetSchedule();
	if ( schedule.m_activeActionPointID != ActionPointBadID )
	{
		String currAPFriendlyName = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( schedule.m_activeActionPointID );

		result += TXT("Current AP name: ") + currAPFriendlyName + TXT("<br>");
	}
	//result += TXT("Active layer: ") + m_agentStub->GetLayerName() + TXT("<br>");

	result += TXT("Processing timer: " ) + String::Printf( TXT("%f"), m_agentStub->m_processingTimer ) + TXT("<br>");

	// Community owner (stub origin)
	result += TXT("<h3>Community owner (stub owner)</h3>");
	result += TXT("Community: ") + m_html.MakeLinkResourceGotoAndOpen( m_agentStub->GetParentSpawnset() ) + TXT(" ") +
		m_agentStub->GetParentSpawnset()->GetFriendlyName() + TXT("<br>");
	result += TXT("Main Entry: ") + m_agentStub->GetActiveSpawnsetLine()->m_comment + TXT(" ")
		+ m_agentStub->GetActiveSpawnsetLine()->m_entryID + TXT("<br>");
	result += TXT("Story phase entry: ") + m_agentStub->GetActivePhase()->m_comment + TXT("<br>");
	result += TXT("Deactivate all communities but ") + m_html.MakeLinkDeactivateAllCommunititesButThis( m_agentStub->GetParentSpawnset() ) + TXT("<br>");
	result += TXT("Turn only one STUB mode ") + m_html.MakeLinkOnlyOneStubMode( stubIdx ) + TXT("<br>");

	// Community entities entry
	if ( m_agentStub->GetEntitiesEntry() )
	{
		result += TXT("<h3>Entities Entry</h3>");
		result += TXT("Appearances: ") + GetStringFromArray( m_agentStub->GetEntitiesEntry()->m_appearances )
			+ TXT("<br>");
		result += TXT("Spawn tags: ") + m_agentStub->GetEntitiesEntry()->m_entitySpawnTags.ToString()
			+ TXT("<br>");
		CEntityTemplate *entityTemplate = m_agentStub->GetEntitiesEntry()->m_entityTemplate.Get();
		if ( entityTemplate )
		{
			result += TXT("Entity template: ")
				+ entityTemplate->GetFriendlyName()
				+ TXT("<br>");
		}
		else
		{
			result += TXT("Entity template: Not loaded<br>");
		}
	}

	// Associated NPC
	if ( m_agentStub->m_npc.Get() )
	{
		result += TXT("NPC is spawned<br>");
	}
	else
	{
		result += TXT("NPC is despawned<br>");
	}

	// Story phase
	if ( m_agentStub->GetParentSpawnset() )
	{
		const CName storyPhase = m_agentStub->GetParentSpawnset()->GetActivePhaseName();
		if ( storyPhase == CName::NONE )
		{
			result += TXT("<br>Story phase: <b>Default</b><br>");
		}
		else
		{
			result += TXT("<br>Story phase: <b>") + storyPhase.AsString() + TXT("</b><br>");
		}
	}

	// Agent stub info
	result += TXT("<h2>Agent stub debug info</h2>");

	HtmlCommunityDebugPage debugPage;
	m_agentStub->GetDebugInfo( debugPage );

	result += debugPage.GetMessage() + TXT("<br>");

	result += TXT("<h2>Programmers info</h2>");

	if ( cs )
	{
		result += String::Printf( TXT("Stub index: %d <br>"), stubIdx );
	}

	return result;
}

Vector CMaraudersMapItemAgentStub::GetWorldPosition() const
{
	return Vector( m_agentStub->m_communityAgent.GetPosition() );
}

wxRect CMaraudersMapItemAgentStub::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );

		// for circle with scaled radius
		Int32 radius = canvas->WorldToCanvas( Vector( 1.0f, 0, 0 ) ).x;
		rect.x = point.x - radius;
		rect.y = point.y - radius;
		rect.width = radius * 2;
		rect.height = radius * 2;
	}
	return rect;
}

void CMaraudersMapItemAgentStub::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 0, 255 );
	Vector worldPos = GetWorldPosition();
	canvas->DrawCircleCanvas( worldPos, 0.6f, color );
}

void CMaraudersMapItemAgentStub::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 0, 255 );
	static wxColor colorNPC( 255, 0, 255 );
	canvas->FillCircleCanvas( GetWorldPosition(), 0.6f, color );

	const NewNPCSchedule& schedule = m_agentStub->GetSchedule();
	if ( schedule.m_activeActionPointID != ActionPointBadID )
	{
		Vector worldAPPos;
		GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetActionExecutionPosition( schedule.m_activeActionPointID, &worldAPPos, NULL );
		canvas->DrawCrossCavnas( worldAPPos, 0.6f, color, 2.0f );

		GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetGoToPosition( schedule.m_activeActionPointID, &worldAPPos, NULL );
		canvas->DrawCrossCavnas( worldAPPos, 0.8f, wxColor( 0, 100, 255 ), 2.0f );
	}

	CNewNPC *actor = m_agentStub->m_npc.Get();
	if ( actor )
	{
		Vector worldNpcPos = actor->GetWorldPosition();
		canvas->DrawCrossCavnas( worldNpcPos, 0.6f, colorNPC, 2.0f );
	}
}

void CMaraudersMapItemAgentStub::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemAgentStub::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemAgentStub *a = dynamic_cast< const CMaraudersMapItemAgentStub* >( &item );
	if ( a )
	{
		return a->m_agentStub == m_agentStub;
	}

	return false;
}

Bool CMaraudersMapItemAgentStub::IsValid() const
{
	if ( GCommonGame->GetSystem< CCommunitySystem >() )
	{
		return GCommonGame->GetSystem< CCommunitySystem >()->DoesStubExist( m_agentStub );
	}
	else
	{
		return false;
	}
}

Bool CMaraudersMapItemAgentStub::IsVisible() const
{
	//return m_agentStub->IsOnlyStub();
	return true;
}

String CMaraudersMapItemAgentStub::GetStringFromArray( const TDynArray< CName > &names ) const
{
	String result;

	for ( TDynArray< CName >::const_iterator name = names.Begin();
		name != names.End(); )
	{
		result += name->AsString();
		if ( ++name != names.End() )
		{
			result += TXT(", ");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////

String CMaraudersMapItemWayPoint::GetShortDescription() const
{
	String txt;
	txt = TXT("Waypoint: ") + m_entity.Get()->GetFriendlyName();
	return txt;
}

String CMaraudersMapItemWayPoint::GetFullDescription() const
{
	String result;
	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		result = TXT("<h2>Way Point</h2><br>");
		result += TXT("Entity name: ") + entity->GetFriendlyName() + TXT("<br>");
		result += TXT("Entity tags: ") + entity->GetTags().ToString() + TXT("<br>");
	}

	return result;
}

Vector CMaraudersMapItemWayPoint::GetWorldPosition() const
{
	Vector vector(0,0,0);
	vector = m_entity.Get()->GetWorldPosition();
	return vector;
}

// RTODO: maybe move this method to base class
wxRect CMaraudersMapItemWayPoint::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( m_entity.Get() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );
		rect.x = point.x - 5;
		rect.y = point.y - 5;
		rect.width = 10;
		rect.height = 10;
	}
	return rect;
}

void CMaraudersMapItemWayPoint::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	if ( !m_entity.Get() ) return;

	static wxColor color( 255, 255, 0 );
	//wxPoint pos = canvas->WorldToCanvas( GetWorldPosition() );
	//canvas->DrawCircle( pos, 10, color );
	canvas->DrawCircleConstRadiusCanvas( GetWorldPosition(), 5, color );
}

void CMaraudersMapItemWayPoint::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	//wxPoint pos = canvas->WorldToCanvas( GetWorldPosition() );
	static wxColor color( 255, 255, 0 );
	//canvas->FillCircle( pos, 10, color );
	canvas->FillCircleConstRadiusCanvas( GetWorldPosition(), 5, color );
}

void CMaraudersMapItemWayPoint::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemWayPoint::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemWayPoint *a = dynamic_cast< const CMaraudersMapItemWayPoint* >( &item );
	if ( a )
	{
		return a->m_entity.Get() == m_entity.Get();
	}

	return false;
}

Bool CMaraudersMapItemWayPoint::IsValid() const
{
	return m_entity.Get() != NULL ? true : false;
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemSticker::CMaraudersMapItemSticker( CEntity *entity )
	: m_entity( entity )
{
	m_sticker = SafeCast< CStickerComponent >( entity->GetComponents()[0] );
}

String CMaraudersMapItemSticker::GetShortDescription() const
{
	String txt;
	txt = TXT("Sticker: ") + m_entity.Get()->GetFriendlyName();
	return txt;
}

String CMaraudersMapItemSticker::GetFullDescription() const
{
	String result;
	result = TXT("<h2>Sticker</h2><br>");
	result += m_sticker->GetText();
	return result;
}

Vector CMaraudersMapItemSticker::GetWorldPosition() const
{
	Vector vector(0,0,0);
	vector = m_entity.Get()->GetWorldPosition();
	return vector;
}

wxRect CMaraudersMapItemSticker::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( m_entity.Get() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );
		rect.x = point.x - 5;
		rect.y = point.y - 4;
		rect.width = 10;
		rect.height = 8;
	}
	return rect;
}

void CMaraudersMapItemSticker::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	if ( !m_entity.Get() ) return;

	static wxColor color( 0, 255, 0 );

	wxPoint pos = canvas->WorldToCanvas( GetWorldPosition() );

	wxPoint pointBeg( pos.x-5, pos.y-4 );
	wxPoint pointEnd( pos.x+5, pos.y+4 );

	wxRect rect( pointBeg, pointEnd );
	canvas->DrawRect( rect, color );
}

void CMaraudersMapItemSticker::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 255, 0 );

	wxPoint pos = canvas->WorldToCanvas( GetWorldPosition() );

	wxPoint pointBeg( pos.x-5, pos.y-4 );
	wxPoint pointEnd( pos.x+5, pos.y+4 );

	wxRect rect( pointBeg, pointEnd );
	canvas->FillRect( rect, color );
}

void CMaraudersMapItemSticker::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemSticker::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemSticker *a = dynamic_cast< const CMaraudersMapItemSticker* >( &item );
	if ( a )
	{
		return a->m_entity.Get() == m_entity.Get();
	}

	return false;
}

Bool CMaraudersMapItemSticker::IsValid() const
{
	return m_entity.Get() != NULL ? true : false;
}

///////////////////////////////////////////////

CMaraudersMapItemDeniedArea::CMaraudersMapItemDeniedArea( const CDeniedAreaComponent* deniedArea )
{
	m_deniedArea = deniedArea;

	Uint8 red	= GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 green	= GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 blue	= GEngine->GetRandomNumberGenerator().Get< Uint8 >();

	m_color.Set( red, green, blue );
}

String CMaraudersMapItemDeniedArea::GetShortDescription() const
{
	String txt(TXT("Denied area short description"));
	return txt;
}

String CMaraudersMapItemDeniedArea::GetFullDescription() const
{
	String txt(TXT("Denied area full desctiption"));
	return txt;
}

Vector CMaraudersMapItemDeniedArea::GetWorldPosition() const
{
	return m_deniedArea.Get()->GetWorldPosition();
}

wxRect CMaraudersMapItemDeniedArea::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	return rect;
}

void CMaraudersMapItemDeniedArea::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	const CAreaComponent::TAreaPoints& points = m_deniedArea.Get()->GetWorldPoints();
	for ( Uint32 i=0; i<points.Size(); ++i )
	{
		const Vector& a = points[i];
		const Vector& b = points[(i+1)%points.Size()];

		canvas->DrawLineCanvas( a, b, m_color );
	}
}

void CMaraudersMapItemDeniedArea::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	const CAreaComponent::TAreaPoints& points = m_deniedArea.Get()->GetWorldPoints();
	for ( Uint32 i=0; i<points.Size(); ++i )
	{
		const Vector& a = points[i];
		const Vector& b = points[(i+1)%points.Size()];

		static wxColor color( 0, 0, 128 );
		canvas->DrawLineCanvas( a, b, color );
	}
}

void CMaraudersMapItemDeniedArea::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemDeniedArea::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemDeniedArea *a = dynamic_cast< const CMaraudersMapItemDeniedArea* >( &item );
	if ( a )
	{
		return a->m_deniedArea.Get() == m_deniedArea.Get();
	}
	return false;
}

Bool CMaraudersMapItemDeniedArea::IsValid() const
{
	return m_deniedArea.Get() != NULL ? true : false;
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemCommunityArea::CMaraudersMapItemCommunityArea( const CCommunityArea* communityArea )
	: m_communityArea( communityArea )
{
	ComponentIterator< CAreaComponent > areaCompI( communityArea );
	m_communityAreaComponent = *areaCompI;
	m_color.Set( 0, 0, 170 );
	m_activeColor.Set( 0, 0, 255 );
	m_selectedColor.Set( 255, 255, 0 );
}

String CMaraudersMapItemCommunityArea::GetShortDescription() const
{
	String txt(TXT("Community area"));

	const CCommunityArea *carea = m_communityArea.Get();
	if ( carea )
	{
		txt += TXT(" : ") + carea->GetFriendlyName();
	}

	return txt;
}

String CMaraudersMapItemCommunityArea::GetFullDescription() const
{
	String txt(TXT("<h1>Community area</h1><br>"));

	const CCommunityArea* commArea = m_communityArea.Get();
	if ( commArea )
	{
		txt += TXT("Friendly name: ") + commArea->GetFriendlyName() + TXT("<br><br>");

		CCommunityAreaType* areaType = commArea->GetAreaType();
		if ( areaType )
		{
			CCommunityAreaTypeDefault *areaDefault = Cast< CCommunityAreaTypeDefault >( areaType );
			if ( areaDefault )
			{
				Float areaSpawnRadius;
				Float areaDespawnRadius;
				Float spawnRadius;
				Float despawnRadius;
				Bool dontRestore;
				Float prevSpawnRadius;
				Float prevDespawnRadius;
				areaDefault->GetAreaData( areaSpawnRadius, areaDespawnRadius, spawnRadius, despawnRadius, dontRestore, prevSpawnRadius, prevDespawnRadius );

				txt += TXT("Area type: DEFAULT<br>");
				txt += String::Printf( TXT("Area spawn radius : %f<br>Area despawn radius: %f<br>Spawn radius: %f<br>Despawn radius: %f<br>PrevSpawnRadius: %f<br>PrevDespawnRadius: %f<br>"),
					areaSpawnRadius, areaDespawnRadius, spawnRadius, despawnRadius, prevSpawnRadius, prevDespawnRadius );
				txt += TXT("Don't restore: ") + ToString(dontRestore) + TXT("<br>");
			}

			CCommunityAreaTypeSpawnRadius *areaSpawnRadius = Cast< CCommunityAreaTypeSpawnRadius >( areaType );
			if ( areaSpawnRadius )
			{
				txt += TXT("Area type: SPAWN RADIUS<br>");

				Float spawnRadius;
				Float despawnRadius;
				Bool dontRestore;
				Float prevSpawnRadius;
				Float prevDespawnRadius;
				areaSpawnRadius->GetAreaData( spawnRadius, despawnRadius, dontRestore, prevSpawnRadius, prevDespawnRadius );

				txt += TXT("Area type: DEFAULT<br>");
				txt += String::Printf( TXT("Spawn radius: %f<br>Despawn radius: %f<br>PrevSpawnRadius: %f<br>PrevDespawnRadius: %f<br>"),
					spawnRadius, despawnRadius, prevSpawnRadius, prevDespawnRadius );
				txt += TXT("Don't restore: ") + ToString(dontRestore) + TXT("<br>");
			}

			txt += TXT("<br><h2>Current settings</h2><br>");
			Float spawnRadius = 0, despawnRadius = 0;
			GCommonGame->GetSystem< CCommunitySystem >()->GetVisibilityRadius( spawnRadius, despawnRadius );
			txt += String::Printf( TXT("Spawn radius: %f<br>Despawn radius: %f<br>"), spawnRadius, despawnRadius );
		}
		else
		{
			txt += TXT("Area Type is NULL - community area is not active!");
		}
	}

	return txt;
}

Vector CMaraudersMapItemCommunityArea::GetWorldPosition() const
{
	return m_communityAreaComponent.Get()->GetWorldPosition();
}

wxRect CMaraudersMapItemCommunityArea::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );

	const CAreaComponent *area = m_communityAreaComponent.Get();
	if ( area )
	{
		const Box &box = area->GetBoundingBox();

		wxPoint pointBeg, pointEnd;
		BoxToPointsClient( canvas, box, pointBeg, pointEnd );

		rect.SetLeft( pointBeg.x );
		rect.SetRight( pointEnd.x );
		rect.SetTop( pointEnd.y );
		rect.SetBottom( pointBeg.y );
	}

	return rect;
}

void CMaraudersMapItemCommunityArea::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	wxColor color = m_color;
	Float width = 1.0f;

	CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
	const CAreaComponent *area = m_communityAreaComponent.Get();
	if ( cs && area )
	{
		if ( cs->GetVisibilityArea() == area )
		{
			color = m_activeColor;
			width = 2.0f;
		}
	}

	const CAreaComponent::TAreaPoints& points = m_communityAreaComponent.Get()->GetWorldPoints();
	for ( Uint32 i=0; i<points.Size(); ++i )
	{
		const Vector& a = points[i];
		const Vector& b = points[(i+1)%points.Size()];

		canvas->DrawLineCanvas( a, b, color );
	}
}

void CMaraudersMapItemCommunityArea::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	const CAreaComponent::TAreaPoints& points = m_communityAreaComponent.Get()->GetWorldPoints();
	for ( Uint32 i=0; i<points.Size(); ++i )
	{
		const Vector& a = points[i];
		const Vector& b = points[(i+1)%points.Size()];

		canvas->DrawLineCanvas( a, b, m_selectedColor );
	}
}

void CMaraudersMapItemCommunityArea::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemCommunityArea::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemCommunityArea *a = dynamic_cast< const CMaraudersMapItemCommunityArea* >( &item );
	if ( a )
	{
		return a->m_communityArea.Get() == m_communityArea.Get();
	}
	return false;
}

Bool CMaraudersMapItemCommunityArea::IsValid() const
{
	return m_communityArea.Get() != NULL ? true : false;
}

void CMaraudersMapItemCommunityArea::BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const
{
	const Float &xBeg = box.Min.X;
	const Float &yBeg = box.Min.Y;
	const Float &xEnd = box.Max.X;
	const Float &yEnd = box.Max.Y;
	Vector vBeg( xBeg, yBeg, 0 );
	Vector vEnd( xEnd, yEnd, 0 );
	pointBeg = canvas->WorldToClient( vBeg );
	pointEnd = canvas->WorldToClient( vEnd );
}

//////////////////////////////////////////////////////////////////////////

CMaraudersMapItemEncounter::CMaraudersMapItemEncounter( CEncounter *encounter )
	: m_encounter( encounter )
{
	TDynArray< CTriggerAreaComponent* > triggers;
	CollectEntityComponents( encounter, triggers );
	ASSERT( !triggers.Empty() );
	if ( !triggers.Empty() )
	{
		m_boundingBox = triggers[0]->GetBoundingBox();
	}
}

String CMaraudersMapItemEncounter::GetShortDescription() const
{
	String txt;
	CEncounter* encounter = m_encounter.Get();
	if ( encounter )
	{
		txt = TXT("Encounter: ") + encounter->GetFriendlyName();
	}
	return txt;
}

String CMaraudersMapItemEncounter::GetFullDescription() const
{
	String result;
	result = TXT("<h2>Encounter</h2><br>");
	//result += m_encounter.Get()->GetCurrentDebugStatus();
	return result;
}

Vector CMaraudersMapItemEncounter::GetWorldPosition() const
{
	Vector vector(0,0,0);
	vector = m_encounter.Get()->GetWorldPosition();
	return vector;
}

wxRect CMaraudersMapItemEncounter::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	wxRect rect( 0, 0, 0, 0 );
	if ( m_encounter.Get() )
	{
		wxPoint point = canvas->WorldToClient( GetWorldPosition() );

		wxPoint pointBeg, pointEnd;
		BoxToPointsClient( canvas, m_boundingBox, pointBeg, pointEnd );

		rect.SetLeft( pointBeg.x );
		rect.SetRight( pointEnd.x );
		rect.SetTop( pointEnd.y );
		rect.SetBottom( pointBeg.y );
	}
	return rect;
}

void CMaraudersMapItemEncounter::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 128, 0, 128 );

	const TDynArray<Vector> *points = m_encounter.Get()->GetWorldPoints();
	if ( points )
	{
		for ( Uint32 i = 0; i < points->Size(); ++i )
		{
			const Vector& a = (*points)[ i ];
			const Vector& b = (*points)[ (i+1)%points->Size() ];

			canvas->DrawLineCanvas( a, b, color );
		}
	}
}

void CMaraudersMapItemEncounter::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 168, 0, 168 );

	const TDynArray<Vector> *points = m_encounter.Get()->GetWorldPoints();
	if ( points )
	{
		for ( Uint32 i=0; i < points->Size(); ++i )
		{
			const Vector& a = (*points)[i];
			const Vector& b = (*points)[(i+1)%points->Size()];

			canvas->DrawLineCanvas( a, b, color );
		}
	}
}

void CMaraudersMapItemEncounter::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemEncounter::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemEncounter *a = dynamic_cast< const CMaraudersMapItemEncounter* >( &item );
	if ( a )
	{
		return a->m_encounter.Get() == m_encounter.Get();
	}

	return false;
}

Bool CMaraudersMapItemEncounter::IsValid() const
{
	return m_encounter.Get() != NULL ? true : false;
}

void CMaraudersMapItemEncounter::BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const
{
	const Float &xBeg = box.Min.X;
	const Float &yBeg = box.Min.Y;
	const Float &xEnd = box.Max.X;
	const Float &yEnd = box.Max.Y;
	Vector vBeg( xBeg, yBeg, 0 );
	Vector vEnd( xEnd, yEnd, 0 );
	pointBeg = canvas->WorldToClient( vBeg );
	pointEnd = canvas->WorldToClient( vEnd );
}

//////////////////////////////////////////////////////////////////////////

const String CMaraudersMapItemDoor::OPTION_OPEN              = TXT("Open door");
const String CMaraudersMapItemDoor::OPTION_CLOSE             = TXT("Close door");

TDynArray< String > CMaraudersMapItemDoor::s_optionsNames;

CMaraudersMapItemDoor::CMaraudersMapItemDoor( CDoorComponent *door )
	: m_door( door )
{
	CEntity* entity = m_door.Get()->GetEntity();
	m_shortDescription = TXT("Door: ") + entity->GetFriendlyName();

	for ( ComponentIterator< CMeshTypeComponent > meshComponent( entity ); meshComponent; ++meshComponent )
	{
		const Box &box = (*meshComponent)->GetBoundingBox();
		m_bboxes.PushBack( box );
	}

	if( s_optionsNames.Size() == 0 )
	{
		s_optionsNames.PushBack( OPTION_OPEN );
		s_optionsNames.PushBack( OPTION_CLOSE );
	}
}

CMaraudersMapItemDoor::~CMaraudersMapItemDoor()
{
}

String CMaraudersMapItemDoor::GetShortDescription() const
{
	return m_shortDescription;
}

String CMaraudersMapItemDoor::GetFullDescription() const
{
	String txt( TXT("<h2>Door</h2>") );
	return txt;
}

Vector CMaraudersMapItemDoor::GetWorldPosition() const
{
	Vector vector( 0, 0, 0 );
	if ( m_door.Get() )
	{
		vector = m_door.Get()->GetEntity()->GetWorldPosition();
	}

	return vector;
}

void CMaraudersMapItemDoor::BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const
{
	const Float &xBeg = box.Min.X;
	const Float &yBeg = box.Min.Y;
	const Float &xEnd = box.Max.X;
	const Float &yEnd = box.Max.Y;
	Vector vBeg( xBeg, yBeg, 0 );
	Vector vEnd( xEnd, yEnd, 0 );
	pointBeg = canvas->WorldToClient( vBeg );
	pointEnd = canvas->WorldToClient( vEnd );
}

wxRect CMaraudersMapItemDoor::GetClientBorder( CMaraudersMapCanvasDrawing *canvas )
{
	m_border = wxRect( 0, 0, 0, 0 );

	wxPoint pointBeg, pointEnd;
	if ( m_bboxes.Size() > 0 )
	{
		BoxToPointsClient( canvas, m_bboxes[ 0 ], pointBeg, pointEnd );

		m_border.SetLeft( pointBeg.x );
		m_border.SetRight( pointEnd.x );
		m_border.SetTop( pointEnd.y );
		m_border.SetBottom( pointBeg.y );
	}

	for ( Uint32 i = 1; i < m_bboxes.Size(); ++i )
	{
		BoxToPointsClient( canvas, m_bboxes[ i ], pointBeg, pointEnd );

		if ( pointBeg.x < m_border.GetLeft()   ) m_border.SetLeft  ( pointBeg.x );
		if ( pointEnd.x > m_border.GetRight()  ) m_border.SetRight ( pointEnd.x );
		if ( pointEnd.y < m_border.GetTop()    ) m_border.SetTop   ( pointEnd.y );
		if ( pointBeg.y > m_border.GetBottom() ) m_border.SetBottom( pointBeg.y );
	}

	return m_border;
}

void CMaraudersMapItemDoor::Draw( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 255, 255 );
	static wxColor color1( 255, 255, 0 );
	static wxColor colorLocked( 215, 0, 0 );

	// get door component
	CDoorComponent *door = m_door.Get();
	if ( door == NULL ) 
	{
		return;
	}

	wxRect rect = canvas->ClientToCanvas( m_border );
	if ( door->IsOpen() )
	{
		canvas->DrawRect( rect, color );
	}
	else
	{
		canvas->DrawRect( rect, color1 );
	}

	if ( door->IsLocked() )
	{
		wxRect lockedRect = rect;
		if ( rect.GetSize().GetX() > 2 && rect.GetSize().GetY() > 2 )
		{
			lockedRect.Deflate( 2, 2 );
		}
		canvas->FillRect( lockedRect, colorLocked );
	}
}

void CMaraudersMapItemDoor::DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const
{
	static wxColor color( 0, 255, 255 );

	if ( m_door.Get() == NULL ) 
	{
		return;
	}

	wxRect rect = canvas->ClientToCanvas( m_border );
	canvas->FillRect( rect, color );
}

void CMaraudersMapItemDoor::DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const
{
	const Vector &v = GetWorldPosition();
	canvas->DrawTooltip( v.X, v.Y, GetShortDescription() );
}

Bool CMaraudersMapItemDoor::operator==( const CMaraudersMapItemBase& item ) const
{
	const CMaraudersMapItemDoor *a = dynamic_cast< const CMaraudersMapItemDoor* >( &item );
	if ( a )
	{
		return a->m_door.Get() == m_door.Get();
	}

	return false;
}

Bool CMaraudersMapItemDoor::IsValid() const
{
	return m_door.Get() != NULL;
}

Bool CMaraudersMapItemDoor::Update( CMaraudersMapCanvasDrawing *canvas )
{
	GetClientBorder( canvas );
	return false;
}

Bool CMaraudersMapItemDoor::ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ )
{
	CDoorComponent *door = m_door.Get();
	if ( door )
	{
		if ( s_optionsNames[ optionNum ] == OPTION_OPEN )
		{
			door->Open();
			return true;
		}
		else if ( s_optionsNames[ optionNum ] == OPTION_CLOSE )
		{
			door->Close();
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

String CMaraudersMapItemHtmlBuilder::MakeLinkVector( const Vector& vec ) const
{
	return String::Printf( TXT("<a href=\"goto:%f %f %f\">(%.1f, %.1f, %.1f)</a>"),
		vec.X, vec.Y, vec.Z, vec.X, vec.Y, vec.Z );
}

String CMaraudersMapItemHtmlBuilder::MakeLinkResourceGotoAndOpen( const CResource* res ) const
{
	return String::Printf( TXT("<a href=\"gotoandopenres:%s\">%s</a>"),
		res->GetFile()->GetDepotPath().AsChar(), res->GetFile()->GetFileName().AsChar() );
}

String CMaraudersMapItemHtmlBuilder::MakeLinkResourceGoto( const CResource* res ) const
{
	return String::Printf( TXT("<a href=\"gotores:%s\">%s</a>"),
		res->GetFile()->GetDepotPath().AsChar(), res->GetFile()->GetFileName().AsChar() );
}

String CMaraudersMapItemHtmlBuilder::MakeLinkMaraudersItemGotoActionPoint( const String& desc, TActionPointID apID ) const
{
	return String::Printf( TXT("<a href=\"goto_maritem_ap:%d\">%s</a>"),
		apID, desc.AsChar() );
}

String CMaraudersMapItemHtmlBuilder::MakeLinkDeactivateAllCommunititesButThis( const CCommunity *community ) const
{
	return String::Printf( TXT("<a href=\"deactivate_communities:%s\">%s</a>"),
		community->GetFile()->GetDepotPath().AsChar(), TXT("this") );
}

String CMaraudersMapItemHtmlBuilder::MakeLinkOnlyOneStubMode( Int32 stubIdx ) const
{
	return String::Printf( TXT("<a href=\"only_one_stub_mode:%d\">activate</a>"), stubIdx );
}
