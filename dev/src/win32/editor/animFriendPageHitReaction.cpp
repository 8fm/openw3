
#include "build.h"
#include "animFriendPageHitReaction.h"
#include "../../common/engine/animationGameParams.h"
#include "../../common/engine/animatedIterators.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/renderFrame.h"

BEGIN_EVENT_TABLE( CEdAnimationFriendHitReactionPage, CEdAnimationFriendSimplePage )
	EVT_BUTTON( XRCID( "btnHit" ), CEdAnimationFriendHitReactionPage::OnButtonHit )
END_EVENT_TABLE()

const String CEdAnimationFriendHitReactionPage::POINTER_NAME( TXT("Pointer") );

CEdAnimationFriendHitReactionPage::CEdAnimationFriendHitReactionPage( CEdAnimationFriend* owner )
	: CEdAnimationFriendSimplePage( owner )
	, m_pointItem( NULL )
	, m_selectedAnimPoint( -1 )
	, m_bestAnimPoint( -1 )
	, m_selector( NULL )
	, m_entityInZeroPoint( false )
{
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimationFriendHitReaction") );

	{
		wxPanel* panel = XRCCTRL( *this, "paramPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_paramPanel = new CEdAnimationParamPanel( panel, ClassID< CSkeletalAnimationHitParam >(), new CEdAnimationHitParamInitializer(), this, false );
		m_paramPanel->AddPreviewPostprocess( new CEdAnimationPreviewPostProcessHit() );
		m_paramPanel->AddPreviewPostprocess( new CEdAnimationPreviewPostSkeleton( Color( 255, 255, 255 ) ) );

		sizer->Add( m_paramPanel, 1, wxEXPAND, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	m_paramPanel->CloneAndUseAnimatedComponent( GetAnimatedComponent() );

	CreatePointerItem();

	RebuildDummyItems();

	ShiftEntityToZeroPoint();

	SelectAnimAsBest( NULL );

	//SetCustomPerspective( wxT("layout2|name=Preview;caption=;state=16779260;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=800;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Browser;caption=;state=16779260;dir=2;layer=5;row=5;pos=1;prop=100000;bestw=320;besth=700;minw=200;minh=300;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=0f3978c0508690a90008dd9900000003;caption=;state=2044;dir=2;layer=5;row=5;pos=0;prop=100000;bestw=200;besth=100;minw=200;minh=100;maxw=-1;maxh=-1;floatx=832;floaty=146;floatw=400;floath=250|dock_size(5,0,0)=360|dock_size(2,5,5)=650|") );
}

CEdAnimationFriendHitReactionPage::~CEdAnimationFriendHitReactionPage()
{
	delete m_selector;
}

wxAuiPaneInfo CEdAnimationFriendHitReactionPage::GetPageInfo() const
{
	wxAuiPaneInfo info;
	info.Dockable( true ).Right().Row( 2 ).Layer( 2 ).MinSize( wxSize( 200, 100 ) ).CloseButton( false ).Name( wxT("CEdAnimationFriendHitReactionPage") );
	return info;
}

void CEdAnimationFriendHitReactionPage::OnTick( Float dt )
{
	if ( !m_playedAnimation && m_bestAnimPoint != -1 )
	{
		if ( !m_entityInZeroPoint )
		{
			ShiftEntityToZeroPoint();

			m_entityInZeroPoint = true;
		}
		else
		{
			m_playedAnimation = PlaySingleAnimation( m_animPoints[ m_bestAnimPoint ].m_animation );

			m_entityInZeroPoint = false;

			m_bestAnimPoint = -1;
		}
	}
}

void CEdAnimationFriendHitReactionPage::OnAnimationStarted( const CPlayedSkeletalAnimation* animation )
{
	ASSERT( m_playedAnimation == animation );
}

void CEdAnimationFriendHitReactionPage::OnAnimationFinished( const CPlayedSkeletalAnimation* animation )
{
	StopHitAnimation();
}

void CEdAnimationFriendHitReactionPage::OnAnimationStopped( const CPlayedSkeletalAnimation* animation )
{
	StopHitAnimation();
}

void CEdAnimationFriendHitReactionPage::StopHitAnimation()
{
	m_bestAnimPoint = -1;

	m_playedAnimation = NULL;
}

void CEdAnimationFriendHitReactionPage::OnSelectPreviewItem( IPreviewItem* item )
{
	m_selectedAnimPoint = -1;

	for ( Int32 i=0; i<m_animPoints.SizeInt(); ++i )
	{
		//if ( m_animPoints[ i ].m_item == item )
		//{
		//	m_selectedAnimPoint = i;
		//}
	}

	OnSelectedItemChanged();
}

void CEdAnimationFriendHitReactionPage::OnDeselectAllPreviewItems()
{
	m_selectedAnimPoint = -1;
	
	OnSelectedItemChanged();
}

void CEdAnimationFriendHitReactionPage::OnGenerateFragments( CRenderFrame *frame )
{
	const CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		SkeletonRenderingUtils::DrawSkeleton( ac, Color( 255, 0, 0 ), frame );
	}

	if ( m_playedAnimation )
	{
		const CName& animName = m_playedAnimation->GetName();

		frame->AddDebugScreenText( 50, frame->GetFrameOverlayInfo().m_height - 100, animName.AsString() );
	}
}

void CEdAnimationFriendHitReactionPage::OnLoadPreviewEntity( CAnimatedComponent* component )
{
	m_paramPanel->CloneAndUseAnimatedComponent( component );
}

void CEdAnimationFriendHitReactionPage::ShiftEntityToZeroPoint()
{
	const CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		CEntity* ent = ac->GetEntity();
		
		ent->Teleport( Vector::ZERO_3D_POINT, EulerAngles::ZEROS );

		ent->GetRootAnimatedComponent()->SetUseExtractedMotion( true );
	}
}

void CEdAnimationFriendHitReactionPage::FireHit()
{
	const CAnimatedComponent* ac = GetAnimatedComponent();

	if ( ac && m_selector )
	{
		AnimationSelector_HitPoint::InputData input;
		input.m_pointWS = m_hitPointWS;

		const CSkeletalAnimationSetEntry* anim = m_selector->DoSelect( input, ac->GetLocalToWorld() );

		SelectAnimAsBest( anim );
	}
}

void CEdAnimationFriendHitReactionPage::RebuildDummyItems()
{
	m_animPoints.Clear();

	IPreviewItemContainer* ic = GetPreviewItemContainer();

	TDynArray< const CSkeletalAnimationSetEntry* > anims;
	CollectAllAnimations( anims );

	const Uint32 size = anims.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CSkeletalAnimationSetEntry* anim = anims[ i ];

		const CSkeletalAnimationHitParam* param = anim->FindParam< CSkeletalAnimationHitParam >();
		if ( param )
		{
			AnimPoint apoint;
			apoint.m_animation = anim;
 			apoint.m_hit = param;
 
			m_animPoints.PushBack( apoint );
		}
		else
		{
			ASSERT( 0 );
		}
	}

	delete m_selector;
	m_selector = new AnimationSelector_HitPoint();

	const CAnimatedComponent* ac = GetAnimatedComponent();
	ComponentAnimationIterator it( ac );

	m_selector->Init( it );
}

void CEdAnimationFriendHitReactionPage::CreatePointerItem()
{
	IPreviewItemContainer* ic = GetPreviewItemContainer();

	ASSERT( !m_pointItem );

	m_pointItem = new CEdHitPointItem( ic, this );
	m_pointItem->Init( POINTER_NAME );
	m_pointItem->SetSize( IPreviewItem::PS_Small );

	m_pointItem->SetPosition( Vector( 0.f, 2.f, 1.8f ) );

	ic->AddItem( m_pointItem );
}

void CEdAnimationFriendHitReactionPage::SetDummyItemNamesVisible( Bool flag )
{

}

void CEdAnimationFriendHitReactionPage::OnSelectedItemChanged()
{
	if ( m_selectedAnimPoint == -1 )
	{

	}
	else
	{

	}
}

void CEdAnimationFriendHitReactionPage::OnHitPointChanged( const Vector& point )
{
	m_hitPointWS = point;
}

void CEdAnimationFriendHitReactionPage::SelectAnimAsBest( const CSkeletalAnimationSetEntry* anim )
{
	Int32 prevBestAnim = m_bestAnimPoint;

	m_bestAnimPoint = -1;

	if ( anim )
	{
		const Int32 size = m_animPoints.SizeInt();

		for ( Int32 i=0; i<size; ++i )
		{
			const AnimPoint& p = m_animPoints[ i ];

			if ( p.m_animation == anim )
			{
				m_bestAnimPoint = i;
			}
		}
	}

	if ( prevBestAnim != m_bestAnimPoint )
	{
		if ( m_bestAnimPoint == -1 )
		{
			StopHitAnimation();
		}
	}
}

void CEdAnimationFriendHitReactionPage::OnButtonHit( wxCommandEvent& event )
{
	FireHit();
}

//////////////////////////////////////////////////////////////////////////

CEdHitPointItem::CEdHitPointItem( IPreviewItemContainer* ic, CEdAnimationFriendHitReactionPage* page )
	: m_container( ic )
	, m_page( page )
{

}

IPreviewItemContainer* CEdHitPointItem::GetItemContainer() const
{
	return m_container;
}

void CEdHitPointItem::Refresh()
{

}

void CEdHitPointItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_page->OnHitPointChanged( newPos );
}

void CEdHitPointItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{

}

//////////////////////////////////////////////////////////////////////////

Bool CEdAnimationHitParamInitializer::Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const
{
	CSkeletalAnimationHitParam* hitParam = Cast< CSkeletalAnimationHitParam >( param );
	if ( hitParam )
	{
		String boneNameStr( TXT("head") );

		if ( !InputBox( NULL, TXT("Bone name"), TXT("Write bone name"), boneNameStr ) )
		{
			return false;
		}

		Int32 boneIndex = animatedComponent->FindBoneByName( boneNameStr.AsChar() );
		if ( boneIndex == -1 )
		{
			return false;
		}

		CSkeletalAnimationHitParam::InitData init;
		init.m_boneName = CName( boneNameStr );
		init.m_pointMS = Vector( 0.f, 1.f, 1.5f );
		init.m_directionMS = -Vector::EY;

		hitParam->Init( init );

		return true;
	}

	return false;
}