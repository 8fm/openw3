
#include "build.h"
#include "animationParamPanel.h"
#include "../../common/engine/animationGameParams.h"

#define ID_ADD_PARAM		7001
#define ID_REMOVE_PARAM		7002

BEGIN_EVENT_TABLE( CEdAnimationParamPanel, CEdAnimationTreeBrowser )
	EVT_TREE_ITEM_MENU( XRCID( "tree" ), CEdAnimationParamPanel::OnTreeItemMenu )
END_EVENT_TABLE()

CEdAnimationParamPanel::CEdAnimationParamPanel( wxWindow* parent, CClass* paramClass, const CEdAnimationParamInitializer* initializer, CEdAnimationParamPanelListener* listener, Bool verticalStyle )
	: CEdAnimationTreeBrowser( parent, SEdAnimationTreeBrowserSettings( verticalStyle, nullptr ) )
	, m_paramClass( paramClass )
	, m_initializer( initializer )
	, m_listener( listener )
{
	m_supportsDragAndDrop = false;

	SetPropSetter( new CEdAnimationTreeBrowserParamSetter( paramClass ) );
}

CEdAnimationParamPanel::~CEdAnimationParamPanel()
{
	delete m_initializer;
}

void CEdAnimationParamPanel::RefreshParamPanel()
{
	FillAnimationTree( m_preview->GetAnimatedComponent() );
}

void CEdAnimationParamPanel::OnItemAdded( wxTreeItemId item, const CSkeletalAnimationSetEntry* animation )
{
	if ( animation->FindParamByClass( m_paramClass ) )
	{
		m_tree->SetItemTextColour( item, *wxRED );
	}
}

void CEdAnimationParamPanel::OnSelectAnimation( CSkeletalAnimationSetEntry* animation )
{
	if ( animation->FindParamByClass( m_paramClass ) && m_listener )
	{
		m_listener->OnAnimationParamSelectedAnimation( animation );
	}
}

void CEdAnimationParamPanel::OnTreeItemMenu( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if ( !item.IsOk() || item != m_tree->GetSelection() || !m_preview->GetAnimatedComponent() )
	{
		return;
	}

	SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( item );
	CSkeletalAnimationSetEntry* animation = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
	if ( animation )
	{
		if ( animation->FindParamByClass( m_paramClass ) )
		{
			wxMenu menu;

			menu.Append( ID_REMOVE_PARAM, TXT( "Remove param" ) );
			menu.Connect( ID_REMOVE_PARAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationParamPanel::OnRemoveParam ), NULL, this );

			PopupMenu( &menu );
		}
		else
		{
			wxMenu menu;

			menu.Append( ID_ADD_PARAM, TXT( "Add param" ) );
			menu.Connect( ID_ADD_PARAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationParamPanel::OnAddParam ), NULL, this );

			PopupMenu( &menu );
		}
	}
}

void CEdAnimationParamPanel::OnRemoveParam( wxCommandEvent& event )
{
	wxTreeItemId item = m_tree->GetSelection();
	if ( item.IsOk() )
	{
		SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( item );
		CSkeletalAnimationSetEntry* animation = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		if ( animation )
		{
			const ISkeletalAnimationSetEntryParam* param = animation->FindParamByClass( m_paramClass );
			if ( param )
			{
				CSkeletalAnimationSet* set = animation->GetAnimSet();
				if ( set && set->MarkModified() )
				{
					animation->RemoveParam( param );

					RefreshParamPanel();

					if ( m_listener )
					{
						m_listener->OnAnimationParamRemovedFromAnimation( animation, param );
					}

					set->Save();
				}
			}
		}
	}
}

void CEdAnimationParamPanel::OnAddParam( wxCommandEvent& event )
{
	wxTreeItemId item = m_tree->GetSelection();
	if ( item.IsOk() )
	{
		SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( item );
		CSkeletalAnimationSetEntry* animation = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		if ( animation )
		{
			const ISkeletalAnimationSetEntryParam* param = animation->FindParamByClass( m_paramClass );
			if ( !param )
			{
				ISkeletalAnimationSetEntryParam* newParam = m_paramClass->CreateObject< ISkeletalAnimationSetEntryParam >();
				if ( newParam )
				{
					CSkeletalAnimationSet* set = animation->GetAnimSet();
					if ( set && set->MarkModified() )
					{
						if ( m_initializer && !m_initializer->Initialize( newParam, animation, m_preview->GetAnimatedComponent() ) )
						{
							return;
						}

						animation->AddParam( newParam );

						set->Save();

						RefreshParamPanel();

						if ( m_listener )
						{
							m_listener->OnAnimationParamAddedToAnimation( animation, newParam );
						}	
					}
				}
			}
		}
	}
}
