/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityEditor.h"
#include "../../common/game/lookAtParam.h"
#include "../../common/game/entityParams.h"
#include "../../common/engine/animConstraintsParam.h"
#include "../../common/engine/animGlobalParam.h"
#include "../../common/engine/animMimicParam.h"
#include "../../common/engine/animBehaviorsAndSetsParam.h"
#include "entityEditorHelpers.h"

#include "../../games/r4/vitalSpot.h"

void CEdEntityEditor::UpdateAnimationTab()
{
	UpdateAnimTabParam< CAnimGlobalParam >( m_animTabGlobalCheck, m_animTabGlobalPanelProp );
	UpdateAnimTabParam< CLookAtStaticParam >( m_animTabLookAtCheck, m_animTabLookAtPanelProp );
	UpdateAnimTabParam< CAnimConstraintsParam >( m_animTabConstCheck, m_animTabConstPanelProp );
	UpdateAnimTabParam< CAnimMimicParam >( m_animTabMimicCheck, m_animTabMimicPanelProp );

	RefreshAnimPanelAddRemoveStyle( CAnimBehaviorsParam, "BehaviorList" );
	RefreshAnimPanelAddRemoveStyle( CAnimAnimsetsParam, "AnimsetList" );
}

void CEdEntityEditor::ValidateAnimTabs( TDynArray< String >& log )
{
	String strOut;

	if ( !EntityEditorHelper::IsTemplateParamWithNameUnique< CAnimBehaviorsParam >( m_template, &strOut ) )
	{
		log.PushBack( String::Printf( TXT("Animation parameter template '%s' has duplicate name '%s'"), CAnimBehaviorsParam::GetStaticClass()->GetName().AsString().AsChar(), strOut.AsChar() ) );
	}

	if ( !EntityEditorHelper::IsTemplateParamWithNameUnique< CAnimAnimsetsParam >( m_template, &strOut ) )
	{
		log.PushBack( String::Printf( TXT("Animation parameter template '%s' has duplicate name '%s'"), CAnimAnimsetsParam::GetStaticClass()->GetName().AsString().AsChar(), strOut.AsChar() ) );
	}

	{
		TDynArray< CAnimBehaviorsParam* > params;
		m_template->GetAllParameters( params );

		const Uint32 size = params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CAnimBehaviorsParam* paramA = params[ i ];

			for ( Uint32 j=0; j<size; ++j )
			{
				CAnimBehaviorsParam* paramB = params[ j ];

				if ( i != j )
				{
					const TDynArray< SBehaviorGraphInstanceSlot >& slotsA = paramA->GetSlots();
					const TDynArray< SBehaviorGraphInstanceSlot >& slotsB = paramB->GetSlots();

					for ( Uint32 sA = 0; sA < slotsA.Size(); ++sA )
					{
						const SBehaviorGraphInstanceSlot& slotA = slotsA[ sA ];

						for ( Uint32 sB = 0; sB < slotsB.Size(); ++sB )
						{
							const SBehaviorGraphInstanceSlot& slotB = slotsB[ sB ];

							if ( slotA.m_instanceName == slotB.m_instanceName )
							{
								log.PushBack( String::Printf( TXT("Animation template parameters ('%s') with names '%s' and '%s' have the same graph's instance name '%s'"), 
									CAnimBehaviorsParam::GetStaticClass()->GetName().AsString().AsChar(), 
									paramA->GetName().AsChar(),
									paramB->GetName().AsChar(),
									slotA.m_instanceName.AsString().AsChar() ) );
							}

							if ( slotA.m_graph.Get() == nullptr )
							{
								log.PushBack( String::Printf( TXT("Animation template parameters ('%s') with name '%s' - Instance '%s' has empty graph resource"), 
									CAnimBehaviorsParam::GetStaticClass()->GetName().AsString().AsChar(), 
									paramA->GetName().AsChar(), slotA.m_instanceName.AsString().AsChar() ) );
							}
							if ( slotB.m_graph.Get() == nullptr )
							{
								log.PushBack( String::Printf( TXT("Animation template parameters ('%s') with name '%s' - Instance '%s' has empty graph resource"), 
									CAnimBehaviorsParam::GetStaticClass()->GetName().AsString().AsChar(), 
									paramB->GetName().AsChar(), slotB.m_instanceName.AsString().AsChar() ) );
							}

							if ( slotA.m_graph.Get() && slotB.m_graph.Get() && slotA.m_graph.Get()->GetDepotPath() == slotB.m_graph.Get()->GetDepotPath() )
							{
								log.PushBack( String::Printf( TXT("Animation template parameters ('%s') with names '%s' (instance '%s') and '%s' (instance '%s') have the same graph resource '%s'"), 
									CAnimBehaviorsParam::GetStaticClass()->GetName().AsString().AsChar(), 
									paramA->GetName().AsChar(), slotA.m_instanceName.AsString().AsChar(),
									paramB->GetName().AsChar(), slotB.m_instanceName.AsString().AsChar(),
									slotA.m_graph.Get()->GetDepotPath().AsChar() ) );
							}
						}
					}
				}
			}
		}
	}

	{
		TDynArray< CAnimAnimsetsParam* > params;
		m_template->GetAllParameters( params );

		const Uint32 size = params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CAnimAnimsetsParam* paramA = params[ i ];

			for ( Uint32 j=0; j<size; ++j )
			{
				CAnimAnimsetsParam* paramB = params[ j ];

				if ( i != j )
				{
					const TDynArray< THandle< CSkeletalAnimationSet > >& setsA = paramA->GetAnimationSets();
					const TDynArray< THandle< CSkeletalAnimationSet > >& setsB = paramB->GetAnimationSets();

					for ( Uint32 sA = 0; sA < setsA.Size(); ++sA )
					{
						const CSkeletalAnimationSet* setA = setsA[ sA ].Get();

						for ( Uint32 sB = 0; sB < setsB.Size(); ++sB )
						{
							const CSkeletalAnimationSet* setB = setsB[ sB ].Get();

							if ( setA && setB && setA->GetDepotPath() == setB->GetDepotPath() )
							{
								log.PushBack( String::Printf( TXT("Animation template parameters ('%s') with names '%s' and '%s' have the same animset resource '%s'"), 
									CAnimAnimsetsParam::GetStaticClass()->GetName().AsString().AsChar(), 
									paramA->GetName().AsChar(),
									paramB->GetName().AsChar(),
									setA->GetDepotPath().AsChar() ) );
							}
						}
					}
				}
			}
		}
	}
}

void CEdEntityEditor::OnAnimTabAnimsetPropModified( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	String propName = eventData->m_propertyName.AsString();
	if ( propName == TXT("name") )
	{		
		RefreshAnimPanelAddRemoveStyle( CAnimAnimsetsParam, "AnimsetList" );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Style - Check box, one param per entity

void CEdEntityEditor::OnAnimTabGlobalCheck( wxCommandEvent& event )
{
	AnimTabCheck< CAnimGlobalParam >( m_animTabGlobalCheck, m_animTabGlobalPanelProp );
}

void CEdEntityEditor::OnAnimTabLookAtCheck( wxCommandEvent& event )
{
	AnimTabCheck< CLookAtStaticParam >( m_animTabLookAtCheck, m_animTabLookAtPanelProp );
}

void CEdEntityEditor::OnAnimTabConstCheck( wxCommandEvent& event )
{
	AnimTabCheck< CAnimConstraintsParam >( m_animTabConstCheck, m_animTabConstPanelProp );
}

void CEdEntityEditor::OnAnimTabMimicCheck( wxCommandEvent& event )
{
	AnimTabCheck< CAnimMimicParam >( m_animTabMimicCheck, m_animTabMimicPanelProp );
}

//////////////////////////////////////////////////////////////////////////

template< typename T >
RED_INLINE void CEdEntityEditor::OnAnimTabUpdateTabList( const AnsiChar* listCtrlXrcId )
{
	wxListBox* l = XRCCTRL( *this, listCtrlXrcId, wxListBox );
	EntityEditorHelper::UpdateAnimTabList< T >( m_template, l );
}

template< typename T >
RED_INLINE void CEdEntityEditor::OnAnimTabSetObject( T* object, const AnsiChar* propPanelXrcId, Bool setNoneIfNull )
{
	wxPanel* rp = XRCCTRL( *this, propPanelXrcId, wxPanel );
	RED_FATAL_ASSERT( rp, "No widget called %hs in %ls", propPanelXrcId, GetName().wc_str() );

	CEdPropertiesPage* page = static_cast< CEdPropertiesPage* >( rp->FindWindow( wxT( "CEdPropertiesPage" ) ) );
	RED_FATAL_ASSERT( page, "No widget called CEdPropertiesPage in parent %hs", propPanelXrcId );

	if ( object )
	{
		page->SetObject( object );
	}
	else if( !object && setNoneIfNull )
	{
		page->SetNoObject();
	}
}

template< typename T >
T* CEdEntityEditor::AnimTabGetParamFromEvent( wxCommandEvent& event )
{
	wxClientData* clientObject = event.GetClientObject();
	T* param = nullptr;

	if ( clientObject )
	{
		TClientDataWrapper< T* >* data = static_cast< TClientDataWrapper< T* >* >( clientObject );
		param = data->GetData();
	}

	return param;
}

template< typename T >
void CEdEntityEditor::AnimTabAddParam( const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId )
{
	T* param = EntityEditorHelper::AddParamWithNameToTemplate< T >( this, m_template );
	OnAnimTabUpdateTabList< T >( listCtrlXrcId );
	OnAnimTabSetObject< T >( param, propPanelXrcId, false );
}

template< typename T >
void CEdEntityEditor::AnimTabRemoveParam( const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId )
{
	wxListBox* l = XRCCTRL( *this, listCtrlXrcId, wxListBox );
	int selection = l->GetSelection();
	wxString paramName = l->GetString( selection );

	T* param = m_template->FindParameter< T >( false, [ &paramName ]( T* param ){ return ( param->GetName() == paramName.wx_str() )? true : false; } );

	if( param )
	{
		if( m_template->RemoveParameter( param ) )
		{
			OnAnimTabUpdateTabList< T >( listCtrlXrcId );
			OnAnimTabSetObject< T >( nullptr, propPanelXrcId, true );
		}
	}
}

template< typename T >
void CEdEntityEditor::AnimTabListParam( wxCommandEvent& event, const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId )
{
	T* param = AnimTabGetParamFromEvent< T >( event );
	OnAnimTabSetObject< T >( param, propPanelXrcId, true );
}

void CEdEntityEditor::OnAnimTabBehAdded( wxCommandEvent& )
{
	AnimTabAddParam< CAnimBehaviorsParam >( "panelBehProp", "BehaviorList" );
}
void CEdEntityEditor::OnAnimTabBehRemoved( wxCommandEvent& )
{
	AnimTabRemoveParam< CAnimBehaviorsParam >( "panelBehProp", "BehaviorList" );
}
void CEdEntityEditor::OnAnimTabBehListChanged( wxCommandEvent& event )
{
	AnimTabListParam< CAnimBehaviorsParam >( event, "panelBehProp", "BehaviorList" );
}
void CEdEntityEditor::OnAnimTabBehPropModified( wxCommandEvent& event )
{

}

void CEdEntityEditor::OnAnimTabAnimsetAdded( wxCommandEvent& )
{
	AnimTabAddParam< CAnimAnimsetsParam >( "panelAnimsetProp", "AnimsetList" );
}
void CEdEntityEditor::OnAnimTabAnimsetRemoved( wxCommandEvent& )
{
	AnimTabRemoveParam< CAnimAnimsetsParam >( "panelAnimsetProp", "AnimsetList" );
}
void CEdEntityEditor::OnAnimTabAnimsetListChanged( wxCommandEvent& event )
{
	AnimTabListParam< CAnimAnimsetsParam >( event, "panelAnimsetProp", "AnimsetList" );
}
void CEdEntityEditor::OnAnimTabAnimsetPropModifiedUnused( wxCommandEvent& )
{

}
