#include "build.h"

#include "enemyAwareComponent.h"
#include "coversManager.h"
#include "combatUtils.h"

#include "..\..\common\game\nodeStorage.h"
#include "..\..\common\engine\physicsWorld.h"
#include "..\..\common\engine\physicsBodyWrapper.h"

CCoversManager::CCoversManager()
{
	m_coverPointsStorage = CreateObject< CNodesBinaryStorage >( this );
}
void CCoversManager::AddCoverPoint( CCoverPointComponent* coverPoint )
{
	m_coverPointsStorage->Add( coverPoint );
	m_covers.PushBack( coverPoint );
}
void CCoversManager::RemoveCoverPoint( CCoverPointComponent* coverPoint )
{
	m_coverPointsStorage->Remove(coverPoint);
	m_covers.Remove( coverPoint );
}

void CCoversManager::Initialize()
{
	for( Uint32 i=0; i<m_covers.Size(); ++i )
	{
		if( m_covers[i].Get() )
		{
			m_covers[i].Get()->CalculateCoverDirectionsAll();
		}		
	}
}

CCoverPointComponent* CCoversManager::GetNearestCover( Vector& coverAgainst, Vector& nearPosition, Float range )
{
	Vector minBound( -range, -range, -range );
	Vector maxBound( range, range, range );
	TDynArray< THandle< CNode > > entsInRange;

	const Box bound( minBound, maxBound );

	m_coverPointsStorage->Query( nearPosition, entsInRange, bound, true, INT_MAX, NULL, 0 );

	CCoverPointComponent* nearestCoverWithoutShoot = NULL;
	CCoverPointComponent* nearestCoverWithShoot = NULL;

	Float nearestCoverDistanceWithoutShoot = 0;
	Float nearestCoverDistanceWithShoot = 0;	

	for( Int32 i=0, end=entsInRange.Size(); i<end; ++i )
	{
		CNode* coverNode = entsInRange[i].Get();
		if( coverNode )
		{
			CCoverPointComponent* cmp = static_cast< CCoverPointComponent* >( coverNode );
			if( !cmp->IsFree() )
			{
				continue;
			}
			if( cmp->IsCoverAgainst( coverAgainst ) )
			{
				Float distanceSqrt = cmp->GetWorldPosition().DistanceSquaredTo( nearPosition );

				if( cmp->CanShootTo( coverAgainst )!=CAD_NONE )
				{					
					if( nearestCoverWithShoot==NULL || nearestCoverDistanceWithShoot>distanceSqrt )
					{
						nearestCoverWithShoot = cmp;
						nearestCoverDistanceWithShoot = distanceSqrt;
					}	
				}
				else
				{					
					if( nearestCoverWithoutShoot==NULL || nearestCoverDistanceWithoutShoot>distanceSqrt )
					{
						nearestCoverWithoutShoot = cmp;
						nearestCoverDistanceWithoutShoot = distanceSqrt;
					}						
				}
			}
		}
	}

	return nearestCoverWithShoot ? nearestCoverWithShoot : nearestCoverWithoutShoot;

}
Float CCoversManager::CountRaitingForCover( const TDynArray< SR6EnemyInfo >& enemies, CCoverPointComponent* cover, const Vector& nearPostion, SCoverRatingsWeights& wieghts, Float range )
{
	if( !cover )
		return -1;
	
	if( !cover->IsCoverAgainst( enemies, wieghts.m_minDistanceFromEnemies, wieghts.m_minPercentageOfAvoidedEnemy ) )
		return -1;

	CEntity* mainTarget = wieghts.m_mainTarget.Get();
	if( wieghts.m_forceShootingOpportunity && mainTarget && !cover->CanShootTo( mainTarget->GetWorldPosition() ) )
		return -1;

	Float shootToMainRating			= 0; 
	Float coverFromMainRating		= 0;
	Float coverRating				= cover->CountCoverRating( enemies, NULL /* wieghts.m_mainTarget.Get()*/, &coverFromMainRating );
	Float shootRating				= cover->CountShootRating( enemies, NULL /*wieghts.m_mainTarget.Get()*/, &shootToMainRating );		
	Float distanceRating			= cover->CountDistanceFromNPCRating( range, nearPostion );
	Float optimalDistanceRating		= cover->CountOptimalDistanceRating( wieghts.m_optimalDistance, NULL /*wieghts.m_mainTarget.Get()*/ );

	return	  coverRating			* wieghts.m_cover 
			+ distanceRating		* wieghts.m_distanceFromNPC 
			+ shootRating			* wieghts.m_shoot 
			+ optimalDistanceRating	* wieghts.m_optimalDistanceRaiting;
		//	+ shootToMainRating		* wieghts.m_shootToMainRaiting
		//	+ coverFromMainRating	* wieghts.m_ceveredFromMainRaiting;
}

CCoverPointComponent* CCoversManager::GetBestCoverForNPC( CNewNPC* npc, Float range, SCoverRatingsWeights& wieghts, CCoverPointComponent* currentCover )
{
	return GetBestCoverForNPCInVicinity( npc, npc->GetWorldPosition(), range, wieghts, currentCover );
}

CCoverPointComponent* CCoversManager::GetBestCoverForNPCInVicinity( CNewNPC* npc, const Vector& position, Float range, SCoverRatingsWeights& wieghts, CCoverPointComponent* currentCover, SRestrictionArea* restrictionArea /* = NULL */  )
{		
	CEnemyAwareComponent* enemyAwareComponent = static_cast< CEnemyAwareComponent* >( npc->FindComponent< CEnemyAwareComponent >() );
	if( !enemyAwareComponent )
		return NULL;

	const TDynArray< SR6EnemyInfo >& enemies = enemyAwareComponent->GetEnemiesInfo();
	//LOG_R6( TXT("GetBestCoverForNPC enemies count = %i"), enemies.Size() );
	Vector minBound( -range, -range, -range );
	Vector maxBound( range, range, range );
	TDynArray< THandle< CNode > > entsInRange;
	const Box bound( minBound, maxBound );

	m_coverPointsStorage->Query( position, entsInRange, bound, true, INT_MAX, NULL, 0 );	

	CCoverPointComponent* bestRatedCover = NULL;

	Float bestRating = 0;	
	Float currentCoverRatting =0;

	if( currentCover && restrictionArea && !restrictionArea->IsInside( currentCover->GetCoverPosition() ) )
		currentCover = NULL;

	if( currentCover && currentCover->IsActive() )
		currentCoverRatting = CountRaitingForCover( enemies, currentCover, position, wieghts, range );

	if( currentCoverRatting<0 )
		currentCover = NULL;

 	for( Int32 i=0, end=entsInRange.Size(); i<end; ++i )
	{
		CNode* coverNode = entsInRange[i].Get( );
		if( coverNode )
		{
			CCoverPointComponent* cmp = static_cast< CCoverPointComponent* >( coverNode );

			if( restrictionArea && !restrictionArea->IsInside( cmp->GetCoverPosition() ) )
				continue;

			if( !cmp->IsFree() && npc!=cmp->GetLockedBy() )
				continue;


			Float rating = CountRaitingForCover(enemies, cmp, position, wieghts, range );
			if( rating < 0 )
				continue;

			if( bestRatedCover==NULL || bestRating<rating )
			{
				bestRatedCover = cmp;
				bestRating = rating;							
			}			
		}
	}
	if( bestRating<=currentCoverRatting )
		return currentCover;
	return bestRatedCover;
}

CCoverPointComponent* CCoversManager::GetSafestCoverInVisinity( CNewNPC* npc, const Vector& position, Float range, CCoverPointComponent* currentCover )
{
	SCoverRatingsWeights weights;

	weights.Reset();

	weights.m_minPercentageOfAvoidedEnemy	= 0.8f;
	weights.m_cover							= 1;
	weights.m_distanceFromNPC				= 1;
	weights.m_minDistanceFromEnemies		= 5;
	return GetBestCoverForNPCInVicinity( npc, npc->GetWorldPosition(), range, weights, currentCover );
}

CCoverPointComponent* CCoversManager::FindFlankingCover(  Vector& flankedPosition, Vector& nearPosition, Float range )
{	

	Vector minBound( -range, -range, -range );
	Vector maxBound( range, range, range );
	TDynArray< THandle< CNode > > entsInRange;
	
	const Box bound( minBound, maxBound );

	m_coverPointsStorage->Query( nearPosition, entsInRange, bound, true, INT_MAX, NULL, 0 );


	CCoverPointComponent* bestFlankingCover = NULL;
	Float bestFlankingRatting=0;

	for( Int32 i=0, end=entsInRange.Size(); i<end; ++i )
	{
		CNode* coverNode = entsInRange[i].Get( );
		if( coverNode )
		{
			CCoverPointComponent* cmp = static_cast< CCoverPointComponent* >( coverNode );
			if( !cmp->IsFree() )
				continue;


			Float rating =  cmp->CountFlankRating( flankedPosition );
			if( rating>0 )
			{
				if( bestFlankingCover==NULL || bestFlankingRatting<rating )
				{
					bestFlankingCover = cmp;
					bestFlankingRatting = rating;							
				}
			}
		}
	}
	return bestFlankingCover;
	//return NULL;
}


void CCoversManager::funcGetNearestCover(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, threatPosition, Vector::ZEROS );
	GET_PARAMETER( Vector, aroundPosition, Vector::ZEROS );
	GET_PARAMETER( Float, range, 20 );
	FINISH_PARAMETERS;

	CCoverPointComponent* retCover = GetNearestCover( threatPosition, aroundPosition, range );

	RETURN_HANDLE( CCoverPointComponent, retCover );
}

void CCoversManager::funcGetBestCoverForNPCInVicinity(  CScriptStackFrame& stack, void* result )
{
	SCoverRatingsWeights w;

	GET_PARAMETER( THandle<CNewNPC>,	npc,		NULL			);
	GET_PARAMETER( Vector,				inVisinity, Vector::ZEROS	);
	GET_PARAMETER( Float,				range,		20				);

	GET_PARAMETER_REF( SCoverRatingsWeights,			wieghts,		w		);
	GET_PARAMETER_REF( THandle<CCoverPointComponent>,	currentCover,	NULL	);
	
	FINISH_PARAMETERS;

	CCoverPointComponent* retCover = GetBestCoverForNPCInVicinity( npc.Get(), inVisinity, range, wieghts, currentCover.Get() );

	RETURN_HANDLE( CCoverPointComponent, retCover );

}

void CCoversManager::funcGetBestCoverForNPCInVicinityRestriction(  CScriptStackFrame& stack, void* result )
{
	SCoverRatingsWeights w;

	GET_PARAMETER( THandle<CNewNPC>					, npc				, NULL				);
	GET_PARAMETER( Vector							, inVisinity		, Vector::ZEROS		);
	GET_PARAMETER( Float							, range				, 20				);	
	GET_PARAMETER_REF( SCoverRatingsWeights			, wieghts			, w					);
	GET_PARAMETER_REF( THandle<CCoverPointComponent>,currentCover		, NULL				);
	GET_PARAMETER( SRestrictionArea					, restrictionArea	, SRestrictionArea());

	FINISH_PARAMETERS;

	CCoverPointComponent* retCover = GetBestCoverForNPCInVicinity( npc.Get(), inVisinity, range, wieghts, currentCover.Get(), &restrictionArea );

	RETURN_HANDLE( CCoverPointComponent, retCover );

}

void CCoversManager::funcGetSafestCoverInVicinity(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNewNPC>,	npc,		NULL			);
	GET_PARAMETER( Vector,				inVisinity, Vector::ZEROS	);
	GET_PARAMETER( Float,				range,		20				);
	GET_PARAMETER_REF( THandle<CCoverPointComponent>,	currentCover,	NULL	);
	FINISH_PARAMETERS;

	CCoverPointComponent* retCover = GetSafestCoverInVisinity( npc.Get(), inVisinity, range, currentCover.Get() );

	RETURN_HANDLE( CCoverPointComponent, retCover );
}
void CCoversManager::funcGetBestCoverForNPC(  CScriptStackFrame& stack, void* result )
{
	SCoverRatingsWeights w;

	GET_PARAMETER( THandle<CNewNPC>, npc, NULL );
	GET_PARAMETER( Float, range, 20 );
	GET_PARAMETER_REF( SCoverRatingsWeights, wieghts, w );
	GET_PARAMETER_REF( THandle<CCoverPointComponent>, currentCover, NULL );
	FINISH_PARAMETERS;

	CCoverPointComponent* retCover = GetBestCoverForNPC( npc.Get(), range, wieghts, currentCover.Get() );

	RETURN_HANDLE( CCoverPointComponent, retCover );
}

void CCoversManager::funcChooseEnemyToShot(  CScriptStackFrame& stack, void* result )
{	
	SCoverAttackInfo ai;
	GET_PARAMETER( THandle<CNewNPC>, npcHandle, NULL );
	GET_PARAMETER_REF( THandle<CCoverPointComponent>, currentCoverHandle, NULL );
	GET_PARAMETER_REF( SCoverAttackInfo, attackInfo, ai );
	FINISH_PARAMETERS;

	CNewNPC* npc = npcHandle.Get();
	CCoverPointComponent* currentCover = currentCoverHandle.Get();
	
	const TDynArray< NewNPCNoticedObject >& enemies = npc->GetNoticedObjects();

	CActor* directShoot = NULL;
	CActor* nearestShoot = NULL;
	Float nearestDistance=0;
	ECoverAttackDirection direction = CAD_NONE;

	for( Uint32 i=0; i<enemies.Size(); ++i )
	{		
		CNewNPC* enemy = Cast< CNewNPC >( enemies[i].m_actorHandle.Get() );
		if( !enemy || !enemy->IsDangerous( npc ) )
			continue;

		if( currentCover )
		{
			direction = currentCover->CanShootTo( enemies[i].GetKnownPosition() );
			if( direction!=CAD_NONE )
			{
				directShoot = enemy;
				break;
			}
		}		
		Float dst = npc->GetWorldPosition().DistanceSquaredTo( enemies[i].GetKnownPosition() );
		if( nearestShoot==NULL || nearestDistance>dst )
		{
			nearestShoot = enemy;
			nearestDistance = dst;
		}
	}

	if( directShoot!=NULL )
	{
		attackInfo.m_attackTarget = directShoot;
	}
	else
	{
		attackInfo.m_attackTarget = nearestShoot;
		if( nearestShoot!=NULL )
		{
			if( currentCover!=NULL )
			{
				direction = currentCover->CalculateNearestDirection( nearestShoot->GetWorldPosition() );
			}
		}
	}
	if( attackInfo.m_attackTarget.Get()==NULL )
	{
		direction = CAD_NONE;		
	}
	attackInfo.m_direction = direction;
	if( attackInfo.m_attackTarget.Get() && ( direction!=CAD_NONE ) )
	{
		attackInfo.m_attackBlend = currentCover->CalculateDirectionBlend( attackInfo.m_attackTarget.Get()->GetWorldPosition(), direction );
	}
}

void CCoversManager::funcFindFlankingCover(  CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER( Vector, flankedPosition, NULL );
	GET_PARAMETER( Vector, nearPosition, NULL );
	GET_PARAMETER( Float, range, 20 );
	FINISH_PARAMETERS;
	CCoverPointComponent* cover = FindFlankingCover( flankedPosition, nearPosition, range );

	RETURN_HANDLE( CCoverPointComponent, cover );
}
void CCoversManager::funcCalculateDirectionBlendToCover(  CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER(Vector, coverDirection, Vector::ZEROS );
	GET_PARAMETER(Vector, npcDirection, Vector::ZEROS );
	FINISH_PARAMETERS;

	Float angle = 0;
	coverDirection = -coverDirection;		
	Vector cross = Vector::Cross( coverDirection, npcDirection );	

	angle = asin( cross.Mag3() );		
	angle/=1.570775f;
	if( cross.Z>0 )
		angle = -angle;	

	Float dot = Vector::Dot3( coverDirection, npcDirection );
	if( dot > 0 )
	{
		angle = angle>0 ? 2 - angle : -2 - angle;
	}

	RETURN_FLOAT( angle );
}

void CCoversManager::funcHasClearLineOfFire(  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity>, shooter, NULL );
	GET_PARAMETER( THandle<CEntity>, target	, NULL );
	FINISH_PARAMETERS;

	Bool hasClerLineOfFire = false;

	if( !( shooter.Get() && target.Get() ) )
	{
		RETURN_BOOL( hasClerLineOfFire );
		return;
	}

	SPhysicsContactInfo contactInfo;
	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );

	if( physicsWorld->RayCastWithSingleResult( shooter.Get()->GetWorldPosition() + Vector3( 0, 0, 0.5f ), target.Get()->GetWorldPosition() + Vector3( 0, 0, 0.5f ), include, 0, contactInfo ) == TRV_Hit )
	{
		CComponent* component = nullptr;
		contactInfo.m_userDataA->GetParent( component );
		CEntity* ent = component->GetEntity();
		
		hasClerLineOfFire = ( ent == target.Get() );
	}
	else
	{
		hasClerLineOfFire = true;
	}

	RETURN_BOOL( hasClerLineOfFire );
}
