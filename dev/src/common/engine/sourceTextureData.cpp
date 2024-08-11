/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../core/feedback.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CSourceTexture );

namespace SourceTextureHelpers
{
	void ConvertBufferToFloat( const void* sourceBuffer, ETextureRawFormat format, Float* targetBuffer, Uint32 pixels )
	{
		if ( format == TRF_HDR || format == TRF_HDRGrayscale )
		{
			// Floats, no need to convert
			Red::System::MemoryCopy( targetBuffer, sourceBuffer, pixels * CBitmapTexture::GetPixelSize( format ) / 8);
			return;
		}

		Uint32 pixelSize = CBitmapTexture::GetPixelSize( format ) / 8;

		for ( Uint32 i = 0; i < pixelSize * pixels; ++i )
		{
			targetBuffer[i] = ((const Uint8*)sourceBuffer)[i] / 255.0f;
		}
	}

	void ConvertBufferFromFloat( void* targetBuffer, Float* sourceBuffer, ETextureRawFormat format, Uint32 pixels )
	{
		if ( format == TRF_HDR || format == TRF_HDRGrayscale )
		{
			// Floats, no need to convert
			Red::System::MemoryCopy( targetBuffer, sourceBuffer, pixels * CBitmapTexture::GetPixelSize( format ) / 8);
			return;
		}

		Uint32 pixelSize = CBitmapTexture::GetPixelSize( format ) / 8;

		for ( Uint32 i = 0; i < pixelSize * pixels; ++i )
		{
			((Uint8*)targetBuffer)[i] = (Uint8)Clamp( sourceBuffer[i] * 255.0f, 0.0f, 255.0f );
		}
	}

	void FillLargerBuffer( Uint8* targetBuffer, Uint8* sourceBuffer, Uint32 sourceWidth, Uint32 sourceHeight, Uint32 targetWidth, Uint32 targetHeight, Uint32 pixelSize )
	{
		ASSERT( targetWidth <= targetWidth );
		ASSERT( sourceHeight <= targetHeight );

		// Very small loops
		for ( Uint32 y = 0; y < targetHeight; ++y )
		{
			Uint32 sourceY = (y < sourceHeight) ? y : (sourceHeight - 1);

			for ( Uint32 x = 0; x < targetWidth; ++x )
			{
				Uint32 sourceX = (x < sourceWidth) ? x : (sourceWidth - 1);

				Uint32 sourcePointer = ( sourceX + sourceY * sourceWidth ) * pixelSize;
				Uint32 targetPointer = ( x + y * targetWidth ) * pixelSize;

				Red::System::MemoryCopy( &(targetBuffer[targetPointer]), &(sourceBuffer[sourcePointer]), pixelSize );
			}
		}

	}

	void DownsampleBox2( Float* sourceData, Float* outData, Uint32 numChannels, Uint32 oldWidth, Uint32 oldHeight, Uint32 width, Uint32 height )
	{
		TDynArray<Float> tempBuff( oldWidth * height * numChannels );

		// First pass, resample in Y direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			Uint32 sourceY1 = y * 2;
			Uint32 sourceY2 = (sourceY1 < (oldHeight - 1)) ? (sourceY1 + 1) : sourceY1;

			for ( Uint32 x = 0; x < oldWidth; ++x )
			{
				Uint32 sourceDataPointer1 = (Uint32)(x + sourceY1 * oldWidth)*numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(x + sourceY2 * oldWidth)*numChannels;

				Uint32 destDataPointer = (x + y * oldWidth) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Float a1 = sourceData[ sourceDataPointer1++ ];
					Float a2 = sourceData[ sourceDataPointer2++ ];

					Float dest = (a1 + a2) * 0.5f;
					tempBuff.TypedData()[destDataPointer++] = dest;
				}
			}
		}

		// Second pass, resample in X direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			for ( Uint32 x = 0; x < width; ++x )
			{
				Uint32 sourceX1 = x * 2;
				Uint32 sourceX2 = (sourceX1 < (oldWidth - 1)) ? (sourceX1 + 1) : sourceX1;

				Uint32 sourceDataPointer1 = (Uint32)(sourceX1 + y * oldWidth)*numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(sourceX2 + y * oldWidth)*numChannels;

				Uint32 destDataPointer = (x + y * width) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Float a1 = tempBuff[ sourceDataPointer1++ ];
					Float a2 = tempBuff[ sourceDataPointer2++ ];

					Float dest = (a1 + a2) * 0.5f;
					outData[destDataPointer++] = dest;
				}
			}
		}
	}

	void DownsampleBox2( Uint8* sourceData, Uint8* outData, Uint32 numChannels, Uint32 oldWidth, Uint32 oldHeight, Uint32 width, Uint32 height, const TDynArray<Float>* residuesIn, TDynArray<Float>* residuesOut )
	{
		TDynArray<Uint8> tempBuff( oldWidth * height * numChannels );

		TDynArray<Float> tempResidues( oldWidth * height * numChannels );

		// First pass, resample in Y direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			Uint32 sourceY1 = y * 2;
			Uint32 sourceY2 = (sourceY1 < (oldHeight - 1)) ? (sourceY1 + 1) : sourceY1;

			for ( Uint32 x = 0; x < oldWidth; ++x )
			{
				Uint32 sourceDataPointer1 = (Uint32)(x + sourceY1 * oldWidth)*numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(x + sourceY2 * oldWidth)*numChannels;

				Uint32 destDataPointer = (x + y * oldWidth) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Uint8 a1 = sourceData[ sourceDataPointer1++ ];
					Uint8 a2 = sourceData[ sourceDataPointer2++ ];

					Uint8 dest = 0;

					if ( residuesOut )
					{
						Float a1f = 0.f;
						Float a2f = 0.f;
						if (residuesIn)
						{
							a1f = (a1 + residuesIn->TypedData()[sourceDataPointer1 - 1])/255.0f;
							a2f = (a2 + residuesIn->TypedData()[sourceDataPointer2 - 1])/255.0f;
						}
						else
						{
							a1f = a1/255.0f;
							a2f = a2/255.0f;
						}
						Float average = (a1f + a2f) * 0.5f;
						Float floatdest = (Uint8)Clamp( average * 255.0f, 0.0f, 255.0f );
						dest = (Uint8)floatdest;
						tempResidues.TypedData()[destDataPointer] = Red::Math::MFract( floatdest );
					}
					else
					{
						dest = (a1 + a2) / 2;
					}

					tempBuff.TypedData()[destDataPointer++] = dest;
				}
			}
		}

		// Second pass, resample in X direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			for ( Uint32 x = 0; x < width; ++x )
			{
				Uint32 sourceX1 = x * 2;
				Uint32 sourceX2 = (sourceX1 < (oldWidth - 1)) ? (sourceX1 + 1) : sourceX1;

				Uint32 sourceDataPointer1 = (Uint32)(sourceX1 + y * oldWidth)*numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(sourceX2 + y * oldWidth)*numChannels;

				Uint32 destDataPointer = (x + y * width) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Uint8 a1 = tempBuff[ sourceDataPointer1++ ];
					Uint8 a2 = tempBuff[ sourceDataPointer2++ ];

					Uint8 dest = 0;

					if ( residuesOut )
					{
						Float a1f = 0.f;
						Float a2f = 0.f;
						if (residuesIn)
						{
							a1f = (a1 + tempResidues.TypedData()[sourceDataPointer1 - 1])/255.0f;
							a2f = (a2 + tempResidues.TypedData()[sourceDataPointer2 - 1])/255.0f;
						}
						else
						{
							a1f = a1/255.0f;
							a2f = a2/255.0f;
						}
						Float average = (a1f + a2f) * 0.5f;
						Float floatdest = (Uint8)Clamp( average * 255.0f, 0.0f, 255.0f );
						dest = (Uint8)floatdest;
						residuesOut->TypedData()[destDataPointer] = Red::Math::MFract( floatdest );
					}
					else
					{
						dest = (a1 + a2) / 2;
					}

					outData[destDataPointer++] = dest;
				}
			}
		}
	}

	Float CubicInterpolate (Float p0, Float p1, Float p2, Float p3, Float x) 
	{
		return p1 + 0.5f * x*(p2 - p0 + x*(2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3 + x*(3.0f*(p1 - p2) + p3 - p0)));
	}

	void FlipCubeFace( Uint32 dimension, Uint32 pitch, Float *dataRGBA, Bool faceFlipX, Bool faceFlipY, Bool faceRotate )
	{
		if ( !faceFlipX && !faceFlipY && !faceRotate )
		{
			return;
		}

		// Build transformed image
		TDynArray< Float > tempBuffer ( pitch * dimension );
		for ( Uint32 y=0; y<dimension; y++ )
		{
			for ( Uint32 x=0; x<dimension; x++ )
			{
				// Rotate
				Uint32 destX = faceRotate ? y : x;
				Uint32 destY = faceRotate ? x : y;

				// Flip
				destX = faceFlipX ? ( (dimension-1) - destX ) : destX;
				destY = faceFlipY ? ( (dimension-1) - destY ) : destY;

				// Copy
				const Float* readPtr = &(dataRGBA[ x * 4 + y * pitch ]);
				Float* writePtr = &(tempBuffer[ destX * 4 + destY * pitch ]);
				writePtr[0] = readPtr[0];
				writePtr[1] = readPtr[1];
				writePtr[2] = readPtr[2];
				writePtr[3] = readPtr[3];
			}
		}

		// Copy our image back
		Red::System::MemoryCopy( dataRGBA, tempBuffer.Data(), tempBuffer.DataSize() );
	}

	void FillAlphaWithOne( Float* data, Uint32 numPixels )
	{
		for ( Uint32 i = 0; i < numPixels; ++i )
		{
			data[ 3 + i * 4] = 1.0f;
		}
	}

	void PropagateGrayscaleToThreeChannells( Float* destData, Float* sourceData, Uint32 numPixels )
	{
		for ( Uint32 i = 0; i < numPixels; ++i )
		{
			destData[ 4*i + 0 ] = sourceData[i];
			destData[ 4*i + 1 ] = sourceData[i];
			destData[ 4*i + 2 ] = sourceData[i];
			destData[ 4*i + 3 ] = 1.0f;
		}
	}

	void ResampleBicubic( Float* sourceData, Float* outData, Uint32 numChannels, Uint32 oldWidth, Uint32 oldHeight, Uint32 width, Uint32 height )
	{
		Float ratioX = (Float)oldWidth / (Float)width;
		Float ratioY = (Float)oldHeight / (Float)height;

		static TDynArray<Float> tempBuff;
		tempBuff.ClearFast();
		tempBuff.ResizeFast(oldWidth * height * numChannels );

		// First pass, resample in Y direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			Float sourceY = (Float)y * ratioY;

			Uint32 sampleY2 = (Uint32)floorf( sourceY );
			Uint32 sampleY1 = (sampleY2 > 0) ? (sampleY2 - 1) : 0;
			Uint32 sampleY3 = (sampleY2 < (oldHeight - 1)) ? (sampleY2 + 1) : oldHeight - 1;
			Uint32 sampleY4 = (sampleY2 < (oldHeight - 2)) ? (sampleY2 + 2) : oldHeight - 1;

			Float interpolation = sourceY - floorf( sourceY );

			for ( Uint32 x = 0; x < oldWidth; ++x )
			{
				Uint32 sourceDataPointer1 = (Uint32)(x + sampleY1 * oldWidth)*numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(x + sampleY2 * oldWidth)*numChannels;
				Uint32 sourceDataPointer3 = (Uint32)(x + sampleY3 * oldWidth)*numChannels;
				Uint32 sourceDataPointer4 = (Uint32)(x + sampleY4 * oldWidth)*numChannels;

				Uint32 destDataPointer = (x + y * oldWidth) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Float a1 = sourceData[ sourceDataPointer1++ ];
					Float a2 = sourceData[ sourceDataPointer2++ ];
					Float a3 = sourceData[ sourceDataPointer3++ ];
					Float a4 = sourceData[ sourceDataPointer4++ ];

					Float dest = CubicInterpolate( a1, a2, a3, a4, interpolation );
					tempBuff.TypedData()[destDataPointer++] = dest;
				}
			}
		}

		// Second pass, resample in X direction
		for ( Uint32 y = 0; y < height; ++y )
		{
			for ( Uint32 x = 0; x < width; ++x )
			{
				Float sourceX = (Float)x * ratioX;

				Uint32 sampleX2 = (Int32)floorf( sourceX );
				Uint32 sampleX1 = (sampleX2 > 0) ? (sampleX2 - 1) : 0;
				Uint32 sampleX3 = (sampleX2 < (oldWidth - 1)) ? (sampleX2 + 1) : oldWidth - 1;
				Uint32 sampleX4 = (sampleX2 < (oldWidth - 2)) ? (sampleX2 + 2) : oldWidth - 1;

				Float interpolation = sourceX - floorf( sourceX );

				Uint32 sourceDataPointer1 = (Uint32)(sampleX1 + y * oldWidth) * numChannels;
				Uint32 sourceDataPointer2 = (Uint32)(sampleX2 + y * oldWidth) * numChannels;
				Uint32 sourceDataPointer3 = (Uint32)(sampleX3 + y * oldWidth) * numChannels;
				Uint32 sourceDataPointer4 = (Uint32)(sampleX4 + y * oldWidth) * numChannels;

				Uint32 destDataPointer = (x + y * width) * numChannels;

				for ( Uint32 i =0; i < numChannels; ++i )
				{
					Float a1 = tempBuff[ sourceDataPointer1++ ];
					Float a2 = tempBuff[ sourceDataPointer2++ ];
					Float a3 = tempBuff[ sourceDataPointer3++ ];
					Float a4 = tempBuff[ sourceDataPointer4++ ];

					Float dest = CubicInterpolate( a1, a2, a3, a4, interpolation );
					outData[destDataPointer++] = dest;
				}
			}
		}
	}


};

CSourceTexture::CSourceTexture()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( TRF_TrueColor )
{
}

CSourceTexture::~CSourceTexture()
{

}

void CSourceTexture::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// No crap is done on mapping or on GC
	if ( file.IsMapper() || file.IsGarbageCollector() )
	{
		return;
	}

	if ( file.IsReader() )
	{
		if ( m_pitch == 0 )
		{
			m_pitch = m_width * CBitmapTexture::GetPixelSize( m_format) / 8;
		}
	}

#ifndef NO_EDITOR
	// Do not serialize during cook
	if( !file.IsCooker() )
	{
		m_dataBuffer.Serialize( file, true, false );
	}
#endif

}

void CSourceTexture::CopyBufferPitched( void* dest, Uint32 destPitch, const void* src, Uint32 srcPitch, Uint32 rowDataSize, Uint32 numRows )
{
	const Uint8* srcPtr = ( const Uint8* ) src;
	Uint8* destPtr = ( Uint8* ) dest;
	for ( Uint32 i=0; i<numRows; i++, srcPtr += srcPitch, destPtr += destPitch )
	{
		Red::System::MemoryCopy( destPtr, srcPtr, rowDataSize );
	}
}

Bool CSourceTexture::CreateFromMip( const CBitmapTexture::MipMap& mip )
{
#ifndef NO_TEXTURE_EDITING
	//ASSERT( mip.m_height <= m_height );
	//ASSERT( mip.m_width <= m_width );

	Uint32 sourceWidth = mip.m_width * CBitmapTexture::GetPixelSize( m_format) / 8;
	CopyBufferPitched( m_dataBuffer.GetData(), m_pitch, mip.m_data.GetData(), mip.m_pitch, Min<Uint32>( m_pitch, sourceWidth ), Min<Uint32>( m_height, mip.m_height ) );

	return true;

#else
	return false;
#endif
}

Bool CSourceTexture::CreateFromRawData( void* buf, Uint32 origWidth, Uint32 origHeight, Uint32 origPitch )
{
#ifndef NO_TEXTURE_EDITING
	ASSERT( origHeight <= m_height );
	ASSERT( origWidth <= m_width );

	Uint32 sourceWidth = origWidth * CBitmapTexture::GetPixelSize( m_format ) / 8;
	CopyBufferPitched( m_dataBuffer.GetData(), m_pitch, buf, origPitch, Min<Uint32>( m_pitch, sourceWidth ), Min<Uint32>( m_height, origHeight ) );

	return true;

#else
	return false;
#endif
}

Bool CSourceTexture::CreateFromDataCompressed( const void* data, size_t dataSize, ETextureCompression textureCompression )
{
#ifndef NO_TEXTURE_EDITING
	GpuApi::eTextureFormat sourceFormat, targetFormat;
	CBitmapTexture::GetCompressedFormat( m_format, textureCompression, sourceFormat, false );
	CBitmapTexture::GetCompressedFormat( m_format, TCM_None, targetFormat);

	Uint32 sourcePitch = GpuApi::CalculateTexturePitch( m_width, sourceFormat );
	CBitmapTexture::ConvertBuffer( m_width, m_height, sourcePitch, sourceFormat, data, dataSize, targetFormat, m_dataBuffer.GetData(), GpuApi::CIH_None );
	CBitmapTexture::SwizzlingStep( (Uint8*)m_dataBuffer.GetData(), m_width, m_height, m_format, textureCompression, m_format, TCM_None );

	return true;
#else
	return false;
#endif
}

Bool CSourceTexture::CreateFromMipCompressed( const CBitmapTexture::MipMap& mip, ETextureCompression textureCompression )
{
#ifndef NO_TEXTURE_EDITING
	if ( !const_cast<CBitmapTexture::MipMap&>( mip ).m_data.Load() )
	{
		return false;
	}

	Bool result = CreateFromDataCompressed( mip.m_data.GetData(), mip.m_data.GetSize(), textureCompression );

	const_cast<CBitmapTexture::MipMap&>( mip ).m_data.Unload();

	return result;
#else
	return false;
#endif
}

// Fill uncompressed mip with data
void CSourceTexture::FillMip( CBitmapTexture::MipMap& mip )
{
#ifndef NO_TEXTURE_EDITING
	m_dataBuffer.Load();
	Uint32 sourceWidth = m_width * CBitmapTexture::GetPixelSize( m_format ) / 8;
	CopyBufferPitched( mip.m_data.GetData(), mip.m_pitch, m_dataBuffer.GetData(), m_pitch, Min<Uint32>( mip.m_pitch, sourceWidth ), Min<Uint32>( m_height, mip.m_height ) );
	m_dataBuffer.Unload();
#endif
}

// Fill raw, truecolor noncompressed buffer
void CSourceTexture::FillBufferTrueColor( Uint8* buf, Uint32 width, Uint32 height )
{
#ifndef NO_TEXTURE_EDITING
	m_dataBuffer.Load();

	if ( width == m_width && height == m_height )
	{
		Uint32 sourceWidth = m_width * CBitmapTexture::GetPixelSize( m_format ) / 8;
		CopyBufferPitched( buf, sourceWidth, m_dataBuffer.GetData(), m_pitch, sourceWidth, m_height );
	}
	else
	{
		static TDynArray< Float > tempSourceData;
		tempSourceData.ClearFast();
		static TDynArray< Float > tempTargetData;
		tempTargetData.ClearFast();

		Uint32 numChannels = CBitmapTexture::GetNumChannels( m_format );

		tempSourceData.ResizeFast( m_width * m_height * numChannels );
		tempTargetData.ResizeFast( width * height * numChannels );

		SourceTextureHelpers::ConvertBufferToFloat( m_dataBuffer.GetData(), m_format, tempSourceData.TypedData(), m_width * m_height );

		SourceTextureHelpers::ResampleBicubic( tempSourceData.TypedData(), tempTargetData.TypedData(), numChannels, m_width, m_height, width, height );

		SourceTextureHelpers::ConvertBufferFromFloat( buf, tempTargetData.TypedData(), m_format, width * height );
	}

	m_dataBuffer.Unload();
#endif
}

void CSourceTexture::FillBufferHDR( Float* buf, Uint32 width, Uint32 height )
{
	m_dataBuffer.Load();

	if ( width == m_width && height == m_height )
	{
		Uint32 sourceWidth = m_width * CBitmapTexture::GetPixelSize( m_format ) / 8;
		CopyBufferPitched( buf, sourceWidth, m_dataBuffer.GetData(), m_pitch, sourceWidth, m_height );
	}
	else
	{
		ASSERT ( !"not supported" );
	}

	m_dataBuffer.Unload();
}

void CSourceTexture::Init( Uint32 width, Uint32 height, ETextureRawFormat format )
{
	m_width = width;
	m_height = height;
	m_format = format;
	m_pitch = m_width * CBitmapTexture::GetPixelSize( m_format) / 8;
	m_dataBuffer.Allocate( m_pitch * m_height );
}

void CSourceTexture::CreateFromSourceBicubicResample( const CSourceTexture* other )
{
#ifndef NO_TEXTURE_EDITING
	ASSERT( m_format == other->m_format );

	static TDynArray< Float > tempSourceData;
	tempSourceData.ClearFast();
	static TDynArray< Float > tempTargetData;
	tempTargetData.ClearFast();

	Uint32 numChannels = CBitmapTexture::GetNumChannels( m_format);

	tempSourceData.ResizeFast( other->m_width * other->m_height * numChannels );
	tempTargetData.ResizeFast( m_width * m_height * numChannels );

	const_cast<CSourceTexture*>(other)->m_dataBuffer.Load();
	SourceTextureHelpers::ConvertBufferToFloat( other->m_dataBuffer.GetData(), m_format, tempSourceData.TypedData(), other->m_width * other->m_height );
	const_cast<CSourceTexture*>(other)->m_dataBuffer.Unload();

	SourceTextureHelpers::ResampleBicubic( tempSourceData.TypedData(), tempTargetData.TypedData(), numChannels, other->m_width, other->m_height, m_width, m_height );

	SourceTextureHelpers::ConvertBufferFromFloat( m_dataBuffer.GetData(), tempTargetData.TypedData(), m_format, m_width * m_height );
#endif
}

void CSourceTexture::CreateMips( CBitmapTexture::MipArray& outMips, Uint32 width, Uint32 height, ETextureCompression textureCompression, Bool allMips, Bool flipX, Bool flipY, Bool rotate, Bool silent )
{
#ifndef NO_EDITOR
	if ( !silent )
	{
		GFeedback->BeginTask( TXT("Compressing bitmap, this may take a while."), false );
	}
#endif

#ifndef NO_TEXTURE_EDITING
	ASSERT( (!flipX && !flipY && !rotate) || (width == height) );
	ASSERT( (!flipX && !flipY && !rotate) || (m_format == TRF_TrueColor) );

	Uint32 numChannels = CBitmapTexture::GetNumChannels( m_format );
	ETextureRawFormat format = m_format;

	m_dataBuffer.Load();

	Uint32 colorDataSize = m_dataBuffer.GetSize();
	Uint8* colorData = static_cast<Uint8*>(RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, colorDataSize ));

	if(  ( (m_format == TRF_Grayscale) && ( textureCompression == TCM_DXTNoAlpha ) ) || ( width != m_width || height != m_height ) || ( flipX || flipY || rotate ) )
	{
		static TDynArray< Float > tempSourceData;
		tempSourceData.ClearFast();
		tempSourceData.ResizeFast( m_width * m_height * numChannels );

		if ( tempSourceData.TypedData() == nullptr )
		{
#ifndef NO_EDITOR
			GFeedback->EndTask();
#endif
			RED_HALT( "Can't allocate a continous buffer big enough for the texture" );
			return;
		}

		SourceTextureHelpers::ConvertBufferToFloat( m_dataBuffer.GetData(), m_format, tempSourceData.TypedData(), m_width * m_height );

		m_dataBuffer.Unload();
	
		if ( (m_format == TRF_Grayscale) && ( textureCompression == TCM_DXTNoAlpha ) )
		{
			format = TRF_TrueColor;
			numChannels = 4;

			static TDynArray<Float> tempData;
			tempData.ClearFast();
			tempData.ResizeFast( m_width * m_height * numChannels );

			SourceTextureHelpers::PropagateGrayscaleToThreeChannells( tempData.TypedData(), tempSourceData.TypedData(), m_width * m_height );

			tempSourceData = tempData;
		}


		if ( width != m_width || height != m_height )
		{
			Uint32 tempWidth = m_width;
			Uint32 tempHeight = m_height;

			while ( (tempWidth != width) && (tempHeight != height) )
			{
				Uint32 newTempWidth = Max( width, tempWidth / 2 );
				Uint32 newTempHeight = Max( height, tempHeight / 2 );

				static TDynArray<Float> tempData;
				tempData.ClearFast();
				tempData.ResizeFast( newTempWidth * newTempHeight * numChannels );

				SourceTextureHelpers::ResampleBicubic( tempSourceData.TypedData(), tempData.TypedData(), numChannels, tempWidth, tempHeight, newTempWidth, newTempHeight );
				tempSourceData.CopyFast( tempData );

				tempWidth = newTempWidth;
				tempHeight = newTempHeight;
			}
		}

		if ( flipX || flipY || rotate )
		{
			SourceTextureHelpers::FlipCubeFace( width, width * numChannels, tempSourceData.TypedData(), flipX, flipY, rotate );
		}

		static TDynArray< Uint8 > tempBuffer;
		tempBuffer.ClearFast();
		tempBuffer.ResizeFast( width * height * CBitmapTexture::GetPixelSize( format ) / 8 );

		SourceTextureHelpers::ConvertBufferFromFloat( tempBuffer.TypedData(), tempSourceData.TypedData(), format, width * height );

		Red::System::MemoryCopy( colorData, tempBuffer.TypedData(), colorDataSize );
	}
	else
	{
		Red::System::MemoryCopy( colorData, m_dataBuffer.GetData(), colorDataSize );
		m_dataBuffer.Unload();
	}

	// naive, yet harmless method of computing desired number of mipmaps to create
	Uint32 mipMapsToCreate = 0;
	if ( !allMips )
	{
		mipMapsToCreate = 1;
	}
	else
	{
		Uint32 tmpWidth = width;
		Uint32 tmpHeight = height;
		while ( tmpWidth != 1 && tmpHeight != 1 )
		{
			tmpWidth = tmpWidth / 2;
			tmpHeight = tmpHeight / 2;
			++mipMapsToCreate;
		}
	}
	Uint32 currentMipNum = 1;

	// all operations were successful, we can clear the previous mips
	outMips.Clear();

	static TDynArray< Float > residues;
	Bool useResidues = false;

	Bool done = false;
	while ( !done )
	{
		CBitmapTexture::MipMap mip;

		GpuApi::eTextureFormat sourceFormat, targetFormat;
		CBitmapTexture::GetCompressedFormat( format, TCM_None, sourceFormat );
		CBitmapTexture::GetCompressedFormat( format, textureCompression, targetFormat, false );

		// swizzling seemed to fuck up normalmaps when importing
		CBitmapTexture::SwizzlingStep( colorData, width, height, format, TCM_None, format, textureCompression );

#ifndef NO_EDITOR
		if ( !silent )
		{
			GFeedback->UpdateTaskProgress( currentMipNum++, mipMapsToCreate );
		}
#endif

		if ( CBitmapTexture::CreateMip( mip, width, height, format, textureCompression ) )
		{
			if ( !CBitmapTexture::ConvertBuffer( width, height, width * CBitmapTexture::GetPixelSize( format ) / 8,
				sourceFormat, 
				colorData, 
				colorDataSize,
				targetFormat, 
				mip.m_data.GetData(),
				GetImageCompressionHint( textureCompression )
				) )
			{
				// No compression, direct copy

				Red::System::MemoryCopy( mip.m_data.GetData(), colorData, width * height * CBitmapTexture::GetPixelSize( format ) / 8 );
			}

			outMips.PushBack( mip );
		}
		else if ( CBitmapTexture::CreateMip( mip, Max<Uint32>( width, 4 ), Max<Uint32>( height, 4 ), format, textureCompression ) )
		{
			// Dxt shit and width & height < 4 

			Uint32 compressedWidth = Max<Uint32>( width, 4 );
			Uint32 compressedHeight = Max<Uint32>( height, 4 );

			static TDynArray< Uint8 > tempBufferBlockCompressed;
			tempBufferBlockCompressed.ClearFast();
			tempBufferBlockCompressed.ResizeFast( compressedWidth * compressedHeight * CBitmapTexture::GetPixelSize( format) / 8 );

			SourceTextureHelpers::FillLargerBuffer( tempBufferBlockCompressed.TypedData(), colorData, width, height, compressedWidth, compressedHeight, CBitmapTexture::GetPixelSize( format) / 8 );

			CBitmapTexture::ConvertBuffer( compressedWidth, compressedHeight, compressedWidth * CBitmapTexture::GetPixelSize( format ) / 8, 
				sourceFormat, 
				tempBufferBlockCompressed.TypedData(), 
				tempBufferBlockCompressed.DataSize(),
				targetFormat, 
				mip.m_data.GetData(),
				GetImageCompressionHint( textureCompression ) );

			outMips.PushBack( mip );
		}

		if ( !allMips || ( width == 1 && height == 1 ) )
		{
			done = true;
			break;
		}

		Uint32 oldWidth = width;
		Uint32 oldHeight = height;

		if ( width > 1 )
		{
			width = width / 2;
		}

		if ( height > 1 )
		{
			height = height / 2;
		}

		//downsample to have the data to start with in the next round

		static TDynArray< Uint8 > tempData;
		tempData.ClearFast();
		tempData.ResizeFast( width * height * numChannels );

		static TDynArray< Float > newResidues;
		newResidues.ClearFast();
		newResidues.ResizeFast( width * height * numChannels );

		SourceTextureHelpers::DownsampleBox2( colorData, tempData.TypedData(), numChannels, oldWidth, oldHeight, width, height, useResidues?&residues:nullptr, &newResidues );

		residues.ClearFast();
		residues.PushBack(newResidues);
		useResidues = true;


		colorDataSize = tempData.Size();
		colorData = static_cast<Uint8*>( RED_MEMORY_REALLOCATE( MemoryPool_Default, colorData, MC_Editor, colorDataSize ) );
		Red::System::MemoryCopy( colorData, tempData.TypedData(), colorDataSize );
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, colorData );

#endif // NO_TEXTURE_EDITING

#ifndef NO_EDITOR
	if ( !silent )
	{
		GFeedback->EndTask();
	}
#endif
}

void* CSourceTexture::GetBufferAccessPointer()
{
	m_dataBuffer.Load();
	return m_dataBuffer.GetData();
}
