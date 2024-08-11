/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphAnimationBlendSlotNode.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/skeletalAnimationContainer.h"

#include "behaviorEditor.h"
#include "behaviorSlotTester.h"
#include "../../common/core/diskFile.h"

BEGIN_EVENT_TABLE( CEdBehaviorGraphSlotTester, CEdBehaviorEditorSimplePanel )
	EVT_CHOICE( XRCID( "slotChoice" ), CEdBehaviorGraphSlotTester::OnSlotChoice )
	EVT_BUTTON( XRCID( "buttPlay" ), CEdBehaviorGraphSlotTester::OnPlay )
	EVT_BUTTON( XRCID( "buttStop" ), CEdBehaviorGraphSlotTester::OnStop )
	EVT_BUTTON( XRCID( "buttAdd" ), CEdBehaviorGraphSlotTester::OnAdd )
	EVT_BUTTON( XRCID( "buttRemove" ), CEdBehaviorGraphSlotTester::OnRemove )
END_EVENT_TABLE()

const Float CEdBehaviorGraphSlotTester::PAINT_TIMER_DURATION = 1.f;

CEdBehaviorGraphSlotTester::CEdBehaviorGraphSlotTester( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_paintTimer( 0.f )
	, m_paintTimerSign( 1.f )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorSlotTester") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_slotChoice = XRCCTRL( *this, "slotChoice", wxChoice );
	m_animTree = XRCCTRL( *this, "animTree", wxTreeCtrl );
	m_animList = XRCCTRL( *this, "animSelect", wxListBox );

	m_dispDuration = XRCCTRL( *this, "dispDuration", wxStaticText );
	m_dispProgress = XRCCTRL( *this, "dispProgress", wxStaticText );
	m_dispTime = XRCCTRL( *this, "dispTime", wxStaticText );
	m_dispBlendIn = XRCCTRL( *this, "dispBlendIn", wxStaticText );
	m_dispBlendOut = XRCCTRL( *this, "dispBlendOut", wxStaticText );
	m_dispBlendInCheck = XRCCTRL( *this, "dispBlendInCheck", wxCheckBox );
	m_dispBlendOutCheck = XRCCTRL( *this, "dispBlendOutCheck", wxCheckBox );

	SetSizer( sizer );	
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphSlotTester::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Dockable( false ).Floatable( true ).Float().MinSize( GetMinSize() ).MaxSize( 475, 600 ).Resizable( false ).Dockable( false );

	return info;
}

void CEdBehaviorGraphSlotTester::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, 
													 CBehaviorGraphInstance& instance, 
													 EStatus status )
{
	if ( m_localSlotAnimation.Size() > 0 )
	{
		if ( !PlayNextAnim() )
		{
			StopAnim();
		}
	}
	else if ( IsLooped() )
	{
		m_localSlotAnimation = m_slotAnimations;

		if ( m_localSlotAnimation.Size() > 0 ) 
		{
			OnSlotAnimationEnd( sender, instance, status );
		}
	}
	else //if ( status != S_BlendOutStarted )
	{
		FinishAnim();
	}
}

Bool CEdBehaviorGraphSlotTester::PlayNextAnim()
{
	if ( !m_slot )
	{
		return false;
	}

	Uint32 animNum = m_slotAnimations.Size() - m_localSlotAnimation.Size();
	UpdateAnimList( (Int32)animNum );

	CName nextAnim = m_localSlotAnimation[0];
	m_localSlotAnimation.Erase( m_localSlotAnimation.Begin() );

	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = GetBlendIn();
	slotSetup.m_blendOut = GetBlendOut();
	slotSetup.m_looped = IsAnimLooped();
	slotSetup.m_listener = this;

	return GetBehaviorGraphInstance()->PlaySlotAnimation( m_slot->GetSlotName(), nextAnim, &slotSetup );
}

void CEdBehaviorGraphSlotTester::StopAnim()
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( instance->HasSlotAnimation( GetSlotName() ) )
	{
		instance->StopSlotAnimation( GetSlotName() );
	}

	FinishAnim();
}

void CEdBehaviorGraphSlotTester::FinishAnim()
{
	m_slot = NULL;

	/*SetFreezeUserElem( false );

	UpdateAnimList();

	UpdateSlotDisp();*/
}

void CEdBehaviorGraphSlotTester::OnReset()
{
	FillSlotChoice();
	FillAnimTree();
	UpdateSlotDisp();

	Enable( !GetEditor()->IsDebugMode() );
}

void CEdBehaviorGraphSlotTester::OnPanelClose()
{
	StopAnim();
}

void CEdBehaviorGraphSlotTester::OnInstanceReload()
{
	StopAnim();
}

void CEdBehaviorGraphSlotTester::OnTick( Float dt )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( m_slot && m_slot->IsSlotActive( *instance ) )
	{
		UpdateSlotDisp();

		m_paintTimer = Clamp( m_paintTimer + m_paintTimerSign * dt, 0.f, PAINT_TIMER_DURATION );

		if ( m_paintTimer >= PAINT_TIMER_DURATION || m_paintTimer <= 0.f )
		{
			m_paintTimerSign *= -1.f;
		}
	}
	else
	{
		m_paintTimer = 0.f;
	}
}

void CEdBehaviorGraphSlotTester::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	if ( IsAutoSelect() && nodes.Size() == 1 )
	{
		CBehaviorGraphAnimationBaseSlotNode* slotNode = Cast< CBehaviorGraphAnimationBaseSlotNode >( nodes[0] );
		if ( slotNode )
		{
			m_slotChoice->SetStringSelection( slotNode->GetName().AsChar() );
		}
	}
}

void CEdBehaviorGraphSlotTester::OnNodesDeselect()
{

}

void CEdBehaviorGraphSlotTester::OnPrintNodes( CEdGraphEditor* graphCanvas )
{
	// Mark selected node
	if ( m_slot )
	{
		const CEdGraphEditor::BlockLayoutInfo* layout = graphCanvas->GetBlockLayout( m_slot );

		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			Uint32 progress = Min< Uint32 >( ( Uint32 )( m_paintTimer / PAINT_TIMER_DURATION * 255.f ), 255 );

			Int32 col = 150;

			//CBehaviorGraphAnimationSlotNode* blendSlot = Cast< CBehaviorGraphAnimationSlotNode >( m_slot );
			//if ( blendSlot && blendSlot->IsBlendingWithCachedPose( *GetBehaviorGraphInstance() ) )
			//{
			//	col = 0;
			//}

			wxColour borderColor( 255, col, 0 );
			wxColour interiorColor( 255, col, 0, progress );

			wxPoint location( m_slot->GetPosition().X, m_slot->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			graphCanvas->DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 8 );
		}
	}
}

void CEdBehaviorGraphSlotTester::OnSlotChoice( wxCommandEvent& event )
{

}

wxString CEdBehaviorGraphSlotTester::GetTimeStr( Float time )
{
	Uint32 temp = (Uint32)( time * 100.f );

	Uint32 sek = time;
	Uint32 ms = temp - sek * 100;

	wxString sekStr;
	wxString msStr;
	
	sekStr = ( sek < 10 ) ? wxString::Format( wxT("0%d"), sek ) : wxString::Format( wxT("%d"), sek );
	msStr = ( ms < 10 ) ? wxString::Format( wxT(":0%d"), ms ) : wxString::Format( wxT(":%d"), ms );

	return sekStr + msStr;
}

wxString CEdBehaviorGraphSlotTester::GetProgressStr( Float progress )
{
	if ( progress < 0.1f )
	{
		return wxString::Format( wxT("0%d"), (Uint32)(progress*100.f) );
	}
	else
	{
		return wxString::Format( wxT("%d"), (Uint32)(progress*100.f) );
	}
}

void CEdBehaviorGraphSlotTester::UpdateSlotDisp()
{
	Float duration = 0.f;
	Float progress = 0.f;
	Float time = 0.f;
	Bool blendIn = false;
	Bool blendOut = false;
	Float blendInTimer = 0.f;
	Float blendOutTimer = 0.f;

	if ( m_slot )
	{
		CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
		if ( instance )
		{
			time = m_slot->GetAnimTime( *instance );
			duration = m_slot->GetAnimDuration( *instance );
			progress = m_slot->GetAnimProgress( *instance );

			CBehaviorGraphAnimationSlotNode* blendSlot = Cast< CBehaviorGraphAnimationSlotNode >( m_slot );
			if ( blendSlot )
			{
				time = blendSlot->GetLocalTime( *instance );
				blendIn = blendSlot->IsBlendIn( *instance );
				blendOut = blendSlot->IsBlendOut( *instance );

				if ( blendIn )
				{
					ASSERT( !blendOut );
					blendInTimer = blendSlot->GetBlendTimer( *instance );
				}
				else if ( blendOut )
				{
					ASSERT( !blendIn );
					blendOutTimer = blendSlot->GetBlendTimer( *instance );
				}
			}
		}
	}

	m_dispDuration->SetLabel( wxT("Duration: ") + GetTimeStr( duration ) );
	m_dispProgress->SetLabel( wxT("Progress: ") + GetProgressStr( progress ) + wxT("%") );
	m_dispTime->SetLabel( wxT("Time: ") + GetTimeStr( time ) );
	m_dispBlendIn->SetLabel( wxT("Blend in   : ") + GetTimeStr( blendInTimer ) );
	m_dispBlendOut->SetLabel( wxT("Blend out: ") + GetTimeStr( blendOutTimer ) );
	m_dispBlendInCheck->SetValue( blendIn );
	m_dispBlendOutCheck->SetValue( blendOut );
}

void CEdBehaviorGraphSlotTester::OnPlay( wxCommandEvent& event )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	m_localSlotAnimation = m_slotAnimations;
	m_slot = Cast< CBehaviorGraphAnimationBaseSlotNode >( instance->FindAnimSlot( GetSlotName() ) );

	if ( m_localSlotAnimation.Size() > 0 && m_slot )
	{
		SetFreezeUserElem( true );

		if ( !PlayNextAnim() )
		{
			StopAnim();	
		}
	}
}

void CEdBehaviorGraphSlotTester::OnStop( wxCommandEvent& event )
{
	StopAnim();
}

void CEdBehaviorGraphSlotTester::SetFreezeUserElem( Bool flag )
{
	Bool enabled = !flag;

	/*m_animTree->Enable( enabled );
	m_animList->Enable( enabled );
	m_slotChoice->Enable( enabled );
	
	XRCCTRL( *this, "buttPlay", wxButton )->Enable( enabled );

	XRCCTRL( *this, "Loop", wxCheckBox )->Enable( enabled );
	XRCCTRL( *this, "autoSelect", wxCheckBox )->Enable( enabled );
	XRCCTRL( *this, "blendIn", wxTextCtrl )->Enable( enabled );
	XRCCTRL( *this, "blendOut", wxTextCtrl )->Enable( enabled );

	XRCCTRL( *this, "buttAdd", wxButton )->Enable( enabled );
	XRCCTRL( *this, "buttRemove", wxButton )->Enable( enabled );

	Refresh();*/
}

void CEdBehaviorGraphSlotTester::OnAdd( wxCommandEvent& event )
{
	wxTreeItemId item = m_animTree->GetSelection();

	if ( item.IsOk() && m_animTree->GetChildrenCount( item, true ) == 0 )
	{
		CName animation( m_animTree->GetItemText( item ) );

		ASSERT( GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( animation ) );

		m_slotAnimations.PushBack( animation );

		UpdateAnimList();
	}
}

void CEdBehaviorGraphSlotTester::OnRemove( wxCommandEvent& event )
{
	Int32 selection = m_animList->GetSelection();
	if ( selection != -1 )
	{
		m_slotAnimations.Erase( m_slotAnimations.Begin() + selection );

		UpdateAnimList();
	}
}

Bool CEdBehaviorGraphSlotTester::IsAutoSelect() const
{
	wxCheckBox* check = XRCCTRL( *this, "autoSelect", wxCheckBox );
	return check->IsChecked();
}

Bool CEdBehaviorGraphSlotTester::IsLooped()	const
{
	return XRCCTRL( *this, "Loop", wxCheckBox )->GetValue();
}

Float CEdBehaviorGraphSlotTester::GetBlendIn() const
{
	wxTextCtrl* blend = XRCCTRL( *this, "blendIn", wxTextCtrl );
	Float value = 0.f;
	FromString( blend->GetValue().wc_str(), value );
	return value;
}

Float CEdBehaviorGraphSlotTester::GetBlendOut() const
{
	wxTextCtrl* blend = XRCCTRL( *this, "blendOut", wxTextCtrl );
	Float value = 0.f;
	FromString( blend->GetValue().wc_str(), value );
	return value;
}

Bool CEdBehaviorGraphSlotTester::IsAnimLooped() const
{
	return XRCCTRL( *this, "animLoop", wxCheckBox )->GetValue();
}

CName CEdBehaviorGraphSlotTester::GetSlotName() const
{
	return CName( m_slotChoice->GetStringSelection().wc_str() );
}

void CEdBehaviorGraphSlotTester::UpdateAnimList( Int32 sel )
{
	m_animList->Freeze();
	m_animList->Clear();

	for ( Uint32 i=0; i<m_slotAnimations.Size(); ++i )
	{
		if ( sel == (Int32)i )
		{
			String item = TXT("--> ") + m_slotAnimations[i];
			m_animList->AppendString( item.AsChar() );
		}
		else
		{
			m_animList->AppendString( m_slotAnimations[i].AsString().AsChar() );
		}
	}

	m_animList->Thaw();
	m_animList->Refresh();
}

void CEdBehaviorGraphSlotTester::FillSlotChoice()
{
	m_slotChoice->Freeze();
	m_slotChoice->Clear();

	TDynArray< String > slots;

	if ( GetBehaviorGraphInstance() )
	{
		GetBehaviorGraphInstance()->EnumAnimationSlots( slots );
	}

	for ( Uint32 i=0; i<slots.Size(); ++i )
	{
		m_slotChoice->AppendString( slots[i].AsChar() );
	}

	m_slotChoice->SetSelection( -1 );
	m_slotChoice->Thaw();
}

void CEdBehaviorGraphSlotTester::FillAnimTree()
{
	m_animTree->Freeze();
	m_animTree->DeleteAllItems();

	wxTreeItemId root = m_animTree->AddRoot( wxT("Animations") );

	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( ac )
	{
		const TSkeletalAnimationSetsArray& sets = ac->GetAnimationContainer()->GetAnimationSets();

		for( auto it = sets.Begin(), end = sets.End(); it != end; ++it )
		{
			// Add set
			CSkeletalAnimationSet* set = ( *it ).Get();
			wxString setName = set->GetFile()->GetFileName().StringBefore(TXT(".")).AsChar();

			wxTreeItemId localRoot = m_animTree->AppendItem( root, setName );

			// Add animations
			TDynArray< CSkeletalAnimationSetEntry* > animation;
			set->GetAnimations( animation );

			for ( Uint32 i=0; i<animation.Size(); ++i )
			{
				if ( animation[i]->GetAnimation() )
				{
					wxString animName( animation[i]->GetAnimation()->GetName().AsString().AsChar() );
					m_animTree->AppendItem( localRoot, animName );
				}
			}
		}

		m_animTree->Expand( root );
	}

	m_animTree->Thaw();
	m_animTree->Refresh();
}
