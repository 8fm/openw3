#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeConditionRespondToMusicInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that responds to music events
////////////////////////////////////////////////////////////////////////
class CBehTreeConditionRespondToMusicDefinition :
	public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeConditionRespondToMusicDefinition, CBehTreeNodeConditionDefinition, CBehTreeConditionRespondToMusicInstance, RespondToMusic );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	CBehTreeValFloat m_musicWaitTimeLimit;
	CBehTreeValFloat m_syncTimeOffset;
	CBehTreeValString m_syncTypes;
	CBehTreeValString m_musicEventToTrigger;
	CBehTreeValFloat m_musicEventPreTriggerTime;
	CBehTreeValBool m_alwaysTriggerEvent;

public:
	CBehTreeConditionRespondToMusicDefinition()
	: m_musicWaitTimeLimit(0.f)
	, m_syncTimeOffset(0.f)
	, m_syncTypes(String(TXT("bar")))
	, m_musicEventToTrigger(String::EMPTY)
	, m_musicEventPreTriggerTime(0.f)
	, m_alwaysTriggerEvent(false)
	{}

};

BEGIN_CLASS_RTTI( CBehTreeConditionRespondToMusicDefinition );
	PARENT_CLASS( CBehTreeNodeConditionDefinition );
PROPERTY_EDIT(m_musicWaitTimeLimit, TXT("Maximum time AI is prepared to wait for a music event"));
PROPERTY_EDIT(m_syncTimeOffset, TXT("An offset added to the time of incoming music events"));
PROPERTY_EDIT(m_syncTypes, TXT("Which music events to respond to e.g. 'bar, grid, user'"));
PROPERTY_EDIT(m_musicEventToTrigger, TXT("If this node should trigger custom music effects when sync occurs, specify the event here"));
PROPERTY_EDIT(m_musicEventPreTriggerTime, TXT("Try and trigger the music event this many seconds before the sync point (e.g. to let wwise manage the sync)"));
PROPERTY_EDIT(m_alwaysTriggerEvent, TXT("Always trigger the sync event when this node returns true"));
END_CLASS_RTTI();



////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeConditionRespondToMusicInstance : public CBehTreeNodeConditionInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeConditionRespondToMusicDefinition Definition;

	CBehTreeConditionRespondToMusicInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );


private:

protected:
	Bool ConditionCheck() override;

	Float m_musicWaitTimeLimit;
	Float m_syncTimeOffset;
	String m_musicEventToTrigger;
	Float m_musicEventPreTrigger;
	Bool m_alwaysTriggerEvent;

	Bool m_canTriggerEvent;

	//Flags set using CMusicSystem::EMusicResponseEventType
	Uint32 m_eventFlags;
};