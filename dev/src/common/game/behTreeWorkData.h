#pragma once

#include "behTreeInstance.h"

#ifndef NO_EDITOR_FRAGMENTS
#define DEBUG_ACTION_POINT_RESERVATION
#endif

class CBehTreeWorkData
{
	DECLARE_RTTI_STRUCT( CBehTreeWorkData );
protected:
	static const Float			SPAWN_TO_WORK_AP_DISTANCE;

	Bool						m_trySpawnToWork;
	Bool						m_tryImmediateActivation;
	Bool						m_isBeingInterrupted;
	Bool						m_isBeingPerformed;
	Bool						m_isInLeaveAction;
	Float						m_workStoppedTime;
	CBehTreeInstance*			m_owner;
	Float						m_spawnToWorkTimeLimit;						// E3: Hack
	Bool						m_forceProcessOnActivation;
	Bool						m_ignoreHardReactions;
	Bool						m_isConscious;
	Bool						m_isSitting;	
	Int32						m_jobTreeType;

	CName						m_selectedAPCategory;
	SActionPointId				m_selectedAP;								// currently selected and reserved are the same - but it might be a subject for a change
	SActionPointId				m_lastAP;
	SActionPointId				m_reservedAP;

	CName						m_forcedCategory;
	Float						m_forcedAPDistance;

public:
	Bool				m_hasLoopingSequence;


	CBehTreeWorkData()
		: m_trySpawnToWork( false )
		, m_tryImmediateActivation( false )
		, m_isBeingInterrupted( false )
		, m_isBeingPerformed( false )
		, m_isInLeaveAction( false )
		, m_forceProcessOnActivation( false )
		, m_ignoreHardReactions( false )		
		, m_isConscious( true )
		, m_isSitting( false )		
		, m_workStoppedTime( 0 )
		, m_jobTreeType( 0 )
	{}

	~CBehTreeWorkData();

	void					SelectAP( const SActionPointId& apID, CName category, Bool reserveAP = true );		// Select & reserve
	void					ClearAP( Bool reserveAP = true )				{ SelectAP( ActionPointBadID, CName::NONE, reserveAP ); }

	void					FreeReservedAP();								// Free reserved AP
	void					ReserveSelectedAP();							// Reserved selected AP

	void					ResetLastAp()									{ m_lastAP = ActionPointBadID; }
	const SActionPointId&	GetLastAP() const								{ return m_lastAP; }
	const SActionPointId&	GetSelectedAP() const							{ return m_selectedAP; }
	const SActionPointId&   GetReservedAP() const							{ return m_reservedAP; }
	CName					GetSelectedAPCategory() const					{ return m_selectedAPCategory; }

	Bool					IsBeingInterrupted() const						{ return m_isBeingInterrupted; }
	Bool					IsBeingPerformed() const						{ return m_isBeingPerformed; }
	Bool					IsInLeaveAction() const							{ return m_isInLeaveAction; }
	void					SetIsInLeaveAction( Bool isInLeaveAction)		{ m_isInLeaveAction = isInLeaveAction; }
	

	void					Interrupt()										{ m_isBeingInterrupted = true; }
	void					StartPerform();
	void					StopPerform();

	static Float			GetSpawnToWorkAPDistance()						{ return SPAWN_TO_WORK_AP_DISTANCE; }

	Bool		WasJustWorking();

	void		SpawnToWork( Float timeLimit )								{ m_trySpawnToWork = true; m_spawnToWorkTimeLimit = timeLimit; }
	void		CancelSpawnToWork()											{ m_trySpawnToWork = false; }
	void		CancelImmediateActivation()									{ m_tryImmediateActivation = false; }
	void		ImmediateActivation( Float timeLimit )						{ m_tryImmediateActivation = true; m_spawnToWorkTimeLimit = timeLimit; }
	void		ForceProcessOnActivation()									{ m_forceProcessOnActivation = true; }
	Bool		ShouldForceProcessOnActivation() const						{ return m_forceProcessOnActivation; }
	Bool		IsTryingToSpawnToWork( CBehTreeInstance* owner );
	Bool		IsInImmediateActivation( CBehTreeInstance* owner );

	RED_INLINE Bool GetIgnoreHardReactions(){ return m_ignoreHardReactions; }
	RED_INLINE void SetIgnoreHadReactions( Bool ignore ){ m_ignoreHardReactions = ignore; }

	RED_INLINE Bool GetIsConscious(){ return m_isConscious; }
	RED_INLINE void SetIsConscious( Bool isConscious ){ m_isConscious = isConscious; }

	RED_INLINE Bool GetIsSitting(){ return m_isSitting; }
	RED_INLINE void SetIsSitting( Bool isSitting ){ m_isSitting = isSitting; }

	RED_INLINE void SetJTType( Int32 jtType ){ m_jobTreeType = jtType; }
	RED_INLINE Int32 GetJTType( ){ return m_jobTreeType; }

	static CName			GetStorageName();

	class CInitializer : public CAIStorageItem::CInitializer
	{
	protected:
		CBehTreeInstance*			m_owner;
	public:
		CInitializer( CBehTreeInstance* owner )
			: m_owner( owner )												{}
		CName GetItemName() const override;
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeWorkData );
END_CLASS_RTTI();

class CBehTreeWorkDataPtr : public TAIStoragePtr< CBehTreeWorkData >
{
	typedef TAIStoragePtr< CBehTreeWorkData > Super;
public:
	CBehTreeWorkDataPtr( CBehTreeInstance* owner )
		: Super( CBehTreeWorkData::CInitializer( owner ), owner )			{}

	CBehTreeWorkDataPtr()
		: Super()															{}
	CBehTreeWorkDataPtr( const CBehTreeWorkDataPtr& p )
		: Super( p )														{}

	CBehTreeWorkDataPtr( CBehTreeWorkDataPtr&& p )
		: Super( Move( p ) )												{}

};