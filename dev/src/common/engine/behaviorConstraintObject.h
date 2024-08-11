/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../core/object.h"
#include "../core/engineQsTransform.h"
#include "../core/instanceVar.h"
#include "behaviorIncludes.h"

class CBehaviorGraphConstraintNode;
struct SBehaviorUpdateContext;
class InstanceDataLayoutCompiler;
//////////////////////////////////////////////////////////////////////////

class IBehaviorConstraintObject : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorConstraintObject, CObject );

protected:
	Vector								m_localPositionOffset;
	EulerAngles							m_localRotationOffset;

protected:
	TInstanceVar< EngineQsTransform >	i_objectTransform;
	TInstanceVar< Vector >				i_localPositionOffset;
	TInstanceVar< EulerAngles >			i_localRotationOffset;

public:
	IBehaviorConstraintObject();

	virtual EngineQsTransform GetTransform( CBehaviorGraphInstance& instance ) const;
	virtual EngineQsTransform RefreshTransform( CBehaviorGraphInstance& instance ) const;
	virtual void SetTransform( CBehaviorGraphInstance& instance, const AnimQsTransform& transform ) const;
	virtual void SetTransformMatrix( CBehaviorGraphInstance& instance, const Matrix& transform ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &/*context*/, CBehaviorGraphInstance& /*instance*/, Float /*timeDelta*/ ) const {}
	virtual void Sample( SBehaviorGraphOutput &/*output*/, CBehaviorGraphInstance& /*instance*/ ) const {}
	virtual void OnActivated( CBehaviorGraphInstance& /*instance*/ ) const {}
	virtual void OnDeactivated( CBehaviorGraphInstance& /*instance*/ ) const {}
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& /*instance*/, Float /*alpha*/ ) const {}
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateSocketForOwner( CBehaviorGraphConstraintNode* /*owner*/ ) {}
#endif
	virtual void CacheConnections( CBehaviorGraphConstraintNode* /*owner*/ ) {}

protected:
	void GetLocalOffsets( CBehaviorGraphInstance& instance, AnimQsTransform& lacalOffset ) const;
	CBehaviorGraphConstraintNode* GetOwner();
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorConstraintObject );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_localPositionOffset, String::EMPTY );
	PROPERTY_EDIT( m_localRotationOffset, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintBoneObject : public IBehaviorConstraintObject
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_ENGINE_CLASS( CBehaviorConstraintBoneObject, IBehaviorConstraintObject, 0 );

protected:
	String				m_boneName;

protected:
	TInstanceVar< Int32 >	i_boneIndex;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorGraphOutput &output, CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintBoneObject )
	PARENT_CLASS( IBehaviorConstraintObject );
	PROPERTY_CUSTOM_EDIT_NAME( m_boneName, TXT("Bone"), String::EMPTY, TXT("BehaviorBoneSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintVectorObject : public IBehaviorConstraintObject
{
	DECLARE_ENGINE_CLASS( CBehaviorConstraintVectorObject, IBehaviorConstraintObject, 0 );

protected:
	TInstanceVar< Vector >			i_positionValue;
	TInstanceVar< Vector >			i_rotationValue;

protected:
	CBehaviorGraphVectorValueNode*	m_cachedTargetPositionNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetRotationNode;

public:
	CBehaviorConstraintVectorObject();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	EngineQsTransform RefreshTransform( CBehaviorGraphInstance& instance ) const;
	
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CreateSocketForOwner( CBehaviorGraphConstraintNode* owner );
#endif
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorGraphOutput &output, CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void CacheConnections( CBehaviorGraphConstraintNode* owner );
};

BEGIN_CLASS_RTTI( CBehaviorConstraintVectorObject )
	PARENT_CLASS( IBehaviorConstraintObject );
	PROPERTY( m_cachedTargetPositionNode );
	PROPERTY( m_cachedTargetRotationNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintComponentObject : public CBehaviorConstraintBoneObject
{
	DECLARE_ENGINE_CLASS( CBehaviorConstraintComponentObject, CBehaviorConstraintBoneObject, 0 );

protected:
	String m_componentName;

public:
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintComponentObject )
	PARENT_CLASS( CBehaviorConstraintBoneObject );
	PROPERTY_EDIT( m_componentName, String::EMPTY );
END_CLASS_RTTI();
