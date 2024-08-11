/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiImage.h"
#include "../core/depot.h"
#include "../core/gatheredResource.h"
#include "bitmapTexture.h"

namespace RedGui
{
	CRedGuiImage::CRedGuiImage( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl( x, y, width, height )
		, m_imageInstance( nullptr )
		, m_pathToFile( TXT("") )
	{
		SetBackgroundColor( Color::WHITE );
		SetNeedKeyFocus( false );
		SetBorderVisible( false );
	}

	CRedGuiImage::~CRedGuiImage()
	{
		/* intentionally empty */
	}

	void CRedGuiImage::Draw()
	{
		GetTheme()->DrawImage( this );
	}

	void CRedGuiImage::SetImage( const String& pathToFile )
	{
		m_pathToFile = pathToFile;
		LoadImage( m_pathToFile );
	}

	void CRedGuiImage::SetImage( THandle< CBitmapTexture > texture )
	{
		m_imageInstance = texture;

		if( m_imageInstance.Get() != nullptr )
		{
			SetSize( m_imageInstance.Get()->GetWidth(), m_imageInstance.Get()->GetHeight() );
			SetOriginalRect( Box2( GetPosition(), GetSize() ) );
		}
	}

	void CRedGuiImage::SetImage( CGatheredResource& resource )
	{
		THandle< CBitmapTexture > image = resource.LoadAndGet< CBitmapTexture >();
		SetImage( image );
	}

	void CRedGuiImage::LoadImage( const String& path )
	{
		THandle< CResource > res = GDepot->LoadResource( path );
		if ( res )
		{
			m_imageInstance = Cast< CBitmapTexture >( res );

			if( m_imageInstance.Get() != nullptr )
			{
				SetSize( m_imageInstance.Get()->GetWidth(), m_imageInstance.Get()->GetHeight() );
				SetOriginalRect( Box2( GetPosition(), GetSize() ) );
			}
		}
	}

	const String& CRedGuiImage::GetImagePath() const
	{
		return m_pathToFile;
	}

	THandle< CBitmapTexture > CRedGuiImage::GetImage()
	{
		if ( !m_imageInstance )
		{
			LoadImage( m_pathToFile );
		}

		return m_imageInstance;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
