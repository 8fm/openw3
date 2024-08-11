#pragma once


#include "r6Component.h"
#include "../../common/game/gameplayStorage.h"
#include "../../common/game/actorsManager.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/physics/physicsWorld.h"

struct SR6EnemyInfo
{

	SR6EnemyInfo( ) :  m_isVisible( false ), m_isVisibleThisFrame( false ), m_timeSinceContactLost( 0 ) {}
	SR6EnemyInfo( CActor* enemy );

	THandle<CActor>	m_enemy;
	Vector			m_lastKnownPosition;
	Vector			m_movementDirection;
	Vector			m_lastKnownDirection;
	Vector			m_predictedPosition;
	EngineTime		m_lastVisibilityTime;
	EngineTime		m_noticedTime;
	Float			m_timeSinceContactLost;
	Float			m_movementSpeed;	
	Bool			m_isVisible;
	Bool			m_isVisibleThisFrame;
	Bool			m_absenceAware;

	RED_INLINE Vector GetKnownPosition() const{ return m_enemy.Get() && m_isVisible ? m_enemy.Get()->GetWorldPosition() : m_predictedPosition; }
	RED_INLINE Bool	GetKnowsPosition() const{ return m_isVisible || !m_absenceAware; }
};

struct SR6ScriptedEnemyInfo
{
	DECLARE_RTTI_STRUCT( SR6ScriptedEnemyInfo );

	SR6ScriptedEnemyInfo(){};

	THandle< CActor >	m_enemy;
	Vector				m_lastKnownPosition;
	Vector				m_movementDirection;
	Vector				m_lastKnownDirection;
	Vector				m_predictedPosition;	
	Float				m_movementSpeed;	
	Bool				m_isVisible;	
	Bool				m_absenceAware;

	void Copy( SR6EnemyInfo* enemy );
};

BEGIN_CLASS_RTTI( SR6ScriptedEnemyInfo )	
	PROPERTY_NAME( m_enemy				, TXT("i_enemy")				);
	PROPERTY_NAME( m_lastKnownPosition	, TXT("i_lastKnownPosition")	);
	PROPERTY_NAME( m_movementDirection	, TXT("i_movementDirection")	);
	PROPERTY_NAME( m_lastKnownDirection	, TXT("i_lastKnownDirection")	);
	PROPERTY_NAME( m_predictedPosition	, TXT("i_predictedPosition")	);
	PROPERTY_NAME( m_movementSpeed		, TXT("i_movementSpeed")		);
	PROPERTY_NAME( m_isVisible			, TXT("i_isVisible")			);
	PROPERTY_NAME( m_absenceAware		, TXT("i_absenceAware")			);
END_CLASS_RTTI();

struct SR6VisionParams
{
	DECLARE_RTTI_STRUCT( SR6VisionParams );

	Float	m_rangeMin;
	Float	m_rangeMax;
	Float	m_backRange;
	Float	m_rangeAngle;
	CName	m_boneName;
	CName	m_slotName;
};

BEGIN_CLASS_RTTI( SR6VisionParams )	
	PROPERTY_EDIT( m_rangeMin	, TXT("") );
	PROPERTY_EDIT( m_rangeMax	, TXT("") );
	PROPERTY_EDIT( m_backRange	, TXT("") );
	PROPERTY_EDIT( m_rangeAngle	, TXT("") );
	PROPERTY_EDIT( m_boneName	, TXT("") );
	PROPERTY_EDIT( m_slotName	, TXT("") );
END_CLASS_RTTI();

template < class CustomAcceptor >
static void FindGameplayEntitiesInRange( TDynArray< CActor* >& output, Vector& origin, Float range, Uint32 flags, CGameplayEntity* target, CustomAcceptor& acceptor )
{
	struct Functor : public CGameplayStorage::DefaultFunctor
	{		
		Functor( TDynArray< CActor* >& output, CustomAcceptor& acceptor )
			: m_output( output )			
			, m_acceptor( acceptor ) {}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& data )
		{
			if ( m_acceptor( data ) )
			{
				m_output.PushBack( data.Get() );				
			}
			return true;
		}

		RED_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr )
		{
			/*if ( m_acceptor( ptr ) )
			{
				m_output.PushBack( ptr.Get() );				
			}*/
			return true;
		}

		TDynArray< CActor* >&		m_output;		
		CustomAcceptor&				m_acceptor;

	} functor( output, acceptor );

	CGameplayStorage::SSearchParams searchParams;
	searchParams.m_origin = origin;
	searchParams.m_flags = flags;
	searchParams.m_target = target;
	GCommonGame->GetActorsManager()->TQuery( functor, searchParams );	
};

class CEnemyAwareComponent : public CR6Component
{
	DECLARE_ENGINE_CLASS( CEnemyAwareComponent, CR6Component, 0 );

	struct ConeAcceptor
	{
		ConeAcceptor( const Vector& basePoint, const Vector2& coneDir, Float coneAngle, Float backRange )
			: m_basePos2( basePoint.AsVector2() )
			, m_basePos3( basePoint )
			, m_heading( coneDir )
			, m_backRangeSqrt( backRange*backRange )
		{		
			m_angleDot = cos( DEG2RAD( coneAngle / 2.f ) );
		}

		RED_INLINE Bool Accept( const Vector& tested ) const
		{
			Vector2 diff = (tested.AsVector2() - m_basePos2).Normalized();

			if( m_heading.Dot( diff ) >= m_angleDot )//if is in cone, then ok
				return true;

			//even if not in cone but close enough, then also ok
			return ( tested - m_basePos3 ).SquareMag3() < m_backRangeSqrt;			
		}

		RED_INLINE Bool Accept( const CGameplayEntity* entity ) const	{ return Accept( entity->GetWorldPositionRef() ); }
		RED_INLINE Bool operator()( const CActorsManagerMemberData& data ) const { return Accept( data.Get() ); }
		RED_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr ) const { return Accept( ptr.Get() ); }

		Vector2				m_basePos2;
		Vector3				m_basePos3;
		Vector2				m_heading;
		Float				m_angleDot;	
		Float				m_backRangeSqrt;
	};

private:
	Bool								m_enebled; 
	TDynArray< SR6EnemyInfo >			m_enemiesInfo;
	SR6VisionParams						m_visionParams;
	Int32								m_visionSourceBoneIndex;
	THandle< CAnimatedComponent >		m_rootAnimatedComponent;
	CPhysicsEngine::CollisionMask		m_LOScollisionMask;
	Float								m_timeToBecomeAbsenceAware;
	Float								m_predictedPositionDistanceMultipler;

private:
	RED_INLINE Bool LOSTest( const Vector& startPosition, const Vector& endPosition )
	{
		SPhysicsContactInfo contactInfo;
		CPhysicsWorld* physicsWorld = nullptr;
		
		if( !GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) )
			return true;

		return  physicsWorld->RayCastWithSingleResult( startPosition, endPosition, m_LOScollisionMask, 0, contactInfo ) != TRV_Hit;
	}

	RED_INLINE Vector GetVisionSourcePosition()
	{ 
		if( m_rootAnimatedComponent.Get() && m_visionSourceBoneIndex!= -1 ) 
		{
			return m_rootAnimatedComponent.Get()->GetBoneMatrixWorldSpace( m_visionSourceBoneIndex ).GetTranslation();
		}
		else
		{
			return GetEntity()->GetWorldPosition();
		}
	};

	RED_INLINE SR6EnemyInfo* FindEnemyInfo( CActor* actor )
	{
		for( Uint32 i=0; i<m_enemiesInfo.Size(); ++i )
		{
			if( m_enemiesInfo[i].m_enemy.Get() == actor )
				return &m_enemiesInfo[i];
		}
		return NULL;
	}

	Bool IsVisible( const Vector& testedPosition );

	Vector GetVisionSourceDirection();

	void StartTicks();
	void StopTicks();
	void RemoveDeathEnemies();
	void ResetThisFrameVisibility();
	void CollectVisibleEnemies( TDynArray< CActor* >& output );
	void CacheBoneIndex();
	void GetEnemiesInCone( TDynArray< CActor* >& output, Vector& initPosition, Vector& initDirection );
	void PerformLOS( TDynArray< CActor* >& output, Vector& initPosition, Vector& initDirection );
	void UpdateKnowledge( TDynArray< CActor* >& visibleEnemies, Float timeDelta );
	void BrodecastEnemyNoticedEvent();

	Int32 FindTheNearestMatch( Bool (*matcher)( SR6EnemyInfo* ) );

public:
	RED_INLINE TDynArray< SR6EnemyInfo >& GetEnemiesInfo(){ return m_enemiesInfo; }

	void Enable();
	void Disable();
	void OnAttached( CWorld* world )	override;
	void OnDetached( CWorld* world )	override;
	void OnTick( Float timeDelta )		override;

	Bool GetNearestKnownPosition( Vector& outPosition );
	SR6EnemyInfo* GetNearestEnemy( Vector& outPosition );
	Bool IsEnemyiesPresent();

private:
	void funcIsEnemyiesPresent				( CScriptStackFrame& stack, void* result );
	void funcCanSeeEnemy					( CScriptStackFrame& stack, void* result );
	void funcOnlyKnowAboutEnemy				( CScriptStackFrame& stack, void* result );
	void funcFindNearestVisibleEnemy		( CScriptStackFrame& stack, void* result );
	void funcNotAwareOfEnemies				( CScriptStackFrame& stack, void* result );
	void funcGetNearestKnownPosition		( CScriptStackFrame& stack, void* result );

	void funcGetNearestVisibleEnemyInfo		( CScriptStackFrame& stack, void* result );
	void funcGetNearestKnownEnemyInfo		( CScriptStackFrame& stack, void* result );
	void funcGetInfoAboutEnemy				( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CEnemyAwareComponent )
	PARENT_CLASS( CR6Component );
	PROPERTY_EDIT( m_visionParams				, TXT("") );
	PROPERTY_EDIT( m_timeToBecomeAbsenceAware	, TXT("") );
	PROPERTY_EDIT_NAME( m_predictedPositionDistanceMultipler	, TXT("i_predictedPositionDistanceMultipler"), TXT("") );	

	NATIVE_FUNCTION( "I_IsEnemyiesPresent"				, funcIsEnemyiesPresent				);
	NATIVE_FUNCTION( "I_CanSeeEnemy"					, funcCanSeeEnemy					);
	NATIVE_FUNCTION( "I_OnlyKnowAboutEnemy"				, funcOnlyKnowAboutEnemy			);
	NATIVE_FUNCTION( "I_FindNearestVisibleEnemy"		, funcFindNearestVisibleEnemy		);
	NATIVE_FUNCTION( "I_NotAwareOfEnemies"				, funcNotAwareOfEnemies				);
	NATIVE_FUNCTION( "I_GetNearestKnownPosition"		, funcGetNearestKnownPosition		);

	NATIVE_FUNCTION( "I_GetNearestVisibleEnemyInfo"		, funcGetNearestVisibleEnemyInfo	);
	NATIVE_FUNCTION( "I_GetNearestKnownEnemyInfo"		, funcGetNearestKnownEnemyInfo		);
	NATIVE_FUNCTION( "I_GetInfoAboutEnemy"				, funcGetInfoAboutEnemy				);
END_CLASS_RTTI();
