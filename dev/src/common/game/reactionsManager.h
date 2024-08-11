/**
 * Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CInterestPointComponent;
class CInterestPoint;
class CInterestPointInstance;
class CActor;

// A manager handling global events happening in the world
class CReactionsManager : public CObject
{
	DECLARE_ENGINE_CLASS( CReactionsManager, CObject, 0 );

public:
	struct MappedInstance
	{
		CNewNPC*					m_npc;
		CInterestPointInstance*		m_instance;

		friend IFile& operator<<( IFile& file, MappedInstance &mi )
		{
			return file << mi.m_npc << mi.m_instance;
		}
	};

private:
	Float									m_interestPointsQueryCooldownTimer;
	Vector									m_impactBBoxSize;
	TDynArray< CInterestPointInstance* >	m_instances;							//!< Broadcasted instances TODO: Use TBinaryStorage (or other spatial data structure) to optimize removals and queries
	TDynArray< CInterestPointInstance* >    m_instancesCopy;
	TDynArray< MappedInstance >				m_mappedInstances;						//!< Instances sent to specific actors
	TDynArray< MappedInstance >				m_mappedInstancesCopy;

public:
	CReactionsManager();
	virtual ~CReactionsManager();

	//! Get instances
	const TDynArray< CInterestPointInstance* >& GetInstances() const { return m_instances; }

	//! Get mapped instances
	const TDynArray< MappedInstance >& GetMappedInstances() const { return m_mappedInstances; }

	virtual void OnSerialize( IFile& file );

	//! Processes the reactions to static interest points
	void Tick( Float timeElapsed );
	
	// Broadcasts an interest point to the interested actors
	void BroadcastInterestPoint( CInterestPoint* interestPoint, const Vector& pos, Float duration );

	// Broadcasts an interest point to the interested actors
	void BroadcastInterestPoint( CInterestPoint* interestPoint, const THandle< CNode >& node, Float duration );

	// Send dynamic interest point
	void SendInterestPoint( CNewNPC* npc, CInterestPoint* interestPoint, const Vector& pos, Float duration );

	// Send dynamic interest point
	void SendInterestPoint( CNewNPC* npc, CInterestPoint* interestPoint, const THandle< CNode >& node, Float duration );

	// NPC detached
	void OnNPCDetached( CNewNPC* npc );

private:	
	void BroadcastInterestPoint( CInterestPointInstance* interestPoint );
	void SendInterestPoint( CNewNPC* npc, CInterestPointInstance* interestPoint );
	void Broadcast( Float timeElapsed );
	void UpdateInterestPoints( Float timeElapsed );
	void QueryStaticInterestPoints();

	// -------------------------------------------------------------------------
	// Scripting support
	// -------------------------------------------------------------------------
private:	
	void funcBroadcastStaticInterestPoint( CScriptStackFrame& stack, void* result );
	void funcBroadcastDynamicInterestPoint( CScriptStackFrame& stack, void* result );
	void funcSendStaticInterestPoint( CScriptStackFrame& stack, void* result );
	void funcSendDynamicInterestPoint( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CReactionsManager );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "BroadcastStaticInterestPoint", funcBroadcastStaticInterestPoint );
	NATIVE_FUNCTION( "BroadcastDynamicInterestPoint", funcBroadcastDynamicInterestPoint );
	NATIVE_FUNCTION( "SendStaticInterestPoint", funcSendStaticInterestPoint );
	NATIVE_FUNCTION( "SendDynamicInterestPoint", funcSendDynamicInterestPoint );
END_CLASS_RTTI();

