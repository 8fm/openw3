#pragma once

#include "enemyAwareComponent.h"
#include "../../common/game/wayPointComponent.h"

enum ECoverPointBehavior
{
	CPB_STAND,
	CPB_CROUCH,
};
BEGIN_ENUM_RTTI( ECoverPointBehavior );
ENUM_OPTION( CPB_STAND );
ENUM_OPTION( CPB_CROUCH );
END_ENUM_RTTI();

enum EPositionInCoverPoint
{
	EPICP_LeftShootingPoint,
	EPICP_RightShootingPoint,
	EPICP_CoverPoint
};
BEGIN_ENUM_RTTI( EPositionInCoverPoint );
ENUM_OPTION( EPICP_LeftShootingPoint );
ENUM_OPTION( EPICP_RightShootingPoint );
ENUM_OPTION( EPICP_CoverPoint );
END_ENUM_RTTI();


enum ECoverAttackDirection
{
	CAD_LEFT,
	CAD_RIGHT,
	CAD_UP,
	CAD_BACK,
	CAD_NONE,
};
BEGIN_ENUM_RTTI( ECoverAttackDirection );
ENUM_OPTION( CAD_LEFT );
ENUM_OPTION( CAD_RIGHT );
ENUM_OPTION( CAD_UP );
ENUM_OPTION( CAD_BACK );
ENUM_OPTION( CAD_NONE );
END_ENUM_RTTI();

struct SCoverRatingsWeights
{
	DECLARE_RTTI_STRUCT( SCoverRatingsWeights );
	
	SCoverRatingsWeights():m_cover(1), m_shoot(1), m_distanceFromNPC(1), m_minDistanceFromEnemies( 1 ), m_minPercentageOfAvoidedEnemy( 0.5f ), m_forceShootingOpportunity( false ) {}

	Float m_cover;
	Float m_shoot;	
	Float m_distanceFromNPC;	
	Float m_optimalDistance;
	Float m_optimalDistanceRaiting;	
	Float m_ceveredFromMainRaiting;
	Float m_shootToMainRaiting;
	Float m_minDistanceFromEnemies;
	Float m_minPercentageOfAvoidedEnemy;
	Bool  m_forceShootingOpportunity;
	//Float m_flankingMainRaiting;

	THandle< CEntity > m_mainTarget;

	void Reset();
};

BEGIN_CLASS_RTTI( SCoverRatingsWeights );
	PROPERTY_EDIT( m_cover,							TXT("Cover importance")					);
	PROPERTY_EDIT( m_shoot,							TXT("Shoot posibilities importance")	);
	PROPERTY_EDIT( m_distanceFromNPC,				TXT("Distance from this NPC importance"));
	PROPERTY_EDIT( m_optimalDistanceRaiting,		TXT("Optimal distance raiting")			);
	PROPERTY_EDIT( m_ceveredFromMainRaiting,		TXT("Covered from main target raiting") );	
	PROPERTY_EDIT( m_shootToMainRaiting,			TXT("Shooting to main target")			);
	PROPERTY_EDIT( m_optimalDistance,				TXT("Optimal distance")					);
	PROPERTY_EDIT( m_minDistanceFromEnemies,		TXT("Min distance from enemies")		);
	PROPERTY_EDIT( m_minPercentageOfAvoidedEnemy,	TXT("Optimal distance")					);
	PROPERTY	 ( m_mainTarget						);
	PROPERTY	 ( m_forceShootingOpportunity		);
END_CLASS_RTTI();


struct SCoverAttackInfo
{
	DECLARE_RTTI_STRUCT( SCoverAttackInfo );

	SCoverAttackInfo():m_attackTarget(NULL), m_direction(CAD_NONE),m_attackBlend(0) {}

	THandle<CActor>			m_attackTarget;
	ECoverAttackDirection	m_direction;
	Float					m_attackBlend;
};
BEGIN_CLASS_RTTI( SCoverAttackInfo );
	PROPERTY( m_attackTarget );
	PROPERTY( m_direction );
	PROPERTY( m_attackBlend );
END_CLASS_RTTI();

class CCoverPointComponent : public CWayPointComponent
{
	static const Int32	DIRECTIONS = 12;
	static const Int32	MAX_COVER_DISTANCE_TEST = 30;
	static const Float	STAND_COVER_HEIGHT;
	static const Float	CROUCH_COVER_HEIGHT;
	static const Float	COVER_VALIDATION_RADIUS;

	static IRenderResource*		m_markerValid;
	static IRenderResource*		m_markerInvalid;
	static IRenderResource*		m_markerDestroyied;
	DECLARE_ENGINE_CLASS( CCoverPointComponent, CWayPointComponent, 0 );

public:
	CCoverPointComponent()
	{
		InitializeCoverMarker();
	}

	RED_INLINE Vector GetCoverPosition( ){ return m_coverPosition; }

private:
	ECoverPointBehavior	m_coverBehavior;
	Bool				m_attackLeft;
	Bool				m_attackRight;
	Bool				m_attackUp;
	Float				m_firePositionDistance;

	Bool				m_isActive;//If is on navMesh	

	Vector				m_leftPosition;
	Vector				m_rightPosition;
	Vector				m_coverPosition;

	Float				m_coverDirections[DIRECTIONS];
	Float				m_leftFireDirections[DIRECTIONS];
	Float				m_rightDirections[DIRECTIONS];
	Float				m_upFireDirections[DIRECTIONS];
	THandle<CActor>		m_lockedBy;
private:
	void InitializeCoverMarker();
	void CalculateCoverDirections( const Vector& position, Float* directionsData );	
	Bool CanShoot( const Vector& enemyPosition, Vector& shootPosition, Float* shootDirections );
	Bool CanShootBack( const Vector& enemyPosition );
	Int32 FindDirection( const Vector& enemy, Vector& basePos );

	RED_INLINE Int32 NextPart( Int32 part ) 
	{
		part+=1;
		if( part>=DIRECTIONS )
			part-=DIRECTIONS;
		return part;
	}

	RED_INLINE Int32 PrevPart( Int32 part ) 
	{
		part-=1;
		if( part<0 )
			part+=DIRECTIONS;
		return part;
	}

protected:
	virtual IRenderResource* GetMarkerValid(){ return m_isActive ? m_markerValid : m_markerDestroyied; }
	virtual IRenderResource* GetMarkerInvalid(){ return m_markerInvalid; }

public:
	//CCoverPointComponent();		

	RED_INLINE Bool IsFree(){ return m_lockedBy.Get()==NULL || !m_lockedBy.Get()->IsAlive(); }
	RED_INLINE CActor* GetLockedBy(){ return m_lockedBy.Get(); }
	RED_INLINE Bool IsActive(){ return m_isActive; }
	void CalculateCoverDirectionsAll( );	
	Bool IsCoverAgainst( const Vector& enemy );
	Bool IsCoverAgainst( const TDynArray< SR6EnemyInfo >&  enemies, Float minDistanceFromEnemies, Float minPercentageOfAvoidedEnemy );
	ECoverAttackDirection CanShootTo( const Vector& enemy );
	Float CountCoverRating( const TDynArray< SR6EnemyInfo >&  enemies, CEntity* mainTarget, Float* coverFromMainRatingRet );
	Float CountShootRating( const TDynArray< SR6EnemyInfo >&  enemies, CEntity* mainTarget, Float* shootToMainRatingRet );
	Float CountFlankRating( const Vector& enemyPosition );
	Float CountDistanceFromNPCRating( Float range,const Vector& npcPosition );
	Float CountOptimalDistanceRating( Float optimalDistance, CEntity* mainTarget ); 	

	Float CalculateDirectionBlend( const Vector& threatPosition, ECoverAttackDirection directionEnum );
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	ECoverAttackDirection CalculateNearestDirection( const Vector& threatPosition );	

	void funcIsCoverAgainst					(  CScriptStackFrame& stack, void* result );
	void funcGetAttackDirection				(  CScriptStackFrame& stack, void* result );
	void funcCalcAttackDirectionBlend		(  CScriptStackFrame& stack, void* result );	
	void funcDeactivateCover				(  CScriptStackFrame& stack, void* result );	
	void funcGetDistanceToNearestObstacle	(  CScriptStackFrame& stack, void* result );	
	void funcGetShootingPositions			(  CScriptStackFrame& stack, void* result );	
	void funcCanShootTo						(  CScriptStackFrame& stack, void* result );	

};

BEGIN_CLASS_RTTI( CCoverPointComponent );
	PARENT_CLASS( CWayPointComponent );			
	PROPERTY_NAME( m_lockedBy, TXT("i_lockedBy") );
	PROPERTY_NAME( m_isActive, TXT("i_isActive") );

	PROPERTY_EDIT_NAME( m_coverBehavior			, TXT("i_coverBehavior")		, TXT( "Behavior" )									);	
	PROPERTY_EDIT_NAME( m_attackLeft			, TXT("i_attackLeft")			, TXT("If can attack from left"	)					);
	PROPERTY_EDIT_NAME( m_attackRight			, TXT("i_attackRight")			, TXT("If can attack from right" )					);
	PROPERTY_EDIT_NAME( m_attackUp				, TXT("i_attackUp")				, TXT("If can attack from up")						);
	PROPERTY_EDIT_NAME( m_firePositionDistance	, TXT("i_firePositionDistance")	, TXT("Distance from cover point to fire position")	);	

	NATIVE_FUNCTION( "I_IsCoverAgainst"					, funcIsCoverAgainst				);
	NATIVE_FUNCTION( "I_GetAttackDirection"				, funcGetAttackDirection			);
	NATIVE_FUNCTION( "I_CalcAttackDirectionBlend"		, funcCalcAttackDirectionBlend		);
	NATIVE_FUNCTION( "I_DeactivateCover"				, funcDeactivateCover				);
	NATIVE_FUNCTION( "I_GetDistanceToNearestObstacle"	, funcGetDistanceToNearestObstacle	);
	NATIVE_FUNCTION( "I_GetShootingPositions"			, funcGetShootingPositions			);
	NATIVE_FUNCTION( "I_CanShootTo"						, funcCanShootTo			);

END_CLASS_RTTI();

