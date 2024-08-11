/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintStirrupsCommmonData;
struct SBehaviorConstraintStirrupsCommmon;
struct SBehaviorConstraintStirrupData;
struct SBehaviorConstraintStirrup;
class CBehaviorConstraintStirrups;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintStirrupsCommmonData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintStirrupsCommmonData );

	Float m_speedForPerpendicular;
	Float m_weight;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintStirrupsCommmonData );
	PROPERTY_EDIT( m_speedForPerpendicular, TXT("" ) );
	PROPERTY_EDIT( m_weight, TXT("" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintStirrupsCommmon
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintStirrupsCommmon );

	Vector m_prevLocation;
	Float m_bePerpendicular;

	void Update( CMovingAgentComponent* mac, const SBehaviorConstraintStirrupsCommmonData & data, Float timeDelta );
	void Reset();
};

BEGIN_CLASS_RTTI( SBehaviorConstraintStirrupsCommmon );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintStirrupData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintStirrupData );

	CName m_stirrupBoneName;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintStirrupData );
	PROPERTY_CUSTOM_EDIT( m_stirrupBoneName, TXT("Stirrup bone name"), TXT( "BehaviorBoneSelection" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintStirrup
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintStirrup );

	Float m_weight;
	Int32 m_stirrupBoneIdx;
	Int32 m_stirrupParentBoneIdx;
	Int32 m_stirrupGrandParentBoneIdx;

	SBehaviorConstraintStirrup();

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintStirrupData & data );

	void UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, Bool blendOut, const SBehaviorConstraintStirrupData & data, const SBehaviorConstraintStirrupsCommmonData & commonData, const SBehaviorConstraintStirrupsCommmon & common, Float timeDelta );
};

BEGIN_CLASS_RTTI( SBehaviorConstraintStirrup );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintStirrups : public CBehaviorGraphPoseConstraintNode
								  , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintStirrups, CBehaviorGraphPoseConstraintNode, "Constraints", "Stirrups" );

protected:
	SBehaviorConstraintStirrupsCommmonData m_common;
	SBehaviorConstraintStirrupData m_left;
	SBehaviorConstraintStirrupData m_right;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< SBehaviorConstraintStirrup > i_left;
	TInstanceVar< SBehaviorConstraintStirrup > i_right;
	TInstanceVar< SBehaviorConstraintStirrupsCommmon > i_common;

public:
	CBehaviorConstraintStirrups();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Stirrups" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintStirrups );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_common, TXT("") );
	PROPERTY_EDIT( m_left, TXT("") );
	PROPERTY_EDIT( m_right, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
