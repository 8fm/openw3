/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorConstraintNode.h"
#include "behaviorIkSolverChain.h"

class CBehaviorGraphConstraintNodeChain	: public CBehaviorGraphConstraintNode
										, public IChainSolver
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeChain, CBehaviorGraphConstraintNode, "Constraints", "Chain" );

protected:
	String					m_startBone;
	String					m_endBone;
	Int32						m_solverSteps;
	EAxis					m_forwardEndBoneDir;

protected:
	TInstanceVar< Int32 >		i_startBoneIndex;
	TInstanceVar< Int32 >		i_endBoneIndex;

public:
	CBehaviorGraphConstraintNodeChain();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
#ifdef USE_HAVOK_ANIMATION
	virtual hkQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#else
	virtual RedQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeChain );
	PARENT_CLASS( CBehaviorGraphConstraintNode );
	PROPERTY_CUSTOM_EDIT( m_startBone, TXT("Start bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_endBone, TXT("Start bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_solverSteps, TXT("Solver steps") );
	PROPERTY_EDIT( m_forwardEndBoneDir, TXT("") );
END_CLASS_RTTI();
