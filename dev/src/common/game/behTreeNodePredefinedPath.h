#pragma once

#include "behTreeNodeCustomSteering.h"
#include "behTreeNodeAtomicMove.h"
#include "behTreeSteeringGraphBase.h"

class CBehTreeNodePredefinedPathInstance;


//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePredefinedPathDefinition : public CBehTreeNodeCustomSteeringDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePredefinedPathDefinition, CBehTreeNodeCustomSteeringDefinition, CBehTreeNodePredefinedPathInstance, FollowPredefinedPath );
protected:
	CBehTreeValCName				m_pathName;
	CBehTreeValFloat				m_pathMargin;
	CBehTreeValFloat				m_tolerance;
	CBehTreeValBool					m_upThePath;
	CBehTreeValFloat				m_arrivalDistance;  

	CBehTreeValBool					m_useExplorations;
public:
	CBehTreeNodePredefinedPathDefinition()
		: m_pathName()
		, m_pathMargin( 1.f )
		, m_upThePath( true )
		, m_arrivalDistance( 0.5f )
		, m_tolerance( 0.5f )
		, m_useExplorations( false )
	{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePredefinedPathDefinition );
	PARENT_CLASS( CBehTreeNodeCustomSteeringDefinition);
	PROPERTY_EDIT( m_pathName, TXT("Tag of path component") );
	PROPERTY_EDIT( m_upThePath, TXT("Unmark this to make character move from path end to beginning") )
	PROPERTY_EDIT( m_pathMargin, TXT("Path margin that movement should keep in") )
	PROPERTY_EDIT( m_tolerance, TXT("How far away the agent could be from the path") )
	PROPERTY_EDIT( m_arrivalDistance, TXT("if the agent it at that distance from the last node of the path then the node will complete ") )
	PROPERTY_EDIT( m_useExplorations, TXT("if explorations should be used diuring following the path") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePredefinedPathInstance : public CBehTreeNodeCustomSteeringInstance
{

	const static Float	MAX_EXPLORATION_PATH_DISTANCE_SQRT;
	const static Float	MAX_EXPLORATION_NPC_DISTANCE_SQRT;

protected:
	// configuration
	CName							m_pathName;
	Float							m_pathMargin;
	Float							m_tolerance;
	Float							m_defaultTolerance;
	Bool							m_upThePath;

	// run-time data
	Bool							m_isSuccess;
	Bool							m_isCompleted;
	THandle< CPathComponent >		m_pathComponent;
	Int32							m_pathEdgeIdx;
	Float							m_pathEdgeAlpha;
	Vector2							m_lastPosition;
	Vector							m_destinationPosition;
	Float							m_arrivalDistance;
	Float							m_distanceFromStart;
	InteractionPriorityType			m_previousInteractionPriority;

	//exploration
	Bool									m_useExplorations;
	Bool									m_wasSnappedOnActivate;
	Bool									m_isInExploration;
	Bool									m_restoreExplorationState;
	Bool									m_movementSwitchedBackToWalking;
	ActorActionExploration*					m_explorationAction;
	THandle< CScriptedExplorationTraverser >m_traverser;	
	THandle< CMovingPhysicalAgentComponent >m_mac;
	Float									m_prevLocalTime;
	const IExploration*						m_lastUsedExploration;
	Float									m_lastExplorationUsageTime;
	Float									m_timeToSwitchOnCollisionAfterExploration;	
	Vector3									m_beforeExplorationPosition;
	EulerAngles								m_beforeExplorationRotation;


	// doors
	Bool									m_isUsingDoors;
	Bool									m_wasForced;
	EMoveType								m_inDoorMoveType;
	THandle< CDoorComponent >				m_doorsInUse;
		
public:
	typedef CBehTreeNodePredefinedPathDefinition Definition;
	typedef CBehTreeNodeCustomSteeringInstance Super;

	CBehTreeNodePredefinedPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool Activate() override;
	void Deactivate() override;

	void Update() override;	
	// IMovementTargeter interface
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;
	Bool OnEvent( CBehTreeEvent& e ) override;

	void OnGenerateDebugFragments( CRenderFrame* frame ) override;

	// saving state support
	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;

protected:	
	void DisableExplorationWalking();
	void EnableExplorationWalking();
	void UpdateExploration();
	void UpdateDoorsMovement();
	void SwitchToExplorationIfNeeded();
	void RenewPathFollow();
	Bool IsThisClosestUser( CDoorComponent* door );
	void RestoreExplorationState();

	EMoveType GetMoveType()const override;
};


//////////////////////////////////////////////////////////////////////////
// Special movement goal

class CBehTreeNodeAtomicMoveToPredefinedPathInstance;

class CBehTreeNodeAtomicMoveToPredefinedPathDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToPredefinedPathDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicMoveToPredefinedPathInstance, MoveToPredefinedPath );
protected:
	CBehTreeValCName							m_pathName;
	CBehTreeValBool								m_upThePath;
	CBehTreeValBool								m_startFromBeginning;
public:
	CBehTreeNodeAtomicMoveToPredefinedPathDefinition()
		: m_upThePath( true )
		, m_startFromBeginning( false )										{}
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToPredefinedPathDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition);
	PROPERTY_EDIT( m_pathName, TXT("Tag of path component") );
	PROPERTY_EDIT( m_upThePath, TXT("Unmark this to make character move from path end to beginning") )
	PROPERTY_EDIT( m_startFromBeginning, TXT("Mark if movemenet should begin always from beginning of path") );
END_CLASS_RTTI();

class CBehTreeNodeAtomicMoveToPredefinedPathInstance : public CBehTreeNodeAtomicMoveToInstance
{
	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	CName										m_pathName;
	THandle< CPathComponent >					m_pathComponent;
	Bool										m_upThePath;
	Bool										m_startFromBeginning;
	Bool										m_arrivedAtPath;

	Bool StartFromBeginning() const														{ return m_startFromBeginning && !m_arrivedAtPath; }
	Bool ComputeTargetAndHeading() override;
public:
	typedef CBehTreeNodeAtomicMoveToPredefinedPathDefinition Definition;
	
	CBehTreeNodeAtomicMoveToPredefinedPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void Update()override;
	void Complete( eTaskOutcome outcome ) override;

	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	void OnDestruction() override;
};

///////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodePredefinedPathWithCompanionInstance;

class CBehTreeNodePredefinedPathWithCompanionDefinition : public CBehTreeNodePredefinedPathDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePredefinedPathWithCompanionDefinition, CBehTreeNodePredefinedPathDefinition, CBehTreeNodePredefinedPathWithCompanionInstance, FollowPredefinedPathWithCompanion );
protected:
	CBehTreeValCName		m_companionTag;
	CBehTreeValFloat		m_maxDistance;
	CBehTreeValFloat		m_minDistance;
	CBehTreeValBool			m_progressWhenCompanionIsAhead;
	CBehTreeValBool			m_progressOnlyWhenCompanionIsAhead;
	CBehTreeValBool			m_matchCompanionSpeed;
	CBehTreeValBool			m_keepMovingWhenMaxDistanceReached;
	CBehTreeValEMoveType	m_moveTypeAfterMaxDistanceReached;
	CBehTreeValFloat		m_matchCompanionSpeedCatchUpDistance;
	CBehTreeValFloat		m_companionOffset;

public:
	CBehTreeNodePredefinedPathWithCompanionDefinition()
		: m_maxDistance( 5.f )
		, m_minDistance( 1.f )
		, m_companionOffset( 0.f )
		, m_progressWhenCompanionIsAhead( false )	
		, m_matchCompanionSpeed( true )
		, m_keepMovingWhenMaxDistanceReached( false )
		, m_moveTypeAfterMaxDistanceReached( MT_Run )
	{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePredefinedPathWithCompanionDefinition );
	PROPERTY_EDIT( m_companionTag, TXT("Tag for the following companion") )
	PROPERTY_EDIT( m_maxDistance, TXT("Maximum seperation distance between NPC and companion, before stopping movement") )
	PROPERTY_EDIT( m_minDistance, TXT("Minimum seperation distance between NPC and companion, before starting movement") )
	PROPERTY_EDIT( m_progressWhenCompanionIsAhead, TXT("If set to true NPC will move even if companion is far ahead. But only if companion is 'close' to the path.") )
	PROPERTY_EDIT( m_progressOnlyWhenCompanionIsAhead, TXT("If set to true NPC will only move if companion is ahead of us.") )
	PROPERTY_EDIT( m_matchCompanionSpeed, TXT("If true the NPC will match the companion speed will a litte lag") )
	PROPERTY_EDIT( m_companionOffset, TXT("Offset in meters which allows to tweak distance between the npc and the companion") )
	PROPERTY_EDIT( m_keepMovingWhenMaxDistanceReached, TXT("If set to true, NPC won't stop after reaching maxDistance but will use the specified movement type.") )
	PROPERTY_EDIT( m_moveTypeAfterMaxDistanceReached, TXT("If keepMovingWhenMaxDistanceReached is true, NPC will use this movement type after reaching max distance.") )
PARENT_CLASS( CBehTreeNodePredefinedPathDefinition);

END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePredefinedPathWithCompanionInstance : public CBehTreeNodePredefinedPathInstance
{
	typedef CBehTreeNodePredefinedPathInstance Super;
protected:
	CName					m_companionTag;
	Float					m_stopMovementDistSq;
	Float					m_startMovementDistSq;
	Float					m_companionTestDelay;
	Float					m_companionOffset;
	Bool					m_progressWhenCompanionIsAhead;
	Bool					m_progressOnlyWhenCompanionIsAhead;
	Bool					m_matchCompanionSpeed;
	Bool					m_keepMovingWhenMaxDistanceReached;
	EMoveType				m_moveTypeAfterMaxDistanceReached;

	Vector					m_companionPositionOnPath;	
	THandle< CEntity >		m_companion;
	EMoveType				m_previousMoveType;
	Bool					m_isRiddenOff;
	Bool					m_paused;
	Bool					m_companionIsInFront;

public:
	typedef CBehTreeNodePredefinedPathWithCompanionDefinition Definition;

	CBehTreeNodePredefinedPathWithCompanionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool					Activate() override;
	void					Update() override;

	// IMovementTargeter interface
	void					UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
protected:
	Bool		IsCompanionAway();
	EMoveType	GetMoveType()const override;
	Bool		ShouldMaintainTargetSpeed( )const;
	void		CalcCompanionIsInFront();
	void		CalcIsRiddenOff();

	virtual CEntity *const GetCompanion()const;
};