#pragma once

#include "coverPoint.h"
#include "enemyAwareComponent.h"

class CNodesBinaryStorage;
struct SRestrictionArea;

class CCoversManager : public CObject
{
	DECLARE_ENGINE_CLASS( CCoversManager, CObject, 0 );
private:
	CNodesBinaryStorage*							m_coverPointsStorage;
	TDynArray< THandle< CCoverPointComponent > >	m_covers;
public:
	CCoversManager();

	RED_INLINE CNodesBinaryStorage* GetCoverPointsStorage(){ return m_coverPointsStorage; }

	void Initialize();

	void AddCoverPoint( CCoverPointComponent* coverPoint );
	void RemoveCoverPoint( CCoverPointComponent* coverPoint );
	CCoverPointComponent* GetNearestCover( Vector& coverAgainst, Vector& nearPosition, Float range );
	CCoverPointComponent* GetBestCoverForNPC( CNewNPC* npc, Float range, SCoverRatingsWeights& wieghts, CCoverPointComponent* currentCover );
	CCoverPointComponent* GetBestCoverForNPCInVicinity( CNewNPC* npc, const Vector& position, Float range, SCoverRatingsWeights& wieghts, CCoverPointComponent* currentCover, SRestrictionArea* restrictionArea = NULL );	
	Float CountRaitingForCover( const TDynArray< SR6EnemyInfo >&  enemies, CCoverPointComponent* cover, const Vector& nearPostion, SCoverRatingsWeights& wieghts, Float range );
	CCoverPointComponent* FindFlankingCover(  Vector& flankedPosition, Vector& nearPosition, Float range );
	CCoverPointComponent* GetSafestCoverInVisinity( CNewNPC* npc, const Vector& position, Float range, CCoverPointComponent* currentCover );

	void funcGetNearestCover							(  CScriptStackFrame& stack, void* result );
	void funcGetBestCoverForNPCInVicinity				(  CScriptStackFrame& stack, void* result );	
	void funcGetBestCoverForNPC							(  CScriptStackFrame& stack, void* result );
	void funcChooseEnemyToShot							(  CScriptStackFrame& stack, void* result );
	void funcFindFlankingCover							(  CScriptStackFrame& stack, void* result );
	void funcCalculateDirectionBlendToCover				(  CScriptStackFrame& stack, void* result );	
	void funcHasClearLineOfFire							(  CScriptStackFrame& stack, void* result );		
	void funcGetSafestCoverInVicinity					(  CScriptStackFrame& stack, void* result );		
	void funcGetBestCoverForNPCInVicinityRestriction	(  CScriptStackFrame& stack, void* result );	

};

BEGIN_CLASS_RTTI( CCoversManager );
	PARENT_CLASS( CObject );
	PROPERTY( m_coverPointsStorage );
	NATIVE_FUNCTION( "I_GetNearestCover"						, funcGetNearestCover							);
	NATIVE_FUNCTION( "I_GetBestCoverForNPC"						, funcGetBestCoverForNPC						);
	NATIVE_FUNCTION( "I_GetBestCoverForNPCInVicinity"			, funcGetBestCoverForNPCInVicinity				);
	NATIVE_FUNCTION( "I_ChooseEnemyToShot"						, funcChooseEnemyToShot							);
	NATIVE_FUNCTION( "I_FindFlankingCover"						, funcFindFlankingCover							);
	NATIVE_FUNCTION( "I_CalculateDirectionBlendToCover"			, funcCalculateDirectionBlendToCover			);
	NATIVE_FUNCTION( "I_HasClearLineOfFire"						, funcHasClearLineOfFire						);	
	NATIVE_FUNCTION( "I_GetSafestCoverInVicinity"				, funcGetSafestCoverInVicinity					);	
	NATIVE_FUNCTION( "I_GetBestCoverForNPCInVicinityRestriction", funcGetBestCoverForNPCInVicinityRestriction	);
END_CLASS_RTTI();