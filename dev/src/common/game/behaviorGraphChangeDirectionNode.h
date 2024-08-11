/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

///////////////////////////////////////////////////////////////////////////////

#include "../engine/behaviorGraphValueNode.h"

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphChangeDirectionNode;

///////////////////////////////////////////////////////////////////////////////

enum EChangeDirectionSide
{
	BGCDS_None,
	BGCDS_Left,
	BGCDS_Right,
	BGCDS_Any,
};

BEGIN_ENUM_RTTI( EChangeDirectionSide );
	ENUM_OPTION( BGCDS_None );
	ENUM_OPTION( BGCDS_Left );
	ENUM_OPTION( BGCDS_Right );
	ENUM_OPTION( BGCDS_Any );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

/**
 *	Returns blend value for different angles requested
 */
class CBehaviorGraphChangeDirectionNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphChangeDirectionNode, CBehaviorGraphValueNode, "Movement", "Blend direction" );

protected:
	Bool				m_anyDirection;
	TDynArray< Float >	m_angles; // angles for blend values
	Float				m_overshootAngle;
	Bool				m_updateOnlyOnActivation; // update blend values only when activated
	Float				m_dirBlendTime;
	Float				m_dirMaxBlendSpeed;
	CName				m_requestedFacingDirectionWSVariableName; // requested facing direction (float variable)

protected:
	TInstanceVar< Float > i_startingDirectionWS;
	TInstanceVar< Float > i_facingDirection;
	TInstanceVar< Float > i_blendValue;

protected:
	CBehaviorGraphValueNode* m_cachedRequestedFacingDirectionWSValueNode;

public:
	CBehaviorGraphChangeDirectionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property ) override;
	virtual void OnRebuildSockets() override;

	virtual String GetCaption() const override;
#endif

public:
	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

private:
	RED_INLINE void CalculateBlendValue( CBehaviorGraphInstance& instance ) const;
	RED_INLINE Float GetRequestedFacingDirection( CBehaviorGraphInstance& instance ) const;

	void MakeSureAnglesAreValid();
};

BEGIN_CLASS_RTTI( CBehaviorGraphChangeDirectionNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_anyDirection, TXT( "" ) )
	PROPERTY_EDIT( m_angles, TXT( "Agnles for blend values" ) )
	PROPERTY_EDIT( m_updateOnlyOnActivation, TXT( "Update blend values only when activated" ) )
	PROPERTY_EDIT( m_dirBlendTime, TXT( "Facing fir blend time" ) )
	PROPERTY_EDIT( m_dirMaxBlendSpeed, TXT( "Facing dir max speed time" ) )
	PROPERTY_EDIT( m_overshootAngle, TXT( "Additional angle to overshoot requested direction" ) )
	PROPERTY_CUSTOM_EDIT( m_requestedFacingDirectionWSVariableName, TXT("Requested facing direction variable name (used if variable not connected)"), TXT("BehaviorVariableSelection") );
	PROPERTY( m_cachedRequestedFacingDirectionWSValueNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
