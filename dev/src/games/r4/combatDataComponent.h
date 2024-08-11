#pragma once

#include "ticketSourceProvider.h"

class CTicketSource;

// This class represent runtime data we spawn for each npc who is actively targeted by some attackers.
// Class is used as a mastermind for systems like strafing or ticketing.

class CCombatDataComponent : public CComponent, public ITicketSourceProvider
{
	DECLARE_ENGINE_CLASS( CCombatDataComponent, CComponent, 0 );
public:
	CCombatDataComponent();	

	struct CAttacker
	{
		THandle < CActor >			m_attacker;
		Float						m_worldSpaceYaw;
		Float						m_distance2DSq;
		Int16						m_actionRing;
		Float						m_desiredSeparation;

		Bool operator<( const CAttacker& c ) const							{ return m_worldSpaceYaw < c.m_worldSpaceYaw ? true : m_worldSpaceYaw > c.m_worldSpaceYaw ? false : m_attacker < c.m_attacker; }
	};

	CActor* GetOwner() const												{ return SafeCast< CActor >( GetEntity() ); }

	void ComputeYawDistances();
	Int32 GetAttackerIndex( CActor* actor );
	Int32 LeftAttackerIndex( Int32 index );
	Int32 RightAttackerIndex( Int32 index );
	const CAttacker& GetAttackerData( Int32 index ) const					{ return m_attackers[ index ]; }

	Int32 RegisterAttacker( CActor* actor );
	Bool UnregisterAttacker( CActor* actor );

	Float GetAttackerDesiredSeparation( Int32 index ) const					{ return m_attackers[ index ].m_desiredSeparation; }
	void SetAttackerDesiredSeparation( Int32 index, Float desiredSepration ){ m_attackers[ index ].m_desiredSeparation = desiredSepration; }
	void SetAttackerActionRing( Int32 index, Int16 actionRing )				{ m_attackers[ index ].m_actionRing = actionRing; }
	void ClearAttackerActionRing( Int32 index )								{ m_attackers[ index ].m_actionRing = -1; }
	void ClearAttackerDesiredSeparation( Int32 index )						{ m_attackers[ index ].m_desiredSeparation = 180.f; }

	CTicketSourceConfiguration* GetCustomConfiguration( CName name ) override;	

	Uint32 GetAttackersCount() const										{ return m_attackers.Size(); }
	Uint32 GetAttackersCount( Int16 actionRing ) const;

	Bool AddToAttackersPool( CActor* attacker, Uint32 priority );

	// CObject interface
	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
	void OnFinalize() override;

	virtual bool UsesAutoUpdateTransform() override { return false; }

	static RED_INLINE Bool IsAttackerIndexValid( Int32 index )			{ return index >= 0; }

	struct AttackersPoolEntry
	{
		THandle< CActor >	m_actor;
		Uint32				m_priority;
		AttackersPoolEntry() : m_actor( NULL ), m_priority( 0 ) { }
		AttackersPoolEntry( THandle< CActor > actor, Uint32 priority) : m_actor( actor ), m_priority( priority ) { }
		Bool operator<( const AttackersPoolEntry& other ) const { return m_priority < other.m_priority; }
	};

protected:
	void CleanAttackersPool();
	void LoadAttackersPoolConfig();
	void Clear();

	TSortedArray< CAttacker >		m_attackers;							// collection is sorted ONLY after calling ComputeYawDistance
	EngineTime						m_lastYawComputation;	

	typedef TDynArray< AttackersPoolEntry >	AttackersPool;
	Uint32					m_attackersPoolMaxSize;
	AttackersPool			m_attackersPool;
	Uint32					m_lowestPriority;
	Float					m_nextPoolCleanUp;
	const Float				m_poolCleanUpInterval;

private:

	void funcGetAttackersCount( CScriptStackFrame& stack, void* result );
	void funcGetTicketSourceOwners( CScriptStackFrame& stack, void* result );
	void funcTicketSourceOverrideRequest( CScriptStackFrame& stack, void* result );
	void funcTicketSourceClearRequest( CScriptStackFrame& stack, void* result );
	void funcForceTicketImmediateImportanceUpdate( CScriptStackFrame& stack, void* result );
	void funcHasAttackersInRange( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCombatDataComponent )
	PARENT_CLASS( CComponent )
	NATIVE_FUNCTION( "GetAttackersCount", funcGetAttackersCount )
	NATIVE_FUNCTION( "GetTicketSourceOwners", funcGetTicketSourceOwners )
	NATIVE_FUNCTION( "TicketSourceOverrideRequest", funcTicketSourceOverrideRequest )
	NATIVE_FUNCTION( "TicketSourceClearRequest", funcTicketSourceClearRequest )
	NATIVE_FUNCTION( "ForceTicketImmediateImportanceUpdate", funcForceTicketImmediateImportanceUpdate )
	NATIVE_FUNCTION( "HasAttackersInRange", funcHasAttackersInRange )
END_CLASS_RTTI()

// Recommended way to use CCombatDataComponent is a basic handler
// that also does all lazy component initialization.

class CCombatDataPtr : public THandle< CCombatDataComponent >
{
	typedef THandle< CCombatDataComponent > Super;

private:
	CCombatDataComponent* LazyCreateComponent( CActor* actor );

public:
	CCombatDataPtr()
		: Super()															{}
	CCombatDataPtr( const CCombatDataPtr& c )
		: Super( c )														{}
	CCombatDataPtr( CCombatDataComponent* c )
		: Super( c )														{}
	CCombatDataPtr( CActor* actor )
		: Super( LazyCreateComponent( actor ) )								{}

	RED_INLINE CCombatDataPtr& operator=( const CCombatDataPtr& other )	{ Super::operator=( other ); return *this; }

	RED_INLINE CCombatDataPtr& operator=( CCombatDataComponent* object )	{ Super::operator=( object ); return *this; }

	void Clear()															{ Super::operator=( NULL ); }

	CCombatDataComponent* operator=( CActor* actor )						{ if ( !actor ) { Clear(); return NULL; } auto* component = LazyCreateComponent( actor ); Super::operator=( component ); return component; }

	operator CCombatDataComponent* () const									{ return Super::Get(); }
};

class CCombatDataComponentParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CCombatDataComponentParam, CGameplayEntityParam, 0 );

protected:
	Uint32   m_attackersPoolMaxSize;

public:
	CCombatDataComponentParam() : m_attackersPoolMaxSize( 3 )
	{}

	Uint32 GetAttackersPoolMaxSize() const  { return m_attackersPoolMaxSize;  }
};

BEGIN_CLASS_RTTI( CCombatDataComponentParam );
PARENT_CLASS( CGameplayEntityParam );
PROPERTY_EDIT( m_attackersPoolMaxSize, TXT( "How many attackers entity should be attacked by" ) );
END_CLASS_RTTI();