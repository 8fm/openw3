
#include "build.h"

#include "gameplayFXSurface.h"

#include "renderCollector.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbeManager.h"


// TODO: Ultimately, groups should be specified per-component or something, so we wouldn't need any global names like this
RED_DEFINE_STATIC_NAME( questGroup );
RED_DEFINE_STATIC_NAME( interactiveGroup );

//////////////////////////////////////////////////////////////////////////
///
/// CSurfacePostFX
///

CSurfacePostFX::CSurfacePostFX( CGameplayEffects* parent, const Vector& fillColor )
	: m_timeSinceStart( 0.0f )
	, m_playerPosition( Vector::ZEROS )
	, m_fillColor( fillColor )
	, m_cBuf( NULL )	
	, IGameplayEffect( parent, EPE_SURFACE , EPO_POST_OPAQUE )
	, m_activeGroups( 0 )
{		
	for(Uint32 i=0; i<SURFACE_POST_GROUPS_MAX; i++) m_cBufPtr.PushBack( Vector(0,0,0,0) );

	m_groups.Resize( SURFACE_POST_GROUPS_MAX );

	for( Uint32 i=0; i<SURFACE_POST_GROUPS_MAX; i++ )
	{
		SSurfacePostFXGroup s;
		s.m_fadeInTime = 0.0f;
		s.m_activeTime = 0.0f;		
		s.m_fadeOutTime = 0.0f;
		s.m_position = Vector::ZEROS;
		s.m_range = 8.0f;
		s.m_type = ESurfacePostFXType::ES_Frost;

		s.m_factor = 0.0f;
		s.m_currentLifeTime = 0.0f;			
		s.m_enabled = false;

		m_groups[i] = s;
	}

}

CSurfacePostFX::~CSurfacePostFX()
{
}

void CSurfacePostFX::Init( )
{
}


// HACK -- stat tracking for how much would have been leaked before this fix
static Uint32 HACK_accumulatedLeak = 0;
void CSurfacePostFX::SetFillColor( const Vector& fillColor )
{
	Uint32 bufSize = SURFACE_POST_GROUPS_MAX*4*sizeof(Float);

	if ( !m_cBuf.isNull() )
	{
		HACK_accumulatedLeak += bufSize;
		LOG_RENDERER( TXT("CSurfacePostFX leak prevented : %u total"), HACK_accumulatedLeak );
	}

	GpuApi::SafeRelease( m_cBuf );

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = m_cBufPtr.Data();
	m_cBuf =  GpuApi::CreateBuffer( bufSize, GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, 0, &bufInitData );

	m_fillColor = fillColor;
}

void CSurfacePostFX::Shutdown()
{
	SetEnabled( false );
	m_cBufPtr.ClearFast();

	m_groups.ClearFast();
	SafeRelease(m_cBuf);
}

void CSurfacePostFX::Add( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type )
{
	Int32 freeSlot = -1;	

	for( Uint32 i=0; i<SURFACE_POST_GROUPS_MAX; i++ )
	{
		if( !m_groups[i].m_enabled )
		{
			freeSlot = (Int32)i;			

			for( Uint32 j=0; j<SURFACE_POST_GROUPS_MAX; j++ )
			{
				if( i != j )
				{
					if( m_groups[j].m_enabled )
					{
						if( (m_groups[j].m_position - position).SquareMag3() < 4.0f && m_groups[j].m_range >= range )
						{						
							freeSlot = -1;						
							break;					
						}						
					}					
				}
			}
		}

		if( freeSlot > -1 )
		{
			Uint32 ind = (Uint32)freeSlot;

			m_groups[ ind ].m_fadeInTime = fadeInTime;
			m_groups[ind].m_activeTime = activeTime;		
			m_groups[ind].m_fadeOutTime = fadeOutTime;
			m_groups[ind].m_position = position;
			m_groups[ind].m_range = range;
			m_groups[ind].m_type = (ESurfacePostFXType)type;

			m_groups[ind].m_factor = 0.0f;
			m_groups[ind].m_currentLifeTime = 0.0f;			
			m_groups[ind].m_enabled = true;

			SetEnabled( true );
			break;
		}
	}

#ifndef NO_ASSERTS
	if( !IsEnabled() )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( Script ), TXT( "Unable to add another Surface Post FX, current limit is: %d " ), SURFACE_POST_GROUPS_MAX );
	}
#endif

}

Bool CSurfacePostFX::Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget )
{
	PC_SCOPE_RENDER_LVL1( RenderSurfacePostFX );

	if( m_groups.Empty() ) return false;

	CRenderTextureArray *flowTexArr = frameInfo.m_envParametersDayPoint.m_wetSurfaceTexture.Get< CRenderTextureArray >();
	
	if( !flowTexArr )
	{
		return false;
	}

	const CGpuApiScopedDrawContext baseDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

	{	
		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Set sampler state
		//GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );	

		CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
		{				
			GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( rtnColor );		
			GpuApi::TextureRef sceneDepth =  surfaces->GetDepthBufferTex();		
			
			// copy over base color (we are not rendering over LC_Character at all!)
			GetRenderer()->StretchRect( colorRef, surfaces->GetRenderTargetTex( rtnTarget ) );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnTarget ) );		
			// need stencil for dynamic object cut-out
			rtSetup.SetDepthStencilTarget( sceneDepth, -1, true );
			rtSetup.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );

			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );			

			flowTexArr->Bind( 4, RST_PixelShader );						
			GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );

			// draw over static stuff
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchNone, LC_DynamicObject | LC_Characters );		
						
			stateManager.SetPixelConst( PSC_Custom_1, Vector( (Float)m_activeGroups, 0.0f, 0.0f, 0.0f ) );

			// Set parameters
			GpuApi::BindConstantBuffer( 5, m_cBuf, GpuApi::PixelShader );		
			GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip );			
			GpuApi::SetSamplerStateCommon( 4, 1, GpuApi::SAMPSTATEPRESET_WrapLinearMip );

			GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchExact, GpuApi::PackDrawContextRefValue( LC_Default, LC_Default|LC_Characters, 0 ) );

			GetRenderer()->m_gameplayPostFXSurface->Bind();		

			// draw over non-character stuff
			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 0), Vector (1, 1, 0, 0), 0.5f );
			
			// if not using swap, copy to color manually
			GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( rtnTarget ), colorRef );			

			// Unbind textures
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );	
			GpuApi::BindTextures( 4, 1, nullptr, GpuApi::PixelShader );	
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );		
		}

		// Restore original rendertargets
		targetStateGrabber.Restore();	

	}

	// We dont need swap buffers -> return false
	return false;
}


void CSurfacePostFX::Tick( Float time )
{	
	m_activeGroups = 0;

	for(Int32 i=m_groups.SizeInt()-1; i>=0; i--)
	{
		if( m_groups[i].m_enabled )
		{		
			Bool markedForRemoval = false;

			m_groups[i].m_currentLifeTime += time;

			// finished fade in
			if( m_groups[i].m_currentLifeTime > m_groups[i].m_fadeInTime )
			{
				// finished active life
				if( m_groups[i].m_currentLifeTime > ( m_groups[i].m_fadeInTime + m_groups[i].m_activeTime ) )
				{
					// effect is gone
					if( m_groups[i].m_currentLifeTime >= ( m_groups[i].m_fadeInTime + m_groups[i].m_activeTime + m_groups[i].m_fadeOutTime ) )
					{							
						m_groups[i].m_enabled = false;
						m_cBufPtr[i] = Vector::ZEROS;

						markedForRemoval = true;
					}
					// currently fading out
					else
					{
						Float timeToDissolve = Clamp<Float>( m_groups[i].m_fadeOutTime - (m_groups[i].m_currentLifeTime - (m_groups[i].m_fadeInTime + m_groups[i].m_activeTime)), 0.0f, m_groups[i].m_fadeOutTime );
						m_groups[i].m_factor = Clamp<Float>( timeToDissolve/m_groups[i].m_fadeOutTime, 0.0f, 0.99f );
					}
				}
				// currently stays active
				else
				{
					m_groups[i].m_factor = 0.99f;				// frac in shader!
				}
			}
			// currently fading in
			else
			{
				m_groups[i].m_factor = Clamp<Float>( m_groups[i].m_currentLifeTime/m_groups[i].m_fadeInTime, 0.0f, 0.99f );

			}

			if( !markedForRemoval )
			{			
				Float resultingRange = floor( m_groups[i].m_range );
				Float packW = m_groups[i].m_factor + resultingRange + 1000.0f*(Float)( m_groups[i].m_type + 1.0f );

				m_cBufPtr[i] = Vector( m_groups[i].m_position.X, m_groups[i].m_position.Y, m_groups[i].m_position.Z, packW );

				++m_activeGroups;
			}
		}
	}

	UpdateBuffer();

	if( m_activeGroups == 0 ) SetEnabled( false );
}

Uint8 CSurfacePostFX::GetUsedStencilBits()
{
	return LC_DynamicObject;
}

void CSurfacePostFX::UpdateBuffer()
{
	char* data = (char*)GpuApi::LockBuffer( m_cBuf, GpuApi::BLF_Discard, 0, SURFACE_POST_GROUPS_MAX*4*sizeof(Float) );
	if( data )
	{
		Vector* pixels = (Vector*)(data);
		Red::System::MemoryCopy( pixels, m_cBufPtr.Data(), SURFACE_POST_GROUPS_MAX*4*sizeof(Float) );
	}
	GpuApi::UnlockBuffer( m_cBuf );	
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CSurfacePostFX::DispatchEditorEvent( const CName& name, IEdEventData* data )
{	
}
#endif
