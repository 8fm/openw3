/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"
#include "../core/engineQsTransform.h"

struct SBehaviorScriptContext
{
	SBehaviorScriptContext() : m_visualDebug( NULL ) {}

	TEngineQsTransformArray	m_poseLS;
	TEngineQsTransformArray	m_poseMS;
	TDynArray< Float >				m_floatTracks;

	TDynArray< Float >				m_inputParamsF;
	TDynArray< Vector >				m_inputParamsV;

	TDynArray< Float >				m_localParamsF;
	TDynArray< Vector >				m_localParamsV;
	TDynArray< Matrix >				m_localParamsM;
	TEngineQsTransformArray			m_localParamsT;

	Float							m_timeDelta;

	CVisualDebug*					m_visualDebug;

	DECLARE_RTTI_STRUCT( SBehaviorScriptContext );
};

BEGIN_CLASS_RTTI( SBehaviorScriptContext );
	PROPERTY( m_poseLS );
	PROPERTY( m_poseMS );
	PROPERTY( m_floatTracks );
	PROPERTY( m_inputParamsF );
	PROPERTY( m_inputParamsV );
	PROPERTY( m_localParamsF );
	PROPERTY( m_localParamsV );
	PROPERTY( m_localParamsM );
	PROPERTY( m_localParamsT );
	PROPERTY( m_timeDelta );
	PROPERTY( m_visualDebug );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IBehaviorScript : public CObject
{
	DECLARE_ENGINE_CLASS( IBehaviorScript, CObject, 0 );

protected:
	Uint32 m_inputFloatNum;
	Uint32 m_inputVectorNum;

public:
	void Run( SBehaviorScriptContext& context );

	Uint32 GetNumberOfInputsFloat() const;
	Uint32 GetNumberOfInputsVector() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void OnPropertyPostChange( IProperty* property );
#endif

protected:
	void DrawLine( SBehaviorScriptContext& scriptContext, const Vector& a, const Vector& b, Color color );
	void DrawAxis( SBehaviorScriptContext& scriptContext, Float scale, const Matrix& mat );
	void DrawAxis( SBehaviorScriptContext& scriptContext, Float scale, const EngineQsTransform& trans );
	void DrawSphere( SBehaviorScriptContext& scriptContext, const Vector& center, Float radius, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const Vector& center, Float radius, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const Vector& center, Float xLength, Float yLength, Float zLength, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const Matrix& mat, Float radius, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const Matrix& mat, Float xLength, Float yLength, Float zLength, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const EngineQsTransform& trans, Float radius, Color color );
	void DrawBox( SBehaviorScriptContext& scriptContext, const EngineQsTransform& trans, Float xLength, Float yLength, Float zLength, Color color );
	void DrawBone2D( SBehaviorScriptContext& scriptContext, const Vector& a, const Vector& b, Color color );
	void DrawBone3D( SBehaviorScriptContext& scriptContext, const Vector& parent, const EngineQsTransform& child, Color color );
	void DrawText2D( SBehaviorScriptContext& scriptContext, const String& text, Color color );
	void DrawText3D( SBehaviorScriptContext& scriptContext, const String& text, const Vector& position, Color color, Bool useBg );

protected:
	void funcDrawLine( CScriptStackFrame& stack, void* result );
	void funcDrawAxisMat( CScriptStackFrame& stack, void* result );
	void funcDrawAxis( CScriptStackFrame& stack, void* result );
	void funcDrawSphere( CScriptStackFrame& stack, void* result );
	void funcDrawBoxRadius( CScriptStackFrame& stack, void* result );
	void funcDrawBox( CScriptStackFrame& stack, void* result );
	void funcDrawBoxRadiusMatOrient( CScriptStackFrame& stack, void* result );
	void funcDrawBoxMatOrinet( CScriptStackFrame& stack, void* result );
	void funcDrawBone2D( CScriptStackFrame& stack, void* result );
	void funcDrawBone3D( CScriptStackFrame& stack, void* result );
	void funcDrawText2D( CScriptStackFrame& stack, void* result );
	void funcDrawText3D( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IBehaviorScript );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_inputFloatNum, TXT("Number of float sockets") );
	PROPERTY_EDIT( m_inputVectorNum, TXT("Number of vector sockets") );
	NATIVE_FUNCTION( "DrawSphere", funcDrawSphere );
	NATIVE_FUNCTION( "DrawLine", funcDrawLine );
	NATIVE_FUNCTION( "DrawBox", funcDrawBox );
	NATIVE_FUNCTION( "DrawBoxRadius", funcDrawBoxRadius );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphScriptNode	: public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphScriptNode, CBehaviorGraphBaseNode, "Misc", "Script" );

protected:
	IBehaviorScript*								m_script;

	TDynArray< CBehaviorGraphValueNode* >			m_cachedFloatNodes;
	TDynArray< CBehaviorGraphVectorValueNode* >		m_cachedVectorNodes;

protected:
	TInstanceVar< IBehaviorScript* >				i_script;
	TInstanceVar< Float >							i_timeDelta;
	TInstanceVar< TDynArray< Float > >				i_inputParamsF;
	TInstanceVar< TDynArray< Vector > >				i_inputParamsV;
	TInstanceVar< TDynArray< Float > >				i_localParamsF;
	TInstanceVar< TDynArray< Vector > >				i_localParamsV;
	TInstanceVar< TDynArray< Matrix > >				i_localParamsM;
	TInstanceVar< TEngineQsTransformArray >			i_localParamsT;
	TInstanceVar< CVisualDebug* >					i_visualDebug;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 155, 0, 0 ); }
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty* property );
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void CacheConnections();

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

private:
	void CopyDataToContext( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, SBehaviorScriptContext& context ) const;
	void CopyDataFromContext( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorScriptContext& context ) const;

protected:
	void SetupVisualDebug( CBehaviorGraphInstance& instance ) const;
	void FindScript( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const;
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphScriptNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_INLINED( m_script, TXT("Behavior script") );
	PROPERTY( m_cachedFloatNodes );
	PROPERTY( m_cachedVectorNodes );
END_CLASS_RTTI();
