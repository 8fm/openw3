#include "build.h"
#include "ridingAiStorage.h"
#include "r4Player.h"
#include "w3GenericVehicle.h"
#include "../../common/game/gameplayStorage.h"
#include "../../common/game/actorsManager.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/engine/behaviorGraphStack.h"

////////////////////////////////////////////////////////////////////////////////
// CHorseRiderSharedParams
IMPLEMENT_RTTI_ENUM( EVehicleMountStatus );
IMPLEMENT_RTTI_ENUM( EMountType );
IMPLEMENT_RTTI_ENUM( EDismountType );
IMPLEMENT_RTTI_ENUM( EVehicleSlot );
IMPLEMENT_RTTI_ENUM( EVehicleMountType );

IMPLEMENT_ENGINE_CLASS( CHorseRiderSharedParams );
CHorseRiderSharedParams::CHorseRiderSharedParams()
	: m_horse()
	, m_mountStatus( VMS_dismounted )
	, m_boat()
	, m_vehicleSlot( EVS_driver_slot )
{
	EnableReferenceCounting( true );

}

////////////////////////////////////////////////////////////////////////////////
// CAIStorageRiderData
IMPLEMENT_RTTI_ENUM( ERidingManagerTask );

IMPLEMENT_ENGINE_CLASS( CAIStorageRiderData );

CAIStorageRiderData::CAIStorageRiderData()
	: m_sharedParams()
	, m_ridingManagerCurrentTask( RMT_None )
	, m_ridingManagerCurrentTaskIsFromScript( false )
	, m_ridingManagerMountError( false )
	, m_horseScriptedActionTree()
	, m_ridingManagerDismountType( DT_normal )
	, m_ridingManagerInstantMount( false )
	, m_unreachableHorseList()
{
	EnableReferenceCounting( true );
	m_sharedParams = CHorseRiderSharedParams::GetStaticClass()->CreateObject< CHorseRiderSharedParams >();
}

CAIStorageRiderData::~CAIStorageRiderData()
{

}


Bool CAIStorageRiderData::CheckHorse( CActor *const horseActor, CActor *const riderActor )
{
	if ( horseActor )
	{
		CR4Player* player					= SafeCast<CR4Player>( GGame->GetPlayerEntity() );
		ComponentIterator< W3HorseComponent > it( horseActor );
		
		if ( it  )
		{
			W3HorseComponent* horseComponent	= *it;
			if (	player->GetHorseWithInventory() != horseActor
					&& !horseActor->IsLockedByScene()
					&& horseComponent->IsTamed()
					&& IsHorseReachable( horseActor, riderActor ) )
			{
				return true;
			}
		}
	}
	return false;
}
Bool CAIStorageRiderData::IsHorseReachable( CActor *const horseActor, CActor *const riderActor )
{
	if ( m_unreachableHorseList.FindPtr( horseActor ) )
	{
		return false;
	}
	return true;
}

struct HorseComparePredicate
{
	HorseComparePredicate( const Vector &origin )
		: m_origin( origin ) {}
	Vector m_origin;
	inline bool operator() ( CActor* a, CActor* b ) const
	{
		const Vector posA	= a->GetWorldPosition();
		const Vector posB	= b->GetWorldPosition();
		const Float sqDistA = (posA - m_origin).SquareMag3();
		const Float sqDistB = (posB - m_origin).SquareMag3();
		return sqDistA < sqDistB;
	}
};
void CAIStorageRiderData::QueryHorses( TDynArray< CActor* >& output, const Vector& origin, Float range, CName tag )
{
	struct Functor : public CGameplayStorage::DefaultFunctor
	{		
		Functor( TDynArray< CActor* >& output )
			: m_output( output ) {}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& data )
		{
			m_output.PushBack( data.Get() );				
			return true;
		}

		RED_INLINE Bool operator()( const TPointerWrapper< CActor >& ptr )
		{
			m_output.PushBack( ptr.Get() );				
			return true;
		}

		TDynArray< CActor* >&		m_output;		
	} functor( output );
	
	CGameplayStorage::SSearchParams searchParams;
	searchParams.m_origin	= origin;
	searchParams.m_flags	= FLAG_OnlyAliveActors;
	searchParams.m_tag		= tag;
	searchParams.m_range	= range;
	GCommonGame->GetActorsManager()->TQuery( functor, searchParams );	
	if ( output.Size() > 1 )
	{
		Sort( output.Begin(), output.End(), HorseComparePredicate( origin ) );
	}
};

CAIStorageRiderData* CAIStorageRiderData::Get( CActor* actor )
{
	CBehTreeMachine *const behTreeMachine = actor->GetBehTreeMachine();
	if ( behTreeMachine == nullptr )
	{
		return nullptr;
	}
	CBehTreeInstance *const behTreeInstance = behTreeMachine->GetBehTreeInstance();
	if ( behTreeInstance == nullptr )
	{
		return nullptr;
	}

	CAIStorageRiderData* riderData = behTreeInstance->TScriptableStoragetFindItem< CAIStorageRiderData >( CNAME( RiderData ) );
	if ( riderData == nullptr )
	{
		return nullptr;
	}
	return riderData;
}

Bool CAIStorageRiderData::PairWithTaggedHorse( CActor *const actor, CName horseTag, Float range )
{
	TDynArray< CActor * > horseArray;
	Vector position =  actor->GetWorldPosition();
	QueryHorses( horseArray, position, range, horseTag );

	for ( Uint32 i = 0; i < horseArray.Size(); i+=1 )
	{
		CActor *const horseActor 			= horseArray[i];
		ComponentIterator< W3HorseComponent > it( horseActor );

		if ( it && CheckHorse( horseActor, actor ) )
		{  
			W3HorseComponent* horseComponent	= *it;
			if ( horseComponent->PairWithRider( horseActor, m_sharedParams.Get() ) )
			{
				return true;
			}
		}
		
	}
	
	return false;
}

Bool CAIStorageRiderData::IsMounted()const
{
	CHorseRiderSharedParams *const sharedParams = m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		return false;
	}
	if ( sharedParams->m_mountStatus == VMS_dismounted )
	{
		return false;
	}
	return true;
}

void CAIStorageRiderData::funcPairWithTaggedHorse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CActor>, actorHandle, nullptr );
	GET_PARAMETER( CName, horseTag, CName::NONE );
	GET_PARAMETER( Float, range, 50.0f );
	FINISH_PARAMETERS;
	CActor *const actor = actorHandle.Get();
	if ( actor == nullptr )
	{
		RETURN_BOOL( false );
	}
	RETURN_BOOL( PairWithTaggedHorse( actor, horseTag, range ) );
}

void CAIStorageRiderData::funcOnInstantDismount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CActor>, riderActorHandle, nullptr );
	FINISH_PARAMETERS;
	CActor *const riderActor = riderActorHandle.Get();
	if( riderActor == nullptr )
	{
		return;
	}
	CAnimatedComponent* ac			= riderActor->GetRootAnimatedComponent();
	CBehaviorGraphStack* behStack	= ac ? ac->GetBehaviorStack() : nullptr;
	if ( behStack )
	{
		if ( riderActor->IsA<CPlayer>() )
		{
			behStack->ActivateAndSyncBehaviorInstances( CNAME( Gameplay ) );
		}
		else
		{
			behStack->ActivateAndSyncBehaviorInstances( CNAME( Exploration ) );
		}

		riderActor->RaiseBehaviorForceEvent( CNAME( ForceIdle ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
// CAIStorageAnimalData
IMPLEMENT_ENGINE_CLASS( CAIStorageAnimalData );


////////////////////////////////////////////////////////////////////////////////
// CAIStorageHorseData
IMPLEMENT_ENGINE_CLASS( CAIStorageHorseData );

CAIStorageHorseData::CAIStorageHorseData()
{
	EnableReferenceCounting( true );
}
