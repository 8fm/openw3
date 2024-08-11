#include "build.h"
#include "behTreeRiderSpecific.h"
#include "../../common/game/behTreeInstance.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeRiderPursueHorseDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	CBehTreeNodeRiderPursueHorseInstance* instance = new Instance( *this, owner, context, parent );
	return instance;
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////
CBehTreeNodeRiderPursueHorseInstance::CBehTreeNodeRiderPursueHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_riderData( owner, CNAME( RiderData ) )
{
}
CNode*const CBehTreeNodeRiderPursueHorseInstance::ComputeTarget()
{
	if ( m_riderData == false )
	{
		return nullptr;
	}
	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		return nullptr;
	}

	return sharedParams->m_horse.Get();
}

//////////////////////////////////////////////////////////////////////////
//////      CBehTreeNodeRiderRotateToHorseDefinition      ////////////////
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeRiderRotateToHorseDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}
///////////////////////////////////////////////////////////////////////////
//CBehTreeNodeRiderRotateToHorseInstance
CBehTreeNodeRiderRotateToHorseInstance::CBehTreeNodeRiderRotateToHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_riderData( owner, CNAME( RiderData ) )
{

}
CNode*const CBehTreeNodeRiderRotateToHorseInstance::GetTarget()const 
{
	if ( m_riderData == false )
	{
		return nullptr;
	}
	CHorseRiderSharedParams *const sharedParams = m_riderData->m_sharedParams.Get();
	if ( sharedParams == nullptr )
	{
		return nullptr;
	}

	return sharedParams->m_horse.Get();
}


