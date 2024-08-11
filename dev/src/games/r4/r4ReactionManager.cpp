#include "build.h"

#include "r4ReactionManager.h"

#include "../../common/game/binaryStorage.h"
#include "../../common/game/actorsManager.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/behTreeMachine.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeWorkData.h"

#include "../../common/engine/weatherManager.h"

#include "commonMapManager.h"
#include "r4interiorAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( CR4ReactionManager );

const Float CR4ReactionManager::RAIN_REACTION_TREESHOLD = 0.01f;
const Float CR4ReactionManager::RAIN_REACTION_DISTANCE	= 100;


Bool RainNotyfier::Notify( CNewNPC* npc )
{	
	if( npc )
	{
		Float currentDistanceSqr = ( npc->GetWorldPositionRef() - m_center ).SquareMag3();
		if( currentDistanceSqr <= m_distanceSqr ) 
		{
			CR4InteriorAreaComponent* foundInteriors[ 1 ];

			Int32 foundInteriorsAmount = 0;
			if( m_mapManager )
			{
				foundInteriorsAmount = m_mapManager->FindInteriorContainingPoint( npc->GetWorldPositionRef(), foundInteriors, 1 );
			}
			if( foundInteriorsAmount == 0 )
			{
				CBehTreeMachine* behTreeMachine = npc->GetBehTreeMachine();
				if( behTreeMachine )
				{
					CBehTreeInstance* behTreeIstance = behTreeMachine ->GetBehTreeInstance();
					if( behTreeIstance )
					{
						CBehTreeWorkData*  workData = behTreeIstance->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
						if( workData )
						{					
							npc->SignalGameplayEvent( m_rainEventData->GetEventName(), m_rainEventData, CBehTreeReactionEventData::GetStaticClass() );													
						}
					}
				}
			}
		}
	}		
	return true;
}


void CR4ReactionManager::FindActorsToBroadecast( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();

	CR4InteriorAreaComponent* foundInteriors[ MAX_INTERIORS_SEARCH ];

	Int32 foundInteriorsAmount = manager->FindInteriorContainingPoint( data.GetInvoker()->GetWorldPositionRef(), foundInteriors, MAX_INTERIORS_SEARCH );
	if( foundInteriorsAmount > 0 )
	{
		for( Int32 i = 0; i < foundInteriorsAmount; ++i )
		{		
			CR4InteriorAreaComponent* interior = foundInteriors[ i ];
			TDynArray< THandle< CActor > >& actorsInside = interior->GetActorsInside();

			const Box box				= data.GetRangeBox();
			const CEntity* invoker		= data.GetInvoker();
			const Vector& invokerPos	= invoker->GetWorldPositionRef();
			for( Int32 j = 0; j < actorsInside.SizeInt(); ++j )
			{
				CActor* actor = actorsInside[ j ].Get();
				if(		actor && ( data.GetSkipInvoker() == false || actor != invoker )
					&&	( actor->GetWorldPositionRef() - invokerPos ).SquareMag3() < data.GetDistanceRangeSqrt() )
				{
					if ( i == 0 )
					{
						actors.PushBack( actor );
					}
					else
					{
						// Interiors can overlap so no need to push the same actor twice
						actors.PushBackUnique( actor );
					}
				}
			}
		}
	}
	else
	{
		STATIC_NODE_FILTER( IsNotPlayer, filterNotPlayer );
		STATIC_NODE_FILTER( IsNotInInterior, inNotInInterior );		
		static const INodeFilter* filters[] = { &filterNotPlayer, &inNotInInterior };
		GCommonGame->GetActorsManager()->GetClosestToPoint( data.GetReactionCenter(), actors, data.GetRangeBox(), INT_MAX, filters, 2 );	
	}
}


void CR4ReactionManager::Update()
{
	UpdateRaining();
	TBaseClass::Update();
}


void CR4ReactionManager::UpdateRaining()
{	
	if( !m_rainReactionsEnabled )
	{	
		return;
	}

	CWeatherManager* weatherManager = GetWeatherManager();
	if( !weatherManager )
		return;

	Float rainStrength = weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN );
	if( rainStrength < RAIN_REACTION_TREESHOLD )
	{
		if( m_rainEventParams.Get() )
		{
			m_rainEventParams->Discard();
			m_rainEventParams = nullptr;
		}			
		return;
	}
	
	CBehTreeReactionEventData* rainEvent = m_rainEventParams.Get();
	if( !rainEvent )
	{
		rainEvent = CreateObject<CBehTreeReactionEventData>( this );

		rainEvent->InitializeData( GGame->GetPlayerEntity(), CNAME( RainReactionEvent ), -1, 0, RAIN_REACTION_DISTANCE, 100, true, false, Vector3( 0, 0 ,0 ) );		
		rainEvent->PostLoad();
		m_rainEventParams = rainEvent;
	}		
	
	Float currTime = GGame->GetEngineTime();
	if( rainEvent->GetBroadcastTime() <= currTime )
	{//for interval == -1 broadcast was fired when event was created
	
		if( !rainEvent->GetInvoker() )
		{
			rainEvent->SetInvoker( GGame->GetPlayerEntity() );
		}

		CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
		
		RainNotyfier rainNotyfier( rainEvent, rainEvent->GetInvoker()->GetWorldPositionRef(),  100, manager );

		for( Uint32 i=0; i<m_rainAwareNPCs.Size(); ++i )
		{
			CNewNPC* npc = m_rainAwareNPCs[ i ].Get();
			if( npc )
			{
				rainNotyfier.Notify( npc );
			}
			else
			{
				m_rainAwareNPCs.RemoveAt( i );
				--i;
			}
		}

		rainEvent->SetBroadcastTime( currTime + 5 );
	}
}

CWeatherManager* CR4ReactionManager::GetWeatherManager()
{
	if( CWorld* world = GGame->GetActiveWorld() )
	{
		if( CEnvironmentManager* envManager = world->GetEnvironmentManager() )
		{
			return envManager->GetWeatherManager();
		}
	}
	return nullptr;
}
