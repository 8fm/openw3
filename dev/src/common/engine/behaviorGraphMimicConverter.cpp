/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphLookAtSystemNode.h"
#include "behaviorGraphMimicConverter.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorProfiler.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/animatedIterators.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/mimicComponent.h"
#include "../engine/renderFrame.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphUtils.inl"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsConverterNode );

const Float CBehaviorGraphMimicsConverterNode::POSE_THRESHOLD = 0.01f;

CBehaviorGraphMimicsConverterNode::CBehaviorGraphMimicsConverterNode()
	: m_normalBlendTracksBegin( 129 )
	, m_mimicLipsyncOffset( 145 )
{
}

RED_DEFINE_STATIC_NAME( MimicBase )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicsConverterNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Anim ) ) );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( MimicBase ) ) );

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			mc->RebuildSockets( this );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

#endif

void CBehaviorGraphMimicsConverterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_placerBones;
	compiler << i_mimicsBones;

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			mc->BuildDataLayout( compiler );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	FindPlacersChildBones( instance );

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			mc->InitInstance( instance, this );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			mc->ReleaseInstance( instance, this );
		}
	}
}

static Bool CheckMimicsBonePrefix( const AnsiChar* boneName, const Char* prefix )
{
	// Check if bone name starts with prefix
	if ( prefix )
	{
		while ( *prefix )
		{
			// Different name
			if ( *prefix != *boneName )
			{
				return false;
			}

			// Advance
			++boneName;
			++prefix;
		}
	}

	// Valid bone name
	return true;
}

void CBehaviorGraphMimicsConverterNode::FindPlacersChildBones( CBehaviorGraphInstance& instance ) const
{
	TDynArray< Int32 >& placerBones = instance[ i_placerBones ];
	TDynArray< Int32 >& mimicsBones = instance[ i_mimicsBones ];

	placerBones.Clear();
	mimicsBones.Clear();

	// No placer
	if ( m_placerPrefix.Empty() )
	{
		return;
	}

	// Check bones	
	for ( BoneIterator it( instance.GetAnimatedComponent() ); it; ++it )
	{
		const Int32 boneParentIndex = (Int32)it.GetParent();
		const Int32 boneIndex = (Int32)it.GetIndex();

		if ( boneParentIndex != -1 )
		{
			const AnsiChar* boneParentName = it.GetParentName();

			if ( CheckMimicsBonePrefix( boneParentName, m_placerPrefix.AsChar() ) )
			{
				mimicsBones.PushBack( boneIndex );
			}
			else
			{
				placerBones.PushBack( boneIndex );
			}
		}
		else
		{
			placerBones.PushBack( boneIndex );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicsConverter );

	if ( CanWork( instance ) )
	{
		if ( m_cachedAnimInputNode )
		{
			m_cachedAnimInputNode->Update( context, instance, timeDelta );
		}

		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->Update( context, instance, timeDelta );
			}
		}

		if ( m_cachedMimicBaseInputNode )
		{
			m_cachedMimicBaseInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphMimicsConverterNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMimicsConverterNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = false;

	if ( m_cachedAnimInputNode )
	{
		ret |= m_cachedAnimInputNode->ProcessEvent( instance, event );
	}

	if ( m_cachedMimicBaseInputNode )
	{
		ret |= m_cachedMimicBaseInputNode->ProcessEvent( instance, event );
	}

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			ret |= mc->ProcessEvent( instance, event );
		}
	}

	return ret;
}


void CBehaviorGraphMimicsConverterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( CanWork( instance ) )
	{
		if ( m_cachedAnimInputNode )
		{
			m_cachedAnimInputNode->Activate( instance );
		}

		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->Activate( instance );
			}
		}

		if ( m_cachedMimicBaseInputNode )
		{
			m_cachedMimicBaseInputNode->Activate( instance );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( CanWork( instance ) )
	{
		if ( m_cachedAnimInputNode )
		{
			m_cachedAnimInputNode->Deactivate( instance );
		}

		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->Deactivate( instance );
			}
		}

		if ( m_cachedMimicBaseInputNode )
		{
			m_cachedMimicBaseInputNode->Deactivate( instance );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	if ( CanWork( instance ) )
	{
		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->Reset( instance );
			}
		}
	}
}

void CBehaviorGraphMimicsConverterNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedAnimInputNode = CacheBlock( TXT("Anim") );

	m_cachedMimicBaseInputNode = CacheMimicBlock( TXT("MimicBase") );

	for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
	{
		IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
		if ( mc )
		{
			mc->CacheConnections( this );
		}
	}
}

void CBehaviorGraphMimicsConverterNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( !m_generateEditorFragments )
	{
		return;
	}

	if ( CanWork( instance ) )
	{
		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->GenerateFragments( instance, frame );
			}
		}
	}
}

void CBehaviorGraphMimicsConverterNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( CanWork( instance ) )
	{
		if ( m_cachedAnimInputNode )
		{
			m_cachedAnimInputNode->ProcessActivationAlpha( instance, alpha );
		}

		for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
		{
			IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
			if ( mc )
			{
				mc->ProcessActivationAlpha( instance, alpha );
			}
		}

		if ( m_cachedMimicBaseInputNode )
		{
			m_cachedMimicBaseInputNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

Bool CBehaviorGraphMimicsConverterNode::CanWork( CBehaviorGraphInstance& instance ) const
{
	return instance.GetAnimatedComponent()->IsA< CMimicComponent >();
}

void CBehaviorGraphMimicsConverterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( MimicsConverter );

	if ( CanWork( instance ) && context.HasMimic() )
	{
		CCacheBehaviorGraphOutput cachePoseFormAnim( context );
		CCacheBehaviorGraphOutput cachePoseFromMimic( context );

		SBehaviorGraphOutput* poseFromAnim = cachePoseFormAnim.GetPose();
		SBehaviorGraphOutput* poseFromMimic = cachePoseFromMimic.GetPose();

		BehaviorUtils::BlendingUtils::SetPoseIdentity( *poseFromMimic );

		AnimQsTransform neckAddTransform( AnimQsTransform::IDENTITY );
		AnimQsTransform headAddTransform( AnimQsTransform::IDENTITY );

		if ( poseFromAnim && poseFromMimic )
		{
			// Animation
			if ( m_cachedAnimInputNode )
			{
				m_cachedAnimInputNode->Sample( context, instance, *poseFromAnim );
			}
			else if ( context.ShouldCorrectPose() )
			{
				// Bone correction
				context.SetPoseCorrection( *poseFromAnim );
			}

			// Mimic
			{
				const CMimicComponent* head = SafeCast< const CMimicComponent >( instance.GetAnimatedComponent() );

				const CMimicFace* face = head->GetMimicFace();
				if ( face )
				{
					CCacheBehaviorGraphOutput trackCachePoseBase( context, true );

					SBehaviorGraphOutput* trackOutputBase = trackCachePoseBase.GetPose();
					if ( trackOutputBase )
					{
						// Sample animation
						if ( m_cachedMimicBaseInputNode )
						{
							m_cachedMimicBaseInputNode->Sample( context, instance, *trackOutputBase );
						}

						trackOutputBase->Touch();

						{
							BEH_PROFILER_LEVEL_2( ConvertMimicPose );

							const Uint32 size = trackOutputBase->m_numFloatTracks;
			

							// Constraints - pre-pass
							for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
							{
								IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
								if ( mc )
								{
									mc->PreSample( context, instance, *poseFromAnim, *trackOutputBase );
								}
							}

							for ( Uint32 i=0; i<size; i++ )
							{
								Float poseWeight = trackOutputBase->m_floatTracks[ i ];

								// Additive blending
								if ( MAbs( poseWeight ) > POSE_THRESHOLD )
								{
									Uint32 poseIndex = i + 0;

									SBehaviorGraphOutput outPose;
									Bool ret = face->GetMimicPose( poseIndex, outPose );

									if ( ret )
									{
										face->AddMimicPose( *poseFromMimic, outPose, poseWeight );
									}
								}
							}

							// Constraints - post-pass
							for ( Uint32 i=0; i<m_mimicsConstraints.Size(); ++i )
							{
								IBehaviorMimicConstraint* mc = m_mimicsConstraints[ i ];
								if ( mc )
								{
									mc->PostSample( context, instance, *poseFromAnim, *trackOutputBase, *poseFromMimic );
								}
							}

							SMimicPostProcessData* mimicData = context.GetMimicPostProcessData();
							if ( mimicData )
							{
								FillMimicData( mimicData, *trackOutputBase );

								if ( trackOutputBase->m_numBones >= 2 )
								{
									neckAddTransform = trackOutputBase->m_outputPose[ 0 ];
									headAddTransform = trackOutputBase->m_outputPose[ 1 ];
								}
							}
							else
							{
								ASSERT( mimicData );
							}
						}

						poseFromMimic->MergeEvents( *trackOutputBase );
					}
				}
			}

			// Build final pose from anim and mimic poses
			// From anim pose we take placers
			// From mimic pose we take placers children

			ASSERT( output.m_numBones == poseFromAnim->m_numBones );
			ASSERT( output.m_numBones == poseFromMimic->m_numBones );
			
			const TDynArray< Int32 >& placerBones = instance[ i_placerBones ];
			const TDynArray< Int32 >& mimicsBones = instance[ i_mimicsBones ];

			const Uint32 numPlacerBones = placerBones.Size();
			const Uint32 numMimicsBones = mimicsBones.Size();
			
			for ( Uint32 i=0; i<numPlacerBones; ++i )
			{
				const Int32 boneIndex = placerBones[ i ];
				output.m_outputPose[ boneIndex ] = poseFromAnim->m_outputPose[ boneIndex ];
			}

			for ( Uint32 i=0; i<numMimicsBones; ++i )
			{
				const Int32 boneIndex = mimicsBones[ i ];
				output.m_outputPose[ boneIndex ] = poseFromMimic->m_outputPose[ boneIndex ];
			}

			// Merge events
			output.MergeEventsAndUsedAnims( *poseFromAnim );
			output.MergeEventsAndUsedAnims( *poseFromMimic );

			// Float tracks
			const Uint32 floatTrackSize = Min( output.m_numFloatTracks, poseFromAnim->m_numFloatTracks );
			for ( Uint32 i=0; i<floatTrackSize; ++i )
			{
				output.m_floatTracks[ i ] = poseFromAnim->m_floatTracks[ i ];
			}
	
			// Custom tracks
			COMPILE_ASSERT( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5 );
			output.m_customFloatTracks[ 0 ] = poseFromAnim->m_customFloatTracks[ 0 ];
			output.m_customFloatTracks[ 1 ] = poseFromAnim->m_customFloatTracks[ 1 ];
			output.m_customFloatTracks[ 2 ] = poseFromAnim->m_customFloatTracks[ 2 ];
			output.m_customFloatTracks[ 3 ] = poseFromAnim->m_customFloatTracks[ 3 ];
			output.m_customFloatTracks[ 4 ] = poseFromAnim->m_customFloatTracks[ 4 ];

			SMimicPostProcessData* mimicData = context.GetMimicPostProcessData();
			if ( mimicData )
			{
				const CMimicComponent* headComp = SafeCast< const CMimicComponent >( instance.GetAnimatedComponent() );

				const CMimicFace* face = headComp->GetMimicFace();

				COMPILE_ASSERT( MIMIC_POSE_BONES_NUM == 0 );

				//RedQsTransform t0 = mimicData->m_mimicBones[ 0 ];
				//RedQsTransform t1 = mimicData->m_mimicBones[ 1 ];

				Int32 neck, head;
				face->GetNeckAndHead( neck, head );

				output.m_outputPose[ neck ].SetMul( output.m_outputPose[ neck ], neckAddTransform ); // neck
				output.m_outputPose[ head ].SetMul( output.m_outputPose[ head ], headAddTransform ); // head
			}
		}
	}
}

void CBehaviorGraphMimicsConverterNode::FillMimicData( SMimicPostProcessData* mimicData, const SBehaviorGraphOutput &pose ) const
{
	COMPILE_ASSERT( MIMIC_POSE_AREAS_NUM == 16 );

	if ( m_normalBlendTracksBegin != -1 )
	{
		Red::System::MemoryCopy( &mimicData->m_mimicAreas[0], &pose.m_floatTracks[m_normalBlendTracksBegin], MIMIC_POSE_AREAS_NUM * sizeof(Float) );
	}

	COMPILE_ASSERT( MIMIC_POSE_BONES_NUM == 0 );

	/*if ( (Int32)pose.m_numFloatTracks <= m_mimicLipsyncOffset || pose.m_numBones < 2 )
	{
		mimicData->m_mimicBones[ 0 ].SetIdentity();
		mimicData->m_mimicBones[ 1 ].SetIdentity();
		mimicData->m_mimicLipsyncOffset = 0.f;

		return;
	}

	ASSERT( pose.m_outputPose[ 0 ].IsOk() );
	ASSERT( pose.m_outputPose[ 1 ].IsOk() );

	mimicData->m_mimicBones[ 0 ] = pose.m_outputPose[ 0 ];
	mimicData->m_mimicBones[ 1 ] = pose.m_outputPose[ 1 ];

	mimicData->m_mimicLipsyncOffset = pose.m_floatTracks[ m_mimicLipsyncOffset ]; // head1
	*/
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( IBehaviorMimicConstraint );
IMPLEMENT_ENGINE_CLASS( CBehaviorMimicLookAtConstraint );

RED_DEFINE_STATIC_NAME( EyesData );
RED_DEFINE_STATIC_NAME( blink_normal_face );

CBehaviorMimicLookAtConstraint::CBehaviorMimicLookAtConstraint()
	: m_eyeHorMax( 90.f )
	, m_eyeVerMax( 90.f )
	, m_eyeVerMin( 90.f )
	, m_dampTime( 1.f )
	, m_eyesTrackClamp( 0.75f )
	, m_blinkAnimName( CNAME( blink_normal_face ) )
	, m_blinkTimeOffset( 0.05f )
	, m_blinkSpeed( 0.9f )
	, m_longTransitionAngleDeg( 40.f )
	, m_longTransitionThrDeg( 15.f )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorMimicLookAtConstraint::RebuildSockets( CBehaviorGraphNode* parent )
{
	parent->CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
	parent->CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	parent->CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( EyesData ) ) );
}

#endif

void CBehaviorMimicLookAtConstraint::CacheConnections( CBehaviorGraphNode* parent )
{
	// Cache connections
	m_cachedControlVariableNode = parent->CacheValueBlock( TXT("Weight") );
	m_cachedTargetNode = parent->CacheVectorValueBlock( TXT("Target") );
	m_cachedControlEyesDataNode = parent->CacheVectorValueBlock( TXT("EyesData") );
}

void CBehaviorMimicLookAtConstraint::BuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_timeDelta;
	//compiler << i_eyeHorLeft;
	//compiler << i_eyeHorRight;
	//compiler << i_eyeVerLeft;
	//compiler << i_eyeVerRight;
	compiler << i_eyeLeftPlacer;
	compiler << i_eyeRightPlacer;
	compiler << i_weight;
	compiler << i_weightPrev;
	compiler << i_target;
	compiler << i_targetPrev;
	compiler << i_shift;
	compiler << i_eyeLeftDirLS;
	compiler << i_eyeRightDirLS;
	compiler << i_eyeHorLeftValue;
	compiler << i_eyeHorRightValue;
	compiler << i_eyeVerLeftValue;
	compiler << i_eyeVerRightValue;
	compiler << i_eyeHorLeftValue_Cached;
	compiler << i_eyeHorRightValue_Cached;
	compiler << i_eyeVerLeftValue_Cached;
	compiler << i_eyeVerRightValue_Cached;
	compiler << i_eyeHorLLeft;
	compiler << i_eyeHorRLeft;
	compiler << i_eyeHorLRight;
	compiler << i_eyeHorRRight;
	compiler << i_eyeVerULeft;
	compiler << i_eyeVerDLeft;
	compiler << i_eyeVerURight;
	compiler << i_eyeVerDRight;
	compiler << i_tracksValid;
	compiler << i_eyesCompressedData;
	compiler << i_longTransitionMode;
	compiler << i_longTransitionCachedTarget;
	compiler << i_longTransitionBlinkTimer;
	compiler << i_longTransitionBlinkAnimTime;
	compiler << i_longTransitionBlinkAnim;
	compiler << i_longTransitionEvtTargetChanged;
	compiler << i_longTransitionTargetChangedFlag;
}

void CBehaviorMimicLookAtConstraint::InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const
{
	InternalReset( instance );

	instance[ i_tracksValid ] = true;
	instance[ i_eyesCompressedData ] = Vector::ZEROS;

	//instance[ i_eyeHorLeft ] = -1;
	//instance[ i_eyeHorRight ] = -1;
	//instance[ i_eyeVerLeft ] = -1;
	//instance[ i_eyeVerRight ] = -1;

	instance[ i_eyeLeftPlacer ] = -1;
	instance[ i_eyeRightPlacer ] = -1;

	instance[ i_eyeLeftDirLS ] = Vector::ZERO_3D_POINT;
	instance[ i_eyeRightDirLS ] = Vector::ZERO_3D_POINT;

	instance[ i_eyeHorLLeft ] = -1;
	instance[ i_eyeHorRLeft ] = -1;
	instance[ i_eyeHorLRight ] = -1;
	instance[ i_eyeHorRRight ] = -1;
	instance[ i_eyeVerULeft ] = -1;
	instance[ i_eyeVerDLeft ] = -1;
	instance[ i_eyeVerURight ] = -1;
	instance[ i_eyeVerDRight ] = -1;

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	const CSkeleton* mimicSkeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();

	if ( skeleton && mimicSkeleton )
	{
		// Tracks
		//instance[ i_eyeHorLeft ] = parent->FindTrackIndex( m_eyeHorLeftTrack, mimicSkeleton );
		//instance[ i_eyeHorRight ] = parent->FindTrackIndex( m_eyeHorRightTrack, mimicSkeleton );

		//instance[ i_eyeVerLeft ] = parent->FindTrackIndex( m_eyeVerLeftTrack, mimicSkeleton );
		//instance[ i_eyeVerRight ] = parent->FindTrackIndex( m_eyeVerRightTrack, mimicSkeleton );

		instance[ i_eyeHorLLeft ] = parent->FindTrackIndex( m_eyeHorLLeftTrack, mimicSkeleton );
		instance[ i_eyeHorRLeft ] = parent->FindTrackIndex( m_eyeHorRLeftTrack, mimicSkeleton );
		instance[ i_eyeHorLRight ] = parent->FindTrackIndex( m_eyeHorLRightTrack, mimicSkeleton );
		instance[ i_eyeHorRRight ] = parent->FindTrackIndex( m_eyeHorRRightTrack, mimicSkeleton );

		instance[ i_eyeVerULeft ] = parent->FindTrackIndex( m_eyeVerULeftTrack, mimicSkeleton );
		instance[ i_eyeVerDLeft ] = parent->FindTrackIndex( m_eyeVerDLeftTrack, mimicSkeleton );
		instance[ i_eyeVerURight ] = parent->FindTrackIndex( m_eyeVerURightTrack, mimicSkeleton );
		instance[ i_eyeVerDRight ] = parent->FindTrackIndex( m_eyeVerDRightTrack, mimicSkeleton );

		// Bones
		instance[ i_eyeLeftPlacer ] = parent->FindBoneIndex( m_eyeLeftPlacerBone.AsChar(), skeleton );
		instance[ i_eyeRightPlacer ] = parent->FindBoneIndex( m_eyeRightPlacerBone.AsChar(), skeleton );

		// Bones direction
		const Vector boneDir = BehaviorUtils::VectorFromAxis( A_X );

		Vector& eyeLeftDirLS = instance[ i_eyeLeftDirLS ];
		Vector& eyeRightDirLS = instance[ i_eyeRightDirLS ];

		eyeLeftDirLS = Vector::ZERO_3D_POINT;
		eyeRightDirLS = Vector::ZERO_3D_POINT;

		if ( instance[ i_eyeLeftPlacer ] != -1 )
		{
			Matrix boneMS = skeleton->GetBoneMatrixMS( instance[ i_eyeLeftPlacer ] );

			boneMS.SetTranslation( Vector::ZERO_3D_POINT );
			boneMS.Invert();

			eyeLeftDirLS = boneMS.TransformVector( boneDir ).Normalized3();
		}
	}

	if ( instance[ i_eyeHorLLeft ] == -1 || instance[ i_eyeHorRLeft ] == -1 || instance[ i_eyeHorLRight ] == -1 || instance[ i_eyeHorRRight ] == -1 || 
		instance[ i_eyeVerULeft ] == -1 || instance[ i_eyeVerDLeft ] == -1 || instance[ i_eyeVerURight ] == -1 || instance[ i_eyeVerDRight ] == -1 ||
		instance[ i_eyeLeftPlacer ] == -1 || instance[ i_eyeRightPlacer ] == -1 )
	{
		instance[ i_tracksValid ] = false;
	}

	instance[ i_longTransitionMode ] = LTS_None;
	instance[ i_longTransitionBlinkTimer ] = 0.f;
	instance[ i_longTransitionBlinkAnimTime ] = 0.f;

	CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
	instance[ i_longTransitionBlinkAnim ] = cont->FindAnimationRestricted( m_blinkAnimName );

	instance[ i_longTransitionEvtTargetChanged ] = instance.GetEventId( CBehaviorGraphMimicsLookAtMediator::EVT_TARGET_CHANGED );
}

void CBehaviorMimicLookAtConstraint::InternalReset( CBehaviorGraphInstance& instance )const
{
	instance[ i_timeDelta ] = 0.f;
	instance[ i_weight ] = 0.f;
	instance[ i_weightPrev ] = 0.f;
	instance[ i_eyesCompressedData ] = Vector::ZEROS;
	instance[ i_target ] = Vector::ZERO_3D_POINT;
	instance[ i_targetPrev ] = Vector::ZERO_3D_POINT;
	instance[ i_shift ] = 0.f;
	instance[ i_longTransitionMode ] = LTS_None;
	instance[ i_longTransitionCachedTarget ] = Vector::ZERO_3D_POINT;
	instance[ i_longTransitionBlinkTimer ] = 0.f;
	instance[ i_longTransitionBlinkAnimTime ] = 0.f;
	instance[ i_longTransitionTargetChangedFlag ] = false;

	instance[ i_eyeHorLeftValue ] = 0.f;
	instance[ i_eyeHorRightValue ] = 0.f;
	instance[ i_eyeVerLeftValue ] = 0.f;
	instance[ i_eyeVerRightValue ] = 0.f;
	instance[ i_eyeHorLeftValue_Cached ] = 0.f;
	instance[ i_eyeHorRightValue_Cached ] = 0.f;
	instance[ i_eyeVerLeftValue_Cached ] = 0.f;
	instance[ i_eyeVerRightValue_Cached ] = 0.f;
}

void CBehaviorMimicLookAtConstraint::Reset( CBehaviorGraphInstance& instance ) const
{
	InternalReset( instance );
}

void CBehaviorMimicLookAtConstraint::Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( LookAtConstraint );

	instance[ i_weightPrev ] = instance[ i_weight ];
	instance[ i_targetPrev ] = instance[ i_target ];
	instance[ i_timeDelta ] = timeDelta;

	CBehaviorGraphMimicsLookAtMediator mediator( instance );
	if ( mediator.IsValid() && mediator.IsEnabled() )
	{
		if ( m_cachedControlVariableNode )
		{
			m_cachedControlVariableNode->Update( context, instance, timeDelta );
		}
		if ( m_cachedControlEyesDataNode )
		{
			m_cachedControlEyesDataNode->Update( context, instance, timeDelta );
		}
		if ( m_cachedTargetNode )
		{
			m_cachedTargetNode->Update( context, instance, timeDelta );
		}

		Vector pointA, pointB;
		Float weight, progress;
		mediator.GetData( weight, pointA, pointB, progress );

		// TODO
		const Float targetWeight = progress;

		const Vector pointWS = Vector::Interpolate( pointA, pointB, targetWeight );

		// Convert to animated component space
		//++TODO
		const Matrix& mat = instance.GetAnimatedComponent()->GetThisFrameTempLocalToWorld();
		Vector targetMS = mat.FullInverted().TransformPoint( pointWS );
		//--TODO

		instance[ i_target ] = targetMS;
		instance[ i_weight ] = weight;
	}
	else
	{
		if ( m_cachedControlVariableNode )
		{
			m_cachedControlVariableNode->Update( context, instance, timeDelta );
			instance[ i_weight ] = Clamp( m_cachedControlVariableNode->GetValue( instance ), 0.f, 1.f );
		}

		if ( m_cachedControlEyesDataNode )
		{
			m_cachedControlEyesDataNode->Update( context, instance, timeDelta );
			instance[ i_eyesCompressedData ] = m_cachedControlEyesDataNode->GetVectorValue( instance );
		}

		UpdateShift( instance, timeDelta );

		if ( m_cachedTargetNode )
		{
			m_cachedTargetNode->Update( context, instance, timeDelta );

			const Vector inputTargetMS = m_cachedTargetNode->GetVectorValue( instance );
			Vector& targetMS = instance[ i_target ];

			targetMS = inputTargetMS;
		}
	}
}

Float CBehaviorMimicLookAtConstraint::GetShiftSpeed( CBehaviorGraphInstance& instance ) const
{
	const Vector& eyesData = instance[ i_eyesCompressedData ];
	const Float dampScale = Max( 0.001f, eyesData.Z );
	const Float speed = 1.f / ( dampScale * m_dampTime );
	return speed;
}

void CBehaviorMimicLookAtConstraint::UpdateShift( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	const Float speed = GetShiftSpeed( instance );
	instance[ i_shift ] = speed * timeDelta;
}

void CBehaviorMimicLookAtConstraint::PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const
{	
	Bool& longTransitionTargetChangedFlag = instance[ i_longTransitionTargetChangedFlag ];

	const Bool valid = instance[ i_tracksValid ];
	if ( !valid || !context.HasMimic() || instance[ i_weight ] < 0.001f )
	{
		instance[ i_longTransitionMode ] = LTS_None;
		instance[ i_longTransitionCachedTarget ] = Vector::ZERO_3D_POINT;
		instance[ i_longTransitionBlinkTimer ] = 0.f;
		instance[ i_longTransitionBlinkAnimTime ] = 0.f;
		instance[ i_longTransitionTargetChangedFlag ] = false;

		instance[ i_eyeHorLeftValue ] = 0.f;
		instance[ i_eyeHorRightValue ] = 0.f;
		instance[ i_eyeVerLeftValue ] = 0.f;
		instance[ i_eyeVerRightValue ] = 0.f;
		instance[ i_eyeHorLeftValue_Cached ] = 0.f;
		instance[ i_eyeHorRightValue_Cached ] = 0.f;
		instance[ i_eyeVerLeftValue_Cached ] = 0.f;
		instance[ i_eyeVerRightValue_Cached ] = 0.f;

		longTransitionTargetChangedFlag = false;
		return;
	}

	const Float timeDelta = instance[ i_timeDelta ];

	// Solver data
	SolverData leftEyeData;
	SolverData rightEyeData;

	// Setup
	const Vector targetMS = instance[ i_target ];
	if ( GetSolversData( leftEyeData, rightEyeData, instance, animPose, mimicPose, targetMS ) )
	{
		CBehaviorGraphMimicsLookAtMediator mediator( instance );

		// Solver
		Solve( leftEyeData );
		Solve( rightEyeData );

		// Compressed data
		const Vector& eyesData = instance[ i_eyesCompressedData ];

		Float coWeight = eyesData.X;
		Bool isAdditive = eyesData.Y > 0.5f;

		// Set data do mimic pose
		const Float shift = instance[ i_shift ];

		ASSERT( coWeight >= 0.f && coWeight <= 1.f );
		coWeight = Clamp( coWeight, 0.f, 1.f );

		if ( coWeight < 1.f )
		{
			ApplyEyesCoWeight( leftEyeData, rightEyeData, coWeight );
		}

		/*
		const Int32 trackLeftHor = instance[ i_eyeHorLeft ];
		const Int32 trackLeftVer = instance[ i_eyeVerLeft ];
		const Int32 trackRightHor = instance[ i_eyeHorRight ];
		const Int32 trackRightVer = instance[ i_eyeVerRight ];

		Float& eyeHorLeftValue = instance[ i_eyeHorLeftValue ];
		Float& eyeVerLeftValue = instance[ i_eyeVerLeftValue ];
		Float& eyeHorRightValue = instance[ i_eyeHorRightValue ];
		Float& eyeVerRightValue = instance[ i_eyeVerRightValue ];

		eyeHorLeftValue = Clamp( eyeHorLeftValue + Clamp( leftEyeData.m_horTrackValue - eyeHorLeftValue, -shift, shift ), -1.f, 1.f );
		eyeVerLeftValue = Clamp( eyeVerLeftValue + Clamp( leftEyeData.m_verTrackValue - eyeVerLeftValue, -shift, shift ), -1.f, 1.f );
		eyeHorRightValue = Clamp( eyeHorRightValue + Clamp( rightEyeData.m_horTrackValue - eyeHorRightValue, -shift, shift ), -1.f, 1.f );
		eyeVerRightValue = Clamp( eyeVerRightValue + Clamp( rightEyeData.m_verTrackValue - eyeVerRightValue, -shift, shift ), -1.f, 1.f );

		mimicPose.m_floatTracks[ trackLeftHor ] += eyeHorLeftValue;
		mimicPose.m_floatTracks[ trackLeftVer ] += eyeVerLeftValue;
		mimicPose.m_floatTracks[ trackRightHor ] += eyeHorRightValue;
		mimicPose.m_floatTracks[ trackRightVer ] += eyeVerRightValue;*/

		const Int32 trackEyeHorLLeft = instance[ i_eyeHorLLeft ];
		const Int32 trackEyeHorRLeft = instance[ i_eyeHorRLeft ];
		const Int32 trackEyeHorLRight = instance[ i_eyeHorLRight ];
		const Int32 trackEyeHorRRight = instance[ i_eyeHorRRight ];

		const Int32 trackEyeVerULeft = instance[ i_eyeVerULeft ];
		const Int32 trackEyeVerDLeft = instance[ i_eyeVerDLeft ];
		const Int32 trackEyeVerURight = instance[ i_eyeVerURight ];
		const Int32 trackEyeVerDRight = instance[ i_eyeVerDRight ];

		Float& eyeHorLeftValue = instance[ i_eyeHorLeftValue ];
		Float& eyeVerLeftValue = instance[ i_eyeVerLeftValue ];
		Float& eyeHorRightValue = instance[ i_eyeHorRightValue ];
		Float& eyeVerRightValue = instance[ i_eyeVerRightValue ];

		const Float eyeHorLeftValue_diff = leftEyeData.m_horTrackValue - eyeHorLeftValue;
		const Float eyeVerLeftValue_diff = leftEyeData.m_verTrackValue - eyeVerLeftValue;
		const Float eyeHorRightValue_diff = rightEyeData.m_horTrackValue - eyeHorRightValue;
		const Float eyeVerRightValue_diff = rightEyeData.m_verTrackValue - eyeVerRightValue;
		
		const Float eyeHor_diff = ( eyeHorLeftValue_diff + eyeHorRightValue_diff ) / 2.f;
		const Float eyeHorAngle_diff = eyeHor_diff * m_eyeHorMax;
		const Float eyeHorAngle_diffAbs = MAbs( eyeHorAngle_diff );

		const Bool wasActive = instance[ i_weightPrev ] > 0.f;
		const Bool isActive = instance[ i_weight ] > 0.f;
		ASSERT( isActive );

		const Bool isLookAtAlreadySet = longTransitionTargetChangedFlag && wasActive;
		const Bool startFromZero = longTransitionTargetChangedFlag && !wasActive && isActive;

		Int32& longTransitionMode = instance[ i_longTransitionMode ];
		if ( longTransitionMode == LTS_None && ( isLookAtAlreadySet || startFromZero ) )
		{
			if ( eyeHorAngle_diffAbs > m_longTransitionAngleDeg )
			{
				longTransitionMode = LTS_0_StartAndWait;
				
				Vector& longTransitionCachedTarget = instance[ i_longTransitionCachedTarget ];
				
				if ( isLookAtAlreadySet )
				{
					longTransitionCachedTarget = instance[ i_targetPrev ];
				}
				else
				{
					ASSERT( startFromZero );
					
					AnimQsTransform boneMidMS;
					boneMidMS.Lerp( leftEyeData.m_placerMS, rightEyeData.m_placerMS, 0.5f );
				
					static EAxis boneMidDir = A_X;
					const AnimVector4 boneMidDirVecLS = BehaviorUtils::RedVectorFromAxis( boneMidDir );
					AnimVector4 boneMidDirVecMS;
					boneMidDirVecMS.RotateDirection( boneMidMS.Rotation, boneMidDirVecLS );

					AnimVector4 vec = Add( boneMidMS.Translation, Mul( boneMidDirVecMS, 2.f ) );
					longTransitionCachedTarget = AnimVectorToVector( vec );
				}

				Float& longTransitionBlinkTimer = instance[ i_longTransitionBlinkTimer ];
				//ASSERT( longTransitionBlinkTimer == 0.f );
				longTransitionBlinkTimer = 0.f;

				Float longTransitionBlinkAnimTime = instance[ i_longTransitionBlinkAnimTime ];
				//ASSERT( longTransitionBlinkAnimTime == 0.f );

				Float& eyeHorLeftValue_Cached = instance[ i_eyeHorLeftValue_Cached ];
				Float& eyeVerLeftValue_Cached = instance[ i_eyeVerLeftValue_Cached ];
				Float& eyeHorRightValue_Cached = instance[ i_eyeHorRightValue_Cached ];
				Float& eyeVerRightValue_Cached = instance[ i_eyeVerRightValue_Cached ];

				eyeHorLeftValue_Cached = eyeHorLeftValue;
				eyeVerLeftValue_Cached = eyeVerLeftValue;
				eyeHorRightValue_Cached = eyeHorRightValue;
				eyeVerRightValue_Cached = eyeVerRightValue;
			}
		}

		if ( longTransitionMode > LTS_None && eyeHorAngle_diffAbs < 1.f )
		{
			// For safe
			longTransitionMode = LTS_None;
		}

		if ( longTransitionMode == LTS_0_StartAndWait || longTransitionMode == LTS_1_WaitForBlink )
		{
			const Vector& longTransitionCachedTargetMS = instance[ i_longTransitionCachedTarget ];

			SolverData leftEyeData_Cached;
			SolverData rightEyeData_Cached;

			if ( GetSolversData( leftEyeData_Cached, rightEyeData_Cached, instance, animPose, mimicPose, longTransitionCachedTargetMS ) )
			{
				Solve( leftEyeData_Cached );
				Solve( rightEyeData_Cached );

				if ( coWeight < 1.f )
				{
					ApplyEyesCoWeight( leftEyeData_Cached, rightEyeData_Cached, coWeight );
				}

				const Float eyeHorLeftValue_Cached = instance[ i_eyeHorLeftValue_Cached ];
				const Float eyeVerLeftValue_Cached = instance[ i_eyeVerLeftValue_Cached ];
				const Float eyeHorRightValue_Cached = instance[ i_eyeHorRightValue_Cached ];
				const Float eyeVerRightValue_Cached = instance[ i_eyeVerRightValue_Cached ];

				Float eyeHorLeftValue_diff_cached = leftEyeData_Cached.m_horTrackValue - eyeHorLeftValue_Cached;
				Float eyeVerLeftValue_diff_cached = leftEyeData_Cached.m_verTrackValue - eyeVerLeftValue_Cached;
				Float eyeHorRightValue_diff_cached = rightEyeData_Cached.m_horTrackValue - eyeHorRightValue_Cached;
				Float eyeVerRightValue_diff_cached = rightEyeData_Cached.m_verTrackValue - eyeVerRightValue_Cached;

				static Bool XXX = true;
				if ( XXX )
				{
					eyeHorLeftValue_diff_cached = leftEyeData_Cached.m_horTrackValue;
					eyeVerLeftValue_diff_cached = leftEyeData_Cached.m_verTrackValue;
					eyeHorRightValue_diff_cached = rightEyeData_Cached.m_horTrackValue;
					eyeVerRightValue_diff_cached = rightEyeData_Cached.m_verTrackValue;
				}

				const Float eyeHor_diff_cached = ( eyeHorLeftValue_diff_cached + eyeHorRightValue_diff_cached ) / 2.f;
				const Float eyeHorAngle_diff_cached = eyeHor_diff_cached * m_eyeHorMax;
				const Float eyeHorAngle_diffAbs_cached = MAbs( eyeHorAngle_diff_cached );

				if ( longTransitionMode == LTS_0_StartAndWait )
				{
					if ( eyeHorAngle_diffAbs_cached > m_longTransitionThrDeg )
					{
						// Start blink
						Float& longTransitionBlinkTimer = instance[ i_longTransitionBlinkTimer ];
						//ASSERT( longTransitionBlinkTimer == 0.f );
						longTransitionBlinkTimer = timeDelta;

						Float& longTransitionBlinkAnimTime = instance[ i_longTransitionBlinkAnimTime ];
						//ASSERT( longTransitionBlinkAnimTime == 0.f );
						if ( longTransitionBlinkAnimTime == 0.f )
						{
							longTransitionBlinkAnimTime = 0.0001f; // start
						}

						longTransitionMode = LTS_1_WaitForBlink;
					}
				}

				if ( longTransitionMode == LTS_1_WaitForBlink )
				{
					Float& longTransitionBlinkTimer = instance[ i_longTransitionBlinkTimer ];
					longTransitionBlinkTimer += timeDelta;
					if ( longTransitionBlinkTimer > m_blinkTimeOffset )
					{
						longTransitionMode = LTS_None;
						longTransitionBlinkTimer = 0.f;
					}
				}

				// Wait
				leftEyeData = leftEyeData_Cached;
				rightEyeData = rightEyeData_Cached;
			}
			else
			{
				longTransitionMode = LTS_None;
			}
		}

		Float& longTransitionBlinkAnimTime = instance[ i_longTransitionBlinkAnimTime ];
		if ( longTransitionBlinkAnimTime > 0.f )
		{
			longTransitionBlinkAnimTime += m_blinkSpeed * timeDelta;

			Float duration( 0.5f );

			CSkeletalAnimationSetEntry* longTransitionBlinkAnim = instance[ i_longTransitionBlinkAnim ];
			if ( longTransitionBlinkAnim )
			{
				duration = longTransitionBlinkAnim->GetDuration();

				const Float animTime = Min( longTransitionBlinkAnimTime, duration );

				if ( context.HasMimic() )
				{
					CCacheBehaviorGraphOutput cachePose( context, true );
					SBehaviorGraphOutput* pose = cachePose.GetPose();
					if ( pose )
					{
						Bool ret = longTransitionBlinkAnim->GetAnimation()->Sample( animTime,
							pose->m_numBones,
							pose->m_numFloatTracks,
							pose->m_outputPose, 
							pose->m_floatTracks );

						const Uint32 poseNumTracks = Min< Uint32 >( mimicPose.m_numFloatTracks, pose->m_numFloatTracks );
						for( Uint32 i=0; i<poseNumTracks; ++i )
						{
							mimicPose.m_floatTracks[i] += pose->m_floatTracks[i];
						}
					}
				}
			}

			if ( longTransitionBlinkAnimTime > duration )
			{
				longTransitionBlinkAnimTime = 0.f; // stop
			}
		}

		if ( mediator.IsValid() && mediator.IsEnabled() )
		{
			eyeHorLeftValue = Clamp( leftEyeData.m_horTrackValue, -1.f, 1.f );
			eyeVerLeftValue = Clamp( leftEyeData.m_verTrackValue, -1.f, 1.f );
			eyeHorRightValue = Clamp( rightEyeData.m_horTrackValue, -1.f, 1.f );
			eyeVerRightValue = Clamp( rightEyeData.m_verTrackValue, -1.f, 1.f );

			 isAdditive = true;
		}
		else
		{
			eyeHorLeftValue = Clamp( eyeHorLeftValue + Clamp( leftEyeData.m_horTrackValue - eyeHorLeftValue, -shift, shift ), -1.f, 1.f );
			eyeVerLeftValue = Clamp( eyeVerLeftValue + Clamp( leftEyeData.m_verTrackValue - eyeVerLeftValue, -shift, shift ), -1.f, 1.f );
			eyeHorRightValue = Clamp( eyeHorRightValue + Clamp( rightEyeData.m_horTrackValue - eyeHorRightValue, -shift, shift ), -1.f, 1.f );
			eyeVerRightValue = Clamp( eyeVerRightValue + Clamp( rightEyeData.m_verTrackValue - eyeVerRightValue, -shift, shift ), -1.f, 1.f );
		}

		const Float trackValEyeHorLLeft = eyeHorLeftValue > 0.f ? eyeHorLeftValue : 0.f;
		const Float trackValEyeHorRLeft = eyeHorLeftValue < 0.f ? -eyeHorLeftValue : 0.f;
		const Float trackValEyeHorLRight = eyeHorRightValue > 0.f ? eyeHorRightValue : 0.f;
		const Float trackValEyeHorRRight = eyeHorRightValue < 0.f ? -eyeHorRightValue : 0.f;

		const Float trackValEyeVerULeft = eyeVerLeftValue < 0.f ? -eyeVerLeftValue : 0.f;
		const Float trackValEyeVerDLeft = eyeVerLeftValue > 0.f ? eyeVerLeftValue : 0.f;
		const Float trackValEyeVerURight = eyeVerRightValue < 0.f ? -eyeVerRightValue : 0.f;
		const Float trackValEyeVerDRight = eyeVerRightValue > 0.f ? eyeVerRightValue : 0.f;

		if ( isAdditive )
		{
			mimicPose.m_floatTracks[ trackEyeHorLLeft ] += trackValEyeHorLLeft;
			mimicPose.m_floatTracks[ trackEyeHorRLeft ] += trackValEyeHorRLeft;
			mimicPose.m_floatTracks[ trackEyeHorLRight ] += trackValEyeHorLRight;
			mimicPose.m_floatTracks[ trackEyeHorRRight ] += trackValEyeHorRRight;

			mimicPose.m_floatTracks[ trackEyeVerULeft ] += trackValEyeVerULeft;
			mimicPose.m_floatTracks[ trackEyeVerDLeft ] += trackValEyeVerDLeft;
			mimicPose.m_floatTracks[ trackEyeVerURight ] += trackValEyeVerURight;
			mimicPose.m_floatTracks[ trackEyeVerDRight ] += trackValEyeVerDRight;

			static Bool CLAMP = true;
			if ( CLAMP )
			{
				mimicPose.m_floatTracks[ trackEyeHorLLeft ] = Clamp( mimicPose.m_floatTracks[ trackEyeHorLLeft ], 0.f, m_eyesTrackClamp );
				mimicPose.m_floatTracks[ trackEyeHorRLeft ] = Clamp( mimicPose.m_floatTracks[ trackEyeHorRLeft ], 0.f, m_eyesTrackClamp );
				mimicPose.m_floatTracks[ trackEyeHorLRight ] = Clamp( mimicPose.m_floatTracks[ trackEyeHorLRight ], 0.f, m_eyesTrackClamp );
				mimicPose.m_floatTracks[ trackEyeHorRRight ] = Clamp( mimicPose.m_floatTracks[ trackEyeHorRRight ], 0.f, m_eyesTrackClamp );

				mimicPose.m_floatTracks[ trackEyeVerULeft ] = Clamp( mimicPose.m_floatTracks[ trackEyeVerULeft ], 0.f, 1.f );
				mimicPose.m_floatTracks[ trackEyeVerDLeft ] = Clamp( mimicPose.m_floatTracks[ trackEyeVerDLeft ], 0.f, 1.f );
				mimicPose.m_floatTracks[ trackEyeVerURight ] = Clamp( mimicPose.m_floatTracks[ trackEyeVerURight ], 0.f, 1.f );
				mimicPose.m_floatTracks[ trackEyeVerDRight ] = Clamp( mimicPose.m_floatTracks[ trackEyeVerDRight ], 0.f, 1.f );
			}
		}
		else
		{
			mimicPose.m_floatTracks[ trackEyeHorLLeft ] = trackValEyeHorLLeft;
			mimicPose.m_floatTracks[ trackEyeHorRLeft ] = trackValEyeHorRLeft;
			mimicPose.m_floatTracks[ trackEyeHorLRight ] = trackValEyeHorLRight;
			mimicPose.m_floatTracks[ trackEyeHorRRight ] = trackValEyeHorRRight;

			mimicPose.m_floatTracks[ trackEyeVerULeft ] = trackValEyeVerULeft;
			mimicPose.m_floatTracks[ trackEyeVerDLeft ] = trackValEyeVerDLeft;
			mimicPose.m_floatTracks[ trackEyeVerURight ] = trackValEyeVerURight;
			mimicPose.m_floatTracks[ trackEyeVerDRight ] = trackValEyeVerDRight;
		}
	}

	longTransitionTargetChangedFlag = false;
}

void CBehaviorMimicLookAtConstraint::ApplyEyesCoWeight( SolverData& leftEyeData, SolverData& rightEyeData, Float coWeight ) const
{
	// TODO
	const Float avgHor = Clamp( ( leftEyeData.m_horTrackValue + rightEyeData.m_horTrackValue ) / 2.f, -1.f, 1.f );
	const Float avgVer = Clamp( ( leftEyeData.m_verTrackValue + rightEyeData.m_verTrackValue ) / 2.f, -1.f, 1.f );

	leftEyeData.m_horTrackValue = Lerp( coWeight, avgHor, leftEyeData.m_horTrackValue );
	rightEyeData.m_horTrackValue = Lerp( coWeight, avgHor, rightEyeData.m_horTrackValue );

	leftEyeData.m_verTrackValue = Lerp( coWeight, avgVer, leftEyeData.m_verTrackValue );
	rightEyeData.m_verTrackValue = Lerp( coWeight, avgVer, rightEyeData.m_verTrackValue );
}

namespace
{
	AnimQsTransform GetBoneModelTransform( const SBehaviorGraphOutput &animPose, const CMimicComponent* ac, Int32 boneIndex, const SBehaviorGraphOutput &mimicPose )
	{
		ASSERT( mimicPose.m_numBones == 2 );

		const Int16* parentIndices = ac->GetSkeleton() ? ac->GetSkeleton()->GetParentIndices() : nullptr;
		if ( !parentIndices || boneIndex < 0 || boneIndex >= (Int32)animPose.m_numBones || mimicPose.m_numBones < 2 )
		{
			RedQsTransform retVal;
			retVal.SetIdentity();
			return retVal;
		}

		Int32 neck = -1;
		Int32 head = -1;
		const CMimicFace* fac = ac->GetMimicFace();
		if ( fac )
		{
			fac->GetNeckAndHead( neck, head );
		}
		ASSERT( neck != -1 );
		ASSERT( head != -1 );

		RedQsTransform bone = animPose.m_outputPose[ boneIndex ];

		if ( neck != -1 && head != -1 )
		{
			Int32 currBone = parentIndices[ boneIndex ];
			while( currBone > head )
			{
				bone.SetMul( animPose.m_outputPose[ currBone ], bone );
				currBone = parentIndices[ currBone ];
			}

			ASSERT( neck < (Int32)animPose.m_numBones );
			ASSERT( head < (Int32)animPose.m_numBones );

			if ( neck < (Int32)animPose.m_numBones && head < (Int32)animPose.m_numBones )
			{
				RedQsTransform headBone;
				RedQsTransform neckBone;

				headBone.SetMul( animPose.m_outputPose[ head ], mimicPose.m_outputPose[ 1 ] );
				neckBone.SetMul( animPose.m_outputPose[ neck ], mimicPose.m_outputPose[ 0 ] );

				bone.SetMul( headBone, bone );
				bone.SetMul( neckBone, bone );
				bone.SetMul( animPose.m_outputPose[ 0 ], bone );
			}
		}

		return bone;
	}
}

Bool CBehaviorMimicLookAtConstraint::GetSolversData( CBehaviorGraphMimicLookAtSystemNode::SolverData& leftEye, 
													 CBehaviorGraphMimicLookAtSystemNode::SolverData& rightEye, 
													 CBehaviorGraphInstance& instance, 
													 const SBehaviorGraphOutput &animPose,
													 const SBehaviorGraphOutput &mimicPose,
													 const Vector& targetMS ) const
{
	const Float weight = instance[ i_weight ];
	const Int32 eyeLeftPlacer = instance[ i_eyeLeftPlacer ];
	const Int32 eyeRightPlacer = instance[ i_eyeRightPlacer ];
	const RedVector4 hkTargetMS = reinterpret_cast< const RedVector4& >( targetMS );

	if ( eyeLeftPlacer != -1 && eyeRightPlacer != -1 )
	{
		const CMimicComponent* mimicComponent = Cast< const CMimicComponent >( instance.GetAnimatedComponent() );

		RedQsTransform placerLeft = GetBoneModelTransform( animPose, mimicComponent, eyeLeftPlacer, mimicPose );
		RedQsTransform placerRight = GetBoneModelTransform( animPose, mimicComponent, eyeRightPlacer, mimicPose );

		//RedQsTransform placerMS;
		//placerMS.Translation.Lerp( placerLeft.Translation, placerRight.Translation, 0.5f );

		//placerLeft.Translation = placerMS.Translation;
		//placerRight.Translation = placerMS.Translation;

		leftEye.m_placerMS = placerLeft;
		leftEye.m_targetMS = hkTargetMS;
		leftEye.m_weight = weight;
		leftEye.m_horMax = m_eyeHorMax;
		leftEye.m_verMin = m_eyeVerMin;
		leftEye.m_verMax = m_eyeVerMax;
		leftEye.m_verOffset = m_eyeVerOffset;
		leftEye.m_mirrored = true;
		
		rightEye.m_placerMS = placerRight;
		rightEye.m_targetMS = hkTargetMS;
		rightEye.m_weight = weight;
		rightEye.m_horMax = m_eyeHorMax;
		rightEye.m_verMin = m_eyeVerMin;
		rightEye.m_verMax = m_eyeVerMax;
		rightEye.m_verOffset = m_eyeVerOffset;
		rightEye.m_mirrored = false;

		return true;
	}
	else
	{
		return false;
	}
}

void CBehaviorMimicLookAtConstraint::Activate( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedControlEyesDataNode )
	{
		m_cachedControlEyesDataNode->Activate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Activate( instance );
	}

	if ( CSkeletalAnimationSetEntry* longTransitionBlinkAnim = instance[ i_longTransitionBlinkAnim ] )
	{
		longTransitionBlinkAnim->GetAnimation()->AddUsage();
	}
}

void CBehaviorMimicLookAtConstraint::Deactivate( CBehaviorGraphInstance& instance ) const
{
	if ( CSkeletalAnimationSetEntry* longTransitionBlinkAnim = instance[ i_longTransitionBlinkAnim ] )
	{
		longTransitionBlinkAnim->GetAnimation()->ReleaseUsage();
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedControlEyesDataNode )
	{
		m_cachedControlEyesDataNode->Deactivate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Deactivate( instance );
	}
}

Bool CBehaviorMimicLookAtConstraint::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( event.GetEventID() == instance[ i_longTransitionEvtTargetChanged ] )
	{
		instance[ i_longTransitionTargetChangedFlag ] = true;
		return true;
	}

	return false;
}

void CBehaviorMimicLookAtConstraint::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlEyesDataNode )
	{
		m_cachedControlEyesDataNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorMimicLookAtConstraint::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( instance[ i_weight ] < 0.01f ) return;

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	// Mark target position
	const Vector targetMS = instance[ i_target ];
	const Int32 eyeLeftPlacer = instance[ i_eyeLeftPlacer ];
	const Int32 eyeRightPlacer = instance[ i_eyeRightPlacer ];

	const Matrix& localToWorld = ac->GetLocalToWorld();
	const Vector targetPosWS = localToWorld.TransformPoint( targetMS );

	frame->AddDebugSphere( targetPosWS, 0.1f, Matrix::IDENTITY, Color::YELLOW, false);
	frame->AddDebugSphere( targetPosWS, 0.01f, Matrix::IDENTITY, Color::YELLOW, false);

	// Placers
	Matrix placerLeftWSMat = ac->GetBoneMatrixWorldSpace( eyeLeftPlacer );
	Matrix placerRightWSMat = ac->GetBoneMatrixWorldSpace( eyeRightPlacer );

	Vector placerLeftWS = placerLeftWSMat.GetTranslation();
	Vector placerRightWS = placerRightWSMat.GetTranslation();

	frame->AddDebugLine( placerLeftWS, targetPosWS, Color::LIGHT_YELLOW );
	frame->AddDebugLine( placerRightWS, targetPosWS, Color::LIGHT_YELLOW );

	frame->AddDebugAxis( placerLeftWS, placerLeftWSMat, 0.05f, true );
	frame->AddDebugAxis( placerRightWS, placerRightWSMat, 0.05f, true );

	Matrix placerChildLeftWS = FindChildBoneMatrixWS( instance, eyeLeftPlacer );
	Matrix placerChildRightWS = FindChildBoneMatrixWS( instance, eyeRightPlacer );

	frame->AddDebugAxis( placerChildLeftWS.GetTranslation(), placerChildLeftWS, 0.1f, true );
	frame->AddDebugAxis( placerChildRightWS.GetTranslation(), placerChildRightWS, 0.1f, true );

	{
		Float longTransitionBlinkAnimTime = instance[ i_longTransitionBlinkAnimTime ];
		Int32 longTransitionMode = instance[ i_longTransitionMode ];
		if ( longTransitionMode > LTS_None || longTransitionBlinkAnimTime > 0.f )
		{
			Vector longTransitionCachedTarget = instance[ i_longTransitionCachedTarget ];
			Float longTransitionBlinkTimer = instance[ i_longTransitionBlinkTimer ];

			Float duration( 0.5f );
			CSkeletalAnimationSetEntry* longTransitionBlinkAnim = instance[ i_longTransitionBlinkAnim ];
			if ( longTransitionBlinkAnim )
			{
				duration = longTransitionBlinkAnim->GetDuration();
			}

			String msg = String::Printf( TXT("TM [%d], timer [%1.2f/%1.2f], anim [%1.2f/%1.2f]"), longTransitionMode, longTransitionBlinkTimer, longTransitionBlinkTimer/m_blinkTimeOffset, longTransitionBlinkAnimTime, longTransitionBlinkAnimTime/duration );

			const Vector msgPos = Vector::Interpolate( placerLeftWS, placerRightWS, 0.5f ) + Vector( 0.f, 0.f, 0.2f );
			frame->AddDebugText( msgPos, msg, true );

			frame->AddDebugSphere( localToWorld.TransformPoint( longTransitionCachedTarget ), 0.1f, Matrix::IDENTITY, Color::RED, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorMimicHeadConstraint );

CBehaviorMimicHeadConstraint::CBehaviorMimicHeadConstraint()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorMimicHeadConstraint::RebuildSockets( CBehaviorGraphNode* parent )
{
	parent->CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( HeadWeight ) ) );
}

#endif

void CBehaviorMimicHeadConstraint::CacheConnections( CBehaviorGraphNode* parent )
{
	// Cache connections
	m_cachedControlVariableNode = parent->CacheValueBlock( TXT("HeadWeight") );
}

void CBehaviorMimicHeadConstraint::BuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_weight;
	compiler << i_headTrack;
}

void CBehaviorMimicHeadConstraint::InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const
{
	InternalReset( instance );

	const CSkeleton* mimicSkeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();

	if ( mimicSkeleton )
	{
		instance[ i_headTrack ] = parent->FindTrackIndex( m_headTrack, mimicSkeleton );
	}
	else
	{
		instance[ i_headTrack ] = -1;
	}
}

void CBehaviorMimicHeadConstraint::InternalReset( CBehaviorGraphInstance& instance )const
{
	instance[ i_weight ] = 0.f;
}

void CBehaviorMimicHeadConstraint::Reset( CBehaviorGraphInstance& instance ) const
{
	InternalReset( instance );
}

void CBehaviorMimicHeadConstraint::Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicHeadConstraint );
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
		instance[ i_weight ] = Clamp( m_cachedControlVariableNode->GetValue( instance ), 0.f, 1.f );
	}
}

void CBehaviorMimicHeadConstraint::PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const
{	
	const Float weight = instance[ i_weight ];

	if ( !context.HasMimic() || weight < 0.01f )
	{
		return;
	}

	const Int32 headTrackIndex = instance[ i_headTrack ];

	if ( headTrackIndex != -1 && headTrackIndex < (Int32)mimicPose.m_numFloatTracks )
	{
		mimicPose.m_floatTracks[ headTrackIndex ] *= weight;
	}
}

void CBehaviorMimicHeadConstraint::Activate( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorMimicHeadConstraint::Deactivate( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorMimicHeadConstraint::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorMimiLipsyncCorrectionConstraint );

CBehaviorMimiLipsyncCorrectionConstraint::CBehaviorMimiLipsyncCorrectionConstraint()
	: m_controlTrack( -1 )
	, m_trackBegin( -1 )
	, m_trackEnd( -1 )
{

}

void CBehaviorMimiLipsyncCorrectionConstraint::PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const
{
	/*const Int32 trackNum = (Int32)mimicPose.m_numFloatTracks;

	if ( m_controlTrack != -1 && m_controlTrack < trackNum &&
		m_trackBegin != -1 && m_trackBegin < trackNum &&
		m_trackEnd != -1 && m_trackEnd < trackNum )
	{
		const Float weight = 1.f - mimicPose.m_floatTracks[ m_controlTrack ];

		for ( Int32 i=0; i<m_trackBegin; ++i )
		{
			mimicPose.m_floatTracks[ i ] *= weight;
		}

		for ( Int32 i=m_trackEnd+1; i<trackNum; ++i )
		{
			mimicPose.m_floatTracks[ i ] *= weight;
		}
	}*/
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsBoneConverterNode );

void CBehaviorGraphMimicsBoneConverterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	// TODO - reset without neck and head

	/*if ( const CMimicComponent* head = Cast< const CMimicComponent >( instance.GetAnimatedComponent() ) )
	{
		if ( const CMimicFace* face = head->GetMimicFace() )
		{
			Int32 headIdx = -1;
			Int32 neckIdx = -1;

			face->GetNeckAndHead( neckIdx, headIdx );

			const Int32 numBones = (Int32)pose.m_numBones;

			if ( headIdx != -1 && neckIdx != -1 && headIdx < numBones && neckIdx < numBones )
			{
				mimicData->m_mimicBones[ 0 ] = pose.m_outputPose[ neckIdx ];
				mimicData->m_mimicBones[ 1 ] = pose.m_outputPose[ headIdx ];
			}
		}
	}*/

	if ( context.ShouldCorrectPose() )
	{
		context.SetPoseCorrectionIdentity( output );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorMimicCloseEyesConstraint );

CBehaviorMimicCloseEyesConstraint::CBehaviorMimicCloseEyesConstraint()
	: m_eyeClosedTrack_Left( 82 )
	, m_eyeClosedTrack_Right( 81 )
{

}

void CBehaviorMimicCloseEyesConstraint::BuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_bonesToOverride_Left;
	compiler << i_bonesToOverride_Right;
}

void CBehaviorMimicCloseEyesConstraint::InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const
{
	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton() )
	{
		TDynArray< Int32 >& bonesToOverride_Left = instance[ i_bonesToOverride_Left ];
		TDynArray< Int32 >& bonesToOverride_Right = instance[ i_bonesToOverride_Right ];

		const Uint32 numLeft = m_bonesToOverride_Left.Size();
		bonesToOverride_Left.Reserve( numLeft );
		for ( Uint32 i=0; i<numLeft; ++i )
		{
			const Int32 boneIdx = parent->FindBoneIndex( m_bonesToOverride_Left[ i ], skeleton );
			if ( boneIdx != -1 )
			{
				bonesToOverride_Left.PushBack( boneIdx );
			}
		}

		const Uint32 numRight = m_bonesToOverride_Right.Size();
		bonesToOverride_Right.Reserve( numRight );
		for ( Uint32 i=0; i<numRight; ++i )
		{
			const Int32 boneIdx = parent->FindBoneIndex( m_bonesToOverride_Right[ i ], skeleton );
			if ( boneIdx != -1 )
			{
				bonesToOverride_Right.PushBack( boneIdx );
			}
		}
	}
}

void CBehaviorMimicCloseEyesConstraint::OverrideBones( const CMimicFace* face, SBehaviorGraphOutput& pose, Float controlValue, Int32 eyeClosedTrack, const TDynArray< Int32 >& bonesToOverride, SBehaviorGraphOutput &mimicPose ) const
{
	const Int32 numMimicPose = (Int32)mimicPose.m_numBones;
	const Uint32 poseIndex = (Uint32)eyeClosedTrack + 0;

	SBehaviorGraphOutput outPose;
	const Bool ret = face->GetMimicPose( poseIndex, outPose );
	if ( ret )
	{
		face->AddMimicPose( pose, outPose, controlValue );

		const Uint32 numBones = bonesToOverride.Size();
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIdx = bonesToOverride[ i ];
			ASSERT( boneIdx != -1 );

			if ( boneIdx != -1 && boneIdx < numMimicPose )
			{
				ASSERT( boneIdx < (Int32)pose.m_numBones );
				mimicPose.m_outputPose[ boneIdx ].Lerp( mimicPose.m_outputPose[ boneIdx ], pose.m_outputPose[ boneIdx ], controlValue );
			}
		}
	}
}

void CBehaviorMimicCloseEyesConstraint::PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const
{	
	if ( !context.HasMimic() || 
		m_eyeClosedTrack_Left == -1 || m_eyeClosedTrack_Right == -1 || 
		m_eyeClosedTrack_Left >= (Int32)floatTrackPose.m_numFloatTracks || m_eyeClosedTrack_Right >= (Int32)floatTrackPose.m_numFloatTracks )
	{
		return;
	}

	const CMimicComponent* head = SafeCast< const CMimicComponent >( instance.GetAnimatedComponent() );
	const CMimicFace* face = head->GetMimicFace();
	if ( face )
	{
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* pose = cachePose.GetPose();

		const TDynArray< Int32 >& bonesToOverride_Left = instance[ i_bonesToOverride_Left ];
		const TDynArray< Int32 >& bonesToOverride_Right = instance[ i_bonesToOverride_Right ];

		const Float controlValueL = floatTrackPose.m_floatTracks[ m_eyeClosedTrack_Left ];
		const Float controlValueR = floatTrackPose.m_floatTracks[ m_eyeClosedTrack_Right ];

		if ( MAbs( controlValueL ) > CBehaviorGraphMimicsConverterNode::POSE_THRESHOLD )
		{
			OverrideBones( face, *pose, controlValueL, m_eyeClosedTrack_Left, bonesToOverride_Left, mimicPose );
		}

		if ( MAbs( controlValueR ) > CBehaviorGraphMimicsConverterNode::POSE_THRESHOLD )
		{
			OverrideBones( face, *pose, controlValueR, m_eyeClosedTrack_Right, bonesToOverride_Right, mimicPose );
		}
	}
}

#ifndef NO_EDITOR

void CBehaviorMimicCloseEyesConstraint::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	m_bonesToOverride_Right.PushBack( TXT("lowwer_right_eyelid1") );
	m_bonesToOverride_Right.PushBack( TXT("lowwer_right_eyelid2") );
	m_bonesToOverride_Right.PushBack( TXT("lowwer_right_eyelid3") );
	m_bonesToOverride_Right.PushBack( TXT("upper_right_eyelid_fold") );

	m_bonesToOverride_Right.PushBack( TXT("upper_right_eyelid1") );
	m_bonesToOverride_Right.PushBack( TXT("upper_right_eyelid2") );
	m_bonesToOverride_Right.PushBack( TXT("upper_right_eyelid3") );
	m_bonesToOverride_Right.PushBack( TXT("lowwer_right_eyelid_fold") );

	m_bonesToOverride_Right.PushBack( TXT("upper_right_eyelash") );

	m_bonesToOverride_Left.PushBack( TXT("lowwer_left_eyelid1") );
	m_bonesToOverride_Left.PushBack( TXT("lowwer_left_eyelid2") );
	m_bonesToOverride_Left.PushBack( TXT("lowwer_left_eyelid3") );
	m_bonesToOverride_Left.PushBack( TXT("lowwer_left_eyelid_fold") );

	m_bonesToOverride_Left.PushBack( TXT("upper_left_eyelid1") );
	m_bonesToOverride_Left.PushBack( TXT("upper_left_eyelid2") );
	m_bonesToOverride_Left.PushBack( TXT("upper_left_eyelid3") );
	m_bonesToOverride_Left.PushBack( TXT("upper_left_eyelid_fold") );

	m_bonesToOverride_Left.PushBack( TXT("upper_left_eyelash") );
}

#endif

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( mimika );
RED_DEFINE_STATIC_NAME( eyesLookAt_isEnabled );
RED_DEFINE_STATIC_NAME( eyesLookAt_targetA );
RED_DEFINE_STATIC_NAME( eyesLookAt_targetB );
RED_DEFINE_STATIC_NAME( eyesLookAt_progress );
RED_DEFINE_STATIC_NAME( eyesLookAt_weight );
RED_DEFINE_STATIC_NAME( eyesLookAt_targetChanged );

const CName& CBehaviorGraphMimicsLookAtMediator::VAR_ISENABLED = CNAME( eyesLookAt_isEnabled );
const CName& CBehaviorGraphMimicsLookAtMediator::VAR_TARGET_A = CNAME( eyesLookAt_targetA );
const CName& CBehaviorGraphMimicsLookAtMediator::VAR_TARGET_B = CNAME( eyesLookAt_targetB );
const CName& CBehaviorGraphMimicsLookAtMediator::VAR_PROGRESS = CNAME( eyesLookAt_progress );
const CName& CBehaviorGraphMimicsLookAtMediator::VAR_WEIGHT = CNAME( eyesLookAt_weight );
const CName& CBehaviorGraphMimicsLookAtMediator::EVT_TARGET_CHANGED = CNAME( eyesLookAt_targetChanged );

CBehaviorGraphMimicsLookAtMediator::CBehaviorGraphMimicsLookAtMediator()
{

}

CBehaviorGraphMimicsLookAtMediator::CBehaviorGraphMimicsLookAtMediator( CEntity* e )
{
	Init( e );
}

CBehaviorGraphMimicsLookAtMediator::CBehaviorGraphMimicsLookAtMediator( CBehaviorGraphInstance& instance )
{
	m_instanceH = &instance;
}

Bool CBehaviorGraphMimicsLookAtMediator::Init( CEntity* e )
{
	if ( IActorInterface* a = e->QueryActorInterface() )
	{
		if ( CMimicComponent* m = a->GetMimicComponent() )
		{
			if ( CBehaviorGraphStack* s = m->GetBehaviorStack() )
			{
				m_instanceH = s->GetBehaviorGraphInstance( CNAME( mimika ) );
			}
		}
	}

	return IsValid();
}

void CBehaviorGraphMimicsLookAtMediator::Deinit()
{
	m_instanceH = nullptr;
}

Bool CBehaviorGraphMimicsLookAtMediator::IsValid() const
{
	CBehaviorGraphInstance* instance = m_instanceH.Get();
	return instance != nullptr && instance->IsBinded();
}

void CBehaviorGraphMimicsLookAtMediator::SetEnabled( Bool f )
{
	ASSERT( IsValid() );
	if ( IsValid() )
	{
		const Float var = f ? 1.f : 0.f;
		m_instanceH->SetFloatValue( VAR_ISENABLED, var );
	}
}

Bool CBehaviorGraphMimicsLookAtMediator::IsEnabled() const
{
	Float var( 0.f );

	ASSERT( IsValid() );
	if ( IsValid() )
	{
		var = m_instanceH->GetFloatValue( VAR_ISENABLED );
	}

	return var > 0.f;
}

void CBehaviorGraphMimicsLookAtMediator::SetData( Float weight, const Vector& pointA, const Vector& pointB, Float progress )
{
	ASSERT( IsValid() );
	if ( IsValid() )
	{
		CBehaviorGraphInstance* instance = m_instanceH.Get();

		instance->SetVectorValue( VAR_TARGET_A, pointA );
		instance->SetVectorValue( VAR_TARGET_B, pointB );
		instance->SetFloatValue( VAR_PROGRESS, progress );
		instance->SetFloatValue( VAR_WEIGHT, weight );
	}
}

void CBehaviorGraphMimicsLookAtMediator::GetData( Float& weight, Vector& pointA, Vector& pointB, Float& progress ) const
{
	ASSERT( IsValid() );
	if ( IsValid() )
	{
		CBehaviorGraphInstance* instance = m_instanceH.Get();

		pointA = instance->GetVectorValue( VAR_TARGET_A );
		pointB = instance->GetVectorValue( VAR_TARGET_B );
		progress = instance->GetFloatValue( VAR_PROGRESS );
		weight = instance->GetFloatValue( VAR_WEIGHT );
	}
	else
	{
		pointA = Vector::ZERO_3D_POINT;
		pointB = Vector::ZERO_3D_POINT;
		progress = 0.f;
		weight = 0.f;
	}
}

Bool CBehaviorGraphMimicsLookAtMediator::NotifyTargetWasChanged()
{
	Bool ret( false );

	ASSERT( IsValid() );
	if ( IsValid() )
	{
		ret = m_instanceH->GenerateEvent( EVT_TARGET_CHANGED );
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
