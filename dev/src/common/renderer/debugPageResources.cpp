/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTexture.h"
#include "renderTextureArray.h"

#ifndef NO_DEBUG_PAGES

#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#ifndef NO_DEBUG_WINDOWS
#include "../engine/debugWindowsManager.h"
#include "../engine/inputBufferedInputEvent.h"
#include "../engine/renderFrame.h"
#endif

struct RenderTypeInfo
{
	CName	m_category;
	Uint32	m_lastCount;
	Uint32	m_count;
	Uint32	m_totalMemSize;
	Float	m_highLightTime;
	Color	m_highLightColor;

	RenderTypeInfo( CName category )
		: m_category( category )
		, m_count( 0 )
		, m_totalMemSize( 0 )
		, m_lastCount( 0 )
		, m_highLightTime( 0.0f )
		, m_highLightColor( Color::WHITE )
	{};
};

struct TextureCategory
{
	CName	m_category;
	Uint32	m_count;
	Uint32	m_totalMemSize;

	TextureCategory( CName category )
		: m_category( category )
		, m_count( 0 )
		, m_totalMemSize( 0 )
	{};
};

static int RenderTypeInfoCompare( const void *arg1, const void *arg2 )
{
	RenderTypeInfo* a = ( RenderTypeInfo* ) arg1;
	RenderTypeInfo* b = ( RenderTypeInfo* ) arg2;
	if ( a->m_totalMemSize < b->m_totalMemSize ) return 1;
	if ( a->m_totalMemSize > b->m_totalMemSize ) return -1;
	return 0;
}

static int TextureCategoryCompare( const void *arg1, const void *arg2 )
{
	TextureCategory* a = ( TextureCategory* ) arg1;
	TextureCategory* b = ( TextureCategory* ) arg2;
	if ( a->m_totalMemSize < b->m_totalMemSize ) return 1;
	if ( a->m_totalMemSize > b->m_totalMemSize ) return -1;
	return 0;
}

/// Debug page with memory status
class CDebugPageRenderResources : public IDebugPage
{
protected:
	TDynArray< RenderTypeInfo >		m_types;

public:
	CDebugPageRenderResources()
		: IDebugPage( TXT("Render resources") )
	{};

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		// Reset hightlights
		for ( TDynArray< RenderTypeInfo >::iterator it=m_types.Begin(); it!=m_types.End(); ++it )
		{
			it->m_highLightColor = Color::WHITE;
			it->m_highLightTime = 0.0f;
		}
	}

	//! Tick the crap
	virtual void OnTick( Float timeDelta )
	{
		IDebugPage::OnTick( timeDelta );

		// Dim the highlight
		for ( TDynArray< RenderTypeInfo >::iterator it=m_types.Begin(); it!=m_types.End(); ++it )
		{
			if ( it->m_highLightTime > 0.0f )
			{
				it->m_highLightTime -= timeDelta;
				if ( it->m_highLightTime < 0.0f )
				{
					it->m_highLightTime = 0.0f;
					it->m_highLightColor = Color::WHITE;
				}
			}
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_LoadedResources );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

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

		// Get all resources
		TDynArray< IDynamicRenderResource* > allResources;
		IDynamicRenderResource::GetAllResources( allResources );

		// Reset 
		for ( TDynArray< RenderTypeInfo >::iterator it=m_types.Begin(); it!=m_types.End(); ++it )
		{
			it->m_lastCount = it->m_count;
			it->m_totalMemSize = 0;
			it->m_count = 0;
		}

		// Generate histogram
		Uint32 totalMemorySize=0;
		TDynArray< TextureCategory > textureCategories;
		TDynArray< TextureCategory > textureArrayCategories;
		for ( TDynArray< IDynamicRenderResource* >::iterator it=allResources.Begin(); it!=allResources.End(); ++it )
		{
			IDynamicRenderResource* res = *it;

			// Skip resources without category
			if ( res->GetCategory() )
			{
				// Get existing category info
				RenderTypeInfo* info = NULL;
				for ( Uint32 j=0; j<m_types.Size(); j++ )
				{
					if ( m_types[j].m_category == res->GetCategory() )
					{
						info = &m_types[j];
						break;
					}
				}

				// Add new category info if not found
				if ( !info )
				{
					info = new ( m_types ) RenderTypeInfo( res->GetCategory() );
				}

				// Update
				info->m_count++;
				info->m_totalMemSize += res->GetUsedVideoMemory();

				// Update global count
				const Uint32 memorySize = res->GetUsedVideoMemory();
				totalMemorySize += memorySize;

				// Well it's a texture, create histogram of texture groups
				if ( res->GetCategory() == CNAME( RenderTexture ) )
				{
					// Get texture category name
					CRenderTexture* texture = static_cast< CRenderTexture* >( res );
					CName textureCategory = texture->GetTextureGroupName();

					// Find matching category
					TextureCategory* catInfo = NULL;
					for ( Uint32 j=0; j<textureCategories.Size(); j++ )
					{
						if ( textureCategories[j].m_category == textureCategory )
						{
							catInfo = &textureCategories[j];
							break;
						}
					}

					// Create new
					if ( !catInfo )
					{
						catInfo = new ( textureCategories ) TextureCategory( textureCategory );
					}

					// Update count
					catInfo->m_count += 1;
					catInfo->m_totalMemSize += memorySize;
				}
				if ( res->GetCategory() == CNAME( RenderTextureArray ) )
				{
					// Get texture category name
					CRenderTextureArray* texture = static_cast< CRenderTextureArray* >( res );
					CName textureCategory = texture->GetTextureGroupName();

					// Find matching category
					TextureCategory* catInfo = NULL;
					for ( Uint32 j=0; j<textureArrayCategories.Size(); j++ )
					{
						if ( textureArrayCategories[j].m_category == textureCategory )
						{
							catInfo = &textureArrayCategories[j];
							break;
						}
					}

					// Create new
					if ( !catInfo )
					{
						catInfo = new ( textureArrayCategories ) TextureCategory( textureCategory );
					}

					// Update count
					catInfo->m_count += 1;
					catInfo->m_totalMemSize += memorySize;
				}
			}

			// Release resource
			res->Release();
		}

		// Sort the elements
		qsort( m_types.TypedData(), m_types.Size(), sizeof( RenderTypeInfo ), &RenderTypeInfoCompare );
		qsort( textureCategories.TypedData(), textureCategories.Size(), sizeof( TextureCategory ), &TextureCategoryCompare );
		qsort( textureArrayCategories.TypedData(), textureArrayCategories.Size(), sizeof( TextureCategory ), &TextureCategoryCompare );

		// Resource list
		{
			// Header
			Uint32 y = 65;
			frame->AddDebugScreenText( 50, y, TXT("Resources"), Color::YELLOW );
			y += 15;

			// Dump categories
			for ( TDynArray< RenderTypeInfo >::iterator it=m_types.Begin(); it!=m_types.End(); ++it )
			{
				// Skip crap with less than MB of data and single instance
				Float memoryCount = it->m_totalMemSize / ( 1024.0f * 1024.0f );
				if ( memoryCount < 1.0f && it->m_count == 1 )
				{
					continue;
				}

				// Update highlight
				if ( it->m_count > it->m_lastCount )
				{
					it->m_highLightColor = Color::RED;
					it->m_highLightTime = 1.0f;
				}
				else if ( it->m_count < it->m_lastCount )
				{
					it->m_highLightColor = Color::GREEN;
					it->m_highLightTime = 1.0f;
				}

				// Draw name with highlight color
				String categoryName = it->m_category.AsString();
				categoryName = categoryName.StringAfter( TXT("Render") );
				frame->AddDebugScreenText( 50, y, categoryName, it->m_highLightColor );

				// Draw data
				frame->AddDebugScreenText( 200, y, String::Printf( TXT("%i"), it->m_count ), Color::WHITE );

				// Draw memory count
				if ( memoryCount < 0.5f )
				{
					memoryCount = memoryCount * 1024.0f;
					frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f KB"), memoryCount ), Color::WHITE );
				}
				else
				{
					frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), memoryCount ), Color::WHITE );
				}

				// Move line 
				y += 15;
			}

#if 0 // GFx 3
			// GUI Renderer textures size hack
			GGpuApiRenderer* guiRenderer = static_cast<GGpuApiRenderer*>( GetRenderer()->GetGUIRenderer() );
			Uint32 guiTextureMemory = guiRenderer->GetGuiTexturesMemory();
			totalMemorySize += guiTextureMemory;
			frame->AddDebugScreenText( 50, y, TXT("GUI Textures"), Color::WHITE );
			frame->AddDebugScreenText( 200, y, String::Printf( TXT("%u"), guiRenderer->GetGuiTexturesCount() ), Color::WHITE );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), guiTextureMemory / ( 1024.f * 1024.f ) ), Color::WHITE );
			y += 15;

			// Total
			y += 10;
			frame->AddDebugScreenText( 130, y, TXT("Total memory:"), Color::GRAY );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), totalMemorySize / ( 1024.0f * 1024.0f ) ), Color::GRAY );

			// GPI api
			y += 20;
			GpuApi::TextureStats stats = GpuApi::GetTextureStats();
			frame->AddDebugScreenText( 50, y, TXT("RAPI System textures") );
			frame->AddDebugScreenText( 200, y, String::Printf( TXT("%i"), stats.m_systemTextureCount ), Color::WHITE );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), stats.m_systemTextureMemory / ( 1024.0f * 1024.0f ) ), Color::WHITE );
			y += 15;
			frame->AddDebugScreenText( 50, y, TXT("RAPI Generic textures") );
			frame->AddDebugScreenText( 200, y, String::Printf( TXT("%i"), stats.m_genericTextureCount ), Color::WHITE );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), stats.m_genericTextureMemory / ( 1024.0f * 1024.0f ) ), Color::WHITE );
			y += 15;
			frame->AddDebugScreenText( 50, y, TXT("RAPI Streamable textures") );
			frame->AddDebugScreenText( 200, y, String::Printf( TXT("%i"), stats.m_streamableTextureCount ), Color::WHITE );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), stats.m_streamableTextureMemory / ( 1024.0f * 1024.0f ) ), Color::WHITE );
			y += 15;
			frame->AddDebugScreenText( 50, y, TXT("RAPI UI textures") );
			frame->AddDebugScreenText( 200, y, String::Printf( TXT("%i"), stats.m_guiTextureCount ), Color::WHITE );
			frame->AddDebugScreenText( 250, y, String::Printf( TXT("%1.2f MB"), stats.m_guiTextureMemory / ( 1024.0f * 1024.0f ) ), Color::WHITE );
			y += 15;
#endif // #if 0
		}

		// List texture categories
		{
			// Header
			Uint32 y = 65;
			frame->AddDebugScreenText( 350, y, TXT("Texture Groups (RenderTexture)"), Color::YELLOW );
			y += 15;

			// Dump texture categories
			for ( TDynArray< TextureCategory >::iterator it=textureCategories.Begin(); it!=textureCategories.End(); ++it )
			{
				// Dump name
				frame->AddDebugScreenText( 350, y, it->m_category.AsString().AsChar(), Color::WHITE );
				frame->AddDebugScreenText( 520, y, String::Printf( TXT("%i"), it->m_count ), Color::WHITE );
				frame->AddDebugScreenText( 560, y, String::Printf( TXT("%1.2fMB"), it->m_totalMemSize / ( 1024.0f * 1024.0f ) ), Color::WHITE );

				// Move line 
				y += 15;
			}

			// Header
			frame->AddDebugScreenText( 350, y, TXT("Texture Groups (TextureArray)"), Color::YELLOW );
			y += 15;

			// Dump texture array categories
			for ( TDynArray< TextureCategory >::iterator it=textureArrayCategories.Begin(); it!=textureArrayCategories.End(); ++it )
			{
				// Dump name
				frame->AddDebugScreenText( 350, y, it->m_category.AsString().AsChar(), Color::WHITE );
				frame->AddDebugScreenText( 520, y, String::Printf( TXT("%i"), it->m_count ), Color::WHITE );
				frame->AddDebugScreenText( 560, y, String::Printf( TXT("%1.2fMB"), it->m_totalMemSize / ( 1024.0f * 1024.0f ) ), Color::WHITE );

				// Move line 
				y += 15;
			}
		}
	}
};

void CreateDebugPageRenderResources()
{
	IDebugPage* page = new CDebugPageRenderResources();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif