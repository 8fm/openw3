#pragma once
#include "entity.h"

class CBehaviorAnimationMultiplyEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CBehaviorAnimationMultiplyEntity, CEntity, 0 );

protected:
	// Properties
	Float				m_multiplier;

private:

public:
	CBehaviorAnimationMultiplyEntity();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );
};

BEGIN_CLASS_RTTI( CBehaviorAnimationMultiplyEntity );
PARENT_CLASS( CEntity );
PROPERTY_EDIT( m_multiplier,	TXT("multiplier") );
END_CLASS_RTTI();
