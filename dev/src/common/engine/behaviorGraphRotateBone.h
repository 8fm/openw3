/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

// Bone rotation axis
enum EBoneRotationAxis
{
	ROTAXIS_X,
	ROTAXIS_Y,
	ROTAXIS_Z,
};

BEGIN_ENUM_RTTI( EBoneRotationAxis );
	ENUM_OPTION( ROTAXIS_X );
	ENUM_OPTION( ROTAXIS_Y );
	ENUM_OPTION( ROTAXIS_Z );
END_ENUM_RTTI();

// Bone used for rotating a bone
class CBehaviorGraphRotateBoneNode	: public CBehaviorGraphBaseNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRotateBoneNode, CBehaviorGraphBaseNode, "Misc", "Rotate bone" );

protected:
	String					m_boneName;					//!< Name of the bone we want to rotate
	EBoneRotationAxis		m_axis;						//!< Rotation axis
	Float					m_scale;					//!< Scaling applied to input rotation angle
	Float					m_biasAngle;				//!< Biasing
	Float					m_minAngle;					//!< Clamping on negative side
	Float					m_maxAngle;					//!< Clamping on positive side
	Bool					m_clampRotation;			//!< Should we clamp rotation
	Bool					m_localSpace;				//!< Perform rotation in local space

protected:
	TInstanceVar< Int32 >		i_boneIndex;				//!< Cached bone index
	TInstanceVar< Float >	i_currentAngle;				//!< Current value of rotation angle

protected:
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;
	CBehaviorGraphValueNode*	m_cachedAngleMinNode;
	CBehaviorGraphValueNode*	m_cachedAngleMaxNode;

public:
	CBehaviorGraphRotateBoneNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Bone rotation") ); }
	virtual void OnPropertyPostChange( IProperty* property );
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphRotateBoneNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to rotate"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_axis, TXT("Rotation axis") );
	PROPERTY_EDIT( m_scale, TXT("Input angle scale") );
	PROPERTY_EDIT( m_biasAngle, TXT("Input angle bias ( applied after scale )") );
	PROPERTY_EDIT( m_minAngle, TXT("Maximum rotation allowed in negative direction") );
	PROPERTY_EDIT( m_maxAngle, TXT("Maximum rotation allowed in positive direction") );
	PROPERTY_EDIT( m_clampRotation, TXT("Clamp rotation") );
	PROPERTY_EDIT( m_localSpace, TXT("Rotate bone in local space instead of model") );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedAngleMinNode );
	PROPERTY( m_cachedAngleMaxNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRotateLimitNode	: public CBehaviorGraphBaseNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRotateLimitNode, CBehaviorGraphBaseNode, "Misc", "Rotate limit" );

protected:
	String					m_boneName;
	EAxis					m_axis;
	Float					m_minAngle;	
	Float					m_maxAngle;	

protected:
	TInstanceVar< Int32 >		i_boneIndex;

public:
	CBehaviorGraphRotateLimitNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Bone rotation limit") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRotateLimitNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to rotate"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_axis, TXT("Limit axis") );
	PROPERTY_EDIT( m_minAngle, TXT("Maximum rotation allowed in negative direction") );
	PROPERTY_EDIT( m_maxAngle, TXT("Maximum rotation allowed in positive direction") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////





class CBehaviorGraphScaleBoneNode	: public CBehaviorGraphBaseNode
	, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphScaleBoneNode, CBehaviorGraphBaseNode, "Misc", "Scale bone" );

protected:
	String					m_boneName;				
	Vector					m_scale;					
protected:
	TInstanceVar< Int32 >		i_boneIndex;		
	TInstanceVar< Vector >		i_currentScale;				

protected:
	CBehaviorGraphVectorValueNode*	m_cachedControlVariableNode;

public:
	CBehaviorGraphScaleBoneNode() : m_scale( Vector::ONES ), m_cachedControlVariableNode( nullptr )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Bone scale") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphScaleBoneNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to rotate"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_scale, TXT("Scale") );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();
