#include "build.h"

#include "combatUtils.h"
#include "teamManager.h"
#include "../../common/engine/pathlibWorld.h"

IMPLEMENT_ENGINE_CLASS( SRestrictionArea );
IMPLEMENT_ENGINE_CLASS( CCombatUtils );

const Float	CCombatUtils::FIND_FLANKING_ANGLE_STEP	= 30;	


Bool SRestrictionArea::IsInside( const Vector& sourcePosition )
{		
	/*Vector side( m_forwardDirection.Y, -m_forwardDirection.X, m_forwardDirection.Z );

	Vector forwardlProjected	= sourcePosition.NearestPointOnEdge( m_center - m_forwardDirection*2*m_forwardLength	, m_center + forwardlProjected*2*m_forwardLength );
	Vector sideProjected		= sourcePosition.NearestPointOnEdge( m_center - side*2*m_sideLength		, m_center + side*2*m_sideLength );

	Vector forwardDirection = forwardlProjected - m_center;
	Vector sideDirection	= sideProjected		- m_center;

	Float forwardDistance	=  forwardDirection.Mag3();
	Float sideDistance		=  sideDirection.Mag3();

	return ( forwardDistance <= m_forwardLength && sideDistance <= m_sideLength );
	*/

	Vector sideDirection = Vector( m_forwardDirection.Y, -m_forwardDirection.X, m_forwardDirection.Z );
	Vector v1 = m_center + m_forwardDirection * m_forwardLength + sideDirection * m_sideLength;
	Vector v2 = m_center - m_forwardDirection * m_forwardLength + sideDirection * m_sideLength;
	Vector v3 = m_center - m_forwardDirection * m_forwardLength - sideDirection * m_sideLength;
	Vector v4 = m_center + m_forwardDirection * m_forwardLength - sideDirection * m_sideLength;
	
	Vector v4v1Dir = v1 - v4;  
	Vector v1v2Dir = v2 - v1;  
	Vector v2v3Dir = v3 - v2;  
	Vector v3v4Dir = v4 - v3;  

	return ( Vector::Dot3( v4v1Dir, sourcePosition - v1 ) <= 0 ) 
		&& ( Vector::Dot3( v1v2Dir, sourcePosition - v2 ) <= 0 ) 
		&& ( Vector::Dot3( v2v3Dir, sourcePosition - v3 ) <= 0 ) 
		&& ( Vector::Dot3( v3v4Dir, sourcePosition - v4 ) <= 0 );
		
}

void CCombatUtils::Initialize()
{
	m_teamManager	= CreateObject< CTeamManager >( this );	

	m_teamManager->Initialize();
}

void CCombatUtils::RotateVector( Vector& toRotate, Float angle )
{
	Float sina = sinf( DEG2RAD( angle ) );
	Float cosa = cosf( DEG2RAD( angle ) );

	Float sinb = toRotate.Y;
	Float cosb = toRotate.X;

	Float cosab = cosb*cosa - sinb*sina;
	toRotate.X = cosab;

	Float sinab = sina*cosb + cosa*sinb;
	toRotate.Y = sinab;		

	toRotate.Normalize3();
}

Bool CCombatUtils::TestLOS( const Vector& startPosition, const Vector& endPosition )
{
	SPhysicsContactInfo contactInfo;

	CPhysicsEngine::CollisionMask LOScollisionMask	= GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
	if( !m_physicsWorld )
	{
		m_LOScollisionMask	= GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
		m_physicsWorld		= GGame->GetActiveWorld()->GetPhysicsWorld();
	}
	if( !m_physicsWorld )
		return true;

	return  m_physicsWorld->RayCastWithSingleResult( startPosition, endPosition, LOScollisionMask, 0, contactInfo ) != TRV_Hit;
}

Vector CCombatUtils::ClampToArea( const SRestrictionArea& restrictionArea, const Vector& sourcePosition )
{
	const Vector& center = restrictionArea.m_center;
	const Vector& normal = restrictionArea.m_forwardDirection;	
	Vector side( normal.Y, -normal.X, normal.Z );

	Vector forwardlProjected	= sourcePosition.NearestPointOnEdge( center - normal*2*restrictionArea.m_forwardLength	, center + normal*2*restrictionArea.m_forwardLength );
	Vector sideProjected		= sourcePosition.NearestPointOnEdge( center - side*2*restrictionArea.m_sideLength		, center + side*2*restrictionArea.m_sideLength );
	
	Vector forwardDirection = forwardlProjected - center;
	Vector sideDirection	= sideProjected		- center;

	Float forwardDistance	=  forwardDirection.Mag3();
	Float sideDistance		=  sideDirection.Mag3();

	if( forwardDistance <= restrictionArea.m_forwardLength && sideDistance <= restrictionArea.m_sideLength )
	{
		return sourcePosition;
	}

	Vector clamped = center;
	forwardDirection.Normalize3();
	sideDirection.Normalize3();

	if( forwardDistance > restrictionArea.m_forwardLength )
	{
		clamped += forwardDirection*restrictionArea.m_forwardLength;
	}
	if( sideDistance > restrictionArea.m_sideLength )
	{
		clamped += sideDirection*restrictionArea.m_sideLength;
	}

	return clamped;
	
}

void CCombatUtils::funcFindFlankingPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER		( Vector	, currentPosition	, Vector::ZEROS );
	GET_PARAMETER		( Vector	, enemyPosition		, Vector::ZEROS );
	GET_PARAMETER		( Float		, minDistanceToEnemy, 0				);
	GET_PARAMETER_REF	( Vector	, foundPosition		, Vector::ZEROS );
	FINISH_PARAMETERS;

	Vector directionFromEnemy = currentPosition - enemyPosition;
	directionFromEnemy.Normalize3();

	Vector currentLeftDirection = directionFromEnemy, currentRightDirection = directionFromEnemy;
	Int32 maxSteps = ( Int32 )( 360/FIND_FLANKING_ANGLE_STEP );
	
	Bool positionFound = false;

	for( Int32 i=0; i<maxSteps; ++i )
	{
		Vector currentDirection;
		if( i%2 == 0 )
		{
			RotateVector( currentLeftDirection, FIND_FLANKING_ANGLE_STEP );
			currentDirection = currentLeftDirection;
		}
		else
		{
			RotateVector( currentRightDirection, -FIND_FLANKING_ANGLE_STEP );
			currentDirection = currentRightDirection;
		}
		foundPosition = enemyPosition + currentDirection*minDistanceToEnemy;
		if( TestLOS( enemyPosition, foundPosition ) )
		{			
			positionFound = true;
			break;
		}
	}

	RETURN_BOOL( positionFound );
}

void CCombatUtils::funcGetTeamManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	THandle< CTeamManager > teamManagerHandle = m_teamManager;

	RETURN_HANDLE( CTeamManager, teamManagerHandle );
}

void CCombatUtils::funcGetNearestOnNavMesh	( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	FINISH_PARAMETERS;

	CPathLibWorld* pathLibWorld = GGame->GetActiveWorld()->GetPathLibWorld();
	Vector targetOnNavMesh = target;
	if( pathLibWorld )
	{			
		PathLib::AreaId areaId = pathLibWorld->GetReadyAreaAtPosition( target.AsVector3() );
		//if( !pathLibWorld->TestLocation( areaId, targetOnNavMesh.AsVector3(), pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )
		{		
			Vector3 out;
			if( pathLibWorld->FindSafeSpot( areaId, target.AsVector3(), 5, pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), out ) )
			{
				targetOnNavMesh = out;
			}
		}
	}

	RETURN_STRUCT( Vector, targetOnNavMesh );
}

void CCombatUtils::funcTestLOS( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, source, Vector::ZEROS );
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	FINISH_PARAMETERS;

	Bool pass = TestLOS( source, target );
	RETURN_BOOL( pass );
}

void CCombatUtils::funcClampToArea( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SRestrictionArea	, restrictionArea	, SRestrictionArea()	);
	GET_PARAMETER( Vector			, source			, Vector::ZEROS			);
	FINISH_PARAMETERS;

	Vector clamped = ClampToArea( restrictionArea, source );
	
	RETURN_STRUCT( Vector, clamped );
}