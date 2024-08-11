/**
 * Copyright © 2011 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "movementObject.h"


///////////////////////////////////////////////////////////////////////////////

class CMovingAgentComponent;

///////////////////////////////////////////////////////////////////////////////

// Responsible for symmetrical agent state modification.
class IMoveStateModifier : public IMovementObject
{
public:
	virtual ~IMoveStateModifier() {}

	virtual void Activate( CMovingAgentComponent& agent ) = 0;

	virtual void Deactivate( CMovingAgentComponent& agent ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

// Alters the pushability setting of an agent
class CMoveCollidabilityModifier : public IMoveStateModifier
{
private:
	Bool		m_wasCollidable;
	Bool		m_collidable;

public:
	CMoveCollidabilityModifier( Bool collidable );

	// ------------------------------------------------------------------------
	// IMoveStateModifier implementation
	// ------------------------------------------------------------------------
	virtual void Activate( CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
};

///////////////////////////////////////////////////////////////////////////////
