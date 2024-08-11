/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphStoreSyncInfo.h"
#include "../engine/behaviorGraphInstance.h"
#include "movingAgentComponent.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStoreSyncInfoNode );

void CBehaviorGraphStoreSyncInfoNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			CSyncInfo syncInfo;
			syncInfo.m_wantSyncEvents = false;

			m_cachedInputNode->GetSyncInfo( instance, syncInfo );
			mac->AccessAnimationProxy().StoreSynchronization( m_storeName, syncInfo );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConvertSyncInfoIntoCyclesNode );

CBehaviorGraphConvertSyncInfoIntoCyclesNode::CBehaviorGraphConvertSyncInfoIntoCyclesNode()
	: m_numCycles( 1 )
{

}

void CBehaviorGraphConvertSyncInfoIntoCyclesNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_numCycles > 1 )
	{
		const Float numCycles = (Float)m_numCycles;

		CSyncInfo infoToConvert;
		TBaseClass::GetSyncInfo( instance, infoToConvert );

		const Float totalTime = infoToConvert.m_totalTime;
		const Float oneCycleTime = totalTime / numCycles;

		// Curr and prev time
		if ( infoToConvert.m_currTime > oneCycleTime || infoToConvert.m_prevTime > oneCycleTime )
		{
			const Float diff = infoToConvert.m_prevTime - infoToConvert.m_currTime;
			const Float convertedCurrTime = fmodf( infoToConvert.m_currTime, oneCycleTime );
			Float convertedPrevTime = convertedCurrTime + diff;
			if ( convertedPrevTime < 0.f )
			{
				convertedPrevTime += oneCycleTime;
			}

			info.m_currTime = convertedCurrTime;
			info.m_prevTime = convertedPrevTime;

		}
		else
		{
			info.m_currTime = infoToConvert.m_currTime;
			info.m_prevTime = infoToConvert.m_prevTime;
		}
		
		// Events
		if ( infoToConvert.m_wantSyncEvents && infoToConvert.m_syncEvents.Size() > 0 )
		{
			info.m_wantSyncEvents = true;

			// All events for now
			info.m_syncEvents = infoToConvert.m_syncEvents;
		}
		
		info.m_totalTime = oneCycleTime;
	}
	else
	{
		TBaseClass::GetSyncInfo( instance, info );
	}
}

/*void CBehaviorGraphConvertSyncInfoIntoCyclesNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	TBaseClass::SynchronizeTo( instance, info );
}*/

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
