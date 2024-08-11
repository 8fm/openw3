
#include "build.h"

#include "gameplayFxDrunk.h"

#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"

#include "../engine/renderCommands.h"
#include "../core/scriptStackFrame.h"

//////////////////////////////////////////////////////////////////////////
///
/// CSepiaEffectPostFX
///

CDrunkEffectPostFX::CDrunkEffectPostFX( CGameplayEffects* parent )
	: m_timeSinceStart( 0.0f )
	, m_fadeInTime( 0.0f )
	, m_fadeOutTime( 0.0f )
	, m_isFadingOut( false )
	, m_timeScale( 1.0f )
	, m_angleScale( 0.03f )
	, m_baseAngle( 0.05f )
	, m_scale( 1.0f )
	, IGameplayEffect( parent, EPE_DRUNK , EPO_POST_TONEMAPPING )
{
}

CDrunkEffectPostFX::~CDrunkEffectPostFX()
{
}

void CDrunkEffectPostFX::Init()
{
}

void CDrunkEffectPostFX::Shutdown()
{
}

Bool CDrunkEffectPostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource , ERenderTargetName rtnColorTarget )
{
	RED_ASSERT( IsEnabled() );

	PC_SCOPE_PIX(RenderDrunkEffect);		

	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

	// Set new rendertarget
	GpuApi::RenderTargetSetup rtSetupMain;
	rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex(rtnColorTarget) );

	Uint32 width	= frameInfo.m_width;
	Uint32 height	= frameInfo.m_height;

	Uint32 samplerFullWidth = surfaces->GetRenderTarget( rtnColorSource )->GetWidth();
	Uint32 samplerFullHeight = surfaces->GetRenderTarget( rtnColorSource )->GetHeight();

	rtSetupMain.SetViewport( width , height , 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupMain );

	// Bind textures
	GpuApi::TextureRef inputTarget = surfaces->GetRenderTargetTex( rtnColorSource );

	GpuApi::BindTextures( 0, 1, &inputTarget, GpuApi::PixelShader );
	GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

	{
		// Setup the rotating image
		Float timeVector = frameInfo.m_engineTime * m_timeScale;
		Float maxOffset = m_factor * m_scale;

		Float cosA = ::MCos( timeVector ) * maxOffset;
		Float sinA = ::MSin( timeVector ) * maxOffset;

		// Encoded 2x2 matrix for rotation
		Vector rotMatrix( cosA , -sinA , sinA, cosA );

		// Center of the image based of the actual area
		Vector imgCenter( 0.5f * width / Float(samplerFullWidth) , 0.5f * height / Float(samplerFullHeight) , 0.0f , 0.0f );

		stateManager.SetPixelConst( PSC_Custom_1, rotMatrix );
		stateManager.SetPixelConst( PSC_Custom_2, imgCenter );

	}
	{
		// Setting up regular parameters
		Vector parameters( m_scale , m_factor, 1.0f / samplerFullWidth , 1.0f / samplerFullHeight );

		stateManager.SetPixelConst( PSC_Custom_0, parameters );
	}

	// Setup draw context
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );

	GetRenderer()->m_gameplayPostFXDrunk->Bind();

	// Blit
	GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );
	
	// Unbind textures
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	return true;
}

void CDrunkEffectPostFX::Tick( Float time )
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

	if ( m_isFadingOut && (m_timeSinceStart > m_fadeOutTime) )
	{
		m_isFadingOut = false;
		SetEnabled( false );
	}
}

void CDrunkEffectPostFX::Enable( Float fadeInTime )
{
	if( IsEnabled() && !m_isFadingOut )
		return;

	SetEnabled( true );
	m_timeSinceStart = m_factor * fadeInTime;
	m_isFadingOut = false;
	m_fadeInTime = fadeInTime;
}

void CDrunkEffectPostFX::Disable( Float fadeOutTime )
{
	if(! IsEnabled() || m_isFadingOut )
		return;
	
	m_fadeOutTime = fadeOutTime;
	m_timeSinceStart = 1.0f - m_factor * m_fadeOutTime;
	m_isFadingOut = true;
}

void CDrunkEffectPostFX::Scale( Float scale )
{
	m_scale = scale;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SCRIPTS

static void funcEnableDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeInTime, 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_AddDrunkPostFx(fadeInTime) )->Commit();
}

static void funcDisableDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeOutTime, 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_RemoveDrunkPostFx(fadeOutTime) )->Commit();
}

static void funcScaleDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, scale , 1.0f );
	FINISH_PARAMETERS;

	( new CRenderCommand_ScaleDrunkPostFx(scale) )->Commit();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ExportDrunkFxNatives()
{
	NATIVE_GLOBAL_FUNCTION( "EnableDrunkFx" , funcEnableDrunkFx );
	NATIVE_GLOBAL_FUNCTION( "DisableDrunkFx" , funcDisableDrunkFx );
	NATIVE_GLOBAL_FUNCTION( "ScaleDrunkFx" , funcScaleDrunkFx );
}

/////////////////////////////////////////////////////////////////////////////////////////////////
