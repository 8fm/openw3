/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "navigationMultiReachabilityQueryInterface.h"

#include "movableRepresentationPathAgent.h"

IMPLEMENT_RTTI_ENUM( EAsyncTestResult )
IMPLEMENT_RTTI_ENUM( ENavigationReachabilityTestType )

IMPLEMENT_ENGINE_CLASS( CNavigationReachabilityQueryInterface )

CNavigationReachabilityQueryInterface::CNavigationReachabilityQueryInterface()
	: m_lastTest( EngineTime::ZERO )
{
	m_reachabilityDataPtr = new CMultiReachabilityData();
}

CNavigationReachabilityQueryInterface::~CNavigationReachabilityQueryInterface()
{
}

void CNavigationReachabilityQueryInterface::funcGetLastOutput( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, validityTime, 1.5f );
	FINISH_PARAMETERS;

	EAsyncTestResult ret = EAsyncTastResult_Invalidated;

	if ( m_lastTest + validityTime >= GGame->GetEngineTime() )
	{
		CMultiReachabilityData* reachabilityData = m_reachabilityDataPtr.Get();

		if ( reachabilityData->IsAsyncTaskRunning() )
		{
			ret = EAsyncTastResult_Pending;
		}
		else
		{
			ret = reachabilityData->GetOveralOutcome()
				? EAsyncTastResult_Success
				: EAsyncTastResult_Failure;
		}
	}

	RETURN_ENUM( ret );
}

void CNavigationReachabilityQueryInterface::funcGetOutputClosestDistance( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( m_reachabilityDataPtr->GetClosestTargetDistance() );

}
void CNavigationReachabilityQueryInterface::funcGetOutputClosestEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMultiReachabilityData* reachabilityData = m_reachabilityDataPtr.Get();

	THandle< CEntity > entity;
	Int32 closestTargetIdx = reachabilityData->GetClosestTargetIdx();
	if ( closestTargetIdx >= 0 )
	{
		entity = reachabilityData->GetDestinationEntity( closestTargetIdx );
	}

	RETURN_HANDLE( CEntity, entity );
}


void CNavigationReachabilityQueryInterface::funcTestActorsList( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ENavigationReachabilityTestType, testType, ENavigationReachability_Any );
	GET_PARAMETER( THandle< CActor >, originActor, THandle< CActor >() );
	GET_PARAMETER_REF( TDynArray< THandle< CActor > >, destinations, TDynArray< THandle< CActor > > () );
	GET_PARAMETER_OPT( Float, safeSpotTolerance, 2.5f );
	GET_PARAMETER_OPT( Float, pathfindDistanceLimit, -1.f );
	FINISH_PARAMETERS;

	EAsyncTestResult ret = EAsyncTastResult_Failure;

	CActor* actor = originActor.Get();
	CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : nullptr;
	if ( mac && !destinations.Empty() )
	{
		Vector3 pos = mac->GetWorldPositionRef().AsVector3();
		CPathAgent* pathAgent = mac->GetPathAgent();
		if ( pathAgent->TestLocation( pos ) || pathAgent->FindSafeSpot( pos, safeSpotTolerance, pos, pathAgent->GetCollisionFlags() ) )
		{
			CMultiReachabilityData* reachabilityData = m_reachabilityDataPtr.Get();

			CMultiReachabilityData::QuerySetupInterface setup = reachabilityData->SetupReachabilityQuery( CMultiReachabilityData::EQueryType( testType ), pos, pathAgent->GetForbiddenPathfindFlags() | PathLib::NF_IS_CUSTOM_LINK );
			if ( setup.IsValid() )
			{
				CPathLibWorld* pathlib = pathAgent->GetPathLib();
				setup.SetCollisionFlags( pathAgent->GetCollisionFlags() );
				setup.SetPersonalSpace( pathlib, pathAgent->GetPersonalSpace() );
				setup.ReserveDestinations( destinations.Size() );
				setup.CorrectDestinationPositions();
				if ( pathfindDistanceLimit > 0.f )
				{
					setup.SetPathfindDistanceLimit( pathfindDistanceLimit );
					setup.RequestClosestTargetComputation();
				}

				for ( auto it = destinations.Begin(), end = destinations.End(); it != end; ++it )
				{
					CActor* entity = it->Get();
					if ( entity )
					{
						setup.AddDestination( entity );
					}
				}
				
				switch( reachabilityData->QueryReachable( pathlib ) )
				{
				case PathLib::PATHRESULT_SUCCESS:
					ret = EAsyncTastResult_Success;
					break;
				case PathLib::PATHRESULT_PENDING:
					ret = EAsyncTastResult_Pending;
					break;
				default:
				case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
				case PathLib::PATHRESULT_FAILED:
					ret = EAsyncTastResult_Failure;
					break;
				}
			}
			else
			{
				ret = EAsyncTastResult_Invalidated;
			}

			m_lastTest = GGame->GetEngineTime();
		}
	}
	
	RETURN_ENUM( ret );
}