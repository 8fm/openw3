/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphChooseRecoverFromRagdollAnimNode;

///////////////////////////////////////////////////////////////////////////////

enum EBehaviorGraphChooseRecoverFromRagdollAnimMode
{
	RFR_JustOne,
	RFR_FrontBack,
	RFR_FrontBackSides,
};

BEGIN_ENUM_RTTI( EBehaviorGraphChooseRecoverFromRagdollAnimMode );
	ENUM_OPTION( RFR_JustOne );
	ENUM_OPTION( RFR_FrontBack );
	ENUM_OPTION( RFR_FrontBackSides );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphChooseRecoverFromRagdollAnimNode : public CBehaviorGraphNode
													 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphChooseRecoverFromRagdollAnimNode, CBehaviorGraphNode, "Ragdoll", "Recover from ragdoll (choose anim)" );

protected:
	EBehaviorGraphChooseRecoverFromRagdollAnimMode m_mode;
	//
	Float m_additionalOneFrameRotationYaw;
	CName m_pelvisBone;
	EAxis m_pelvisBoneFrontAxis;
	Bool m_pelvisBoneFrontAxisInverted;
	Float m_pelvisBoneWeight;
	CName m_shoulderBone;
	EAxis m_shoulderBoneFrontAxis;
	Bool m_shoulderBoneFrontAxisInverted;
	Float m_shoulderBoneWeight;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< Int32 > i_pelvisBoneIdx;
	TInstanceVar< Int32 > i_shoulderBoneIdx;
	TInstanceVar< Int32 > i_chosenChildIdx;
	TInstanceVar< EulerAngles > i_oneFrameRotation;
	TInstanceVar< Vector > i_groundNormalWS;
	TInstanceVar< Float > i_groundNormalWeight;

protected: // cached
	TDynArray< CBehaviorGraphNode* > m_cachedInputNodes;

public:
	CBehaviorGraphChooseRecoverFromRagdollAnimNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Recover from ragdoll (choose anim)"); }
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

public: // IBehaviorGraphBonesPropertyOwner
	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

private:
	Int32 GetNumberOfChildren() const;
	String CreateSocketName( Int32 childIdx ) const;

	void ChooseAnimAndRotation( CBehaviorGraphInstance& instance ) const;
	void IncreaseFrontVector( CAnimatedComponent const * ac, Int32 boneIdx, EAxis frontAxis, Bool invertAxis, Float boneWeight, Vector& inOutFrontVector, Float& inOutWeight ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphChooseRecoverFromRagdollAnimNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_mode, TXT( "Mode of choosing (defining number of anims)" ) )
	PROPERTY_EDIT( m_additionalOneFrameRotationYaw, TXT( "" ) )
	PROPERTY_CUSTOM_EDIT( m_pelvisBone, TXT( "" ), TXT( "BehaviorBoneSelection" ) )
	PROPERTY_EDIT( m_pelvisBoneFrontAxis, TXT( "" ) )
	PROPERTY_EDIT( m_pelvisBoneFrontAxisInverted, TXT( "" ) )
	PROPERTY_EDIT( m_pelvisBoneWeight, TXT( "" ) )
	PROPERTY_CUSTOM_EDIT( m_shoulderBone, TXT( "" ), TXT( "BehaviorBoneSelection" ) )
	PROPERTY_EDIT( m_shoulderBoneFrontAxis, TXT( "" ) )
	PROPERTY_EDIT( m_shoulderBoneFrontAxisInverted, TXT( "" ) )
	PROPERTY_EDIT( m_shoulderBoneWeight, TXT( "" ) )
	PROPERTY( m_cachedInputNodes )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
