/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "gameplayFXCatView.h"

#include "renderCollector.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderPostProcess.h"

using namespace PostProcessUtilities;

#include "../engine/renderFragment.h"
#include "../engine/renderCommands.h"
#include "../core/scriptStackFrame.h"

//////////////////////////////////////////////////////////////////////////
///
/// CCatViewEffectPostFX
///

CCatViewEffectPostFX::CCatViewEffectPostFX( CGameplayEffects* parent )
	: m_timeSinceStart( 0.0f )
	, m_fadeInTime( 0.0f )
	, m_fadeOutTime( 0.0f )
	, m_isFadingOut( false )
	, m_viewRange( 15.0f )
	, m_tintColorNear( 1.2f, 1.0f, 0.8f, 0.0f )
	, m_tintColorFar( 1.2f, 1.0f, 0.8f, 0.0f )
	, m_playerPosition( Vector::ZEROS )
	, m_desaturation( 1.0f )
	, m_blurStrength( 1.5f )
	, m_brightStrength( 3.0f )
	, m_pulseBase( 0.5f )
	, m_pulseScale( 0.25f )
	, m_pulseSpeed( 1.0f )
	, m_hightlightColor( 1.0f , 0.0f ,0.0f , 3.0f )	// RED x3 multiply
	, m_blurSize( 12.0f )
	, m_hightlightInterior( 0.05f )
	, m_fogStartOffset( 50.0f )
	, m_fogDensity( 0.2f )
	, IGameplayEffect( parent, EPE_CAT_VIEW , EPO_POST_TONEMAPPING )
{
}

CCatViewEffectPostFX::~CCatViewEffectPostFX()
{
	
}

void CCatViewEffectPostFX::Init()
{
}

void CCatViewEffectPostFX::Shutdown()
{
}

Bool CCatViewEffectPostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget )
{
	PC_SCOPE_RENDER_LVL1( CatViewMode );

	RED_ASSERT( IsEnabled() );

	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

	ERenderTargetName helperColor = RTN_PostProcessTempHalf3;
	ERenderTargetName helperFinal = RTN_PostProcessTempHalf1;

	const Uint32 width  = frameInfo.m_width;
	const Uint32 height = frameInfo.m_height;
	const Uint32 sampleFullWidth = surfaces->GetRenderTarget(helperColor)->GetWidth();
	const Uint32 sampleFullHeight = surfaces->GetRenderTarget(helperColor)->GetHeight();

	TexelArea halfArea( width/2 , height/2 );
	TexelArea renderArea( width , height );

	Vector halfSizeParams( (halfArea.m_width - 0.5f) / sampleFullWidth , (halfArea.m_height - 0.5f) / sampleFullHeight , 1.0f/sampleFullWidth , 1.0f/sampleFullHeight );
	Vector halfSizeScale( 0.5f * halfSizeParams.Z / halfSizeParams.X , 0.5f * halfSizeParams.W / halfSizeParams.Y , halfSizeParams.Z , halfSizeParams.W );

	{
		PC_SCOPE_PIX(BlurCatView);		

		GpuApi::RenderTargetSetup rtSetup;
		GpuApi::TextureRef	target = surfaces->GetRenderTargetTex( helperFinal );
		rtSetup.SetColorTarget( 0, target );
		rtSetup.SetViewportFromTarget( target );
		GpuApi::SetupRenderTargets( rtSetup );	

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		GpuApi::TextureRef stencilTexture = surfaces->GetDepthBufferTex();
		GpuApi::BindTextureStencil( 0, stencilTexture, GpuApi::PixelShader );

		const Vector calculateTexCoordTransform = CalculateTexCoordTransform( halfArea, halfArea, sampleFullWidth , sampleFullHeight );
		stateManager.SetVertexConst( VSC_Custom_0,	calculateTexCoordTransform );

		Vector parameters( m_blurSize , m_hightlightInterior , 0.0f, 0.0f );

		stateManager.SetPixelConst( PSC_Custom_0, parameters );
		stateManager.SetPixelConst( PSC_Custom_1, halfSizeParams );

		GetRenderer()->m_gameplayPostFXCatViewObjects->Bind();

		GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );

		GpuApi::BindTextures( 0, 0, nullptr, GpuApi::PixelShader );

	}

	{
		PC_SCOPE_PIX(RenderCatView);		

		// Set new rendertarget
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex(rtnTarget) );
		rtSetupMain.SetViewport( width, height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetupMain );

		// Bind textures
		GpuApi::TextureRef inputTarget[] = { 
			surfaces->GetRenderTargetTex( rtnColor ), 
			surfaces->GetDepthBufferTex() , 
			surfaces->GetRenderTargetTex( helperFinal )
		};

		GpuApi::BindTextures( 0, 3, inputTarget, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 2, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 3, 1, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );

		const Vector calculateTexCoordTransform = CalculateTexCoordTransform( renderArea , halfArea , sampleFullWidth , sampleFullHeight );
		stateManager.SetVertexConst( VSC_Custom_0,	calculateTexCoordTransform );

		// Set parameters (all depends on m_factor)
		Vector parameters( 
			m_factor , 
			m_factor * m_desaturation , 
			m_factor * m_blurStrength , 
			m_brightStrength );

		Vector viewParameters( 
			1.0f / m_viewRange , 
			m_pulseScale * Red::Math::MSin( frameInfo.m_frameTime * m_pulseSpeed ) + m_pulseBase , 
			0.0f, 
			0.0f );

		stateManager.SetPixelConst( PSC_Custom_0, parameters );
		stateManager.SetPixelConst( PSC_Custom_1, viewParameters );
		stateManager.SetPixelConst( PSC_Custom_2, m_tintColorNear );
		stateManager.SetPixelConst( PSC_Custom_3, m_tintColorFar );

		if( m_autoPositioning && GGame && GGame->GetPlayerEntity() ) 
		{
			CEntity* player = GGame->GetPlayerEntity();
			stateManager.SetPixelConst( PSC_Custom_4, player->GetLocalToWorld().GetTranslation() );
		}
		else
		{
			stateManager.SetPixelConst( PSC_Custom_4, m_playerPosition );
		}
				
		stateManager.SetPixelConst( PSC_Custom_5, m_hightlightColor * m_hightlightColor.W );
		stateManager.SetPixelConst( PSC_Custom_6, halfSizeScale );

		// Setup draw context
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

		GetRenderer()->m_gameplayPostFXCatView->Bind();

		// Blit
		GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );
	}

	// Unbind textures
	GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );

	return true;
}

static void SetFogMultiplayers( Float targetStartOffset, Float targetDensity, Float blendScale )
{
	CEnvGlobalFogParameters::m_fogStartOffset			= ::Lerp( blendScale , 0.0f , targetStartOffset );
	CEnvGlobalFogParameters::m_fogDensityMultiplier		= ::Lerp( blendScale , 1.0f , targetDensity );
}

void CCatViewEffectPostFX::Tick( Float time )
{
	m_timeSinceStart += time;

	if ( m_isFadingOut )
	{
		m_factor = 1.0f - (Clamp<Float>( m_timeSinceStart, 0.0f, Max<Float>( m_fadeOutTime, 0.0001f ) ) / Max<Float>( m_fadeOutTime, 0.0001f ));
	}
	else
	{
		m_factor = Clamp<Float>( m_timeSinceStart, 0.0f, Max<Float>( m_fadeInTime, 0.0001f ) ) / Max<Float>( m_fadeInTime, 0.0001f );
	}

	SetFogMultiplayers( m_fogStartOffset, m_fogDensity, m_factor );

	if ( m_isFadingOut && (m_timeSinceStart > m_fadeOutTime) )
	{
		m_isFadingOut = false;
		SetEnabled( false );

		SetFogMultiplayers( 0.0f, 1.0f, 0.0f );
	}
}


void CCatViewEffectPostFX::Reset()
{
	SetFogMultiplayers( 0.0f, 1.0f, 0.0f );
}

void CCatViewEffectPostFX::Enable( Float fadeInTime )
{
	if( IsEnabled() && !m_isFadingOut )
		return;

	SetEnabled( true );
	m_timeSinceStart = m_factor * fadeInTime;
	m_isFadingOut = false;
	m_fadeInTime = fadeInTime;
}

void CCatViewEffectPostFX::Disable( Float fadeOutTime )
{
	if( !IsEnabled() || m_isFadingOut )
		return;

	m_fadeOutTime = fadeOutTime;
	m_timeSinceStart = 1.0f - m_factor * m_fadeOutTime;
	m_isFadingOut = true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// SCRIPTS
//
// TODO: generalize this ugly functions

static void funcEnableCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeInTime, 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_EnableCatViewPostFx(fadeInTime) )->Commit();
}

static void funcDisableCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeOutTime, 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_DisableCatViewPostFx(fadeOutTime) )->Commit();
}

static void funcSetPositionCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position , Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, autoPositioning, false );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetPlayerPosition(position,autoPositioning) )->Commit();
}

static void funcSetTintColorsCatView(IScriptable* context,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, tintNear , Vector::ZEROS );
	GET_PARAMETER( Vector, tintFar , Vector::ZEROS );
	GET_PARAMETER_OPT( Float, desaturation , 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetTintColors(tintNear,tintFar,desaturation) )->Commit();
}

static void funcSetBrightnessCatView( IScriptable* context,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, brightStrength, 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetBrightness(brightStrength) )->Commit();
}

static void funcSetViewRangeCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, viewRanger, 30.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetViewRange(viewRanger) )->Commit();
}

static void funcSetHightlightCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, color, Vector::ZEROS );
	GET_PARAMETER_OPT( Float, hightlightInterior, 0.05f );
	GET_PARAMETER_OPT( Float, blurSize, 1.5f );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetHightlight(color,hightlightInterior,blurSize) )->Commit();
}

static void funcSetFogDensityCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, density, 0.0f );
	GET_PARAMETER_OPT( Float, startOffset, 0.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_CatViewSetFog(density,startOffset) )->Commit();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ExportCatViewFxNatives()
{
	NATIVE_GLOBAL_FUNCTION( "EnableCatViewFx" , funcEnableCatView );
	NATIVE_GLOBAL_FUNCTION( "DisableCatViewFx" , funcDisableCatView );
	NATIVE_GLOBAL_FUNCTION( "SetPositionCatViewFx" , funcSetPositionCatView );
	NATIVE_GLOBAL_FUNCTION( "SetTintColorsCatViewFx" , funcSetTintColorsCatView );
	NATIVE_GLOBAL_FUNCTION( "SetBrightnessCatViewFx" , funcSetBrightnessCatView );
	NATIVE_GLOBAL_FUNCTION( "SetViewRangeCatViewFx" , funcSetViewRangeCatView );
	NATIVE_GLOBAL_FUNCTION( "SetHightlightCatViewFx" , funcSetHightlightCatView );
	NATIVE_GLOBAL_FUNCTION( "SetFogDensityCatViewFx" , funcSetFogDensityCatView );
}

/////////////////////////////////////////////////////////////////////////////////////////////////
