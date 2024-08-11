/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "gameplayFxOutline.h"

#include "renderCollector.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"

#include "../../common/core/gatheredResource.h"
#include "../core/2darray.h"
#include "../engine/renderFragment.h"

extern CGatheredResource resGameplayEffectsOutlineGroups;

// TODO: Ultimately, groups should be specified per-component or something, so we wouldn't need any global names like this
RED_DEFINE_STATIC_NAME( occlusion );
RED_DEFINE_STATIC_NAME( target );

//////////////////////////////////////////////////////////////////////////
///
/// COutlineEffectPostFX
///


COutlineEffectPostFX::COutlineEffectPostFX( CGameplayEffects* parent )
	: IGameplayEffect( parent, EPE_OUTLINE )
{
}

COutlineEffectPostFX::~COutlineEffectPostFX()
{
}

void COutlineEffectPostFX::Init()
{
	/*
	m_outlineStates[ FXOUTLINE_Target ].m_groupName = RED_NAME( target );
	m_outlineStates[ FXOUTLINE_Target ].m_maskStencilBits = LC_TargetOutline;
	m_outlineStates[ FXOUTLINE_Target ].m_maskInternalStencilBits = LC_TargetOutline;
	m_outlineStates[ FXOUTLINE_Target ].m_edgeDetectStencilBits = 0;
	m_outlineStates[ FXOUTLINE_Target ].m_useStencilBuffer = false;
	*/
	m_outlineStates[ FXOUTLINE_Occlusion ].m_groupName = RED_NAME( occlusion );
	m_outlineStates[ FXOUTLINE_Occlusion ].m_maskStencilBits = LC_FoliageOutline;
	m_outlineStates[ FXOUTLINE_Occlusion ].m_maskInternalStencilBits = /*LC_OutlineOccluder | */ LC_FoliageOutline;
	m_outlineStates[ FXOUTLINE_Occlusion ].m_edgeDetectStencilBits = /*LC_OutlineOccluder | */ LC_FoliageOutline;
	m_outlineStates[ FXOUTLINE_Occlusion ].m_useStencilBuffer = true;

	ReloadOutlineSettings();
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );
#endif
}

void COutlineEffectPostFX::Shutdown()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( CNAME( CSVFileSaved ), this );
#endif
	m_outlineGroups.ClearFast();
}

void COutlineEffectPostFX::Tick( Float time )
{

	Bool stillEnabled = false;

	for ( Uint32 i = 0; i < FXOUTLINE_MAX; ++i )
	{
		OutlineState& state = m_outlineStates[ i ];

		if ( state.m_isEnabled && state.m_fadeSpeed != 0.0f )
		{
			state.m_strength += state.m_fadeSpeed * time;
			if ( state.m_strength <= 0.0f )
			{
				state.m_strength	= 0.0f;
				state.m_isEnabled	= false;
				state.m_fadeSpeed	= 0.0f;
			}
			else if ( state.m_strength >= 1.0f )
			{
				state.m_strength	= 1.0f;
				state.m_fadeSpeed	= 0.0f;
			}
		}

		stillEnabled = stillEnabled || state.m_isEnabled;
	}

	SetEnabled( stillEnabled );
}


void COutlineEffectPostFX::Enable( EGameplayEffectOutlineType type, Float fadeTime )
{
	RED_ASSERT( type >= 0 && type < FXOUTLINE_MAX, TXT("Invalid outline type: %d"), type );
	if ( type < 0 || type >= FXOUTLINE_MAX )
	{
		return;
	}

	SetEnabled( true );

	OutlineState& state = m_outlineStates[ type ];

	if ( fadeTime > 0.0f )
	{
		state.m_fadeSpeed = 1.0f / fadeTime;
	}
	else
	{
		state.m_fadeSpeed = 0.0f;
		state.m_strength = 1.0f;
	}

	state.m_isEnabled = true;
}

void COutlineEffectPostFX::Disable( EGameplayEffectOutlineType type, Float fadeTime )
{
	RED_ASSERT( type >= 0 && type < FXOUTLINE_MAX, TXT("Invalid outline type: %d"), type );
	if ( type < 0 || type >= FXOUTLINE_MAX )
	{
		return;
	}


	OutlineState& state = m_outlineStates[ type ];

	if ( fadeTime > 0.0f )
	{
		state.m_fadeSpeed = -1.0f / fadeTime;
	}
	else
	{
		state.m_fadeSpeed = 0.0f;
		state.m_strength = 0.0f;
		state.m_isEnabled = false;
	}
}

Uint8 COutlineEffectPostFX::GetUsedStencilBits() const
{
	Uint8 bits = 0;
	for ( Uint32 i = 0; i < FXOUTLINE_MAX; ++i )
	{
		// Only need to check enabled outlines, that use the stencil buffer
		if ( m_outlineStates[ i ].m_isEnabled && m_outlineStates[ i ].m_useStencilBuffer )
		{
			bits |= m_outlineStates[ i ].m_maskStencilBits;
			bits |= m_outlineStates[ i ].m_edgeDetectStencilBits;
		}
	}
	return bits;
}

Bool COutlineEffectPostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource, ERenderTargetName rtnColorTarget )
{
	PC_SCOPE_PIX( DrawOutlines );
	CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();

	// Several context switches involved, so we'll just use a single Scoped context and set it directly each time.
	CGpuApiScopedDrawContext restoreDrawContext;

	{
		GpuApi::RenderTargetSetup rtOffscreen;
		rtOffscreen.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
		rtOffscreen.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
		rtOffscreen.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );

		//
		// Set up temp render target. Each color channel with be 1.0 where an object using the corresponding outline is.
		//
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		GpuApi::SetupRenderTargets( rtOffscreen );

		GetRenderer()->ClearColorTarget( Vector::ZEROS );

		// Each outline gets marked in a different color channel.
		static const Color outlineIDs[ 4 ] = 
		{
			Color( 255, 0, 0, 0 ),
			Color( 0, 255, 0, 0 ),
			Color( 0, 0, 255, 0 ),
			Color( 0, 0, 0, 255 ),
		};
		for ( Uint32 i = 0; i < FXOUTLINE_MAX; ++i )
		{
			const OutlineState& state = m_outlineStates[ i ];
			if ( state.m_isEnabled )
			{
#ifndef NO_EDITOR_EVENT_SYSTEM
				// Lock mutex. We could get notification of CSV file changed on main thread, which replaces the groups.
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif

				const SOutlineGroup& group = FindOutlineGroup( state.m_groupName );

				if ( state.m_useStencilBuffer )
				{
					Uint8 maskBits = group.m_internalEdges ? state.m_maskInternalStencilBits : state.m_maskStencilBits;
					Int32 refValue = GpuApi::PackDrawContextRefValue( maskBits, maskBits, 0 );
					GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_PostProcAdd_StencilMatchExact, refValue );

					GetRenderer()->GetDebugDrawer().DrawQuad2DEx( 0, 0, frameInfo.m_width, frameInfo.m_height, outlineIDs[ i ] );
				}
				else
				{
					// Restore perspective projection, since we're gonna render some scene elements.
					GetRenderer()->GetStateManager().SetLocalToWorld( nullptr );
					GetRenderer()->GetStateManager().SetCamera( frameInfo.m_camera );

					// Set up a fake "hit proxies" pass, except that we render with a constant proxy ID
					RenderingContext rc( frameInfo.m_camera );
					rc.m_pass = RP_HitProxies;
					rc.m_lightChannelFilter = LCF_AllBitsSet;
					rc.m_lightChannelFilterMask = group.m_internalEdges ? state.m_maskInternalStencilBits : state.m_maskStencilBits;
					rc.m_useConstantHitProxyID = true;
					rc.m_constantHitProxyID = CHitProxyID( outlineIDs[ i ] );

					// PostProcSet has no depth test, which is what we want...
					// "Set" works for now, but maybe in the future we'll need something more advanced, like writing just the color channel
					// for this outline group...
					GpuApi::SetDrawContext(GpuApi::DRAWCONTEXT_PostProcSet, 0 );

					// Render stuff! *should* only get the outlined things, because of the light channel test.
					// Ordering of groups isn't important, since we just need to write a constant value, and depth doesn't matter.
					// If additional types of things need to be outlined, can add more sort groups.
					ERenderingSortGroup groups[] = { RSG_LitOpaque, RSG_LitOpaqueWithEmissive, RSG_Skin, RSG_Hair, RSG_Unlit, RSG_Forward };
					collector.RenderElementsAllPlanesFrontFirst( ARRAY_COUNT(groups), groups, rc, RECG_ALL );
					collector.RenderDynamicFragments( ARRAY_COUNT(groups), groups, rc );

					GetRenderer()->GetStateManager().SetCamera2D();
				}
			}
		}
	}


	{
		// Restore original render target, so we can draw the outlines over the scene.
		GpuApi::RenderTargetSetup rtOriginal;
		rtOriginal.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorSource ) );
		rtOriginal.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
		rtOriginal.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtOriginal );

		// Use linear sampling into the mask, so that fractional outline widths will work.
		GpuApi::TextureRef helperColor = surfaces->GetRenderTargetTex( rtnColorTarget );
		GpuApi::BindTextures( 0, 1, &helperColor, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		GetRenderer()->m_gameplayCharacterOutline->Bind();

		// Because each outline has its own stencil test bits, we need to apply the outline shader separately.
		for ( Uint32 i = 0; i < FXOUTLINE_MAX; ++i )
		{
			const OutlineState& state = m_outlineStates[ i ];
			if ( state.m_isEnabled )
			{
#ifndef NO_EDITOR_EVENT_SYSTEM
				// Lock mutex. We could get notification of CSV file changed on main thread, which replaces the groups.
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif

				const SOutlineGroup& group = FindOutlineGroup( state.m_groupName );


				// Scale colors by outline strength. Would only need to adjust alpha, but we're premultiplied so...
				Vector outlineColor	= group.m_outlineColor * state.m_strength;
				Vector fillColor	= group.m_fillColor * state.m_strength;

				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, fillColor );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, outlineColor );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector( group.m_outlineWidth, static_cast< Float >( i ), 0, 0 ) );

				Int32 refValue = GpuApi::PackDrawContextRefValue( state.m_edgeDetectStencilBits, state.m_edgeDetectStencilBits, 0 );
				GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_PostProcPremulBlend_StencilMatchExact, refValue );

				GetRenderer()->GetDebugDrawer().DrawQuad( Vector(-1, -1, 0, 0), Vector(1, 1, 0, 0), 0.5f );
			}
		}
	}

	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	
	// We dont need swap buffers -> return false
	return false;
}


#ifndef NO_EDITOR_EVENT_SYSTEM
void COutlineEffectPostFX::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( resGameplayEffectsOutlineGroups.GetPath().ToString().ContainsSubstring( GetEventData< String >( data ) ) )
		{
			ReloadOutlineSettings();
		}
	}
}
#endif

void COutlineEffectPostFX::ReloadOutlineSettings()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Lock mutex, since this happens on main thread, but render thread also accesses.
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_groupsMutex );
#endif

	C2dArray* groupsArray = resGameplayEffectsOutlineGroups.LoadAndGet< C2dArray >();

	RED_ASSERT( groupsArray, TXT("Unable to open '%ls'"), resGameplayEffectsOutlineGroups.GetPath().ToString().AsChar() );
	if ( !groupsArray )
	{
		m_outlineGroups.ClearFast();
		return;
	}

	Uint32 numGroups = ( Uint32 )groupsArray->GetNumberOfRows();

	m_outlineGroups.Resize( numGroups );
	for ( Uint32 i = 0; i < numGroups; ++i )
	{
		SOutlineGroup& group = m_outlineGroups[ i ];

		String name = groupsArray->GetValue( TXT( "GroupName" ), i );
		group.m_name = CName( name );

		Vector fillColor, outlineColor;
		Float outlineWidth;
		Bool internalEdges;

		C2dArray::ConvertValue( groupsArray->GetValue( TXT( "FillColor" ), i ), fillColor );
		C2dArray::ConvertValue( groupsArray->GetValue( TXT( "OutlineColor" ), i ), outlineColor );
		C2dArray::ConvertValue( groupsArray->GetValue( TXT( "OutlineWidth" ), i ), outlineWidth );
		C2dArray::ConvertValue( groupsArray->GetValue( TXT( "InternalEdges" ), i ), internalEdges );

		group.m_fillColor = Vector::Clamp4( fillColor, 0.0f, 1.0f );
		group.m_outlineColor = Vector::Clamp4( outlineColor, 0.0f, 1.0f );
		group.m_outlineWidth = outlineWidth;
		group.m_internalEdges = internalEdges;

		// Apply gamma correction to the colors, and premultiply alpha.
		const Float colorGamma = 2.2f;
		group.m_outlineColor.X = powf( group.m_outlineColor.X, colorGamma ) * group.m_outlineColor.W;
		group.m_outlineColor.Y = powf( group.m_outlineColor.Y, colorGamma ) * group.m_outlineColor.W;
		group.m_outlineColor.Z = powf( group.m_outlineColor.Z, colorGamma ) * group.m_outlineColor.W;

		group.m_fillColor.X = powf( group.m_fillColor.X, colorGamma ) * group.m_fillColor.W;
		group.m_fillColor.Y = powf( group.m_fillColor.Y, colorGamma ) * group.m_fillColor.W;
		group.m_fillColor.Z = powf( group.m_fillColor.Z, colorGamma ) * group.m_fillColor.W;
	}
}

const SOutlineGroup& COutlineEffectPostFX::FindOutlineGroup( const CName& name ) const
{
	static const SOutlineGroup defaultGroup = {
		CNAME( default ),
		Vector( 1.0f, 1.0f, 1.0f, 0.0f ),
		Vector( 1.0f, 1.0f, 1.0f, 1.0f ),
		1.0f,
		false,
	};

	for ( Uint32 i = 0; i < m_outlineGroups.Size(); ++i )
	{
		if ( m_outlineGroups[i].m_name == name )
		{
			return m_outlineGroups[i];
		}
	}

	return defaultGroup;
}
