/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "actionAreaVertex.h"
#include "../engine/triggerAreaComponent.h"

class CMoveLocomotion;
enum EPathEngineAgentType;

enum EAllowedActorGroups
{
	AAG_Player			= FLAG( 0 ),		//!< Player
	AAG_PlayerInCombat	= FLAG( 1 ),		//!< Player in combat (AAG_Player must be enabled)
	AAG_TallNPCs		= FLAG( 2 ),		//!< tall NPCs ( humans, elves )
	AAG_ShortNPCs		= FLAG( 3 ),		//!< short NPCs( children, dwarves )
	AAG_Monsters		= FLAG( 4 ),		//!< Monsters
	AAG_Ghost			= FLAG( 5 ),		//!< Ghosts ( formations etc. )
};

BEGIN_BITFIELD_RTTI( EAllowedActorGroups, 1 );
	BITFIELD_OPTION( AAG_Player );
	BITFIELD_OPTION( AAG_PlayerInCombat );
	BITFIELD_OPTION( AAG_TallNPCs );
	BITFIELD_OPTION( AAG_ShortNPCs );
	BITFIELD_OPTION( AAG_Monsters );
	BITFIELD_OPTION( AAG_Ghost );
END_BITFIELD_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CActionAreaBlendActor;

///////////////////////////////////////////////////////////////////////////////

struct SAnimShift
{
	DECLARE_RTTI_STRUCT( SAnimShift );

	Matrix				m_originalTransform;
	Matrix				m_transform;
	Float				m_time;

	SAnimShift( const Matrix& transform = Matrix::IDENTITY, Float time = 0.0f ) 
		: m_originalTransform( transform )
		, m_transform( transform )
		, m_time( time )
	{}
};
BEGIN_CLASS_RTTI( SAnimShift );
	PROPERTY( m_originalTransform );
	PROPERTY( m_transform );
	PROPERTY( m_time );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CAnimDef : public CObject
{
	DECLARE_ENGINE_CLASS( CAnimDef, CObject, 0 );

private:
	CName						m_animName;
	CAnimDef*					m_parent;
	TDynArray< SAnimShift >		m_shifts;
	Matrix						m_totalTransform;
	Float						m_duration;

public:
	CAnimDef() 
		: m_animName( CName::NONE )
		, m_parent( NULL )
		, m_totalTransform( Matrix::IDENTITY )
		, m_duration( 0.0f )
	{}

	CAnimDef* Initialize( const CName& animName, CAnimDef* parent );

	void SpawnGhosts( CLayer* layer, const Matrix& originTransform, TDynArray< CActionAreaBlendActor* >& outGhosts );

	Matrix GetParentTotalTransform() const;

	Matrix GetTotalTransform() const;

	void SetPosition( Uint32 shiftIdx, const Vector& pos );

	Matrix GetShiftTransform( Uint32 shiftIdx ) const;

	RED_INLINE CName GetAnimationName() const { return m_animName; }

	void SetupTrajectoryBlendPoints( TSortedArray< CSlotAnimationShiftingInterval >& outBlendPoints ) const;

	void SetShifts( const TDynArray< Vector >& shifts );

	RED_INLINE Uint32 GetShiftsCount() const { return m_shifts.Size(); }

	Vector GetTotalShift() const;

private:
	void CalculateTotalShiftForAnimation( CSkeletalAnimationSetEntry & animation );
	CActionAreaBlendActor* SpawnGhost( CLayer* layer, const Matrix& transform, CEntityTemplate* witcherTemplate, const CName& animationName, Float animationTime ) const;
	CSkeletalAnimationSetEntry* GetAnimation() const;
};
BEGIN_CLASS_RTTI( CAnimDef );
	PARENT_CLASS( CObject );
	PROPERTY( m_animName );
	PROPERTY( m_parent );
	PROPERTY( m_shifts );
	PROPERTY( m_totalTransform );
	PROPERTY( m_duration );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CActionAreaBlendActor : public CVertexEditorEntity
{
	DECLARE_ENGINE_CLASS( CActionAreaBlendActor, CVertexEditorEntity, 0 );

private:
	CAnimDef*		m_animationDefinition;
	Int32				m_shiftIdx;

public:
	CActionAreaBlendActor();

	void Initialize( CAnimDef& animationDefinition, Int32 shiftIdx );

	Bool SetBlendPosition( const Matrix& originTransform, const Vector& pos );

	void UpdatePosition( const Matrix& originTransform );

	RED_INLINE const CAnimDef* GetAnimationDefinition() const { return m_animationDefinition; }
};
BEGIN_CLASS_RTTI( CActionAreaBlendActor );
	PARENT_CLASS( CVertexEditorEntity );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

/// Action area component
class CActionAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CActionAreaComponent, CTriggerAreaComponent );

	friend class ExplorationAreaComponentImpl;

protected:
	enum EAnimType
	{
		AT_Pre,
		AT_Loop,
		AT_Post
	};

	struct EditorOnlyData
	{
		// Cached visual info about off-mesh links validity
		struct LinkInfo
		{
			Vector m_start;
			Bool   m_startValid;
			Vector m_end;
			Bool   m_endValid;
			Bool   m_isBurned;
		};
		TDynArray< LinkInfo >						m_offMeshLinks;

		// Debug parabola
		TDynArray< Vector >							m_localParabolaPoints;
		TDynArray< Vector >							m_worldParabolaPoints;
		TDynArray< Vector >							m_worldParabolaArrowPoints;

		TDynArray< CActionAreaBlendActor* >			m_blendActors;				//!< Temporary actors used during animation tunning
		Bool										m_lockBlendActorUpdates;	//!< Flag that locks blend actor updates on entity move

		EditorOnlyData() : m_lockBlendActorUpdates( false ) {}

		void RenderLinkValidity( CRenderFrame* frame, const Vector & worldPosition, const CHitProxyID & hitProxyId );

		void CreateParabola( const Matrix & localToWorld, const TDynArray< Vector > & keyPoints );
	};

protected:
	// ActionArea configuration
	Uint8										m_allowedGroups;
	Float										m_walkToSideDistance;	// distance to action area from which we may also trigger action and player will walk/run to it
	Float										m_walkToBackDistance;	// distance to action area from which we may also trigger action and player will walk/run to it
	Float										m_walkToFrontDistance;	// distance to action area from which we may also trigger action and player will walk/run to it

	// Path-Engine runtime data
	Uint32										m_nextLinkIdx;
	Bool										m_isConnectionEnabled;

	// Editor-only data
	EditorOnlyData*								m_editorData;

private:
	Bool										m_totalTransformationIsKnown;
	Matrix										m_fullTransformation;  // it's a transformation in the component's local space
	TDynArray< CAnimDef* >						m_animations;

	// ---- TODO: remove when VER_EXPLORATION_AREAS_USE_TRANSFORM_MATRIX is outdated ----
	Vector										m_totalTransformation;
	TDynArray<Vector>							m_animShiftStart;
	TDynArray<Vector>							m_animShiftLoop;
	TDynArray<Vector>							m_animShiftStop;
	// ----------------------------------------------------------------------------------

public:
	CActionAreaComponent();

	virtual void OnPropertyPostChange( IProperty* prop );
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	
	void EnableTriggerArea( Bool enable );

	void EnableOffMeshConnection( Bool enable );

	virtual void SetEnabled( Bool enabled );

	void SetAllowedGroupFlag( EAllowedActorGroups group, Bool flag );

	Bool IsAllowedForActorGroup( EAllowedActorGroups group ) const { return ( m_allowedGroups & group ) != 0; }

	// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	// Something have exited zone
	virtual void ExitedArea( CComponent* component );

	virtual Bool CheckShouldSave() const { return true; }

	virtual Vector GetClosestActionPosition( const Vector & worldLocation, Bool toStartingPos ) const = 0;

	Vector GetActionEndPosition( const Vector& startPosition ) const;

	void RefreshInPathEngine();

    void OnTickPostPhysics( Float timeDelta );

protected:
	void ReFillLocalPoints( Bool fillWorldPoints );
	
	void InvalidateParabola( Bool fillParabolaNow );

	/************************************************************************/
	/* Derived from IPathEngineOffMeshConnection                            */
	/************************************************************************/
public:
	virtual void CalculateOffMeshLinks( ) const {}

	virtual Bool IsAvailableForAgent( EPathEngineAgentType agentType ) const;

	virtual Uint32 GetConnectionPenalty() const { return 0; }

	virtual Vector GetClosestPosition( const Vector& userPos ) const;

	virtual EulerAngles GetOrientation() const;

	virtual void NotifyConnectionUsage();

	RED_INLINE virtual Bool IsConnectionEnabled() const { return m_isConnectionEnabled; }

	/************************************************************************/
	/* Some player only stuff                                               */
	/************************************************************************/
	virtual void TriggerExploration( CPlayer * player );

	// Is player set to use the area?
	Bool CanBeAutoTriggered( CPlayer * player ) const;

	/************************************************************************/
	/* Action definition methods                                            */
	/************************************************************************/
public:
	RED_INLINE virtual String GetActionName() const = 0;

	RED_INLINE virtual Bool IsInteractionTriggered() const { return true; }

protected:
	RED_INLINE virtual Uint32 GetLoopCount() const { return 1; }

	RED_INLINE virtual CName GetAnimationPre( EAllowedActorGroups group )  const { return CName::NONE; }
	RED_INLINE virtual CName GetAnimationLoop( EAllowedActorGroups group ) const = 0;
	RED_INLINE virtual CName GetAnimationPost( EAllowedActorGroups group ) const { return CName::NONE; }

	RED_INLINE virtual Float GetBlendInTime() const { return 0.2f; }
	RED_INLINE virtual Float GetBlendOutTime() const { return 0.2f; }
	RED_INLINE virtual Float GetCameraAngle() const { return 0.f; }
	RED_INLINE virtual Float GetCameraBlendIn() const { return 0.2f; }
	RED_INLINE virtual Float GetCameraBlendOut() const { return 0.2f; }
	RED_INLINE virtual Float GetCameraResetTime() const { return 0.5f; }

	RED_INLINE virtual Float GetAngleTolerance() const { return 180.f; }

	RED_INLINE virtual Bool IsActiveWhenIdle() const { return true; }
	RED_INLINE virtual Bool IsActiveWhenWalk() const { return true; }
	RED_INLINE virtual Bool IsActiveWhenRun()  const { return true; }
	
	/************************************************************************/
	/* Editor tunning ghosts                                                */
	/************************************************************************/
#ifndef NO_EDITOR
public:
	virtual Bool OnEditorBeginSpriteEdit( TDynArray<CEntity*> &spritesToEdit );
	virtual void OnEditorEndSpriteEdit();

	virtual Bool OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height );

	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
#endif

protected:
#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

	void UpdateTotalTransformation();

	void GetEndpointTransformation( Matrix& outTransformation ) const;

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

private:
	void CacheOffMeshLinks( CWorld* world );
	void ToggleNextLink();
	void GetGroundTransform( const Matrix& originalTransformation, Matrix& modifiedTransform ) const;
	void CalculateTotalShift();
	void SetTotalTransformation( const Matrix & transformation )
	{
		m_fullTransformation         = transformation;
		m_totalTransformationIsKnown = true;
	}

};
BEGIN_ABSTRACT_CLASS_RTTI( CActionAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_walkToSideDistance, TXT("Distance to action area from which we may also trigger action and player will walk/run to it") );
	PROPERTY_EDIT( m_walkToBackDistance, TXT("Distance to action area from which we may also trigger action and player will walk/run to it") );
	PROPERTY_EDIT( m_walkToFrontDistance, TXT("Distance to action area from which we may also trigger action and player will walk/run to it") );
	PROPERTY_BITFIELD_EDIT( m_allowedGroups, EAllowedActorGroups, TXT("Groups of actors that may use this exploration area") );
	PROPERTY( m_animShiftStart );
	PROPERTY( m_animShiftLoop );
	PROPERTY( m_animShiftStop );
	PROPERTY( m_totalTransformation ); // TODO: remove when VER_EXPLORATION_AREAS_USE_TRANSFORM_MATRIX is outdated
	PROPERTY( m_fullTransformation );
	PROPERTY( m_animations );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
