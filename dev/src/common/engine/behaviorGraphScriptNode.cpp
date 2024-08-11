/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphScriptNode.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/visualDebug.h"
#include "../core/scriptStackFrame.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( SBehaviorScriptContext );
IMPLEMENT_ENGINE_CLASS( IBehaviorScript );

void IBehaviorScript::Run( SBehaviorScriptContext& scriptContext )
{
	CallFunctionRef( this, CNAME( Run ), scriptContext );
}

Uint32 IBehaviorScript::GetNumberOfInputsFloat() const
{
	return m_inputFloatNum;
}

Uint32 IBehaviorScript::GetNumberOfInputsVector() const
{
	return m_inputVectorNum;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void IBehaviorScript::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("inputFloatNum") || property->GetName() == TXT("inputVectorNum") )
	{
		CBehaviorGraphScriptNode* node = SafeCast< CBehaviorGraphScriptNode >( GetParent() );
		if ( node )
		{
			node->OnRebuildSockets();
		}
	}
}

#endif

void IBehaviorScript::DrawLine( SBehaviorScriptContext& scriptContext, const Vector& a, const Vector& b, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddLine( CName::NONE, a, b, false, color );
	}
}

void IBehaviorScript::DrawAxis( SBehaviorScriptContext& scriptContext, Float scale, const Matrix& mat )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddAxis( CName::NONE, scale, mat.GetTranslation(), mat.ToEulerAngles() );
	}
}

void IBehaviorScript::DrawAxis( SBehaviorScriptContext& scriptContext, Float scale, const EngineQsTransform& trans )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawSphere( SBehaviorScriptContext& scriptContext, const Vector& center, Float radius, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddSphere( CName::NONE, radius, center, false, color, 0.f );
	}
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const Vector& center, Float radius, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddBox( CName::NONE, Vector( radius, radius, radius ), center, EulerAngles::ZEROS, false, color );
	}
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const Vector& center, Float xLength, Float yLength, Float zLength, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddBox( CName::NONE, Vector( xLength, yLength, zLength ), center, EulerAngles::ZEROS, false, color );
	}
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const Matrix& mat, Float radius, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddBox( CName::NONE, Vector( radius, radius, radius ), mat.GetTranslation(), mat.ToEulerAngles(), false, color );
	}
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const Matrix& mat, Float xLength, Float yLength, Float zLength, Color color )
{
	if ( scriptContext.m_visualDebug )
	{
		scriptContext.m_visualDebug->AddBox( CName::NONE, Vector( xLength, yLength, zLength ), mat.GetTranslation(), mat.ToEulerAngles(), false, color );
	}
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const EngineQsTransform& trans, Float radius, Color color )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawBox( SBehaviorScriptContext& scriptContext, const EngineQsTransform& trans, Float xLength, Float yLength, Float zLength, Color color )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawBone2D( SBehaviorScriptContext& scriptContext, const Vector& a, const Vector& b, Color color )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawBone3D( SBehaviorScriptContext& scriptContext, const Vector& parent, const EngineQsTransform& child, Color color )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawText2D( SBehaviorScriptContext& scriptContext, const String& text, Color color )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::DrawText3D( SBehaviorScriptContext& scriptContext, const String& text, const Vector& position, Color color, Bool useBg )
{
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::funcDrawLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawLine( scriptContext, a, b, color );
}

void IBehaviorScript::funcDrawAxisMat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	GET_PARAMETER_OPT( Float, scale, 0.1f );
	FINISH_PARAMETERS;

	DrawAxis( scriptContext, scale, mat );
}

void IBehaviorScript::funcDrawAxis( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform( EngineQsTransform::IDENTITY ) );
	GET_PARAMETER_OPT( Float, scale, 0.1f );
	FINISH_PARAMETERS;

	DrawAxis( scriptContext, scale, trans );
}

void IBehaviorScript::funcDrawSphere( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Vector, pos, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawSphere( scriptContext, pos, radius, color );
}

void IBehaviorScript::funcDrawBoxRadius( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawBox( scriptContext, pos, radius, color );
}

void IBehaviorScript::funcDrawBox( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, x, 0.1f );
	GET_PARAMETER( Float, y, 0.1f );
	GET_PARAMETER( Float, z, 0.1f );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawBox( scriptContext, pos, x, y, z, color );
}

void IBehaviorScript::funcDrawBoxRadiusMatOrient( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawBox( scriptContext, mat, radius, color );
}

void IBehaviorScript::funcDrawBoxMatOrinet( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SBehaviorScriptContext, scriptContext, SBehaviorScriptContext() );
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	GET_PARAMETER( Float, radius, 0.1f );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	DrawBox( scriptContext, mat, radius, color );
}

void IBehaviorScript::funcDrawBone2D( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::funcDrawBone3D( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::funcDrawText2D( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ASSERT( !TXT("Not implemented") );
}

void IBehaviorScript::funcDrawText3D( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ASSERT( !TXT("Not implemented") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphScriptNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphScriptNode::GetCaption() const 
{ 
	return TXT("Script");
}

void CBehaviorGraphScriptNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("script") )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphScriptNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	if ( m_script )
	{
		Uint32 fNum = m_script->GetNumberOfInputsFloat();
		Uint32 vNum = m_script->GetNumberOfInputsVector();

		for ( Uint32 i=0; i<fNum; ++i )
		{
			CName socketName = CName( String::Printf( TXT("Float_%d"), i ) );
			CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( socketName, true ) );
		}

		for ( Uint32 i=0; i<vNum; ++i )
		{
			CName socketName = CName( String::Printf( TXT("Vector_%d"), i ) );
			CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( socketName, true ) );
		}
	}
}

#endif

void CBehaviorGraphScriptNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	if ( m_script )
	{
		Uint32 fNum = m_script->GetNumberOfInputsFloat();
		Uint32 vNum = m_script->GetNumberOfInputsVector();

		m_cachedFloatNodes.Resize( fNum );
		m_cachedVectorNodes.Resize( vNum );

		for ( Uint32 i=0; i<fNum; ++i )
		{
			m_cachedFloatNodes[ i ] = CacheValueBlock( String::Printf( TXT("Float_%d"), i ) );

		}

		for ( Uint32 i=0; i<vNum; ++i )
		{
			m_cachedVectorNodes[ i ] = CacheVectorValueBlock( String::Printf( TXT("Vector_%d"), i ) );
		}
	}
	else
	{
		m_cachedFloatNodes.Clear();
		m_cachedVectorNodes.Clear();
	}
}

void CBehaviorGraphScriptNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_script;
	compiler << i_visualDebug;
	compiler << i_timeDelta;
	compiler << i_inputParamsF;
	compiler << i_inputParamsV;
	compiler << i_localParamsF;
	compiler << i_localParamsV;
	compiler << i_localParamsM;
	compiler << i_localParamsT;
}

void CBehaviorGraphScriptNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	SetupVisualDebug( instance );
	FindScript( instance );
 	CVisualDebug* vd = ::CreateObject< CVisualDebug >( &instance );
 	if ( vd )
 	{
 		instance[ i_visualDebug ] = vd;
		vd->AddToRootSet();
 	}
 	else
 	{
 		ASSERT( vd );
 		instance[ i_visualDebug ] = NULL;
 	}

	if ( m_script )
	{
		instance[ i_script ] = SafeCast< IBehaviorScript >( m_script->Clone( &instance ) );
	}

	IBehaviorScript* script = instance[ i_script ];

	if ( script )
	{
		// Tomsin TODO
		// Temp
		script->AddToRootSet();

		// Inputs
		TDynArray< Float >& inputsFloat = instance[ i_inputParamsF ];
		TDynArray< Vector >& inputsVec = instance[ i_inputParamsV ];

		inputsFloat.Resize( m_script->GetNumberOfInputsFloat() );
		inputsVec.Resize( m_script->GetNumberOfInputsVector() );

		for ( Uint32 i=0; i<inputsFloat.Size(); ++i )
		{
			inputsFloat[ i ] = 0.f;
		}

		for ( Uint32 i=0; i<inputsVec.Size(); ++i )
		{
			inputsVec[ i ] = Vector::ZEROS;
		}

		// Time
		instance[ i_timeDelta ] = 0.f;
	}
}

void CBehaviorGraphScriptNode::SetupVisualDebug( CBehaviorGraphInstance& instance ) const
{
	CVisualDebug* vd = ::CreateObject< CVisualDebug >( &instance );
	if ( vd )
	{
		instance[ i_visualDebug ] = vd;
		vd->AddToRootSet();
	}
	else
	{
		ASSERT( vd );
		instance[ i_visualDebug ] = NULL;
	}
}

void CBehaviorGraphScriptNode::FindScript( CBehaviorGraphInstance& instance ) const
{
	if ( m_script )
	{
		instance[ i_script ] = SafeCast< IBehaviorScript >( m_script->Clone( &instance ) );
	}
}

void CBehaviorGraphScriptNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	IBehaviorScript* script = instance[ i_script ];
	if ( script )
	{
		// Temp
		// Tomsin TODO
		script->RemoveFromRootSet();

		script->Discard();
	}
	instance[ i_script ] = NULL;

	CVisualDebug* vd = instance[ i_visualDebug ];
	if ( vd )
	{
		vd->RemoveFromRootSet();
		vd->Discard();
	}
	instance[ i_visualDebug ] = NULL;
}

void CBehaviorGraphScriptNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	IBehaviorScript* script = instance[ i_script ];
	if ( script )
	{
		// Build context
		SBehaviorScriptContext scriptContext;

		CopyDataToContext( instance, output, scriptContext );

		script->Run( scriptContext );

		CopyDataFromContext( instance, output, scriptContext );
	}
}

void CBehaviorGraphScriptNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	IBehaviorScript* script = instance[ i_script ];
	if ( script )
	{
		// Cache time delta
		instance[ i_timeDelta ] = timeDelta;

		TDynArray< Float >& inputsFloat = instance[ i_inputParamsF ];
		TDynArray< Vector >& inputsVec = instance[ i_inputParamsV ];

		ASSERT( m_cachedFloatNodes.Size() == inputsFloat.Size() );
		ASSERT( m_cachedVectorNodes.Size() == inputsVec.Size() );

		// Cache inputs
		for ( Uint32 i=0; i<m_cachedFloatNodes.Size(); ++i )
		{
			CBehaviorGraphValueNode* node = m_cachedFloatNodes[ i ];
			if ( node )
			{
				node->OnUpdate( context, instance, timeDelta );
				inputsFloat[ i ] = node->GetValue( instance );
			}
		}

		for ( Uint32 i=0; i<m_cachedVectorNodes.Size(); ++i )
		{
			CBehaviorGraphVectorValueNode* node = m_cachedVectorNodes[ i ];
			if ( node )
			{
				node->OnUpdate( context, instance, timeDelta );
				inputsVec[ i ] = node->GetVectorValue( instance );
			}
		}
	}
}

void CBehaviorGraphScriptNode::CopyDataToContext( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, SBehaviorScriptContext& context ) const
{
	context.m_visualDebug = instance[ i_visualDebug ];
	context.m_timeDelta = instance[ i_timeDelta ];

	context.m_inputParamsF = instance[ i_inputParamsF ];
	context.m_inputParamsV = instance[ i_inputParamsV ];

	context.m_localParamsF = instance[ i_localParamsF ];
	context.m_localParamsV = instance[ i_localParamsV ];
	context.m_localParamsM = instance[ i_localParamsM ];
	context.m_localParamsT = instance[ i_localParamsT ];

	// Pose LS
	context.m_poseLS.Resize( output.m_numBones );
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		//reinterpret_cast< const EngineQsTransform& >( in );
#ifdef USE_HAVOK_ANIMATION
		HkToEngineQsTransform( output.m_outputPose[ i ], context.m_poseLS[ i ] );
#else
		context.m_poseLS[i] = reinterpret_cast< const EngineQsTransform& >( output.m_outputPose[ i ] );
#endif
	}

	// Pose MS
	if ( instance.GetAnimatedComponent()->GetSkeleton() )
	{
#ifdef USE_HAVOK_ANIMATION
		TDynArray< hkQsTransform > bonesHkMS( output.m_numBones );
#else
		TDynArray< RedQsTransform > bonesHkMS( output.m_numBones );
#endif
		context.m_poseMS.Resize( output.m_numBones );

		output.GetBonesModelSpace( instance.GetAnimatedComponent()->GetSkeleton(), bonesHkMS );

		for ( Uint32 i=0; i<bonesHkMS.Size(); ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			HkToEngineQsTransform( bonesHkMS[ i ], context.m_poseMS[ i ] );
#else
			context.m_poseMS[ i ] = reinterpret_cast< const EngineQsTransform& >(  bonesHkMS[ i ] );
#endif
		}
	}

	// Float tracks
	context.m_floatTracks.Resize( output.m_numFloatTracks );
	for ( Uint32 i=0; i<output.m_numFloatTracks; ++i )
	{
		context.m_floatTracks[ i ] = output.m_floatTracks[ i ];
	}
}

void CBehaviorGraphScriptNode::CopyDataFromContext( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorScriptContext& context ) const
{
	instance[ i_localParamsF ] = context.m_localParamsF;
	instance[ i_localParamsV ] = context.m_localParamsV;
	instance[ i_localParamsM ] = context.m_localParamsM;
	instance[ i_localParamsT ] = context.m_localParamsT;

	// Pose LS
	{
		ASSERT( context.m_poseLS.Size() == output.m_numBones );
		Uint32 size = Min( context.m_poseLS.Size(), output.m_numBones );

		for ( Uint32 i=0; i<size; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			EngineToHkQsTransform( context.m_poseLS[ i ], output.m_outputPose[ i ] );
#else
			output.m_outputPose[ i ] = reinterpret_cast< const RedQsTransform& >( context.m_poseLS[ i ] );
#endif
		}
	}

	// Float tracks
	{
		ASSERT( context.m_floatTracks.Size() == output.m_numFloatTracks );
		Uint32 size = Min( context.m_floatTracks.Size(), output.m_numFloatTracks );

		for ( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[ i ] = context.m_floatTracks[ i ];
		}
	}
}

void CBehaviorGraphScriptNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	for ( Uint32 i=0; i<m_cachedFloatNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* node = m_cachedFloatNodes[ i ];
		if ( node )
		{
			node->OnActivated( instance );
		}
	}

	for ( Uint32 i=0; i<m_cachedVectorNodes.Size(); ++i )
	{
		CBehaviorGraphVectorValueNode* node = m_cachedVectorNodes[ i ];
		if ( node )
		{
			node->OnActivated( instance );
		}
	}
}

void CBehaviorGraphScriptNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	for ( Uint32 i=0; i<m_cachedFloatNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* node = m_cachedFloatNodes[ i ];
		if ( node )
		{
			node->OnDeactivated( instance );
		}
	}

	for ( Uint32 i=0; i<m_cachedVectorNodes.Size(); ++i )
	{
		CBehaviorGraphVectorValueNode* node = m_cachedVectorNodes[ i ];
		if ( node )
		{
			node->OnDeactivated( instance );
		}
	}
}

void CBehaviorGraphScriptNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	for ( Uint32 i=0; i<m_cachedFloatNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* node = m_cachedFloatNodes[ i ];
		if ( node )
		{
			node->ProcessActivationAlpha( instance, alpha );
		}
	}

	for ( Uint32 i=0; i<m_cachedVectorNodes.Size(); ++i )
	{
		CBehaviorGraphVectorValueNode* node = m_cachedVectorNodes[ i ];
		if ( node )
		{
			node->ProcessActivationAlpha( instance, alpha );
		}
	}
}

void CBehaviorGraphScriptNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	CVisualDebug* vd = instance[ i_visualDebug ];
	if ( vd )
	{
		vd->Render( frame, instance.GetAnimatedComponent()->GetLocalToWorld() );
		vd->RemoveAll();
	}
}

void CBehaviorGraphScriptNode::OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const
{
	TBaseClass::OnLoadingSnapshot( instance, snapshotData );
}

void CBehaviorGraphScriptNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	// visual debug was not destroyed, keep it
	instance[ i_visualDebug ] = previousData[ i_visualDebug ];
	FindScript( instance );
}
