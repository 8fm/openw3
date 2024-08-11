
#include "build.h"
#include "entityMimicsPanel.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/mimicComponent.h"

BEGIN_EVENT_TABLE( CEdEntityEditorMimicsPanel, wxPanel )
END_EVENT_TABLE()

//#pragma optimize("",off)

RED_DEFINE_STATIC_NAME( CEdEntityEditorMimicsPanel_Pose );

CEdEntityEditorMimicsPanel::CEdEntityEditorMimicsPanel( wxWindow* parent )
	: CPatToolPanel( parent, "get_faceed_joysticks" )
	, m_isActive( false )
{

}

void CEdEntityEditorMimicsPanel::SetEntity( CEntity* e )
{
	m_entity = e;

	Activate( true );

	RefreshLogic();
}

void CEdEntityEditorMimicsPanel::Activate( Bool flag )
{
	m_isActive = flag;

	RefreshLogic();
}

void CEdEntityEditorMimicsPanel::RefreshLogic()
{
	m_slot = CBehaviorMixerSlotInterface();

	if ( CActor* a = Cast< CActor >( m_entity.Get() ) )
	{
		if ( CMimicComponent* m = a->GetMimicComponent() )
		{
			if ( m->GetBehaviorStack() && m->GetMimicSkeleton() )
			{
				if ( m_isActive )
				{
					m->MimicHighOn();

					if ( m->GetBehaviorStack()->GetSlot( CName( TXT( "MIXER_SLOT_OVERRIDE" ) ), m_slot, false ) )
					{
						if ( m_slot.IsValid() )
						{
							m_tracks.Resize( m->GetMimicSkeleton()->GetTracksNum() );
							SetData( m_tracks.Data(), m_tracks.DataSize() );
						}
					}
				}
				else
				{
					m->MimicHighOff();
				}
			}
		}
	}

}

void CEdEntityEditorMimicsPanel::OnControlsPreChanged()
{
	CPatToolPanel::OnControlsPreChanged();
}

void CEdEntityEditorMimicsPanel::OnControlsChanging()
{
	CPatToolPanel::OnControlsChanging();

	if ( m_isActive && m_working && m_slot.IsValid() )
	{
		SAnimationMappedPose pose;
		pose.m_weight = 1.f;
		pose.m_mode = AMPM_Override;
		pose.m_tracks = m_tracks;
		m_slot.AddPoseToSample( CNAME( CEdEntityEditorMimicsPanel_Pose ), pose );
	}
}

void CEdEntityEditorMimicsPanel::OnControlsPostChanged()
{
	CPatToolPanel::OnControlsPostChanged();

	// Hmm, Do we need this?
	if ( m_isActive && m_slot.IsValid() )
	{
		SAnimationMappedPose pose;
		pose.m_weight = 1.f;
		pose.m_mode = AMPM_Override;
		pose.m_tracks = m_tracks;
		m_slot.AddPoseToSample( CNAME( CEdEntityEditorMimicsPanel_Pose ), pose );
	}
}

//#pragma optimize("",on)
