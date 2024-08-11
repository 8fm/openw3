#pragma once
#include "..\..\..\..\engine\behaviorGraphNode.h"
#include "..\..\..\..\engine\Behavior\Tools\common.h"
#include "..\..\..\..\engine\Behavior\SharedHeaders\enumSide.h"

//////////////////////////////////////////////////////////////////////////
struct SFootDetectionBoneInfo
{
	DECLARE_RTTI_STRUCT( SFootDetectionBoneInfo )

	Vector	m_prevFrameLocalPos;
	Int32	m_boneIdx;
	Uint16	m_eventFrameCounter;
_DBG_ONLY_CODE_( Bool m_debugFoundStepThisFrame; )
//-----------------------------------------------

	SFootDetectionBoneInfo() 
		: m_prevFrameLocalPos(Vector::ZEROS)
		, m_boneIdx(-1)
		, m_eventFrameCounter(0)
_DBG_ONLY_CODE_( , m_debugFoundStepThisFrame(false) )
		{}
};

BEGIN_CLASS_RTTI( SFootDetectionBoneInfo );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
class CBehaviorGraphFootStepDetectorNode : public CBehaviorGraphBaseNode
{
public:
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFootStepDetectorNode, CBehaviorGraphBaseNode, "Measurements", "Foot Step Detector" );

	TInstanceVar< SFootDetectionBoneInfo >	i_leftFootBoneInfo;
	TInstanceVar< SFootDetectionBoneInfo >	i_rightFootBoneInfo;

	TInstanceVar< Float >	i_dt;
	//------------------------------------------------------------
	
	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override; 
	void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	String GetCaption() const override { return TXT("Foot Step Detector >Prorotype!<"); }
#endif

protected:
	Bool DetectFootStep( const Vector& localBonePosThisFrame, const Vector& localBonePosePrevFrame, const Vector& delatRefFrame, Float deltaRefFrameAvgSpeed, Float dt ) const;
	void UpdateFootDetectionForBone( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output, SFootDetectionBoneInfo& boneInfo, const Vector& deltaRefFrame, Float deltaRefAvgSpeed, Float dt, ESide side ) const;
	Bool UpdateFootStepActionForBone( Int32 boneIdx, SBehaviorSampleContext& context, Bool shouldSendThisFrame, Uint16& eventFrameCounter ) const;
	void GenerateFootStepAction( Int32 boneIdx, SBehaviorSampleContext& context ) const;
	Bool DoesEventOccured( const SBehaviorGraphOutput &output, ESide side ) const;

private:
_DBG_ONLY_CODE_( void RenderDebug( const CAnimatedComponent* ac, const SFootDetectionBoneInfo& boneInfo, const Color& color ) const; )

};

BEGIN_CLASS_RTTI( CBehaviorGraphFootStepDetectorNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
END_CLASS_RTTI();
//////////////////////////////////////////////////////////////////////////