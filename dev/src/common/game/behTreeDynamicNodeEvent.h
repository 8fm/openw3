#pragma once

class CSimpleBufferReader;
class CSimpleBufferWriter;

struct SBehTreeDynamicNodeEventData
{
	DECLARE_RTTI_STRUCT( SBehTreeDynamicNodeEventData );
public:
	typedef TStaticArray< const IAIParameters*, 1 > Parameters;

	enum EOutput
	{
		OUTPUT_NOT_HANDLED,
		OUTPUT_HANDLED,
		OUTPUT_DELAYED
	};
	SBehTreeDynamicNodeEventData( IAITree* treeDef = NULL, Bool interrupt = true, IGameDataStorage* aiState = nullptr )
		: m_treeDefinition( treeDef )
		, m_savedState( aiState )
		, m_isHandled( OUTPUT_NOT_HANDLED )
		, m_interrupt( interrupt )											{}

	void PushParameters( const Parameters& params )
	{
		m_parameters.PushBack( params );
	}

	IAITree*					m_treeDefinition;
	IGameDataStorage*			m_savedState;
	Parameters					m_parameters;
	EOutput						m_isHandled : 16;
	Bool						m_interrupt;
};

BEGIN_NODEFAULT_CLASS_RTTI( SBehTreeDynamicNodeEventData );
END_CLASS_RTTI();

struct SBehTreeDynamicNodeCancelEventData
{
	DECLARE_RTTI_STRUCT( SBehTreeDynamicNodeCancelEventData );
public:
	SBehTreeDynamicNodeCancelEventData( Bool interrupt = false )
		: m_isHandled( OUTPUT_NOT_HANDLED )
		, m_interrupt( interrupt )											{}
	enum EOutput
	{
		OUTPUT_NOT_HANDLED,
		OUTPUT_HANDLED,
		OUTPUT_DELAYED
	};
	EOutput		m_isHandled;
	Bool		m_interrupt;
};

BEGIN_NODEFAULT_CLASS_RTTI( SBehTreeDynamicNodeCancelEventData );
END_CLASS_RTTI();
		   
enum class EAIForcedBehaviorInterruptionLevel
{
	Low,
	High
};

struct SForcedBehaviorEventData : public SBehTreeDynamicNodeEventData
{
	DECLARE_RTTI_STRUCT( SForcedBehaviorEventData );
public:
	SForcedBehaviorEventData( IAITree* treeDef = nullptr, Int8 overridenPriority = -1, Bool interrupt = false, IGameDataStorage* aiState = nullptr, EAIForcedBehaviorInterruptionLevel interruptionLevel = EAIForcedBehaviorInterruptionLevel::High )
		: SBehTreeDynamicNodeEventData( treeDef, interrupt, aiState )
		, m_overridenPriority( overridenPriority )
		, m_uniqueActionId( -1 )
		, m_interruptionLevel( interruptionLevel )
		, m_fireEventAndForget( true )	{}

	SForcedBehaviorEventData( Int16 actionId, Bool interrupt = true, Bool fireEventAndForget = true )
		: SBehTreeDynamicNodeEventData( NULL, interrupt )
		, m_overridenPriority( -1 )
		, m_uniqueActionId( actionId )
		, m_interruptionLevel( EAIForcedBehaviorInterruptionLevel::High )
		, m_fireEventAndForget( fireEventAndForget )	{}


	Int16								m_uniqueActionId;
	Int8								m_overridenPriority;
	EAIForcedBehaviorInterruptionLevel	m_interruptionLevel;
	Bool								m_fireEventAndForget;

	static CName			GetEventName();
	static CName			GetCancelEventName();

	static const Int16 IgnoreActionId = 0xffffu;
};

BEGIN_NODEFAULT_CLASS_RTTI( SForcedBehaviorEventData );
END_CLASS_RTTI();

struct SDynamicNodeSaveStateRequestEventData
{
	DECLARE_RTTI_STRUCT( SDynamicNodeSaveStateRequestEventData )
public:
	SDynamicNodeSaveStateRequestEventData()
		: m_processed( false )
		, m_invalidated( false )
		, m_dataBuffer( nullptr )											{}

	Bool					m_processed;
	Bool					m_invalidated;
	IGameDataStorage*		m_dataBuffer;
};

BEGIN_NODEFAULT_CLASS_RTTI( SDynamicNodeSaveStateRequestEventData );
END_CLASS_RTTI();