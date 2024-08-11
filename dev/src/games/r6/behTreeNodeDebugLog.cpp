/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeDebugLog.h"

#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeInstance.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeDebugLogDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= nullptr */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeDebugLogInstance::CBehTreeNodeDebugLogInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
{
	def.m_text.GetValRef( context, m_text );
}

Bool CBehTreeNodeDebugLogInstance::Activate()
{
	RED_LOG( AITree, TXT("CBehTreeNodeDebugLogInstance::Activate(): %s"), m_text.AsChar() );
	return Super::Activate();
}

void CBehTreeNodeDebugLogInstance::Update()
{
	RED_LOG( AITree, TXT("CBehTreeNodeDebugLogInstance::Update(): %s"), m_text.AsChar() );
	Complete( BTTO_SUCCESS );
}

void CBehTreeNodeDebugLogInstance::Deactivate()
{
	RED_LOG( AITree, TXT("CBehTreeNodeDebugLogInstance::Deactivate(): %s"), m_text.AsChar() );
	Super::Deactivate();
}