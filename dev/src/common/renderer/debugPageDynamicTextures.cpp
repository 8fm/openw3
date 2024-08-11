/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderMaterial.h"

#ifndef NO_DEBUG_WINDOWS
#include "../engine/debugWindowsManager.h"
#endif
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/inputBufferedInputEvent.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

/// Debug page with dynamic textures stats ( render targets )
class CDebugPageDynamicTextures : public IDebugPage
{
public:
	Int32		m_textureCount;
	Int32		m_textureIndex;
	Int32		m_currentSlice;
	Int32		m_currentMip;
	Int32		m_maxSlices;
	Int32		m_maxMips;
	Float	m_minColor;
	Float	m_maxColor;

public:
	CDebugPageDynamicTextures()
		: IDebugPage( TXT("Dynamic Textures") )
		, m_textureCount( 0 )
		, m_textureIndex( 0 )
		, m_currentSlice( 0 )
		, m_currentMip( 0 )
		, m_maxSlices( 1 )
		, m_maxMips( 1 )
		, m_minColor( 0.0f )
		, m_maxColor( 1.0f )
	{}

	//! Refresh preview settings
	void refreshTexture()
	{
		// get texture info
		GpuApi::TextureRef textureRef = GpuApi::GetDynamicTextureRef( m_textureIndex );
		if ( textureRef )
		{
			GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( textureRef );
			m_maxMips = desc.initLevels;
			m_maxSlices = desc.sliceNum;
		}

		// clamp mip and slice index
		m_currentSlice = Clamp<Int32>( m_currentSlice, 0, m_maxSlices-1 );
		m_currentMip = Clamp<Int32>( m_currentSlice, 0, m_maxMips-1 );

		//GpuApi::TextureRef textureRef = GpuApi::GetDynamicTextureRef( m_textureIndex );
		GetRenderer()->SetTexturePreview( textureRef, m_currentMip, m_currentSlice, m_minColor, m_maxColor );
	}

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		// get number of dynamic textures
		m_textureCount = GpuApi::GetNumDynamicTextures();
		m_textureIndex = 0;
		m_minColor = 0.0f;
		m_maxColor = 1.0f;
		m_maxMips = 1;
		m_maxSlices = 1;

		// refresh preview
		// refreshTexture();
	}

	//! Page hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();
		GetRenderer()->SetTexturePreview( GpuApi::TextureRef::Null(), 0, 0, 0.0f, 1.0f );
	}

	//! Tick the crap
	virtual void OnTick( Float timeDelta )
	{
		IDebugPage::OnTick( timeDelta );
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_DynamicTextures );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Send the event
		if ( action == IACT_Press )
		{
			switch ( key )
			{
			case IK_PageDown:
				if ( m_textureIndex < m_textureCount )
					m_textureIndex += 1;
				else
					m_textureIndex = 0;
				break;

			case IK_PageUp:
				if ( m_textureIndex > 0 )
					m_textureIndex -= 1;
				else
					m_textureIndex = m_textureCount-1;
				break;

			case IK_Comma:
				if ( m_currentSlice > 0 )
					m_currentSlice = m_currentSlice-1;
				else
					m_currentSlice = m_maxSlices-1;
				break;

			case IK_Period:
				if ( m_currentSlice < m_maxSlices )
					m_currentSlice *= 1;
				else
					m_currentSlice = 0;
				break;

			case IK_Minus:
				m_maxColor /= 1.1f;
				if ( m_maxColor < m_minColor ) m_minColor = m_maxColor - 0.01f;
				break;

			case IK_Equals:
				m_maxColor *= 1.1f;
				break;

			case IK_LeftBracket:
				m_minColor = m_minColor - 0.1f*m_maxColor;
				break;

			case IK_RightBracket:
				m_minColor = m_minColor + 0.1f*m_maxColor;
				if ( m_minColor > m_maxColor ) m_maxColor = m_minColor + m_maxColor * 0.1f;
				break;

			case IK_R:
				m_minColor = 0.0f;
				m_maxColor = 1.0f;
				break;

			}

			refreshTexture();
		}

		// Not processed
		return false;
	}

	static const Char* GpiApiFormatToStr( GpuApi::eTextureFormat format )
	{
		switch ( format )
		{
			case GpuApi::TEXFMT_A8: return TXT("A8");
			case GpuApi::TEXFMT_L8: return TXT("L8");
			case GpuApi::TEXFMT_R8G8B8X8: return TXT("RGBX8");
			case GpuApi::TEXFMT_R8G8B8A8: return TXT("RGBA8");
			case GpuApi::TEXFMT_A8L8: return TXT("A8L8");
			case GpuApi::TEXFMT_Uint_16_norm: return TXT("Uint16Norm");
			case GpuApi::TEXFMT_Uint_16: return TXT("Uint16");
			case GpuApi::TEXFMT_Uint_32: return TXT("Uint32");
			case GpuApi::TEXFMT_R16G16_Uint: return TXT("RG16");
			case GpuApi::TEXFMT_Float_R10G10B10A2: return TXT("RGB10A2");
			case GpuApi::TEXFMT_Float_R16G16B16A16: return TXT("RGBA16F");
			case GpuApi::TEXFMT_Float_R11G11B10: return TXT("R11G11B10F");
			case GpuApi::TEXFMT_Float_R16G16: return TXT("RG16F");
		#ifdef RED_PLATFORM_DURANGO
			case GpuApi::TEXFMT_R10G10B10_6E4_A2_FLOAT: return TXT("R10G10B10_6E4_A2_FLOAT");
		#endif
			//dex++
			case GpuApi::TEXFMT_Float_R32G32: return TXT("RG32F");
			//dex--
			case GpuApi::TEXFMT_Float_R32G32B32A32: return TXT("RGBA32F");
			case GpuApi::TEXFMT_Float_R32: return TXT("R32F");
			case GpuApi::TEXFMT_Float_R16: return TXT("R16F");
			case GpuApi::TEXFMT_D24S8: return TXT("D24S8");
			case GpuApi::TEXFMT_D24FS8: return TXT("D24FS8");
			case GpuApi::TEXFMT_BC1: return TXT("BC1");
			case GpuApi::TEXFMT_BC2: return TXT("BC2");
			case GpuApi::TEXFMT_BC3: return TXT("BC3");
			case GpuApi::TEXFMT_BC4: return TXT("BC4");
			case GpuApi::TEXFMT_BC5: return TXT("BC5");
			case GpuApi::TEXFMT_BC6H: return TXT("BC6H");
			case GpuApi::TEXFMT_BC7: return TXT("BC7");
			case GpuApi::TEXFMT_R8_Uint: return TXT("R8_Uint");
		}

		return TXT("Unknown");
	}

	static const Char* GpiApiTexTypeToStr( GpuApi::eTextureType type )
	{
		switch ( type )
		{
			case GpuApi::TEXTYPE_2D: return TXT("2D");
			case GpuApi::TEXTYPE_CUBE: return TXT("CUBE");
			case GpuApi::TEXTYPE_ARRAY: return TXT("ARRAY");
		}

		return TXT("Unknown");
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

		GpuApi::TextureRef textureRef = GpuApi::GetDynamicTextureRef( m_textureIndex );
		const char* textureName = GpuApi::GetDynamicTextureName( m_textureIndex );
		if ( textureRef && textureName )
		{
			GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( textureRef );

			Uint32 x = 100;
			Uint32 y = 100;
			frame->AddDebugScreenFormatedText( x, y, TXT("Name: '%" ) RED_PRIWas TXT("'"),  textureName ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("Format: %s"), GpiApiFormatToStr(desc.format) ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("Type: %s"), GpiApiTexTypeToStr(desc.type) ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("Size: %dx%d"), desc.width, desc.height ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("MipLevel: %d/%d"), m_currentMip, desc.initLevels ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("ArraySlice: %d/%d"), m_currentSlice, desc.sliceNum ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("MinColor: %f"), m_minColor ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, TXT("MaxColor: %f"), m_maxColor ); y += 15;
		}
	}
};

void CreateDebugPageDynamicTextures()
{
	IDebugPage* page = new CDebugPageDynamicTextures();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

/// Debug page with dynamic textures stats ( render targets )
class CDebugPageRenderMaterials : public IDebugPage
{
public:
	CDebugPageRenderMaterials()
		: IDebugPage( TXT("Render Materials") )
	{}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		Int32 x = 100;
		Int32 y = 300;

		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( CRenderMaterial::st_recompileMutex );

		Uint32 currentlyRecompilingMaterialsCount = CRenderMaterial::st_currentlyRecompilingMaterials.Size();

		frame->AddDebugScreenText( x, y, String::Printf( TXT( "Number of materials under compilation: %d" ), currentlyRecompilingMaterialsCount )  ); y += 15;

		// register material as being currently recompiled
		for ( Uint32 i = 0; i < CRenderMaterial::st_currentlyRecompilingMaterials.Size(); ++i )
		{
			CRenderMaterial* mat = CRenderMaterial::st_currentlyRecompilingMaterials[ i ];
			frame->AddDebugScreenText( x, y, String::Printf( TXT( "  %d: '%ls'" ), i, mat->GetDisplayableName().AsChar() )  );
			y += 15;
		}
	}
};

void CreateDebugPageRenderMaterials()
{
	IDebugPage* page = new CDebugPageRenderMaterials();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
