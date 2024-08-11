
#pragma once

struct SHorseStateOffsets
{
	DECLARE_RTTI_STRUCT( SHorseStateOffsets )

	Float m_speedValue;
	Float m_maxValue;

	Float m_legFY;
	Float m_legFZ;

	Float m_legBY;
	Float m_legBZ;

	Float m_pelvisY;
	Float m_pelvisZ;

	Float m_headFirstAngle;
	Float m_headSecondAngle;
	Float m_headThirdAngle;
};

BEGIN_CLASS_RTTI( SHorseStateOffsets );
	PROPERTY_EDIT( m_speedValue, TXT( "" ) );
	PROPERTY_EDIT( m_maxValue, TXT( "" ) );
	PROPERTY_EDIT( m_legFY, TXT( "" ) );
	PROPERTY_EDIT( m_legFZ, TXT( "" ) );
	PROPERTY_EDIT( m_legBY, TXT( "" ) );
	PROPERTY_EDIT( m_legBZ, TXT( "" ) );
	PROPERTY_EDIT( m_pelvisY, TXT( "" ) );
	PROPERTY_EDIT( m_pelvisZ, TXT( "" ) );
	PROPERTY_EDIT( m_headFirstAngle, TXT( "" ) );
	PROPERTY_EDIT( m_headSecondAngle, TXT( "" ) );
	PROPERTY_EDIT( m_headThirdAngle, TXT( "" ) );
END_CLASS_RTTI();

class CBehaviorGraphHorseNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphHorseNode, CBehaviorGraphBaseNode, "Gameplay", "Horse" );

protected:
	CName		m_slopeFBVar;
	CName		m_slopeLRVar;

	String		m_firstBoneF;
	String		m_secondBoneF;
	String		m_thirdBoneF;
	String		m_endBoneF;
	EAxis		m_hingeAxisF;

	String		m_firstBoneB;
	String		m_secondBoneB;
	String		m_thirdBoneB;
	String		m_endBoneB;
	EAxis		m_hingeAxisB;

	String		m_pelvis;
	String		m_root;
	EAxis		m_axisRootFB;
	EAxis		m_axisRootLR;

	String		m_headFirst;
	String		m_headSecond;
	String		m_headThird;
	EAxis		m_hingeAxisHead;

	Float		m_speedStep;

	SHorseStateOffsets m_walkFBP;
	SHorseStateOffsets m_trotFBP;
	SHorseStateOffsets m_gallopFBP;
	SHorseStateOffsets m_canterFBP;

	SHorseStateOffsets m_walkFBN;
	SHorseStateOffsets m_trotFBN;
	SHorseStateOffsets m_gallopFBN;
	SHorseStateOffsets m_canterFBN;

	SHorseStateOffsets m_walkLR;
	SHorseStateOffsets m_trotLR;
	SHorseStateOffsets m_gallopLR;
	SHorseStateOffsets m_canterLR;

protected:
	TInstanceVar< Float >				i_slopeFB;
	TInstanceVar< Float >				i_slopeLR;

	TInstanceVar< Float >				i_speedWeight;

	TInstanceVar< Int32 >					i_rootIdx;
	TInstanceVar< Int32 >					i_pelvisIdx;

	TInstanceVar< Int32 >					i_headFirstIdx;
	TInstanceVar< Int32 >					i_headSecondIdx;
	TInstanceVar< Int32 >					i_headThirdIdx;

	TInstanceVar< TDynArray< Int32 > >	i_bones;
	TInstanceVar< TDynArray< Int32 > >	i_variables;

protected:
	CBehaviorGraphValueNode*			m_cachedSpeedValueNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Horse") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
	virtual void OnSpawned(const GraphBlockSpawnInfo& info );
#endif

public:
	CBehaviorGraphHorseNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
	void RotateRoot( AnimQsTransform& bone, const SHorseStateOffsets& stateFB, Float weightFB, const SHorseStateOffsets& stateLR, Float weightLR ) const;
	void TranslatePelvis( AnimQsTransform& bone, const SHorseStateOffsets& state, Float weight ) const;
	void RotateHead( AnimQsTransform& h1, AnimQsTransform& h2, AnimQsTransform& h3, const SHorseStateOffsets& state, Float weight ) const;
	void ProcessLegsIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SHorseStateOffsets& stateFB, Float weightFB, const SHorseStateOffsets& stateLR, Float weightLR ) const;
	void ProcessLegIK( SBehaviorGraphOutput &output, Float y, Float z, Int32 boneA, Int32 boneB, Int32 boneC, Int32 boneD ) const;

	const SHorseStateOffsets& FindOffsetStateFB( Float slope, Float speedWeight, Float& weight ) const;
	const SHorseStateOffsets& FindOffsetStateLR( Float slope, Float speedWeight, Float& weight ) const;
	const SHorseStateOffsets& FindOffsetState( Float slope, Float speedWeight, Float& weight, 
		const SHorseStateOffsets& canter, const SHorseStateOffsets& gallop, const SHorseStateOffsets& trot, const SHorseStateOffsets& walk ) const;

	Float CalcWeightForState( const SHorseStateOffsets& step, Float slope, Float speedWeight ) const;


};

BEGIN_CLASS_RTTI( CBehaviorGraphHorseNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedSpeedValueNode );
	PROPERTY_EDIT( m_speedStep, TXT("") );
	PROPERTY_EDIT( m_slopeFBVar, TXT("") );
	PROPERTY_EDIT( m_slopeLRVar, TXT("") );
	PROPERTY_EDIT( m_firstBoneF, TXT("") );
	PROPERTY_EDIT( m_secondBoneF, TXT("") );
	PROPERTY_EDIT( m_thirdBoneF, TXT("") );
	PROPERTY_EDIT( m_endBoneF, TXT("") );
	PROPERTY_EDIT( m_hingeAxisF, TXT("") );
	PROPERTY_EDIT( m_firstBoneB, TXT("") );
	PROPERTY_EDIT( m_secondBoneB, TXT("") );
	PROPERTY_EDIT( m_thirdBoneB, TXT("") );
	PROPERTY_EDIT( m_endBoneB, TXT("") );
	PROPERTY_EDIT( m_hingeAxisB, TXT("") );
	PROPERTY_EDIT( m_root, TXT("") );
	PROPERTY_EDIT( m_pelvis, TXT("") );
	PROPERTY_EDIT( m_axisRootFB, TXT("") );
	PROPERTY_EDIT( m_axisRootLR, TXT("") );
	PROPERTY_EDIT( m_headFirst, TXT("") );
	PROPERTY_EDIT( m_headSecond, TXT("") );
	PROPERTY_EDIT( m_headThird, TXT("") );
	PROPERTY_EDIT( m_hingeAxisHead, TXT("") );
	PROPERTY_EDIT( m_walkFBP, TXT("") );
	PROPERTY_EDIT( m_trotFBP, TXT("") );
	PROPERTY_EDIT( m_gallopFBP, TXT("") );
	PROPERTY_EDIT( m_canterFBP, TXT("") );
	PROPERTY_EDIT( m_walkFBN, TXT("") );
	PROPERTY_EDIT( m_trotFBN, TXT("") );
	PROPERTY_EDIT( m_gallopFBN, TXT("") );
	PROPERTY_EDIT( m_canterFBN, TXT("") );
	PROPERTY_EDIT( m_walkLR, TXT("") );
	PROPERTY_EDIT( m_trotLR, TXT("") );
	PROPERTY_EDIT( m_gallopLR, TXT("") );
	PROPERTY_EDIT( m_canterLR, TXT("") );
END_CLASS_RTTI();
