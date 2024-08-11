/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include <ft2build.h>
#include "../../common/core/importer.h"
#include "../../common/core/dataBuffer.h"
#include "../../common/engine/fonts.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment ( lib, "..\\..\\..\\external\\freetype\\objs\\win32\\vc2010\\freetype2411MT_D_x64.lib" )	
	#else
		#pragma comment ( lib, "..\\..\\..\\external\\freetype\\objs\\win32\\vc2010\\freetype2411MT_x64.lib" )
	#endif
#else
	#ifdef _DEBUG
		#pragma comment ( lib, "..\\..\\..\\external\\freetype\\objs\\win32\\vc2010\\freetype2411MT_D.lib" )	
	#else
		#pragma comment ( lib, "..\\..\\..\\external\\freetype\\objs\\win32\\vc2010\\freetype2411MT.lib" )
	#endif
#endif

class CTTFFontImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CTTFFontImporter, IImporter, 0 );

	static const Uint32			MIN_FONT_TEXTURE_WIDTH = 1024;
	static const Uint32			MAX_FONT_TEXTURE_WIDTH = 2048;
	static const Uint32			MAX_FONT_TEXTURE_HEIGHT = 2048;

	FT_Library					m_library;
	FT_Face						m_face;	
	
	Uint32						m_faceIndex;
	Uint32						m_fontSize;
	Uint32						m_outline;
	Float						m_outlineAlphaMultiplier;

	TDynArray< Uint32 >			m_engineGlyphsToFTMapping;

	CFont::FactoryInfo			m_factoryInfo;

private:
	void RetrieveGlyphData();

	void RenderGlyphs();

	void RetrieveKerningData();

	Uint32 GetFontTextureHeight( Uint32 textureWidth,
							   Uint32 firstGlyph,
							   Uint32 &numGlyphsUsed );

	Uint32 FindBestFontTextureWidth();

	void ApplyOutline( Uint8* destData, Uint32 x, Uint32 y, Uint32 destPitch,
					   Uint8 *srcData, Uint32 width, Uint32 height, Uint32 srcPitch );

public:
	CTTFFontImporter();

	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CTTFFontImporter )
	PARENT_CLASS(IImporter)	
	PROPERTY_EDIT( m_fontSize, TXT("Font size" ) );
	PROPERTY_EDIT( m_outline, TXT("Size of font outline") );
	PROPERTY_EDIT( m_outlineAlphaMultiplier, TXT("Multiplier for outline opacity") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CTTFFontImporter);

CTTFFontImporter::CTTFFontImporter()
{
	m_resourceClass = ClassID< CFont >();
	m_formats.PushBack( CFileFormat( TXT("ttf"), TXT("TrueType font") ) );
	m_fontSize = 12;
	m_outline = 1;
	m_outlineAlphaMultiplier = 2.0f;
}

void CTTFFontImporter::RetrieveGlyphData()
{
	m_factoryInfo.m_fontInfo.m_maxGlyphHeight = 0;
	// 
	FT_ULong  charCode;
	FT_UInt   faceGlyphIndex;

	// retrieve all glyphs from font file
	charCode = FT_Get_First_Char( m_face, &faceGlyphIndex );
	while( faceGlyphIndex != 0 )
	{
		SFontGlyph currGlyph;

		// load glyph data

#if 0
		FT_Load_Char( m_face, charCode, FT_LOAD_RENDER );		

		currGlyph.m_width		= m_face->glyph->bitmap.width;
		currGlyph.m_height		= m_face->glyph->bitmap.rows;
		currGlyph.m_bearingX	= m_face->glyph->bitmap_left;
		currGlyph.m_bearingY	= m_face->glyph->bitmap_top;
		currGlyph.m_advanceX	= m_face->glyph->advance.x >> 6;

		ASSERT( ((*(m_face->glyph)).bitmap).width == ( ((*(m_face->glyph)).metrics).width >> 6 ) );
		ASSERT(	((*(m_face->glyph)).bitmap).rows == ( ((*(m_face->glyph)).metrics).height >> 6 ) );
		ASSERT( (*(m_face->glyph)).bitmap_left == ( ((*(m_face->glyph)).metrics).horiBearingX >> 6 ) );
		ASSERT( (*(m_face->glyph)).bitmap_top == ( ((*(m_face->glyph)).metrics).horiBearingY >> 6 ) );
#else
		FT_Load_Char( m_face, charCode, 0 );							// only grab metrics

		currGlyph.m_width		= ( m_face->glyph->metrics.width >> 6 ) + 2 * m_outline;
		currGlyph.m_height		= ( m_face->glyph->metrics.height >> 6 ) + 2 * m_outline;
		currGlyph.m_bearingX	= ( m_face->glyph->metrics.horiBearingX >> 6 ) - m_outline;
		currGlyph.m_bearingY	= ( m_face->glyph->metrics.horiBearingY >> 6 ) + m_outline;
		currGlyph.m_advanceX	= (m_face->glyph->advance.x >> 6) + ((Int32)m_outline);
#endif

		m_factoryInfo.m_fontInfo.m_maxGlyphHeight = max( m_factoryInfo.m_fontInfo.m_maxGlyphHeight, currGlyph.m_height );

		// set mapping data
		Uint32 newGlyphIndex = m_factoryInfo.m_fontInfo.m_glyphs.Size();
		m_engineGlyphsToFTMapping.PushBack( faceGlyphIndex );

		RED_ASSERT( newGlyphIndex <= MAXINT16, TXT("too many glyphs, imported font will be broken") );

		// add code->index mapping
		m_factoryInfo.m_fontInfo.m_unicodeMapping[ static_cast< int >( charCode ) ] = static_cast<Uint16>( newGlyphIndex );

		// add new glyph to list
		m_factoryInfo.m_fontInfo.m_glyphs.PushBack( currGlyph );

		// advance to the next character		
		charCode = FT_Get_Next_Char( m_face, charCode, &faceGlyphIndex );        
	}
}

Uint32 CTTFFontImporter::GetFontTextureHeight( Uint32 textureWidth,
											 Uint32 firstGlyph,
											 Uint32 &numGlyphsUsed )
{
	Uint32 glyphXPos = 0;
	Uint32 glyphYPos = 0;
	Uint32 maxGlyphHeight = 0;

	numGlyphsUsed = 0;

	for( Uint32 i=firstGlyph; i<m_factoryInfo.m_fontInfo.m_glyphs.Size(); ++i )
	{
		SFontGlyph &currGlyph = m_factoryInfo.m_fontInfo.m_glyphs[ i ];

		if ( glyphXPos + currGlyph.m_width > textureWidth )
		{
			glyphYPos += maxGlyphHeight + 1;
			glyphXPos = 0;
			maxGlyphHeight = 0;

			if ( glyphYPos + m_factoryInfo.m_fontInfo.m_maxGlyphHeight > MAX_FONT_TEXTURE_HEIGHT )
			{
				return glyphYPos;
			}
		}

		glyphXPos += currGlyph.m_width + 1;
		maxGlyphHeight = max( maxGlyphHeight, currGlyph.m_height );	

		++numGlyphsUsed;
	}

	return glyphYPos + maxGlyphHeight;
}

static Uint32 RoundToPow2( Uint32 val )
{
	Uint32 retVal = 1;
	while( retVal < val )
		retVal <<= 1;

	return retVal;
}

Uint32 CTTFFontImporter::FindBestFontTextureWidth()
{
	TDynArray< TPair< Uint32, Uint32> >		m_layouts;

	Uint32 currWidth = MIN_FONT_TEXTURE_WIDTH;
	while( currWidth <= MAX_FONT_TEXTURE_WIDTH )
	{
		Uint32 wasted = 0;

		Uint32 firstGlyph = 0;
		Uint32 numGlyphsUsed = 0;
		do 
		{
			Uint32 currHeight = GetFontTextureHeight( currWidth, firstGlyph, numGlyphsUsed );
			Uint32 currHeightRounded = RoundToPow2( currHeight );
			Uint32 currWasted = ( currHeightRounded - currHeight ) * currWidth;

			wasted += currWasted;

			firstGlyph += numGlyphsUsed;

		} while ( firstGlyph < m_factoryInfo.m_fontInfo.m_glyphs.Size() );

		m_layouts.PushBack( MakePair( currWidth, wasted ) );

		currWidth <<= 1;
	}

	Uint32 minIndex = 0;
	for( Uint32 i=1; i<m_layouts.Size(); ++i )
	{
		if ( m_layouts[i].m_second < m_layouts[minIndex].m_second )
			minIndex = i;
	}

	return m_layouts[ minIndex ].m_first;
}


void CopyRect( Uint8* destData, Uint32 x, Uint32 y, Uint32 destPitch,
			   Uint8 *srcData, Uint32 width, Uint32 height, Uint32 srcPitch )
{		
	for( Uint32 i=0; i<height; ++i )
	{
		Uint8 *lineSrcData = srcData + i * srcPitch;
		Uint8 *lineDestData = destData + destPitch * ( y + i ) + x;
		Red::System::MemoryCopy( lineDestData, lineSrcData, width );
	}
}

void CTTFFontImporter::ApplyOutline( Uint8* destData, Uint32 x, Uint32 y, Uint32 destPitch,
									 Uint8 *srcData, Uint32 width, Uint32 height, Uint32 srcPitch )
{
	if ( m_outline > 0 )
	{
		Uint32 outlineBufferWidth = width + m_outline * 2;
		Uint32 outlineBufferHeight = height + m_outline * 2;
		Uint8 *tempOutlineBuffer0 = new Uint8[ outlineBufferWidth * outlineBufferHeight ];
		Uint8 *tempOutlineBuffer1 = new Uint8[ outlineBufferWidth * outlineBufferHeight ];

		Red::System::MemoryZero( tempOutlineBuffer0, outlineBufferWidth * outlineBufferHeight );
		Red::System::MemoryZero( tempOutlineBuffer1, outlineBufferWidth * outlineBufferHeight );

		// copy font data to temp buffer
		for( Uint32 i=0; i<height; ++i )
		{
			Uint8 *lineSrcData = srcData + i * srcPitch;
			Uint8 *lineDestData = tempOutlineBuffer0 + ( outlineBufferWidth * ( i + m_outline ) + m_outline );
			for( Uint32 j=0; j<width; ++j )
			{
				if ( lineSrcData[j] > 128 )
					lineDestData[j] = 255;
				else
					lineDestData[j] = 0;
			}			
		}

		// do blurring		
		for( Uint32 iter=0; iter<m_outline; ++iter )
		{
			for( Int32 nY = 0; nY < (Int32)outlineBufferHeight; ++nY )
				for( Int32 nX = 0; nX < (Int32)outlineBufferWidth; ++nX )
				{
					Uint32 pixelCount = 9;
					Float pixelSum = 0;

					for( int i=-1; i<2; ++i )
						for( int j=-1; j<2; ++j )
						{
							if ( ( nY + i < 0 ) || ( nY + i >= (Int32)outlineBufferHeight ) ||
								 ( nX + j < 0 ) || ( nX + j >= (Int32)outlineBufferWidth ) )
								continue;

							pixelSum += (Float)tempOutlineBuffer0[ ( nY + i ) * outlineBufferWidth + ( nX + j ) ];
						}

					pixelSum *= m_outlineAlphaMultiplier;
					pixelSum /= pixelCount;
					if ( pixelSum > 255.0f ) pixelSum = 255.0f;
					tempOutlineBuffer1[ nY * outlineBufferWidth + nX ] = (Uint8)pixelSum;
				}

				Swap( tempOutlineBuffer0, tempOutlineBuffer1 );
		}

		// copy blurred data together with original one
		for( Uint32 i=0; i<height; ++i )
		{
			Uint8 *lineSrcData = srcData + i * srcPitch;
			Uint8 *lineDestData = destData + 2 * ( destPitch * ( y + i + m_outline ) + x + m_outline );
			for( Uint32 j=0; j<width; ++j )
			{
				lineDestData[ 2*j+0 ] = lineSrcData[j];
			}
		}

		for( Uint32 i=0; i<outlineBufferHeight; ++i )
		{			
			Uint8 *lineSrcData = tempOutlineBuffer0 + i * outlineBufferWidth;
			Uint8 *lineDestData = destData + 2 * ( destPitch * ( y + i ) + x );
			for( Uint32 j=0; j<outlineBufferWidth; ++j )
			{
				Uint8 fontAlpha = lineDestData[ 2*j+0 ];
				Uint8 outlineAlpha = lineSrcData[j];
				lineDestData[ 2*j+1 ] = Max( fontAlpha, outlineAlpha );
			}		
		}

		delete[] tempOutlineBuffer0;
		delete[] tempOutlineBuffer1;
	}
	else
	{
		for( Uint32 i=0; i<height; ++i )
		{
			Uint8 *lineSrcData = srcData + i * srcPitch;
			Uint8 *lineDestData = destData + 2 * ( destPitch * ( y + i ) + x );
			for( Uint32 j=0; j<width; ++j )
			{
				lineDestData[ 2*j+0 ] = lineSrcData[j];
				lineDestData[ 2*j+1 ] = lineSrcData[j];
			}		
		}
	}
}

void CTTFFontImporter::RenderGlyphs()
{
	Uint32 textureWidth = FindBestFontTextureWidth();

	m_factoryInfo.m_textures.Clear();
	
	Uint32 firstGlyph = 0;
	Uint32 numGlyphsUsed = 0;
	do 
	{
		Uint32 textureHeightUnrounded = GetFontTextureHeight( textureWidth, firstGlyph, numGlyphsUsed );
		Uint32 textureHeight = RoundToPow2( textureHeightUnrounded );

		m_factoryInfo.m_textures.PushBack( CBitmapTexture::MipMap() );
		CBitmapTexture::MipMap& currBuffer = m_factoryInfo.m_textures[ m_factoryInfo.m_textures.Size() - 1 ];

		currBuffer.m_width	= textureWidth;
		currBuffer.m_height	= textureHeight;
		currBuffer.m_pitch	= textureWidth * 2;
		currBuffer.m_data.Allocate( textureWidth * textureHeight * 2 );
		Red::System::MemoryZero( currBuffer.m_data.GetData(), textureWidth * textureHeight * 2 );

		// 
		Uint32 currGlyphXPos = 0;
		Uint32 currGlyphYPos = 0;
		Uint32 maxGlyphHeight = 0;

		for( Uint32 i=firstGlyph; i<firstGlyph + numGlyphsUsed; ++i )
		{
			Uint32 ftGlyphIndex = m_engineGlyphsToFTMapping[ i ];

			SFontGlyph &currGlyph = m_factoryInfo.m_fontInfo.m_glyphs[ i ];

			// render current glyph
			FT_Load_Glyph( m_face, ftGlyphIndex, FT_LOAD_RENDER );

			if ( currGlyphXPos + currGlyph.m_width > textureWidth )
			{
				currGlyphYPos += maxGlyphHeight + 1;
				currGlyphXPos = 0;
				maxGlyphHeight = 0;

				ASSERT( currGlyphYPos + m_factoryInfo.m_fontInfo.m_lineDist <= textureHeight );
			}

			ApplyOutline( (Uint8*)currBuffer.m_data.GetData(),
						  currGlyphXPos,	
						  currGlyphYPos,
						  currBuffer.m_width,
						  m_face->glyph->bitmap.buffer,
						  m_face->glyph->bitmap.width,
						  m_face->glyph->bitmap.rows,
						  m_face->glyph->bitmap.pitch );

			// 
			currGlyph.m_UVs[0][0]	= (Float)(currGlyphXPos)/ textureWidth;
			currGlyph.m_UVs[0][1]	= (Float)(currGlyphYPos)/ textureHeight;
			currGlyph.m_UVs[1][0]	= (Float)(currGlyphXPos + currGlyph.m_width) / textureWidth;
			currGlyph.m_UVs[1][1]	= (Float)(currGlyphYPos + currGlyph.m_height) / textureHeight;

			// set texture index
			currGlyph.m_textureIndex	= m_factoryInfo.m_textures.Size() - 1;

			// proceed to next glyph
			currGlyphXPos += currGlyph.m_width + 1;
			maxGlyphHeight = max( maxGlyphHeight, currGlyph.m_height );	
		}
		
		firstGlyph += numGlyphsUsed;

	} while ( firstGlyph < m_factoryInfo.m_fontInfo.m_glyphs.Size() );
}

void CTTFFontImporter::RetrieveKerningData()
{
	m_factoryInfo.m_fontInfo.m_kerning = false;
}

CResource* CTTFFontImporter::DoImport( const ImportOptions& options )
{
	FT_Error error;

	m_factoryInfo.m_fontInfo.m_glyphs.Clear();

	error = FT_Init_FreeType( &m_library );
	if ( error )
	{
		WARN_IMPORTER( TXT("Unable to initialize FreeType library !") );
		return NULL;
	}

	// select face
	error = FT_New_Face( m_library,
						 UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() ),
						 0, 
						 &m_face );
	if ( error )
	{
		WARN_IMPORTER( TXT("Unable to retrieve font face from %s file !"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	// set font size
	error = FT_Set_Pixel_Sizes( m_face, 
								0, 
								(FT_UInt)m_fontSize );
	if ( error )
	{
		WARN_IMPORTER( TXT("Unable to set desired font size for %s !"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	// select charmap
	error = FT_Select_Charmap( m_face, 
							   FT_ENCODING_UNICODE );
	if ( error )
	{
		WARN_IMPORTER( TXT("Unable to select unicode charmap for %s !"), options.m_sourceFilePath.AsChar() );
		return NULL;
	}

	for( Int32 i=0; i<65536; ++i )
		m_factoryInfo.m_fontInfo.m_unicodeMapping[ i ] = SFontInfo::GLYPH_DOESNT_EXIST;
	 
	m_factoryInfo.m_fontInfo.m_lineDist	= m_face->size->metrics.height >> 6;

	// mapping from engine glyph indices to FreeType indices 
	m_engineGlyphsToFTMapping.Clear();

	RetrieveGlyphData();

	RenderGlyphs();

	RetrieveKerningData();

	FT_Done_FreeType( m_library );
	m_face = NULL;

	//////////////////////////////////////////////////////////////////////////	
#if 0
	for( Uint32 i=0; i<m_factoryInfo.m_textures.Size(); ++i )
	{
		char buf[256];
		Red::System::SNPrintF( buf, ARRAY_COUNT( buf ), "d:\\texFile%02d_%d_%d.raw", i, m_factoryInfo.m_textures[i], m_factoryInfo.m_textures[i].m_height );
		FILE *f = fopen( buf, "wb" );
		for( Uint32 j=0; j<m_factoryInfo.m_textures[i].m_height; ++j )
			for( Uint32 k=0; k<m_factoryInfo.m_textures[i].m_width; ++k )
			{
				fwrite( (Uint8*)m_factoryInfo.m_textures[i].m_data.GetData() + 2 * m_factoryInfo.m_textures[i].m_width * j + 2 * k , 
						1, 
						2, 
						f );

				Uint8 temp = 0;
				fwrite( &temp, 
						1, 
						1, 
						f );
			}		
		fclose( f );
	}
#endif
	
	CFont* retVal;
	
	if (options.m_existingResource != NULL)
	{
		retVal = Cast< CFont >( options.m_existingResource );
		if ( !retVal->MarkModified() )
		{
			return retVal;
		}
	}
	else
	{
		retVal = m_factoryInfo.CreateResource();
	}

	retVal->Init( m_factoryInfo );

	return retVal;
}
