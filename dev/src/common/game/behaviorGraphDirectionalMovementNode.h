/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphDirectionalMovementNode;

///////////////////////////////////////////////////////////////////////////////

/**
 *	Directional movement node for 360' movement, using any number of animations and groups
 *
 *	For this example we will discuss 2 groups with 3 anims each:
 *
 *			Forward animations					Backward animations
 *				  .'2'.								  0   4							numbers are anim-in-group indices
 *				 '	|  '							 ' \ / '						here, between 1 and 3 can be any number of anims
 *				3---o---1							1---o---3						rules:
 *				 . / \ .							 .  |  .						0 and 4 are musthaves - they are at sideAngleRange
 *				  4   0								  '.2.'							1 and 3 are optional - they are at groupDir
 *																					2 is optional - it is at 0
 *			cover forward and					cover backward and					any extra anims are added between 1 and 3
 *			  sides movement					  sides movement
 *
 *	Groups rise clockwise, as anims within groups do
 *	Character may blend between both groups only at certain times (to allow nice synchronization between feet).
 *	Synchronization is done by events (defined by user)
 *	Forward and backward animations overlap to allow blend between them.
 *	Direction is damped internally to allow tweaking by modifying values only in this node.
 *	First group is always facing forward.
 *	Writes current movement direction to allow other states to use it. Internal float variable "MovementDirectionFromDirectionalMovement"
 *
 *	Inputs:
 *		requested movement direction WS
 *		requested facing direction WS
 */
class CBehaviorGraphDirectionalMovementNode : public CBehaviorGraphNode
											, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDirectionalMovementNode, CBehaviorGraphNode, "Movement", "Directional movement" );

protected:
	Int32 m_groupCount; // number of groups
	Int32 m_animsPerGroup; // number of animations per group
	Float m_groupDir; // direction of each group is increased by this value (autocalculated)
	Float m_firstGroupDirOffset; // offset of first group
	Float m_keepInCurrentGroupAngle; // extra buffer angle to keep in current group (to avoid constant switching)
	Float m_findGroupDirOffset; // offset for dir to prefer some initial choice for groups
	//
	Float m_extraOverlapAngle; // extra angle used for overlap range
	Float m_sideAngleRange; // range to side from center of the group, example: if 3 animations cover whole 180 angle, sideAngleRange is 90 (autocalculated)
	Bool m_singleAnimOnly; // use single animation only, do not blend between
	Bool m_doNotSwitchAnim; // stay at initial animation, do not switch
	Float m_movementDirBlendTime; // blend time for movement direction
	Float m_movementDirMaxSpeedChange; // max speed for movement direction change
	Float m_groupsBlendTime; // blend time between groups
	Float m_quickTurnBlendTime; // quick turn blend time
	Float m_fasterQuickTurnBlendTime; // faster quick turn blend time
	Float m_angleThresholdForQuickTurn; // when angle difference is greater than this value, quick turn is started
	Bool m_reverseSyncOnQuickTurnFwdBwd; // reverse when changing forward/backward
	Bool m_reverseSyncOnQuickTurnLeftRight; // reverse when changing left/right
	Float m_syncBlendingOffsetPTLOnQuickTurn; // syncing offset when doing quick turn
	Float m_startPTLRightFootInFront; // starting prop through loop from right foot in front
	Float m_startPTLLeftFootInFront; // starting prop through loop from left foot in front
	Bool m_alwaysStartAtZero; // always start at beginning of the animation
	//
	Int32 m_loopCount; // number of loops
	Float m_syncGroupOffsetPTL; // sync done when switching to opposite side when switching groups (refers to loop) TODO add for higher number of groups?
	//
	Float m_facingDirBlendTime; // blend time for facing direction
	Float m_facingDirMaxSpeedChange; // max speed for facing direction change
	//
	Bool m_useSimpleBlendForMovementDelta; // use simple, linear blend for movement delta, if it is off - delta will be calculated from blend of yaw and blend of speed
	//
	Bool m_useDefinedVariablesAsRequestedInput; // if no variables connected, use stored variables as requested input
	//
	CName m_requestedMovementDirectionVariableName; // requested movement direction variable name (used if variable not connected)
	CName m_requestedFacingDirectionVariableName; // requested facing direction variable name (used if variable not connected)
	//
	Bool m_useDefinedInternalVariablesAsInitialInput;// if no variables connected, use stored internal variables as initial input
	//
	CName m_movementDirectionInternalVariableName;
	CName m_groupDirInternalVariableName;
	//
	CName m_rightFootBone; // used for synchronization
	CName m_leftFootBone; // used for synchronization

private:
	IBehaviorSyncMethod* m_syncMethod;

private: // auto calculated values
	Float m_loopCountFloat;
	Float m_groupCountFloat;
	Float m_anglePerInsideAnim;

protected:
	TInstanceVar< Float > i_requestedQuickTurnTime; // current blend time
	TInstanceVar< Float > i_currentBlendTime; // current blend time
	TInstanceVar< Float > i_currentRotationYaw; // current rotation yaw of character (used to keep movement direction nicely)
	TInstanceVar< Float > i_requestedFacingDirection; // requested facing direction in model space (-180.0f -> 180.0f)
	TInstanceVar< Float > i_facingDirectionChange; // facing direction change to be put when sampling
	TInstanceVar< Float > i_requestedMovementDirection; // requested movement direction in model space (-180.0f -> 180.0f)
	TInstanceVar< Float > i_movementDirection; // current direction in model space (-180.0f -> 180.0f)
	TInstanceVar< Float > i_additionalMovementDirection; // additional movement direction that is result of clamping
	TInstanceVar< Float > i_prevGroupMovementDirection; // movement direction for previous group
	TInstanceVar< Bool > i_quickTurnIsAllowed; // can do quick turn now
	TInstanceVar< Bool > i_switchGroupIsAllowed; // can now switch to different group
	TInstanceVar< Bool > i_isBlending; // blending now
	TInstanceVar< Float	> i_blendLeft; // blend left (starts with 1, goes to 0)
	TInstanceVar< Bool > i_isDoingQuickTurn; // blend is quick turn
	TInstanceVar< Int32 > i_currGroupIdx; // currently active group
	TInstanceVar< Int32 > i_currGroupAnimIdx; // animation in current group (right, if animBlend is non zero, it blends with next one)
	TInstanceVar< Float > i_currGroupAnimBlend; // blend of animation (0.0f right, 1.0f left)
	TInstanceVar< Int32 > i_prevGroupIdx; // previous group
	TInstanceVar< Int32 > i_prevGroupAnimIdx; // see above
	TInstanceVar< Float > i_prevGroupAnimBlend; // see above
	TInstanceVar< Int32 > i_rightFootBoneIdx;
	TInstanceVar< Int32 > i_leftFootBoneIdx;
	// Variables
	TInstanceVar< Bool > i_hasRequestedMovementDirectionVariable;
	TInstanceVar< Bool > i_hasRequestedFacingDirectionVariable;
	// Internal variables
	TInstanceVar< Bool > i_hasMovementDirectionInternalVariable;
	TInstanceVar< Bool > i_hasGroupDirInternalVariable;

protected: // cached
	Bool m_allInputsValid;
	TDynArray< CBehaviorGraphNode* > m_cachedInputNodes;
	CBehaviorGraphValueNode* m_cachedRequestedMovementDirectionWSValueNode;
	CBehaviorGraphValueNode* m_cachedRequestedFacingDirectionWSValueNode;
	CBehaviorGraphValueNode* m_cachedInitialMovementDirectionWSValueNode;
	CBehaviorGraphValueNode* m_cachedInitialGroupDirMSValueNode;

public:
	CBehaviorGraphDirectionalMovementNode();

	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Directional movement"); }
#endif

public:
	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public: // IBehaviorGraphBonesPropertyOwner
	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

private:
	void AutoCalculateValues();

private:
	RED_INLINE Bool AreAllInputsValid() const;
	RED_INLINE Int32 GetNumberOfAnimations() const { return m_groupCount * m_animsPerGroup; }
	RED_INLINE void GetGroupAndAnimation( Int32 idx, Int32& outGroup, Int32& outAnimInGroup ) const { outAnimInGroup = idx % m_animsPerGroup; outGroup = ( idx - outAnimInGroup ) / m_animsPerGroup; }
	RED_INLINE String CreateSocketName( Int32 groupIdx, Int32 animInGroupIdx ) const;

	RED_INLINE void UpdateRotationAndDirections( CBehaviorGraphInstance& instance, Bool initial = false ) const;

	RED_INLINE Float ChooseStartingPTLBasingOnPreviousPose( CBehaviorGraphInstance& instance, Int32 forGroupIdx ) const;

	// currentGroupIdx -1 -> don't check against current group
	RED_INLINE Int32 FindGroupForDirection( Float dir, Int32 currentGroupIdx = -1) const;
	RED_INLINE Int32 FindClosestGroupForDirectionAvoiding( Float dir, Int32 tryToAvoidGroupIdx = -1) const;
	RED_INLINE Float GetRawDirForGroup( Int32 groupIdx ) const;
	RED_INLINE Float GetDirForGroup( Int32 groupIdx ) const;
	RED_INLINE Float GetDirForGroup180( Int32 groupIdx ) const;

	RED_INLINE void AdvanceFacingDirection( Float& refFacingDirection, const Float requestedFacingDirection, Float timeDelta ) const;
	RED_INLINE Int32 AdvanceMovementDirection( Float& refMovementDirection, const Float requestedMovementDirection, const Int32 groupIdx, Float timeDelta, Float& outClampDifference, Bool allowReversing = false ) const;

	RED_INLINE Int32 NormalizeGroupIndex( Int32 groupIdx ) const;
	RED_INLINE Float TransformPTLToPT( Float ptl ) const;

	// activation and deactivation of whole group (!)
	RED_INLINE void ActivateGroup( CBehaviorGraphInstance& instance, Int32 groupIdx ) const;
	RED_INLINE void DeactivateGroup( CBehaviorGraphInstance& instance, Int32 groupIdx ) const;

	RED_INLINE void UpdateAnimBlendForGroup( CBehaviorGraphInstance& instance, const Float movementDirection, const Int32 groupIdx, Int32& outGroupAnimIdx, Float& outGroupAnimBlend ) const;

	// inputs
	RED_INLINE CBehaviorGraphNode* GetMoreImportantInput( Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const;
	RED_INLINE void GetInputsBasedOnImportance( Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend, CBehaviorGraphNode*& outMoreImportant, CBehaviorGraphNode*& outLessImportant ) const;
	RED_INLINE void GetInputs( Int32 groupIdx, Int32 groupAnimIdx, CBehaviorGraphNode*& outRight, CBehaviorGraphNode*& outLeft ) const;

	// synchronization and update
	RED_INLINE void UpdateAnims( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const;
	RED_INLINE void StartAnimsGroupAt( CBehaviorGraphInstance& instance, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend, Float startAtPTL ) const;
	RED_INLINE void SynchronizeAnimsGroup( CBehaviorGraphInstance& instance, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const;
	RED_INLINE void SynchronizeAnimsBetweenGroups( CBehaviorGraphInstance& instance, Int32 srcGroupIdx, Int32 srcGroupAnimIdx, Float srcGroupAnimBlend, Int32 destGroupIdx, Int32 destGroupAnimIdx, Bool reverseSync = false, Float offsetPTL = 0.0f ) const;
	RED_INLINE void SynchronizeGroupTo( CBehaviorGraphInstance& instance, const CSyncInfo &info, Int32 groupIdx, Int32 groupAnimIdx ) const;
	
	// sampling
	RED_INLINE void SamplePosesForGroup( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const Int32 groupIdx, const Int32 groupAnimIdx, const Float groupAnimBlend ) const;
	RED_INLINE void BlendPoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &first, const SBehaviorGraphOutput &second, const Float alpha, const Bool takeMEfromFirstInput = false ) const;
	RED_INLINE void AdjustTranslation( SBehaviorGraphOutput &output, Float byYaw ) const;

	// events
	RED_INLINE Bool ProcessEventInGroup( CBehaviorGraphInstance& instance, const CBehaviorEvent &event, Int32 groupIdx, Int32 groupAnimIdx ) const;
	RED_INLINE Bool ProcessForceEventInGroup( CBehaviorGraphInstance& instance, const CBehaviorEvent &event, Int32 groupIdx, Int32 groupAnimIdx ) const;
	
	// alpha processing
	RED_INLINE void ProcessActivationAlphaInGroup( CBehaviorGraphInstance& instance, Float alpha, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphDirectionalMovementNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_groupCount, TXT( "Number of groups" ) )
	PROPERTY_EDIT( m_animsPerGroup, TXT( "Number of animations per group" ) )
	PROPERTY_EDIT( m_firstGroupDirOffset, TXT( "Offset for first group" ) )
	PROPERTY_EDIT( m_extraOverlapAngle, TXT( "Extra angle used for overlap with another group" ) )
	PROPERTY_EDIT( m_keepInCurrentGroupAngle, TXT( "Extra buffer angle to keep in current group (to avoid constant switching)" ) )
	PROPERTY_EDIT( m_findGroupDirOffset, TXT( "Offset for dir to prefer some initial choice for groups") )
	PROPERTY_EDIT( m_singleAnimOnly, TXT( "Use single animation only, do not blend between" ) )
	PROPERTY_EDIT( m_doNotSwitchAnim, TXT( "Stay at initial animation, do not switch" ) )
	PROPERTY_EDIT( m_movementDirBlendTime, TXT( "Blend time for blending direction" ) )
	PROPERTY_EDIT( m_movementDirMaxSpeedChange, TXT( "Max speed when changing direction" ) )
	PROPERTY_EDIT( m_groupsBlendTime, TXT( "Blend time for blending groups" ) )
	PROPERTY_EDIT( m_quickTurnBlendTime, TXT( "Blend time for quick turns" ) )
	PROPERTY_EDIT( m_fasterQuickTurnBlendTime, TXT( "Blend time for quick turns that are required to be sped up" ) )
	PROPERTY_EDIT( m_angleThresholdForQuickTurn, TXT( "When angle difference is greater than this value, quick turn is started" ) )
	PROPERTY_EDIT( m_reverseSyncOnQuickTurnFwdBwd, TXT( "Reverse when changing forward/backward" ) )
	PROPERTY_EDIT( m_reverseSyncOnQuickTurnLeftRight, TXT( "Reverse when changing left/right" ) )
	PROPERTY_EDIT( m_syncBlendingOffsetPTLOnQuickTurn, TXT( "Syncing offset when doing quick turn" ) )
	PROPERTY_EDIT( m_startPTLRightFootInFront, TXT( "Starting prop through loop from right foot in front" ) )
	PROPERTY_EDIT( m_startPTLLeftFootInFront, TXT( "Starting prop through loop from left foot in front" ) )
	PROPERTY_EDIT( m_alwaysStartAtZero, TXT( "Always start at beginning of the animation" ) )
	PROPERTY_EDIT( m_useSimpleBlendForMovementDelta, TXT( "Use simple, linear blend for movement delta, if it is off - delta will be calculated from blend of yaw and blend of speed" ) )
	PROPERTY_EDIT( m_useDefinedVariablesAsRequestedInput, TXT( "If no variables connected, use stored variables as requested input" ) )
	PROPERTY_CUSTOM_EDIT( m_requestedMovementDirectionVariableName, TXT("Requested movement direction variable name (used if variable not connected)"), TXT("BehaviorVariableSelection") )
	PROPERTY_CUSTOM_EDIT( m_requestedFacingDirectionVariableName, TXT("Requested facing direction variable name (used if variable not connected)"), TXT("BehaviorVariableSelection") )
	PROPERTY_EDIT( m_useDefinedInternalVariablesAsInitialInput, TXT( "If no variables connected, use stored internal variables as initial input" ) )
	PROPERTY_CUSTOM_EDIT( m_movementDirectionInternalVariableName, TXT("Movement direction internal variable name"), TXT("BehaviorInternalVariableSelection") )
	PROPERTY_CUSTOM_EDIT( m_groupDirInternalVariableName, TXT("Group dir internal variable name"), TXT("BehaviorInternalVariableSelection") )
	PROPERTY_EDIT( m_loopCount, TXT( "Number of movement loops per animation loop" ) )
	PROPERTY_EDIT( m_syncGroupOffsetPTL, TXT( "Sync offset for group, when there is difference between group idx, this value is added" ) )
	PROPERTY_CUSTOM_EDIT_NAME( m_rightFootBone, TXT("Right foot bone name"), TXT("Right foot bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_CUSTOM_EDIT_NAME( m_leftFootBone, TXT("Left foot bone name"), TXT("Left foot bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY( m_groupDir );
	PROPERTY( m_sideAngleRange );
	PROPERTY( m_syncMethod );
	PROPERTY( m_allInputsValid );
	PROPERTY( m_cachedInputNodes );
	PROPERTY( m_cachedRequestedMovementDirectionWSValueNode );
	PROPERTY( m_cachedRequestedFacingDirectionWSValueNode );
	PROPERTY( m_cachedInitialMovementDirectionWSValueNode );
	PROPERTY( m_cachedInitialGroupDirMSValueNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

/**
 *	It works very similar to directional movement node.
 *	Exceptions/features:
 *		allows only little blending for movement and facing directions
 *		no blending between groups allowed
 */
class CBehaviorGraphDirectionalMovementStartNode : public CBehaviorGraphDirectionalMovementNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDirectionalMovementStartNode, CBehaviorGraphDirectionalMovementNode, "Movement", "Directional movement (start)" );

public:
	CBehaviorGraphDirectionalMovementStartNode();
	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Directional movement (start)"); }
#endif

protected:
	void DisableBlending();
};

BEGIN_CLASS_RTTI( CBehaviorGraphDirectionalMovementStartNode );
	PARENT_CLASS( CBehaviorGraphDirectionalMovementNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

/**
 *	It works very similar to directional movement start node.
 *	Exceptions/features:
 *		by default assumes that movement direction should be read from internal variables
 */
class CBehaviorGraphDirectionalMovementStopNode : public CBehaviorGraphDirectionalMovementStartNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDirectionalMovementStopNode, CBehaviorGraphDirectionalMovementStartNode, "Movement", "Directional movement (stop)" );

public:
	CBehaviorGraphDirectionalMovementStopNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Directional movement (stop)"); }
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphDirectionalMovementStopNode );
	PARENT_CLASS( CBehaviorGraphDirectionalMovementStartNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
