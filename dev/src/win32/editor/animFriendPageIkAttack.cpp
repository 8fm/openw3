
#include "build.h"
#include "animFriendPageIkAttack.h"
#include "animationParamPanel.h"
#include "../../common/engine/animatedIterators.h"
#include "../../common/engine/animationGameParams.h"
#include "animationPreviewPostprocess.h"

BEGIN_EVENT_TABLE( CEdAnimationFriendIkAttackPage, CEdAnimationFriendSimplePage )
	EVT_CHECKBOX( XRCID( "checkGhosts" ), CEdAnimationFriendIkAttackPage::OnGhostsToggle )
	EVT_BUTTON( XRCID( "btnAttack" ), CEdAnimationFriendIkAttackPage::OnAttack )
	EVT_TOGGLEBUTTON( XRCID( "btnAutoAttack" ), CEdAnimationFriendIkAttackPage::OnAutoAttack )
	EVT_TOGGLEBUTTON( XRCID( "btnSyncBreak" ), CEdAnimationFriendIkAttackPage::OnSyncBreak )
END_EVENT_TABLE()

const String CEdAnimationFriendIkAttackPage::TARGET_NAME( TXT("Target") );

CEdAnimationFriendIkAttackPage::CEdAnimationFriendIkAttackPage( CEdAnimationFriend* owner )
	: CEdAnimationFriendSimplePage( owner )
	, m_pointItem( NULL )
	, m_selectedAnimPoint( -1 )
	, m_bestAnimPoint( -1 )
	, m_nearestAnimPoint( -1 )
	, m_isPlayingAttack( 0 )
	, m_player( NULL )
	, m_autoAttack( false )
	, m_targetPosition( Vector::ZERO_3D_POINT )
	//, m_nearestTrajectoryL( NULL )
	//, m_nearestTrajectoryR( NULL )
	, m_syncBreak( true )
	, m_syncBreakDuration( 3.f )
{
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimationFriendIkAttack") );

	{
		wxPanel* panel = XRCCTRL( *this, "paramPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_paramPanel = new CEdAnimationParamPanel( panel, ClassID< CSkeletalAnimationAttackTrajectoryParam >(), new CEdAnimationAttackTrajectoryParamInitializer(), this );
		m_paramPanel->AddPreviewPostprocess( new CEdAnimationPreviewPostProcessTrajectory() );
		m_paramPanel->AddPreviewPostprocess( new CEdAnimationPreviewPostSkeleton( Color( 255, 255, 255 ) ) );

		sizer->Add( m_paramPanel, 1, wxEXPAND, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	//SetCustomPerspective( wxT("layout2|name=Preview;caption=;state=16779260;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Browser;caption=;state=16779260;dir=2;layer=5;row=5;pos=0;prop=100000;bestw=320;besth=700;minw=200;minh=300;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=CEdAnimationFriendIkAttackPage;caption=;state=2044;dir=2;layer=5;row=5;pos=1;prop=100000;bestw=200;besth=100;minw=200;minh=100;maxw=-1;maxh=-1;floatx=826;floaty=131;floatw=400;floath=250|dock_size(5,0,0)=360|dock_size(2,5,5)=641|") );

	m_paramPanel->CloneAndUseAnimatedComponent( GetAnimatedComponent() );

	CreatePointerItem();

	RebuildDummyItems();

	ShiftEntityToZeroPoint();

	SelectAnimAsBest( NULL, m_bestAnimPoint );
}

CEdAnimationFriendIkAttackPage::~CEdAnimationFriendIkAttackPage()
{
	delete m_player;
	//delete m_nearestTrajectoryL;
	//delete m_nearestTrajectoryR;
}

wxAuiPaneInfo CEdAnimationFriendIkAttackPage::GetPageInfo() const
{
	wxAuiPaneInfo info;
	info.Dockable( true ).Right().Row( 2 ).Layer( 2 ).MinSize( wxSize( 200, 100 ) ).CloseButton( false ).Name( wxT("CEdAnimationFriendIkAttackPage") );
	return info;
}

void CEdAnimationFriendIkAttackPage::OnTick( Float dt )
{
	if ( m_player )
	{
		m_player->Tick( dt );

		if ( !m_player->IsPlayingAnimation() )
		{
			if ( m_isPlayingAttack > 5 )
			{
				m_isPlayingAttack = 0;

				ShiftEntityToZeroPoint();

				UnblockFireButton();
			}
			else if ( m_isPlayingAttack > 0 )
			{
				m_isPlayingAttack++;
			}
		}
	}
}

void CEdAnimationFriendIkAttackPage::OnAnimationStarted( const CPlayedSkeletalAnimation* animation )
{
	//ASSERT( m_playedAnimation == animation );
}

void CEdAnimationFriendIkAttackPage::OnAnimationFinished( const CPlayedSkeletalAnimation* animation )
{
	//m_playedAnimation = NULL;
}

void CEdAnimationFriendIkAttackPage::OnAnimationStopped( const CPlayedSkeletalAnimation* animation )
{
	//m_playedAnimation = NULL;
}

void CEdAnimationFriendIkAttackPage::OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param )
{

}

void CEdAnimationFriendIkAttackPage::OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param )
{

}

void CEdAnimationFriendIkAttackPage::OnSelectPreviewItem( IPreviewItem* item )
{
	m_selectedAnimPoint = -1;

	for ( Int32 i=0; i<m_animPoints.SizeInt(); ++i )
	{
		if ( m_animPoints[ i ].m_item == item )
		{
			m_selectedAnimPoint = i;
		}
	}

	OnSelectedItemChanged();
}

void CEdAnimationFriendIkAttackPage::OnDeselectAllPreviewItems()
{
	m_selectedAnimPoint = -1;
	
	OnSelectedItemChanged();
}

void CEdAnimationFriendIkAttackPage::OnGenerateFragments( CRenderFrame *frame )
{
	const CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		SkeletonRenderingUtils::DrawSkeleton( ac, Color( 255, 0, 0 ), frame );

		//if ( m_nearestTrajectoryL )
		//{
		//	m_nearestTrajectoryL->GenerateFragments( frame, 0.f, Matrix::IDENTITY, Color( 255, 0, 0 ), true );
		//}

		//if ( m_nearestTrajectoryR )
		//{
		//	m_nearestTrajectoryR->GenerateFragments( frame, 0.f, Matrix::IDENTITY, Color( 255, 0, 0 ), true );
		//}

		if ( m_player )
		{
			m_player->GenerateFragments( frame );
		}

		//to nie potrzebne taj moze rycowa traj bez i po zmianach a traj pralyer tez z tego korzysta
		//if ( m_nearestAnimPoint != -1 )
		//{
		//	DrawTrajectory( m_nearestAnimPoint, -1.f, frame );
		//}

		//if ( m_bestAnimPoint != -1 && m_bestAnimPoint == m_selectedAnimPoint )
		//{
		//
		//}
		//else
		{
			if ( m_selectedAnimPoint != -1 )
			{
				//DrawTrajectory( m_selectedAnimPoint, -1.f, frame );
			}

			if ( m_bestAnimPoint != -1 )
			{
				//DrawTrajectory( m_bestAnimPoint, -1.f, frame );
			}
		}
	}
}

void CEdAnimationFriendIkAttackPage::OnLoadPreviewEntity( CAnimatedComponent* component )
{
	m_paramPanel->CloneAndUseAnimatedComponent( component );

	delete m_player;
	m_player = NULL;

	if ( component )
	{
		m_player = new AnimationTrajectoryPlayer();
		m_player->Init( component->GetEntity() );

		UpdateSyncBreak();
	}
}

void CEdAnimationFriendIkAttackPage::ShiftEntityToZeroPoint()
{
	const CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		CEntity* ent = ac->GetEntity();
		
		ent->Teleport( Vector::ZERO_3D_POINT, EulerAngles::ZEROS );

		ent->GetRootAnimatedComponent()->SetUseExtractedMotion( true );
	}
}

void CEdAnimationFriendIkAttackPage::BlockFireButton()
{
	wxButton* btn = XRCCTRL( *this, "btnAttack", wxButton );
	btn->Enable( false );
}

void CEdAnimationFriendIkAttackPage::UnblockFireButton()
{
	wxButton* btn = XRCCTRL( *this, "btnAttack", wxButton );
	btn->Enable( true );
}

void CEdAnimationFriendIkAttackPage::DrawTrajectory( Int32 index, Float time, CRenderFrame *frame ) const
{
	const CAnimatedComponent* ac = GetAnimatedComponent();

	const CSkeletalAnimationAttackTrajectoryParam* param = m_animPoints[ index ].m_trajectory;
	if ( ac && param && param->IsParamValid() )
	{
		if ( time < 0.f )
		{
			time = 0.f;
		}
		
		//AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtrO( frame, param->GetData(), ac->GetLocalToWorld(), time, m_animPoints[ index ].m_animation->GetDuration() );
	}
}

void CEdAnimationFriendIkAttackPage::RebuildDummyItems()
{
	m_animPoints.Clear();

	IPreviewItemContainer* ic = GetPreviewItemContainer();

	TDynArray< const CSkeletalAnimationSetEntry* > anims;
	CollectAllAnimations( anims );

	const Uint32 size = anims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CSkeletalAnimationSetEntry* anim = anims[ i ];

		const CSkeletalAnimationAttackTrajectoryParam* param = anim->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
		if ( param )
		{
			CEdIkAttackDummyItem* item = CreateDummyItem( ic, anim, param );

			AnimPoint apoint;
			apoint.m_item = item;
			apoint.m_animation = anim;
			apoint.m_trajectory = param;

			m_animPoints.PushBack( apoint );
		}
		else
		{
			ASSERT( 0 );
		}
	}

	const CAnimatedComponent* ac = GetAnimatedComponent();

	delete m_player;
	m_player = NULL;

	if ( ac )
	{
		m_player = new AnimationTrajectoryPlayer();
		m_player->Init( ac->GetEntity() );
	}

	UpdateSyncBreak();
}

CEdIkAttackDummyItem* CEdAnimationFriendIkAttackPage::CreateDummyItem( IPreviewItemContainer* ic, const CSkeletalAnimationSetEntry* anim, const CSkeletalAnimationAttackTrajectoryParam* traj )
{
	const CAnimatedComponent* ac = GetAnimatedComponent();

	CEdIkAttackDummyItem* item = new CEdIkAttackDummyItem( ic );
	item->Init( anim->GetName().AsString() );
	item->SetSize( IPreviewItem::PS_Tiny );

	Vector syncPoint;
	if ( !traj->GetSyncPointRightMS( syncPoint ) )
	{
		return NULL;
	}

	Vector point = ac ? ac->GetLocalToWorld().TransformPoint( syncPoint ) : syncPoint;
	item->SetPosition( point );

	ic->AddItem( item );

	return item;
}

void CEdAnimationFriendIkAttackPage::CreatePointerItem()
{
	IPreviewItemContainer* ic = GetPreviewItemContainer();

	ASSERT( !m_pointItem );

	m_pointItem = new CEdIkAttackPointItem( ic, this );
	m_pointItem->Init( TARGET_NAME );
	m_pointItem->SetSize( IPreviewItem::PS_Small );

	m_pointItem->SetPosition( Vector( 0.f, 2.f, 1.8f ) );

	ic->AddItem( m_pointItem );
}

void CEdAnimationFriendIkAttackPage::SetDummyItemNamesVisible( Bool flag )
{

}

void CEdAnimationFriendIkAttackPage::OnSelectedItemChanged()
{
	if ( m_selectedAnimPoint == -1 )
	{

	}
	else
	{

	}
}

void CEdAnimationFriendIkAttackPage::OnAttackPointChanged( const Vector& point )
{
	m_targetPosition = point;

	if ( m_autoAttack )
	{
		FireAttack();
	}

	if ( m_player )
	{
		/*const CSkeletalAnimationSetEntry* anim = m_player->CheckSelection( m_targetPosition );

		const Int32 prev = m_nearestAnimPoint;

		SelectAnimAsBest( anim, m_nearestAnimPoint );

		if ( prev != m_nearestAnimPoint )
		{
			if ( m_nearestTrajectoryL )
			{
				delete m_nearestTrajectoryL;
				m_nearestTrajectoryL = NULL;
			}

			if ( m_nearestTrajectoryR )
			{
				delete m_nearestTrajectoryR;
				m_nearestTrajectoryR = NULL;
			}

			if ( m_nearestAnimPoint != -1 && anim )
			{
				const AnimationTrajectoryData* dataL = GetTrajectoryDataFromAnimation( anim, true );
				if ( dataL )
				{
					m_nearestTrajectoryL = new tAnimationTrajectory_Trajectory( new AnimationTrajectoryModifier_OnePoint( *dataL ), anim->GetDuration() );
				}

				const AnimationTrajectoryData* dataR = GetTrajectoryDataFromAnimation( anim, false );
				if ( dataR )
				{
					m_nearestTrajectoryR = new tAnimationTrajectory_Trajectory( new AnimationTrajectoryModifier_OnePoint( *dataR ), anim->GetDuration() );
				}
			}
		}

		if ( m_nearestTrajectoryL )
		{
			m_nearestTrajectoryL->SetSyncPoint( m_targetPosition, Matrix::IDENTITY, 0.f );
		}

		if ( m_nearestTrajectoryR )
		{
			m_nearestTrajectoryR->SetSyncPoint( m_targetPosition, Matrix::IDENTITY, 0.f );
		}*/
	}
}

const AnimationTrajectoryData* CEdAnimationFriendIkAttackPage::GetTrajectoryDataFromAnimation( const CSkeletalAnimationSetEntry* animation, Bool left ) const
{
	const CSkeletalAnimationAttackTrajectoryParam* param = animation->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
	return param ? left ? &(param->GetDataL()) : &(param->GetDataR()) : NULL;
}

void CEdAnimationFriendIkAttackPage::FireAttack()
{
	const CAnimatedComponent* ac = GetAnimatedComponent();

	if ( ac && m_player && !m_player->IsPlayingAnimation() )
	{
		/*SAnimationTrajectoryPlayerInput input;
		input.m_pointWS = m_targetPosition;

		SAnimationTrajectoryPlayerToken token;

		Bool ret = m_player->SelectAnimation( input, token );
		if ( ret )
		{
			ret = m_player->PlayAnimation( token );
		}

		const CSkeletalAnimationSetEntry* anim = m_player->GetCurrentAnimation();

		SelectAnimAsBest( anim, m_bestAnimPoint );

		if ( ret )
		{
			m_isPlayingAttack = 1;

			BlockFireButton();
		}*/
	}
}

void CEdAnimationFriendIkAttackPage::SelectAnimAsBest( const CSkeletalAnimationSetEntry* anim, Int32& index )
{
	Int32 prevBestAnim = index;

	index = -1;

	if ( anim )
	{
		const Int32 size = m_animPoints.SizeInt();

		for ( Int32 i=0; i<size; ++i )
		{
			const AnimPoint& p = m_animPoints[ i ];

			if ( p.m_animation == anim )
			{
				index = i;
			}
		}
	}
}

void CEdAnimationFriendIkAttackPage::UpdateSyncBreak()
{
	if ( m_player )
	{
		//m_player->SetSyncPointBreak( m_syncBreak ? m_syncBreakDuration : 0.f );
	}
}

void CEdAnimationFriendIkAttackPage::OnGhostsToggle( wxCommandEvent& event )
{
	
}

void CEdAnimationFriendIkAttackPage::OnAttack( wxCommandEvent& event )
{
	FireAttack();
}

void CEdAnimationFriendIkAttackPage::OnAutoAttack( wxCommandEvent& event )
{
	m_autoAttack = event.IsChecked();
}

void CEdAnimationFriendIkAttackPage::OnSyncBreak( wxCommandEvent& event )
{
	m_syncBreak = event.IsChecked();

	UpdateSyncBreak();
}

//////////////////////////////////////////////////////////////////////////

CEdIkAttackDummyItem::CEdIkAttackDummyItem( IPreviewItemContainer* ic )
	: m_container( ic )
{
	
}

IPreviewItemContainer* CEdIkAttackDummyItem::GetItemContainer() const
{
	return m_container;
}


//////////////////////////////////////////////////////////////////////////

CEdIkAttackPointItem::CEdIkAttackPointItem( IPreviewItemContainer* ic, CEdAnimationFriendIkAttackPage* page )
	: m_container( ic )
	, m_page( page )
{

}

IPreviewItemContainer* CEdIkAttackPointItem::GetItemContainer() const
{
	return m_container;
}

void CEdIkAttackPointItem::Refresh()
{

}

void CEdIkAttackPointItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_page->OnAttackPointChanged( newPos );
}

void CEdIkAttackPointItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{

}

//////////////////////////////////////////////////////////////////////////

Bool CEdAnimationAttackTrajectoryParamInitializer::Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const
{
	CSkeletalAnimationAttackTrajectoryParam* trajParam = Cast< CSkeletalAnimationAttackTrajectoryParam >( param );
	if ( trajParam )
	{
		String boneNameStrL( TXT("l_weapon") );
		String boneNameStrR( TXT("r_weapon") );

		if ( !InputBox( NULL, TXT("Bone name"), TXT("Write left bone name"), boneNameStrL ) )
		{
			return false;
		}

		if ( !InputBox( NULL, TXT("Bone name"), TXT("Write right bone name"), boneNameStrR ) )
		{
			return false;
		}

		Int32 boneIndexL = animatedComponent->FindBoneByName( boneNameStrL.AsChar() );
		if ( boneIndexL == -1 )
		{
			return false;
		}

		Int32 boneIndexR = animatedComponent->FindBoneByName( boneNameStrR.AsChar() );
		if ( boneIndexR == -1 )
		{
			return false;
		}

		String timeStr( TXT("0.3") );

		if ( !InputBox( NULL, TXT("Time"), TXT("Write time for sync point"), timeStr ) )
		{
			return false;
		}

		Float time = 0.f;
		FromString( timeStr, time );

		AnimationTrajectoryData dataL;
		AnimationTrajectoryData dataR;

		if ( !AnimationTrajectoryBuilder::CreateTrajectoryFromCompressedAnimation( animation, animatedComponent->GetSkeleton(), boneIndexL, time, animatedComponent->GetTrajectoryBone(), dataL ) )
		{
			return false;
		}

		if ( !AnimationTrajectoryBuilder::CreateTrajectoryFromCompressedAnimation( animation, animatedComponent->GetSkeleton(), boneIndexR, time, animatedComponent->GetTrajectoryBone(), dataR ) )
		{
			return false;
		}

		trajParam->Init( dataL, dataR );

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
