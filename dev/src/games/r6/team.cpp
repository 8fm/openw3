#include "build.h"

#include "team.h"
#include "enemyAwareComponent.h"
#include "teamSharedKnowledge.h"
#include "../../common/engine/pathlibWorld.h"

IMPLEMENT_ENGINE_CLASS( SCombatLine );
IMPLEMENT_ENGINE_CLASS( SCombatAlley );
IMPLEMENT_ENGINE_CLASS( CTeam );

const Float CTeam::COMBAT_ALLEY_MARGIN_PERCENTAGE	= 0.1f;
const Float CTeam::COMBAT_LINE_LENGHT				= 100;

void CTeam::Initialize()
{
	m_teamSharedKnowladge = CreateObject< CTeamSharedKnowladge >( this );	
	m_teamSharedKnowladge->SetTeam( this );

	CallFunction( this, CNAME( Export_Initialize ) ); 
}

void CTeam::AddMember		( CTeamMemberComponent* teamMember )
{
	for( Uint32 i=0; i<m_members.Size(); ++i )
	{
		if( m_members[i].Get() && m_members[i].Get() == teamMember )
		{
			return;
		}
	}

	m_members.PushBack( teamMember );
}

void CTeam::RemoveMember( CTeamMemberComponent* teamMember )
{
	for( Uint32 i=0; i<m_members.Size(); ++i )
	{
		if( m_members[i].Get() && m_members[i].Get() == teamMember )
		{
			//m_members[i] = NULL;
			m_members.RemoveAt( i );
			return;
		}
	}
}

SCombatLine CTeam::CalculateCombatLine( Float linePosition, Int32 subteamFlag/* = 0*/ )
{
	SCombatLine combatLine;

	if( m_members.Size()==0 )
		return combatLine;	
	
	Uint32 membersCount = 0;
	Vector teamCenter( 0, 0, 0 );
	for( Uint32 i=0; i<m_members.Size(); ++i )
	{	
		if( m_members[i].Get() )
		{
			CTeamMemberComponent *member = m_members[i].Get();
			if( subteamFlag == 0 || ( subteamFlag & member->GetSubteamFlag() ) )
			{
				teamCenter += m_members[i].Get()->GetEntity()->GetWorldPosition();	
				++membersCount;
			}
		}
	}
	
	if( membersCount == 0 )
		return combatLine;
	
	teamCenter /= (Float)membersCount;

	Uint32 visibleEnemiesCount = 0;
	const TDynArray< SR6SharedEnemyInfo >& enemies = m_teamSharedKnowladge->GetEnemiesInfo();
	Vector enemiesCenter( 0, 0, 0 );
	for( Uint32 i=0; i<enemies.Size(); ++i )
	{
		enemiesCenter += enemies[i].GetKnownPosition();
		++visibleEnemiesCount;		
	}	
	
	if( visibleEnemiesCount == 0 )
	{
		enemiesCenter = teamCenter;
	}
	else
	{
		enemiesCenter /= (Float)(visibleEnemiesCount);
	}
	

	Vector centersDistance = enemiesCenter - teamCenter;	

	combatLine.m_position			= teamCenter + centersDistance * linePosition;	
	combatLine.m_normalDirection	= centersDistance;
	combatLine.m_lineDirection		= Vector( centersDistance.Y, -centersDistance.X, 0 );
	combatLine.m_teamCenter			= teamCenter;
	combatLine.m_enemiesCenter		= enemiesCenter;

	combatLine.m_normalDirection	.Normalize3();	
	combatLine.m_lineDirection		.Normalize3();	

	combatLine.m_isValid			= true;

	return combatLine;
}

Vector	CTeam::ProjectPointOnCombatLine( const SCombatLine& combatLine, const Vector& projectedPoint, Bool onNavMesh )
{
	Vector projection = projectedPoint.NearestPointOnEdge( combatLine.m_position - combatLine.m_lineDirection*COMBAT_LINE_LENGHT, combatLine.m_position + combatLine.m_lineDirection*COMBAT_LINE_LENGHT );
		//projectedPoint.Project( combatLine.m_position, combatLine.m_normalDirection );
	
	if( onNavMesh )
	{
		CPathLibWorld* pathLibWorld = GGame->GetActiveWorld()->GetPathLibWorld();

		if( !pathLibWorld )
		{
			return projection;
		}
		PathLib::AreaId areaId = pathLibWorld->GetReadyAreaAtPosition( projection.AsVector3() );
		/*if( pathLibWorld->TestLocation( areaId, projection.AsVector3(), pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )
		{
			return projection;
		}*/
		Vector3 out;
		if( pathLibWorld->FindSafeSpot( areaId, projection.AsVector3(), 5, pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), out ) )
		{
			projection = out;
		}
	}
	return projection;
}
SCombatAlley CTeam:: CalculateCombatAlleyBasedOnCombatLine( const SCombatLine& combatLine, Float minMargin )
{
	SCombatAlley combatAlley;
	if( !combatLine.m_isValid )
		return combatAlley;

	if( !combatLine.m_isValid )
		return combatAlley;

	combatAlley.m_usedCombatLine = combatLine;

	Vector positiveEdge;
	Vector negativeEdge;
	Float positiveWidth = -1;
	Float negativeWidth = -1;

	Vector combatLineDirection	= combatLine.m_lineDirection;
	Vector center				= combatLine.m_position; 

	const TDynArray< SR6SharedEnemyInfo >& enemies = m_teamSharedKnowladge->GetEnemiesInfo();

	for( Uint32 i=0; i<enemies.Size(); ++i )
	{		
		Vector knownPositionProjected	= ProjectPointOnCombatLine( combatLine, enemies[i].GetKnownPosition(), false );		
		Vector direction				= knownPositionProjected - center;
		Float width						= direction.Mag3();

		direction.Normalize3();

		if( Vector::Dot3( combatLineDirection, direction ) > 0 )
		{			
			if( positiveWidth < 0 || width > positiveWidth )
			{
				positiveEdge	= knownPositionProjected;
				positiveWidth	= width;
			}
		}
		else
		{
			if( negativeWidth < 0 || width > negativeWidth )
			{
				negativeEdge	= knownPositionProjected;
				negativeWidth	= width;
			}
		}

	}

	if( positiveWidth>0 || negativeWidth>0 )
	{
		if( positiveWidth < 0 )
		{
			positiveWidth	= negativeWidth;
			positiveEdge	= negativeEdge;
		}
		else if( negativeWidth < 0 )
		{
			negativeWidth	= positiveWidth;
			negativeEdge	= positiveEdge;
		}

		Float totalWidth = positiveWidth + negativeWidth;

		Float margin = Max< Float >( totalWidth*COMBAT_ALLEY_MARGIN_PERCENTAGE, minMargin );
		positiveEdge = positiveEdge + combatLineDirection * margin;
		negativeEdge = negativeEdge - combatLineDirection * margin;

		combatAlley.m_positiveEdgePosition	= positiveEdge;
		combatAlley.m_negativeEdgePosition	= negativeEdge;
		combatAlley.m_positiveWidth			= positiveWidth + margin; 
		combatAlley.m_negativeWidth			= -negativeWidth - margin;
		combatAlley.m_isValid				= true;
	}

	return combatAlley;

}
SCombatAlley CTeam::CalculateCombatAlley( Float minMargin )
{	
	SCombatLine combatLine = CalculateCombatLine( 1 );
	return CalculateCombatAlleyBasedOnCombatLine( combatLine, minMargin );
}

Vector CTeam::FindNearesrPointOutsideCombatAlley( const SCombatAlley& combatAlley, const Vector& sourcePosition, Bool skipIfOutside )
{
	if( !combatAlley.m_isValid )
		return sourcePosition;

	Vector positionProjectedOnCombatLine	= ProjectPointOnCombatLine( combatAlley.m_usedCombatLine, sourcePosition, false );
	Vector projectedSourceDirection			= positionProjectedOnCombatLine - combatAlley.m_usedCombatLine.m_position;

	Float projectedSourceDistance = projectedSourceDirection.Mag3();

	projectedSourceDirection.Normalize3();

	Float dotProduct = Vector::Dot3( combatAlley.m_usedCombatLine.m_lineDirection, projectedSourceDirection );
	Float edgeDistance = 0;
	if( dotProduct > 0 )
	{
		edgeDistance = combatAlley.m_positiveWidth;
	}
	else
	{
		edgeDistance = combatAlley.m_negativeWidth;
	}

	if( skipIfOutside && projectedSourceDistance >= edgeDistance )
		return sourcePosition;

	Vector nearestEdgePosition;
	if( dotProduct > 0 )
	{
		nearestEdgePosition = combatAlley.m_positiveEdgePosition;
	}
	else
	{
		nearestEdgePosition = combatAlley.m_negativeEdgePosition;
	}

	Vector nearestPointOutside = sourcePosition.NearestPointOnEdge( 
						nearestEdgePosition - combatAlley.m_usedCombatLine.m_normalDirection*COMBAT_LINE_LENGHT, 
						nearestEdgePosition + combatAlley.m_usedCombatLine.m_normalDirection*COMBAT_LINE_LENGHT 
					);

	return nearestPointOutside;
}

Bool CTeam::AcquireMovementTicket( CTeamMemberComponent* member )
{
	if( member->GetMovementTickedAcquired() )
		return true;

	if( m_movementTicketsLeft > 0 )
	{
		--m_movementTicketsLeft;
		member->SetMovementTickedAcquired( true );
		return true;
	}

	return false;
}

void CTeam::ReturnMovementTicket( CTeamMemberComponent* member )
{
	if( !member->GetMovementTickedAcquired() )
		return;

	++m_movementTicketsLeft;
	member->SetMovementTickedAcquired( false );
}

void CTeam::CleanupNullMembers()
{
	for( Uint32 i=0; i<m_members.Size(); ++i )
	{
		if( !m_members[i].Get()  )
		{
			m_members.EraseFast( m_members.Begin() + i );
			--i;
		}		
	}
}

void CTeam::Tick( Float timeDelta )
{
	//CleanupNullMembers();
	if( m_teamSharedKnowladge )
	{
		m_teamSharedKnowladge->Tick( timeDelta );
	}
}

void CTeam::funcCalculateCombatLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, linePosition, 0 );
	FINISH_PARAMETERS;

	SCombatLine combatLine = CalculateCombatLine( linePosition );

	RETURN_STRUCT( SCombatLine, combatLine );
}

void CTeam::funcCalculateCombatAlley( CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER( Float, minMargin, 0 );
	FINISH_PARAMETERS;

	SCombatAlley combatAlley = CalculateCombatAlley( minMargin );

	RETURN_STRUCT( SCombatAlley, combatAlley );
}

void CTeam::funcCalculateCombatLineByDistance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, distance, 0 );
	FINISH_PARAMETERS;

	SCombatLine combatLine = CalculateCombatLine( 0 );
	combatLine.m_position = combatLine.m_teamCenter + combatLine.m_normalDirection * distance;

	RETURN_STRUCT( SCombatLine, combatLine );
}

void CTeam::funcProjectPointOnCombatLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SCombatLine	, combatLine	, SCombatLine() );
	GET_PARAMETER( Vector		, projectedPoint, Vector::ZEROS );
	GET_PARAMETER( Bool			, useNavMesh	, true			);
	FINISH_PARAMETERS;

	Vector projection = ProjectPointOnCombatLine( combatLine, projectedPoint, useNavMesh );

	RETURN_STRUCT( Vector, projection );
}

void CTeam::funcFindNearestPointOutsideCombatAlley( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SCombatAlley	, combatAlley	, SCombatAlley() );
	GET_PARAMETER( Vector		, projectedPoint, Vector::ZEROS );	
	GET_PARAMETER( Bool			, skipIfOutside	, true			);
	FINISH_PARAMETERS;

	Vector nearestPoint = FindNearesrPointOutsideCombatAlley( combatAlley, projectedPoint, skipIfOutside );

	RETURN_STRUCT( Vector, nearestPoint );
}

void CTeam::funcCalculateCombatAlleyBasedOnCombatLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SCombatLine	, combatLine	, SCombatLine() );
	GET_PARAMETER( Float, minMargin, 0 );
	FINISH_PARAMETERS;

	SCombatAlley combatAlley = CalculateCombatAlleyBasedOnCombatLine( combatLine, minMargin );

	RETURN_STRUCT( SCombatAlley, combatAlley );
}

void CTeam::funcCalculateCombatLineFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, linePosition, 0 );
	GET_PARAMETER( Int32, subteamFlag, 0 );
	FINISH_PARAMETERS;

	SCombatLine combatLine = CalculateCombatLine( linePosition, subteamFlag );

	RETURN_STRUCT( SCombatLine, combatLine );
}

void CTeam::funcCalculateCombatLineByDistanceFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, distance, 0 );
	GET_PARAMETER( Int32, subteamFlag, 0 );
	FINISH_PARAMETERS;

	SCombatLine combatLine = CalculateCombatLine( 0, subteamFlag );
	combatLine.m_position = combatLine.m_teamCenter + combatLine.m_normalDirection * distance;

	RETURN_STRUCT( SCombatLine, combatLine );
}