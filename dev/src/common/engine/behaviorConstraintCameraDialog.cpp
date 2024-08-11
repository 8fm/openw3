/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorConstraintCameraDialog.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphValueNode.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/animatedComponent.h"
#include "../engine/renderFrame.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintCameraDialog );

CBehaviorGraphConstraintCameraDialog::CBehaviorGraphConstraintCameraDialog()
{

}

void CBehaviorGraphConstraintCameraDialog::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_controlValue;
	compiler << i_boneTransform;
	compiler << i_sourceMS;
	compiler << i_destMS;
}

void CBehaviorGraphConstraintCameraDialog::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_cameraBone, instance );
	instance[ i_controlValue ] = true;
	instance[ i_boneTransform ] = Matrix::IDENTITY;
	instance[ i_sourceMS ] = Vector::ZERO_3D_POINT;
	instance[ i_destMS ] = Vector::ZERO_3D_POINT;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintCameraDialog::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( SourceTarget ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( DestTarget ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphConstraintCameraDialog::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );

	m_cachedSourceTargetValueNode = CacheVectorValueBlock( TXT("SourceTarget") );
	m_cachedDestTargetValueNode = CacheVectorValueBlock( TXT("DestTarget") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintCameraDialog::GetCaption() const
{
	return String( TXT("Camera Dialog") );
}

#endif

void CBehaviorGraphConstraintCameraDialog::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedSourceTargetValueNode )
	{
		m_cachedSourceTargetValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedDestTargetValueNode )
	{
		m_cachedDestTargetValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphConstraintCameraDialog::OnReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_controlValue ] = false;
}

void CBehaviorGraphConstraintCameraDialog::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}

	if ( m_cachedSourceTargetValueNode )
	{
		m_cachedSourceTargetValueNode->Activate( instance );
	}

	if ( m_cachedDestTargetValueNode )
	{
		m_cachedDestTargetValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintCameraDialog::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}

	if ( m_cachedSourceTargetValueNode )
	{
		m_cachedSourceTargetValueNode->Deactivate( instance );
	}

	if ( m_cachedDestTargetValueNode )
	{
		m_cachedDestTargetValueNode->Deactivate( instance );
	}
}

Bool CBehaviorGraphConstraintCameraDialog::IsRunning( CBehaviorGraphInstance& instance ) const
{
	return m_cachedSourceTargetValueNode && m_cachedDestTargetValueNode && instance[ i_controlValue ] && instance[ i_boneIndex ] != -1;
}

void CBehaviorGraphConstraintCameraDialog::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update control value
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		instance[ i_controlValue ] = ( m_cachedControlValueNode->GetValue( instance ) > 0.5 ) ? true : false;
	}
	else
	{
		instance[ i_controlValue ] = true;
	}

	if ( IsRunning( instance ) )
	{
		m_cachedSourceTargetValueNode->Update( context, instance, timeDelta );
		m_cachedDestTargetValueNode->Update( context, instance, timeDelta );

		instance[ i_sourceMS ] = m_cachedSourceTargetValueNode->GetVectorValue( instance );
		instance[ i_destMS ] = m_cachedDestTargetValueNode->GetVectorValue( instance );
	}
}

Matrix CBehaviorGraphConstraintCameraDialog::BuildSpecialMatrixFromXDirVector( const Vector& xVec ) const
{
	Matrix mat;

	// EX from direction, EY pointing up
	mat.V[0] = xVec.Normalized3();
	mat.V[1] = Vector::EZ;

	// EZ as cross product of EX and EY
	mat.V[2] = Vector::Cross( mat.V[0], mat.V[1] ).Normalized3();

	mat.V[0].W = 0.0f;
	mat.V[1].W = 0.0f;
	mat.V[2].W = 0.0f;
	mat.V[3] = Vector::EW;

	return mat;
}

void CBehaviorGraphConstraintCameraDialog::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	if ( IsRunning( instance ) )
	{
		const Int32 boneIndex = instance[ i_boneIndex ];
		Matrix& cameraBoneFinalMS = instance[ i_boneTransform ];
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform parentBoneTransformMS = output.GetParentBoneModelTransform( instance.GetAnimatedComponent(), boneIndex );

		hkQsTransform parentBoneTransformMSInv;
		parentBoneTransformMSInv.setInverse( parentBoneTransformMS );
		
		hkQsTransform boneTransformMS;
		boneTransformMS.setMul( parentBoneTransformMS, output.m_outputPose[ boneIndex ] );
#else
		RedQsTransform parentBoneTransformMS = output.GetParentBoneModelTransform( instance.GetAnimatedComponent(), boneIndex );

		RedQsTransform parentBoneTransformMSInv;
		parentBoneTransformMSInv.SetInverse( parentBoneTransformMS );
		
		RedQsTransform boneTransformMS;
		boneTransformMS.SetMul( parentBoneTransformMS, output.m_outputPose[ boneIndex ] );
#endif
		{
			// Get current input data
			const Vector destMS = instance[ i_destMS ];
			const Vector sourceMS = instance[ i_sourceMS ];
			if ( Vector::Equal3( sourceMS, destMS ) )
			{
				cameraBoneFinalMS.SetIdentity();
			}
			else
			{
				Vector refDestMS = destMS;			refDestMS.SetZ( m_referenceZ );
				Vector refSourceMS = sourceMS;		refSourceMS.SetZ( m_referenceZ );

				// Build reference matrix 
				Matrix matRef = BuildSpecialMatrixFromXDirVector( refDestMS - refSourceMS );
				matRef.SetTranslation( refSourceMS );

				// Invert it
				Matrix matRefInv = matRef.FullInverted();

				// Get camera bone transform in MS

#ifdef USE_HAVOK_ANIMATION
				Matrix cameraBoneMS;
				HavokTransformToMatrix_Renormalize( boneTransformMS, &cameraBoneMS );
#else
				RedMatrix4x4 conversionMatrix = boneTransformMS.ConvertToMatrixNormalized();
				Matrix cameraBoneMS( reinterpret_cast< const Matrix& >( conversionMatrix ) );
#endif
				
				// Bone transform in reference space
				Matrix cameraBoneRS = cameraBoneMS * matRefInv;

				// Build matrix from current input data
				Matrix matCurr = BuildSpecialMatrixFromXDirVector( destMS - sourceMS );
				matCurr.SetTranslation( sourceMS );

				// Final matrix
				Matrix finalMat = cameraBoneRS * matCurr;

				cameraBoneFinalMS.BuildFromDirectionVector( finalMat.V[1] );
				cameraBoneFinalMS.SetTranslation( finalMat.GetTranslation() );
			}
		}
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform transformMS;
		MatrixToHavokQsTransform( cameraBoneFinalMS, transformMS );

		output.m_outputPose[ boneIndex ].setMul( parentBoneTransformMSInv, transformMS );
#else
		Vector vecTrans = cameraBoneFinalMS.GetTranslation();
		// Gimbal lock is possible
		RedQuaternion quatTemp;
		EulerAngles anglesTemp = cameraBoneFinalMS.ToEulerAnglesFull();
		Vector quatVecTemp = anglesTemp.ToQuat();
		quatTemp.Quat = reinterpret_cast< const RedVector4& >( quatVecTemp );
		Vector vecScale = cameraBoneFinalMS.GetScale33();


		RedQsTransform transformMS(reinterpret_cast< const RedVector4& >( vecTrans ),
								   quatTemp,
								   reinterpret_cast< const RedVector4& >( vecScale ) );

		output.m_outputPose[ boneIndex ].SetMul( parentBoneTransformMSInv, transformMS );
#endif
	}
}

void CBehaviorGraphConstraintCameraDialog::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if( !m_generateEditorFragments || !IsRunning( instance ) ) return;

	const Vector destMS = instance[ i_destMS ];
	const Vector sourceMS = instance[ i_sourceMS ];

	const Matrix transformMat = instance[ i_boneTransform ];

	const Matrix& localToWorld = instance.GetAnimatedComponent()->GetLocalToWorld();

	frame->AddDebugSphere( localToWorld.TransformPoint( sourceMS ), 0.1f, Matrix::IDENTITY, Color::YELLOW, false );
	frame->AddDebugSphere( localToWorld.TransformPoint( destMS ), 0.1f, Matrix::IDENTITY, Color::LIGHT_CYAN, false );
}
