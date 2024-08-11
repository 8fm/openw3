
#pragma once

#include "r6Component.h"


class CAimHelpTargetsGatherer;






/// @n Moved logics from r6Player
class CR6AimingComponent : public CR6Component
{
	DECLARE_ENGINE_CLASS( CR6AimingComponent, CR6Component, 0 );

	/// @todo MS: don't keep this in memory - pick this every frame, when needed
	/// @todo MS: maybe remove CAimHelpTargetGatherer completely?
	THandle< CAimHelpTargetsGatherer >				m_aimHelpGatherer;
	THandle<CEntity>								m_currAimedEntity;
	Bool											m_aimedEntityChangedSinceLastFrame;

public:

	CR6AimingComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached(  CWorld* world ) override;

	virtual void OnTick( Float timeDelta ) override;


private:

	void UpdateAimedEntity( Float timeDelta );
	void SetNewAimedEntity( CEntity* currAimEntity );

public:

	CEntity* GetAimedEntity() const { return m_currAimedEntity.Get(); }


	void funcGetAimedEntity							( CScriptStackFrame& stack, void* result );
	void funcGetAimedEntityChangedSinceLastFrame	( CScriptStackFrame& stack, void* result );
};








BEGIN_CLASS_RTTI( CR6AimingComponent );

	PARENT_CLASS( CR6Component );

	NATIVE_FUNCTION( "GetAimedEntity"						, funcGetAimedEntity );
	NATIVE_FUNCTION( "GetAimedEntityChangedSinceLastFrame"	, funcGetAimedEntityChangedSinceLastFrame );

END_CLASS_RTTI();
