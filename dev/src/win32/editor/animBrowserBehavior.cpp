/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphAnimationBlendSlotNode.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/actor.h"
#include "../../common/core/feedback.h"

#include "animBrowserBehavior.h"
#include "animBrowserPreview.h"
#include "behaviorSlotTester.h"

#include "undoManager.h"
#include "undoAnimBrowser.h"

//////////////////////////////////////////////////////////////////////////

CLookAtTargetItem::CLookAtTargetItem( CAnimBrowserBehaviorPage* page, Bool isStatic /* = true */ )
	: m_page( page )
	, m_static( isStatic )
{
}

IPreviewItemContainer* CLookAtTargetItem::GetItemContainer() const 
{ 
	return m_page; 
}

Bool CLookAtTargetItem::IsStatic() const
{
	return m_static;
}

Bool CLookAtTargetItem::HasCaption() const
{
	return !IsStatic();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CAnimBrowserBehaviorPage, wxPanel )
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID("notebook"), CAnimBrowserBehaviorPage::OnPageChanged )
	EVT_MENU( XRCID( "lookAtButton" ), CAnimBrowserBehaviorPage::OnLookAtTool )
	EVT_CHOICE( XRCID( "choiceBehInstance" ), CAnimBrowserBehaviorPage::OnBehInstanceSelected )
	EVT_CHOICE( XRCID( "choiceBehSlot" ), CAnimBrowserBehaviorPage::OnAnimSlotSelected )
	EVT_BUTTON( XRCID( "buttBehAdd" ), CAnimBrowserBehaviorPage::OnAddAnim )
	EVT_BUTTON( XRCID( "buttBehRemove" ), CAnimBrowserBehaviorPage::OnRemoveAnim )
	EVT_LISTBOX( XRCID( "behAnimSelect" ), CAnimBrowserBehaviorPage::OnAnimListClick )
	EVT_LISTBOX_DCLICK( XRCID( "behAnimSelect" ), CAnimBrowserBehaviorPage::OnAnimListDClick )
	EVT_BUTTON( XRCID( "buttPlay" ), CAnimBrowserBehaviorPage::OnPlay )
	EVT_BUTTON( XRCID( "buttStop" ), CAnimBrowserBehaviorPage::OnStop )
	EVT_BUTTON( XRCID( "buttCatDrawAsync" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatHolsterAsync" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatDrawLatent" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatHolsterLatent" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatDrawTwoLatent" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatHolsterTwoLatent" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatDrawAndAttack" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatDrawTwoInstant" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatHolsterTwoInstant" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_BUTTON( XRCID( "buttCatHolsterInHands" ), CAnimBrowserBehaviorPage::OnItemAction )
	EVT_CHOICE( XRCID( "itemWeaponCatChoice" ), CAnimBrowserBehaviorPage::OnItemCategory )
	EVT_CHOICE( XRCID( "itemWeaponCatChoice2" ), CAnimBrowserBehaviorPage::OnItemCategory2 )
	EVT_BUTTON( XRCID( "gmAnimPrev" ), CAnimBrowserBehaviorPage::OnGMAnimPrev )
	EVT_BUTTON( XRCID( "gmAnimNext" ), CAnimBrowserBehaviorPage::OnGMAnimNext )
	EVT_CHOICE( XRCID( "gmAnimChoice" ), CAnimBrowserBehaviorPage::OnGMAnimChoice )
END_EVENT_TABLE()

const Vector CAnimBrowserBehaviorPage::DEFAULT_TARGET( 0.f, 2.f, 1.7f );

CAnimBrowserBehaviorPage::CAnimBrowserBehaviorPage( wxWindow* parent, CEdAnimBrowser* browser, CEdUndoManager* undoManager )
	: wxPanel( parent )
	, m_browser( browser )
	, m_slot( NULL )
	, m_itemLatentAction( -1 )
	, m_gameplayMimicPlayer( NULL )
	, m_active( false )
	, m_undoManager( undoManager )
{
	m_slotListener.m_page = this;
	m_gameplayMimicListener.m_page = this;

	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimBrowserBehPanel") );

	m_notebook = XRCCTRL( *this, "notebook", wxNotebook );

	m_animTree = XRCCTRL( *this, "behAnimTree", wxTreeCtrl );
	m_animList = XRCCTRL( *this, "behAnimSelect", wxListBox );
	m_selAnimName = XRCCTRL( *this, "textSelectedAnimation", wxStaticText );

	m_dispDuration = XRCCTRL( *this, "dispDuration", wxStaticText );
	m_dispProgress = XRCCTRL( *this, "dispProgress", wxStaticText );
	m_dispTime = XRCCTRL( *this, "dispTime", wxStaticText );
	m_dispBlendIn = XRCCTRL( *this, "dispBlendIn", wxStaticText );
	m_dispBlendOut = XRCCTRL( *this, "dispBlendOut", wxStaticText );
	m_dispBlendInCheck = XRCCTRL( *this, "dispBlendInCheck", wxCheckBox );
	m_dispBlendOutCheck = XRCCTRL( *this, "dispBlendOutCheck", wxCheckBox );

	// Prop panel
	{
		wxPanel* rp = XRCCTRL( *this, "gmPropPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_gmAnimProp = new CEdPropertiesPage( rp, settings, m_undoManager );
		sizer1->Add( m_gmAnimProp, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();
}

void CAnimBrowserBehaviorPage::DestroyPanel()
{
	// Destroy item container
	DestroyItemContainer();
}

void CAnimBrowserBehaviorPage::EnablePanel( Bool flag )
{
	if ( m_active == flag )
	{
		return;
	}
	m_active = flag;

	if ( flag )
	{
		FillAnimationTree();

		FillBehaviorInstances();

		ActivateBehaviorStack( true );

		InitItemContainer();

		CreateLookAtTargets();

		ShowItems( false );

		FillItemList();

		RestoreCachedBehaviorInstance();

		ChangePage( 0 );
	}
	else
	{
		if ( m_slot )
		{
			StopAnim();
		}

		if ( m_lookAtTool )
		{
			LookAtEnabled( false );
		}

		ActivateBehaviorStack( false );

		DestroyItemContainer();

		ChangePage( PAGE_LAST );
	}

	m_slotAnimations.Clear();
	m_localSlotAnimation.Clear();

	UpdateAnimList();

	OnSelectedAnimation();

	UpdateLookAtToolIcon();
}

void CAnimBrowserBehaviorPage::OnSelectedAnimation()
{
	if ( m_browser->m_selectedAnimation )
	{
		m_selAnimName->SetLabel( m_browser->m_selectedAnimation->GetName().AsString().AsChar() );
	}
	else
	{
		m_selAnimName->SetLabel( wxT("<empty>") );
	}

	m_selAnimName->SetWindowStyle( wxALIGN_CENTRE );

	if ( m_browser->m_selectedAnimation && m_browser->m_selectedAnimation->GetAnimation() )
	{
		m_gmAnimProp->SetObject( m_browser->m_selectedAnimation->GetAnimation() );
	}
	else
	{
		m_gmAnimProp->SetNoObject();
	}
}

void CAnimBrowserBehaviorPage::OnRefreshAnimation()
{
	CAnimatedComponent* ac = m_browser->m_animatedComponent;
	if ( ac )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();
		if( m_browser->m_playedAnimation )
		{
			if ( m_browser->m_playedAnimation->IsPaused() == true && stack->HasFrozenPose() == false )
			{
				stack->FreezePose();
			}
			else if ( m_browser->m_playedAnimation->IsPaused() == false && stack->HasFrozenPose() == true )
			{
				stack->UnfreezePose();
			}
		}
	}
}

void CAnimBrowserBehaviorPage::Tick( Float timeDelta )
{
	UpdateLookAtTargets();

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( m_slot && m_slot->IsSlotActive( *instance ) )
	{
		UpdateSlotDisp();

		if ( m_browser->m_playedAnimation && m_slot->GetAnimation( *instance )->GetName() == m_browser->m_playedAnimation->GetName() )
		{
			m_browser->m_playedAnimation->SetTime( m_slot->GetAnimTime( *instance ) );
		}
		else
		{
			m_browser->m_playedAnimation->SetTime( 0.f );
		}
	}

	if ( m_itemLatentAction != -1 )
	{
		// Check item latent action
		CActor* actor = GetPreviewActor();
		if ( actor && !SItemEntityManager::GetInstance().HasActorAnyLatentAction( actor ) )
		{
			EndItemLatentAction();
		}
	}

	if ( m_notebook )
	{
		Int32 page = m_notebook->GetSelection();

		switch ( page )
		{
		case PAGE_SLOT :
			TickPageSlot();
			break;
		case PAGE_ITEM :
			TickPageItem();
			break;
		case PAGE_GAMEPLAY_MIMIC :
			TickPageGameplayMimic();
			break;
		}
	}
}

CLookAtTargetItem* CAnimBrowserBehaviorPage::FindItemByNode( const CNode* node )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[i]->GetComponent() == node )
		{
			return static_cast< CLookAtTargetItem* >( m_items[i] );
		}
	}

	ASSERT( 0 );
	return NULL;
}

CLookAtTargetItem* CAnimBrowserBehaviorPage::FindItemByName( const String& name )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		if ( m_items[i]->GetName() == name )
		{
			return static_cast< CLookAtTargetItem* >( m_items[i] );
		}
	}

	ASSERT( 0 );
	return NULL;
}

void CAnimBrowserBehaviorPage::OnSelectItem( IPreviewItem* item )
{
	if ( item )
	{
		CLookAtTargetItem* targetItem = static_cast< CLookAtTargetItem* >( item );

		m_browser->m_previewPanel->EnableWidgets( targetItem->IsStatic() == false  );

		m_targetName = targetItem->GetName();
	}
}

void CAnimBrowserBehaviorPage::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	HandleItemSelection( objects );
}

void CAnimBrowserBehaviorPage::Save( CUserConfigurationManager& config )
{
	config.Write( TXT("CachedInstanceName"), m_cachedInstanceName.AsString() );
	config.Write( TXT("CachedCategory"), m_cachedCategory );
	config.Write( TXT("CachedCategory2"), m_cachedCategory2 );
}

void CAnimBrowserBehaviorPage::Load( CUserConfigurationManager& config )
{
	String str = config.Read( TXT("CachedInstanceName"), String::EMPTY );
	if ( !str.Empty() )
	{
		m_cachedInstanceName = CName( str );
	}

	m_cachedCategory = config.Read( TXT("CachedCategory"), String::EMPTY );
	m_cachedCategory2 = config.Read( TXT("CachedCategory2"), String::EMPTY );
}

void CAnimBrowserBehaviorPage::Reset( CUserConfigurationManager& config )
{
	config.DeleteEntry( TXT("CachedInstanceName") );
	config.DeleteEntry( TXT("CachedCategory") );
	config.DeleteEntry( TXT("CachedCategory2") );
}

void CAnimBrowserBehaviorPage::ChangePage( const unsigned int page )
{
	if ( !m_notebook ) return;

	Int32 oldPage = m_notebook->GetSelection();

	switch ( oldPage )
	{
	case PAGE_SLOT :
		HidePageSlot();
		break;
	case PAGE_ITEM :
		HidePageItem();
		break;
	case PAGE_GAMEPLAY_MIMIC :
		HidePageGameplayMimic();
		break;
	}

	if ( page < PAGE_LAST )
	{
		m_notebook->ChangeSelection( page );

		switch ( page )
		{
		case PAGE_SLOT :
			ShowPageSlot();
			break;
		case PAGE_ITEM :
			ShowPageItem();
			break;
		case PAGE_GAMEPLAY_MIMIC :
			ShowPageGameplayMimic();
			break;
		}
	}
}

void CAnimBrowserBehaviorPage::LookAtEnabled( Bool flag )
{
	if ( flag )
	{
		if ( m_targetName.Empty() == false )
		{
			OnSelectItem( FindItemByName( m_targetName ) );
		}
	}
	else
	{
		//...
	}

	ShowItems( flag );

	m_lookAtTool = flag;

	UpdateLookAtToolIcon();
}

void CAnimBrowserBehaviorPage::CreateLookAtTarget( Uint32 num, const Vector& pos )
{
	CLookAtTargetItem* item = new CLookAtTargetItem( this );
	item->Init( ToString( num ) );
	item->SetPosition( pos );

	if ( item->IsStatic() )
	{
		item->SetColor( Color::YELLOW );
	}
	else
	{
		item->SetColor( Color::RED );
	}

	AddItem( item );
}

void CAnimBrowserBehaviorPage::CreateLookAtTargets()
{
	Float h = 0.f;
	Float x = 2.5f;

	Vector offset( 0.f, 2.f, 0.f );

	for ( Uint32 i=0; i<2; i++ )
	{
		CreateLookAtTarget( i * 8 + 0, Vector( -x, x, h ) + offset );
		CreateLookAtTarget( i * 8 + 1, Vector( 0, x, h ) + offset );
		CreateLookAtTarget( i * 8 + 2, Vector( x, x, h ) + offset );
		CreateLookAtTarget( i * 8 + 3, Vector( x, 0, h ) + offset );
		CreateLookAtTarget( i * 8 + 4, Vector( x, -x, h ) + offset );
		CreateLookAtTarget( i * 8 + 5, Vector( 0, -x, h ) + offset );
		CreateLookAtTarget( i * 8 + 6, Vector( -x, -x, h ) + offset );
		CreateLookAtTarget( i * 8 + 7, Vector( -x, 0, h ) + offset );

		h += 1.7f;
	}

	{
		CLookAtTargetItem* item = new CLookAtTargetItem( this, false );
		item->Init( TXT("Dynamic") );
		item->SetPosition( DEFAULT_TARGET );
		AddItem( item );
	}
}

Vector CAnimBrowserBehaviorPage::GetLookAtTarget() const
{
	TDynArray< CNode* > nodes;
	m_browser->m_previewPanel->GetSelectionManager()->GetSelectedNodes( nodes );

	if ( nodes.Size() > 0 )
	{
		ASSERT( nodes.Size() == 1 );

		return nodes[0]->GetWorldPosition();
	}

	return DEFAULT_TARGET;
}

void CAnimBrowserBehaviorPage::UpdateLookAtTargets()
{
	CAnimatedComponent* ac = m_browser->m_animatedComponent;

	if ( ac )
	{
		if ( m_itemContainer )
		{
			m_itemContainer->SetPosition( ac->GetWorldPosition() );
		}

		CActor* actor = Cast< CActor >( ac->GetEntity() );
		if ( actor )
		{
			if ( m_lookAtTool )
			{
				actor->DisableLookAts();

				SLookAtDebugStaticInfo info;
				info.m_target = GetLookAtTarget();
				actor->EnableLookAt( info );
			}
			else if ( actor->IsLookAtEnabled() )
			{
				actor->DisableLookAts();
			}
		}
	}
}

void CAnimBrowserBehaviorPage::ShowItems( Bool flag )
{
	for ( Uint32 i=0; i<m_items.Size(); ++i )
	{
		m_items[i]->SetVisible( flag );
	}
}

void CAnimBrowserBehaviorPage::FillBehaviorInstances()
{
	wxChoice* instChoice = XRCCTRL( *this, "choiceBehInstance", wxChoice );

	CAnimatedComponent* ac = m_browser->m_animatedComponent;
	if ( ac )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();
		if ( stack )
		{
			// Instance slots
			TDynArray< CName > instSlots;
			ac->GetBehaviorInstanceSlots( instSlots );

			CName currInstName = stack->GetActiveTopInstance();

			instChoice->Freeze();
			instChoice->Clear();

			for ( Uint32 i=0; i<instSlots.Size(); ++i )
			{
				instChoice->AppendString( instSlots[i].AsString().AsChar() );
			}

			instChoice->Thaw();

			if ( !( !m_cachedInstanceName.Empty() && instChoice->SetStringSelection( m_cachedInstanceName.AsString().AsChar() ) ) )
			{
				instChoice->SetStringSelection( currInstName.AsString().AsChar() );
			}
		}
	}

	FillBehaviorSlots();
}

void CAnimBrowserBehaviorPage::FillBehaviorSlots()
{
	wxChoice* slotChoice = XRCCTRL( *this, "choiceBehSlot", wxChoice );

	CAnimatedComponent* ac = m_browser->m_animatedComponent;
	if ( ac )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();
		if ( stack )
		{
			// Instance slots
			TDynArray< CName > instSlots;
			ac->GetBehaviorInstanceSlots( instSlots );

			CName currInstName = stack->GetActiveTopInstance();

			// Animation slots
			CBehaviorGraphInstance* instance = stack->GetBehaviorGraphInstance( currInstName );
			if ( instance )
			{
				TDynArray< String > animSlots;
				instance->EnumAnimationSlots( animSlots );

				slotChoice->Freeze();
				slotChoice->Clear();

				for ( Uint32 i=0; i<animSlots.Size(); ++i )
				{
					slotChoice->AppendString( animSlots[i].AsChar() );
				}

				slotChoice->Thaw();

				if ( animSlots.Size() > 0 )
				{
					slotChoice->SetStringSelection( animSlots[0].AsChar() );
				}
			}
			else
			{
				ASSERT( instance );
			}
		}
	}
}

void CAnimBrowserBehaviorPage::FillAnimationTree()
{
	m_animTree->Freeze();
	m_animTree->DeleteAllItems();

	TSkeletalAnimationSetsArray animSets;
	m_browser->GetAnimsets( animSets );

	m_animTree->AddRoot( TXT("Animations") );

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();

		String setName = m_browser->GetAnimSetName( set );

		wxTreeItemId currSetItem = m_animTree->AppendItem( m_animTree->GetRootItem(), setName.AsChar() );
		m_animTree->SetItemData( currSetItem, new SerializableItemWrapper( set ) );

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		m_browser->GetAnimationsFromAnimset( set, setAnims);

		for( Uint32 j=0; j<setAnims.Size(); ++j )
		{
			CSkeletalAnimationSetEntry *currAnim = setAnims[ j ];
			if ( currAnim->GetAnimation() )
			{
				wxString itemName( currAnim->GetAnimation()->GetName().AsString().AsChar() );
				wxTreeItemId item = m_animTree->AppendItem( currSetItem, itemName, -1, -1, new SerializableItemWrapper( currAnim ) );
			}
		}
	}

	m_animTree->SortChildren( m_animTree->GetRootItem() );

	m_animTree->Expand( m_animTree->GetRootItem() );

	m_animTree->Thaw();
}

void CAnimBrowserBehaviorPage::UpdateLookAtToolIcon()
{
	wxToolBar* tb = XRCCTRL( *this, "toolbarBehControls", wxToolBar );
	tb->ToggleTool( XRCID( "lookAtButton" ), m_lookAtTool );
}

void CAnimBrowserBehaviorPage::ActivateBehaviorStack( Bool flag )
{
	CAnimatedComponent* ac = m_browser->m_animatedComponent;
	if ( ac )
	{
		flag ? ac->GetBehaviorStack()->Activate() : ac->GetBehaviorStack()->Deactivate();
	}
}

void CAnimBrowserBehaviorPage::RestoreCachedBehaviorInstance()
{
	wxChoice* instChoice = XRCCTRL( *this, "choiceBehInstance", wxChoice );
	instChoice->SetStringSelection( m_cachedInstanceName.AsString().AsChar() );

	wxCommandEvent fake;
	OnBehInstanceSelected( fake );
}

void CAnimBrowserBehaviorPage::ActivateBehaviorInstance( const CName& instNameStr )
{
	m_instName = instNameStr;

	CAnimatedComponent* ac = m_browser->m_animatedComponent;
	if ( ac && ac->GetBehaviorStack() )
	{
		ASSERT( ac->GetBehaviorStack()->IsActive() );

		TDynArray< CName > instances;
		instances.PushBack( m_instName );

		ac->GetBehaviorStack()->ActivateBehaviorInstances( instances );
	}

	m_cachedInstanceName = instNameStr;

	FillBehaviorSlots();
}

void CAnimBrowserBehaviorPage::SelectBehaviorSlot( const String& slotName )
{
	m_animSlotName = slotName;
}

void CAnimBrowserBehaviorPage::UpdateAnimList( Int32 sel )
{
	m_animList->Freeze();
	m_animList->Clear();

	for ( Uint32 i=0; i<m_slotAnimations.Size(); ++i )
	{
		String item;

		if ( sel == (Int32)i )
		{
			item = TXT("-> ");
		}
		else
		{
			item = TXT("     ");
		}

		item += m_slotAnimations[i].AsString();

		m_animList->AppendString( item.AsChar() );
	}

	m_animList->Thaw();
	m_animList->Refresh();
}

void CAnimBrowserBehaviorPage::UpdateSlotDisp()
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

		if ( instance == NULL )
		{
			return;
		}

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

	m_dispDuration->SetLabel( wxT("Duration: ") + CEdBehaviorGraphSlotTester::GetTimeStr( duration ) );
	m_dispProgress->SetLabel( wxT("Progress: ") + CEdBehaviorGraphSlotTester::GetProgressStr( progress ) + wxT("%") );
	m_dispTime->SetLabel( wxT("Time: ") + CEdBehaviorGraphSlotTester::GetTimeStr( time ) );
	m_dispBlendIn->SetLabel( wxT("Blend in   : ") + CEdBehaviorGraphSlotTester::GetTimeStr( blendInTimer ) );
	m_dispBlendOut->SetLabel( wxT("Blend out: ") + CEdBehaviorGraphSlotTester::GetTimeStr( blendOutTimer ) );
	m_dispBlendInCheck->SetValue( blendIn );
	m_dispBlendOutCheck->SetValue( blendOut );
}

void CAnimBrowserBehaviorPage::SetFreezeUserElem( Bool flag )
{
	Bool enabled = !flag;

	m_animTree->Enable( enabled );
	//m_animList->Enable( enabled );

	wxChoice* slotChoice = XRCCTRL( *this, "choiceBehSlot", wxChoice );
	wxChoice* instChoice = XRCCTRL( *this, "choiceBehInstance", wxChoice );

	slotChoice->Enable( enabled );
	instChoice->Enable( enabled );

	XRCCTRL( *this, "buttPlay", wxButton )->Enable( enabled );

	XRCCTRL( *this, "Loop", wxCheckBox )->Enable( enabled );
	XRCCTRL( *this, "blendIn", wxTextCtrl )->Enable( enabled );
	XRCCTRL( *this, "blendOut", wxTextCtrl )->Enable( enabled );

	XRCCTRL( *this, "buttBehAdd", wxButton )->Enable( enabled );
	XRCCTRL( *this, "buttBehRemove", wxButton )->Enable( enabled );

	Refresh();
}

CBehaviorGraphInstance* CAnimBrowserBehaviorPage::GetBehaviorGraphInstance() const
{
	if ( m_instName != CName::NONE )
	{
		CAnimatedComponent* ac = m_browser->m_animatedComponent;
		if ( ac )
		{
			CBehaviorGraphStack* stack = ac->GetBehaviorStack();
			if ( stack )
			{
				return stack->GetBehaviorGraphInstance( m_instName );
			}
		}
	}

	return NULL;
}

Bool CAnimBrowserBehaviorPage::PlayNextAnim()
{
	if ( !m_slot )
	{
		return false;
	}

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( instance == NULL )
	{
		ASSERT( instance );
		return false;
	}

	Uint32 animNum = m_slotAnimations.Size() - m_localSlotAnimation.Size();
	UpdateAnimList( (Int32)animNum );

	CName nextAnim = m_localSlotAnimation[0];
	m_localSlotAnimation.Erase( m_localSlotAnimation.Begin() );

	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = GetBlendIn();
	slotSetup.m_blendOut = GetBlendOut();
	slotSetup.m_looped = IsLooped();
	slotSetup.m_listener = &m_slotListener;

	return instance->PlaySlotAnimation( m_slot->GetSlotName(), nextAnim, &slotSetup );
}

void CAnimBrowserBehaviorPage::StopAnim()
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( instance == NULL )
	{
		ASSERT( instance );
		return;
	}

	instance->StopSlotAnimation( GetSlotName() );

	FinishAnim();
}

void CAnimBrowserBehaviorPage::FinishAnim()
{
	m_slot = NULL;

	SetFreezeUserElem( false );

	UpdateAnimList();

	UpdateSlotDisp();
}

Bool CAnimBrowserBehaviorPage::IsLooped()	const
{
	return XRCCTRL( *this, "Loop", wxCheckBox )->GetValue();
}

Float CAnimBrowserBehaviorPage::GetBlendIn() const
{
	wxTextCtrl* blend = XRCCTRL( *this, "blendIn", wxTextCtrl );
	Float value = 0.f;
	FromString( blend->GetValue().wc_str(), value );
	return value;
}

Float CAnimBrowserBehaviorPage::GetBlendOut() const
{
	wxTextCtrl* blend = XRCCTRL( *this, "blendOut", wxTextCtrl );
	Float value = 0.f;
	FromString( blend->GetValue().wc_str(), value );
	return value;
}

CName CAnimBrowserBehaviorPage::GetSlotName() const
{
	wxChoice* slotChoice = XRCCTRL( *this, "choiceBehSlot", wxChoice );
	return CName( slotChoice->GetStringSelection().wc_str() );
}

void CAnimBrowserBehaviorPage::SlotPageListener::OnSlotAnimationEnd(	const CBehaviorGraphAnimationBaseSlotNode * sender, 
																		CBehaviorGraphInstance& instance, 
																		EStatus status )
{
	if ( m_page->m_localSlotAnimation.Size() > 0 )
	{
		if ( !m_page->PlayNextAnim() )
		{
			m_page->StopAnim();
		}
	}
	else if ( m_page->IsLooped() )
	{
		m_page->m_localSlotAnimation = m_page->m_slotAnimations;

		if ( m_page->m_localSlotAnimation.Size() > 0 ) 
		{
			OnSlotAnimationEnd( sender, instance, status );
		}
	}
	else
	{
		m_page->FinishAnim();
	}
}

void CAnimBrowserBehaviorPage::GameplayMimicPageListener::OnSlotAnimationEnd(	const CBehaviorGraphAnimationBaseSlotNode * sender, 
																				CBehaviorGraphInstance& instance, 
																				EStatus status )
{
	if ( m_page->m_gameplayMimicPlayer && m_page->m_browser->m_playedAnimation )
	{
		m_page->PlayGameplayMimicAnimation( m_page->m_browser->m_playedAnimation->GetName() );
	}
}

//////////////////////////////////////////////////////////////////////////


void CAnimBrowserBehaviorPage::OnPageChanged(wxNotebookEvent& event)
{
	if ( event.GetSelection() != -1 )
	{
		ChangePage( (unsigned int)event.GetSelection() );
	}
}

void CAnimBrowserBehaviorPage::OnLookAtTool( wxCommandEvent& event )
{
	LookAtEnabled( event.IsChecked() );
}

void CAnimBrowserBehaviorPage::OnBehInstanceSelected( wxCommandEvent& event )
{
	wxChoice* instChoice = XRCCTRL( *this, "choiceBehInstance", wxChoice );

	String instNameStr = instChoice->GetStringSelection().wc_str();

	if ( instNameStr.Empty() == false )
	{
		ActivateBehaviorInstance( CName( instNameStr ) );
	}
}

void CAnimBrowserBehaviorPage::OnAnimSlotSelected( wxCommandEvent& event )
{
	wxChoice* slotChoice = XRCCTRL( *this, "choiceBehSlot", wxChoice );

	String slotName = slotChoice->GetStringSelection().wc_str();

	if ( slotName.Empty() == false )
	{
		SelectBehaviorSlot( slotName );
	}
}

void CAnimBrowserBehaviorPage::OnAddAnim( wxCommandEvent& event )
{
	wxTreeItemId item = m_animTree->GetSelection();

	if ( item.IsOk() && m_animTree->GetChildrenCount( item, true ) == 0 && m_browser->m_animatedComponent )
	{
		CName animation( m_animTree->GetItemText( item ) );

		ASSERT( m_browser->m_animatedComponent->GetAnimationContainer()->FindAnimation( animation ) );

		m_slotAnimations.PushBack( animation );

		UpdateAnimList();
	}
}

void CAnimBrowserBehaviorPage::OnRemoveAnim( wxCommandEvent& event )
{
	Int32 selection = m_animList->GetSelection();
	if ( selection != -1 )
	{
		m_slotAnimations.Erase( m_slotAnimations.Begin() + selection );

		UpdateAnimList();
	}
}

void CAnimBrowserBehaviorPage::OnAnimListClick( wxCommandEvent& event )
{
	
}

void CAnimBrowserBehaviorPage::OnAnimListDClick( wxCommandEvent& event )
{
	Int32 selection = m_animList->GetSelection();
	if ( selection != -1 && (Int32)m_slotAnimations.Size() > selection )
	{
		String animName = m_slotAnimations[ selection ].AsString();

		m_browser->SelectAnimation( animName );
	}
}

void CAnimBrowserBehaviorPage::OnPlay( wxCommandEvent& event )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if ( instance == NULL )
	{
		ASSERT( instance );
		return;
	}

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

void CAnimBrowserBehaviorPage::OnStop( wxCommandEvent& event )
{
	StopAnim();
}

//////////////////////////////////////////////////////////////////////////
// Items

CActor* CAnimBrowserBehaviorPage::GetPreviewActor()
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	if ( !instance )
	{
		return NULL;
	}

	return Cast< CActor >( instance->GetAnimatedComponent()->GetEntity() );
}

void CAnimBrowserBehaviorPage::ShowItemActionResult( Bool isOk )
{
	wxStaticText* text = XRCCTRL( *this, "itemTextRes", wxStaticText );
	if ( isOk )
	{
		text->SetLabel( wxT("Success") );
	}
	else
	{
		text->SetLabel( wxT("Failure") );
	}
}

void CAnimBrowserBehaviorPage::FillItemList()
{
	wxChoice* choice = XRCCTRL( *this, "itemWeaponCatChoice", wxChoice );
	wxChoice* choice2 = XRCCTRL( *this, "itemWeaponCatChoice2", wxChoice );

	choice->Freeze();
	choice2->Freeze();
	choice->Clear();
	choice2->Clear();

	if ( GCommonGame && GCommonGame->GetDefinitionsManager() )
	{
		TDynArray< CName > cats;
		GCommonGame->GetDefinitionsManager()->GetItemCategories( cats );

		wxArrayString strs;

		for ( Uint32 i=0; i<cats.Size(); ++i )
		{
			strs.push_back( cats[ i ].AsString().AsChar() );
		}

		strs.Sort();
		choice->Append( strs );
		choice2->Append( strs );

		if ( !( !m_cachedCategory.Empty() && choice->SetStringSelection( m_cachedCategory.AsChar() ) ) )
		{
			choice->SetStringSelection( wxT("opponent_weapon") );
		}
	}

	choice->Thaw();
	choice2->Thaw();
}

void CAnimBrowserBehaviorPage::OnItemCategory( wxCommandEvent& event )
{
	m_cachedCategory = event.GetString().wc_str();
}

void CAnimBrowserBehaviorPage::OnItemCategory2( wxCommandEvent& event )
{
	m_cachedCategory2 = event.GetString().wc_str();
}

void CAnimBrowserBehaviorPage::OnItemAction( wxCommandEvent& event )
{
	//ASSERT( m_itemLatentAction == -1 );

	wxChoice* choice = XRCCTRL( *this, "itemWeaponCatChoice", wxChoice );
	wxChoice* choice2 = XRCCTRL( *this, "itemWeaponCatChoice2", wxChoice );
	Int32 sel = choice->GetSelection();
	Int32 sel2 = choice->GetSelection();

	if ( sel != -1 )
	{
		String itemCat = choice->GetStringSelection().wc_str();

		CActor* actor = GetPreviewActor();
		if  ( !actor )
		{
			GFeedback->ShowMsg( TXT("Message"), TXT("Preview entity is not CActor") );
			return;
		}

		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( !inventory )
		{
			GFeedback->ShowMsg( TXT("Message"), TXT("Preview entity hasn't got inventory component") );
			return;
		}

		SItemUniqueId itemId = inventory->GetItemByCategory( CName( itemCat ), false );

		if ( event.GetId() == XRCID( "buttCatDrawAsync" ) )
		{
			ShowItemActionResult( actor->DrawItem( itemId, true ) );
		}
		else if ( event.GetId() == XRCID( "buttCatHolsterAsync" ) )
		{
			ShowItemActionResult( actor->HolsterItem( itemId, true ) );
		}
		else if ( event.GetId() == XRCID( "buttCatDrawLatent" ) )
		{
			ShowItemActionResult( actor->DrawItem( itemId, false ) );
			StartItemLatentAction( ILA_Draw );
		}
		else if ( event.GetId() == XRCID( "buttCatHolsterLatent" ) )
		{
			ShowItemActionResult( actor->HolsterItem( itemId, false ) );
			StartItemLatentAction( ILA_Holster );
		}
		else if ( event.GetId() == XRCID( "buttCatDrawAndAttack" ) )
		{
			ShowItemActionResult( actor->DrawWeaponAndAttack( itemId ) );
			StartItemLatentAction( ILA_Draw );
		}
		else if ( event.GetId() == XRCID( "buttCatHolsterInHands" ) )
		{
			CInventoryComponent* comp = actor->GetInventoryComponent();
			ShowItemActionResult( comp && comp->HolsterItem( comp->GetItemIdHeldInSlot( CNAME( l_weapon ) ), true) && comp->GetItemIdHeldInSlot( CNAME( r_weapon ) ) );
			StartItemLatentAction( ILA_Holster );
		}

		if ( sel2 != -1 )
		{
			String itemCat2 = choice2->GetStringSelection().wc_str();
			SItemUniqueId itemId2 = inventory->GetItemByCategory( CName( itemCat2 ), false );

			if ( event.GetId() == XRCID( "buttCatDrawTwoLatent" ) )
			{
				ShowItemActionResult( actor->DrawItem( itemId, false ) && actor->DrawItem( itemId2, false ) );
				StartItemLatentAction( ILA_Draw );
			}
			else if ( event.GetId() == XRCID( "buttCatHolsterTwoLatent" ) )
			{
				ShowItemActionResult(  actor->HolsterItem( itemId, false ) && actor->HolsterItem( itemId2, false ) );
				StartItemLatentAction( ILA_Holster );
			}
			else if ( event.GetId() == XRCID( "buttCatDrawTwoInstant" ) )
			{
				ShowItemActionResult( actor->DrawItem( itemId, true ) && actor->DrawItem( itemId2, true )  );
				StartItemLatentAction( ILA_Draw );
			}
			else if ( event.GetId() == XRCID( "buttCatHolsterTwoInstant" ) )
			{
				ShowItemActionResult( actor->HolsterItem( itemId, true) && actor->HolsterItem( itemId2, true ) );
				StartItemLatentAction( ILA_Holster );
			}
		}
	}
}

void CAnimBrowserBehaviorPage::OnGMAnimPrev( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "gmAnimChoice", wxChoice );

	Int32 selection = choice->GetSelection();
	if ( selection != -1 )
	{
		CSkeletalAnimationSetEntry* sel = m_browser->GetSelectedAnimationEntry();
		if ( sel )
		{
			CSkeletalAnimationSet* set = sel->GetAnimSet();
			if ( set )
			{
				const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();
				if ( selection - 1 >= 0 && selection - 1 < anims.SizeInt() )
				{
					selection -= 1;

					CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
					CName animName = anims[ selection ]->GetName();
					m_browser->SelectAnimation( animName.AsString() );

					choice->SetSelection( selection );

					PlayAnimationForMimicPage( animName );
				}
			}
		}
	}
}

void CAnimBrowserBehaviorPage::OnGMAnimNext( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "gmAnimChoice", wxChoice );

	Int32 selection = choice->GetSelection();
	if ( selection != -1 )
	{
		CSkeletalAnimationSetEntry* sel = m_browser->GetSelectedAnimationEntry();
		if ( sel )
		{
			CSkeletalAnimationSet* set = sel->GetAnimSet();
			if ( set )
			{
				const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();
				if ( anims.SizeInt() > selection + 1 )
				{
					selection += 1;

					CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
					CName animName = anims[ selection ]->GetName();
					m_browser->SelectAnimation( animName.AsString() );

					choice->SetSelection( selection );

					PlayAnimationForMimicPage( animName );
				}
			}
		}
	}
}

void CAnimBrowserBehaviorPage::OnGMAnimChoice( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "gmAnimChoice", wxChoice );

	Int32 selection = choice->GetSelection();
	if ( selection != -1 )
	{
		CSkeletalAnimationSetEntry* sel = m_browser->GetSelectedAnimationEntry();
		if ( sel )
		{
			CSkeletalAnimationSet* set = sel->GetAnimSet();
			if ( set )
			{
				const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();
				if ( anims.SizeInt() > selection )
				{
					CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
					CName animName = anims[ selection ]->GetName();
					m_browser->SelectAnimation( animName.AsString() );

					PlayAnimationForMimicPage( animName );
				}
			}
		}
	}
}

Bool CAnimBrowserBehaviorPage::IsWaitingForFinished() const
{
	return XRCCTRL( *this, "itemWaitForFinishCheck", wxCheckBox )->GetValue();
}

void CAnimBrowserBehaviorPage::StartItemLatentAction( EItemLatentAction action )
{
	m_itemLatentAction = action;

	if ( IsWaitingForFinished() )
	{
		XRCCTRL( *this, "buttCatDrawAsync", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatHolsterAsync", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatDrawLatent", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatHolsterLatent", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatDrawTwoLatent", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatDrawTwoInstant", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatHolsterTwoLatent", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatHolsterTwoInstant", wxButton )->Enable( false );
		XRCCTRL( *this, "buttCatDrawAndAttack", wxButton )->Enable( false );
	}
}

void CAnimBrowserBehaviorPage::EndItemLatentAction()
{
	m_itemLatentAction = -1;

	XRCCTRL( *this, "buttCatDrawAsync", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatHolsterAsync", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatDrawLatent", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatHolsterLatent", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatDrawTwoLatent", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatDrawTwoInstant", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatHolsterTwoLatent", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatHolsterTwoInstant", wxButton )->Enable( true );
	XRCCTRL( *this, "buttCatDrawAndAttack", wxButton )->Enable( true );
}

void CAnimBrowserBehaviorPage::PlayGameplayMimicAnimation( const CName& animation )
{
	SBehaviorSlotSetup setup;
	setup.m_listener = &m_gameplayMimicListener;
	setup.m_blendIn = 0.f;
	setup.m_blendOut = 0.f;
	m_gameplayMimicPlayer->PlayAnimation( animation, &setup );
}

void CAnimBrowserBehaviorPage::PlayAnimationForMimicPage( const CName& animation )
{
	if ( m_gameplayMimicPlayer )
	{
		StopAnimationForMimicPage();
	}

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	if ( instance )
	{
		m_gameplayMimicPlayer = new IBehaviorGraphSlotInterface();

		if ( !instance->GetSlot( CNAME( NPC_ANIM_SLOT ), *m_gameplayMimicPlayer ) )
		{
			delete m_gameplayMimicPlayer;
			m_gameplayMimicPlayer = NULL;
			return;
		}

		PlayGameplayMimicAnimation( animation );
	}
}

void CAnimBrowserBehaviorPage::StopAnimationForMimicPage()
{
	if ( m_gameplayMimicPlayer )
	{
		m_gameplayMimicPlayer->StopAnimation();
		delete m_gameplayMimicPlayer;
		m_gameplayMimicPlayer = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

void CAnimBrowserBehaviorPage::ShowPageSlot()
{

}

void CAnimBrowserBehaviorPage::ShowPageItem()
{

}

void CAnimBrowserBehaviorPage::ShowPageGameplayMimic()
{
	wxChoice* choice = XRCCTRL( *this, "gmAnimChoice", wxChoice );
	
	// Fill
	choice->Freeze();
	choice->Clear();

	CSkeletalAnimationSetEntry* sel = m_browser->GetSelectedAnimationEntry();
	Int32 choiceSel = 0;

	if ( sel )
	{
		CSkeletalAnimationSet* set = sel->GetAnimSet();
		if ( set )
		{
			const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();
			for ( Uint32 i=0; i<anims.Size(); ++i )
			{
				const CName& animName = anims[ i ]->GetName();

				choice->AppendString( animName.AsString().AsChar() );

				if ( sel == anims[ i ] )
				{
					choiceSel = i;
				}
			}
		}
	}

	choice->SetSelection( choiceSel );
	choice->Thaw();

	if ( sel )
	{
		PlayAnimationForMimicPage( sel->GetName() );
	}
}

void CAnimBrowserBehaviorPage::HidePageSlot()
{

}

void CAnimBrowserBehaviorPage::HidePageItem()
{

}

void CAnimBrowserBehaviorPage::HidePageGameplayMimic()
{
	StopAnimationForMimicPage();
}

void CAnimBrowserBehaviorPage::TickPageSlot()
{

}

void CAnimBrowserBehaviorPage::TickPageItem()
{

}

void CAnimBrowserBehaviorPage::TickPageGameplayMimic()
{
	if ( m_gameplayMimicPlayer )
	{
		if ( m_browser->m_playedAnimation && m_gameplayMimicPlayer->GetAnimationName() == m_browser->m_playedAnimation->GetName() )
		{
			CSyncInfo info;
			m_gameplayMimicPlayer->GetSyncInfo( info );
			m_browser->m_playedAnimation->SetTime( info.m_currTime );
		}
		else
		{
			m_browser->m_playedAnimation->SetTime( 0.f );
		}
	}
}
