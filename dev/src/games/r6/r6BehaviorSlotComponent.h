#pragma once


#include "../../common/game/selfUpdatingComponent.h"





enum EBehaviorSlot
{
	BS_LOWER_BODY,
	BS_UPPER_BODY,
	BS_LOOK_AROUND,
	BS_INTERACTION,
	BS_RETARGETING,
	BS_COUNT
};





class CR6BehaviorSlotComponent : public CSelfUpdatingComponent
{
	DECLARE_ENGINE_CLASS( CR6BehaviorSlotComponent, CSelfUpdatingComponent, 0 );	

protected:

	TDynArray< CName > m_behaviorSlots;

public:

	CR6BehaviorSlotComponent();

	virtual void OnAttached( CWorld* world )	override;


	void SetBehaviorSlot( EBehaviorSlot slot, const CName& behaviorName );



private:
	void funcSetBehaviorSlot( CScriptStackFrame& stack, void* result );
};




BEGIN_CLASS_RTTI( CR6BehaviorSlotComponent );
	
	PARENT_CLASS( CSelfUpdatingComponent );

	NATIVE_FUNCTION( "SetBehaviorSlot", funcSetBehaviorSlot );

END_CLASS_RTTI();
