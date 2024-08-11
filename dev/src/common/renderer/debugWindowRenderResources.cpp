/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "renderDynamicResource.h"
#include "renderTexture.h"
#include "renderTextureArray.h"

#include "../engine/redGuiList.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiLabel.h"
#include "../engine/redGuiListItem.h"
#include "../engine/redGuiGridLayout.h"
#include "../engine/redGuiManager.h"
#include "debugWindowRenderResources.h"

namespace DebugWindows
{
	CDebugWindowdRenderResources::CDebugWindowdRenderResources() 
		: RedGui::CRedGuiWindow( 300, 100, 600, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowdRenderResources::NotifyOnTick );

		SetCaption( TXT("Render resources") );
		CreateControls();
	}

	CDebugWindowdRenderResources::~CDebugWindowdRenderResources()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowdRenderResources::NotifyOnTick );
	}

	void CDebugWindowdRenderResources::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == false )
		{
			return;
		}

		struct STextureCategory
		{
			CName	m_category;
			Uint32	m_count;
			Uint32	m_totalMemSize;

			STextureCategory( CName category )
				: m_category( category )
				, m_count( 0 )
				, m_totalMemSize( 0 )
			{ /* intentionally empty */ };
		};

		// Get all resources
		typedef TDynArray< IDynamicRenderResource* > ResPtrArray;
		ResPtrArray allResources;
		IDynamicRenderResource::GetAllResources( allResources );

		// Reset 
		for( TDynArray< SRenderTypeInfo >::iterator it=m_renderCategoryTypes.Begin(); it!=m_renderCategoryTypes.End(); ++it )
		{
			it->m_totalMemSize = 0;
			it->m_count = 0;
		}

		// process resources
		Uint32 totalMemorySize = 0;
		TDynArray< STextureCategory > textureCategories;
		TDynArray< STextureCategory > textureArrayCategories;
		for( ResPtrArray::iterator it=allResources.Begin(); it!=allResources.End(); ++it )
		{
			IDynamicRenderResource* res = *it;

			// Skip resources without category
			const CName& name = res->GetCategory();
			if ( name.GetIndex() != CNamesPool::INDEX_NONE )
			{
				// Get existing category info
				SRenderTypeInfo* info = nullptr;
				const Uint32 typeCount = m_renderCategoryTypes.Size();
				for ( Uint32 i=0; i<typeCount; ++i )
				{
					if( m_renderCategoryTypes[i].m_category == name )
					{
						info = &m_renderCategoryTypes[i];
						break;
					}
				}

				// Add new category info if not found
				if( info == nullptr )
				{
					info = new ( m_renderCategoryTypes ) SRenderTypeInfo( res->GetCategory() );
				}

				// Update
				info->m_count++;
				info->m_totalMemSize += res->GetUsedVideoMemory();

				// Update global count
				const Uint32 memorySize = res->GetUsedVideoMemory();
				totalMemorySize += memorySize;

				// Well it's a texture, create histogram of texture groups
				if( name == CNAME( RenderTexture ) )
				{
					// Get texture category name
					CRenderTexture* texture = static_cast< CRenderTexture* >( res );
					CName textureCategory = texture->GetTextureGroupName();

					// Find matching category
					STextureCategory* catInfo = nullptr;
					const Uint32 textureCategoryCount = textureCategories.Size();
					for( Uint32 i=0; i<textureCategoryCount; ++i )
					{
						if( textureCategories[i].m_category == textureCategory )
						{
							catInfo = &textureCategories[i];
							break;
						}
					}

					// Create new
					if ( catInfo == nullptr )
					{
						catInfo = new ( textureCategories ) STextureCategory( textureCategory );
					}

					// Update count
					catInfo->m_count += 1;
					catInfo->m_totalMemSize += memorySize;
				}
				if( name == CNAME( RenderTextureArray ) )
				{
					// Get texture category name
					CRenderTextureArray* texture = static_cast< CRenderTextureArray* >( res );
					CName textureCategory = texture->GetTextureGroupName();

					// Find matching category
					STextureCategory* catInfo = nullptr;
					const Uint32 textureArrayCategoryCount = textureArrayCategories.Size();
					for( Uint32 i=0; i<textureArrayCategoryCount; ++i )
					{
						if( textureArrayCategories[i].m_category == textureCategory )
						{
							catInfo = &textureArrayCategories[i];
							break;
						}
					}

					// Create new
					if ( catInfo  == nullptr)
					{
						catInfo = new ( textureArrayCategories ) STextureCategory( textureCategory );
					}

					// Update count
					catInfo->m_count += 1;
					catInfo->m_totalMemSize += memorySize;
				}
			}

			// Release resource
			res->Release();
		}

		// update lists
		{
			// general resources list update
			{
				const Uint32 categoryCount = m_renderCategoryTypes.Size();
				for( Uint32 i=0; i<categoryCount; ++i )
				{
					// Skip crap with less than MB of data and single instance
					Float memoryCount = m_renderCategoryTypes[i].m_totalMemSize / ( 1024.0f * 1024.0f );
					if ( memoryCount < 1.0f && m_renderCategoryTypes[i].m_count == 1 )
					{
						continue;
					}

					Bool found = false;
					const String name = m_renderCategoryTypes[i].m_category.AsString();

					const Uint32 categoryListCount = m_renderResourceList->GetItemCount();
					for( Uint32 j=0; j<categoryListCount; ++j )
					{
						if( m_renderResourceList->GetItem( j )->GetText( 0 ) == name )
						{
							m_renderResourceList->SetItemText( ToString( m_renderCategoryTypes[i].m_count ), j, 1 );
							m_renderResourceList->SetItemText( String::Printf( TXT("%1.2f"), m_renderCategoryTypes[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), j, 2 );
							m_renderResourceList->SetItemText( String::Printf( TXT("%d"), m_renderCategoryTypes[i].m_totalMemSize ), j, 3 );
							found = true;
							break;
						}
					}

					if( found == false )
					{
						Uint32 newItemIndex = m_renderResourceList->AddItem( name );
						m_renderResourceList->SetItemText( ToString( m_renderCategoryTypes[i].m_count ), newItemIndex, 1 );
						m_renderResourceList->SetItemText( String::Printf( TXT("%1.2f"), m_renderCategoryTypes[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), newItemIndex, 2 );
						m_renderResourceList->SetItemText( String::Printf( TXT("%d"), m_renderCategoryTypes[i].m_totalMemSize ), newItemIndex, 3 );
					}
				}
			}

			// texture groups list update
			{
				const Uint32 textureCategoryCount = textureCategories.Size();
				for( Uint32 i=0; i<textureCategoryCount; ++i )
				{
					Bool found = false;
					const String name = textureCategories[i].m_category.AsString();

					const Uint32 textureCategoryListCount = m_renderTexturesList->GetItemCount();
					for( Uint32 j=0; j<textureCategoryListCount; ++j )
					{
						if( m_renderTexturesList->GetItem( j )->GetText( 0 ) == name )
						{
							m_renderTexturesList->SetItemText( ToString( textureCategories[i].m_count ), j, 1 );
							m_renderTexturesList->SetItemText( String::Printf( TXT("%1.2f"), textureCategories[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), j, 2 );
							m_renderTexturesList->SetItemText( String::Printf( TXT("%d"), textureCategories[i].m_totalMemSize ), j, 3 );
							found = true;
							break;
						}
					}

					if( found == false )
					{
						Uint32 newItemIndex = m_renderTexturesList->AddItem( name );
						m_renderTexturesList->SetItemText( ToString( textureCategories[i].m_count ), newItemIndex, 1 );
						m_renderTexturesList->SetItemText( String::Printf( TXT("%1.2f"), textureCategories[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), newItemIndex, 2 );
						m_renderTexturesList->SetItemText( String::Printf( TXT("%d"), textureCategories[i].m_totalMemSize ), newItemIndex, 3 );
					}
				}
			}

			// texture groups list update
			{
				const Uint32 textureArrayCategoryCount = textureArrayCategories.Size();
				for( Uint32 i=0; i<textureArrayCategoryCount; ++i )
				{
					Bool found = false;
					const String name = textureArrayCategories[i].m_category.AsString();

					const Uint32 textureArrayCategoryListCount = m_renderTextureArraysList->GetItemCount();
					for( Uint32 j=0; j<textureArrayCategoryListCount; ++j )
					{
						if( m_renderTextureArraysList->GetItem( j )->GetText( 0 ) == name )
						{
							m_renderTextureArraysList->SetItemText( ToString( textureArrayCategories[i].m_count ), j, 1 );
							m_renderTextureArraysList->SetItemText( String::Printf( TXT("%1.2f"), textureArrayCategories[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), j, 2 );
							m_renderTextureArraysList->SetItemText( String::Printf( TXT("%d"), textureArrayCategories[i].m_totalMemSize ), j, 3 );
							found = true;
							break;
						}
					}

					if( found == false )
					{
						Uint32 newItemIndex = m_renderTextureArraysList->AddItem( name );
						m_renderTextureArraysList->SetItemText( ToString( textureArrayCategories[i].m_count ), newItemIndex, 1 );
						m_renderTextureArraysList->SetItemText( String::Printf( TXT("%1.2f"), textureArrayCategories[i].m_totalMemSize / ( 1024.0f * 1024.0f ) ), newItemIndex, 2 );
						m_renderTextureArraysList->SetItemText( String::Printf( TXT("%d"), textureArrayCategories[i].m_totalMemSize ), newItemIndex, 3 );
					}
				}
			}
		}
	}

	void CDebugWindowdRenderResources::CreateControls()
	{
		// create layout
		RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
		layout->SetDock( RedGui::DOCK_Fill );
		layout->SetDimensions( 1, 3 );
		AddChild( layout );

		// general resources
		{
			RedGui::CRedGuiPanel* effectsInfoPanel = new RedGui::CRedGuiPanel( 0,0, 250, 150 );
			effectsInfoPanel->SetMargin( Box2(5, 5, 5, 5) );
			effectsInfoPanel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
			effectsInfoPanel->SetDock( RedGui::DOCK_Top );
			layout->AddChild( effectsInfoPanel );

			RedGui::CRedGuiLabel* categoryLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			categoryLabel->SetMargin( Box2( 5, 5, 5, 7 ) );
			categoryLabel->SetDock( RedGui::DOCK_Top );
			categoryLabel->SetText( TXT("General resources") );
			effectsInfoPanel->AddChild( categoryLabel );

			m_renderResourceList = new RedGui::CRedGuiList( 0, 125, 400, 100 );
			m_renderResourceList->SetDock( RedGui::DOCK_Fill );
			m_renderResourceList->SetSelectionMode( RedGui::SM_None );
			m_renderResourceList->AppendColumn( TXT("Category"), 200 );
			m_renderResourceList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
			m_renderResourceList->AppendColumn( TXT("Size [MB]"), 100, RedGui::SA_Real );
			m_renderResourceList->AppendColumn( TXT("Size [B]"), 200, RedGui::SA_Integer );
			m_renderResourceList->SetSorting( true );
			effectsInfoPanel->AddChild( m_renderResourceList );
		}

		// texture resources
		{
			RedGui::CRedGuiPanel* effectsInfoPanel = new RedGui::CRedGuiPanel( 0,0, 250, 150 );
			effectsInfoPanel->SetMargin( Box2(5, 5, 5, 5) );
			effectsInfoPanel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
			effectsInfoPanel->SetDock( RedGui::DOCK_Top );
			layout->AddChild( effectsInfoPanel );

			RedGui::CRedGuiLabel* categoryLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			categoryLabel->SetMargin( Box2( 5, 5, 5, 7 ) );
			categoryLabel->SetDock( RedGui::DOCK_Top );
			categoryLabel->SetText( TXT("Texture resources") );
			effectsInfoPanel->AddChild( categoryLabel );

			m_renderTexturesList = new RedGui::CRedGuiList( 0, 125, 400, 350 );
			m_renderTexturesList->SetDock( RedGui::DOCK_Fill );
			m_renderTexturesList->SetSelectionMode( RedGui::SM_None );
			m_renderTexturesList->AppendColumn( TXT("Category"), 200 );
			m_renderTexturesList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
			m_renderTexturesList->AppendColumn( TXT("Size [MB]"), 100, RedGui::SA_Real );
			m_renderTexturesList->AppendColumn( TXT("Size [B]"), 200, RedGui::SA_Integer );
			m_renderTexturesList->SetSorting( true );
			effectsInfoPanel->AddChild( m_renderTexturesList );
		}

		// texture arrays resources
		{
			RedGui::CRedGuiPanel* effectsInfoPanel = new RedGui::CRedGuiPanel( 0,0, 250, 150 );
			effectsInfoPanel->SetMargin( Box2(5, 5, 5, 5) );
			effectsInfoPanel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
			effectsInfoPanel->SetDock( RedGui::DOCK_Top );
			layout->AddChild( effectsInfoPanel );

			RedGui::CRedGuiLabel* categoryLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			categoryLabel->SetMargin( Box2( 5, 5, 5, 7 ) );
			categoryLabel->SetDock( RedGui::DOCK_Top );
			categoryLabel->SetText( TXT("Texture array resources") );
			effectsInfoPanel->AddChild( categoryLabel );

			m_renderTextureArraysList = new RedGui::CRedGuiList( 0, 125, 400, 350 );
			m_renderTextureArraysList->SetDock( RedGui::DOCK_Fill );
			m_renderTextureArraysList->SetSelectionMode( RedGui::SM_None );
			m_renderTextureArraysList->AppendColumn( TXT("Category"), 200 );
			m_renderTextureArraysList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
			m_renderTextureArraysList->AppendColumn( TXT("Size [MB]"), 100, RedGui::SA_Real );
			m_renderTextureArraysList->AppendColumn( TXT("Size [B]"), 200, RedGui::SA_Integer );
			m_renderTextureArraysList->SetSorting( true );
			effectsInfoPanel->AddChild( m_renderTextureArraysList );
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
