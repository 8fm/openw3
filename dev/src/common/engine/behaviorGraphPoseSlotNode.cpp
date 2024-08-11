
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphPoseSlotNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseSlotNode );

CBehaviorGraphPoseSlotNode::CBehaviorGraphPoseSlotNode()
	: m_worldSpace( false )
	, m_interpolation( IT_Bezier )
	, m_blendFloatTracks( true )
	, m_ignoreZeroFloatTracks( true )
{

}

void CBehaviorGraphPoseSlotNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("name") )
	{
		m_slotName = CName( GetName() );
	}
}

void CBehaviorGraphPoseSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_running;
	compiler << i_blendTime;
	compiler << i_blendTimer;
	compiler << i_blendType;
	compiler << i_firstBoneIndex;
	compiler << i_firstBoneWS;
	compiler << i_listener;
	compiler << i_pose;
}

void CBehaviorGraphPoseSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_running ] = false;
	instance[ i_blendTime ] = 0.f;
	instance[ i_blendTimer ] = 0.f;
	instance[ i_blendType ] = -1;
	instance[ i_firstBoneIndex ] = m_firstBone.Empty() ? 0 : FindBoneIndex( m_firstBone, instance );
	instance[ i_firstBoneWS ] = Matrix::IDENTITY;
	instance[ i_listener ] = 0;
}

void CBehaviorGraphPoseSlotNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_running );
	INST_PROP( i_blendTime );
	INST_PROP( i_blendTimer );
	INST_PROP( i_blendType );
	INST_PROP( i_firstBoneIndex );
}

void CBehaviorGraphPoseSlotNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

String CBehaviorGraphPoseSlotNode::GetCaption() const
{
	return String::Printf( TXT("Pose [ %s ]"), GetName().AsChar() );
}

void CBehaviorGraphPoseSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphPoseSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_running ] )
	{
		FinishBlending( instance );
	}

	DestroyPose( instance );

	instance[ i_blendType ] = -1;

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphPoseSlotNode::SetPose(	CBehaviorGraphInstance& instance, 
											const CAnimatedComponent* componentWithPose, 
											Float blendTime, 
											EBlendType type,
											IPoseSlotListener* l ) const
{
	if ( !instance[ i_running ] )
	{
		CreatePose( instance );
	}
	else
	{
		FinishBlending( instance );
	}

	CachePose( instance, componentWithPose );

	const Int32 bone = instance[ i_firstBoneIndex ];
	if ( m_worldSpace && bone != -1 )
	{
		instance[ i_firstBoneWS ] = componentWithPose->GetBoneMatrixWorldSpace( instance[ i_firstBoneIndex ] );
	}

	instance[ i_blendTime ] = blendTime;
	instance[ i_blendTimer ] = 0.f;
	instance[ i_running ] = true;
	instance[ i_blendType ] = type;

	SetListener( instance, l );
}

void CBehaviorGraphPoseSlotNode::SetPose(	CBehaviorGraphInstance& instance, 
#ifdef USE_HAVOK_ANIMATION
											const TDynArray< hkQsTransform >&poseLS, 
#else
											const TDynArray< RedQsTransform >&poseLS, 
#endif
											const TDynArray< Float >& floatTracks,
											const Matrix& localToWorld, 
											Float blendTime, 
											EBlendType type,
											IPoseSlotListener* l ) const
{
	if ( !instance[ i_running ] )
	{
		CreatePose( instance );
	}
	else
	{
		FinishBlending( instance );
	}

	SBehaviorSampleContext* context = instance.GetAnimatedComponent()->GetBehaviorGraphSampleContext();
	if ( !context )
	{
		ASSERT( context );
		return;
	}

	CachePose( instance, poseLS, floatTracks );

	const Int32 bone = instance[ i_firstBoneIndex ];
	if ( m_worldSpace && bone != -1 )
	{
		// Bone model space

		SBehaviorGraphOutput* pose = instance[ i_pose ].GetPose();
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform boneMS = pose->GetBoneModelTransform( instance.GetAnimatedComponent(), bone );

		Matrix boneMatMS;
		HavokTransformToMatrix_Renormalize( boneMS, &boneMatMS );
#else
		RedQsTransform boneMS = pose->GetBoneModelTransform( instance.GetAnimatedComponent(), bone );

		Matrix boneMatMS;
		RedMatrix4x4 conversionMatrix;
		conversionMatrix = boneMS.ConvertToMatrixNormalized();
		boneMatMS = reinterpret_cast< const Matrix& >( conversionMatrix );
		//HavokTransformToMatrix_Renormalize( boneMS, &boneMatMS );
#endif
		// Bone world space
		instance[ i_firstBoneWS ] = boneMatMS * localToWorld;
	}

	instance[ i_blendTime ] = blendTime;
	instance[ i_blendTimer ] = 0.f;
	instance[ i_running ] = true;
	instance[ i_blendType ] = type;

	SetListener( instance, l );
}

void CBehaviorGraphPoseSlotNode::ResetPose( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_running ] )
	{
		FinishBlending( instance );
	}

	instance[ i_blendTimer ] = 0.f;
	instance[ i_blendType ] = -1;
	instance[ i_firstBoneWS ] = Matrix::IDENTITY;

	DestroyPose( instance );
}

Bool CBehaviorGraphPoseSlotNode::IsSlotActive( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_running ];
}

void CBehaviorGraphPoseSlotNode::CachePose( CBehaviorGraphInstance& instance, const CAnimatedComponent* componentWithPoseLS ) const
{
	instance[ i_pose ].Cache( componentWithPoseLS );
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphPoseSlotNode::CachePose( CBehaviorGraphInstance& instance, const TDynArray< hkQsTransform >&poseLS, const TDynArray< Float >& floatTracks ) const
#else
void CBehaviorGraphPoseSlotNode::CachePose( CBehaviorGraphInstance& instance, const TDynArray< RedQsTransform >&poseLS, const TDynArray< Float >& floatTracks ) const
#endif
{
	SBehaviorGraphOutput* pose = instance[ i_pose ].GetPose();
	if ( pose )
	{
		Uint32 boneNum = Min( pose->m_numBones, poseLS.Size() );

		for ( Uint32 i=0; i<boneNum; ++i )
		{
			pose->m_outputPose[ i ] = poseLS[ i ];
		}

		Uint32 numberOfTracks = Min( pose->m_numFloatTracks, floatTracks.Size() );
		for ( Uint32 j = 0; j < numberOfTracks; ++j )
		{
			pose->m_floatTracks[ j ] = floatTracks[ j ];
		}
	}
}

void CBehaviorGraphPoseSlotNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Create( instance );
}

void CBehaviorGraphPoseSlotNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Free( instance );
}

void CBehaviorGraphPoseSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( PoseSlot );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( instance[ i_running ] )
	{
		instance[ i_blendTimer ] = Clamp( instance[ i_blendTimer ] + timeDelta, 0.f, instance[ i_blendTime ] );
	}
}

Float CBehaviorGraphPoseSlotNode::GetWeight( CBehaviorGraphInstance& instance ) const
{
	const Int32 blendType = instance[ i_blendType ];

	Float value = 0.f;

	if ( blendType == BT_BlendOut )
	{
		value = 1.f - instance[ i_blendTimer ] / instance[ i_blendTime ];
	}
	else if ( blendType == BT_BlendIn )
	{
		value = instance[ i_blendTimer ] / instance[ i_blendTime ];
	}
	else
	{
		ASSERT( 0 );
	}

	return m_interpolation == IT_Bezier ? BehaviorUtils::BezierInterpolation( value ) : value;
}

void CBehaviorGraphPoseSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( instance[ i_running ] )
	{
		SBehaviorGraphOutput* pose = instance[ i_pose ].GetPose();
		const Int32 boneIndex = instance[ i_firstBoneIndex ];

		if ( !pose )
		{
			ASSERT( pose );
			return;
		}

		Int32 boneNum = Min( output.m_numBones, pose->m_numBones );

		// Bones
		if ( pose && boneIndex != -1 && boneIndex < boneNum )
		{
			const Float weight = GetWeight( instance );
			
			ASSERT( weight >= 0.f && weight <= 1.f );

			if ( m_worldSpace )
			{
				Matrix boneBMatWS = instance[ i_firstBoneWS ];
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform boneBWS;
				MatrixToHavokQsTransform( boneBMatWS, boneBWS );
#else
				RedQuaternion rotation;
				Vector vecStore = boneBMatWS.ToQuat();
				rotation.Quat = reinterpret_cast< const RedVector4& >( vecStore );
				Vector vecScale  = boneBMatWS.GetScale33();
				Vector vecTrans = boneBMatWS.GetTranslation();
				RedQsTransform boneBWS( reinterpret_cast< const RedVector4& >( vecTrans ), 
									    rotation,
										reinterpret_cast< const RedVector4& >( vecScale ) ) ;

#endif
				BlendBoneInWS( instance, output, boneIndex, boneBWS, weight );
			}
			else
			{
				BlendBoneInLS( output.m_outputPose[ boneIndex ], pose->m_outputPose[ boneIndex ], weight );
			}

			for ( Int32 i=boneIndex+1; i<boneNum; ++i )
			{
				BlendBoneInLS( output.m_outputPose[ i ], pose->m_outputPose[ i ], weight );
			}

			// Tracks
			if ( m_blendFloatTracks )
			{
				Int32 trackNum = Min( output.m_numFloatTracks, pose->m_numFloatTracks );

				if ( m_ignoreZeroFloatTracks )
				{
					for ( Int32 i=0; i<trackNum; ++i )
					{
						if ( pose->m_floatTracks[ i ] > 0.01f )
						{
							if ( output.m_floatTracks[ i ] > 0.01f )
							{
								output.m_floatTracks[ i ] = Lerp( weight, output.m_floatTracks[ i ], pose->m_floatTracks[ i ] );
							}
							else
							{
								output.m_floatTracks[ i ] = pose->m_floatTracks[ i ];
							}
						}
					}
				}
				else
				{
					for ( Int32 i=0; i<trackNum; ++i )
					{
						output.m_floatTracks[ i ] = Lerp( weight, output.m_floatTracks[ i ], pose->m_floatTracks[ i ] );
					}
				}
			}
		}

		if ( instance[ i_blendTimer ] >= instance[ i_blendTime ] && instance[ i_blendType ] != BT_BlendIn )
		{
			FinishBlending( instance );
			DestroyPose( instance );
		}
	}
}

void CBehaviorGraphPoseSlotNode::FinishBlending( CBehaviorGraphInstance& instance ) const
{
	instance[ i_running ] = false;

	IPoseSlotListener* l = GetListener( instance );
	if ( l )
	{
		// Reset
		instance[ i_listener ] = 0;

		l->OnPoseSlotEnd( this, instance );
	}
}

void CBehaviorGraphPoseSlotNode::BlendBoneInWS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &pose, Int32 bone, const RedQsTransform& boneBWS, Float weight ) const
{
	// Get bone A in WS
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CSkeleton* skeleton = ac->GetSkeleton();

	if ( !skeleton )
	{
		return;
	}

	RedQsTransform parentTransWS = pose.GetBoneWorldTransform( ac, skeleton->GetParentIndices()[ bone ] );

	RedQsTransform parentTransWSInv;
	parentTransWSInv.SetInverse( parentTransWS );

	RedQsTransform boneAWS;
	boneAWS.SetMul( parentTransWS, pose.m_outputPose[ bone ] );

	// Blend in WS
	boneAWS.Lerp( boneAWS, boneBWS, weight );

	// Back to LS
	RedQsTransform boneALS;
	boneALS.SetMul( parentTransWSInv, boneAWS );

	// Back to LS
	pose.m_outputPose[ bone ].SetMul( parentTransWSInv, boneAWS );

}

IPoseSlotListener* CBehaviorGraphPoseSlotNode::GetListener( CBehaviorGraphInstance& instance ) const
{
	return reinterpret_cast< IPoseSlotListener* >( instance[ i_listener ] );
}

void CBehaviorGraphPoseSlotNode::SetListener( CBehaviorGraphInstance& instance, IPoseSlotListener* l ) const
{
	ASSERT( GetListener( instance ) == NULL );
	instance[ i_listener ] = reinterpret_cast< TGenericPtr >( l );
}

const CName& CBehaviorGraphPoseSlotNode::GetSlotName() const
{
	return m_slotName;
}

void CBehaviorGraphPoseSlotNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	// zero listener, as it may no longer exist, or could exist at different address
	// NOTE - this should be used only for debug, so it shouldn't break any gameplay
	instance[ i_listener ] = 0;
}
