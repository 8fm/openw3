
#include "build.h"
#include "animFriendPagePointCloudLookAt.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "../../common/engine/animationTrajectoryTrackParam.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "../../common/core/feedback.h"
#include "../../common/engine/skeleton.h"

//#pragma optimize("",off)

Bool CEdAnimPointCloudLookAtParamInitializer::Initialize( ISkeletalAnimationSetEntryParam* _param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const
{
	CAnimPointCloudLookAtParam* param = Cast< CAnimPointCloudLookAtParam >( _param );
	if ( param )
	{
		String boneNameStr( TXT("head") );

		if ( !InputBox( NULL, TXT("Bone name"), TXT("Write bone name"), boneNameStr ) )
		{
			return false;
		}

		const Int32 boneIndex = animatedComponent->FindBoneByName( boneNameStr.AsChar() );
		if ( boneIndex == -1 )
		{
			return false;
		}

		// Select axis
		Vector direction( 0.f, 1.f, 0.f );
		String directionStr = ToString( direction );
		if ( !InputBox( NULL, TXT("Bone direction"), TXT("Write bone direction in local space"), directionStr ) )
		{
			return false;
		}

		if ( !FromString( directionStr, direction ) )
		{
			GFeedback->ShowError( TXT("Can not parse your text. You wrote wrong string.") );
			return false;
		}
		direction.Normalize3();

		const Bool additiveMode = GFeedback->AskYesNo( TXT("Do you want to use point cloud in additive mode?\n(First pose in animation has to be reference pose and this pose will be droped by point cloud.)") );


		// TODO - ask
		Bool autoGenerate = true;
		//...

		CAnimPointCloudLookAtParam::InitData init;
		init.m_boneName = CName( boneNameStr );
		init.m_directionLS = direction;

		if ( autoGenerate )
		{
			AnimPointCloud::Generators::Grid::Input input;
			input.m_directionLS = init.m_directionLS;

			// Prepare timers for sampling
			Float time = 0.f;
			const Float timeDelta = 1.f;
			const Float duration = animation->GetDuration() + 0.001f;

			// Initial bone transform
			Matrix initialBoneMS( Matrix::IDENTITY );
			if ( !additiveMode )
			{
				initialBoneMS = animatedComponent->GetSkeleton()->GetBoneMatrixMS( boneIndex );
			}

			// Sample
			const CSkeletalAnimationTrajectoryTrackParam* trajectory = animation->FindParam< CSkeletalAnimationTrajectoryTrackParam >();
			if ( trajectory )
			{
				// Sample trajectory data
				if ( trajectory->m_datas.Size() > 0 )
				{
					Bool firstFrame = true;
					initialBoneMS = animatedComponent->GetSkeleton()->GetBoneMatrixMS( boneIndex );

					SBehaviorGraphOutput pose;
					pose.Init( animation->GetAnimation()->GetBonesNum(), animation->GetAnimation()->GetTracksNum() );

					const TDynArray< Vector >& data = trajectory->m_datas[ 0 ];
					const Int32 dataSize = data.SizeInt();

					for ( ; time <= duration; time += timeDelta )
					{
						// Animation
						animation->GetAnimation()->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

						if ( animatedComponent->UseExtractedTrajectory() )
						{
							pose.ExtractTrajectory( animatedComponent );
						}

						Matrix boneMS = pose.GetBoneModelMatrix( animatedComponent, boneIndex );

						if ( additiveMode && firstFrame )
						{
							init.m_refPose.Resize( pose.m_numBones );
							for ( Uint32 j=0; j<init.m_refPose.Size(); ++j )
							{
								init.m_refPose[ j ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( pose.m_outputPose[ j ] );
							}

							initialBoneMS = boneMS;

							firstFrame = false;
						}
						else
						{
							// Trajectory
							const Uint32 dataIndex = (Uint32)( ( time / duration ) * (Float)(dataSize-1) );
							const Vector& trajVec = data[ dataIndex ];

							Vector dirToTrajPoint = trajVec - boneMS.GetTranslation();
							dirToTrajPoint.Normalize3();

							// Modify bone matrix
							{
								AnimQsTransform boneA = MatrixToAnimQsTransform( boneMS );
								
								AnimVector4 vecA;
								vecA.RotateDirection( boneA.Rotation, VectorToAnimVector( direction ) );
								AnimVector4 vecB;
								vecB = VectorToAnimVector( dirToTrajPoint );

								AnimQuaternion quat;
								quat.SetShortestRotation( vecA, vecB );

								AnimQsTransform boneB( boneA );
								boneB.Rotation.SetMul( quat, boneA.Rotation );

								boneMS = AnimQsTransformToMatrix( boneB );
							}

							input.m_boneTransformsMS.PushBack( boneMS );
						}
					}
				}
			}
			else
			{
				// Sample animation data
				SBehaviorGraphOutput pose;
				pose.Init( animation->GetAnimation()->GetBonesNum(), animation->GetAnimation()->GetTracksNum() );

				Bool firstFrame = true;

				for ( ; time <= duration; time += timeDelta )
				{
					animation->GetAnimation()->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

					if ( animatedComponent->UseExtractedTrajectory() )
					{
						pose.ExtractTrajectory( animatedComponent );
					}

					const Matrix boneMS = pose.GetBoneModelMatrix( animatedComponent, boneIndex );

					if ( additiveMode && firstFrame )
					{
						init.m_refPose.Resize( pose.m_numBones );
						for ( Uint32 j=0; j<init.m_refPose.Size(); ++j )
						{
							init.m_refPose[ j ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( pose.m_outputPose[ j ] );
						}

						initialBoneMS = boneMS;

						firstFrame = false;
					}
					else
					{
						input.m_boneTransformsMS.PushBack( boneMS );
					}
				}
			}

			AnimPointCloud::Generators::Grid::OutData out;
			AnimPointCloud::Generators::Grid::Generate( input, out );

			init.m_pointsBS = out.m_pointsBS;
			init.m_tris = out.m_tris;
			init.m_pointToTriMapping = out.m_pointToTriMapping;

			initialBoneMS.SetTranslation( Vector::ZERO_3D_POINT );
			init.m_boneMS = initialBoneMS;
		}

		param->Init( init );

		static Bool removeSourceParam = false;
		if ( removeSourceParam )
		{
			const_cast< CSkeletalAnimationSetEntry* >( animation )->RemoveAllParamsByClass< CSkeletalAnimationTrajectoryTrackParam >();
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdAnimationPointCloudLookAtPlayerPage, CEdAnimationFriendSimplePage )
END_EVENT_TABLE()

CEdAnimationPointCloudLookAtPlayerPage::CEdAnimationPointCloudLookAtPlayerPage( CEdAnimationFriend* owner )
	: CEdAnimationFriendSimplePage( owner )
	, m_selectedAnimation( NULL )
	, m_cachedBoneIndex( -1 )
	, m_targetWS( 0.f, 2.f, 1.8f )
	, m_targetMode( TARGET_AUTO )
	, m_lastDt( 0.f )
	, m_lastPos( 2.f, 0.f, 0.f )
	, m_speed( Vector::ZEROS )
	, m_speedAng( 1.f, 0.8f, 1.f, 1.f )
	, m_lastPosOffset( 0.f, 0.f, 1.f )
{
	wxPanel* panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	{
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_paramPanel = new CEdAnimationParamPanel( panel, ClassID< CAnimPointCloudLookAtParam >(), new CEdAnimPointCloudLookAtParamInitializer(), this );

		sizer->Add( m_paramPanel, 1, wxEXPAND|wxALL, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( panel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	m_paramPanel->CloneAndUseAnimatedComponent( GetAnimatedComponent() );
}

CEdAnimationPointCloudLookAtPlayerPage::~CEdAnimationPointCloudLookAtPlayerPage()
{
	
}

wxAuiPaneInfo CEdAnimationPointCloudLookAtPlayerPage::GetPageInfo() const
{
	wxAuiPaneInfo info;
	info.Dockable( true ).Right().Row( 2 ).Layer( 2 ).MinSize( wxSize( 200, 100 ) ).CloseButton( false ).Name( wxT("CEdAnimationPointCloudLookAtPlayerPage") );
	return info;
}

void CEdAnimationPointCloudLookAtPlayerPage::OnTick( Float dt )
{
	m_lastDt = dt;

	if ( m_targetMode == TARGET_AUTO )
	{
		m_lastPos.Y += dt * m_speedAng.X;
		m_lastPos.Z += dt * m_speedAng.Y;

		AnimVector4 pos( m_lastPos );
		BehaviorUtils::CartesianFromSpherical( pos );

		m_targetWS = AnimVectorToVector( pos ) + m_lastPosOffset;
	}
	else if ( m_targetMode == TARGET_MANUAL )
	{
		m_targetWS += m_speed * dt;
	}
}

void CEdAnimationPointCloudLookAtPlayerPage::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( key == IK_Space && action == IACT_Press )
	{
		m_targetMode += 1;

		if ( m_targetMode == TARGET_LAST )
		{
			m_targetMode = TARGET_FIRST;
		}
	}
	else if ( key == IK_NumPad8 )
	{
		m_speed.Y = -data;
	}
	else if ( key == IK_NumPad2 )
	{
		m_speed.Y = data;
	}
	else if ( key == IK_NumPad4 )
	{
		m_speed.X = data;
	}
	else if ( key == IK_NumPad6 )
	{
		m_speed.X = -data;
	}
	else if ( key == IK_NumPad7 )
	{
		m_speed.Z = data;
	}
	else if ( key == IK_NumPad9 )
	{
		m_speed.Z = -data;
	}
};

void CEdAnimationPointCloudLookAtPlayerPage::OnLoadPreviewEntity( CAnimatedComponent* component )
{
	CacheBoneIndex();
}

void CEdAnimationPointCloudLookAtPlayerPage::OnUnloadPreviewEntity()
{
	CacheBoneIndex();
}

void CEdAnimationPointCloudLookAtPlayerPage::OnAnimationParamSelectedAnimation( const CSkeletalAnimationSetEntry* animation )
{
	m_selectedAnimation = animation;

	CacheBoneIndex();

	PlayAnimation( animation );
	PauseAnimation();
}

void CEdAnimationPointCloudLookAtPlayerPage::OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param )
{
	ASSERT( m_selectedAnimation == animation );

	CacheBoneIndex();
}

void CEdAnimationPointCloudLookAtPlayerPage::OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param )
{
	ASSERT( m_selectedAnimation == animation );

	CacheBoneIndex();
}

void CEdAnimationPointCloudLookAtPlayerPage::OnGenerateFragments( CRenderFrame *frame )
{
	if ( m_cachedBoneIndex != -1 )
	{
		const CAnimPointCloudLookAtParam* param = m_selectedAnimation->FindParam< CAnimPointCloudLookAtParam >();
		const CAnimatedComponent* ac = GetAnimatedComponent();
		
		ASSERT( m_selectedAnimation );
		ASSERT( param );
		ASSERT( ac );

		const Matrix mat = ac->GetBoneMatrixWorldSpace( (Uint32)m_cachedBoneIndex );

		{
			const Vector direction = param->GetDirectionLS();

			const Vector start = mat.GetTranslation();
			const Vector end = mat.TransformVector( direction );

			frame->AddDebugLine( start, start+end, Color( 255, 0, 0 ), false );
		}

		{
			AnimPointCloud::Render::DrawPointCloud( frame, mat, param );
		}

		frame->AddDebugSphere( m_targetWS, 0.11f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
		frame->AddDebugSphere( m_targetWS, 0.011f, Matrix::IDENTITY, Color( 255, 0, 0 ) );

		{
			Int32 y = 25;
			const Int32 lineStepY = 25;

			{
				if ( m_targetMode == TARGET_AUTO )
				{
					frame->AddDebugScreenFormatedText( 30, y, TXT("Target mode: Auto") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("Inputs:") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     Space - change mode") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad+ - speed up") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad- - speed down") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad/ - inc radius") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad* - dec radius") );
				}
				else if ( m_targetMode == TARGET_MANUAL )
				{
					frame->AddDebugScreenFormatedText( 30, y, TXT("Target mode: Manual") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("Inputs:") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     Space - change mode") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad4 - left (-X)") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad6 - right (+X)") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad8 - backward (-Y)") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad2 - forward (+Y)") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad7 - down (-Z)") );
					y += lineStepY;
					frame->AddDebugScreenFormatedText( 30, y, TXT("     NumPad9 - up (+Z)") );
				}
			}

			//...
		}

		{
			const CSkeletalAnimationTrajectoryTrackParam* trajectory = m_selectedAnimation->FindParam< CSkeletalAnimationTrajectoryTrackParam >();
			if ( trajectory && trajectory->m_datas.Size() > 0 )
			{
				const Float duration = m_selectedAnimation->GetDuration();
				const Float timeDelta = 1.f;
				const TDynArray< Vector >& data = trajectory->m_datas[ 0 ];
				const Int32 dataSize = data.SizeInt();

				for ( Float time = 0.f; time <= duration; time += timeDelta )
				{
					const Uint32 dataIndex = (Uint32)( ( time / duration ) * (Float)(dataSize-1) );
					const Vector& trajVec = data[ dataIndex ];

					frame->AddDebugSphere( trajVec, 0.02523f, Matrix::IDENTITY, Color( 128, 128, 128 ) );
				}	
			}
		}
	}
}

void CEdAnimationPointCloudLookAtPlayerPage::CacheBoneIndex()
{
	m_cachedBoneIndex = -1;

	if ( m_selectedAnimation )
	{
		const CAnimPointCloudLookAtParam* param = m_selectedAnimation->FindParam< CAnimPointCloudLookAtParam >();
		const CAnimatedComponent* ac = GetAnimatedComponent();
		if ( param && ac )
		{
			CName boneName = param->GetBoneName();

			m_cachedBoneIndex = ac->FindBoneByName( boneName );
		}
	}
}

//#pragma optimize("",on)
