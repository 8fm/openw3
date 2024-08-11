
#include "build.h"
#include "animFriendPageStepPlayer.h"

//////////////////////////////////////////////////////////////////////////

Bool CEdAnimationStepPlayerParamInitializer::Initialize( ISkeletalAnimationSetEntryParam* _param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const
{
	CSkeletalAnimationStepClipParam* param = Cast< CSkeletalAnimationStepClipParam >( _param );
	if ( param )
	{
		/*String boneNameStr( TXT("head") );

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

		hitParam->Init( init );*/

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdAnimationFriendStepPlayerPage, CEdAnimationFriendSimplePage )
END_EVENT_TABLE()

CEdAnimationFriendStepPlayerPage::CEdAnimationFriendStepPlayerPage( CEdAnimationFriend* owner )
	: CEdAnimationFriendSimplePage( owner )
	, m_destDirectionWS( 0.f, 1.f, 0.f )
{
	wxPanel* panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	{
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_paramPanel = new CEdAnimationParamPanel( panel, ClassID< CSkeletalAnimationStepClipParam >(), new CEdAnimationStepPlayerParamInitializer() );

		sizer->Add( m_paramPanel, 1, wxEXPAND|wxALL, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( panel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	m_paramPanel->CloneAndUseAnimatedComponent( GetAnimatedComponent() );

	//m_stepPlayer = new StepPlayer( GetAnimatedComponent() );
}

CEdAnimationFriendStepPlayerPage::~CEdAnimationFriendStepPlayerPage()
{
	
}

wxAuiPaneInfo CEdAnimationFriendStepPlayerPage::GetPageInfo() const
{
	wxAuiPaneInfo info;
	info.Dockable( true ).Right().Row( 2 ).Layer( 2 ).MinSize( wxSize( 200, 100 ) ).CloseButton( false ).Name( wxT("CEdAnimationFriendStepPlayerPage") );
	return info;
}

void CEdAnimationFriendStepPlayerPage::OnTick( Float dt )
{
	const CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		//m_stepPlayer->Move( dt, ac->GetWorldForward(), m_destDirectionWS );
	}
}

void CEdAnimationFriendStepPlayerPage::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	//m_destDirectionWS

	// TEMP
	if ( key == IK_Space )
	{
		// Save
	}
};
