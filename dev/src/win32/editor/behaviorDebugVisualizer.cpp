/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behaviorEditor.h"
#include "behaviorDebugVisualizer.h"
#include "../../common/engine/behaviorGraphAnimationNode.h"
#include "../../common/engine/behaviorGraphContext.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/tickManager.h"
#include "../../common/engine/skeleton.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorDebugVisualizer );

CBehaviorDebugVisualizer::CBehaviorDebugVisualizer()
	: m_behaviorEditor( NULL )
	, m_animatedComponent( NULL )
	, m_nodeToSample( NULL )
	, m_color( 255, 255, 0 )
	, m_showAxis( false )
	, m_showNames( false )
	, m_showBox( false )
	, m_boneStyle( BVBS_Line )
	, m_time( -1.f )
	, m_pose( NULL )
	, m_dispTime( -1.f )
	, m_dispProgress( -1.f )
	, m_collectData( false )
{
	m_pose = new SBehaviorGraphOutput();
}

CBehaviorDebugVisualizer::~CBehaviorDebugVisualizer()
{
	if ( m_pose )
	{
		m_pose->Deinit();
		delete m_pose;
		m_pose = NULL;
	}
}

void CBehaviorDebugVisualizer::OnAttached( CWorld *world )
{
	TBaseClass::OnAttached( world );

	world->GetTickManager()->AddToGroup( this, TICK_Main );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
}

void CBehaviorDebugVisualizer::OnDetached( CWorld *world )
{
	TBaseClass::OnDetached( world );

	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
}

void CBehaviorDebugVisualizer::OnUpdateBounds()
{
	m_boundingBox = Box( Vector::ZERO_3D_POINT, 100.0f );
}

void CBehaviorDebugVisualizer::OnTick( Float timeDelta )
{
	SetPosition( m_animatedComponent->GetPosition() );
}

void CBehaviorDebugVisualizer::DisplayPose( const SBehaviorGraphOutput& pose, CRenderFrame* frame ) const
{
	TDynArray< Matrix > bonesMS;
	//dex++: switched to general CSkeleton interface
	const Uint32 numBones = m_animatedComponent->GetSkeleton()->GetBonesNum();
	bonesMS.Resize( numBones );
	//dex-
	pose.GetBonesModelSpace( m_animatedComponent, bonesMS );

	Int32 trajectoryBoneIndex = m_animatedComponent->GetTrajectoryBone();

	if ( m_boneStyle == BVBS_Line )
	{
		TDynArray< DebugVertex > skeletonPoints;

		// Draw bones
		for( Uint32 index=0; index<m_boneIndex.Size(); ++index )
		{
			Int32 i = m_boneIndex[index];
			//dex++: switched to general CSkeleton interface
			const Int32 parentIndex = m_animatedComponent->GetSkeleton()->GetParentBoneIndex(i);
			//dex--

			Matrix rot = bonesMS[i] * m_animatedComponent->GetLocalToWorld();

			if ( i == trajectoryBoneIndex )
			{
				frame->AddDebugAxis( rot.GetTranslation(), rot, 0.75f );
				continue;
			}

			Vector parentPosition = Vector::ZEROS;

			if ( parentIndex >= 0 )
			{
				parentPosition = bonesMS[ parentIndex ].GetTranslation();
			}

			Vector position = bonesMS[ i ].GetTranslation();

			skeletonPoints.PushBack( DebugVertex( parentPosition, m_color ) );
			skeletonPoints.PushBack( DebugVertex( position, m_color ) );
		}

		if ( skeletonPoints.Size()>0 )
		{
			new ( frame ) CRenderFragmentDebugLineList( frame, 
														m_animatedComponent->GetLocalToWorld(),
														&skeletonPoints[0],
														skeletonPoints.Size(),
														RSG_DebugOverlay );
		}
	}
	else if ( m_boneStyle == BVBS_3D )
	{
		// Draw bones
		for( Uint32 index=0; index<m_boneIndex.Size(); ++index )
		{
			Int32 i = m_boneIndex[index];
			//dex++: switched to general CSkeleton interface
			const Int32 parentIndex = m_animatedComponent->GetSkeleton()->GetParentBoneIndex(i);
			//dex--

			Matrix rot = bonesMS[i] * m_animatedComponent->GetLocalToWorld();

			if ( i == trajectoryBoneIndex )
			{
				frame->AddDebugAxis( rot.GetTranslation(), rot, 0.75f );
				continue;
			}

			if ( parentIndex < 0 )
				continue;

			Matrix parentRot = bonesMS[parentIndex] * m_animatedComponent->GetLocalToWorld();
			
			frame->AddDebugBone( rot, parentRot, m_color, true );
		}
	}

	// Draw helpers
	for( Uint32 index=0; index<m_boneHelpers.Size(); ++index )
	{
		Int32 i = m_boneHelpers[index];

		Matrix rot = bonesMS[i] * m_animatedComponent->GetLocalToWorld();

		if ( m_showAxis )
		{
			frame->AddDebugAxis( rot.GetTranslation(), rot, 0.20f, true );
		}
		if ( m_showNames )
		{
			//dex++: switched to general CSkeleton interface
			const String boneName = m_animatedComponent->GetSkeleton()->GetBoneName(i);
			frame->AddDebugText( rot.GetTranslation(), boneName );
			//dex--
		}
	}

// 	// Display pose bounding box
// 	if( m_showBox )
// 	{
// 		frame->AddDebugBox( pose.m_outputBox.m_box, m_animatedComponent->GetLocalToWorld(), m_color, false );
// 	}

	// Display time and progress
	if ( m_dispTime >= 0.f && m_dispProgress >= 0.f )
	{
		Uint32 num = GetVisualizerNum();

		String text = String::Printf( TXT("t:%.2f\np:%.2f"), m_dispTime, m_dispProgress );
		Float offset = 50.f * num;

		Float width = frame->GetFrameOverlayInfo().m_width;

		if ( offset + 10.f < width - 10.f )
		{
			frame->AddDebugRect( offset, frame->GetFrameOverlayInfo().m_height - 60, 50, 30, Color::BLACK );
			frame->AddDebugScreenText( offset + 10.f , frame->GetFrameOverlayInfo().m_height - 50, text, m_color );
		}
	}
}

Uint32 CBehaviorDebugVisualizer::GetVisualizerNum() const
{
	CEntity* entity = m_animatedComponent->GetEntity();
	TDynArray< CBehaviorDebugVisualizer* > visualizers;

	CollectEntityComponents( entity, visualizers );

	Uint32 num = visualizers.Size();

	for ( Uint32 i=0; i<num; ++i )
	{
		if ( visualizers[i] == this )
		{
			return i;
		}
	}

	ASSERT( 0 );
	return 0;
}

void CBehaviorDebugVisualizer::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{	
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	m_collectData = false;

	// Draw extra shit
	if ( m_nodeToSample && m_behaviorEditor )
	{
		CBehaviorGraphInstance& instance = *m_behaviorEditor->GetBehaviorGraphInstance();

		if ( !m_nodeToSample->IsActive( instance ) || !CanWork() )
			return;

		ASSERT( m_pose->IsValid() );

		ResetPose();

		// Sample node
		SBehaviorSampleContext* behaviorContext = m_animatedComponent->GetBehaviorGraphSampleContext();
		if ( !behaviorContext )
		{
			return;
		}

		behaviorContext->PrepareForSample();

		m_nodeToSample->Sample( *behaviorContext, instance, *m_pose );

		if( m_nodeToSample->IsA< CBehaviorGraphAnimationNode >() )
		{
			CBehaviorGraphAnimationNode* animNode = Cast< CBehaviorGraphAnimationNode >( m_nodeToSample );

			m_dispTime = animNode->GetAnimTime( instance );
			m_dispProgress = animNode->GetAnimProgress( instance );
		}
		else
		{
			m_dispTime = -1.f;
			m_dispProgress = -1.f;
		}

		behaviorContext->CompleteSample();

		DisplayPose( *m_pose, frame );

		m_collectData = true;
	}
	else if ( !m_animation.Empty() && m_time >= 0.f )
	{
		if ( !CanWork() ) return;

		ASSERT( m_pose->IsValid() );

		ResetPose();

		CSkeletalAnimationSetEntry* animEntry = m_animatedComponent->GetAnimationContainer()->FindAnimation( m_animation );
		if ( !animEntry || !animEntry->GetAnimation() )
		{
			return;
		}
		Float totalTime = animEntry->GetDuration();

		m_dispTime = m_time;
		m_dispProgress = totalTime > 0.f ? Clamp( m_time / totalTime, 0.f, 1.f ) : 0.f;

		if ( totalTime < m_time )
		{
			return;
		}

		// Sample animation
		//dex++: switched to generalized CSkeleton interface
		const Uint32 numBones = m_animatedComponent->GetSkeleton()->GetBonesNum();
		const Uint32 numTracks = m_animatedComponent->GetSkeleton()->GetTracksNum();
		animEntry->GetAnimation()->Sample( m_time, numBones, numTracks, m_pose->m_outputPose, m_pose->m_floatTracks );
		//dex--

		DisplayPose( *m_pose, frame );

		m_collectData = true;
	}
}

void CBehaviorDebugVisualizer::SetAnimatedComponent( CAnimatedComponent *component )
{
	m_animatedComponent = component;

	SelectDefaultBones();
	OnUpdateBounds();
}

Bool CBehaviorDebugVisualizer::CanWork() const
{
	if ( !m_animatedComponent )
	{
		return false;
	}

	if ( m_nodeToSample )
	{
		if ( m_nodeToSample->IsMimic() )
		{
			return m_animatedComponent->GetMimicSkeleton() && m_animatedComponent->GetMimicSkeleton()->IsValid();
		}
		else
		{
			return m_animatedComponent->GetSkeleton() && m_animatedComponent->GetSkeleton()->IsValid();
		}
	}

	return !m_animation.Empty();
}

void CBehaviorDebugVisualizer::ChooseSelectBones()
{
	static const String boneGroupRoot = TXT("IK_Root");
	static const String boneRoot = TXT("Root");

	//dex++: switched to generalized CSkeleton interface
	CEdBoneSelectionDialog dlg(m_behaviorEditor, m_animatedComponent->GetSkeleton());
	//dex--

	if (m_boneIndex.Empty())
	{
		dlg.DeselectBones(boneGroupRoot, true, false);		// for visual reason
		dlg.DeselectBones(boneRoot, false, true);
	}
	else
	{
		for (Uint32 i=0; i<m_boneIndex.Size()-1; i++)
		{
			//dex++: switched to general CSkeleton interface
			const String name = m_animatedComponent->GetSkeleton()->GetBoneName(m_boneIndex[i]);
			dlg.SelectBones( name, false, false);
			//dex--
		}

		//dex++: switched to general CSkeleton interface
		{
			const Uint32 lastIndex = m_boneIndex[ m_boneIndex.Size()-1 ];
			const String name = m_animatedComponent->GetSkeleton()->GetBoneName(lastIndex);
			dlg.SelectBones( name, false, true);
		}
		//dex--
	}

	dlg.DoModal();

	m_boneIndex.Clear();
	dlg.GetSelectedBones(m_boneIndex);
}

void CBehaviorDebugVisualizer::SelectDefaultBones()
{
	if (m_animatedComponent)
	{
		m_boneIndex.Clear();
		static String deselBone1 = TXT("IK_Root");
		//static String deselBone2 = TXT("Root");
		//dex++: switched to generalized CSkeleton interface
		const Uint32 numBones = m_animatedComponent->GetSkeleton()->GetBonesNum();
		for (Uint32 i=0; i<numBones; i++)
		{
			const Int32 parentIndex = m_animatedComponent->GetSkeleton()->GetParentBoneIndex(i);
			const String boneName = m_animatedComponent->GetSkeleton()->GetBoneName(i);
			//dex--
			
			if (parentIndex!=-1) 
			{
				//dex++
				const String parentBoneName = m_animatedComponent->GetSkeleton()->GetBoneName(parentIndex);
				//dex--
				if (parentBoneName == deselBone1)
				{
					continue;
				}
			}

			if (boneName != deselBone1 )
			{
				m_boneIndex.PushBack(i);
			}
		}
	}
}

void CBehaviorDebugVisualizer::ChooseBonesHelpers()
{
	m_showAxis = true;

	//dex++: switched to general CSkeleton interface
	CEdBoneSelectionDialog dlg( m_behaviorEditor, m_animatedComponent->GetSkeleton() );
	//dex--

	if (!m_boneHelpers.Empty())
	{
		for (Int32 i=0; i<Int32(m_boneHelpers.Size())-2; i++)
		{
			//dex++
			const Uint32 boneIndex = m_boneHelpers[i];
			const String name = m_animatedComponent->GetSkeleton()->GetBoneName(boneIndex);
			//dex--
			dlg.SelectBones( name, false, false);
		}

		//dex++
		{
			const Uint32 boneIndex = m_boneHelpers.Back();
			const String name = m_animatedComponent->GetSkeleton()->GetBoneName(boneIndex);
			dlg.SelectBones( name, false, true);
		}
		//dex--
	}

	dlg.DoModal();

	m_boneHelpers.Clear();
	dlg.GetSelectedBones(m_boneHelpers);
}

void CBehaviorDebugVisualizer::SetNodeOfInterest( CBehaviorGraphNode *node )
{
	ASSERT( m_time < 0.f && m_animation.Empty() );
	ASSERT( m_animatedComponent );

	if ( node )
	{
		// Change pose
		m_pose->Deinit();
		
		CSkeleton* skeleton = node->IsMimic() ? m_animatedComponent->GetMimicSkeleton() : m_animatedComponent->GetSkeleton();

		if ( skeleton )
		{
			m_pose->Init( skeleton->GetBonesNum(), skeleton->GetTracksNum() );

			if ( m_nodeToSample )
			{
				ResetPose();
			}
		}
	}

	m_nodeToSample = node;

	m_collectData = false;
}

void CBehaviorDebugVisualizer::SetAnimationAndTime( const CName& animation, Float time )
{
	ASSERT( !m_nodeToSample );

	m_animation = animation;
	m_time = time;

	if ( !m_pose->IsValid() )
	{
		CSkeleton* skeleton = m_animatedComponent->GetSkeleton();
		if ( skeleton )
		{
			m_pose->Init( skeleton->GetBonesNum(), skeleton->GetTracksNum() );
		}
	}

	m_collectData = false;
}

void CBehaviorDebugVisualizer::SetColor( const Color &color )
{
	m_color = color;
}

void CBehaviorDebugVisualizer::SetEditor( CEdBehaviorEditor *editor )
{
	m_behaviorEditor = editor;
}

void CBehaviorDebugVisualizer::GetPoseBones( TDynArray< Matrix >& bonesLS, 
											 TDynArray< Matrix >& bonesMS, 
											 TDynArray< Matrix >& bonesWS ) const
{
	if ( m_collectData && m_animatedComponent && m_pose->IsValid() )
	{
		// LS
		for ( Uint32 i=0; i<m_pose->m_numBones; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			Matrix mat;
			HavokTransformToMatrix_Renormalize( m_pose->m_outputPose[i], &mat );
#else
			RedMatrix4x4 conversionMatrix = m_pose->m_outputPose[i].ConvertToMatrixNormalized();
			Matrix mat( reinterpret_cast< const Matrix& >( conversionMatrix ) );
#endif
			bonesLS.PushBack( mat );
		}

		// MS
		bonesMS.Resize( bonesLS.Size() );
		m_pose->GetBonesModelSpace( m_animatedComponent, bonesMS );

		// WS
		bonesWS.Resize( bonesMS.Size() );

		Matrix localToWorld = m_animatedComponent->GetLocalToWorld();

		for ( Uint32 i=0; i<bonesMS.Size(); i++ )
		{
			bonesWS[i] = bonesMS[i] * localToWorld;
		}
	}
	else
	{
		ASSERT( m_pose->IsValid() );
	}
}

void CBehaviorDebugVisualizer::GetPoseTracks( TDynArray< Float >& tracks ) const
{
	if ( m_collectData && m_pose->IsValid() )
	{
		for ( Uint32 i=0; i<m_pose->m_numFloatTracks; ++i )
		{
			tracks.PushBack( m_pose->m_floatTracks[i] );
		}
	}
	else
	{
		ASSERT( m_pose->IsValid() );
	}
}

void CBehaviorDebugVisualizer::GetPoseCustomTracks( TDynArray< Float >& tracks ) const
{
	if ( m_collectData && m_pose->IsValid() )
	{
		for ( Uint32 i=0; i<SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS; ++i )
		{
			tracks.PushBack( m_pose->m_customFloatTracks[i] );
		}
	}
	else
	{
		ASSERT( m_pose->IsValid() );
	}
}

void CBehaviorDebugVisualizer::GetPoseEvents( TDynArray< String >& events ) const
{
	if ( m_collectData && m_pose->IsValid() )
	{
		for ( Uint32 i=0; i<m_pose->m_numEventsFired; ++i )
		{
			if ( m_pose->m_eventsFired[i].m_extEvent )
			{
				String name = String::Printf( TXT("%s [%3.2f]- %s"), 
					m_pose->m_eventsFired[i].m_extEvent->GetEventName().AsString().AsChar(), 
					m_pose->m_eventsFired[i].m_alpha,
					m_pose->m_eventsFired[i].m_extEvent->GetClass()->GetName().AsString().AsChar() ); 
				
				events.PushBack( name );
			}
			else
			{
				ASSERT( m_pose->m_eventsFired[i].m_extEvent );
			}
		}
	}
	else
	{
		ASSERT( m_pose->IsValid() );
	}
}

void CBehaviorDebugVisualizer::GetMotionEx( String& motionEx ) const
{
#ifdef USE_HAVOK_ANIMATION
	Vector trans = TO_CONST_VECTOR_REF( m_pose->m_deltaReferenceFrameLocal.getTranslation() );
	EulerAngles rot;
	HavokQuaternionToEulerAngles( m_pose->m_deltaReferenceFrameLocal.getRotation(), rot );
#else
	Vector trans = reinterpret_cast<const Vector& >( m_pose->m_deltaReferenceFrameLocal.GetTranslation() );
	
	RedMatrix4x4 conversionMatrix = BuildFromQuaternion( m_pose->m_deltaReferenceFrameLocal.GetRotation().Quat );
	RedEulerAngles temp = conversionMatrix.ToEulerAnglesFull();
	EulerAngles rot( reinterpret_cast< const EulerAngles& >( temp ) );
#endif
	motionEx = String::Printf( TXT("Trans: %s, Rot: %s"), ToString( trans ).AsChar(), ToString( rot ).AsChar() );
}

Bool CBehaviorDebugVisualizer::HasNodeOfInterest() const
{
	return m_nodeToSample != NULL;
}

Bool CBehaviorDebugVisualizer::HasAnimation() const
{
	return m_animation.Empty() == false;
}

void CBehaviorDebugVisualizer::ResetPose()
{
	ASSERT( m_nodeToSample );
	CSkeleton* skeleton = m_nodeToSample->IsMimic() ? m_animatedComponent->GetMimicSkeleton() : m_animatedComponent->GetSkeleton();
	m_pose->Reset( skeleton );
}
