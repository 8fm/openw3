/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodePlayerStateIdle.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodePlayerStateIdleDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= nullptr */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodePlayerStateIdleInstance::CBehTreeNodePlayerStateIdleInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodePlayerStateInstance( def, owner, context, parent )
{
}

CBehTreeNodePlayerStateIdleInstance::~CBehTreeNodePlayerStateIdleInstance()
{
}

Bool CBehTreeNodePlayerStateIdleInstance::Activate()
{
	return Super::Activate();
}

void CBehTreeNodePlayerStateIdleInstance::Update()
{
}

void CBehTreeNodePlayerStateIdleInstance::Deactivate()
{
	Super::Deactivate();
}

Bool CBehTreeNodePlayerStateIdleInstance::IsAvailable()
{
	return true;
}

Bool CBehTreeNodePlayerStateIdleInstance::Interrupt()
{
	return Super::Interrupt();
}
