/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderSimDiffusion2D.h"
#include "renderShaderPair.h"

/////////////////////////////////////////////////////////////////
CRenderSimDiffusion2D::CRenderSimDiffusion2D() :
	m_tex1( GpuApi::TextureRef::Null() ),
	m_tex2( GpuApi::TextureRef::Null() ),
	m_resolution( 0 ),
	m_flipped( false ),
	m_initialized( false ),
	m_cameraPreviousForward( Vector(0.0f, 1.0f, 0.0f) ),
	m_timeAccum( 0.0f )
{

}

CRenderSimDiffusion2D::~CRenderSimDiffusion2D()
{
	GpuApi::SafeRelease(m_tex1);
	GpuApi::SafeRelease(m_tex2);
}

void CRenderSimDiffusion2D::Initialize( )
{
	if( !m_initialized )
	{
		m_initialized = false;
		m_points[2].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );
		m_points[1].Set( Vector(  1, -1,  0 ),	Color::WHITE, 1, 1 );
		m_points[0].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
		m_points[5].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );
		m_points[4].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
		m_points[3].Set( Vector( -1,  1,  0 ),	Color::WHITE, 0, 0 );
		m_resolution = DIFFUSIONMAPRESOLUTION;

		GpuApi::TextureDesc texDescTex1;
		texDescTex1.type = GpuApi::TEXTYPE_2D;
		texDescTex1.format = GpuApi::TEXFMT_Float_R16G16;
		texDescTex1.initLevels = 1;
		texDescTex1.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDescTex1.width = m_resolution;
		texDescTex1.height = m_resolution;
		m_tex1 = GpuApi::CreateTexture( texDescTex1, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tex1, "simDiffusion2D_tex1" );

		GpuApi::TextureDesc texDescTex2;
		texDescTex2.type = GpuApi::TEXTYPE_2D;
		texDescTex2.format = GpuApi::TEXFMT_Float_R16G16;
		texDescTex2.initLevels = 1;
		texDescTex2.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDescTex2.width = m_resolution;
		texDescTex2.height = m_resolution;
		m_tex2 = GpuApi::CreateTexture( texDescTex2, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tex2, "simDiffusion2D_tex2" );

		Vector clearColor = Vector(0.0f, 0.0f, 0.0f, 0.0f);
		GetRenderer()->ClearColorTarget( m_tex1, clearColor );
		GetRenderer()->ClearColorTarget( m_tex2, clearColor );

		if( m_tex1 && m_tex2 )
		{
			m_initialized = true;
		}
	}
}

GpuApi::TextureRef CRenderSimDiffusion2D::Calculate( GpuApi::TextureRef helperMask, const Float frameTime, const Vector& cameraForward )
{	
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	GpuApi::RenderTargetSetup rtSetupOld;
	rtSetupOld = GpuApi::GetRenderTargetSetup();
	
	CStandardRand frandom = GetRenderer()->GetRandomNumberGenerator();		
	Vector data( Vector::Dot3( m_cameraPreviousForward, cameraForward ), frandom.Get<Float>(),  frandom.Get<Float>(), frandom.Get<Float>() );
	
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, data );

	// required by focus mode, to not to diffuse too much wwhen rotating the camera like crazy
	if( m_timeAccum > 0.2f )
	{		
		m_timeAccum = 0.0f;
		m_cameraPreviousForward = cameraForward;
	}
	m_timeAccum += frameTime;

	// pong ping
	PC_SCOPE_RENDER_LVL1(RenderDiffusion2D);	
	{
		GpuApi::TextureRef currentTexOutput = m_flipped ? m_tex2 : m_tex1;
		GpuApi::TextureRef currentTexInput = m_flipped ? m_tex1 : m_tex2;
		
		GpuApi::RenderTargetSetup rtSetup1;
		rtSetup1.SetColorTarget(0, currentTexOutput);
		rtSetup1.SetViewport(m_resolution, m_resolution);
		GpuApi::SetupRenderTargets(rtSetup1);

		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
		GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );
		
		GpuApi::TextureRef diffTex[2] = { currentTexInput, helperMask };
		GpuApi::BindTextures( 0, 2, &(diffTex[0]), GpuApi::PixelShader );		

		const GpuApi::SamplerStateRef &stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
		GpuApi::SetSamplerState( 0, stateRef, GpuApi::PixelShader );
		GpuApi::SetSamplerState( 1, stateRef, GpuApi::PixelShader );
		
		GetRenderer()->m_shaderDiffusion2D->Bind();	
		GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
		m_flipped = !m_flipped;
		GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
		GpuApi::SetupRenderTargets(rtSetupOld);
		return currentTexOutput;		
	}	
}
