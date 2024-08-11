#include "build.h"

#include "enemyAwareComponent.h"
#include "eventRouterComponent.h"

#include "../../common/game/gameplayStorage.h"
#include "../../common/game/actorsManager.h"
#include "../../common/engine/tickManager.h"

IMPLEMENT_ENGINE_CLASS( CEnemyAwareComponent );
IMPLEMENT_ENGINE_CLASS( SR6ScriptedEnemyInfo );
IMPLEMENT_ENGINE_CLASS( SR6VisionParams );

void SR6ScriptedEnemyInfo::Copy( SR6EnemyInfo* enemy )
{
	m_enemy					= enemy->m_enemy;
	m_lastKnownPosition		= enemy->m_lastKnownPosition;
	m_movementDirection		= enemy->m_movementDirection;
	m_lastKnownDirection	= enemy->m_lastKnownDirection;
	m_predictedPosition		= enemy->m_predictedPosition;	
	m_movementSpeed			= enemy->m_movementSpeed;	
	m_isVisible				= enemy->m_isVisible;	
	m_absenceAware			= enemy->m_absenceAware;
}

SR6EnemyInfo::SR6EnemyInfo( CActor* enemy )
	: m_enemy( enemy ), m_isVisible( false ), m_isVisibleThisFrame( false )
{}

void CEnemyAwareComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	
	if( GGame->IsActive() )
	{
		CacheBoneIndex();
		Enable();

		m_LOScollisionMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
	}
}

void CEnemyAwareComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	if( GGame->IsActive() )
	{
		Disable();
		m_enemiesInfo.Clear();
	}
}

void CEnemyAwareComponent::CacheBoneIndex()
{	
	CAnimatedComponent* animated = GetEntity()->GetRootAnimatedComponent();	
	const ISkeletonDataProvider* provider = animated ? animated->QuerySkeletonDataProvider() : NULL;	
	m_visionSourceBoneIndex = provider ? provider->FindBoneByName( m_visionParams.m_boneName ) : -1;
	m_rootAnimatedComponent = animated;
}

void CEnemyAwareComponent::StartTicks()
{
	GetLayer()->GetWorld()->GetTickManager()->AddToGroupDelayed( this, TICK_Main );
}

void CEnemyAwareComponent::StopTicks()
{
	GetLayer()->GetWorld()->GetTickManager()->RemoveFromGroupDelayed( this, TICK_Main );
}

void CEnemyAwareComponent::Enable()
{
	if( !m_enebled )
	{
		m_enebled = true; 
		StartTicks(); 
	}	
}

void CEnemyAwareComponent::Disable()
{
	if( m_enebled )
	{
		m_enebled = false; 
		StopTicks(); 
	}	
}

void CEnemyAwareComponent::RemoveDeathEnemies()
{
	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		if( m_enemiesInfo[i].m_enemy.Get() && !m_enemiesInfo[i].m_enemy.Get()->IsAlive() )
		{
			m_enemiesInfo.EraseFast( m_enemiesInfo.Begin() + i );
			--i;
		}		
	}
}

void CEnemyAwareComponent::ResetThisFrameVisibility()
{
	for( Uint32 i = 0; i<m_enemiesInfo.Size(); ++i )
	{
		m_enemiesInfo[i].m_isVisibleThisFrame = false;
	}
}

void CEnemyAwareComponent::OnTick( Float timeDelta )
{
	PC_SCOPE( CEnemyAwareComponent );

	RemoveDeathEnemies();
	ResetThisFrameVisibility();

	TDynArray< CActor* > output;
	CollectVisibleEnemies( output );
	UpdateKnowledge( output, timeDelta );
}

void CEnemyAwareComponent::UpdateKnowledge( TDynArray< CActor* >& visibleEnemies, Float timeDelta )
{
	EngineTime now = GGame->GetEngineTime();

	for( Uint32 i=0; i<visibleEnemies.Size(); ++i )
	{
		if( visibleEnemies[i] )
		{
			SR6EnemyInfo* enemyInfo = FindEnemyInfo( visibleEnemies[i] );
			
			if( !enemyInfo )
			{
				Uint32 newIdx = static_cast< Uint32 >( m_enemiesInfo.Grow( 1 ) );
				new ( &m_enemiesInfo[newIdx] ) SR6EnemyInfo( visibleEnemies[i] );
				enemyInfo = &m_enemiesInfo[newIdx];
				
				enemyInfo->m_lastKnownPosition =  enemyInfo->m_enemy.Get() ? enemyInfo->m_enemy.Get()->GetWorldPosition() : Vector::ZEROS;
			}
			
			CActor*					enemy	= enemyInfo->m_enemy.Get();
			CMovingAgentComponent*	mac		= enemy->GetMovingAgentComponent();

			Vector movementDirection = enemy->GetWorldPosition() - enemyInfo->m_lastKnownPosition;
			movementDirection.Normalize3();

			enemyInfo->m_isVisibleThisFrame		= true;
			enemyInfo->m_lastKnownPosition		= enemy->GetWorldPosition();
			enemyInfo->m_lastVisibilityTime		= now;				
			enemyInfo->m_movementDirection		= movementDirection;
			enemyInfo->m_lastKnownDirection		= enemy->GetWorldForward();
			enemyInfo->m_movementSpeed			= mac ? mac->GetAbsoluteMoveSpeed() : 0;	
			enemyInfo->m_timeSinceContactLost	= 0;	
			enemyInfo->m_predictedPosition		= enemyInfo->m_lastKnownPosition + movementDirection *  m_predictedPositionDistanceMultipler;		
		}		
	}

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemy = &m_enemiesInfo[i];
		if( enemy->m_isVisibleThisFrame && !enemy->m_isVisible )
		{
			enemy->m_isVisible		= true;
			enemy->m_noticedTime	= now;
			BrodecastEnemyNoticedEvent();		
		}
		if( !enemy->m_isVisibleThisFrame && enemy->m_isVisible )
		{
			//LARGE_INTEGER deltaTime = now.m_time - enemy->m_lastVisibilityTime.m_time;			
			enemy->m_isVisible		= false;
			enemy->m_absenceAware	= false; //not aware yet			
			GetEntity()->CallEvent( CNAME( OnEnemyDisapeare ) );			
		}

		//enemy->m_predictedPosition = enemy->m_lastKnownPosition + enemy->m_movementDirection * enemy->m_movementSpeed * m_predictedPositionDistanceMultipler;

		if( !enemy->m_isVisible && !enemy->m_absenceAware )
		{
			if( IsVisible( enemy->m_predictedPosition ) )
			{
				enemy->m_timeSinceContactLost += timeDelta;
				if( enemy->m_timeSinceContactLost > m_timeToBecomeAbsenceAware  )
				{
					enemy->m_absenceAware = true;
				}
			}			
		}
	}
}

void CEnemyAwareComponent::CollectVisibleEnemies( TDynArray< CActor* >& output )
{
	Vector position		= GetVisionSourcePosition();
	Vector direction	= GetVisionSourceDirection();
	
	GetEnemiesInCone( output, position, direction );
	PerformLOS( output, position, direction );
}

void CEnemyAwareComponent::GetEnemiesInCone(TDynArray< CActor* >& output, Vector& initPosition, Vector& initDirection )
{		
					
	struct ConeAcceptor acceptor( initPosition, initDirection.AsVector2(), m_visionParams.m_rangeAngle, m_visionParams.m_backRange );

	FindGameplayEntitiesInRange< ConeAcceptor >( 
		output, 
		initPosition, 
		m_visionParams.m_rangeMax, 
		FLAG_OnlyAliveActors /*| FLAG_Attitude_Hostile*/, 		
		( CGameplayEntity* )GetEntity(), 
		acceptor 
	);
}

void CEnemyAwareComponent::PerformLOS(TDynArray< CActor* >& enemies, Vector& initPosition, Vector& initDirection )
{	

	for( Uint32 i=0; i<enemies.Size(); ++i )
	{
		if( enemies[i] == GetEntity()  )
		{
			enemies.EraseFast( enemies.Begin() + i );
			--i;
		}
		else if( enemies[i] )
		{
			CActor* thisActor = static_cast< CActor* >( GetEntity() );
			if( !thisActor->IsDangerous( enemies[i] ) ||  !enemies[i]->IsAlive() || !LOSTest( initPosition, enemies[i]->GetHeadPosition() ) )
			{
				enemies.EraseFast( enemies.Begin() + i );
				--i;
			}		
		}
	}

}

Bool CEnemyAwareComponent::IsVisible( const Vector& testedPosition )
{	
	Vector position		= GetVisionSourcePosition();
	Vector direction	= GetVisionSourceDirection();

	struct ConeAcceptor acceptor( position, direction.AsVector2(), m_visionParams.m_rangeAngle, m_visionParams.m_backRange );

	if( acceptor.Accept( testedPosition ) )
	{
		if( LOSTest( position, testedPosition ) )
		{
			return true;
		}
	}

	return false;
}

Vector CEnemyAwareComponent::GetVisionSourceDirection()
{
	if( m_rootAnimatedComponent.Get() && m_visionSourceBoneIndex!= -1 )
	{
	
		Vector retVec = m_rootAnimatedComponent.Get()->GetBoneMatrixWorldSpace( m_visionSourceBoneIndex ).V[1];
		retVec.Z = 0.0f;

		if( retVec.SquareMag2() < 0.001f )
		{
			// If head is pointing up, use entity forward
			retVec = GetEntity()->GetWorldForward();
		}
		else
		{
			retVec.Normalize3();
		}
		return retVec;
	}
	else
	{
		return GetEntity()->GetWorldForward();
	}
}

Bool CEnemyAwareComponent::GetNearestKnownPosition( Vector& outPosition )
{
	Float distanceToNearestSqrt = 0;
	CActor* nearestEnemy = NULL;
	Vector nearestPosition( 0, 0, 0 );
	Vector currentPosition = GetEntity()->GetWorldPosition();
	Bool found = false;

	for( TDynArray< SR6EnemyInfo >::iterator it	= m_enemiesInfo.Begin(); it != m_enemiesInfo.End(); ++it )
	{		
		SR6EnemyInfo* enemyInfo = &(*it);		

		if( enemyInfo && enemyInfo->m_enemy.Get() && ( enemyInfo->m_isVisible || !enemyInfo->m_absenceAware ) )
		{
			found = true;

			Float curentDistanceSqrt = enemyInfo->GetKnownPosition().DistanceSquaredTo( currentPosition );

			if( nearestEnemy == NULL || distanceToNearestSqrt > curentDistanceSqrt )
			{
				nearestEnemy			= enemyInfo->m_enemy.Get();
				distanceToNearestSqrt	= curentDistanceSqrt;
				nearestPosition			= enemyInfo->GetKnownPosition();
			}
		}
	}	

	outPosition = nearestPosition;
	return found;
}

SR6EnemyInfo* CEnemyAwareComponent::GetNearestEnemy( Vector& outPosition )
{
	Float distanceToNearestSqrt = 0;
	SR6EnemyInfo* nearestEnemy = NULL;
	Vector nearestPosition( 0, 0, 0 );	
	Vector currentPosition = GetEntity()->GetWorldPosition();
	Bool found = false;

	for( TDynArray< SR6EnemyInfo >::iterator it	= m_enemiesInfo.Begin(); it != m_enemiesInfo.End(); ++it )
	{		
		SR6EnemyInfo* enemyInfo = &(*it);		

		if( enemyInfo && enemyInfo->m_enemy.Get() && ( enemyInfo->m_isVisible || !enemyInfo->m_absenceAware ) )
		{
			found = true;

			Float curentDistanceSqrt = enemyInfo->GetKnownPosition().DistanceSquaredTo( currentPosition );

			if( nearestEnemy == NULL || distanceToNearestSqrt > curentDistanceSqrt )
			{
				nearestEnemy			= enemyInfo;
				distanceToNearestSqrt	= curentDistanceSqrt;
				nearestPosition			= enemyInfo->GetKnownPosition();
			}
		}
	}	
	
	return nearestEnemy;
}

Bool CEnemyAwareComponent::IsEnemyiesPresent()
{
	Bool enemyPresent = false;

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];
		if( enemyInfo->m_isVisible || !enemyInfo->m_absenceAware )
		{
			enemyPresent = true;
			break;
		}
	}

	return enemyPresent;
}

void CEnemyAwareComponent::BrodecastEnemyNoticedEvent()
{	
	CEventRouterComponent* router = GetEntity()->FindComponent< CEventRouterComponent >();
	if( router )
	{
		router->RouteEvent( CNAME( OnEnemyNoticed ) );
	}
	GetEntity()->CallEvent( CNAME( OnEnemyNoticed ) );		
}
void CEnemyAwareComponent::funcIsEnemyiesPresent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool enemyPresent = IsEnemyiesPresent();
	
	RETURN_BOOL( enemyPresent );
}

void CEnemyAwareComponent::funcCanSeeEnemy		( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool enemyPresent = false;

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];
		if( enemyInfo->m_isVisible )
		{
			enemyPresent = true;
			break;
		}
	}

	RETURN_BOOL( enemyPresent );
}

void CEnemyAwareComponent::funcOnlyKnowAboutEnemy	( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool enemyPresent = false;

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];
		if( !enemyInfo->m_isVisible && !enemyInfo->m_absenceAware )
		{
			enemyPresent = true;
			break;
		}
	}

	RETURN_BOOL( enemyPresent );
}

void CEnemyAwareComponent::funcFindNearestVisibleEnemy	( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float distanceToNearestSqrt = 0;
	CActor* nearestEnemy = NULL;
	Vector currentPosition = GetEntity()->GetWorldPosition();


	for( TDynArray< SR6EnemyInfo >::iterator it	= m_enemiesInfo.Begin(); it != m_enemiesInfo.End(); ++it )
	{		
		SR6EnemyInfo* enemyInfo = &(*it);
		//SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];

		if( enemyInfo && enemyInfo->m_isVisible && enemyInfo->m_enemy.Get() )
		{
			Float curentDistanceSqrt = enemyInfo->m_enemy.Get()->GetWorldPosition().DistanceSquaredTo( currentPosition );
			
			if( nearestEnemy == NULL || distanceToNearestSqrt > curentDistanceSqrt )
			{
				nearestEnemy			= enemyInfo->m_enemy.Get();
				distanceToNearestSqrt	= curentDistanceSqrt;
			}
			
		}
	}	
	//CActor* retActor = ( CActor* )GGame->GetPlayerEntity();
	//RETURN_HANDLE( CActor, retActor );
	RETURN_HANDLE( CActor, nearestEnemy );
}

Int32 CEnemyAwareComponent::FindTheNearestMatch( Bool (*matcher)( SR6EnemyInfo* ) )
{
	Float distanceToNearestSqrt = 0;
	Int32	nearestIndex = -1;
	Vector currentPosition = GetEntity()->GetWorldPosition();

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{		
		SR6EnemyInfo* enemyInfo = &(m_enemiesInfo[i]);
		
		if( enemyInfo && matcher( enemyInfo ) )
		{
			Float curentDistanceSqrt = enemyInfo->m_enemy.Get() ? enemyInfo->m_enemy.Get()->GetWorldPosition().DistanceSquaredTo( currentPosition ) : 10000;

			if( nearestIndex == -1 || distanceToNearestSqrt > curentDistanceSqrt )
			{
				nearestIndex			= i;
				distanceToNearestSqrt	= curentDistanceSqrt;
			}

		}
	}	
	return nearestIndex;
}

void CEnemyAwareComponent::funcNotAwareOfEnemies( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool onlyNotVisibleEnemies = true;

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];
		if( enemyInfo->m_isVisible || !enemyInfo->m_absenceAware )
		{
			onlyNotVisibleEnemies = false;
			break;
		}
	}

	RETURN_BOOL( onlyNotVisibleEnemies );
}

void CEnemyAwareComponent::funcGetNearestKnownPosition	( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector nearestPosition( 0, 0, 0 );

	GetNearestKnownPosition( nearestPosition );
		
	RETURN_STRUCT( Vector, nearestPosition );
}

void CEnemyAwareComponent::funcGetNearestVisibleEnemyInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SR6ScriptedEnemyInfo, outEnemyInfo, SR6ScriptedEnemyInfo() );
	FINISH_PARAMETERS;

	Bool (*visible)( SR6EnemyInfo* ) = []( SR6EnemyInfo* enemyInfo )-> Bool { return enemyInfo->m_isVisible; };

	Int32 nearestIdx = FindTheNearestMatch( visible );
	Bool ret = nearestIdx >= 0 && nearestIdx < (Int32) m_enemiesInfo.Size();
	if( ret )
	{
		outEnemyInfo.Copy( &m_enemiesInfo[ nearestIdx ] );
	}

	RETURN_BOOL( ret );
}

void CEnemyAwareComponent::funcGetNearestKnownEnemyInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SR6ScriptedEnemyInfo, outEnemyInfo, SR6ScriptedEnemyInfo() );
	FINISH_PARAMETERS;

	Bool (*visible)( SR6EnemyInfo* ) = []( SR6EnemyInfo* enemyInfo )-> Bool { return enemyInfo->m_isVisible || !enemyInfo->m_absenceAware; };

	Int32 nearestIdx = FindTheNearestMatch( visible );
	Bool ret = nearestIdx >= 0 && nearestIdx < (Int32) m_enemiesInfo.Size();
	if( ret )
	{
		outEnemyInfo.Copy( &m_enemiesInfo[ nearestIdx ] );
	}

	RETURN_BOOL( ret );
}

void CEnemyAwareComponent::funcGetInfoAboutEnemy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, NULL );
	GET_PARAMETER_REF( SR6ScriptedEnemyInfo, outEnemyInfo, SR6ScriptedEnemyInfo() );
	FINISH_PARAMETERS;

	CActor* actor = actorHandle.Get();

	if( !actor )
	{
		RETURN_BOOL( false );
		return;
	}

	Bool ret = false;

	for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
	{
		SR6EnemyInfo* enemyInfo = &m_enemiesInfo[i];
		if( actor == enemyInfo->m_enemy.Get() )
		{
			outEnemyInfo.Copy( enemyInfo );
			ret = true;
			break;
		}
	}
	
	RETURN_BOOL( ret );
}