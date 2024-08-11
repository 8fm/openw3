#include "build.h"
#include "behTreeRidingManager.h"

#include "../../common/game/encounter.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeNodePlayScene.h"

#include "w3GenericVehicle.h"
#include "r4CreateEntityManager.h"
#include "r4Player.h"
#include "dynamicTagsContainer.h"

IMPLEMENT_ENGINE_CLASS( SBehTreePairHorseEventParam )

///////////////////////////////////////////////////////////// 
// CBehTreeDecoratorRidingManagerDefinition
CBehTreeDecoratorRidingManagerDefinition::CBehTreeDecoratorRidingManagerDefinition() 
	: m_child( nullptr ) 
	, m_mountHorseChild( nullptr )
	, m_dismountHorseChild( nullptr )
	, m_mountBoatChild( nullptr )
	, m_dismountBoatChild( nullptr )
{

}
IBehTreeNodeInstance* CBehTreeDecoratorRidingManagerDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	if ( !m_child || !m_mountHorseChild || !m_dismountHorseChild || !m_mountBoatChild || !m_dismountBoatChild )
	{
		return nullptr;
	}

	IBehTreeNodeInstance* child					= m_child->SpawnInstance( owner, context );
	IBehTreeNodeInstance* mountHorseChild		= m_mountHorseChild->SpawnInstance( owner, context );
	IBehTreeNodeInstance* dismountHorseChild	= m_dismountHorseChild->SpawnInstance( owner, context );
	IBehTreeNodeInstance* mountBoatChild		= m_mountBoatChild->SpawnInstance( owner, context );
	IBehTreeNodeInstance* dismountBoatChild		= m_dismountBoatChild->SpawnInstance( owner, context );

	if ( !mountHorseChild || !dismountHorseChild || !mountBoatChild || !dismountBoatChild )
	{
		return nullptr;
	}
	
	Instance* me				= new Instance( *this, owner, context, parent );
	me->m_child					= child;
	me->m_mountHorseChild		= mountHorseChild;
	me->m_dismountHorseChild	= dismountHorseChild;
	me->m_mountBoatChild		= mountBoatChild;
	me->m_dismountBoatChild		= dismountBoatChild;
	child->SetParent( me );
	mountHorseChild->SetParent( me );
	dismountHorseChild->SetParent( me );
	mountBoatChild->SetParent( me );
	dismountBoatChild->SetParent( me );
	return me;
}


Bool CBehTreeDecoratorRidingManagerDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeDecoratorRidingManagerDefinition::IsValid() const
{
	if ( !m_child || !m_mountHorseChild || !m_dismountHorseChild || !m_mountBoatChild || !m_dismountBoatChild )
	{
		return false;
	}
	return TBaseClass::IsValid();
}

Bool CBehTreeDecoratorRidingManagerDefinition::CanAddChild() const
{
	return !m_child || !m_mountHorseChild || !m_dismountHorseChild || !m_mountBoatChild || !m_dismountBoatChild;
}
void CBehTreeDecoratorRidingManagerDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( node == m_child )
	{
		m_child					= m_mountHorseChild;
		m_mountHorseChild		= m_dismountHorseChild;
		m_dismountHorseChild	= m_mountBoatChild;
		m_mountBoatChild		= m_dismountBoatChild;
		m_dismountBoatChild		= nullptr;
	}
	else if ( node == m_mountHorseChild )
	{
		m_mountHorseChild		= m_dismountHorseChild;
		m_dismountHorseChild	= m_mountBoatChild;
		m_mountBoatChild		= m_dismountBoatChild;
		m_dismountBoatChild		= nullptr;
	}
	else if ( node == m_dismountHorseChild )
	{
		m_dismountHorseChild	= m_mountBoatChild;
		m_mountBoatChild		= m_dismountBoatChild;
		m_dismountBoatChild		= nullptr;
	}
	else if ( node == m_mountBoatChild )
	{
		m_mountBoatChild		= m_dismountBoatChild;
		m_dismountBoatChild		= nullptr;
	}
	else if ( node == m_dismountBoatChild )
	{
		m_dismountBoatChild		= nullptr;
	}
}
Int32 CBehTreeDecoratorRidingManagerDefinition::GetNumChildren() const
{
	if ( m_child == nullptr )
	{
		return 0;
	}
	if ( m_mountHorseChild == nullptr )
	{
		return 1;
	}
	if ( m_dismountHorseChild == nullptr )
	{
		return 2;
	}
	if ( m_mountBoatChild == nullptr )
	{
		return 3;
	}
	if ( m_dismountBoatChild == nullptr )
	{
		return 4;
	}
	return 5;
}
IBehTreeNodeDefinition* CBehTreeDecoratorRidingManagerDefinition::GetChild( Int32 index ) const
{
	ASSERT( index < 5 );
	switch ( index )
	{
	case 0:
		return m_child;
	case 1:
		return m_mountHorseChild;
	case 2:
		return m_dismountHorseChild;
	case 3:
		return m_mountBoatChild;
	case 4:
		return m_dismountBoatChild;
	}
	return nullptr;
}
void CBehTreeDecoratorRidingManagerDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	ASSERT( node->GetParent() == this );
	ASSERT( m_dismountBoatChild == nullptr );
	if ( m_child == nullptr )
	{
		m_child = node;
	}
	else if ( m_mountHorseChild == nullptr )
	{
		m_mountHorseChild = node;
	}
	else if ( m_dismountHorseChild == nullptr )
	{
		m_dismountHorseChild = node;
	}
	else if ( m_mountBoatChild == nullptr )
	{
		m_mountBoatChild = node;
	}
	else
	{
		m_dismountBoatChild = node;
	}
}

void CBehTreeDecoratorRidingManagerDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeDecoratorRidingManagerDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
	if ( m_mountHorseChild )
	{
		m_mountHorseChild->CollectNodes( nodes );
	}
	if ( m_dismountHorseChild )
	{
		m_dismountHorseChild->CollectNodes( nodes );
	}
	if ( m_mountBoatChild )
	{
		m_mountBoatChild->CollectNodes( nodes );
	}
	if ( m_dismountBoatChild )
	{
		m_dismountBoatChild->CollectNodes( nodes );
	}
	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CBehTreeDecoratorRidingManagerDefinition::CorrectChildrenOrder()
{
	Bool result = false;
	if ( m_child && m_mountHorseChild )
	{
		if ( m_mountHorseChild->GetGraphPosX() < m_child->GetGraphPosX() )
		{
			Swap( m_child, m_mountHorseChild );
			result = true;
		}
	}
	if ( m_mountHorseChild && m_dismountHorseChild )
	{
		if ( m_dismountHorseChild->GetGraphPosX() < m_mountHorseChild->GetGraphPosX() )
		{
			Swap( m_mountHorseChild, m_dismountHorseChild );
			result = true;
		}
	}
	if ( m_dismountHorseChild && m_mountBoatChild )
	{
		if ( m_mountBoatChild->GetGraphPosX() < m_dismountHorseChild->GetGraphPosX() )
		{
			Swap( m_dismountHorseChild, m_mountBoatChild );
			result = true;
		}
	}
	if ( m_mountBoatChild && m_dismountBoatChild )
	{
		if ( m_dismountBoatChild->GetGraphPosX() < m_mountBoatChild->GetGraphPosX() )
		{
			Swap( m_mountBoatChild, m_dismountBoatChild );
			result = true;
		}
	}
	return TBaseClass::CorrectChildrenOrder() || result;
}
#endif

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRidingManagerInstance
CBehTreeDecoratorRidingManagerInstance::CBehTreeDecoratorRidingManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent ) 
	, m_child( nullptr )
	, m_mountHorseChild( nullptr )
	, m_dismountHorseChild( nullptr )
	, m_mountBoatChild( nullptr )
	, m_dismountBoatChild( nullptr )
	, m_riderData( owner, CNAME( RiderData ) )
	, m_nextTask( RMT_None )
	, m_nextTaskDismountType( DT_normal )
	, m_nextTaskIsFromScript( false )
	, m_wasMountedWhenSaving( false )
	, m_vehicleSlot_save( EVS_driver_slot )
	, m_wasMountedOnBoatWhenSaving( false )
	, m_loadFailTimeout( 0.0f )
	, m_tryFindVehicleTimeout( -1.0f )
{
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerMountHorse );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerDismountHorse );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerMountBoat );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerDismountBoat );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = SPlaySceneRequestData::EVENT_ID;
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
	
}
CBehTreeDecoratorRidingManagerInstance::~CBehTreeDecoratorRidingManagerInstance()
{
	delete m_child;
	delete m_mountHorseChild;
	delete m_dismountHorseChild;
	delete m_mountBoatChild;
	delete m_dismountBoatChild;
}

void CBehTreeDecoratorRidingManagerInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerMountHorse );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerDismountHorse );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerMountBoat );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( RidingManagerDismountBoat );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = SPlaySceneRequestData::EVENT_ID;
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}

	m_mountHorseChild->OnDestruction();
	m_dismountHorseChild->OnDestruction();
	m_mountBoatChild->OnDestruction();
	m_dismountBoatChild->OnDestruction();
	m_child->OnDestruction();

	Super::OnDestruction();

	
}

void CBehTreeDecoratorRidingManagerInstance::Update()
{
	if ( m_child->IsActive() )
	{
		m_child->Update();
	}

	CAIStorageRiderData& riderData = *m_riderData;
	CHorseRiderSharedParams *const sharedParams = riderData.m_sharedParams.Get();
	// This needs to be done here because player doesn't have Pairing Logic Node
	sharedParams->m_rider = m_owner->GetActor();

	switch ( riderData.m_ridingManagerCurrentTask )
	{
	case RMT_MountHorse:
		if ( m_mountHorseChild->IsActive() == false )
		{
			m_mountHorseChild->Activate();
		}
		break;
	case RMT_DismountHorse:
		if ( m_dismountHorseChild->IsActive() == false )
		{
			m_dismountHorseChild->Activate();
		}
		break;
	case RMT_MountBoat:
		if ( m_mountBoatChild->IsActive() == false )
		{
			m_mountBoatChild->Activate();
		}
		break;
	case RMT_DismountBoat:
		if ( m_dismountBoatChild->IsActive() == false )
		{
			m_dismountBoatChild->Activate();
		}
		break;
	case RMT_None:
		if ( m_mountHorseChild->IsActive() )
		{
			m_mountHorseChild->Deactivate();
		}
		if ( m_dismountHorseChild->IsActive() )
		{
			m_dismountHorseChild->Deactivate();
		}
		if ( m_mountBoatChild->IsActive() )
		{
			m_mountBoatChild->Deactivate();
		}
		if ( m_dismountBoatChild->IsActive() )
		{
			m_dismountBoatChild->Deactivate();
		}
		break;
	}

	if ( m_mountHorseChild->IsActive() )
	{
		m_mountHorseChild->Update();
	}
	if ( m_dismountHorseChild->IsActive() )
	{
		m_dismountHorseChild->Update();
	}
	if ( m_mountBoatChild->IsActive() )
	{
		m_mountBoatChild->Update();
	}
	if ( m_dismountBoatChild->IsActive() )
	{
		m_dismountBoatChild->Update();
	}

	UpdatePlayerLoadSave();
	UpdateSound();
}
Bool CBehTreeDecoratorRidingManagerInstance::Activate()
{
	if ( m_child->Activate() == false )
	{
		DebugNotifyActivationFail();
		return false;
	}
	return Super::Activate();
}
void CBehTreeDecoratorRidingManagerInstance::Deactivate()
{
	if ( m_child->IsActive() )
	{
		m_child->Deactivate();
	}
	if ( m_mountHorseChild->IsActive() )
	{
		m_mountHorseChild->Deactivate();
	}
	if ( m_dismountHorseChild->IsActive() )
	{
		m_dismountHorseChild->Deactivate();
	}
	if ( m_mountBoatChild->IsActive() )
	{
		m_mountBoatChild->Deactivate();
	}
	if ( m_dismountBoatChild->IsActive() )
	{
		m_dismountBoatChild->Deactivate();
	}
	// Cancel any ongoing tasks
	// If the entity is pooled we need it clean when we reactivate
	m_riderData->m_ridingManagerCurrentTask = RMT_None;
	m_nextTask = RMT_None;

	Super::Deactivate();
}

void CBehTreeDecoratorRidingManagerInstance::OnSubgoalCompleted( eTaskOutcome outcome ) 
{
	m_riderData->m_ridingManagerCurrentTask				= RMT_None;
	m_riderData->m_ridingManagerCurrentTaskIsFromScript	= false;
	if ( m_nextTask != RMT_None )
	{
		m_riderData->m_ridingManagerCurrentTask 			= m_nextTask;
		m_riderData->m_ridingManagerDismountType			= m_nextTaskDismountType;
		m_riderData->m_ridingManagerCurrentTaskIsFromScript	= m_nextTaskIsFromScript;

		m_nextTask 						= RMT_None;
		m_nextTaskIsFromScript			= false;
		
	}
	if ( m_child->IsActive() == false )
	{
		m_child->Activate();
	}
}
Bool CBehTreeDecoratorRidingManagerInstance::OnEvent( CBehTreeEvent& e )
{
	Bool b0 = m_child->IsActive() ?					m_child->OnEvent( e )				: false;
	Bool b1 = m_mountHorseChild->IsActive() ?		m_mountHorseChild->OnEvent( e )		: false;
	Bool b2 = m_dismountHorseChild->IsActive() ?	m_dismountHorseChild->OnEvent( e )	: false;
	Bool b3 = m_mountBoatChild->IsActive() ?		m_mountBoatChild->OnEvent( e )		: false;
	Bool b4 = m_dismountBoatChild->IsActive() ?		m_dismountBoatChild->OnEvent( e )	: false;
	return b0 || b1 || b2 || b3 || b4;
}

Bool CBehTreeDecoratorRidingManagerInstance::OnListenedEvent( CBehTreeEvent& e )
{
	Bool b0 = m_child->OnListenedEvent( e );
	Bool b1 = m_mountHorseChild->OnListenedEvent( e );
	Bool b2 = m_dismountHorseChild->OnListenedEvent( e );
	Bool b3 = m_mountBoatChild->OnListenedEvent( e );
	Bool b4 = m_dismountBoatChild->OnListenedEvent( e );

	if ( m_riderData )
	{
		CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
		if ( sharedParams )
		{
			if ( e.m_eventType == BTET_GameplayEvent )
			{
				// Mounting event :
				if ( e.m_eventName == CNAME( RidingManagerMountHorse ) )
				{
					SGameplayEventParamInt* params				= e.m_gameplayEventData.Get< SGameplayEventParamInt >();
					const Bool isInstantMount = params && ( params->m_value & MT_instant );
					if ( m_riderData->m_ridingManagerCurrentTask == RMT_None )
					{
						// mount only if we are dismounted
						if ( sharedParams->m_mountStatus == VMS_dismounted )
						{
							m_riderData->m_ridingManagerMountError 				= false;
							m_riderData->m_ridingManagerCurrentTask 			= RMT_MountHorse; 
							m_riderData->m_ridingManagerCurrentTaskIsFromScript	= params && ( params->m_value & MT_fromScript );
							m_riderData->m_ridingManagerInstantMount			= isInstantMount;
						}
						m_nextTask						= RMT_None;
						m_nextTaskIsFromScript			= false;
					}
					else if ( m_riderData->m_ridingManagerCurrentTask == RMT_DismountHorse )
					{
						m_nextTask									= RMT_MountHorse;
						m_nextTaskIsFromScript						= params && ( params->m_value & MT_fromScript );
						m_riderData->m_ridingManagerInstantMount	= isInstantMount;
					}
					else // riderData.ridingManagerCurrentTask == RMT_MountHorse 
					{
						m_nextTask				= RMT_None; // canceling potential dismount
						m_nextTaskIsFromScript	= false;

						// Instant must always prevail
						m_riderData->m_ridingManagerInstantMount	= m_riderData->m_ridingManagerInstantMount || isInstantMount;
					}					
				}
				// dismount event :
				else if ( e.m_eventName == CNAME( RidingManagerDismountHorse ) )
				{
					SGameplayEventParamInt* params		= e.m_gameplayEventData.Get< SGameplayEventParamInt >();
					const EDismountType dismountType	= params ? (EDismountType) ( params->m_value & ~DT_fromScript ) : DT_normal;
					
					// treating message 
					if ( m_riderData->m_ridingManagerCurrentTask == RMT_None )
					{
						// dismount only if we are mounted
						if ( sharedParams->m_mountStatus == VMS_mounted )
						{
							m_riderData->m_ridingManagerCurrentTask 			= RMT_DismountHorse;
							m_riderData->m_ridingManagerDismountType			= dismountType;
							m_riderData->m_ridingManagerCurrentTaskIsFromScript	= params && ( params->m_value & DT_fromScript );
						}
						m_nextTask						= RMT_None;
						m_nextTaskIsFromScript			= false;
						
					}
					else if ( m_riderData->m_ridingManagerCurrentTask == RMT_MountHorse )
					{
						m_nextTask				= RMT_DismountHorse;
						m_nextTaskDismountType	= dismountType;
						m_nextTaskIsFromScript	= params && ( params->m_value & MT_fromScript );
					}
					else // riderData.ridingManagerCurrentTask == RMT_DismountHorse 
					{
						m_nextTask				= RMT_None;		// canceling potential mount
						m_nextTaskIsFromScript	= false;
					}

					// If dismount is instant then bypassing the system because it needs to be done in one frame
					if ( dismountType == DT_instant )
					{
						// Setting things to none will deactivate stuff
						m_riderData->m_ridingManagerCurrentTask = RMT_None;
						m_nextTask								= RMT_None;
						GetOwner()->GetActor()->SignalGameplayEvent( CNAME( RequestInstantDismount ) );
					}
				}
				else if ( e.m_eventName == CNAME( RidingManagerMountBoat ) )
				{
					SGameplayEventParamInt* params				= e.m_gameplayEventData.Get< SGameplayEventParamInt >();
					if ( m_riderData->m_ridingManagerCurrentTask == RMT_None )
					{
						// mount only if we are dismounted
						if ( sharedParams->m_mountStatus == VMS_dismounted )
						{
							m_riderData->m_ridingManagerMountError 				= false;
							m_riderData->m_ridingManagerCurrentTask 			= RMT_MountBoat; 
							m_riderData->m_ridingManagerCurrentTaskIsFromScript	= params && ( params->m_value & MT_fromScript );
						}
						m_nextTask						= RMT_None;
						m_nextTaskIsFromScript			= false;
					}
					else if ( m_riderData->m_ridingManagerCurrentTask == RMT_DismountBoat )
					{
						m_nextTask				= RMT_MountBoat;
						m_nextTaskIsFromScript	= params && ( params->m_value & MT_fromScript );
					}
					else // riderData.ridingManagerCurrentTask == RMT_MountBoat 
					{
						ASSERT( m_riderData->m_ridingManagerCurrentTask == RMT_MountBoat, TXT("horsemen should not use boats!") );
						m_nextTask				= RMT_None; // canceling potential dismount
						m_nextTaskIsFromScript	= false;
					}
					// Is instant mount
					m_riderData->m_ridingManagerInstantMount	= params && ( params->m_value & MT_instant );
				}
				// dismount event :
				else if ( e.m_eventName == CNAME( RidingManagerDismountBoat ) )
				{
					SGameplayEventParamInt* params		= e.m_gameplayEventData.Get< SGameplayEventParamInt >();
					const EDismountType dismountType	= params ? (EDismountType) ( params->m_value & ~DT_fromScript ) : DT_normal;
					
					// treating message 
					if ( m_riderData->m_ridingManagerCurrentTask == RMT_None )
					{
						// dismount only if we are mounted
						if ( sharedParams->m_mountStatus == VMS_mounted )
						{
							m_riderData->m_ridingManagerCurrentTask 			= RMT_DismountBoat;
							m_riderData->m_ridingManagerDismountType			= dismountType;
							m_riderData->m_ridingManagerCurrentTaskIsFromScript	= params && ( params->m_value & DT_fromScript );
						}
						m_nextTask						= RMT_None;
						m_nextTaskIsFromScript			= false;
						
					}
					else if ( m_riderData->m_ridingManagerCurrentTask == RMT_MountBoat )
					{
						m_nextTask				= RMT_DismountBoat;
						m_nextTaskDismountType	= dismountType;
						m_nextTaskIsFromScript	= params && ( params->m_value & MT_fromScript );
					}
					else // riderData.ridingManagerCurrentTask == RMT_DismountBoat 
					{
						ASSERT( m_riderData->m_ridingManagerCurrentTask == RMT_DismountBoat, TXT("horsemen should not use boats!") );
						m_nextTask				= RMT_None;		// canceling potential mount
						m_nextTaskIsFromScript	= false;
					}
				}
				else if ( e.m_eventName == SPlaySceneRequestData::EVENT_ID )
				{
					// Setting things to none will deactivate stuff
					m_riderData->m_ridingManagerCurrentTask = RMT_None;
					m_nextTask								= RMT_None;
					GetOwner()->GetActor()->SignalGameplayEvent( CNAME( RequestInstantDismount ) );
				}
			}
		}
	}
	return b0 || b1 || b2 || b3 || b4;
}


Int32 CBehTreeDecoratorRidingManagerInstance::GetNumChildren() const
{
	return 5;
}
IBehTreeNodeInstance* CBehTreeDecoratorRidingManagerInstance::GetChild( Int32 index ) const
{
	switch ( index )
	{
	case 0:
		return m_child;
	case 1:
		return m_mountHorseChild;
	case 2:
		return m_dismountHorseChild;
	case 3:
		return m_mountBoatChild;
	case 4:
		return m_dismountBoatChild;
	}
	return nullptr;
}

Uint32 CBehTreeDecoratorRidingManagerInstance::GetActiveChildCount() const
{
	return 5;
}
IBehTreeNodeInstance* CBehTreeDecoratorRidingManagerInstance::GetActiveChild( Uint32 activeChild ) const
{
	switch ( activeChild )
	{
	case 0:
		return m_child;
	case 1:
		return m_mountHorseChild;
	case 2:
		return m_dismountHorseChild;
	case 3:
		return m_mountBoatChild;
	case 4:
		return m_dismountBoatChild;
	}
	return nullptr;
}

void CBehTreeDecoratorRidingManagerInstance::UpdateSound()
{
	// Updating distance to player
	CR4Player* player	= Cast< CR4Player >( GCommonGame->GetPlayer() );
	CActor * riderActor = GetOwner()->GetActor();

	if ( player == riderActor )
	{
		return;
	}
	CHorseRiderSharedParams *const sharedparams		= m_riderData->m_sharedParams.Get();
	if ( sharedparams == nullptr || sharedparams->m_mountStatus != VMS_mounted )
	{
		return;
	}

	const Vector riderWorldPosition		= riderActor->GetWorldPosition();
	const Vector playerWorldPosition	= player->GetWorldPosition();
	const Float distanceFromPlayer		= ( playerWorldPosition - riderWorldPosition ).Mag3();

	CAnimatedComponent* animatedComponent			= riderActor->GetRootAnimatedComponent();
	CSoundEmitterComponent* soundEmitterComponent	= riderActor->GetSoundEmitterComponent();
	
	if ( animatedComponent == nullptr || soundEmitterComponent == nullptr )
	{
		return;
	}
	Int32 boneId = animatedComponent->FindBoneByName( CNAME( head ) );
	soundEmitterComponent->SoundParameter( UNICODE_TO_ANSI(TXT("vo_horse_distance") ), distanceFromPlayer, 0.0f, boneId );
}

void CBehTreeDecoratorRidingManagerInstance::UpdatePlayerLoadSave()
{
	if ( m_wasMountedWhenSaving )
	{
		CActor *const actor = GetOwner()->GetActor();
		if ( actor->IsA<CR4Player>() )
		{
			CR4Player *const player = static_cast< CR4Player * >( actor );
		
			// We must detect if player is mounted to set m_wasMountedWhenSaving to false
			// because calling MountVehicle is not enough ! ( call can fail )
			CHorseRiderSharedParams *const sharedParams = m_riderData == false ? nullptr : m_riderData->m_sharedParams.Get();
			const bool isMounted						= sharedParams == nullptr ? true : (sharedParams->m_mountStatus == VMS_mounted || sharedParams->m_mountStatus == VMS_mountInProgress );
	
			if ( isMounted )
			{
				m_wasMountedWhenSaving = false;
			}

			if ( m_tryFindVehicleTimeout < GetOwner()->GetLocalTime() )
			{
				
				// Next call to this will be in 1 sec so the mounting has enough time to happen
				if ( m_wasMountedWhenSaving && player->GetCurrentStateName() != CNAME( MountHorse ) && player->GetCurrentStateName() != CNAME( MountBoat )  )
				{
					m_tryFindVehicleTimeout = GetOwner()->GetLocalTime() + 1.0f; // try again in 1 second
					if ( m_wasMountedOnBoatWhenSaving )
					{
						CEntity *const boatEntity = sharedParams->m_boat.Get();
						if ( boatEntity )
						{
							CallFunction( player, CNAME( MountVehicle ), THandle< CEntity >( boatEntity ), VMT_ImmediateUse, m_vehicleSlot_save );
						}
					}
					else
					{
						TDynArray< CActor* > output;
						CAIStorageRiderData::QueryHorses( output, actor->GetWorldPositionRef(), 10.f, PLAYER_LAST_MOUNTED_VEHICLE_TAG );
						if ( output.Empty() )
						{
							CAIStorageRiderData::QueryHorses( output, actor->GetWorldPositionRef(), 10.f, PLAYER_HORSE_TAG );
						}
						
						if ( !output.Empty() )
						{
							CallFunction( player, CNAME( MountVehicle ), THandle< CEntity >( output[0] ), VMT_ImmediateUse, m_vehicleSlot_save );
						}
					}
				}
			}
		}
		if ( m_loadFailTimeout < GetOwner()->GetLocalTime() )
		{
			m_wasMountedWhenSaving = false; 
		}
	}
}


void CBehTreeDecoratorRidingManagerInstance::SaveState( IGameSaver* writer )
{
	CHorseRiderSharedParams *const sharedParams = m_riderData == false ? nullptr : m_riderData->m_sharedParams.Get();
	const bool wasMountedWhenSaving				= sharedParams == nullptr ? false : (sharedParams->m_mountStatus == VMS_mounted || sharedParams->m_mountStatus == VMS_mountInProgress );
	const EVehicleSlot vehicleSlot				= sharedParams == nullptr ? EVS_driver_slot : sharedParams->m_vehicleSlot;
	writer->WriteValue< Bool >( CNAME( wasMountedWhenSaving ), wasMountedWhenSaving );
	writer->WriteValue< EVehicleSlot >( CNAME( vehicleSlot_save ), vehicleSlot );
	writer->WriteValue< Bool >( CNAME( wasMountedOnBoatWhenSaving ), sharedParams->m_boat.Get() == nullptr ? false : true );
	writer->WriteValue< EntityHandle >( CNAME( pairedBoat_save ), sharedParams->m_boat );
}

Bool CBehTreeDecoratorRidingManagerInstance::LoadState( IGameLoader* reader )
{
	CHorseRiderSharedParams *const sharedParams = m_riderData == false ? nullptr : m_riderData->m_sharedParams.Get();

	reader->ReadValue< Bool >( CNAME( wasMountedWhenSaving ), m_wasMountedWhenSaving );
	reader->ReadValue< EVehicleSlot >( CNAME( vehicleSlot_save ), m_vehicleSlot_save );
	reader->ReadValue< Bool >( CNAME( wasMountedOnBoatWhenSaving ), m_wasMountedOnBoatWhenSaving );	
	reader->ReadValue< EntityHandle >( CNAME( pairedBoat_save ), sharedParams->m_boat );

	m_loadFailTimeout = GetOwner()->GetLocalTime() + 5.0f;
	return true;
}

Bool CBehTreeDecoratorRidingManagerInstance::IsSavingState() const
{
	CActor *const actor = GetOwner()->GetActor();
	if ( actor->IsA<CR4Player>() )
	{
		return true;
	}
	return false;
}



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorRidingCheckDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeDecoratorRidingCheckDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRidingCheckInstance
////////////////////////////////////////////////////////////////////////
CBehTreeDecoratorRidingCheckInstance::CBehTreeDecoratorRidingCheckInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_riderData( owner, CNAME( RiderData ) )
	, m_riderActionFromMyBranch( false )
{
	
}
void  CBehTreeDecoratorRidingCheckInstance::Deactivate()
{
	m_riderActionFromMyBranch = false;
	Super::Deactivate();
}

Bool CBehTreeDecoratorRidingCheckInstance::ConditionCheck()
{
	// needed here because the branch that requests the mount/dismount must stay active
	if ( m_riderActionFromMyBranch || m_isActive )
	{
		return true;
	}

	if ( m_riderData == false )
	{
		return true;
	}

	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		return true;
	}
	if ( sharedParams->m_horse.Get() == nullptr )
	{
		return true;
	}
	if ( m_riderData->m_ridingManagerCurrentTask != RMT_None && m_riderData->m_ridingManagerCurrentTaskIsFromScript == false )
	{
		return false;
	}
	else
	{
		// No riding sync anims we are good to go
		if ( m_isActive )
		{
			m_riderActionFromMyBranch = true;
		}
		return true;
	}
}

Bool CBehTreeDecoratorRidingCheckInstance::IsAvailable()
{
	if ( !ConditionCheck() )
	{
		DebugNotifyAvailableFail();
		return false;
	}
	return Super::IsAvailable();
}

Int32 CBehTreeDecoratorRidingCheckInstance::Evaluate()
{
	if ( !ConditionCheck() )
	{
		DebugNotifyAvailableFail();
		return -1;
	}
	return Super::Evaluate();
}


/////////////////////////////////////////////////////////////
// CBehTreeDecoratorDismountCheckDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorDismountCheckDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorDismountCheckInstance
CBehTreeDecoratorDismountCheckInstance::CBehTreeDecoratorDismountCheckInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_riderData( owner, CNAME( RiderData ) )
	, m_dismountType( def.m_dismountType )
{

}
Bool CBehTreeDecoratorDismountCheckInstance::Check()
{
	if ( m_riderData == false )
	{
		m_isDismounting = false;
		return true;
	}
	const CAIStorageRiderData & riderData = *m_riderData;
	CHorseRiderSharedParams *const sharedParams = riderData.m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		m_isDismounting = false;
		return true;
	}

	// HACK for boat 
	// If npc is using the boat the logic doesn't apply
	if ( sharedParams->m_boat.Get() )
	{
		return true;
	}

	if ( riderData.m_ridingManagerCurrentTask == RMT_None && sharedParams->m_mountStatus == VMS_dismounted )
	{
		m_isDismounting = false;
		return true;
	}
	else // sharedParams->m_mountStatus == VMS_mounted  || sharedParams->m_mountStatus == VMS_mountInProgress || sharedParams->m_mountStatus == VMS_dismountInProgress 
	{
		if (	riderData.m_ridingManagerCurrentTask != RMT_DismountHorse  // Do not resend the message if actor is already dismounting
			&&	riderData.m_ridingManagerCurrentTask != RMT_MountHorse )   // and wait for any mounting to finish before sending dismount ( because right after the mounting a rider scripted action might be triggered )
		{
			// Ordering dismount :
			m_owner->GetActor()->SignalGameplayEvent( CNAME( RidingManagerDismountHorse ), m_dismountType );
			m_isDismounting = true;
		}		
		return false;
	}
}
Bool CBehTreeDecoratorDismountCheckInstance::Activate()
{
	if ( Check() )
	{
		return Super::Activate();
	}
	else
	{
		// Calling IBehTreeNodeInstance to avoid activating child
		return IBehTreeNodeInstance::Activate();
	}
}

void CBehTreeDecoratorDismountCheckInstance::Update()
{
	// If we are not dismounted and updating then stop everything
	if ( Check() == false )
	{
		Complete( BTTO_SUCCESS );
		return;
	}

	if ( m_isDismounting )
	{
		// Does this ever happen ?
		CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
		if ( sharedParams->m_mountStatus != VMS_dismounted )
		{
			return;
		}
		
		m_isDismounting = false;
	}

	// If we are dismounted and child is not active activate child !
	if ( m_child->IsActive() == false )
	{
		if ( m_child->Activate() == false )
		{
			Complete( BTTO_FAILED );
			return;
		}
	}
	Super::Update();
}

Bool CBehTreeDecoratorDismountCheckInstance::OnEvent( CBehTreeEvent& e )
{
	if( m_isDismounting )
	{
		return false;
	}

	return Super::OnEvent( e );
}


/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairingLogicDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorRiderPairingLogicDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairingLogicInstance
CBehTreeDecoratorRiderPairingLogicInstance::CBehTreeDecoratorRiderPairingLogicInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_preferedHorseTag( def.m_preferedHorseTag.GetVal( context ) )
	, m_range( def.m_range )
	, m_riderData( owner, CNAME( RiderData ) )
	, m_timeToNextUpdate( 0.0f )
	, m_updateFreq( 0.25f )
	, m_horseNeeded( false )
	, m_createEntityHelperHandle( )
	, m_wasMountedWhenSaving( false )
	, m_loadFailTimeout( 0.0f )
{
	m_encounter = context.GetVal< CEncounter* >( CNAME( encounter ), NULL );

	{
		SBehTreeEvenListeningData eventListenerMount;
		eventListenerMount.m_eventName = CNAME( RidingManagerMountHorse );
		eventListenerMount.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListenerMount, this );
	}
	{
		SBehTreeEvenListeningData eventListenerHorseNeeded;
		eventListenerHorseNeeded.m_eventName = CNAME( HorseNeeded );
		eventListenerHorseNeeded.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListenerHorseNeeded, this );
	}
	{
		SBehTreeEvenListeningData eventListenerHorseMounted;
		eventListenerHorseMounted.m_eventName = CNAME( HorseMountEnd );
		eventListenerHorseMounted.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListenerHorseMounted, this );
	}
}

void CBehTreeDecoratorRiderPairingLogicInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData eventListenerMount;
		eventListenerMount.m_eventName = CNAME( RidingManagerMountHorse );
		eventListenerMount.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListenerMount, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( HorseNeeded );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( HorseMountEnd );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListener, this );
	}
	Super::OnDestruction();
}

Bool CBehTreeDecoratorRiderPairingLogicInstance::Activate()
{
	//m_riderData->m_despawning = false;
	return Super::Activate();
}
void CBehTreeDecoratorRiderPairingLogicInstance::Deactivate()
{
	if ( m_riderData == false )
	{
		Super::Deactivate();
		return;
	}
	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		Super::Deactivate();
		return;
	}
	// On deactivate don't forget to unpair :
	CActor * horseActor		= sharedParams->m_horse.Get();
	if ( horseActor )
	{
		ComponentIterator< W3HorseComponent > it( horseActor );
		if ( it  )
		{
			W3HorseComponent* horseComponent	= *it;

			horseComponent->Unpair();
		}
	}
	Super::Deactivate();
}
void CBehTreeDecoratorRiderPairingLogicInstance::Update()
{
	const Float & localTime = GetOwner()->GetLocalTime();
	if ( m_timeToNextUpdate > localTime )
	{
		Super::Update();
		return;
	}
	m_timeToNextUpdate = localTime + m_updateFreq;
	if ( m_riderData == false )
	{
		Super::Update();
		return;
	}

	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		Super::Update();
		return;
	}

	// [ Step 1 ] if npc has a horse check if it is still valid and unpair if necessary
	CActor *const actor		= m_owner->GetActor();
	CActor * horseActor		= sharedParams->m_horse.Get();
	if ( horseActor )
	{
		m_horseNeeded = false; // As soon as we have a horse reinit flag so that next time we need one works

		// I have a paired horse :
		if ( m_riderData->CheckHorse( horseActor, actor ) )
		{
			Super::Update();
			return;
		}
		else
		{
			ComponentIterator< W3HorseComponent > it( horseActor );
			if ( it  )
			{
				W3HorseComponent* horseComponent	= *it;
				horseComponent->Unpair();
			}
		}
		
	}
	// [Step] If no horse try to pair through encounter system
	CEncounter* encounter = m_encounter.Get();
	if ( PairUsingEncounter( *m_riderData, encounter, actor ) )
	{
		Super::Update();
		return;
	}

	// [Step] if the actor is performing an action with a horse we need to pair
	// But not before because unwanted horses might appear
	if ( m_horseNeeded )
	{
		// [Step] If no horse try to pair with closest available horse with specific tag
		if ( m_riderData->PairWithTaggedHorse( actor, m_preferedHorseTag, m_range ) )
		{
			// In the case where we are loading a game and we are just waiting for horse to appear 
			// Once he does npc needs to instant mount him
			if ( m_wasMountedWhenSaving )
			{
				m_wasMountedWhenSaving = false;
				actor->SignalGameplayEvent( CNAME( RidingManagerMountHorse ), MT_instant );
			}

			Super::Update();
			return;
		}
		// if loading fails make sure we still summon a horse anyway
		if ( m_wasMountedWhenSaving && m_loadFailTimeout < GetOwner()->GetLocalTime() )
		{
			m_wasMountedWhenSaving = false;
		}


		if ( m_wasMountedWhenSaving == false )
		{
			// [Step] If still no horse that means no horse was given by quest desiners for this guy
			// So horse needs to be summoned ! BAM !
			SummonHorseForNPC( *m_riderData, m_preferedHorseTag );
		}
	}
	Super::Update();
}

Bool CBehTreeDecoratorRiderPairingLogicInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( RidingManagerMountHorse ) )
	{
		// Is instant mount
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params && ( params->m_value & MT_instant ) )
		{
			m_horseNeeded = true;
		}
	}
	else if ( e.m_eventName == CNAME( HorseNeeded ) )
	{
		m_horseNeeded = true;
	}
	else if ( e.m_eventName == CNAME( HorseMountEnd ) )
	{
		if ( CHorseRiderSharedParams* const sharedParams = m_riderData->m_sharedParams.Get() )
		{
			CActor* const rider = m_owner->GetActor();
			CActor* const horse = sharedParams->m_horse.Get();
			const CDynamicTagsContainer* const dynamicTagsContainer = GR4Game->GetDynamicTagsContainer();
			if ( rider && horse && dynamicTagsContainer )
			{
				dynamicTagsContainer->UpdateDynamicTags( horse, m_preferedHorseTag );
			}
		}
	}
	return false;
}

Bool CBehTreeDecoratorRiderPairingLogicInstance::PairUsingEncounter( const CAIStorageRiderData & riderData, CEncounter *const encounter, CActor *const riderActor )
{
	if ( encounter == nullptr )
	{
		return false;
	} 
	
	CActor* horsePartyMember = encounter->GetCreaturePool().GetCreaturePartyMember( riderActor, CNAME( horse ) );
	if ( horsePartyMember == nullptr )
	{
		return false;
	}
	ComponentIterator< W3HorseComponent > it( horsePartyMember );	
	if ( !it )
	{
		return false;
	}
	W3HorseComponent* horseComponent	= *it;
	if ( horseComponent->PairWithRider( horsePartyMember, riderData.m_sharedParams.Get() ) == false )
	{
		return false;
	}
	horsePartyMember->SignalGameplayEvent( CNAME( HorseSummon ), GetOwner()->GetActor() );
	return true;	
}

void CBehTreeDecoratorRiderPairingLogicInstance::SaveState( IGameSaver* writer )
{
	CHorseRiderSharedParams *const sharedParams = m_riderData == false ? nullptr : m_riderData->m_sharedParams.Get();
	const bool wasMountedWhenSaving				= sharedParams == nullptr ? false : (sharedParams->m_mountStatus == VMS_mounted || sharedParams->m_mountStatus == VMS_mountInProgress );
	writer->WriteValue< Bool >( CNAME( wasMountedWhenSaving ), wasMountedWhenSaving );

	//No need to save stuff for the boat because the scripted action will mount the npc if it is not mounted
}
Bool CBehTreeDecoratorRiderPairingLogicInstance::LoadState( IGameLoader* reader )
{
	reader->ReadValue< Bool >( CNAME( wasMountedWhenSaving ), m_wasMountedWhenSaving );
	m_loadFailTimeout = GetOwner()->GetLocalTime() + 5.0f;
	return true;
}

Bool CBehTreeDecoratorRiderPairingLogicInstance::IsSavingState() const
{
	return true;
}

class CNPCHorseSpawnEventHandler : public ISpawnEventHandler
{
public :
	CNPCHorseSpawnEventHandler( CHorseRiderSharedParams *const ridingSharedParams, CName preferedHorseTag )
		: m_ridingSharedParams( ridingSharedParams )
		, m_preferedHorseTag( preferedHorseTag )
	{
	}
protected:
	void OnPostAttach( CEntity* entity ) override
	{
		CActor *const horseActor	= static_cast< CActor* >( entity );
		TagList tagList				= horseActor->GetTags( );
		tagList.AddTag( m_preferedHorseTag );
		horseActor->SetTags( tagList );

		ComponentIterator< W3HorseComponent > it( horseActor );	
		if ( !it )
		{
			// Error !
			return;
		}
		W3HorseComponent* horseComponent			= *it;
		CHorseRiderSharedParams *const sharedParams = m_ridingSharedParams.Get();
		if ( sharedParams )
		{
			horseComponent->PairWithRider( horseActor, sharedParams );
			CActor *const rider = sharedParams->m_rider.Get();
			if ( rider )
			{
				horseActor->SignalGameplayEvent( CNAME( HorseSummon ), rider );
			}
		}
	}
	CName								m_preferedHorseTag;
	THandle< CHorseRiderSharedParams >	m_ridingSharedParams;
};

void CBehTreeDecoratorRiderPairingLogicInstance::SummonHorseForNPC( const CAIStorageRiderData & riderData, CName preferedHorseTag )
{
	CR4CreateEntityHelper * createEntityHelper			= m_createEntityHelperHandle.Get();
	// We need a horse and query was not done:
	// If there is an error the handle will nullify itself
	if ( createEntityHelper == nullptr )
	{
		CR4CreateEntityManager *const createEntityManager	= GR4Game->GetR4CreateEntityManager();
		CActor *const actor									= m_owner->GetActor();

		createEntityHelper			= new CR4CreateEntityHelper();
		m_createEntityHelperHandle = createEntityHelper;

		String horseAllias( TXT("horse") );
		//++hack
		String syannaUnicornAlias( TXT( "q704_ft_unicorn_syanna" ) );
		if ( preferedHorseTag.AsString() == syannaUnicornAlias )
		{
			horseAllias = syannaUnicornAlias;
		}
		//// --hack

		createEntityManager->SpawnAliasEntityAroundOwner( createEntityHelper, horseAllias, actor, IdTag(), TDynArray<CName>(), new CNPCHorseSpawnEventHandler( riderData.m_sharedParams.Get(), preferedHorseTag ), false, true );
	}
	if ( createEntityHelper->IsCreating() == false )
	{
		// an error occured 
		m_createEntityHelperHandle = nullptr;
	}
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithHorseDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorRiderPairWithHorseDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithHorseInstance
CBehTreeDecoratorRiderPairWithHorseInstance::CBehTreeDecoratorRiderPairWithHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_riderData( owner, CNAME( RiderData ) )
	, m_messageSend( false )
{

}

Bool CBehTreeDecoratorRiderPairWithHorseInstance::Activate()
{
	if ( m_riderData == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( sharedParams->m_horse.Get() )
	{
		return Super::Activate();
	}

	// Else do not activate child until we find a horse
	m_messageSend = false;
	return IBehTreeNodeInstance::Activate();
}
void CBehTreeDecoratorRiderPairWithHorseInstance::Update()
{
	// [Step]
	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	CActor * horseActor							= sharedParams->m_horse.Get();
	if ( horseActor )
	{
		m_messageSend = false; // needed here because rider might loose horse during the scripted action
		if ( m_child->IsActive() == false )
		{
			m_child->Activate();
		}
		// Activate might fail so checking again
		if ( m_child->IsActive()  )
		{
			Super::Update();
		}
		return;
	}
	CActor *const actor							= m_owner->GetActor();

	if ( m_messageSend == false )
	{
		m_messageSend = true;
		m_owner->GetActor()->SignalGameplayEvent( CNAME( HorseNeeded ) );
	}

	// Do not call Super::Update() if no horse is there !
}

Bool CBehTreeDecoratorRiderPairWithHorseInstance::OnEvent( CBehTreeEvent& e )
{
	if ( m_child->IsActive() )
	{
		return m_child->OnEvent( e );
	}
	return false;
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithRiderDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorHorsePairWithRiderDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithRiderInstance
CBehTreeDecoratorHorsePairWithRiderInstance::CBehTreeDecoratorHorsePairWithRiderInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_horseData( owner, CNAME( HorseData ) )
{
	SBehTreeEvenListeningData eventListenerPair;
	eventListenerPair.m_eventName = CNAME( PairHorse );
	eventListenerPair.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerPair, this );

	SBehTreeEvenListeningData eventListenerUnpair;
	eventListenerUnpair.m_eventName = CNAME( UnpairHorse );
	eventListenerUnpair.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerUnpair, this );
	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( PrePoolRequest );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( eventListener, this );
	}
}
void CBehTreeDecoratorHorsePairWithRiderInstance::OnDestruction()
{
	SBehTreeEvenListeningData eventListenerPair;
	eventListenerPair.m_eventName = CNAME( PairHorse );
	eventListenerPair.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerPair, this );

	SBehTreeEvenListeningData eventListenerUnpair;
	eventListenerUnpair.m_eventName = CNAME( UnpairHorse );
	eventListenerUnpair.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerUnpair, this );

	{
		SBehTreeEvenListeningData eventListener;
		eventListener.m_eventName = CNAME( PrePoolRequest );
		eventListener.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( eventListenerUnpair, this );
	}

	Super::OnDestruction();
}

Bool CBehTreeDecoratorHorsePairWithRiderInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( m_horseData == false )
	{
		return false;
	}
	CActor *const horseActor = m_owner->GetActor();	
	ComponentIterator< W3HorseComponent > it( horseActor );
	if ( it == false )
	{
		return false;
	}
	W3HorseComponent* horseComponent	= *it;
	if ( e.m_eventName == CNAME( PairHorse ) )
	{
		SBehTreePairHorseEventParam* eventParams = e.m_gameplayEventData.Get< SBehTreePairHorseEventParam >();

		if ( eventParams )
		{
			CHorseRiderSharedParams* currParams = horseComponent->m_riderSharedParams.Get();
			if ( currParams != eventParams->m_ptr && currParams )
			{
				// trying to pair while being paired already
				return false;
			}

			// we will end up being paired
			eventParams->m_outcome = true;

			// pair me
			if ( currParams != eventParams->m_ptr )
			{
				// we are now pairing :
				horseComponent->m_riderSharedParams = eventParams->m_ptr;
				eventParams->m_ptr->m_horse = horseActor;
			}
		}
	}
	else if ( e.m_eventName == CNAME( UnpairHorse ) )
	{
		CHorseRiderSharedParams * sharedParams		= horseComponent->m_riderSharedParams.Get();
		if ( sharedParams )
		{
			sharedParams->m_rider 				= NULL;		// the shared params are the riders
			sharedParams->m_horse 				= NULL;
			horseComponent->m_riderSharedParams = NULL;
		}	
	}
	else if ( e.m_eventName == CNAME( PrePoolRequest ) )
	{
		// NOTICE: We can pool only if our rider is pooled in
		CHorseRiderSharedParams * sharedParams		= horseComponent->m_riderSharedParams.Get();
		if ( sharedParams && sharedParams->m_rider )
		{
			SGameplayEventParamInt* param = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
			if ( param )
			{
				param->m_value = false;
			}
		}

		// This messages must be listened to here because it is the only node that is always alive
		// And because if we do that in the rider the horse might be unloaded
		//horseComponent->Unpair();
	}
	return false;
}
