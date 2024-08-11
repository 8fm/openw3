/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiReachabilityQuery.h"

#include "../../common/game/movableRepresentationPathAgent.h"

///////////////////////////////////////////////////////////////////////////////
// 
// 
//                                                      ___ 
//                                                   ,o88888 
//                                                ,o8888888' 
//                          ,:o:o:oooo.        ,8O88Pd8888" 
//                      ,.::.::o:ooooOoOoO. ,oO8O8Pd888'" 
//                    ,.:.::o:ooOoOoOO8O8OOo.8OOPd8O8O" 
//                   , ..:.::o:ooOoOOOO8OOOOo.FdO8O8" 
//                  , ..:.::o:ooOoOO8O888O8O,COCOO" 
//                 , . ..:.::o:ooOoOOOO8OOOOCOCO" 
//                  . ..:.::o:ooOoOoOO8O8OCCCC"o 
//                     . ..:.::o:ooooOoCoCCC"o:o 
//                     . ..:.::o:o:,cooooCo"oo:o: 
//                  `   . . ..:.:cocoooo"'o:o:::' 
//                  .`   . ..::ccccoc"'o:o:o:::' 
//                 :.:.    ,c:cccc"':.:.:.:.:.' 
//               ..:.:"'`::::c:"'..:.:.:.:.:.' 
//             ...:.'.:.::::"'    . . . . .' 
//            .. . ....:."' `   .  . . '' 
//          . . . ...."' 
//          .. . ."'    
//         . 
// 
// 
///////////////////////////////////////////////////////////////////////////////
// CAIReachabilityQuery
///////////////////////////////////////////////////////////////////////////////
CAIReachabilityQuery::CAIReachabilityQuery( CActor* owner )
	: PathLib::CReachabilityData( 0.01f )
	, m_owner( owner )
	, m_cachedOutputIsSuccessful( false )
	, m_chechedOutputTimeout( 0.f )
{
	CMovingAgentComponent* mac = owner->GetMovingAgentComponent();
	if ( mac )
	{
		CPathAgent* pathAgent = mac->GetPathAgent();
		CPathLibWorld* pathlib = owner->GetLayer()->GetWorld()->GetPathLibWorld();
		Initialize( pathlib, pathAgent->GetPersonalSpace() );

		// TODO: this might need to get updated once in a while (as this could change dynamically sometimes
		m_defaultCollisionFlags = pathAgent->GetCollisionFlags();
		m_forbiddenPathfindFlags = pathAgent->GetForbiddenPathfindFlags();
	}
	
}

Bool CAIReachabilityQuery::QueryReachable( const Vector3& destinationPos, Float reachabilityTolerance, Float localTime )
{
	if ( localTime < m_chechedOutputTimeout )
	{
		return m_cachedOutputIsSuccessful;
	}

	switch( Super::QueryReachable( GGame->GetActiveWorld()->GetPathLibWorld(), m_owner->GetWorldPositionRef().AsVector3(), destinationPos, reachabilityTolerance ) )
	{
	case PathLib::PATHRESULT_PENDING:
		return m_cachedOutputIsSuccessful;
	case PathLib::PATHRESULT_SUCCESS:
		m_cachedOutputIsSuccessful = true;
		m_chechedOutputTimeout = localTime + 1.5f;
		return true;
	default:
	case PathLib::PATHRESULT_FAILED:
		m_cachedOutputIsSuccessful = false;
		m_chechedOutputTimeout = localTime + 1.f;
		return false;
	}
		
	
}
