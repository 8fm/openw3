/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../redIO/redIO.h"

#include "redGuiScrollPanel.h"
#include "redGuiPanel.h"
#include "redGuiTab.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiButton.h"
#include "redGuiTab.h"
#include "redGuiLabel.h"
#include "redGuiGridLayout.h"
#include "redGuiSaveFileDialog.h"
#include "redGuiManager.h"
#include "debugWindowLoadedResources.h"
#include "mesh.h"
#include "meshEnum.h"
#include "fxTrackItem.h"

#include "../core/depot.h"
#include "game.h"
#include "fxState.h"
#include "../core/events.h"
#include "../core/objectIterator.h"
#include "tickManager.h"
#include "world.h"
#include "bitmapTexture.h"
#include "fxDefinition.h"
#include "entity.h"
#include "renderer.h"

namespace DebugWindows
{
	CDebugWindowLoadedResources::CDebugWindowLoadedResources() 
		: RedGui::CRedGuiWindow(300,100,1200,600)
		, m_characterMeshesMB(0)
		, m_environmentMeshesMB(0)
		, m_itemMeshesMB(0)
		, m_otherMeshesMB(0)
		, m_characterTexturesMB(0)
		, m_environmentTexturesMB(0)
		, m_otherTexturesMB(0)
		, m_selectedEffect( nullptr )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowLoadedResources::NotifyOnTick );

		SetCaption( TXT("Loaded resources") );

		m_tabs = new RedGui::CRedGuiTab( 0, 0, GetWidth(), GetHeight() );
		m_tabs->SetMargin( Box2(5, 5, 5, 5) );
		AddChild( m_tabs );
		m_tabs->SetDock( RedGui::DOCK_Fill );

		// create tabs
		for( Uint32 i=0; i<LRT_Count; ++i )
		{
			switch( i )
			{
			case LRT_Texture:
				CreateTexturesTab();
				break;
			case LRT_Mesh:
				CreateMeshesTab();
				break;
			case LRT_Effect:
				CreateEffectsTab();
				break;
			}
		}

		// save file dialog
		m_saveFileDialog = new RedGui::CRedGuiSaveFileDialog();
		m_saveFileDialog->AddFilter( TXT("Text file"), TXT("txt") );
		m_saveFileDialog->EventFileOK.Bind( this, &CDebugWindowLoadedResources::NotifyEventFileOK );
	}

	CDebugWindowLoadedResources::~CDebugWindowLoadedResources()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowLoadedResources::NotifyOnTick );
	}

	void CDebugWindowLoadedResources::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_saveFileDialog->SetVisible(true);
	}

	void CDebugWindowLoadedResources::DumpLoadedTextures( const String& path )
	{
		Uint32 helperSizer = 0;
		Red::IO::CNativeFileHandle dumpFile;
		if( dumpFile.Open( path.AsChar(), Red::IO::eOpenFlag_WriteNew ) == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot open file: %s"), path.AsChar() );
			return;
		}

		Uint32 hasRenderResourceCount = 0;
		Float hasRenderResourceSize = 0;
		Float highestMipsSize = 0;

		String bigAndNastyStringCSV = String::EMPTY;
		bigAndNastyStringCSV = TXT("Path;");
		bigAndNastyStringCSV += TXT("Texture group;");
		bigAndNastyStringCSV += TXT("Mip;");
		bigAndNastyStringCSV += TXT("Mip Count;");
		bigAndNastyStringCSV += TXT("Width;");
		bigAndNastyStringCSV += TXT("Height;");
		bigAndNastyStringCSV += TXT("Mip Memory (kB) Cost;");
		bigAndNastyStringCSV += TXT("Memory (kB) Cost\n");
		dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );

		for (ObjectIterator<CBitmapTexture> it; it; ++it)
		{
			CBitmapTexture* tex = (*it);
			Uint32 size = 0, mipSize = 0;
			Int8 mip = tex->GetRenderResource() == nullptr ? -1 : GRender->GetTextureStreamedMipIndex( tex->GetRenderResource() );

			if ( tex->IsCooked() )
			{
				size = tex->GetCookedData().GetSize();
				Uint32 firstMipSize = 0;
				Float factor = 0;
				for ( Uint32 mipInd = 0; mipInd < tex->GetMipCount(); ++mipInd )
				{
					factor += 1.0f / (Float)( 1 << (mipInd*2) );
				}
				mipSize = (Uint32)(size / factor) >> (mip*2);
			}
#ifndef NO_EDITOR
			else
			{
				size = tex->CalcTextureDataSize();
				const CBitmapTexture::MipArray& mips = tex->GetMips();
				if ( mip >= 0 && (Uint8)mip < mips.Size() )
				{
					mipSize = mips[ mip ].m_data.GetSize();	
				}	
			}
#endif

			if (!tex->GetResourceLoadError() && tex->GetRenderResource() != nullptr)
			{
				String name = tex->GetFile() == nullptr ? tex->GetFriendlyName() : tex->GetFile()->GetDepotPath();
				Float sizeInkB = size / 1024.f;
				Float mipSizeInkB = mipSize / 1024.f;
				if ( name.ContainsSubstring(TXT("Unnamed")) && tex->GetParent() != nullptr )
				{
					name = String::Printf(TXT("Child of %s"), tex->GetParent()->GetFriendlyName().AsChar());
				}

				bigAndNastyStringCSV = String::Printf( TXT("%s;%s;%d;%d;%u;%u;%.4f;%.4f\n"), name.AsChar(), tex->GetTextureGroupName().AsString().AsChar(),
					mip, tex->GetMipCount(), tex->GetWidth(), tex->GetHeight(), mipSizeInkB, sizeInkB );

				dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );
				
				hasRenderResourceCount++;
				hasRenderResourceSize += sizeInkB;
				highestMipsSize += mipSizeInkB;
			}
		}

		bigAndNastyStringCSV = String::Printf( TXT("%u texture(s) have render resources with size: %.4f, highest used mips size: %.4f\n\n"), 
			hasRenderResourceCount, hasRenderResourceSize / 1024.0f, highestMipsSize / 1024.0f );
		dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );

		if( dumpFile.Flush() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot flush file: %s"), path.AsChar() );
		}

		if( dumpFile.Close() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot close file: %s"), path.AsChar() );
		}
	}

	void CDebugWindowLoadedResources::DumpLoadedMeshes( const String& path )
	{
		Uint32 helperSizer = 0;
		Red::IO::CNativeFileHandle dumpFile;
		if( dumpFile.Open( path.AsChar(), Red::IO::eOpenFlag_WriteNew ) == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot open file: %s"), path.AsChar() );
			return;
		}

		String helperText = String::EMPTY;
		Uint32 hasRenderResourceCount = 0;
		Uint32 hasRenderResourceSize = 0;

		String bigAndNastyStringCSV = String::EMPTY;
		bigAndNastyStringCSV = TXT("Path;");
		bigAndNastyStringCSV += TXT("Memory (MB) Cost;");
		bigAndNastyStringCSV += TXT("Author\n");
		dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );

		for (ObjectIterator< CMesh > it; it; ++it)
		{
			CMesh* mesh = *it;
			Uint32 size = 0;
			if ( mesh->IsCooked() )
			{
				size = mesh->GetCookedDataSize();
			}
#ifndef NO_EDITOR
			else
			{
				Uint32 lodLevels = mesh->GetNumLODLevels();
				for ( Uint32 i = 0; i< lodLevels; ++i )
				{
					size += mesh->EstimateMemoryUsageGPU(i);
				}
			}
#endif

			if (!mesh->GetResourceLoadError() && mesh->GetRenderResource() != nullptr)
			{
				bigAndNastyStringCSV = String::Printf( TXT("%s"), mesh->GetFriendlyName().AsChar() );	
				bigAndNastyStringCSV += TXT(";");
				bigAndNastyStringCSV += String::Printf( TXT("%.2f Mb"), size/(1024.0f * 1024.0f) );	
				bigAndNastyStringCSV += TXT(";");					

#ifndef NO_RESOURCE_IMPORT
				bigAndNastyStringCSV += String::Printf( TXT("%s\n"), mesh->GetAuthorName().AsChar() );
#endif

				dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );

				hasRenderResourceCount++;
				hasRenderResourceSize += static_cast< Uint32 >( size/(1024.0f * 1024.0f) );
			}
		}

		bigAndNastyStringCSV = String::Printf( TXT("%u mesh(es) have render resources with size: %u\n\n"), hasRenderResourceCount, hasRenderResourceSize );
		dumpFile.Write( bigAndNastyStringCSV.AsChar(), bigAndNastyStringCSV.GetLength() * sizeof( Char ), helperSizer );

		if( dumpFile.Flush() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot flush file: %s"), path.AsChar() );
		}

		if( dumpFile.Close() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot close file: %s"), path.AsChar() );
		}
	}

	void CDebugWindowLoadedResources::RefreshTexturesTab()
	{
		CollectAllTextures();

		UpdateListContents( m_notLoadedTexturesList, m_corruptedTextures );
		UpdateListContents( m_texturesList, m_textures, true );

		String stats = String::Printf(TXT("Character textures: %1.2f MB"), m_characterTexturesMB);
		m_characterTextures->SetText(stats);

		stats = String::Printf(TXT("Environment textures: %1.2f MB"), m_environmentTexturesMB);
		m_environmentTextures->SetText(stats);

		stats = String::Printf(TXT("Other textures: %1.2f MB"), m_otherTexturesMB);
		m_otherTextures->SetText(stats);

		stats = String::Printf(TXT("All textures: %1.2f MB"), m_otherTexturesMB + m_characterTexturesMB + m_environmentTexturesMB );
		m_allTextures->SetText(stats);
	}

	void CDebugWindowLoadedResources::RefreshMeshesTab()
	{
		CollectAllMeshes();

		UpdateListContents( m_notLoadedMeshesList, m_corruptedMeshes );
		UpdateListContents( m_meshesList, m_meshes );

		// set info labels
		String stats = String::Printf(TXT("Character meshes: %1.2f MB"), m_characterMeshesMB);
		m_characterMeshes->SetText(stats);

		stats = String::Printf(TXT("Environment meshes: %1.2f MB"), m_environmentMeshesMB);
		m_environmentMeshes->SetText(stats);

		stats = String::Printf(TXT("Item meshes: %1.2f MB"), m_itemMeshesMB);
		m_itemMeshes->SetText(stats);

		stats = String::Printf(TXT("Other meshes: %1.2f MB"), m_otherMeshesMB);
		m_otherMeshes->SetText(stats);
	}

	void CDebugWindowLoadedResources::UpdateListContents( RedGui::CRedGuiList* list, THashMap< String, ResourceInfo >& content, Bool includeAdditionalInfo /* = false */ )
	{
		Uint32 itemsCount = list->GetItemCount();
		for ( Uint32 i = 0; i < itemsCount; )
		{
			const String& path = list->GetItem( i )->GetText(0);
			if ( ResourceInfo* ri = content.FindPtr( path ) )
			{
				list->GetItem( i )->SetText( ri->m_info, 0 );

				Uint32 colShift = 0;
				if ( includeAdditionalInfo )
				{
					for ( ; colShift < ri->m_additionalParams.Size(); ++colShift )
					{
						list->GetItem( i )->SetText( ri->m_additionalParams[ colShift ], colShift + 1 );
					}
					list->GetItem( i )->SetText( String::Printf( TXT("%.4f"), ri->m_partSize ), colShift + 1 );
					++colShift;
				}

				list->GetItem( i )->SetText( String::Printf( TXT("%.4f"), ri->m_size ), colShift + 1 );

				ri->OnUsed();
				++i;
			}
			else
			{
				list->RemoveItem( i );
				--itemsCount;
			}
		}

		for ( THashMap< String, ResourceInfo >::iterator it = content.Begin(); it != content.End(); ++it )
		{
			if ( it->m_second.m_new )
			{
				RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( it->m_second.m_info );

				Uint32 colShift = 0;
				if ( includeAdditionalInfo )
				{
					for ( ; colShift < it->m_second.m_additionalParams.Size(); ++colShift )
					{
						newItem->SetText( it->m_second.m_additionalParams[ colShift ], colShift + 1 );
					}
					newItem->SetText( String::Printf( TXT("%.4f"), it->m_second.m_partSize ), colShift + 1 );
					++colShift;
				}

				newItem->SetText( String::Printf( TXT("%.4f"), it->m_second.m_size ), colShift + 1 );
				list->AddItem( newItem );
				it->m_second.OnUsed();
			}
		}
	}

	void CDebugWindowLoadedResources::EreaseNonActive( THashMap< String, ResourceInfo >& resources )
	{
		TDynArray< String > toErease;
		for ( THashMap< String, ResourceInfo >::iterator it = resources.Begin(); it != resources.End(); ++it )
		{
			if ( !it->m_second.m_active )
			{
				toErease.PushBack( it->m_first );
			}
		}

		for ( auto it = toErease.Begin(); it != toErease.End(); ++it )
		{
			resources.Erase( *it );
		}
	}

	void CDebugWindowLoadedResources::CollectAllTextures()
	{
		m_characterTexturesMB = 0;
		m_environmentTexturesMB = 0;
		m_otherTexturesMB = 0;

		for ( ObjectIterator< CBitmapTexture > it; it; ++it )
		{
			CBitmapTexture* tex = *it;
			if ( tex->GetDepotPath().Empty() )
			{
				continue;
			}

			String texGroup = tex->GetTextureGroupName().AsString();
			IRenderResource* renderRes = tex->GetRenderResource();
			Int8 mip = renderRes == nullptr ? -1 : GRender->GetTextureStreamedMipIndex( renderRes );
			Uint32 mipCount = tex->GetMipCount();

			Uint32 mipSize = 0, size = 0;
			if ( tex->IsCooked() )
			{
				size = tex->GetCookedData().GetSize();
				
				Uint32 firstMipSize = 0;
				Float factor = 0;
				for ( Uint32 mipInd = 0; mipInd < tex->GetMipCount(); ++mipInd )
				{
					factor += 1.0f / (Float)( 1 << (mipInd*2) );
				}
				mipSize = (Uint32)(size / factor) >> (mip*2);
			}
#ifndef NO_EDITOR
			else
			{
				const CBitmapTexture::MipArray& mips = tex->GetMips();
				if ( mip >= 0 && (Uint8)mip < mips.Size() )
				{
					mipSize = mips[ mip ].m_data.GetSize();	
				}
				size = tex->CalcTextureDataSize();
			}
#endif

			Float sizeInkB = size / 1024.f;
			Float mipSizeInkB = mipSize/ 1024.f;

			if ( ResourceInfo* ri = m_textures.FindPtr( tex->GetDepotPath() ) )
			{
				ri->m_active = true;
				ri->m_additionalParams.ClearFast();
				ri->m_additionalParams.PushBack( texGroup );
				ri->m_additionalParams.PushBack( String::Printf( TXT("%d"), mip ) );
				ri->m_additionalParams.PushBack( String::Printf( TXT("%d"), mipCount ) );
				ri->m_additionalParams.PushBack( String::Printf( TXT("%u x %u"), tex->GetWidth(), tex->GetHeight() ) );
				ri->m_size = sizeInkB;
				ri->m_partSize = mipSizeInkB;
				UpdateTexturesSizeCounters( tex->GetTextureGroupName().AsString(), ri->m_partSize );
			}
			else
			{
				String info = String::Printf( TXT( "%s" ), tex->GetFile() == nullptr ? tex->GetFriendlyName().AsChar() : tex->GetFile()->GetDepotPath().AsChar() );

				if ( tex->m_resourceLoadError == true )
				{
					// Error happened when loading resource
					if ( ResourceInfo* ri = m_corruptedTextures.FindPtr( tex->GetDepotPath() ) )
					{
						ri->m_active = true;
						ri->m_info = info;
						ri->m_size = sizeInkB;
					}
					else
					{
						m_corruptedTextures.Insert( tex->GetDepotPath(), ResourceInfo( tex, info, sizeInkB, mipSizeInkB ) );
					}
				}
				else
				{
					// Texture is loaded correctly 
					ResourceInfo newRI( tex, info, sizeInkB, mipSizeInkB );
					newRI.m_additionalParams.PushBack( texGroup );
					newRI.m_additionalParams.PushBack( String::Printf( TXT("%d"), mip ) );
					newRI.m_additionalParams.PushBack( String::Printf( TXT("%d"), mipCount ) );
					newRI.m_additionalParams.PushBack( String::Printf( TXT("%u x %u"), tex->GetWidth(), tex->GetHeight() ) );
					m_textures.Insert( tex->GetDepotPath(), newRI );
					UpdateTexturesSizeCounters( tex->GetTextureGroupName().AsString(), mipSizeInkB );
				}

			}

		}
		EreaseNonActive( m_textures );
	}


	void CDebugWindowLoadedResources::CollectAllMeshes()
	{
		m_characterMeshesMB = 0;
		m_environmentMeshesMB = 0;
		m_itemMeshesMB = 0;
		m_otherMeshesMB = 0;

		for ( ObjectIterator< CMesh > it; it; ++it )
		{
			CMesh* mesh = *it;
			if ( mesh->GetDepotPath().Empty() )
			{
				continue;
			}

			if( ResourceInfo* ri = m_meshes.FindPtr( mesh->GetDepotPath() ) )
			{
				ri->m_active = true;
				UpdateMeshesSizeCounters( mesh->GetFriendlyName(), ri->m_size );
			}
			else
			{
				Uint32 size = 0;
				if ( mesh->IsCooked() )
				{
					size = mesh->GetCookedDataSize();
				}
#ifndef NO_EDITOR
				else
				{
					Uint32 lodLevels = mesh->GetNumLODLevels();
					for ( Uint32 i = 0; i< lodLevels; ++i )
					{
						size += mesh->EstimateMemoryUsageGPU(i);
					}
				}
#endif

				Float sizeInMB = size / (1024.f * 1024.f);

				if ( mesh->ErrorOccuredDuringLoading() == true )
				{
					// Error happened when loading resource
					if ( ResourceInfo* ri = m_corruptedMeshes.FindPtr( mesh->GetDepotPath() ) )
					{
						ri->m_active = true;
						ri->m_info = mesh->GetFriendlyName();
						ri->m_size = sizeInMB;
					}
					else
					{
						m_corruptedTextures.Insert( mesh->GetDepotPath(), 
							ResourceInfo( mesh, mesh->GetFile() == nullptr ? mesh->GetFriendlyName() : mesh->GetFile()->GetDepotPath(), sizeInMB ) );
					}
				}
				else
				{
					// Mesh is loaded correctly 
					m_meshes.Insert( mesh->GetDepotPath(), 
						ResourceInfo( mesh, mesh->GetFile() == nullptr ? mesh->GetFriendlyName() : mesh->GetFile()->GetDepotPath(), sizeInMB ) );
					UpdateMeshesSizeCounters( mesh->GetFriendlyName(), sizeInMB );
				}
			}
		}

		EreaseNonActive( m_meshes );
	}

	void CDebugWindowLoadedResources::UpdateMeshesSizeCounters( const String& firendlyName, const Float sizeInMB )
	{
		if ( firendlyName.ContainsSubstring( TXT("characters") ) == true )
		{
			m_characterMeshesMB += sizeInMB;
		}
		else if( firendlyName.ContainsSubstring( TXT("environment") ) == true )
		{
			m_environmentMeshesMB += sizeInMB;
		}
		else if( firendlyName.ContainsSubstring( TXT("items") ) == true )
		{
			m_itemMeshesMB += sizeInMB;
		}
		else
		{
			m_otherMeshesMB += sizeInMB;
		}
	}

	void CDebugWindowLoadedResources::UpdateTexturesSizeCounters( const String& textureGroup, const Float sizeInkB )
	{
		if( textureGroup.ContainsSubstring( TXT("Character") ) || textureGroup.ContainsSubstring( TXT("Head") ) )
		{
			m_characterTexturesMB += sizeInkB / 1024.0f;
		}
		else if( textureGroup.ContainsSubstring( TXT("Default") ) || textureGroup.ContainsSubstring( TXT("Font") ) || textureGroup.ContainsSubstring( TXT("GUI") ) 
			|| textureGroup.ContainsSubstring( TXT("MimicDecal") ) || textureGroup.ContainsSubstring( TXT("SpecialQuest") ) )
		{
			m_otherTexturesMB += sizeInkB / 1024.0f;
		}
		else
		{
			m_environmentTexturesMB += sizeInkB / 1024.0f;
		}
	}

	void CDebugWindowLoadedResources::CreateTexturesTab()
	{
		m_tabs->AddTab( TXT("Textures") );

		// create textures tab
		RedGui::CRedGuiScrollPanel* textureTab = m_tabs->GetTabAt( LRT_Texture );
		if(textureTab != nullptr)
		{
			// create info panel about textures
			RedGui::CRedGuiPanel* textureInfoPanel = new RedGui::CRedGuiPanel(0,0, 250, 80);
			textureInfoPanel->SetMargin(Box2(5, 5, 5, 5));
			textureInfoPanel->SetPadding(Box2(5, 5, 5, 5));
			textureInfoPanel->SetBackgroundColor(Color(20, 20, 20, 255));
			textureInfoPanel->SetDock(RedGui::DOCK_Top);
			textureTab->AddChild(textureInfoPanel);

			// add info labels
			m_characterTextures = new RedGui::CRedGuiLabel(10,10,0,0);
			m_characterTextures->SetMargin(Box2(5, 5, 0, 0));
			textureInfoPanel->AddChild(m_characterTextures);
			m_characterTextures->SetDock(RedGui::DOCK_Top);

			m_environmentTextures = new RedGui::CRedGuiLabel(10,25,0,0);
			m_environmentTextures->SetMargin(Box2(5, 5, 0, 0));
			textureInfoPanel->AddChild(m_environmentTextures);
			m_environmentTextures->SetDock(RedGui::DOCK_Top);

			m_otherTextures = new RedGui::CRedGuiLabel(10,40,0,0);
			m_otherTextures->SetMargin(Box2(5, 5, 0, 0));
			textureInfoPanel->AddChild(m_otherTextures);
			m_otherTextures->SetDock(RedGui::DOCK_Top);

			m_allTextures = new RedGui::CRedGuiLabel(10,55,0,0);
			m_allTextures->SetMargin(Box2(5, 5, 0, 0));
			textureInfoPanel->AddChild(m_allTextures);
			m_allTextures->SetDock(RedGui::DOCK_Top);

			// create button for make dump file
			m_makeTexturesDumpFile = new RedGui::CRedGuiButton( 325, 50, 300, 25);
			m_makeTexturesDumpFile->SetMargin(Box2(5, 5, 5, 5));
			m_makeTexturesDumpFile->SetText(TXT("Dump loaded textures into file"));
			m_makeTexturesDumpFile->EventButtonClicked.Bind(this, &CDebugWindowLoadedResources::NotifyButtonClicked);
			m_makeTexturesDumpFile->SetDock(RedGui::DOCK_Bottom);
			textureTab->AddChild(m_makeTexturesDumpFile);

			// add lists with textures
			m_texturesList = new RedGui::CRedGuiList(0, 125, textureTab->GetWidth(), 250);
			m_texturesList->SetDock(RedGui::DOCK_Top);
			m_texturesList->SetMargin(Box2(5, 5, 5, 5));
			m_texturesList->SetSelectionMode( RedGui::SM_None );
			m_texturesList->AppendColumn( TXT("Loaded textures"), textureTab->GetWidth() - 520 );
			m_texturesList->AppendColumn( TXT("Texture group"), 120 );
			m_texturesList->AppendColumn( TXT("Mip"), 50, RedGui::SA_Integer );
			m_texturesList->AppendColumn( TXT("Mip count"), 60, RedGui::SA_Integer );
			m_texturesList->AppendColumn( TXT("Dims"), 80 );
			m_texturesList->AppendColumn( TXT("Mip size"), 80, RedGui::SA_Real );
			m_texturesList->AppendColumn( TXT("Size"), 100, RedGui::SA_Real );
			m_texturesList->EventDoubleClickItem.Bind( this, &CDebugWindowLoadedResources::NotifyEventDoubleClickItem );
			m_texturesList->SetSorting( true );
			textureTab->AddChild(m_texturesList);

			m_notLoadedTexturesList = new RedGui::CRedGuiList(0, 400, textureTab->GetWidth(), 100);
			m_notLoadedTexturesList->SetDock(RedGui::DOCK_Top);
			m_notLoadedTexturesList->SetMargin(Box2(5, 5, 5, 5));
			m_notLoadedTexturesList->SetSelectionMode( RedGui::SM_None );
			m_notLoadedTexturesList->AppendColumn( TXT("Textures with error during loading"), textureTab->GetWidth() - 130 );
			m_notLoadedTexturesList->AppendColumn( TXT("Size"), 100, RedGui::SA_Real );
			m_notLoadedTexturesList->EventDoubleClickItem.Bind( this, &CDebugWindowLoadedResources::NotifyEventDoubleClickItem );
			m_notLoadedTexturesList->SetSorting( true );
			textureTab->AddChild(m_notLoadedTexturesList);
		}
	}

	void CDebugWindowLoadedResources::CreateMeshesTab()
	{
		m_tabs->AddTab( TXT("Meshes") );

		// create meshes tab
		RedGui::CRedGuiScrollPanel* meshTab = m_tabs->GetTabAt( LRT_Mesh );
		if(meshTab != nullptr)
		{
			// create info panel about textures
			RedGui::CRedGuiPanel* meshInfoPanel = new RedGui::CRedGuiPanel(0,0, 250, 80);
			meshInfoPanel->SetMargin(Box2(5, 5, 5, 5));
			meshInfoPanel->SetPadding(Box2(5, 5, 5, 5));
			meshInfoPanel->SetBackgroundColor(Color(20, 20, 20, 255));
			meshInfoPanel->SetDock(RedGui::DOCK_Top);
			meshTab->AddChild(meshInfoPanel);

			// add info labels
			m_characterMeshes = new RedGui::CRedGuiLabel(10,10,0,0);
			m_characterMeshes->SetMargin(Box2(5, 5, 0, 0));
			meshInfoPanel->AddChild(m_characterMeshes);
			m_characterMeshes->SetDock(RedGui::DOCK_Top);

			m_environmentMeshes = new RedGui::CRedGuiLabel(10,25,0,0);
			m_environmentMeshes->SetMargin(Box2(5, 5, 0, 0));
			meshInfoPanel->AddChild(m_environmentMeshes);
			m_environmentMeshes->SetDock(RedGui::DOCK_Top);

			m_itemMeshes = new RedGui::CRedGuiLabel(10,40,0,0);
			m_itemMeshes->SetMargin(Box2(5, 5, 0, 0));
			meshInfoPanel->AddChild(m_itemMeshes);
			m_itemMeshes->SetDock(RedGui::DOCK_Top);

			m_otherMeshes = new RedGui::CRedGuiLabel(10,55,0,0);
			m_otherMeshes->SetMargin(Box2(5, 5, 0, 0));
			meshInfoPanel->AddChild(m_otherMeshes);
			m_otherMeshes->SetDock(RedGui::DOCK_Top);

			// create button for make dump file
			m_makeMeshesDumpFile = new RedGui::CRedGuiButton( 325, 50, 300, 25);
			m_makeMeshesDumpFile->SetMargin(Box2(5, 5, 5, 5));
			m_makeMeshesDumpFile->SetText(TXT("Dump loaded meshes into file"));
			m_makeMeshesDumpFile->EventButtonClicked.Bind(this, &CDebugWindowLoadedResources::NotifyButtonClicked);
			m_makeMeshesDumpFile->SetDock(RedGui::DOCK_Bottom);
			meshTab->AddChild(m_makeMeshesDumpFile);

			// add lists with textures
			m_meshesList = new RedGui::CRedGuiList( 0, 125, meshTab->GetWidth(), 250 );
			m_meshesList->SetDock( RedGui::DOCK_Top );
			m_meshesList->SetMargin( Box2(5, 5, 5, 5) );
			m_meshesList->SetSelectionMode( RedGui::SM_None );
			m_meshesList->AppendColumn( TXT("Loaded meshes"), meshTab->GetWidth() - 130 );
			m_meshesList->AppendColumn( TXT("Size"), 100, RedGui::SA_Real );
			m_meshesList->SetSorting( true );
			meshTab->AddChild(m_meshesList);

			m_notLoadedMeshesList = new RedGui::CRedGuiList(0, 200, meshTab->GetWidth(), 100);
			m_notLoadedMeshesList->SetDock(RedGui::DOCK_Top);
			m_notLoadedMeshesList->SetMargin(Box2(5, 5, 5, 5));
			m_notLoadedMeshesList->SetSelectionMode( RedGui::SM_None );
			m_notLoadedMeshesList->AppendColumn( TXT("Meshes with error during loading"), meshTab->GetWidth() - 130 );
			m_notLoadedMeshesList->AppendColumn( TXT("Size"), 100, RedGui::SA_Real );
			m_notLoadedMeshesList->SetSorting( true );
			meshTab->AddChild(m_notLoadedMeshesList);
		}
	}

	void CDebugWindowLoadedResources::NotifyEventDoubleClickItem( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		String assetPath = String::EMPTY;
		if ( RedGui::CRedGuiList* list = ( RedGui::CRedGuiList* )sender )
		{
			assetPath = list->GetItem( selectedIndex )->GetText(0);
		}

#ifndef NO_EDITOR_EVENT_SYSTEM
		SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( assetPath ) );
#endif	// NO_EDITOR_EVENT_SYSTEM
	}	

	void CDebugWindowLoadedResources::OnWindowOpened( RedGui::CRedGuiControl* control )
	{
		RefreshMeshesTab();
		m_tabs->SetActiveTab( LRT_Mesh );
	}

	void CDebugWindowLoadedResources::NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		String depotPath = String::EMPTY;
		GDepot->GetAbsolutePath(depotPath);
		depotPath += m_saveFileDialog->GetFileName();

		if( m_tabs->GetActiveTabIndex() == LRT_Texture )
		{
			DumpLoadedTextures( depotPath );
		}
		else if( m_tabs->GetActiveTabIndex() == LRT_Mesh )
		{
			DumpLoadedMeshes( depotPath );
		}
	}

	void CDebugWindowLoadedResources::CreateEffectsTab()
	{
		m_tabs->AddTab( TXT("Effects") );

		// create effects tab
		RedGui::CRedGuiScrollPanel* effectsTab = m_tabs->GetTabAt( LRT_Effect );
		if( effectsTab != nullptr )
		{
			// add info labels
			m_allActiveEffects = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_allActiveEffects->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_allActiveEffects->SetDock( RedGui::DOCK_Top );
			effectsTab->AddChild( m_allActiveEffects );

			// create layout
			RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			layout->SetDock( RedGui::DOCK_Fill );
			layout->SetDimensions( 2, 1 );
			effectsTab->AddChild( layout );

			// add list with textures
			m_effectsList = new RedGui::CRedGuiList( 0, 125, 400, 350 );
			m_effectsList->SetDock( RedGui::DOCK_Fill );
			m_effectsList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_effectsList->SetSelectionMode( RedGui::SM_Single );
			m_effectsList->AppendColumn( TXT("Effect name"), 300 );
			m_effectsList->EventSelectedItem.Bind( this, &CDebugWindowLoadedResources::NotifyOnClickedOnEffect );
			layout->AddChild( m_effectsList );

			// create info panel about effects
			RedGui::CRedGuiPanel* effectsInfoPanel = new RedGui::CRedGuiPanel( 0,0, 250, 150 );
			effectsInfoPanel->SetMargin( Box2(5, 5, 5, 5) );
			effectsInfoPanel->SetPadding( Box2(5, 5, 5, 5) );
			effectsInfoPanel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
			effectsInfoPanel->SetDock( RedGui::DOCK_Top );
			layout->AddChild( effectsInfoPanel );

			m_pathToEffect = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_pathToEffect->SetMargin( Box2( 5, 5, 0, 0 ) );
			m_pathToEffect->SetDock( RedGui::DOCK_Top );
			effectsInfoPanel->AddChild( m_pathToEffect );

			m_entityTemplateName = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_entityTemplateName->SetMargin( Box2( 5, 5, 0, 0 ) );
			m_entityTemplateName->SetDock( RedGui::DOCK_Top );
			effectsInfoPanel->AddChild( m_entityTemplateName );

			m_entityWorldPosition = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_entityWorldPosition->SetMargin( Box2( 5, 5, 0, 0 ) );
			m_entityWorldPosition->SetDock( RedGui::DOCK_Top );
			effectsInfoPanel->AddChild( m_entityWorldPosition );

			m_currentTime = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_currentTime->SetMargin( Box2( 5, 5, 0, 0 ) );
			m_currentTime->SetDock( RedGui::DOCK_Top );
			effectsInfoPanel->AddChild( m_currentTime );

			m_effectState = new RedGui::CRedGuiLabel( 10,10,0,0 );
			m_effectState->SetMargin( Box2( 5, 5, 0, 5 ) );
			m_effectState->SetDock( RedGui::DOCK_Top );
			effectsInfoPanel->AddChild( m_effectState );

			m_optymizationList = new RedGui::CRedGuiList( 0, 125, 400, 100 );
			m_optymizationList->SetDock( RedGui::DOCK_Top );
			m_optymizationList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_optymizationList->SetSelectionMode( RedGui::SM_None );
			m_optymizationList->AppendColumn( TXT("Optimization comments"), 300 );
			effectsInfoPanel->AddChild( m_optymizationList );

			m_playDataList = new RedGui::CRedGuiList( 0, 125, 400, 350 );
			m_playDataList->SetDock( RedGui::DOCK_Fill );
			m_playDataList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_playDataList->SetSelectionMode( RedGui::SM_None );
			m_playDataList->AppendColumn( TXT("All active track items play data"), 300 );
			effectsInfoPanel->AddChild( m_playDataList );

			ClearEffectInfo();
		}
	}

	void CDebugWindowLoadedResources::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == true )
		{
			switch( m_tabs->GetActiveTabIndex() )
			{
			case LRT_Texture:
				RefreshTexturesTab();
				break;
			case LRT_Mesh:
				RefreshMeshesTab();
				break;
			case LRT_Effect:
				UpdateEffectTab();
				break;
			}
		}
	}

	Bool CDebugWindowLoadedResources::ValidateEffectOptmialization( CFXState* fxState, TDynArray< String >* comments )
	{
		Bool result = true;
		TDynArray< IFXTrackItemPlayData* > playDatas;
		fxState->GetPlayData( playDatas );

		for( Uint32 i=0; i<playDatas.Size(); ++i )
		{
			if( !playDatas[i]->ValidateOptimization( comments ) )
			{
				// Not optimally made
				result = false;
				if( comments == nullptr )
				{
					break;	// No need to test further
				}
			}
		}

		return result;
	}

	void CDebugWindowLoadedResources::NotifyOnClickedOnEffect( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RED_UNUSED( eventPackage );

		ClearEffectInfo();

		if( selectedIndex != -1 && selectedIndex < (Int32)m_effectsList->GetItemCount() )
		{
			CFXState* effect = m_effectsList->GetItem( selectedIndex )->GetUserData< CFXState >();
			m_selectedEffect = effect;
		}
	}

	void CDebugWindowLoadedResources::ClearEffectInfo()
	{
		m_entityTemplateName->SetText( TXT("Source:") );
		m_entityWorldPosition->SetText( TXT("Position:") );
		m_pathToEffect->SetText( TXT("Name:") );
		m_currentTime->SetText( TXT("Current time:") );
		m_effectState->SetText( TXT("Effect state:") );

		m_optymizationList->RemoveAllItems();
		m_playDataList->RemoveAllItems();
	}

	void CDebugWindowLoadedResources::UpdateEffectTab()
	{
		if ( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if( world != nullptr )
			{
				m_camPos = world->GetCameraPosition();
				CTickManager* tickMgr = GGame->GetActiveWorld()->GetTickManager();
				if( tickMgr != nullptr )
				{
					// get all active effects
					auto& effects = tickMgr->GetEffects();

					// remove old effects
					{
						Uint32 effectCount = m_effectsList->GetItemCount();
						for( Int32 i=0; i<(Int32)effectCount; ++i )
						{
							CFXState* effect = m_effectsList->GetItem( i )->GetUserData< CFXState >();

							const Bool found = effect && effects.Exist( effect );
							if( !effect->IsInitialized() || !found )
							{
								if( effect == m_selectedEffect )
								{
									m_selectedEffect = nullptr;
								}

								m_effectsList->RemoveItem( i );
								--effectCount;
								--i;
							}
						}
					}

					// update list
					{
						struct EffectVisitor
						{
							CDebugWindowLoadedResources* m_window;

							EffectVisitor( CDebugWindowLoadedResources* window )
								: m_window( window )
							{}

							void Process( CFXState* effectInSet )
							{
								CFXState* testEffect = nullptr;

								const Uint32 effectOnListCount = m_window->m_effectsList->GetItemCount();
								for( Uint32 j=0; j<effectOnListCount; ++j )
								{
									CFXState* effect = m_window->m_effectsList->GetItem( j )->GetUserData< CFXState >();
									if( effect != nullptr )
									{
										if( effect == effectInSet )
										{
											testEffect = effectInSet;
											break;
										}
									}
								}

								if( testEffect == nullptr && effectInSet->IsInitialized() )
								{
									Color textColor = Color::WHITE;
									if ( m_window->ValidateEffectOptmialization( effectInSet, nullptr ) == false )
									{
										textColor = Color::YELLOW;
									}
									
									RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( effectInSet->GetDefinition()->GetName().AsString(), effectInSet, textColor );
									m_window->m_effectsList->AddItem( newItem );
								}
							}
						} effectVisitor( this );

						effects.ProcessAll( effectVisitor );
					}
				}
			}
		}

		m_allActiveEffects->SetText( String::Printf( TXT("All effect count: %d"), m_effectsList->GetItemCount() ) );

		if( m_selectedEffect != nullptr )
		{
			ClearEffectInfo();
			UpdateInfoAboutEffect( m_selectedEffect );
		}
	}

	void CDebugWindowLoadedResources::UpdateInfoAboutEffect( CFXState* effect )
	{
		CEntity* entity = effect->GetEntity();
		if( entity )
		{
			const CEntityTemplate* templ = entity->GetEntityTemplate();
			String name(TXT("Non-templated"));
			if( templ )
			{
				name = templ->GetFile()->GetFileName();
			}
			const String& friendlyName = entity->GetFriendlyName();
			const Vector& pos = entity->GetWorldPosition();
			m_entityTemplateName->SetText( String::Printf( TXT("Name: %s"), name.AsChar() ) );
			m_pathToEffect->SetText( String::Printf( TXT("Source: %s"), friendlyName.AsChar() ) );
			m_entityWorldPosition->SetText( String::Printf( TXT("Position: (%0.2f, %0.2f, %0.2f) | Distance: %0.2f | Show distance: %0.2f"), pos.X, pos.Y, pos.Z, m_camPos.DistanceTo( pos ), effect->GetDefinition()->GetShowDistance() ) );
		}
		
		m_currentTime->SetText( String::Printf( TXT("Current time: %1.3f"), effect->GetCurrentTime() ) );

		if ( effect->IsStopping() == true )
		{
			m_effectState->SetText( String::Printf( TXT("Effect state: STOPPING (LOD: %d)"), effect->GetCurrentLOD() ), Color::RED );
		}
		else if ( effect->IsPaused() == true )
		{
			m_effectState->SetText( String::Printf( TXT("Effect state: PAUSED (LOD: %d)"), effect->GetCurrentLOD() ), Color::MAGENTA );
		}
		else
		{
			m_effectState->SetText( String::Printf( TXT("Effect state: RUNNING (LOD: %d)"), effect->GetCurrentLOD() ), Color::WHITE );
		}

		TDynArray< String > optimizationComments;
		if( ValidateEffectOptmialization( effect, &optimizationComments ) == false )
		{
			m_optymizationList->AddItems( optimizationComments );
		}

		// List all active track items play data (the implemented ones :))
		TDynArray< String > description;
		TDynArray< IFXTrackItemPlayData* > playDatas;
		effect->GetPlayData( playDatas );
		for ( Uint32 i=0; i<playDatas.Size(); ++i )
		{
			description.Clear();
			playDatas[i]->GetDescription( description );
			m_playDataList->AddItems( description );
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
