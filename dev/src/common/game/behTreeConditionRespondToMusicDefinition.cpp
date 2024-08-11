#include "build.h"
#include "behTreeConditionRespondToMusicDefinition.h"
#include "..\engine\soundSystem.h"
#include "..\engine\game.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeConditionRespondToMusicDefinition )

CBehTreeConditionRespondToMusicInstance::CBehTreeConditionRespondToMusicInstance(const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent)
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
{
	m_musicWaitTimeLimit = def.m_musicWaitTimeLimit.GetVal(context);
	m_syncTimeOffset = def.m_syncTimeOffset.GetVal(context);

	m_eventFlags = 0;

	String eventTypes;
	def.m_syncTypes.GetValRef(context, eventTypes);
	eventTypes.ToLower();
	size_t substringIndex;
	if(eventTypes.FindSubstring(TXT("bar"), substringIndex))
	{
		m_eventFlags |= CMusicSystem::Bar;
	}
	if(eventTypes.FindSubstring(TXT("grid"), substringIndex))
	{
		m_eventFlags |= CMusicSystem::Grid;
	}
	if(eventTypes.FindSubstring(TXT("user"), substringIndex))
	{
		m_eventFlags |= CMusicSystem::User;
	}
	if(eventTypes.FindSubstring(TXT("beat"), substringIndex))
	{
		m_eventFlags |= CMusicSystem::Beat;
	}

	def.m_musicEventToTrigger.GetValRef(context, m_musicEventToTrigger);
	m_alwaysTriggerEvent = def.m_alwaysTriggerEvent.GetVal(context);

	m_musicEventPreTrigger = def.m_musicEventPreTriggerTime.GetVal(context);

	m_canTriggerEvent = true;
}


Bool CBehTreeConditionRespondToMusicInstance::ConditionCheck()
{
	Float engineTime = GGame->GetEngineTime();

	static const Float maxTimeVariance = 0.1f;

	Float nexEventTime = GSoundSystem->GetMusicSystem().GetTimeToNextValidEventBeforeTime(m_eventFlags, engineTime+m_musicWaitTimeLimit, m_syncTimeOffset);


	if(nexEventTime < engineTime + maxTimeVariance)
	{
		//We've successfully synched, reset the play event
		if(m_canTriggerEvent && !m_musicEventToTrigger.Empty())
		{
			GSoundSystem->SoundEvent(UNICODE_TO_ANSI(m_musicEventToTrigger.AsChar()));
		}
		m_canTriggerEvent = true;
#ifndef RED_FINAL_BUILD
		CMusicSystem::SMusicSyncDebugInfo debugInfo;
		debugInfo.eventName = m_musicEventToTrigger;
		debugInfo.nextEventTime = nexEventTime;
		debugInfo.timeOffset = m_syncTimeOffset;
		debugInfo.type = m_eventFlags;
		debugInfo.uniqueId = (Uint64)this;
#ifdef EDITOR_AI_DEBUG
		debugInfo.debugName = GetDebugName();
#endif
		GSoundSystem->GetMusicSystem().RegisterPendingSync(debugInfo);
		GSoundSystem->GetMusicSystem().RegisterSync((Uint64)this);
#endif // !RED_FINAL_BUILD
		nexEventTime = 0.f;
		return true;
	}

#ifndef RED_FINAL_BUILD
	CMusicSystem::SMusicSyncDebugInfo debugInfo;
	debugInfo.eventName = m_musicEventToTrigger;
	debugInfo.nextEventTime = nexEventTime;
	debugInfo.timeOffset = m_syncTimeOffset;
	debugInfo.type = m_eventFlags;
	debugInfo.uniqueId = (Uint64)this;
#ifdef EDITOR_AI_DEBUG
	debugInfo.debugName = GetDebugName();
#endif
	GSoundSystem->GetMusicSystem().RegisterPendingSync(debugInfo);
#endif
	
	return false;
}








