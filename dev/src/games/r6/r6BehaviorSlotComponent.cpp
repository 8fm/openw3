#include "build.h"

#include "r6BehaviorSlotComponent.h"
#include "..\..\common\engine\behaviorGraphStack.h"
#include "..\..\common\engine\animatedComponent.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CR6BehaviorSlotComponent );










CR6BehaviorSlotComponent::CR6BehaviorSlotComponent()
{
}










void CR6BehaviorSlotComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// create default behavior stack
	m_behaviorSlots.Resize( BS_COUNT );

	m_behaviorSlots[ BS_LOWER_BODY ]	= CNAME( lowerbody );
	m_behaviorSlots[ BS_UPPER_BODY ]	= CName::NONE;
	m_behaviorSlots[ BS_LOOK_AROUND ]	= CNAME( lookaround );
	m_behaviorSlots[ BS_INTERACTION ]	= CNAME( interactions );
	m_behaviorSlots[ BS_RETARGETING ]	= CNAME( retargeting );
}











void CR6BehaviorSlotComponent::SetBehaviorSlot( EBehaviorSlot slot, const CName& behaviorName )
{
	R6_ASSERT( slot < BS_COUNT, TXT( "Unknown slot in CR6BehaviorSlotComponent::SetBehaviorSlot." ) );

	m_behaviorSlots[ slot ] = behaviorName;

	R6_ASSERT( GetEntity(), TXT( "There is no entity for CR6BehaviorSlotComponent" ) );

	CAnimatedComponent* ac = GetEntity()->GetRootAnimatedComponent();
	if( !ac )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT( "Animation" ), TXT( "CR6BehaviorSlotComponent requires CAnimatedComponent to work properly." ) );
		return;
	}

	CBehaviorGraphStack* bstack = ac->GetBehaviorStack();
	R6_ASSERT( bstack, TXT( "CAnimatedComponent without a Behavior Stack." ) );
	if( !bstack )
	{
		return;
	}


	// create list of active behaviors
	TDynArray< CName > activeSlots;
	activeSlots.Reserve( BS_COUNT );
	for( Int32 i = 0; i < BS_COUNT; ++i )
	{
		if( m_behaviorSlots[ i ] != CName::NONE )
		{
			activeSlots.PushBack( m_behaviorSlots[ i ] );
		}
	}


	if( !bstack->ActivateBehaviorInstances( activeSlots ) )
	{
		BEH_ERROR( TXT("Script - Behavior instance activation fail - '%ls'"), GetEntity()->GetName().AsChar() );
	}
}










void CR6BehaviorSlotComponent::funcSetBehaviorSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EBehaviorSlot,  slot, BS_COUNT );
	GET_PARAMETER( CName, behaviorName, CName::NONE );
	FINISH_PARAMETERS;

	if( slot == BS_COUNT )
	{
		R6_ASSERT( false, TXT( "Wrong slot specified from the script. (SetBehaviorSlot called)" ) );
		return;
	}

	SetBehaviorSlot( slot, behaviorName );
}
