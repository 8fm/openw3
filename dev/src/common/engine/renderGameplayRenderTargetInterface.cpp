 /**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/

#include "build.h"

#include "renderGameplayRenderTargetInterface.h"

IRenderGameplayRenderTarget::IRenderGameplayRenderTarget()
	: m_copyMode( CopyMode::Normal )
	, m_imageHeight( 0 )
	, m_imageWidth( 0 )
	, m_textureHeight( 0 )
	, m_textureWidth( 0 )
	, m_textureMask( NULL )
	, m_backgroundColor( 0.0f, 0.0f, 0.0f, 0.0f )
{

}

IRenderGameplayRenderTarget::~IRenderGameplayRenderTarget()
{
	if( m_textureMask )
	{
		m_textureMask->Release();
		m_textureMask = NULL;
	}
}

void IRenderGameplayRenderTarget::SetNormal()
{
	m_copyMode = CopyMode::Normal;
}

void IRenderGameplayRenderTarget::SetTextureMask( IRenderResource* textureMask ) 
{ 
	m_copyMode = CopyMode::TextureMask;

	if ( m_textureMask )
	{
		m_textureMask->Release();
	}
	
	m_textureMask = textureMask;

	if ( m_textureMask ) 
	{
		m_textureMask->AddRef();
	}
}

void IRenderGameplayRenderTarget::SetBackgroundColor( const Vector& backgroundColor )
{
	m_copyMode = CopyMode::BackgroundColor;

	m_backgroundColor = backgroundColor;
}