#pragma once

#include "aiParameters.h"
#include "encounter.h"

class CBehTreeSpawnContext;

class IAISpawnTreeParameters : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAISpawnTreeParameters )
};

BEGIN_ABSTRACT_CLASS_RTTI( IAISpawnTreeParameters )
	PARENT_CLASS( IAIParameters )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////

class CEncounterParameters : public IAISpawnTreeParameters
{
	typedef IAISpawnTreeParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CEncounterParameters )
public:
	CEncounterParameters()									{}

	RED_INLINE void			SetEncounter( CEncounter* encounter )			{ m_encounter = encounter; }
	RED_INLINE TDynArray< THandle< IAISpawnTreeSubParameters > >&
							GetGlobalDefaults()								{ return m_globalDefaults; }

	static CName				EncounterParamName();

	static CEncounter*			GetEncounter( const CBehTreeSpawnContext& context );
protected:
	THandle< CEncounter >									m_encounter;
	TDynArray< THandle< IAISpawnTreeSubParameters > >		m_globalDefaults;
};

BEGIN_CLASS_RTTI( CEncounterParameters )
	PARENT_CLASS( IAISpawnTreeParameters )
	PROPERTY_EDIT( m_encounter, TXT("Encounter. Used externally") )
	PROPERTY_INLINED( m_globalDefaults, TXT("List of parameters visible globaly in whole spawn tree") )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////

class IAISpawnTreeSubParameters : public IAISpawnTreeParameters
{
	typedef IAISpawnTreeParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAISpawnTreeSubParameters )
};

BEGIN_ABSTRACT_CLASS_RTTI( IAISpawnTreeSubParameters )
	PARENT_CLASS( IAISpawnTreeParameters )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////

class CGuardAreaParameters : public IAISpawnTreeSubParameters 
{
	typedef IAISpawnTreeSubParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CGuardAreaParameters )

protected:
	EntityHandle	m_guardArea;
	EntityHandle	m_guardPursuitArea;
	Float			m_guardPursuitRange;

public:
	CGuardAreaParameters()
		: m_guardPursuitRange( 15.0f )
	{
	}

	// accessors
	CEntity*				GetGuardArea() { return m_guardArea.Get(); }
	CEntity*				GetGuardPursuitArea() { return m_guardPursuitArea.Get(); }
	Float					GetGuardPursuitRange() { return m_guardPursuitRange; }

	// statics
	static CName			GuardAreaParamName();
	static CName			GuardPursuitAreaParamName();
	static CName			GuardPursuitRangeParamName();

	static CAreaComponent*	GetDefaultGuardArea( const CBehTreeSpawnContext& context );
	static CAreaComponent*	GetDefaultPursuitArea( const CBehTreeSpawnContext& context );
	static Bool				GetDefaultPursuitRange( const CBehTreeSpawnContext& context, Float& outRange );
};

BEGIN_CLASS_RTTI( CGuardAreaParameters )
	PARENT_CLASS( IAISpawnTreeSubParameters )
	PROPERTY_EDIT( m_guardArea, TXT("Inner area to be guarded by actors"))
	PROPERTY_EDIT( m_guardPursuitArea, TXT("Outer area for pursuing player") )
	PROPERTY_EDIT( m_guardPursuitRange, TXT("Range for pursuing player, used if pursuit area not defined") )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////

class CIdleBehaviorsDefaultParameters : public IAISpawnTreeSubParameters
{
	typedef IAISpawnTreeSubParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CIdleBehaviorsDefaultParameters )
protected:
	EntityHandle		m_actionPointsArea;
	EntityHandle		m_wanderArea;
	CName				m_wanderPointsTag;
public:
	// accessors
	CEntity*				GetActionPointsArea() { return m_actionPointsArea.Get(); }
	CEntity*				GetWanderArea() { return m_wanderArea.Get(); }
	CName					GetWanderPointsTag() { return m_wanderPointsTag; }

	// statics
	static CName			ActionPointAreaParamName();
	static CName			WanderAreaParamName();
	static CName			WanderPointsTagParamName();

	static CAreaComponent*	GetDefaultActionPointArea( const CBehTreeSpawnContext& context );
	static CAreaComponent*	GetDefaultWanderArea( const CBehTreeSpawnContext& context );
	static CName			GetDefaultWanderPointsTag( const CBehTreeSpawnContext& context );
};

BEGIN_CLASS_RTTI( CIdleBehaviorsDefaultParameters )
	PARENT_CLASS( IAISpawnTreeSubParameters )
	PROPERTY_EDIT( m_actionPointsArea, TXT("Area at which ai search for action points by default"))
	PROPERTY_EDIT( m_wanderArea, TXT("Default ai wander area") )
	PROPERTY_EDIT( m_wanderPointsTag, TXT("Default wander points tags") )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////
