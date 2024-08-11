/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

// ace_todo!!!! pododawac testy z ktorego watku sie funkcje wolaja zeby wykluczyc misuse

// ace_todo!!!! renderer moglby bardziej korzystac z ustawiania kilku renderstate'ow i tekstur naraz (chodzi mi teraz glownie o crendertexture::bind, ktore rozperdala ta koncepcje)

// ace_todo!!!! niektore rzeczy, takie jak np IsSetupSupported() sa gpuapi independent i powinny byc wyciagniete do jakiegos pliku ktory nie wchodzi w sklad implementacji gpugpi

// ace_todo!!!! po utracie urzadzenie niektore rzeczy powinny byc ustawiane jeszcze raz chyba (np. fillmode, jakies defaultowe shity itd)

// ace_todo!!!! niektore rzeczy sa przez wartosc zamiast przez referencje zwracane. zrobic moze przez referencje... (np gettexturedesc)

// --------------------------------------------------------------------------
// CGpuApiDeviceStateGrabber

sce::Gnm::Buffer				GpuApi::SShaderData::s_graphicsScratchBuffer = sce::Gnm::Buffer();
GpuApi::Uint32					GpuApi::SShaderData::s_graphicsMaxNumWaves = 0;
GpuApi::Uint32					GpuApi::SShaderData::s_graphicsNum1KbyteChunksPerWave = 0;
GpuApi::Uint32					GpuApi::SShaderData::s_graphicsScratchBufferSize = 0;

CGpuApiDeviceStateGrabber::CGpuApiDeviceStateGrabber ( GpuApi::Uint32 categories )
: m_State( GpuApi::GrabDeviceState( categories ) )
{}

CGpuApiDeviceStateGrabber::~CGpuApiDeviceStateGrabber ()
{
	Restore();
}

void CGpuApiDeviceStateGrabber::Restore()
{
	if ( m_State.GetCategories() )
	{
		GpuApi::RestoreDeviceState( m_State );
		m_State.Clear();
		GPUAPI_ASSERT( !m_State.GetCategories() );
	}
}

// --------------------------------------------------------------------------
// CGpuApiScopedDrawContext

CGpuApiScopedDrawContext::CGpuApiScopedDrawContext ( )
{
	m_prevContext	= GpuApi::GetDrawContext();
	m_prevRefValue	= GpuApi::GetDrawContextRefValue();
}

CGpuApiScopedDrawContext::CGpuApiScopedDrawContext ( GpuApi::eDrawContext context, GpuApi::Uint32 refValue )
{
	m_prevContext	= GpuApi::GetDrawContext();
	m_prevRefValue	= GpuApi::GetDrawContextRefValue();
	GpuApi::SetDrawContext( context, refValue );	
}

CGpuApiScopedDrawContext::~CGpuApiScopedDrawContext ()
{
	GpuApi::SetDrawContext( m_prevContext, m_prevRefValue );
}

// --------------------------------------------------------------------------
// CGpuApiScopedTwoSidedRender

CGpuApiScopedTwoSidedRender::CGpuApiScopedTwoSidedRender ()
{
	m_prevTwoSided = GpuApi::IsForcedTwoSidedRender();
}

CGpuApiScopedTwoSidedRender::CGpuApiScopedTwoSidedRender ( GpuApi::Bool forceTwoSidedRendering )
{
	m_prevTwoSided = GpuApi::IsForcedTwoSidedRender();
	GpuApi::SetForcedTwoSidedRender( forceTwoSidedRendering );
}

CGpuApiScopedTwoSidedRender::~CGpuApiScopedTwoSidedRender ()
{
	GpuApi::SetForcedTwoSidedRender( m_prevTwoSided );
}

void CGpuApiScopedTwoSidedRender::SetForcedTwoSided( GpuApi::Bool forceTwoSidedRendering )
{
	m_prevTwoSided = GpuApi::IsForcedTwoSidedRender();
	GpuApi::SetForcedTwoSidedRender( forceTwoSidedRendering );
}
