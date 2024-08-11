/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphCurveDampValueNode	: public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCurveDampValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Curve damp value" );		

protected:
	CCurve*					m_curve;
	Float					m_abscissaAxisScale;

protected:
	TInstanceVar< Float	>	i_targetValue;
	TInstanceVar< Float	>	i_prevTarget;
	TInstanceVar< Float	>	i_curveTimer;

protected:
	CBehaviorGraphValueNode* m_cachedDurationValueNode;

public:
	CBehaviorGraphCurveDampValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Curve damp value") ); }
	virtual void OnSpawned(const GraphBlockSpawnInfo& info );
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphCurveDampValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_CUSTOM_EDIT( m_curve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_EDIT( m_abscissaAxisScale, TXT("Abscissa axis scale for curve") );
	PROPERTY( m_cachedDurationValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphCurveMapValueNode	: public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCurveMapValueNode, CBehaviorGraphValueBaseNode, "Float", "Curve map value" );		

protected:
	CCurve*					m_curve;
	Float					m_axisXScale;
	Float					m_valueScale;
	Float					m_valueOffet;
	Bool					m_mirrorY;

public:
	CBehaviorGraphCurveMapValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Curve map value") ); }
	virtual void OnSpawned(const GraphBlockSpawnInfo& info );
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;

private:
	void CalcValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCurveMapValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_CUSTOM_EDIT( m_curve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_EDIT( m_axisXScale, TXT("") );
	PROPERTY_EDIT( m_valueScale, TXT("") );
	PROPERTY_EDIT( m_valueOffet, TXT("") );
	PROPERTY_EDIT( m_mirrorY, TXT("") );
END_CLASS_RTTI();
