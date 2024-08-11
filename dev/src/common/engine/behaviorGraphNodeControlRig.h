
#pragma once

class TCrInstance;

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphControlRigNode : public CBehaviorGraphBaseNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphControlRigNode, CBehaviorGraphBaseNode, "Constraints", "Control rig" );

protected:
	CName				m_eHandLeftW;
	CName				m_eHandLeftP;
	CName				m_eHandLeftWeaponOffset;
	Bool				m_offsetHandLeft;

	CName				m_eHandRightW;
	CName				m_eHandRightP;
	CName				m_eHandRightWeaponOffset;
	Bool				m_offsetHandRight;

protected:
	TInstanceVar< TGenericPtr > i_controlRigPtr;
	TInstanceVar< Bool >		i_manualControl;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Control rig") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
#endif

public:
	CBehaviorGraphControlRigNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

public:
	void SetManualControl( CBehaviorGraphInstance& instance, Bool flag ) const;
	TCrInstance* GetControlRig( CBehaviorGraphInstance& instance ) const;

private:
	TCrInstance* CreateControlRig( CBehaviorGraphInstance& instance ) const;
	void DestroyControlRig( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const;
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphControlRigNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_eHandLeftW, TXT("") );
	PROPERTY_EDIT( m_eHandLeftP, TXT("") );
	PROPERTY_EDIT( m_eHandRightW, TXT("") );
	PROPERTY_EDIT( m_eHandRightP, TXT("") );
	PROPERTY_EDIT( m_offsetHandLeft, TXT("") );
	PROPERTY_EDIT( m_offsetHandRight, TXT("") );
	PROPERTY_EDIT( m_eHandLeftWeaponOffset, TXT("") );
	PROPERTY_EDIT( m_eHandRightWeaponOffset, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphControlRigInterface
{
	CBehaviorGraphControlRigNode*			m_node;
	CBehaviorGraphInstance*					m_instance;
	THandle< CBehaviorGraphInstance >		m_instanceH;

public:
	CBehaviorGraphControlRigInterface();
	~CBehaviorGraphControlRigInterface();

	void Init( CBehaviorGraphControlRigNode* node, CBehaviorGraphInstance* instance );
	void Clear();

	Bool IsValid() const;

	void SetManualControl( Bool flag );
	
	TCrInstance* GetControlRig();
	const TCrInstance* GetControlRig() const;
};
