#pragma once

#include "budgetedTickContainer.h"
#include "tickStats.h"

class CEntity;

/**
 *	Entity specific tick manager supporting budgeted component tick.
 *
 *	Note: only to be used internally by CTickManager.
 */
class CEntityTickManager
{
	friend class CTickManager;
	
	// Tick related entity representation
	class EntityProxy
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
		DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( EntityProxy, tick );
	public:
		enum LOD
		{
			LOD_TickEveryFrame = 0,			// Ticked every frame
			LOD_BudgetedTick,				// Ticked only as often as we can within time limit
			LOD_DisabledTick				// Not ticked at all
		};

	private:
		CEntity*	m_entity;
		Float		m_deltaTimeAccumulator;	// Accumulated delta time (only used when 
		LOD			m_LOD;					// Current LOD

	public:
		EntityProxy( CEntity* entity, LOD lod )
			: m_entity( entity )
			, m_LOD( lod )
			, m_deltaTimeAccumulator( 0.0f )
		{}

		void		AccumulateDeltaTime( Float deltaTime ) { m_deltaTimeAccumulator += deltaTime; }
		Float		GetAccumulatedDeltaTime() const { return m_deltaTimeAccumulator; }
		void		ResetAccumulatedDeltaTime() { m_deltaTimeAccumulator = 0.0f; }

		CEntity*	GetEntity() const { return m_entity; }
		
		LOD			GetLOD() const { return m_LOD; }
		void		SetLOD( LOD lod ) { m_LOD = lod; }

		struct HashFunc
		{
			static RED_FORCE_INLINE Uint32 GetHash( const EntityProxy* proxy ) { return GetPtrHash( proxy->m_entity ); }
			static RED_FORCE_INLINE Uint32 GetHash( const CEntity* entity ) { return GetPtrHash( entity ); }
		};
		struct EqualFunc
		{
			static RED_INLINE Bool Equal( const EntityProxy* a, const EntityProxy* b ) { return a->m_entity == b->m_entity; }
			static RED_INLINE Bool Equal( const EntityProxy* a, const CEntity* b ) { return a->m_entity == b; }
		};
	};

	typedef TBudgetedContainer< EntityProxy, CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( EntityProxy, tick ) >	TickList;
	typedef THashSet< EntityProxy*, EntityProxy::HashFunc, EntityProxy::EqualFunc >									ProxyByEntitySet;

	TickList			m_tickList;				// Entities being ticked: budgeted and non-budgeted
	ProxyByEntitySet	m_proxies;				// All entities registered for ticking (including those with disabled tick due to LOD)

	Vector				m_position;
	Float				m_budgetableDistance;
	Float				m_budgetableDistanceSqr;
	Float				m_disableDistance;
	Float				m_disableDistanceSqr;
	Bool				m_positionValid;		// Reference position was set at least once

	STickGenericStats	m_stats;

public:
	CEntityTickManager();
	~CEntityTickManager();

	const STickGenericStats& GetStats() const { return m_stats; }
	void ResetStats() { m_stats.Reset(); }

	void Add( CEntity* entity );
	void Remove( CEntity* entity );
	void Tick( Float deltaTime );

#ifdef USE_ANSEL
	void TickNPCLODs();
#endif // USE_ANSEL

	// setup reference position for streaming and other calculations
	void SetReferencePosition( const Vector& position );

	EntityProxy::LOD DetermineEntityLOD( CEntity* entity );

	RED_FORCE_INLINE EntityProxy* GetProxy( CEntity* entity )
	{
		auto it = m_proxies.Find( entity );
		return it != m_proxies.End() ? *it : nullptr;
	}
};