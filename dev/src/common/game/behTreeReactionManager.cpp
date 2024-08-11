#include "build.h"
#include "behTreeReactionManager.h"

#include "../engine/areaComponent.h"

#include "actorsManager.h"
#include "binaryStorage.h"
#include "gameWorld.h"
#include "reactionScene.h"
#include "reactionSceneActor.h"
#include "behTreeMachine.h"
#include "behTreeInstance.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeReactionManager )

RED_DEFINE_STATIC_NAME( AttackAction )
RED_DEFINE_STATIC_NAME( BeingHitAction )
RED_DEFINE_STATIC_NAME( BombExplosionAction )

Float CBehTreeReactionManager::CHATING_IN_AP_DISTANCE_SQR = 2.5f*2.5f;

CBehTreeReactionManager::CBehTreeReactionManager()
	: m_suppressScaredReactions( false )
{
	m_reactionEvents.ClearFast();
}

void CBehTreeReactionManager::AddRainAwareNPC( CNewNPC* npc )
{
	m_rainAwareNPCs.PushBackUnique( npc );
}

void CBehTreeReactionManager::RemoveRainAwareNPC( CNewNPC* npc )
{
	m_rainAwareNPCs.Remove( npc );
}

Bool CBehTreeReactionManager::SuppressReactions( Bool toggle, CName areaTag )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return false;
	}

	CEntity* areaEntity = world->GetTagManager()->GetTaggedEntity( areaTag );
	if ( !areaEntity )
	{
		return false;
	}

	CAreaComponent* areaComponent = areaEntity->FindComponent< CAreaComponent >();
	if ( !areaComponent )
	{
		return false;
	}

	if ( toggle )
	{
		return m_suppressedAreas.PushBackUnique( areaComponent );
	}
	else
	{
		return m_suppressedAreas.RemoveFast( areaComponent );
	}
}

void  CBehTreeReactionManager::Init()
{
	m_reactionSceneGroups.ClearFast();
	CallFunction( this, CNAME( RegisterReactionSceneGroups ) );
	m_rainAwareNPCs.Clear();
	m_suppressScaredReactions = false;
}

void CBehTreeReactionManager::Update()
{
	if( m_reactionEvents.Size() > 0 )
	{
		for( Int32 i = m_reactionEvents.Size()-1; i >= 0; --i )
		{
			if( m_reactionEvents[i]->HasExpired() )
			{
				DeleteReactionEvent( i );
				continue;
			}

			Float currTime = GGame->GetEngineTime();
			if( m_reactionEvents[i]->GetBroadcastTime() <= currTime && m_reactionEvents[i]->GetBroadcastInterval() > 0 )
			{//for interval == -1 broadcast was fired when event was created

				Broadcast( *m_reactionEvents[i] );				

				m_reactionEvents[i]->SetBroadcastTime( currTime + m_reactionEvents[i]->GetBroadcastInterval() );
			}
		}
	}
}

void CBehTreeReactionManager::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	file << m_reactionEvents;

	for( Uint32 i = 0; i < m_reactionEvents.Size(); ++i )
	{
		m_reactionEvents[i]->PostLoad();
	}
}

void CBehTreeReactionManager::BroadcastReactionScene( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data )
{
	CReactionScene* reactionScene = data.GetReactionScene();
	CEntity* invoker = data.GetInvoker();
	CReactionSceneActorComponent* sceneInvokerCmp = invoker->FindComponent< CReactionSceneActorComponent >();
	sceneInvokerCmp->SetSceneStartedSuccesfully( false );

	const Float maxAngle = 65.f;
	const Float invokerYaw = invoker->GetWorldYaw();

	CName invokerGroup = sceneInvokerCmp->GetVoicesetGroup();	
	CActor* invokerAsActor = Cast< CActor >( invoker );	

	Bool isInvokerAtWork = invokerAsActor->IsAtWork();

	for( Uint32 i = 0; i < actors.Size(); ++i )
	{
		if( data.CanBeBroadcasted() )
		{
			CActor* actor = actors[ i ].Get();
			if( actor && actor != invoker )
			{
				//NPC's have to face each other to within a certain degree, otherwise, skip this particular actor
				// TODO ML: fix this condition
				/*const Float angleDiff = EulerAngles::AngleDistance( actor->GetWorldYaw(), invokerYaw );
				if ( Abs(angleDiff) > maxAngle )
				{
					continue;
				}*/

				if( !actor->WasVisibleLastFrame() )
				{
					continue;
				}

				CNewNPC* npc = Cast< CNewNPC >( actor );				

				if( !npc || npc->IsInGameplayScene() )
				{
					continue;
				}

				Bool npcAtWork = npc->IsAtWork();

				if( npcAtWork )
				{
					// both must be working
					if( !isInvokerAtWork )
					{
						continue;
					}

					// close to each other
					if( npc->GetWorldPositionRef().DistanceSquaredTo2D( invoker->GetWorldPositionRef() ) > CHATING_IN_AP_DISTANCE_SQR )
					{
						continue;
					}

					// and not GreetingAction
					if( data.GetEventName() == CNAME( GreetingAction ) )
					{
						continue;
					}
				}

				CReactionSceneActorComponent* actorCmp = actor->FindComponent< CReactionSceneActorComponent >();
				
				if( !actorCmp )
				{
					continue;
				}
				
				CName recieverGroup = actorCmp->GetVoicesetGroup();

				if( recieverGroup == CName::NONE )
				{
					continue;
				}

				if( invokerGroup != CNAME( _acceptAll ) && invokerGroup != recieverGroup )
				{
					continue;
				}

				if( !actorCmp->IfActorHasSceneInput( sceneInvokerCmp->GetRequiredSceneInputs() ) )
				{
					continue;
				}

				actor->SignalGameplayEvent( data.GetEventName(), &data, CBehTreeReactionEventData::GetStaticClass() );
				if( reactionScene->IfAllActrorsCollected() )
				{
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	if( reactionScene->IfAllActrorsCollected() )
	{
		THashMap< CName, THandle< CReactionSceneActorComponent > >* assignedActors = reactionScene->GetActors();
		for( auto it = assignedActors->Begin(); it != assignedActors->End(); ++it )
		{
			if( !(*it).m_second.Get() )
			{
				reactionScene->Clear();
				return;
			}
		}

		sceneInvokerCmp->SetSceneStartedSuccesfully( true );

		data.GetReactionScene()->SetScenePhase( ERSEP_Playing );		

		for( auto it = assignedActors->Begin(); it != assignedActors->End(); ++it )
		{
			CReactionSceneActorComponent* actorCmp = (*it).m_second.Get();			
			CActor* actor = Cast< CActor >( actorCmp->GetParent() );
			if( actor )
			{
				actor->SignalGameplayEvent( data.GetEventName(), &data, CBehTreeReactionEventData::GetStaticClass() );
			}
		}
	}
	else
	{
		reactionScene->Clear();
	}
}

CName CBehTreeReactionManager::FindGroupForVoiceset( const String& voiceset )
{
	if( voiceset.Empty() )
		return CName::NONE;

	auto it = m_reactionSceneGroups.Find( voiceset );
	if( it != m_reactionSceneGroups.End() )
	{
		return it->m_second;
	}

	return CName::NONE;
}

void CBehTreeReactionManager::Broadcast( CBehTreeReactionEventData& data )
{
	if( data.CanBeBroadcasted() )
	{
		if( data.GetReactionScene() )
		{
			data.GetReactionScene()->SetScenePhase( ERSEP_Assigning );
			CActor* invokerAsActor = Cast< CActor >( data.GetInvoker() );
			if( invokerAsActor )
			{
				invokerAsActor->SignalGameplayEvent( data.GetEventName(), &data, CBehTreeReactionEventData::GetStaticClass() );
			}
		}
		
		m_actors.ClearFast();

		CEntity* invoker = data.GetInvoker();
		if( !invoker )
			return;

		FindActorsToBroadecast( m_actors, data );
		
		if( m_actors.Size() > 0 )
		{
			if( data.GetReactionScene() )
			{
				BroadcastReactionScene( m_actors, data );
			}			
			else
			{
				
				
				Bool skipInvoker = data.GetSkipInvoker();

				for( Uint32 i = 0; i < m_actors.Size(); ++i )
				{
					CNewNPC* npc = Cast< CNewNPC >( m_actors[ i ].Get() );

					//in chat scenes all reactions are disabled
					if( !npc )
					{
						continue;
					}

					if( data.CanBeBroadcasted() )
					{
						CActor* actor = m_actors[ i ].Get();
						if( actor )
						{
							if( skipInvoker && invoker == actor )
							{
								continue;
							}
							
							if( CBehTreeMachine* behTreeMachine = actor->GetBehTreeMachine() )
							{
								if( behTreeMachine->GetBehTreeInstance() )
								{
									behTreeMachine->GetBehTreeInstance()->SetNamedTarget( CNAME( ReactionTarget ), invoker );
								}									
							}								
							
							actor->SignalGameplayEvent( data.GetEventName(), &data, CBehTreeReactionEventData::GetStaticClass() );
						}
					}
					else
					{
						break;
					}
				}
			}
			
			m_actors.ClearFast();

			return;
		}
	}

	return;
}

void CBehTreeReactionManager::FindActorsToBroadecast( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data )
{
	STATIC_NODE_FILTER( IsNotPlayer, filterNotPlayer );
	static const INodeFilter* filters[] = { &filterNotPlayer };
	GCommonGame->GetActorsManager()->GetClosestToPoint( data.GetReactionCenter(), actors, data.GetRangeBox(), INT_MAX, filters, 1 );	
}

CReactionScene* CBehTreeReactionManager::CreateReactionScene( CName name, CEntity* invoker, CBehTreeReactionEventData* sourceEvent )
{
	PC_SCOPE( CBehTreeReactionManager_CreateReactionScene );
	CReactionScene* reactionScene;
	{
		PC_SCOPE( Allocate_CReactionScene );
		reactionScene = CreateObject< CReactionScene >( this );
	}
	
	m_reactionScens.PushBackUnique( reactionScene );

	reactionScene->SetSourceEvent( sourceEvent );
	
	CReactionSceneActorComponent* actorComponent = invoker->FindComponent< CReactionSceneActorComponent >( );
	ASSERT( actorComponent && "CReactionSceneActorComponent not found in invoker" );
	if( actorComponent )
	{
		actorComponent->SetReactionScene( reactionScene );
		reactionScene->SetInvoker( actorComponent );
	}
	return reactionScene;
}

Bool CBehTreeReactionManager::IsScaryEvent( CName eventName )
{
	return eventName == CNAME( AttackAction ) || eventName == CNAME( BeingHitAction ) || eventName == CNAME( BombExplosionAction );
}

Bool CBehTreeReactionManager::CreateReactionEventIfPossible( CEntity* invoker, CName eventName, Float lifetime, Float distanceRange, Float broadcastInterval, Int32 recipientCount, Bool skipInvoker, Bool useCustomReactionCenter, const Vector& reactionCenter )
{
	if ( CNewNPC* npc = ::Cast< CNewNPC >( invoker ) )
	{
		if ( npc->IsSuppressingBroadcastingReactions() )
		{
			return false;
		}

		if ( npc->IsInFistFightMiniGame() )
		{
			return false;
		}
	}

	if ( m_suppressScaredReactions && IsScaryEvent( eventName ) )
	{
		return false;
	}

	if ( !m_suppressedAreas.Empty() )
	{
		const Vector& worldPos = invoker->GetWorldPositionRef();
		for ( const THandle< CAreaComponent >& areaHandle : m_suppressedAreas )
		{
			CAreaComponent* area = areaHandle.Get();
			if ( area->GetBoundingBox().Contains( worldPos ) && area->TestPointOverlap( worldPos ) )
			{
				return false;
			}
		}
	}

	return CreateReactionEvent( invoker, eventName, lifetime, distanceRange, broadcastInterval, recipientCount, false, skipInvoker, useCustomReactionCenter, reactionCenter );
}

Bool CBehTreeReactionManager::CreateReactionEvent( CEntity* invoker, CName name, Float lifetime, Float distanceRange, Float broadcastInterval, Int32 recipientCount, Bool isScene, Bool skipInvoker, Bool useCustomReactionCenter, const Vector& reactionCenter )
{ 
	PC_SCOPE( CBehTreeReactionManager_CreateReactionEvent );
	if( invoker && !name.Empty() )
	{
		CBehTreeReactionEventData* data = NULL;
		for( Uint32 i = 0; i < m_reactionEvents.Size(); ++i )
		{
			if( m_reactionEvents[i]->GetInvoker() == invoker && m_reactionEvents[i]->GetEventName() == name )
			{
				data = m_reactionEvents[i];
			}
		}

		// Create reaction data, if this is a new reaction
		if( !data )
		{
			{
				PC_SCOPE( Allocate_CBehTreeReactionEventData );
				data = CreateObject<CBehTreeReactionEventData>( this );
			}
			
			if( data )
			{
				data->InitializeData( invoker, name, lifetime, broadcastInterval, distanceRange, recipientCount, skipInvoker, useCustomReactionCenter, reactionCenter );
			}
			if( isScene )
			{
				data->SetReactionScene( CreateReactionScene( name, invoker, data ) );
			}			
		}
		else
		{
			data->UpdateRecipietCount( recipientCount );
			if( useCustomReactionCenter )
			{
				data->SetCutomReactionCenter( reactionCenter );
			}
			else
			{
				data->ResetCutomReactionCenter();
			}
		}


		// Always try to add, if it already exists it will only get updated!
		if( data && data->GetInvoker() )
		{
			return AddReactionEvent( data );
		}
	}

	return false;
}

Bool CBehTreeReactionManager::AddReactionEvent( CBehTreeReactionEventData* data )
{
	if( data )
	{
		// Don't have it? Create! Both cases - broadcast and reset expiration time
		if( !m_reactionEvents.Exist( data ) )
		{
			data->PostLoad();
			m_reactionEvents.PushBack( data );
		}

		data->SetExpirationTime(GGame->GetEngineTime()+data->GetLifetime());
		Broadcast( *data );
		data->SetBroadcastTime( GGame->GetEngineTime() + data->GetBroadcastInterval() );
		return true;
	}

	return false;
}

Bool CBehTreeReactionManager::DeleteReactionEvent( Uint32 index )
{
	if( index < m_reactionEvents.Size() )
	{
		CBehTreeReactionEventData* data = m_reactionEvents[index];
		if( data )
		{
			if( data->GetReactionScene() )
			{
				data->GetReactionScene()->Clear();
				m_reactionScens.RemoveFast( data->GetReactionScene() );
			}			
			m_reactionEvents.RemoveAtFast( index );			
			data->Discard();
			data = NULL;
			return true;
		}
	}

	return false;
}

Bool CBehTreeReactionManager::RemoveReactionEvent( CEntity* invoker, CName name )
{
	if( invoker )
	{
		for( Uint32 i = 0; i < m_reactionEvents.Size(); ++i )
		{
			if( m_reactionEvents[i]->GetInvoker() == invoker && m_reactionEvents[i]->GetEventName() == name )
			{
				return DeleteReactionEvent( i );
			}
		}
	}

	return false;
}

void CBehTreeReactionManager::AddReactionSceneGroup( const String&  voiceset, const CName group )
{
	m_reactionSceneGroups.Insert( voiceset, group );
}

void CBehTreeReactionManager::funcAddReactionSceneGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String,  voiceset, String::EMPTY );
	GET_PARAMETER( CName,  group	, CName::NONE );
	FINISH_PARAMETERS;

	AddReactionSceneGroup( voiceset, group );
}

void CBehTreeReactionManager::funcCreateReactionEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity>, invoker, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Float, lifetime, 5.0f );
	GET_PARAMETER( Float, distanceRange, 15.0f );
	GET_PARAMETER( Float, broadcastInterval, 2.0f );
	GET_PARAMETER( Int32, recipientCount, -1 );
	GET_PARAMETER( Bool,  skipInvoker, false );
	GET_PARAMETER( Bool,  setActionTargetOnBroadcast, false );
	FINISH_PARAMETERS;

	RETURN_BOOL( CreateReactionEvent( invoker.Get(), eventName, lifetime, distanceRange, broadcastInterval, recipientCount, false, skipInvoker , false, Vector() ) );
}

void CBehTreeReactionManager::funcCreateReactionEventCustomCenter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity>, invoker, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Float, lifetime, 5.0f );
	GET_PARAMETER( Float, distanceRange, 15.0f );
	GET_PARAMETER( Float, broadcastInterval, 2.0f );
	GET_PARAMETER( Int32, recipientCount, -1 );
	GET_PARAMETER( Bool,  skipInvoker, false );
	GET_PARAMETER( Bool,  setActionTargetOnBroadcast, false );
	GET_PARAMETER( Vector,customCenter, Vector() );
	FINISH_PARAMETERS;

	RETURN_BOOL( CreateReactionEvent( invoker.Get(), eventName, lifetime, distanceRange, broadcastInterval, recipientCount, false, skipInvoker , true, customCenter ) );
}

void CBehTreeReactionManager::funcRemoveReactionEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity>, invoker, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( RemoveReactionEvent( invoker.Get(), eventName ) );
}

void CBehTreeReactionManager::funcInitReactionScene( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity>, invoker, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Float, lifetime, 5.0f );
	GET_PARAMETER( Float, distanceRange, 15.0f );
	GET_PARAMETER( Float, broadcastInterval, 2.0f );
	GET_PARAMETER( Int32, recipientCount, -1 );
	FINISH_PARAMETERS;

	CReactionSceneActorComponent* actorCmp = invoker.Get()->FindComponent< CReactionSceneActorComponent >();

	if( !actorCmp )
	{
		return;
	}

	RETURN_BOOL( CreateReactionEvent( invoker.Get(), eventName, lifetime, distanceRange, broadcastInterval, recipientCount, true, false, false, Vector() ) );
}

void CBehTreeReactionManager::funcSuppressReactions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, toggle, false );
	GET_PARAMETER( CName, areaTag, CName::NONE );
	FINISH_PARAMETERS;

	SuppressReactions( toggle, areaTag );
}

void CBehTreeReactionManager::funcCreateReactionEventIfPossible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, invoker, nullptr );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Float, lifetime, 0.f );
	GET_PARAMETER( Float, distanceRange, 0.f );
	GET_PARAMETER( Float, broadcastInterval, 0.f );
	GET_PARAMETER( Int32, recipientCount, 0 );
	GET_PARAMETER( Bool, skipInvoker, false );
	GET_PARAMETER_OPT( Bool, setActionTargetOnBroadcast, false );
	GET_PARAMETER_OPT( Vector, customCenter, Vector::ZEROS );
	FINISH_PARAMETERS;

	if ( CEntity* invokerPtr = invoker.Get() )
	{
		CreateReactionEventIfPossible(invokerPtr, eventName, lifetime, distanceRange, broadcastInterval, recipientCount, skipInvoker, customCenter != Vector::ZEROS, customCenter );
	}
}
