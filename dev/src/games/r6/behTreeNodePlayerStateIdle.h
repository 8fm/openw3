/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodePlayerState.h"

//////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerStateIdleInstance;

class CBehTreeNodePlayerStateIdleDefinition : public IBehTreeNodePlayerStateDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayerStateIdleDefinition, IBehTreeNodePlayerStateDefinition, CBehTreeNodePlayerStateIdleInstance, Idle )
	DECLARE_AS_R6_ONLY

public:
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodePlayerStateIdleDefinition )
	PARENT_CLASS( IBehTreeNodePlayerStateDefinition )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePlayerStateIdleInstance : public IBehTreeNodePlayerStateInstance
{
	typedef IBehTreeNodePlayerStateInstance Super;

public:
	typedef CBehTreeNodePlayerStateIdleDefinition Definition;

	CBehTreeNodePlayerStateIdleInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	virtual ~CBehTreeNodePlayerStateIdleInstance();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	virtual Bool IsAvailable();
};