#include "build.h"

#include "coverPoint.h"
#include "../../common/engine/pathlibWorld.h"

IMPLEMENT_RTTI_ENUM( ECoverPointBehavior );
IMPLEMENT_RTTI_ENUM( ECoverAttackDirection );
IMPLEMENT_RTTI_ENUM( EPositionInCoverPoint );


IMPLEMENT_ENGINE_CLASS( CCoverPointComponent );
IMPLEMENT_ENGINE_CLASS( CCoversManager );
IMPLEMENT_ENGINE_CLASS( SCoverRatingsWeights );
IMPLEMENT_ENGINE_CLASS( SCoverAttackInfo );

const Float	CCoverPointComponent::STAND_COVER_HEIGHT		= 1.0f;
const Float	CCoverPointComponent::CROUCH_COVER_HEIGHT		= 0.5f;
const Float	CCoverPointComponent::COVER_VALIDATION_RADIUS	= 0.1f;

IRenderResource*	CCoverPointComponent::m_markerValid;
IRenderResource*	CCoverPointComponent::m_markerDestroyied;
IRenderResource*	CCoverPointComponent::m_markerInvalid;

void SCoverRatingsWeights::Reset()
{
	m_cover							= 0;
	m_shoot							= 0;
	m_distanceFromNPC				= 0;	
	m_optimalDistance				= 0;
	m_optimalDistanceRaiting		= 0;	
	m_ceveredFromMainRaiting		= 0;
	m_shootToMainRaiting			= 0;
	m_minDistanceFromEnemies		= 0;
	m_minPercentageOfAvoidedEnemy	= 0;
}

void CCoverPointComponent::InitializeCoverMarker()
{
	struct InitOnce
	{
		InitOnce( )
		{		
			m_markerValid		= CWayPointComponent::CreateAgentMesh( 0.15f, 1.2f, Color::BLUE );		
			m_markerDestroyied	= CWayPointComponent::CreateAgentMesh( 0.15f, 1.2f, Color::GRAY );			
			m_markerInvalid		= CWayPointComponent::CreateAgentMesh( 0.15f, 0.2f, Color::RED );	
		}
	};
	static InitOnce initOncePP;
}

void CCoverPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world ); 
	
	if( !GGame->GetActiveWorld() )
	{
		return;
	}

	CPathLibWorld* pathLibWorld = world->GetPathLibWorld();
	
	if( !pathLibWorld )
	{
		return;
	}
	PathLib::AreaId areaId = pathLibWorld->GetReadyAreaAtPosition( GetWorldPosition().AsVector3() );
	if( !pathLibWorld->TestLocation( areaId, GetWorldPosition().AsVector3(), pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )	
	//if( !pathLibWorld->TestLocation( GetWorldPosition(), COVER_VALIDATION_RADIUS, PathLib::CT_DEFAULT | PathLib::CT_NO_OBSTACLES ) )	
	{
		m_isActive = false;
		//m_normalColor = Color::RED;
		//LOG_R6( TXT("Cover point %s is NOT valid"), GetName() );
	}
	else
	{
		CR6Game* game = static_cast< CR6Game* >( GGame );
		m_isActive = true;
		game->GetCoversManager()->AddCoverPoint( this );
		//CalculateCoverDirectionsAll();

	}
}

void CCoverPointComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );	
	CR6Game* game = static_cast< CR6Game* >( GGame );
	game->GetCoversManager()->RemoveCoverPoint( this );
}

void CCoverPointComponent::CalculateCoverDirectionsAll()
{	
	m_coverPosition = GetWorldPosition();
	
	Vector coverPos = m_coverPosition;
	if( m_coverBehavior == CPB_STAND )
	{
		coverPos.Z = coverPos.Z + STAND_COVER_HEIGHT;
	}
	else
	{
		coverPos.Z = coverPos.Z + CROUCH_COVER_HEIGHT;
	}

	CalculateCoverDirections( coverPos, m_coverDirections );	

	if( m_attackUp )
	{
		Vector forward =  GetWorldForward();
		Vector upPosition = coverPos;
		upPosition.Z = upPosition.Z + 1;		
		CalculateCoverDirections( upPosition, m_upFireDirections );					
	}
	if( m_attackLeft )
	{		
		Vector left =  -GetWorldRight();
		Vector leftPosition = coverPos;

		leftPosition = coverPos + left * m_firePositionDistance;
		//LOG_R6( TXT("Left x,y=%f,%f"),leftPosition.X, leftPosition.Y );
		
		CPathLibWorld* pathLibWorld = GGame->GetActiveWorld()->GetPathLibWorld();
			
		if( pathLibWorld )
		{
			PathLib::AreaId areaId = pathLibWorld->GetReadyAreaAtPosition( GetWorldPosition().AsVector3() );
			if( !pathLibWorld->TestLocation( areaId, GetWorldPosition().AsVector3(), pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )
			//if( !pathLibWorld->TestLocation( GetWorldPosition(), COVER_VALIDATION_RADIUS, PathLib::CT_DEFAULT | PathLib::CT_NO_OBSTACLES ) )
			{
				m_attackLeft = false;
			}
			else
			{
				CalculateCoverDirections( leftPosition, m_leftFireDirections );	
				m_leftPosition = leftPosition;
			}
		}
	}	

	if( m_attackRight )
	{
		Vector right =  GetWorldRight();
		Vector rightPosition = coverPos;

		rightPosition = coverPos + right * m_firePositionDistance;		

		//LOG_R6( TXT("Right x,y=%f,%f"),rightPosition.X, rightPosition.Y );
		
		CPathLibWorld* pathLibWorld = GGame->GetActiveWorld()->GetPathLibWorld();
		if( pathLibWorld )
		{

			PathLib::AreaId areaId = pathLibWorld->GetReadyAreaAtPosition( GetWorldPosition().AsVector3() );
			if( !pathLibWorld->TestLocation( areaId, GetWorldPosition().AsVector3(), pathLibWorld->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )
			//if( !pathLibWorld->TestLocation( GetWorldPosition(), COVER_VALIDATION_RADIUS, PathLib::CT_DEFAULT | PathLib::CT_NO_OBSTACLES ) )	
			{
				m_attackRight = false;
			}
			else
			{
				CalculateCoverDirections( rightPosition, m_rightDirections );	
				m_rightPosition = rightPosition;
			}
		}
	}
}

void CCoverPointComponent::CalculateCoverDirections( const Vector& startPosition, Float* directionsData )
{
	//LOG_R6( TXT("CalculateCoverDirections for %s"), GetName() );
	
	for( Int32 i=0; i<DIRECTIONS; ++i )
	{
		directionsData[i] = MAX_COVER_DISTANCE_TEST*MAX_COVER_DISTANCE_TEST;
	}

	if( !GGame->GetActiveWorld() )
		return;

	SPhysicsContactInfo contactInfo;
	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	if( !physicsWorld )
		return;

	Vector startDirection = GetWorldForward();
	Vector currentDirection = startDirection;	
	Float rotationStep = 6.2831/DIRECTIONS;			
	
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
	
	Float sina = sinf( rotationStep );
	Float cosa = cosf( rotationStep );


	for( Int32 i=0; i<DIRECTIONS; ++i )
	{						
		//currentDirection.X = startDirection.X + cosf( i*rotationStep );
		//currentDirection.Y = startDirection.Y + sinf( i*rotationStep );		

		//currentDirection.Normalize3();

		if( physicsWorld->RayCastWithSingleResult( startPosition, startPosition+currentDirection*MAX_COVER_DISTANCE_TEST, include, 0, contactInfo ) == TRV_Hit )
		{
			//CEntity* ent = contactInfo.m_userDataA->GetParent()->GetEntity();

			Float dstSqr = startPosition.DistanceSquaredTo( contactInfo.m_position );
			directionsData[i] = dstSqr;			
		}	

		Float sinb = currentDirection.Y;
		Float cosb = currentDirection.X;

		Float cosab = cosb*cosa - sinb*sina;
		currentDirection.X = cosab;

		Float sinab = sina*cosb + cosa*sinb;
		currentDirection.Y = sinab;		

		currentDirection.Normalize3();

		//LOG_R6( TXT("cover for direction %i is %f"), i, directionsData[i] );
	}
}

Int32 CCoverPointComponent::FindDirection( const Vector& enemy, Vector& basePos )
{
	Float distanceSqrt = basePos.DistanceSquaredTo( enemy );
	Vector direction = enemy - basePos;			
	direction.Normalize3();
	Float angle = (Float)acos( Vector::Dot3( GetWorldForward(), direction ) );

	Vector cross = Vector::Cross( GetWorldForward(), direction );
	angle+= 3.1415/DIRECTIONS;
	if( cross.Z<0 )
	{
		angle*=-1;
	}
	
	Int32 part = ( Int32 )( angle/(6.2831/DIRECTIONS) );
	if( part<0 )
	{
		part+=DIRECTIONS;
	}
	if( part>=DIRECTIONS )
	{
		part-=DIRECTIONS;
	}
	return part;
}

Bool CCoverPointComponent::IsCoverAgainst( const TDynArray< SR6EnemyInfo >&  enemies, Float minDistanceFromEnemies, Float minPercentageOfAvoidedEnemy )
{
	Int32 covered( 0 ), notCovered( 0 );

	Float distanceSqrt = minDistanceFromEnemies * minDistanceFromEnemies;

	for( Uint32 i=0; i<enemies.Size(); ++i )
	{
		Float distance = m_coverPosition.DistanceSquaredTo( enemies[i].GetKnownPosition()  );
		if( distance <= minDistanceFromEnemies )
			return false;

		if( IsCoverAgainst( enemies[i].GetKnownPosition() ) )
		{
			++covered;
		}
		else
		{
			++notCovered;
		}
	}

	Float coveredPercentages = ( (Float) covered )/( covered + notCovered );

	return coveredPercentages>=minPercentageOfAvoidedEnemy;
}

Bool CCoverPointComponent::IsCoverAgainst( const Vector& enemy )
{
	//LOG_R6( TXT("IsCoverAgainst for %s"), GetName() );

	Float distanceSqrt = m_coverPosition.DistanceSquaredTo( enemy );	

	Int32 part		= FindDirection( enemy, m_coverPosition );	
	Int32 nextPart	= NextPart( part );
	Int32 prevPart	= PrevPart( part );

	Bool isCover = m_coverDirections[part]<distanceSqrt;
	part+=1;
	if( part>=DIRECTIONS )
		part-=DIRECTIONS;

	isCover=isCover&&m_coverDirections[nextPart]<distanceSqrt;

	part-=2;
	if( part<0 )
		part+=DIRECTIONS;

	isCover=isCover&&m_coverDirections[prevPart]<distanceSqrt;

	return isCover;
}

Float CCoverPointComponent::CountCoverRating( const TDynArray< SR6EnemyInfo >&  enemies, CEntity* mainTarget, Float* coverFromMainRatingRet )
{
	static const Float MAX_COVER_SUBSCORE = 10;

	Float raiting				= 0;
	Float coverFromMainRating	= 0;
	Float partCoverMainRating	= 0.11;

	// 0 - unknown
	// 1 - not covered
	// 2 - covered
	char covered[DIRECTIONS];
	for( Int32 i=0; i<DIRECTIONS; ++i )
	{
		covered[i] = 0;
	}
	//Think about Bool notCovered[DIRECTIONS] = {false};
	Vector coverPos =  m_coverPosition;
	for( Uint32 i=0; i<enemies.Size(); ++i )
	{
		const SR6EnemyInfo& enemyInfo = enemies[i];
		
		if( !enemyInfo.GetKnowsPosition() )
			continue;

		Vector enemyPos = enemyInfo.GetKnownPosition(); 
		Int32 part = FindDirection( enemyPos, coverPos );
		Int32 nextPart = NextPart( part );
		Int32 prevPart = PrevPart( part );

		Float distanceSqrt = m_coverPosition.DistanceSquaredTo( enemyPos );

		if( m_coverDirections[part] > distanceSqrt )		
			covered[part] = 1;	// not covered	
		else if( covered[part] != 1 )
			covered[part] = 2; // covered only when it was not uncovered					

		if( m_coverDirections[nextPart] > distanceSqrt )
			covered[nextPart] = 1;
		else if( covered[nextPart] != 1 )
			covered[nextPart] = 2;
		
		if( m_coverDirections[prevPart] > distanceSqrt )
			covered[prevPart] = 1;
		else if( covered[prevPart] != 1 )
			covered[prevPart] = 2;
		
		if( mainTarget && mainTarget==enemyInfo.m_enemy.Get() )
		{
			coverFromMainRating  +=( m_coverDirections[part]	== 2 ? 1 : 0 )	* partCoverMainRating 
								 + ( m_coverDirections[nextPart]== 2 ? 1 : 0 )	* partCoverMainRating 
								 + ( m_coverDirections[prevPart]== 2 ? 1 : 0 )	* partCoverMainRating;
		}
	}

	Int32 notCoveredDirections	= 0;
	Int32 coveredDirections		= 0;

	for( Int32 i=0; i<DIRECTIONS; ++i )
	{
		if( covered[i] == 1 )
		{
			++notCoveredDirections;
		}
		else if( covered[i] == 2 )
		{
			Float subRating =  MAX_COVER_SUBSCORE - m_coverDirections[i] + 1;
			subRating = Clamp( subRating, 1.0f, MAX_COVER_SUBSCORE );
			raiting += subRating;	
			++coveredDirections;
		}
	}

	(*coverFromMainRatingRet) = coverFromMainRating;

	return raiting/ ( coveredDirections * MAX_COVER_SUBSCORE + notCoveredDirections * MAX_COVER_SUBSCORE );
}
Float CCoverPointComponent::CountFlankRating( const Vector& enemyPosition )
{
	Float raiting = 0;	
	if( !IsCoverAgainst( enemyPosition ) )
		return 0;

	if( m_attackLeft )
	{
		Bool leftShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			leftShoot[i] = false;
		}
		
		Int32 part = FindDirection( enemyPosition, m_leftPosition );
		Int32 nextPart = NextPart( part );
		Int32 prevPart = PrevPart( part );

		Float distanceSqrt = m_leftPosition.DistanceSquaredTo( enemyPosition );

		if( m_leftFireDirections[part] > distanceSqrt )
			leftShoot[part] = true;
		if( m_leftFireDirections[nextPart] > distanceSqrt )
			leftShoot[nextPart] = true;
		if( m_leftFireDirections[prevPart] > distanceSqrt )
			leftShoot[prevPart] = true;

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( leftShoot[i] ) ++raiting;
		}
	}
	if( m_attackRight )
	{
		Bool rightShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			rightShoot[i] = false;
		}
		
		Int32 part = FindDirection( enemyPosition, m_rightPosition );
		Int32 nextPart = NextPart( part );
		Int32 prevPart = PrevPart( part );

		Float distanceSqrt = m_rightPosition.DistanceSquaredTo( enemyPosition );

		if( m_rightDirections[part] > distanceSqrt )
			rightShoot[part] = true;
		if( m_rightDirections[nextPart] > distanceSqrt )
			rightShoot[nextPart] = true;
		if( m_rightDirections[prevPart] > distanceSqrt )
			rightShoot[prevPart] = true;	

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( rightShoot[i] ) ++raiting;
		}
	}
	if( m_attackUp )
	{
		Bool upShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			upShoot[i] = false;
		}


		Int32 part = FindDirection( enemyPosition, m_rightPosition );
		Int32 nextPart = NextPart( part );
		Int32 prevPart = PrevPart( part );

		Float distanceSqrt = m_rightPosition.DistanceSquaredTo( enemyPosition );

		if( m_upFireDirections[part] > distanceSqrt )
			upShoot[part] = true;
		if( m_upFireDirections[nextPart] > distanceSqrt )
			upShoot[nextPart] = true;
		if( m_upFireDirections[prevPart] > distanceSqrt )
			upShoot[prevPart] = true;

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( upShoot[i] ) ++raiting;
		}
	}
	return raiting;
}

Float CCoverPointComponent::CountShootRating( const TDynArray<SR6EnemyInfo>&  enemies, CEntity* mainTarget, Float* shootToMainRatingRet )
{
	Float raiting = 0;
	Float shootToMainRating = 0;

	Float partShootMainRating = 0.11;

	if( m_attackLeft )
	{
		Bool leftShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			leftShoot[i] = false;
		}

		for( Uint32 i=0; i<enemies.Size(); ++i )
		{
			const SR6EnemyInfo& enemyInfo = enemies[i];

			if( !enemyInfo.GetKnowsPosition() )
				continue;

			Vector enemyPos = enemyInfo.GetKnownPosition(); 
			Int32 part = FindDirection( enemyPos, m_leftPosition );
			Int32 nextPart = NextPart( part );
			Int32 prevPart = PrevPart( part );

			Float distanceSqrt = m_leftPosition.DistanceSquaredTo( enemyPos );

			if( m_leftFireDirections[part] > distanceSqrt )
				leftShoot[part] = true;
			if( m_leftFireDirections[nextPart] > distanceSqrt )
				leftShoot[nextPart] = true;
			if( m_leftFireDirections[prevPart] > distanceSqrt )
				leftShoot[prevPart] = true;

			if( mainTarget &&  mainTarget==enemyInfo.m_enemy.Get() )
			{
				shootToMainRating   +=( leftShoot[part] ? 1 : 0 )		*partShootMainRating 
									+ ( leftShoot[nextPart] ? 1 : 0 )	*partShootMainRating 
									+ ( leftShoot[prevPart] ? 1 : 0 )	*partShootMainRating;
			}
			
		}

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( leftShoot[i] ) ++raiting;
		}
	}
	if( m_attackRight )
	{
		Bool rightShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			rightShoot[i] = false;
		}

		for( Uint32 i=0; i<enemies.Size(); ++i )
		{
			const SR6EnemyInfo& enemyInfo = enemies[i];

			if( !enemyInfo.GetKnowsPosition() )
				continue;

			Vector enemyPos = enemyInfo.GetKnownPosition(); 
			Int32 part = FindDirection( enemyPos, m_rightPosition );
			Int32 nextPart = NextPart( part );
			Int32 prevPart = PrevPart( part );

			Float distanceSqrt = m_rightPosition.DistanceSquaredTo( enemyPos );

			if( m_rightDirections[part] > distanceSqrt )
				rightShoot[part] = true;
			if( m_rightDirections[nextPart] > distanceSqrt )
				rightShoot[nextPart] = true;
			if( m_rightDirections[prevPart] > distanceSqrt )
				rightShoot[prevPart] = true;

			if( mainTarget && mainTarget==enemyInfo.m_enemy.Get() )
			{
				shootToMainRating   +=( m_rightDirections[part] ? 1 : 0 )		*partShootMainRating 
									+ ( m_rightDirections[nextPart] ? 1 : 0 )	*partShootMainRating 
									+ ( m_rightDirections[prevPart] ? 1 : 0 )	*partShootMainRating;
			}
		}

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( rightShoot[i] ) ++raiting;
		}
	}
	if( m_attackUp )
	{
		Bool upShoot[DIRECTIONS];
		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			upShoot[i] = false;
		}

		for( Uint32 i=0; i<enemies.Size(); ++i )
		{
			const SR6EnemyInfo& enemyInfo = enemies[i];

			if( !enemyInfo.GetKnowsPosition() )
				continue;

			Vector enemyPos = enemyInfo.GetKnownPosition(); 
			Int32 part = FindDirection( enemyPos, m_rightPosition );
			Int32 nextPart = NextPart( part );
			Int32 prevPart = PrevPart( part );

			Float distanceSqrt = m_rightPosition.DistanceSquaredTo( enemyPos );

			if( m_upFireDirections[part] > distanceSqrt )
				upShoot[part] = true;
			if( m_upFireDirections[nextPart] > distanceSqrt )
				upShoot[nextPart] = true;
			if( m_upFireDirections[prevPart] > distanceSqrt )
				upShoot[prevPart] = true;

			if( mainTarget && mainTarget==enemyInfo.m_enemy.Get() )
			{
				shootToMainRating   +=( upShoot[part] ? 1 : 0 )		*partShootMainRating 
									+ ( upShoot[nextPart] ? 1 : 0 )	*partShootMainRating 
									+ ( upShoot[prevPart] ? 1 : 0 )	*partShootMainRating;
			}
		}

		for( Int32 i=0; i<DIRECTIONS; ++i )
		{
			if( upShoot[i] ) ++raiting;
		}
	}
	raiting = raiting/DIRECTIONS;
	raiting = raiting>1 ? 1 : raiting;

	(*shootToMainRatingRet) = shootToMainRating;

	return raiting ;
}

Float CCoverPointComponent::CountDistanceFromNPCRating( Float range, const Vector& npcPosition )
{
	Float distance = GetWorldPosition().DistanceTo( npcPosition );			
	Float distanceRating = range - Min( distance, range );
	distanceRating = distanceRating/range;
	return distanceRating;
}

Float CCoverPointComponent::CountOptimalDistanceRating( Float optimalDistance, CEntity* mainTarget )
{
	if( !mainTarget )
		return 0;
	Float distance	= GetWorldPosition().DistanceTo( mainTarget->GetWorldPosition() );	
	Float delta		= Abs( optimalDistance - distance );
	delta = delta > optimalDistance ? optimalDistance : delta;
	Float rating	= 1 - delta / optimalDistance;
	return rating;
}
Bool CCoverPointComponent::CanShootBack( const Vector& enemyPosition )
{
	Float distanceSqrt = m_coverPosition.DistanceSquaredTo( enemyPosition );
	Vector direction = enemyPosition - m_coverPosition;
	direction.Normalize3();

	Float dot =  Vector::Dot3( GetWorldForward(), direction );
	if( dot>0 )
		return false;
	Float angle = (Float)acos( dot );
	Vector cross = Vector::Cross( GetWorldForward(), direction );	
	angle+= 3.1415/DIRECTIONS;
	if( cross.Z<0 )
	{
		angle*=-1;
	}

	Int32 part = (Int32)( angle/(6.2831/DIRECTIONS) );
	if( part<0 )
	{
		part+=DIRECTIONS;
	}

	if( part<DIRECTIONS)
	{
		return ( part>=4 && part<=9 ) && m_coverDirections[part]>=distanceSqrt;
	}
	return false;
}
Bool CCoverPointComponent::CanShoot( const Vector& enemyPosition, Vector& shootPosition, Float* shootDirections )
{
	Float distanceSqrt = shootPosition.DistanceSquaredTo( enemyPosition );
	
	Int32 part = FindDirection( enemyPosition, shootPosition );
	return shootDirections[part]>=distanceSqrt;
}

ECoverAttackDirection CCoverPointComponent::CanShootTo( const Vector& enemy )
{
	Bool canShootLeft(false), canShootRight(false), canShootUp(false), canShootBack(false);

	canShootBack = CanShootBack( enemy );
	if( canShootBack )
	{
		return CAD_BACK;
	}
	if( m_attackLeft )
	{
		canShootLeft = CanShoot( enemy, m_leftPosition, m_leftFireDirections );					
	}
	if( m_attackRight )
	{
		canShootRight = CanShoot( enemy, m_rightPosition, m_rightDirections );					
	}	
	if( m_attackUp )
	{
		Vector p = m_coverPosition;
		canShootUp = CanShoot( enemy, p, m_upFireDirections );		
	}

	if( canShootLeft && canShootRight )
	{
		Float leftDst = enemy.DistanceSquaredTo( m_leftPosition );
		Float rightDst = enemy.DistanceSquaredTo( m_rightPosition );

		if( leftDst<rightDst )
		{
			return CAD_LEFT;
		}
		else
		{
			return CAD_RIGHT;
		}

	}
	if( canShootLeft )
	{
		return CAD_LEFT;
	}
	if( canShootRight )
	{
		return CAD_RIGHT;
	}
	if( canShootUp )
		return CAD_UP;
	return CAD_NONE;

}


void CCoverPointComponent::funcIsCoverAgainst(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, threatPosition, Vector::ZEROS );
	FINISH_PARAMETERS;
	Bool isCover = IsCoverAgainst( threatPosition );
	RETURN_BOOL( isCover );
}

void CCoverPointComponent::funcGetAttackDirection(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, threatPosition, Vector::ZEROS );
	FINISH_PARAMETERS;
	ECoverAttackDirection direction = CAD_UP;
	if( CanShoot( threatPosition, m_leftPosition, m_leftFireDirections ) )
	{		
		direction = CAD_LEFT;
	}
	if( CanShoot( threatPosition, m_rightPosition, m_rightDirections ) )
	{
		direction = CAD_RIGHT;
	}
	RETURN_INT( direction );
}
Float CCoverPointComponent::CalculateDirectionBlend( const Vector& threatPosition, ECoverAttackDirection directionEnum )
{
	Float angle = 0;
	if( directionEnum==CAD_BACK || directionEnum==CAD_UP )
	{
		Vector forward = -GetWorldForward();	
		Vector direction = threatPosition-m_coverPosition;		
		direction.Normalize3();
		Vector cross = Vector::Cross( forward, direction );	
		angle = asin( cross.Mag3() );		
		angle/=1.570775f;

		if( cross.Z>0 )
			return -angle;
		return angle;		
	}
	if( directionEnum!=CAD_NONE )
	{		
		Vector direction;
		if( directionEnum ==CAD_LEFT )
			direction = threatPosition - m_leftPosition;
		if( directionEnum==CAD_RIGHT )
			direction = threatPosition - m_rightPosition;
		if( directionEnum==CAD_UP )
			direction = threatPosition - m_coverPosition;

		direction.Normalize3();

		angle = (Float)acos( Vector::Dot3( GetWorldForward(), direction ) );		
		angle = 1.570775f - angle;
	}

	Float ret = abs( angle/1.570775f);
	return ret;
}
ECoverAttackDirection CCoverPointComponent::CalculateNearestDirection( const Vector& threatPosition )
{
	Float dstLeft = threatPosition.DistanceSquaredTo( m_leftPosition );
	Float dstRight= threatPosition.DistanceSquaredTo( m_rightPosition );
	Float dstUp = threatPosition.DistanceSquaredTo( m_coverPosition );

	Vector direction = threatPosition - m_coverPosition;
	direction.Normalize3();

	Float dot = Vector::Dot3( GetWorldForward(), direction );
	if( dot<0 )
	{
		return CAD_BACK;
	}
	if( m_attackLeft && dstLeft<=dstRight && dstLeft<=dstUp )
	{
		return CAD_LEFT;
	}
	if( m_attackRight && dstRight<=dstLeft && dstRight<=dstUp )
	{
		return CAD_RIGHT;
	}
	if( m_attackUp && dstUp<=dstLeft && dstUp<=dstRight )
	{
		return CAD_UP;
	}
	return CAD_NONE;	
}
void CCoverPointComponent::funcCalcAttackDirectionBlend(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, threatPosition, Vector::ZEROS );
	GET_PARAMETER( ECoverAttackDirection, directionEnum, CAD_NONE );
	FINISH_PARAMETERS;

	Float ret = CalculateDirectionBlend( threatPosition, directionEnum );

	RETURN_FLOAT( ret );
}

void CCoverPointComponent::funcDeactivateCover( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	m_isActive = false;
	CR6Game* game = static_cast< CR6Game* >( GGame );
	game->GetCoversManager()->RemoveCoverPoint( this );	
}

void CCoverPointComponent::funcGetDistanceToNearestObstacle	(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, testedPosition, Vector::ZEROS );
	GET_PARAMETER( EPositionInCoverPoint, inCoverPosition, EPICP_LeftShootingPoint );
	FINISH_PARAMETERS;

	Float* distances		= m_coverDirections;
	Vector sourcePosition	= m_coverPosition;

	if( inCoverPosition == EPICP_LeftShootingPoint )
	{
		distances		= m_leftFireDirections;
		sourcePosition	= m_leftPosition;
	}
	else if( inCoverPosition == EPICP_RightShootingPoint )
	{
		distances		= m_rightDirections;
		sourcePosition	= m_rightPosition;
	}

	Int32 part		= FindDirection( testedPosition, sourcePosition );
	Float distance	= distances[ part ]; 

	RETURN_FLOAT( distance );
}

void CCoverPointComponent::funcGetShootingPositions(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, leftPosition, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, rightPosition, Vector::ZEROS );
	FINISH_PARAMETERS;

	leftPosition = m_leftPosition;
	rightPosition = m_rightPosition;
}

void CCoverPointComponent::funcCanShootTo(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, enemyPosition, Vector::ZEROS );
	FINISH_PARAMETERS;

	ECoverAttackDirection retDirection = CanShootTo( enemyPosition );

	RETURN_ENUM( retDirection );
}