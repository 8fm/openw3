/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugWindowsManager.h"
#include "mesh.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"
#include "meshEnum.h"
#include "bitmapTexture.h"

#include "../../common/core/objectIterator.h"

void DumpAllStrings();

/// Interactive list of NPC
class CDebugPageLoadedResources : public IDebugPage
{
protected:
	TDynArray< String > m_notLoadedMeshes;
	TDynArray< String > m_notLoadedTextures;
	Uint32 m_selectedEntry;

	Float m_characterMeshesMB;
	Float m_environmentMeshesMB;
	Float m_itemMeshesMB;
	Float m_otherMeshesMB;

	Float m_characterTexturesMB;
	Float m_environmentTexturesMB;
	Float m_itemTexturesMB;
	Float m_otherTexturesMB;

public:
	CDebugPageLoadedResources()
		: IDebugPage( TXT("Loaded Resources") )
		, m_selectedEntry(0)
		, m_characterMeshesMB(0)
		, m_environmentMeshesMB(0)
		, m_itemMeshesMB(0)
		, m_otherMeshesMB(0)
		, m_characterTexturesMB(0)
		, m_environmentTexturesMB(0)
		, m_itemTexturesMB(0)
		, m_otherTexturesMB(0)
	{}

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		// Collect all meshes
		for ( ObjectIterator< CMesh > it; it; ++it )
		{
			CMesh* mesh = *it;

			if (mesh->ErrorOccuredDuringLoading())
			{
				m_notLoadedMeshes.PushBack(mesh->GetFriendlyName());
			}
			else
			{
				Float sizeInMB = mesh->GetCookedDataSize() / (1024.f * 1024.f);
				if (mesh->GetFriendlyName().ContainsSubstring(TXT("characters")))
				{
					m_characterMeshesMB += sizeInMB;
				}
				else if (mesh->GetFriendlyName().ContainsSubstring(TXT("environment")))
				{
					m_environmentMeshesMB += sizeInMB;
				}
				else if (mesh->GetFriendlyName().ContainsSubstring(TXT("items")))
				{
					m_itemMeshesMB += sizeInMB;
				}
				else
				{
					m_otherMeshesMB += sizeInMB;
				}
			}
		}

		// Collect all textures
		for ( ObjectIterator< CBitmapTexture > it; it; ++it )
		{
			CBitmapTexture* tex = *it;

			if (tex->m_resourceLoadError)
			{
				String info;
				Float sizeInMB = tex->GetCookedData().GetSize() / (1024.f * 1024.f);
				info = info.Printf( TXT( "%s (%u x %u : %1.2f MB)" ), tex->GetFriendlyName().AsChar(), tex->GetWidth(), tex->GetHeight(), sizeInMB );
				m_notLoadedTextures.PushBack( info );
			}
			else
			{
				Float sizeInMB = tex->GetCookedData().GetSize() / (1024.f * 1024.f);
				if (tex->GetFriendlyName().ContainsSubstring(TXT("characters")))
				{
					m_characterTexturesMB += sizeInMB;
				}
				else if (tex->GetFriendlyName().ContainsSubstring(TXT("environment")))
				{
					m_environmentTexturesMB += sizeInMB;
				}
				else if (tex->GetFriendlyName().ContainsSubstring(TXT("items")))
				{
					m_itemTexturesMB += sizeInMB;
				}
				else
				{
					m_otherTexturesMB += sizeInMB;
				}
			}
		}
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();
		m_notLoadedMeshes.Clear();
		m_notLoadedTextures.Clear();

		m_characterMeshesMB = 0;
		m_environmentMeshesMB = 0;
		m_itemMeshesMB = 0;
		m_otherMeshesMB = 0;
		m_characterTexturesMB = 0;
		m_environmentTexturesMB = 0;
		m_itemTexturesMB = 0;
		m_otherTexturesMB = 0;
	}

	void DumpLoadedMeshes()
	{
		FILE* dumpFile = fopen( "meshload_log.txt", "wt" );

		if (dumpFile == NULL)
		{
			return;
		}

		Uint32 hasRenderResourceCount = 0;
		Uint32 hasRenderResourceSize = 0;

		Uint32 numMeshes = 0;
		for ( ObjectIterator<CMesh> it; it; ++it )
		{
			CMesh* mesh = *it;
			numMeshes += 1;

			if (!mesh->ErrorOccuredDuringLoading() && mesh->GetRenderResource() != NULL)
			{
				const Uint32 size = mesh->GetCookedDataSize();
				fprintf( dumpFile, "%s - %u\n", UNICODE_TO_ANSI(mesh->GetFriendlyName().AsChar()), size );
				hasRenderResourceCount++;
				hasRenderResourceSize += size;
			}
		}
		fprintf( dumpFile, "%u mesh(es) loaded:\n\n", numMeshes );

		fprintf( dumpFile, "%u mesh(es) have render resources with size: %u\n\n", hasRenderResourceCount, hasRenderResourceSize );

		for ( ObjectIterator<CMesh> it; it; ++it )
		{
			CMesh* mesh = *it;

			if (!mesh->ErrorOccuredDuringLoading() && mesh->GetRenderResource() == NULL)
			{
				fprintf( dumpFile, "%s - %u\n", UNICODE_TO_ANSI(mesh->GetFriendlyName().AsChar()), mesh->GetCookedDataSize() );
			}
		}
		fflush(dumpFile);
		fclose( dumpFile );

		//FILE* of = fopen( "game:\\Log.txt", "at" );
		//if( !of ) return;
		//fprintf( of, "%s\n", UNICODE_TO_ANSI( string ) );
		//fclose( of );
	}

	void DumpLoadedTextures()
	{
		FILE* dumpFile = fopen( "textureload_log.txt", "wt" );	

		if (dumpFile == NULL)
		{
			return;
		}


		Uint32 hasRenderResourceCount = 0;
		Uint32 hasRenderResourceSize = 0;

		Uint32 numTextures = 0;
		for ( ObjectIterator< CBitmapTexture > it; it; ++it )
		{
			CBitmapTexture* tex = *it;

			if (!tex->m_resourceLoadError && tex->m_renderResource != NULL)
			{
				String name = tex->GetFriendlyName();
				if ( name.ContainsSubstring(TXT("Unnamed")) && tex->GetParent() != NULL )
				{
					name = String::Printf(TXT("Child of %s"), tex->GetParent()->GetFriendlyName().AsChar());
				}
				fprintf( dumpFile, "%s - %u\n", UNICODE_TO_ANSI(name.AsChar()), tex->GetCookedData().GetSize() );
				hasRenderResourceCount++;
				hasRenderResourceSize += static_cast< Uint32 >( tex->GetCookedData().GetSize() );
			}
		}

		fprintf( dumpFile, "%u texture(s) loaded:\n\n", numTextures );

		fprintf( dumpFile, "%u texture(s) have render resources with size: %u\n\n", hasRenderResourceCount, hasRenderResourceSize );

		for ( ObjectIterator< CBitmapTexture > it; it; ++it )
		{
			CBitmapTexture* tex = *it;

			if (!tex->m_resourceLoadError && tex->m_renderResource == NULL)
			{
				fprintf( dumpFile, "%s - %u\n", UNICODE_TO_ANSI(tex->GetFriendlyName().AsChar()), tex->GetCookedData().GetSize() );
			}
		}
		fflush(dumpFile);
		fclose( dumpFile );

		//FILE* of = fopen( "game:\\Log.txt", "at" );
		//if( !of ) return;
		//fprintf( of, "%s\n", UNICODE_TO_ANSI( string ) );
		//fclose( of );
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_LoadedResources );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( ( key == IK_Up || key == IK_W || key == IK_Pad_DigitUp ) && action == IACT_Press )
		{
			if ( m_selectedEntry > 0 )	--m_selectedEntry;
			return true;
		}
		if ( ( key == IK_Down || key == IK_S || key == IK_Pad_DigitDown ) && action == IACT_Press )
		{
			if ( m_selectedEntry < 1 )	++m_selectedEntry;
			return true;
		}

		if ( ( key == IK_LeftMouse || key == IK_Space || key == IK_Enter || key == IK_Pad_A_CROSS ) && action == IACT_Press )
		{
			switch (m_selectedEntry)
			{
			case 0:
				DumpLoadedMeshes();
				break;
			case 1:
				DumpLoadedTextures();
				break;
			}

			return true;
		}

		// Not handled
		return false;
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		// Draw info background
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;
		frame->AddDebugRect( 50, 50, width-100, height-120, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 50, 50, width-100, height-120, Color::WHITE );

		if (m_selectedEntry == 0)
		{
			frame->AddDebugScreenText( 55, 65, TXT("> Dump loaded meshes into file <"), Color::YELLOW );
		}
		else
		{
			frame->AddDebugScreenText( 55, 65, TXT("  Dump loaded meshes into file  "), Color::WHITE );
		}

		if (m_selectedEntry == 1)
		{
			frame->AddDebugScreenText( 55, 75, TXT("> Dump loaded textures into file <"), Color::YELLOW );
		}
		else
		{
			frame->AddDebugScreenText( 55, 75, TXT("  Dump loaded textures into file  "), Color::WHITE );
		}

		String stats = String::Printf(TXT("Character meshes: %1.2f MB"), m_characterMeshesMB);
		frame->AddDebugScreenText( 55, 95, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Environment meshes: %1.2f MB"), m_environmentMeshesMB);
		frame->AddDebugScreenText( 55, 105, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Item meshes: %1.2f MB"), m_itemMeshesMB);
		frame->AddDebugScreenText( 55, 115, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Other meshes: %1.2f MB"), m_otherMeshesMB);
		frame->AddDebugScreenText( 55, 125, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Character textures: %1.2f MB"), m_characterTexturesMB);
		frame->AddDebugScreenText( 455, 95, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Environment textures: %1.2f MB"), m_environmentTexturesMB);
		frame->AddDebugScreenText( 455, 105, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Item textures: %1.2f MB"), m_itemTexturesMB);
		frame->AddDebugScreenText( 455, 115, stats.AsChar(), Color::WHITE );

		stats = String::Printf(TXT("Other textures: %1.2f MB"), m_otherTexturesMB);
		frame->AddDebugScreenText( 455, 125, stats.AsChar(), Color::WHITE );


		frame->AddDebugScreenText( 55, 145, TXT("Meshes not loaded:"), Color::RED );

		for ( Uint32 i = 0; i < m_notLoadedMeshes.Size(); ++i )
		{
			frame->AddDebugScreenText( 55, 155 + 10*i, m_notLoadedMeshes[i].AsChar(), Color::RED );
		}

		frame->AddDebugScreenText( 455, 145, TXT("Textures not loaded:"), Color::RED );

		for ( Uint32 i = 0; i < m_notLoadedTextures.Size(); ++i )
		{
			frame->AddDebugScreenText( 455, 155 + 10*i, m_notLoadedTextures[i].AsChar(), Color::RED );
		}
	}
};

void CreateDebugPageLoadedResources()
{
	IDebugPage* page = new CDebugPageLoadedResources();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
