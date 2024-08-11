/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

// Bone used for rotating a bone
class CBehaviorGraphTranslateBoneNode	: public CBehaviorGraphBaseNode
										, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphTranslateBoneNode, CBehaviorGraphBaseNode, "Misc", "Translate bone" );

protected:
	String					m_boneName;					//!< Name of the bone we want to rotate
	Vector					m_axis;						//!< Translation axis
	Float					m_scale;					//!< Scaling applied to input value
	Float					m_biasValue;				//!< Biasing applied to input value
	Float					m_minValue;					//!< Clamping on negative side
	Float					m_maxValue;					//!< Clamping on positive side
	Bool					m_clampValue;				//!< Should we clamp value

protected:
	TInstanceVar< Int32 >		i_boneIndex;				//!< Cached bone index
	TInstanceVar< Float >	i_currentValue;				//!< Current value of translation

protected:
	CBehaviorGraphValueNode*	m_cachedValueNode;			//!< ( Connection Cache ) Control variable

public:
	CBehaviorGraphTranslateBoneNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Bone translation") ); }
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

BEGIN_CLASS_RTTI( CBehaviorGraphTranslateBoneNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to translate"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_axis, TXT("Rotation axis") );
	PROPERTY_EDIT( m_scale, TXT("Input value scale") );
	PROPERTY_EDIT( m_biasValue, TXT("Input value bias ( applied after scale )") );
	PROPERTY_EDIT( m_minValue, TXT("Maximum rotation allowed in negative direction") );
	PROPERTY_EDIT( m_maxValue, TXT("Maximum rotation allowed in positive direction") );
	PROPERTY_EDIT( m_clampValue, TXT("Clamp rotation") );
	PROPERTY( m_cachedValueNode );
END_CLASS_RTTI();

