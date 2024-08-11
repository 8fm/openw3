/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "textureGroup.h"

IMPLEMENT_ENGINE_CLASS( TextureGroup );

GpuApi::EImageCompressionHint GetImageCompressionHint( ETextureCompression compression )
{
	if ( TCM_Normals == compression || TCM_NormalsGloss == compression )
	{
		return GpuApi::CIH_NormalmapRGB;
	}

	return GpuApi::CIH_None;
}

TextureGroup::TextureGroup()
	: m_groupName( RED_NAME( Default ) )
	, m_isStreamable( true )
	, m_isUser( true )
	, m_isResizable( true )
	, m_isDetailMap( false )
	, m_isAtlas( false )
	, m_compression( TCM_None )
	, m_maxSize( 4096 )
	, m_hasMipchain( true )
	, m_highPriority( false )
	, m_category( eTextureCategory_Generic )
{}

TextureGroup::~TextureGroup()
{
}
