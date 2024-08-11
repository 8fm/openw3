#include "build.h"
#include "aiSpawnTreeParameters.h"

#include "behTreeVars.h"
#include "../engine/areaComponent.h"
#include "../engine/triggerAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( IAISpawnTreeParameters )
IMPLEMENT_ENGINE_CLASS( CEncounterParameters )
IMPLEMENT_ENGINE_CLASS( IAISpawnTreeSubParameters )
IMPLEMENT_ENGINE_CLASS( CGuardAreaParameters )
IMPLEMENT_ENGINE_CLASS( CIdleBehaviorsDefaultParameters )

////////////////////////////////////////////////////////////////////////
// CEncounterParameters
////////////////////////////////////////////////////////////////////////
CName CEncounterParameters::EncounterParamName()
{
	return CNAME( encounter );
}
CEncounter* CEncounterParameters::GetEncounter( const CBehTreeSpawnContext& context )
{
	return context.GetVal< CEncounter* >( EncounterParamName(), NULL );
}
////////////////////////////////////////////////////////////////////////
// CGuardAreaParameters
////////////////////////////////////////////////////////////////////////
CName CGuardAreaParameters::GuardAreaParamName()
{
	return CNAME( guardArea );
}
CName CGuardAreaParameters::GuardPursuitAreaParamName()
{
	return CNAME( guardPursuitArea );
}
CName CGuardAreaParameters::GuardPursuitRangeParamName()
{
	return CNAME( guardPursuitRange );
}
CAreaComponent* CGuardAreaParameters::GetDefaultGuardArea( const CBehTreeSpawnContext& context )
{
	EntityHandle h;
	if ( context.GetValRef( GuardAreaParamName(), h ) )
	{
		CEntity* e = h.Get();
		if ( e )
		{
			CAreaComponent* c = e->FindComponent< CAreaComponent >();
			if ( c )
			{
				return c;
			}
		}
	}
	CEncounter* e = CEncounterParameters::GetEncounter( context );
	if ( e )
	{
		return e->GetTriggerArea();
	}
	return NULL;
}
CAreaComponent* CGuardAreaParameters::GetDefaultPursuitArea( const CBehTreeSpawnContext& context )
{
	EntityHandle h;
	if ( context.GetValRef( GuardPursuitAreaParamName(), h ) )
	{
		CEntity* e = h.Get();
		if ( e )
		{
			CAreaComponent* c = e->FindComponent< CAreaComponent >();
			if ( c )
			{
				return c;
			}
		}
	}
	return NULL;
}
Bool CGuardAreaParameters::GetDefaultPursuitRange( const CBehTreeSpawnContext& context, Float& outRange )
{
	Float pursuitRange;
	if ( context.GetValRef( GuardPursuitRangeParamName(), pursuitRange ) )
	{
		if ( pursuitRange >= 0.f )
		{
			outRange = pursuitRange;
			return true;
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////////////
// CIdleBehaviorsDefaultParameters
////////////////////////////////////////////////////////////////////////

CName CIdleBehaviorsDefaultParameters::ActionPointAreaParamName()
{
	return CNAME( actionPointsArea );
}

CName CIdleBehaviorsDefaultParameters::WanderAreaParamName()
{
	return CNAME( wanderArea );
}
CName CIdleBehaviorsDefaultParameters::WanderPointsTagParamName()
{
	return CNAME( wanderPointsTag );
}

CAreaComponent*	CIdleBehaviorsDefaultParameters::GetDefaultActionPointArea( const CBehTreeSpawnContext& context )
{
	EntityHandle h;
	if ( context.GetValRef( ActionPointAreaParamName(), h ) )
	{
		CEntity* e = h.Get();
		if ( e )
		{
			CAreaComponent* c = e->FindComponent< CAreaComponent >();
			if ( c )
			{
				return c;
			}
		}
	}
	CEncounter* e = CEncounterParameters::GetEncounter( context );
	if ( e )
	{
		return e->GetTriggerArea();
	}
	return NULL;
}
CAreaComponent*	CIdleBehaviorsDefaultParameters::GetDefaultWanderArea( const CBehTreeSpawnContext& context )
{
	EntityHandle h;
	if ( context.GetValRef( WanderAreaParamName(), h ) )
	{
		CEntity* e = h.Get();
		if ( e )
		{
			CAreaComponent* c = e->FindComponent< CAreaComponent >();
			if ( c )
			{
				return c;
			}
		}
	}
	CEncounter* e = CEncounterParameters::GetEncounter( context );
	if ( e )
	{
		return e->GetTriggerArea();
	}
	return NULL;
}
CName CIdleBehaviorsDefaultParameters::GetDefaultWanderPointsTag( const CBehTreeSpawnContext& context )
{
	return context.GetVal( WanderPointsTagParamName(), CName::NONE );
}