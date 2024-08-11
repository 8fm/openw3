#include "build.h"
#include "w3GenericVehicle.h"

#include "ridingAiStorage.h"
#include "behTreeRidingManager.h"

IMPLEMENT_ENGINE_CLASS( W3HorseComponent );

IMPLEMENT_RTTI_ENUM( EHorseMoveType );

////////////////////////////////////////////////////////
// W3HorseComponent
W3HorseComponent::W3HorseComponent()
	: m_riderSharedParams()
{
	m_isHorse = true;
}

Bool W3HorseComponent::PairWithRider( CActor* horseActor, CHorseRiderSharedParams *const riderSharedParams )
{
	SBehTreePairHorseEventParam e( riderSharedParams );

	horseActor->SignalGameplayEvent( CNAME( PairHorse ), &e, SBehTreePairHorseEventParam::GetStaticClass() );

	return e.m_outcome;
}

Bool W3HorseComponent::IsTamed( CActor* horseActor )
{
	CName horseBaseAttitude = horseActor->GetBaseAttitudeGroup();
	// The horse takes the attitude groups of its rider :
	return horseBaseAttitude != CNAME( animals_peacefull );
}


Bool W3HorseComponent::IsTamed()
{
	CActor *const actor = static_cast< CActor* >( GetEntity() );
	if ( actor == nullptr )
	{
		return false;
	}
	return IsTamed( actor );
}


void W3HorseComponent::Unpair()
{
	CHorseRiderSharedParams *const sharedParams = m_riderSharedParams.Get();
	if ( sharedParams  )
	{
		CActor *const currentRiderActor				= sharedParams->m_rider.Get();
		if ( currentRiderActor )
		{
			// notifying the rider that his horse is upaired from him 
			currentRiderActor->SignalGameplayEvent( CNAME( HorseLost ) );
		}
	}

	CActor *const horseActor					= static_cast<CActor*>( GetEntity() );
	horseActor->SignalGameplayEvent( CNAME( UnpairHorse ) );
}

Bool W3HorseComponent::IsDismounted() const
{
	if ( m_riderSharedParams.IsValid() )
	{
		return m_riderSharedParams->m_mountStatus == VMS_dismounted;
	}
	return true;
}

Bool W3HorseComponent::IsFullyMounted() const
{
	if ( m_riderSharedParams.IsValid() )
	{
		return m_riderSharedParams->m_mountStatus == VMS_mounted;
	}
	return false;
}

void W3HorseComponent::funcPairWithRider( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CHorseRiderSharedParams>, riderSharedParams, nullptr );
	FINISH_PARAMETERS;

	CActor* actor = Cast< CActor >( GetEntity() );
	CHorseRiderSharedParams* riderParams = riderSharedParams.Get();
	Bool ret = false;
	if ( actor && riderParams )
	{
		ret = PairWithRider( actor, riderParams );
	}

	RETURN_BOOL( ret );
}

void W3HorseComponent::funcIsTamed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsTamed() );
}

void W3HorseComponent::funcUnpair( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Unpair();
}

void W3HorseComponent::funcIsDismounted( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsDismounted() );
}

void W3HorseComponent::funcIsFullyMounted( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsFullyMounted() );
}