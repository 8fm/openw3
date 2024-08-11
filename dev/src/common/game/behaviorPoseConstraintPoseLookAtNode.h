/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../../common/engine/behaviorPoseConstraintNode.h"

#include "behaviorPoseConstraintPoseLookAtModifier.h"
#include "behaviorPoseConstraintPoseLookAtSegment.h"

class CBehaviorGraphPoseConstraintPoseLookAtNode	: public CBehaviorGraphPoseConstraintWithTargetNode
													, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseConstraintPoseLookAtNode, CBehaviorGraphPoseConstraintWithTargetNode, "Pose Constraints", "Pose look at" );

protected:
	TDynArray< SPoseLookAtSegmentData >				m_dataSegments;

	TDynArray< IBehaviorPoseConstraintPoseLookAtModifier* > m_modifiers;

protected:
	TInstanceVar< CEntity* >						i_owner;
	TInstanceVar< Bool >							i_valid;
	TInstanceVar< TDynArray< SPoseLookAtSegment > > i_segments;
	TInstanceVar< Int32 >							i_targetId;
	TInstanceVar< Bool >							i_active;
	TInstanceVar< Bool >							i_isLimit;
public:

	CBehaviorGraphPoseConstraintPoseLookAtNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
	virtual void InternalReset( CBehaviorGraphInstance& instance ) const;

	virtual void CalcAnglesForSegment(	CBehaviorGraphInstance& instance, 
										Float hAngle, Float vAngle, 
										const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, CLookAtContextParam& contextParam, 
										Bool isTargetChanged, Bool instant,
										SPoseLookAtSegment& segment ) const;

protected:
	Bool IsValid( CBehaviorGraphInstance& instance ) const;
	Bool CreateSegments( CBehaviorGraphInstance& instance ) const;

	Bool LimitAngles( Float& hAngle, Float& vAngle, Float hAnglePose, Float vAnglePose, const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, const SPoseLookAtSegment& segment ) const;
	Float CalcAngleThreshold( Float angle, Float thresholdAngleDifference ) const;
	void ClampAngles( Float& hAngle, Float& vAngle, const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, const SPoseLookAtSegment& segment ) const;
	Float DampAngle( Float angle, Float angleThr, Float maxAngleDifference, Float bendingMultiplier ) const;

	ELookAtLevel GetLevel( const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam ) const;

	Bool GetLookAtParams( CBehaviorGraphInstance& instance, const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const;
	void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseConstraintPoseLookAtNode );
	PARENT_CLASS( CBehaviorGraphPoseConstraintWithTargetNode );
	PROPERTY_EDIT( m_dataSegments, TXT("") );
	PROPERTY_INLINED( m_modifiers, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SPoseLookAtSegmentDampData
{
	DECLARE_RTTI_STRUCT( SPoseLookAtSegmentDampData );

	SPoseLookAtSegmentDampData();

	Float m_progress;
};

BEGIN_CLASS_RTTI( SPoseLookAtSegmentDampData );
END_CLASS_RTTI();

class CBehaviorGraphPoseConstraintPoseCurveLookAtNode	: public CBehaviorGraphPoseConstraintPoseLookAtNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseConstraintPoseCurveLookAtNode, CBehaviorGraphPoseConstraintPoseLookAtNode, "Pose Constraints", "Pose curve look at" );

protected:
	Bool					m_useCurve;
	CCurve*					m_curve;
	Bool					m_doubleDamp;

protected:
	TInstanceVar< TDynArray< SPoseLookAtSegmentDampData > > i_segmentsDampData;

public:

	CBehaviorGraphPoseConstraintPoseCurveLookAtNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
	virtual void InternalReset( CBehaviorGraphInstance& instance ) const;

	virtual void CalcAnglesForSegment(	CBehaviorGraphInstance& instance, 
										Float hAngle, Float vAngle, 
										const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, CLookAtContextParam& contextParam,
										Bool isTargetChanged, Bool instant,
										SPoseLookAtSegment& segment ) const;

protected:
	void CreateSegmentsDampData( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseConstraintPoseCurveLookAtNode );
	PARENT_CLASS( CBehaviorGraphPoseConstraintPoseLookAtNode );
	PROPERTY_EDIT( m_useCurve, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("CurveSelection") );
	PROPERTY_EDIT( m_doubleDamp, TXT("") );
END_CLASS_RTTI();
