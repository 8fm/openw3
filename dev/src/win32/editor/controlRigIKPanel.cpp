#include "build.h"
#include "controlRigIKPanel.h"
#include "controlRigPanel.h"
#include "..\..\common\game\storySceneEventPoseKey.h"
#include "..\..\common\engine\behaviorGraphStack.h"
#include "..\..\common\engine\behaviorGraphUtils.inl"
#include "..\..\common\engine\controlRig.h"
#include "..\..\common\engine\behaviorGraphContext.h"
#include "..\..\common\engine\behaviorGraphOutput.h"
#include "..\..\common\engine\controlRigPropertySet.h"
#include "..\..\common\engine\controlRigDefinition.h"
#include "..\..\common\engine\skeleton.h"
#include "..\..\common\engine\skeletalAnimationContainer.h"
#include "..\..\common\engine\animationEvent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

BEGIN_EVENT_TABLE( CEdControlRigIKPanel, wxPanel )
	EVT_BUTTON( XRCID( "ikHandL"), CEdControlRigIKPanel::OnHandL )
	EVT_BUTTON( XRCID( "ikHandR"), CEdControlRigIKPanel::OnHandR )
	EVT_SLIDER( XRCID( "ikWeightSlider"), CEdControlRigIKPanel::OnWeightSlider )
	EVT_TEXT_ENTER( XRCID( "ikWeightEdit" ), CEdControlRigIKPanel::OnWeightEdit )
END_EVENT_TABLE()

CEdControlRigIKPanel::CEdControlRigIKPanel( wxWindow* parent, CEdControlRigPanel* ed )
	: m_parentWin( ed )
	, m_running( false )
	, m_position( Vector::ZERO_3D_POINT )
	, m_isPoseCached( false )
	, m_rig( nullptr )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("IKTransformPanel") );

	m_weightSlider = XRCCTRL( *this, "ikWeightSlider", wxSlider );
	m_weightText = XRCCTRL( *this, "ikWeightEdit", wxTextCtrl );
	m_effectorText = XRCCTRL( *this, "ikTextBone", wxTextCtrl );
	SetWeight( 0.f, true );
	EnableUI( false );
}

CEdControlRigIKPanel::~CEdControlRigIKPanel()
{
	SCENE_ASSERT( !m_isPoseCached );

	if ( m_rig )
	{
		delete m_rig;
	}
}

void CEdControlRigIKPanel::OnActorSelectionChange( const CActor* e )
{
	//m_ikInterface.Clear();

	m_animPose.Deinit();
	m_ikPose.Deinit();
	m_isPoseCached = false;

	if ( m_rig )
	{
		delete m_rig;
		m_rig = nullptr;
	}

	if ( e )
	{
		if ( CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
		{
			if ( CBehaviorGraphStack* stack = ac->GetBehaviorStack() )
			{
				//stack->GetInterface< CBehaviorGraphControlRigNode, CBehaviorGraphControlRigInterface >( CName::NONE, m_ikInterface );

				//m_ikInterface.SetManualControl( true );
				//m_ikInterface.SetListener( this );

				if ( SBehaviorSampleContext* context = ac->GetBehaviorGraphSampleContext() )
				{
					if ( CSkeleton* s = ac->GetSkeleton() )
					{
						const TCrPropertySet* ps = s->GetDefaultControlRigPropertySet();
						const TCrDefinition* def = s->GetControlRigDefinition();

						if ( ps && def && def->IsValid() )
						{
							m_rig = def->CreateRig( ps );
							if ( m_rig )								
							{
								const SBehaviorGraphOutput& mainPose = context->GetMainPose();

								m_animPose.Init( mainPose.m_numBones, mainPose.m_numFloatTracks );
								m_ikPose.Init( mainPose.m_numBones, mainPose.m_numFloatTracks );

								m_isPoseCached = true;
								m_rig->SetLocalToWorld( ac->GetLocalToWorld() );
								for ( Int32 id=TCrEffector_First; id<TCrEffector_Last; ++id )
								{
									Vector val( Vector::ZERO_3D_POINT );
									Float w( 0.f );
									if ( m_parentWin->GetEvent()->LoadIkEffector( id, val, w ) )
									{
										LoadEffectorFromEvent( id, val, w );
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CEdControlRigIKPanel::OnUpdateTransforms( CStorySceneEventPoseKey* evt )
{
	SCENE_ASSERT( evt );

	if ( m_rig )
	{
		// 1. Solve full body ik
		CName parentAnimName;
		Float parentAnimTime( 0.f );
		const Bool isBinded = m_parentWin->GetEvtParentAnimation( evt, parentAnimName, parentAnimTime );

		if ( isBinded )
		{
			const CAnimatedComponent* ac = m_parentWin->GetActorsAnimatedComponent();
			const CSkeleton* s = ac->GetSkeleton();
			const TCrDefinition* def = s->GetControlRigDefinition();

			const Bool animSampled = SamplePose( ac, m_animPose, parentAnimName, parentAnimTime );
			SCENE_ASSERT( animSampled );

			m_rig->SetLocalToWorld( ac->GetLocalToWorld() );

			m_ikPose = m_animPose; // TEMP

			def->SetRigFromPoseLS( m_animPose, m_rig );

			m_rig->SyncMSFromLS();
			m_rig->SyncWSFromMS();

			if ( m_rig->IsAnyEffectorSet() )
			{
				m_rig->Solve();

				def->SetPoseLSFromRig( m_rig, m_ikPose );

				// 2. Calc diff
				m_boneTrans.ClearFast();
				m_boneIdx.ClearFast();

				SCENE_ASSERT( m_animPose.m_numBones == m_ikPose.m_numBones );

				const Int32 numBones = (Int32)m_animPose.m_numBones;
				for ( Int32 i=0; i<numBones; ++i )
				{
					if ( def->IsRigBone( i ) )
					{
						const RedQsTransform& boneA = m_animPose.m_outputPose[ i ];
						const RedQsTransform& boneB = m_ikPose.m_outputPose[ i ];

						RedQsTransform diff;
						diff.SetMulInverseMul( boneA, boneB );

						if ( !diff.Rotation.Quat.IsAlmostEqual( RedVector4::ZERO_3D_POINT, 1e-4f ) )
						{
							m_boneIdx.PushBack( i );

							diff.Translation.SetZeros();
							diff.Scale.SetOnes();

							m_boneTrans.PushBack( ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( diff ) );
						}
					}
				}

				// 3. Setup event
				evt->SetBonesTransformLS_IK( m_boneIdx, m_boneTrans );
			}
			else
			{
				m_boneTrans.ClearFast();
				m_boneIdx.ClearFast();

				evt->SetBonesTransformLS_IK( m_boneIdx, m_boneTrans );
			}
		}
		else
		{
			PrintDebugInfo();
		}
	}
}

Bool CEdControlRigIKPanel::SamplePose( const CAnimatedComponent* ac, SBehaviorGraphOutput& pose, CName animName, Float animTime ) const
{
	SCENE_ASSERT( ac );

	if ( CSkeletalAnimationContainer* c = ac->GetAnimationContainer() )
	{
		if ( const CSkeletalAnimationSetEntry* anim = c->FindAnimationRestricted( animName ) )
		{
			const Bool ret = anim->GetAnimation()->Sample( animTime, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
			if ( ret && ac->UseExtractedTrajectory() && ac->HasTrajectoryBone() )
			{
				pose.ExtractTrajectory( ac );
			}

			return ret;
		}
	}

	return false;
}

void CEdControlRigIKPanel::EnableUI( Bool flag )
{
	m_weightSlider->Enable( flag );
	m_weightText->Enable( flag );
}

void CEdControlRigIKPanel::PrintDebugInfo()
{

}

Matrix CEdControlRigIKPanel::GetEffectorDefaultMatrixWorldSpace( Int32 effectorId ) const
{
	Matrix mat( Matrix::IDENTITY );

	if ( m_rig )
	{
		RedQsTransform trasWS;
		m_rig->GetEffectorDefaultWS( effectorId, trasWS );
		mat = AnimQsTransformToMatrix( trasWS );
	}

	return mat;
}

void CEdControlRigIKPanel::SelectEffector( const wxString& name )
{
	 // TODO use ( Red::StringSearch( CEnum::ToString< ETCrEffectorId > ( ETCrEffectorId( i ) ).AsChar(), TXT("_") ) + 1 );

	if ( name == wxT("HandL") )
	{
		SelectEffector( TCrEffector_HandL );
	}
	else if ( name == wxT("HandR") )
	{
		SelectEffector( TCrEffector_HandR );
	}
}

void CEdControlRigIKPanel::SelectEntityHeleperFor( ETCrEffectorId id )
{
	m_parentWin->OnIkTransformPanel_EffectorSelected( id );
}

void CEdControlRigIKPanel::RefreshEffector( Int32 effectorID, const EngineTransform& transWS )
{
	if ( m_rig )
	{
		m_rig->SetEffectorWS( ETCrEffectorId( effectorID ), transWS.GetPosition() );		
	}

	if ( effectorID == m_effectorId )
	{
		SetPosition( transWS.GetPosition() );
	}

	RefreshEffector();
}

void CEdControlRigIKPanel::ActivateEffector( Int32 effectorID )
{
	if ( effectorID == m_effectorId && m_weight == 0.f)
	{
		SetWeight( 1.f, true );
		RefreshEffector();
	}
}

void CEdControlRigIKPanel::RefreshEffector()
{
	if( m_running )
	{
		if( m_rig )
		{
			m_rig->SetEffectorWS( m_effectorId, m_position );
			m_rig->SetTranslationActive( m_effectorId, m_weight );
		}

		if ( CStorySceneEventPoseKey* e = m_parentWin->GetEvent() )
		{
			e->CacheIkEffector( m_effectorId, m_position, m_weight );
		}
	}
}

void CEdControlRigIKPanel::SelectEffector( ETCrEffectorId id )
{
	if ( m_running )
	{
		//...
	}

	m_running = true;
	m_effectorId = id;

	EnableUI( true );

	Vector vec( Vector::ZEROS  );
	Float w( 0.f );

	GetDataFromEvent( vec, w );

	SetWeight( w, true );
	SetPosition( vec );

	SetEffectorName( Red::StringSearch( CEnum::ToString< ETCrEffectorId >( id ).AsChar(), TXT("_") ) + 1 );

	RefreshEffector();
}

void CEdControlRigIKPanel::DeselectEffector()
{
	m_running = false;
	SetWeight( 0.f, true );
	EnableUI( false );
}

void CEdControlRigIKPanel::SetPosition( const Vector& vec )
{
	m_position = vec;
}

void CEdControlRigIKPanel::SetEffectorName( const wxString& name )
{
	m_effectorText->SetLabelText( name );
}

void CEdControlRigIKPanel::SetWeightEdit( Float w )
{
	wxString str = wxString::Format( wxT("%1.2f"), w );
	m_weightText->SetLabelText( str );
}

void CEdControlRigIKPanel::SetWeightSlider( Float w )
{
	m_weightSlider->SetValue( (Int32)(w*100.f) );
}

void CEdControlRigIKPanel::SetWeight( Float w, Bool updateUI )
{
	m_weight = Clamp( w, 0.f, 1.f );

	if ( updateUI )
	{
		SetWeightSlider( m_weight );
		SetWeightEdit( m_weight );
	}
}

Bool CEdControlRigIKPanel::GetDataFromEvent( Vector& vec, Float& w ) const
{
	SCENE_ASSERT( m_parentWin->GetEvent() );

	Bool ret = false;

	if ( CStorySceneEventPoseKey* e = m_parentWin->GetEvent() )
	{
		ret = e->LoadIkEffector( m_effectorId, vec, w );
	}

	return ret;
}


void CEdControlRigIKPanel::LoadEffectorFromEvent( Int32 id, const Vector& val, Float w )
{
	if ( m_rig )
	{
		m_rig->SetEffectorWS( ETCrEffectorId( id ), val );		
		m_rig->SetTranslationActive( ETCrEffectorId( id ), w );
	}
}


void CEdControlRigIKPanel::OnGenerateFragments( CRenderFrame *frame )
{
	if ( m_rig )
	{
		m_rig->DrawSkeleton( frame, true );

		RedQsTransform handL;
		RedQsTransform handR;
		m_rig->GetEffectorDefaultWS( TCrEffector_HandL, handL );
		m_rig->GetEffectorDefaultWS( TCrEffector_HandR, handR );

		RedQsTransform handLEf;
		RedQsTransform handREf;
		m_rig->GetEffectorWS( TCrEffector_HandL, handLEf );
		m_rig->GetEffectorWS( TCrEffector_HandR, handREf );

		frame->AddDebugSphere( AnimVectorToVector( handL.Translation ), 0.025f, Matrix::IDENTITY, Color( 255,0,0 ) );
		frame->AddDebugSphere( AnimVectorToVector( handR.Translation ), 0.025f, Matrix::IDENTITY, Color( 255,0,0 ) );

		frame->AddDebugSphere( AnimVectorToVector( handLEf.Translation ), 0.05f, Matrix::IDENTITY, Color( 255,0,0 ) );
		frame->AddDebugSphere( AnimVectorToVector( handREf.Translation ), 0.05f, Matrix::IDENTITY, Color( 255,0,0 ) );
	}
}

void CEdControlRigIKPanel::OnWeightSlider( wxCommandEvent& event )
{
	m_weight = (Float)event.GetInt() / 100.f;

	SetWeightEdit( m_weight );

	RefreshEffector();
	m_parentWin->OnIkTransformPanel_RefreshPlayer();
}

void CEdControlRigIKPanel::OnWeightEdit( wxCommandEvent& event )
{
	String str = m_weightText->GetLabelText().wc_str();
	Float w( 0.f );
	if ( FromString( str, w ) )
	{
		m_weight = Clamp( w, 0.f, 1.f );

		SetWeightSlider( m_weight );
		SetWeightEdit( m_weight );

		RefreshEffector();
	}
	m_parentWin->OnIkTransformPanel_RefreshPlayer();
}

void CEdControlRigIKPanel::OnHandR( wxCommandEvent& event )
{
	SelectEffector( TCrEffector_HandR );

	SelectEntityHeleperFor( TCrEffector_HandR );
}

void CEdControlRigIKPanel::OnHandL( wxCommandEvent& event )
{
	SelectEffector( TCrEffector_HandL );

	SelectEntityHeleperFor( TCrEffector_HandL );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
