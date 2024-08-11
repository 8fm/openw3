/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../gpuApiUtils/gpuApiDDSLoader.h"
#include "../redMath/redmathbase.h"

#define	GPUAPI_BLANK2D_TEXTURE_SIZE				4
#define	GPUAPI_DEFAULT2D_TEXTURE_SIZE			16
#define	GPUAPI_DEFAULTCUBE_TEXTURE_SIZE			16
#define	GPUAPI_DISSOLVE_TEXTURE_SIZE			16
#define	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE	32
#define GPUAPI_SSAO_ROTATION_TEXTURE_SIZE		4

namespace GpuApi
{
	// ----------------------------------------------------------------------

	#include "../redMath/float16compressor.h"

	// ----------------------------------------------------------------------

	namespace Utils
	{
		// Texture factory
		void TextureFactoryOGL( const Capabilities &caps, const TextureDesc &desc, bool *outSupported, GLuint* outTexture, Bool isCooked = false, const TextureInitData* initData = nullptr )
		{
			if ( outSupported )
			{
				*outSupported = false;
			}

			GPUAPI_ASSERT( !(outTexture && *outTexture) );

			if ( !IsTextureSizeValidForFormat( desc.width, desc.height, desc.format ) )
			{
				return;
			}

			if ( outSupported )
			{
				if (desc.width > 0 && desc.height > 0 && !(desc.usage & TEXUSAGE_BackBuffer))
				{
					*outSupported = true;
				}
			}

			if ( desc.type != TEXTYPE_2D )
			{
				GPUAPI_HALT("NOT IMPLEMENTED");
			}

			if ( desc.usage & TEXUSAGE_RenderTarget && desc.format != TEXFMT_R8G8B8A8)
			{
				GPUAPI_HALT("NOT IMPLEMENTED");
			}

			if ( desc.usage & TEXUSAGE_DepthStencil && desc.format != TEXFMT_D32F)
			{
				GPUAPI_HALT("NOT IMPLEMENTED");
			}

			if ( outTexture )
			{
				// The texture we're going to render to
				GLuint texture;
				glGenTextures( 1, &texture );

				if ( desc.type == TEXTYPE_2D )
				{
					// "Bind" the newly created texture : all future texture functions will modify this texture
					glBindTexture( GL_TEXTURE_2D, texture );

					// Give an empty image to OpenGL ( the last "0" )
					if ( IsTextureFormatDXT( desc.format ) )
					{
						if ( initData != nullptr )
						{
							for ( Uint32 slice = 0; slice < desc.sliceNum; ++slice )
							{
								for ( Uint32 mip = 0; mip < desc.initLevels; ++mip )
								{
									Uint32 width = desc.width >> mip;
									Uint32 height = desc.height >> mip;

									// Calculate size
									const Uint32 totalBits = ((width+3)/4) * ((height+3)/4) * Utils::GetTextureFormatPixelSize( desc.format ) * 16;
									Uint32 size = totalBits / 8;

									const Uint32 dataIndex = mip + slice * desc.initLevels;
									const void* initDataForLevel = initData->m_mipsInitData[dataIndex].m_data;

									// Allocate memory for the texture
									glCompressedTexImage2D( GL_TEXTURE_2D, mip, MapInternalFormat( desc.format ), width, height, 0, size, initDataForLevel );
								}
							}
						}
						else
						{
							GPUAPI_HALT("No init data for compressed texture");
						}
					}
					else
					{
						for ( Uint32 slice = 0; slice < desc.sliceNum; ++slice )
						{
							for ( Uint32 mip = 0; mip < desc.initLevels; ++mip )
							{
								Uint32 width = desc.width >> mip;
								Uint32 height = desc.height >> mip;
								
								if (initData != nullptr)
								{
									const Uint32 dataIndex = mip + slice * desc.initLevels;
									const void* initDataForLevel = initData->m_mipsInitData[dataIndex].m_data;
									glTexImage2D( GL_TEXTURE_2D, 0, MapInternalFormat( desc.format ), width, height, 0, MapFormat( desc.format ), GL_UNSIGNED_BYTE, initDataForLevel );
								}
								else
								{
									glTexImage2D( GL_TEXTURE_2D, 0, MapInternalFormat( desc.format ), width, height, 0, MapFormat( desc.format ), GL_UNSIGNED_BYTE, nullptr );
								}
							}
						}
					}
				}
				else if ( desc.type == TEXTYPE_CUBE )
				{
					// "Bind" the newly created texture : all future texture functions will modify this texture
					glBindTexture( GL_TEXTURE_CUBE_MAP, texture );
				}
				else if ( desc.type == TEXTYPE_ARRAY )
				{
					// "Bind" the newly created texture : all future texture functions will modify this texture
					glBindTexture( GL_TEXTURE_2D_ARRAY, texture );
				}

				// Poor filtering. Needed !
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

				*outTexture = texture;

//				D3D11_TEXTURE2D_DESC texDesc;
//				texDesc.Usage = D3D11_USAGE_DEFAULT;
//				texDesc.Width = desc.width;
//				texDesc.Height = desc.height;
//				texDesc.MipLevels = desc.initLevels;
//				texDesc.ArraySize = 1;
//				texDesc.Format = Map(desc.format);
//				texDesc.SampleDesc.Count = desc.msaaLevel > 0 ? desc.msaaLevel : 1;
//				texDesc.SampleDesc.Quality = 0;
//				texDesc.BindFlags = 0;
//				texDesc.ESRAMOffsetBytes = 0;
//
//				if (desc.usage & TEXUSAGE_Samplable)
//				{
//					texDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
//				}
//
//				if (desc.usage & TEXUSAGE_RenderTarget)
//				{
//					if ( desc.msaaLevel < 1 )
//					{
//						texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
//					}
//					else
//					{
//						texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
//					}
//				}
//
//				if (desc.usage & TEXUSAGE_DepthStencil)
//				{
//					texDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
//					if ( desc.usage & TEXUSAGE_Samplable ) 
//					{
//						texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
//					}
//				}
//
//				texDesc.CPUAccessFlags = 0;
//				texDesc.MiscFlags = 0;
//
//				if (desc.usage & TEXUSAGE_Dynamic)
//				{
//					texDesc.Usage = D3D11_USAGE_DYNAMIC;
//					texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//				}
//
//				if ( desc.usage & TEXUSAGE_Staging )
//				{
//					texDesc.Usage = D3D11_USAGE_STAGING;
//					texDesc.CPUAccessFlags = ( ( desc.usage & TEXUSAGE_StagingWrite ) == TEXUSAGE_StagingWrite ) ? D3D11_CPU_ACCESS_WRITE : D3D11_CPU_ACCESS_READ;
//				}
//
//				if ( TEXTYPE_CUBE == desc.type )
//				{
//					texDesc.ArraySize = 6 * desc.sliceNum;
//					texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
//				}
//
//				if ( TEXTYPE_ARRAY == desc.type )
//				{
//					texDesc.ArraySize = desc.sliceNum;
//					texDesc.MiscFlags = 0;
//				}
//
//				//dex++
//				if ( (desc.usage & TEXUSAGE_GenMip) && (desc.usage & TEXUSAGE_RenderTarget) )
//				{
//					texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
//				}
//				//dex--
//
//				if ( (desc.usage & TEXUSAGE_ESRAMResident) )
//				{
//					texDesc.MiscFlags |= D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;
//					texDesc.ESRAMOffsetBytes = desc.esramOffset;
//				}
//
//				// Create texture
//				ID3D11Texture2D* texture2D = NULL;
//				if ( !SUCCEEDED( GetDevice()->CreateTexture2D( &texDesc, NULL, &texture2D ) ) )
//				{
//					GPUAPI_ASSERT( !(outTexture && *outTexture) );
//					return;
//				}
//
//#ifdef GPU_API_DEBUG_PATH
//				const char* debugName = "texture";
//				texture2D->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
//#endif
//
//				*outTexture = texture2D;
//
//				GPUAPI_ASSERT( outTexture && *outTexture );
			}
		}

		typedef void(*Texture2DFillFunction)(		  RedVector4* pOut,
												const RedVector2* pTexCoord,
												const RedVector2* pTexelSize,
													  void*						  pData);
			
		void InitInternalTextureData2D( STextureData &td, Uint32 size, Texture2DFillFunction funcFill )
		{
			//D3D11_TEXTURE2D_DESC texDesc;
			//texDesc.Usage = D3D11_USAGE_DEFAULT;
			//texDesc.Width = size;
			//texDesc.Height = size;
			//texDesc.MipLevels = 1;
			//texDesc.ArraySize = 1;
			//texDesc.Format = Map(TEXFMT_R8G8B8A8);
			//texDesc.SampleDesc.Count = 1;
			//texDesc.SampleDesc.Quality = 0;
			//texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			//texDesc.CPUAccessFlags = 0;
			//texDesc.MiscFlags = 0;

			td.m_Desc.type	 = TEXTYPE_2D;
			td.m_Desc.width  = size;
			td.m_Desc.height = size;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = 1;

			Uint8* data = NULL;
			Uint32 bytesPerPixel = 4;
			Uint32 pitch = bytesPerPixel * size;
			{ // create init data
				RedVector4 value;
				RedVector2 coord, texelSize;
				Uint8* pos;

				data = (Uint8*) GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size * size * 4, 16 );
				if ( data == nullptr )
				{
					GPUAPI_HALT("NOT IMPLEMENTED");
					return;
				}

				for (Uint32 y = 0; y < size; y++)
				{
					coord.Y = (y + 0.5f) / size;

					for (Uint32 x = 0; x < size; x++)
					{
						coord.X = (x + 0.5f) / size;

						funcFill(&value, &coord, &texelSize, NULL);

						pos = data + y * pitch + x * bytesPerPixel;

						pos[0] = (Uint8)(value.X * 255);
						pos[1] = (Uint8)(value.Y * 255);
						pos[2] = (Uint8)(value.Z * 255);
						pos[3] = (Uint8)(value.W * 255);
					}
				}
			}

			// Create one OpenGL texture
			GLuint textureID;
			glGenTextures(1, &textureID);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, textureID);

			// Give the image to OpenGL
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			td.m_texture = textureID;
		}

		typedef void(*Texture3DFillFunction)(
													  RedVector4* pOut, 
												const RedVector3* pTexCoord, 
												const RedVector3* pTexelSize,
													  void*						  pData);

		void InitInternalTextureDataCUBE( STextureData &td, Uint32 size, Texture3DFillFunction funcFill )
		{
			td.m_Desc.type	 = TEXTYPE_2D;
			td.m_Desc.width  = size;
			td.m_Desc.height = size;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = 1;

			Uint8* data = NULL;
			Uint32 bytesPerPixel = 4;
			Uint32 pitch = bytesPerPixel * size;
			{ // create init data
				RedVector4 value;
				RedVector3 coord, texelSize;
				Uint8* pos;

				data = (Uint8*) GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size * size * bytesPerPixel * 6, 16 );
				if ( data == nullptr )
				{
					GPUAPI_HALT( "Can't allocate texture init memory" );
					return;
				}

				for ( Uint32 face = 0; face < 6; ++face )
				{
					coord.Z = 0;
					for (Uint32 y = 0; y < size; y++)
					{
						coord.Y = (y + 0.5f) / size;

						for (Uint32 x = 0; x < size; x++)
						{
							coord.X = (x + 0.5f) / size;

							funcFill(&value, &coord, &texelSize, NULL);

							pos = data + y * pitch + x * bytesPerPixel + pitch * size * 0;

							pos[0] = (Uint8)(value.X * 255);
							pos[1] = (Uint8)(value.Y * 255);
							pos[2] = (Uint8)(value.Z * 255);
							pos[3] = (Uint8)(value.W * 255);
						}
					}
				}
			}


			// Create one OpenGL texture
			GLuint textureID;
			glGenTextures(1, &textureID);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture( GL_TEXTURE_CUBE_MAP, textureID );

			// Give the image to OpenGL
			glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 0 * pitch * size );
			glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 1 * pitch * size );
			glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 2 * pitch * size );
			glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 3 * pitch * size );
			glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 4 * pitch * size );
			glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + 5 * pitch * size );

			glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

			td.m_texture = textureID;
		}

		RED_FORCE_INLINE void DissolveCoordStep( Uint8 &value, Uint8 &outX, Uint8 &outY )
		{
			Uint8 v = (value&3);
			value >>= 2;
			Uint8 dx[] = { 0, 1, 1, 0 };
			Uint8 dy[] = { 0, 1, 0, 1 };
			outX = (outX<<1) + dx[v];
			outY = (outY<<1) + dy[v];
		}

		RED_FORCE_INLINE void DissolveCoord( Uint8 value, Uint8 &outX, Uint8 &outY )
		{
			outX = 0;
			outY = 0;
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
		}

		RED_FORCE_INLINE Uint8 DissolveValueForCoord( Uint32 x, Uint32 y )
		{
			GPUAPI_ASSERT( x < 16 && y < 16 );

			// lame (suboptimal)

			Uint8 val = 0;
			for ( Uint32 i=0; i<256; ++i )
			{
				Uint8 _x, _y;
				DissolveCoord( (Uint8)i, _x, _y );
				if ( x==_x && y==_y )
				{
					val = (Uint8)i;
					break;
				}
			}

			return val;
		}

		// Dissolve texture generator
		void GenerateDissolveTexture(	  RedVector4* pOut,
											const RedVector2* pTexCoord,
											const RedVector2* pTexelSize,
												  void*						  pData)
		{
			Uint32 res = GPUAPI_DISSOLVE_TEXTURE_SIZE;
			Uint32 x = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->X), 0, res-1 );
			Uint32 y = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->Y), 0, res-1 );

			// get dissolve value
			Uint8 val = DissolveValueForCoord( x, y );

			// clamp values so that 0.f clip threshold will give up full opacity, and 1.f full transparency
			val = Red::Math::NumericalUtils::Clamp< Uint8 >( val, 1, 255 );

			// build result
			*pOut = RedVector4(val / 256.f, val / 256.f, val / 256.f, val / 256.f);
		}

		// Poisson rotation texture generator
		void GeneratePoissonRotationTexture(RedVector4* pOut,
			const RedVector2* pTexCoord,
			const RedVector2* pTexelSize,
			void*						  pData)
		{	
			Uint32 res = GPUAPI_POISSON_ROTATION_TEXTURE_SIZE;
			Uint32 x = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->X), 0, res-1 );
			Uint32 y = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->Y), 0, res-1 );

			// TODO RED RANDOM
			Float angle = Float( rand() % ( 360*20 ) ); // DissolveValueForCoord( x, y ) * (360.f / 256.f);
			angle /= 20.0f;
			Float dx	= sinf( DEG2RAD( angle ) );
			Float dy	= cosf( DEG2RAD( angle ) );

			pOut->X = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
			pOut->Y = Red::Math::NumericalUtils::Clamp(  dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->Z = Red::Math::NumericalUtils::Clamp( -dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->W = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
		}

		/// Xor fill
		void BlankTextureFill(RedVector4* pOut,
			const RedVector2* pTexCoord,
			const RedVector2* pTexelSize,
			void*						  pData)
		{
			*pOut = RedVector4(0.0f, 0.0f, 0.0f, 0.0f);
		}

		void FlatNormalTextureFill(RedVector4* pOut,
			const RedVector2* pTexCoord,
			const RedVector2* pTexelSize,
			void*						  pData)
		{
			*pOut = RedVector4(1.0f, 0.5f, 0.5f, 0.0f);
		}

		/// Xor fill
		void DefaultTextureFill(RedVector4* pOut,
			const RedVector2* pTexCoord,
			const RedVector2* pTexelSize,
			void*						  pData)
		{
			Uint32 x = (Uint32)(pTexCoord->X * 255);
			Uint32 y = (Uint32)(pTexCoord->Y * 255);
			Uint32 xor = x ^ y;
			*pOut = RedVector4(x / 255.0f, 0.0f, y / 255.0f, 1.0f);
		}

		// Cube fill
		void DefaultCubeTextureFill(		  RedVector4* pOut,
										const RedVector3* pTexCoord,
										const RedVector3* pTexelSize,
											  void*						  pData)
		{
			if ( fabs(pTexCoord->X) > fabs(pTexCoord->Y) )
			{
				if ( fabs(pTexCoord->X) > fabs(pTexCoord->Z) )
				{
					if ( pTexCoord->X > 0.0f )
					{
						*pOut = RedVector4(1.0f, 0.5f, 0.5f, 1.0f);
					}
					else
					{
						*pOut = RedVector4(0.0f, 0.5f, 0.5f, 1.0f);
					}
				}
				else
				{
					if ( pTexCoord->Z > 0.0f )
					{
						*pOut = RedVector4(0.5f, 0.5f, 1.0f, 1.0f);
					}
					else
					{
						*pOut = RedVector4(0.5f, 0.5f, 0.0f, 1.0f);
					}
				}
			}
			else
			{
				if ( fabs(pTexCoord->Y) > fabs(pTexCoord->Z) )
				{
					if ( pTexCoord->Y > 0.0f )
					{
						*pOut = RedVector4(0.5f, 1.0f, 0.5f, 1.0f);
					}
					else
					{
						*pOut = RedVector4(0.5f, 0.0f, 0.5f, 1.0f);
					}
				}
				else
				{
					if ( pTexCoord->Z > 0.0f )
					{
						*pOut = RedVector4(0.5f, 0.5f, 1.0f, 1.0f);
					}
					else
					{
						*pOut = RedVector4(0.5f, 0.5f, 0.0f, 1.0f);
					}
				}
			}
		}

		// SSAO rotation generator
		void GenerateSSAORotationNoise(RedVector4* pOut,
			const RedVector2* pTexCoord,
			const RedVector2* pTexelSize,
			void*						  pData)
		{
			const Uint8 values[16][4] =
			{
				{ 129, 253, 0, 0 },
				{  69,  69, 0, 0 },
				{   9, 221, 0, 0 },
				{ 205, 237, 0, 0 },
				{  85,  57, 0, 0 },
				{ 145,   9, 0, 0 },
				{ 221,  25, 0, 0 },
				{  25,  41, 0, 0 },
				{  41, 117, 0, 0 },
				{ 101, 205, 0, 0 },
				{ 161,  85, 0, 0 },
				{ 237, 101, 0, 0 },
				{ 253, 177, 0, 0 },
				{  57, 129, 0, 0 },
				{ 117, 145, 0, 0 },
				{ 177, 161, 0, 0 }
			};

			Uint32 res       = 4;
			Uint32 x         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->X), 0, res-1 );
			Uint32 y         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->Y), 0, res-1 );
			const Uint8 *v = values[x + y * res];

			Float fX		= v[0] - 127.5f;
			Float fY		= v[1] - 127.5f;
			Float fZ		= v[2] - 127.5f;
			Float fLen		= sqrtf( fX*fX + fY*fY + fZ*fZ );
			Float fLenInv	= fLen > 0.0001f ? 1.f / fLen : 0.f;
			Float fMul		= 0.5f * fLenInv;

			pOut->X = fMul * fX + 0.5f;
			pOut->Y = fMul * fY + 0.5f;
			pOut->Z = fMul * fZ + 0.5f;
			pOut->W = 0;
		}
	}

	// ----------------------------------------------------------------------
	// TextureDesc

	TextureDesc::TextureDesc ()
		: type( TEXTYPE_2D )
		, width( 0 )
		, height( 0 )
		, initLevels( 0 )
		, usage( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, esramOffset( 0 )
		, sliceNum ( 1 )
		, msaaLevel( 0 )
		//, expBias( 0 )
		//, edramHeight( 0 )
		//, hizOffset( 0 )
	{}

	Uint32 TextureDesc::CalcTargetLevels() const
	{
		return initLevels > 0 ? initLevels : (1 + Red::Math::MLog2( Red::Math::NumericalUtils::Max( width, height ) ));
	}

	Bool TextureDesc::HasMips() const
	{
		Bool hasMips = (initLevels > 1 || 0 == initLevels && Red::Math::NumericalUtils::Max( width, height ) >= 2);
		GPUAPI_ASSERT( hasMips == (CalcTargetLevels() > 1) );
		return hasMips;
	}

	// ----------------------------------------------------------------------

	TextureDataDesc::TextureDataDesc()
		: width( 0 )
		, height( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, rowPitch( 0 )
		, slicePitch( 0 )
		, data( NULL )
	{}

	//// ----------------------------------------------------------------------
	//ID3D11Resource* GetD3DTextureBase( const TextureRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pTexture) );
	//	return ref ? GetDeviceData().m_Textures.Data(ref).m_pTexture : NULL;
	//}

	//ID3D11Texture2D* GetD3DTexture2D( const TextureRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && (!GetDeviceData().m_Textures.Data(ref).m_pTexture ) ) );
	//	return ref ? static_cast<ID3D11Texture2D*>( GetDeviceData().m_Textures.Data(ref).m_pTexture ) : NULL;
	//}

	//ID3D11Texture2D* GetD3DTextureCube( const TextureRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && (!GetDeviceData().m_Textures.Data(ref).m_pTexture || TEXTYPE_CUBE!=GetDeviceData().m_Textures.Data(ref).m_Desc.type)) );
	//	return ref ? static_cast<ID3D11Texture2D*>( GetDeviceData().m_Textures.Data(ref).m_pTexture ) : NULL;
	//}

	//ID3D11RenderTargetView* GetD3DRenderTargetView( const TextureRef &ref, int sliceID/*=-1*/ )
	//{
	//	//dex++: added slice support
	//	const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);
	//	if ( ref )
	//	{
	//		if ( sliceID == -1 )
	//		{
	//			GPUAPI_ASSERT( texData.m_pRenderTargetView != NULL );
	//			return texData.m_pRenderTargetView;
	//		}
	//		else
	//		{
	//			if ( texData.m_pRenderTargetViewsArray == nullptr )
	//			{
	//				GPUAPI_HALT( TXT( "No rendertarget view array in array texture" ) );
	//				return nullptr;
	//			}

	//			GPUAPI_ASSERT( sliceID < texData.m_RenderTargetViewsArraySize );
	//			GPUAPI_ASSERT( texData.m_pRenderTargetViewsArray[sliceID] != NULL, TXT( "Rendertarget view not created" ) );
	//			return texData.m_pRenderTargetViewsArray[sliceID];
	//		}
	//	}
	//	else
	//	{
	//		return NULL;
	//	}
	//	//dex--
	//}

	//ID3D11UnorderedAccessView* GetD3DUnorderedAccessView( const TextureRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pUnorderedAccessView) );
	//	return ref ? GetDeviceData().m_Textures.Data(ref).m_pUnorderedAccessView : NULL;
	//}

	//ID3D11DepthStencilView* GetD3DDepthStencilView( const TextureRef &ref, int sliceID/*=-1*/, Bool isReadOnly/*=false*/ )
	//{
	//	//dex++: added slice support
	//	const GpuApi::STextureData& texData = GetDeviceData().m_Textures.Data(ref);
	//	if ( ref )
	//	{
	//		if ( sliceID == -1 )
	//		{
	//			GPUAPI_ASSERT( NULL != texData.m_pDepthStencilView );
	//			GPUAPI_ASSERT( NULL != texData.m_pDepthStencilViewReadOnly );
	//			return isReadOnly ? texData.m_pDepthStencilViewReadOnly : texData.m_pDepthStencilView;
	//		}
	//		else
	//		{
	//			GPUAPI_ASSERT( !isReadOnly, TXT( "Only non-sliced readonly dsv supported" ) );

	//			if ( texData.m_pDepthStencilViewsArray == nullptr )
	//			{
	//				GPUAPI_HALT( TXT( "No depthstencil view array in array texture" ) );
	//				return nullptr;
	//			}

	//			//GPUAPI_ASSERT( sliceID < texData.m_depthTargetViewsArraySize );
	//			GPUAPI_ASSERT( texData.m_pDepthStencilViewsArray[sliceID] != NULL, TXT( "Stencil view not created" ) );
	//			GPUAPI_ASSERT( sliceID >= 0 && sliceID < (int)texData.m_Desc.sliceNum );
	//			return texData.m_pDepthStencilViewsArray[sliceID];
	//		}
	//	}
	//	else
	//	{
	//		return NULL;
	//	}
	//	//dex--
	//}

	//ID3D11ShaderResourceView* GetD3DShaderResourceView( const TextureRef &ref )
	//{
	//	GPUAPI_ASSERT( !(ref && !GetDeviceData().m_Textures.Data(ref).m_pShaderResourceView) );
	//	return ref ? GetDeviceData().m_Textures.Data(ref).m_pShaderResourceView : NULL;
	//}

	//// ----------------------------------------------------------------------

	void AddRef( const TextureRef &texture )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );
		GetDeviceData().m_Textures.IncRefCount( texture );
	}

	Int32 Release( const TextureRef &texture )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );

		TextureRef parentRef;
		
		// Release given texture
		{
			SDeviceData &dd = GetDeviceData();
			
			// Release and optionally destroy
			GPUAPI_ASSERT( texture );
			GPUAPI_ASSERT( dd.m_Textures.GetRefCount( texture ) >= 1 );

			if ( 0 == dd.m_Textures.DecRefCount( texture ) )
			{	
				STextureData &data = dd.m_Textures.Data( texture );

				// Take ownership over parent reference (will be released later)
				parentRef = data.parentTexId;
				data.parentTexId = TextureRef::Null();

				// Count memory usage
				if ( data.m_Desc.usage == TEXUSAGE_Samplable )
				{
					const Int32 textureMemory = CalcTextureSize( data.m_Desc );
					dd.m_TextureStats.RemoveTexture( textureMemory, data.m_Group );
				}

				if ( glIsTexture( data.m_texture ) )
				{
					glDeleteTextures( 1, &data.m_texture );
				}

				//if ( data.m_pShaderResourceView )
				//{
				//	data.m_pShaderResourceView->Release();
				//	data.m_pShaderResourceView = NULL;
				//}

				//if ( data.m_pUnorderedAccessView )
				//{
				//	data.m_pUnorderedAccessView->Release();
				//	data.m_pUnorderedAccessView = NULL;
				//}

				//if ( data.m_pRenderTargetView )
				//{
				//	data.m_pRenderTargetView->Release();
				//	data.m_pRenderTargetView = NULL;
				//}

				//if ( data.m_pDepthStencilView )
				//{
				//	data.m_pDepthStencilView->Release();
				//	data.m_pDepthStencilView = NULL;
				//}

				//if ( data.m_pDepthStencilViewReadOnly )
				//{
				//	data.m_pDepthStencilViewReadOnly->Release();
				//	data.m_pDepthStencilViewReadOnly = NULL;
				//}

				//// release renderTargetViewArray related resources
				//{
				//	GPUAPI_ASSERT( (data.m_RenderTargetViewsArraySize > 0) == (NULL != data.m_pRenderTargetViewsArray) );
				//	for ( Uint32 i=0; i<data.m_RenderTargetViewsArraySize; ++i )
				//	{
				//		if ( data.m_pRenderTargetViewsArray[i] )
				//		{
				//			data.m_pRenderTargetViewsArray[i]->Release();
				//			data.m_pRenderTargetViewsArray[i] = NULL;
				//		}
				//	}

				//	delete[] data.m_pRenderTargetViewsArray;
				//	data.m_pRenderTargetViewsArray = NULL;
				//	data.m_RenderTargetViewsArraySize = 0;
				//}

				//// release depthTargetViewArray related resources
				//{
				//	GPUAPI_ASSERT( (data.m_DepthTargetViewsArraySize > 0) == (NULL != data.m_pDepthStencilViewsArray) );
				//	for ( Uint32 i=0; i<data.m_DepthTargetViewsArraySize; ++i )
				//	{
				//		if ( data.m_pDepthStencilViewsArray[i] )
				//		{
				//			data.m_pDepthStencilViewsArray[i]->Release();
				//			data.m_pDepthStencilViewsArray[i] = NULL;
				//		}
				//	}

				//	delete[] data.m_pDepthStencilViewsArray;
				//	data.m_pDepthStencilViewsArray = NULL;
				//	data.m_DepthTargetViewsArraySize = 0;
				//}

				// Destroy texture
				dd.m_Textures.Destroy( texture );
			}
		}

		// Release parent reference
		if ( parentRef )
		{
			Release( parentRef );
			parentRef = TextureRef::Null();
		}

		return 0;
	}

	void GetTextureDesc( const TextureRef &ref, TextureDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		outDesc = GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	const TextureDesc& GetTextureDesc( const TextureRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		return GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	void SetTextureDebugPath( const TextureRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		STextureData &data = GetDeviceData().m_Textures.Data(ref);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAYSIZE(data.m_debugPath) );

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( data.m_debugPath );

		// Destroy previous data
		data.m_pTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );
		if (data.m_pShaderResourceView)
		{
			data.m_pShaderResourceView->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );
		}

		if (pathLen > 0)
		{
			data.m_pTexture->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
			if (data.m_pShaderResourceView)
			{
				data.m_pShaderResourceView->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, data.m_debugPath );
			}
		}
#endif
	}

	void GetTextureLevelDesc( const TextureRef &ref, Uint16 level, TextureLevelDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);

		outDesc.width = data.m_Desc.width >> level;
		outDesc.height = data.m_Desc.height >> level;


		//GPUAPI_ASSERT( level < data.m_Desc.CalcTargetLevels() );

		//// Get d3d texture level desc
		//D3DSURFACE_DESC d3dDesc;
		//switch ( data.m_Desc.type )
		//{
		//case TEXTYPE_2D:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture || data.m_pSurface );
		//		if ( data.m_pTexture )
		//		{
		//			static_cast<IDirect3DTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//		}
		//		else
		//		{
		//			data.m_pSurface->GetDesc( &d3dDesc );
		//		}
		//	}
		//	break;

		//case TEXTYPE_CUBE:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture );
		//		static_cast<IDirect3DCubeTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//	}
		//	break;

		//default:
		//	GPUAPI_HALT( TXT( "invalid" ) );
		//	return; // keep compiler happy (uninit variable)
		//}

		//// Get data
		//GPUAPI_ASSERT( d3dDesc.Width <= data.m_Desc.width && d3dDesc.Height <= data.m_Desc.height );
		//outDesc.width = d3dDesc.Width;
		//outDesc.height = d3dDesc.Height;		
	}

	TextureLevelDesc GetTextureLevelDesc( const TextureRef &ref, Uint16 level )
	{
		TextureLevelDesc desc;
		GetTextureLevelDesc( ref, level, desc );
		return desc;
	}

	const TextureStats* GetTextureStats()
	{
		return &GpuApi::GetDeviceData().m_TextureStats;
	}

#ifndef NO_TEXTURE_IMPORT
	Bool ImportTexture( const wchar_t* importPath, GpuApi::eTextureImportFormat format, /*out*/void*& dst, /*out*/Uint32& rowPitch, /*out*/Uint32& depthPitch, /*out*/TextureDesc& desc )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;

		//if ( !IsInit() )
		//{
		//	return false;
		//}

		//// Read bitmap info
		//DirectX::TexMetadata metadata;
		//Red::System::MemorySet( &metadata, 0, sizeof( metadata ) );

		//HRESULT hr = S_FALSE;
		//switch ( format )
		//{
		//case TIF_DDS:
		//	hr = DirectX::GetMetadataFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, metadata );
		//	break;
		//case TIF_TGA:
		//	hr = DirectX::GetMetadataFromTGAFile( importPath, metadata );
		//	break;
		//case TIF_WIC:
		//	// PNGs (and possibly others?) can be imported in BGR order, but WIC_FLAGS_FORCE_RGB forces it into RGB, which is good for us!
		//	hr = DirectX::GetMetadataFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, metadata );
		//	break;
		//}
		//
		//if ( FAILED( hr ) )
		//{
		//	GPUAPI_HALT( TXT( "Unknown texture import format!" ) );
		//	return false;
		//}

		//// Texture size should be power of 2 !
		//
		//Bool isPow2Width = Red::Math::IsPow2( static_cast< Int32 >( metadata.width ) );
		//Bool isPow2Height = Red::Math::IsPow2( static_cast< Int32 >( metadata.height ) );
		//if ( !isPow2Width || !isPow2Height )
		//{
		//	GPUAPI_HALT( TXT( "Texture size should be power of two" ) );
		//	return false;
		//}

		//DirectX::ScratchImage image;
		//hr = S_FALSE;
		//switch ( format )
		//{
		//case TIF_DDS:
		//	hr = DirectX::LoadFromDDSFile( importPath, DirectX::DDS_FLAGS_NONE, NULL, image );
		//	break;
		//case TIF_TGA:
		//	hr = DirectX::LoadFromTGAFile( importPath, NULL, image );
		//	break;
		//case TIF_WIC:
		//	hr = DirectX::LoadFromWICFile( importPath, DirectX::WIC_FLAGS_FORCE_RGB, NULL, image );
		//	break;
		//}
		//
		//if ( FAILED( hr ) )
		//{
		//	GPUAPI_HALT( TXT( "Unable to create texture from file" ) );
		//	return false;
		//}

		//const DirectX::Image* loadedImage = image.GetImage(0, 0, 0);
		//desc.width	= static_cast< GpuApi::Uint32 >( metadata.width );
		//desc.height = static_cast< GpuApi::Uint32 >( metadata.height );
		//desc.format = Map( metadata.format );
		//rowPitch	= static_cast< GpuApi::Uint32 >( loadedImage->rowPitch );
		//depthPitch	= static_cast< GpuApi::Uint32 >( loadedImage->slicePitch );

		//GPUAPI_ASSERT( !dst, TXT( "Destination memory already allocated" ) );
		//dst = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, loadedImage->slicePitch, 16 );
		//GPUAPI_ASSERT( dst, TXT( "Destination memory can't be allocated" ) );

		//Red::System::MemoryCopy( dst, loadedImage->pixels, static_cast< GpuApi::Uint32 >( loadedImage->slicePitch ) );

		//return true;
	}
#endif
	TextureRef CreateCookedTexture( const TextureDesc &desc, eTextureGroup group )
	{
		if ( !IsInit() )
		{
			return TextureRef::Null();
		}
			
		if ( !IsDescSupported( desc ) )
		{
			return TextureRef::Null();
		}

		GPUAPI_HALT( "Cooked textures are not supported on current platform" );
		return TextureRef::Null();
	}

	TextureRef CreateAliasTexture( const TextureRef &tex, eTextureFormat format, Int32 expBias )
	{
		if ( !IsInit() )
		{
			return TextureRef::Null();
		}
		
		GPUAPI_HALT( "Alias textures are not supported on current platform" );
		return TextureRef::Null();
	}

	TextureRef CreateTexture( const TextureDesc &desc, eTextureGroup group, const TextureInitData* initData )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}
			
		if ( !IsDescSupported( desc ) )
		{
			return TextureRef::Null();
		}

		// Create d3d texture
		GLuint oglTex      = 0;
		GPUAPI_ASSERT( TEXTYPE_2D == desc.type || TEXTYPE_CUBE == desc.type || TEXTYPE_ARRAY == desc.type );
		Utils::TextureFactoryOGL( GetDeviceData().m_Caps, desc, NULL, &oglTex, false, initData );
		if ( !glIsTexture(oglTex) )
		{
			return TextureRef::Null();
		}

		// Allocate resource
		SDeviceData &dd = GetDeviceData();
		Uint32 texId = dd.m_Textures.Create( 1 );
		if ( !texId )
		{
			glDeleteTextures( 1, &oglTex );
			return TextureRef::Null();
		}

		// Init resource
		GpuApi::STextureData &data = dd.m_Textures.Data( texId );
		data.m_texture = oglTex;
		data.m_Desc = desc;
		data.m_Group = group;

		// Initialize handle
		GPUAPI_ASSERT( texId && dd.m_Textures.GetRefCount(texId) );
		TextureRef texRef( texId );

//		if(desc.usage & TEXUSAGE_RenderTarget)
//		{
//			DXGI_FORMAT RTVfmt = MapRenderTargetView(desc.format);
//
//			D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
//			rtvDesc.Format = RTVfmt;
//
//			if ( desc.type == TEXTYPE_2D )
//			{
//				if ( desc.msaaLevel < 1 )
//				{
//					rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2D;
//					rtvDesc.Texture2D.MipSlice = 0;
//				}
//				else
//				{
//					rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DMS;
//				}
//			}
//			else if ( desc.type == TEXTYPE_CUBE )
//			{
//				rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
//				rtvDesc.Texture2DArray.MipSlice = 0;
//				rtvDesc.Texture2DArray.FirstArraySlice = 0;
//				rtvDesc.Texture2DArray.ArraySize = 6;
//			}
//			//dex++
//			else if ( desc.type == TEXTYPE_ARRAY )
//			{
//				rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
//				rtvDesc.Texture2DArray.MipSlice = 0;
//				rtvDesc.Texture2DArray.FirstArraySlice = 0;
//				rtvDesc.Texture2DArray.ArraySize = desc.sliceNum;
//			}
//			//dex--
//
//			HRESULT res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetView) );
//#ifdef GPU_API_DEBUG_PATH
//			if ( SUCCEEDED(res) )
//			{
//				data.m_pRenderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 5, "tex2D" );
//			}
//#endif
//			// No UAV for msaa :( 
//			if ( desc.msaaLevel < 1 )
//			{
//				D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
//				DescUAV.Format = SRVfmt;
//
//				if ( desc.type == TEXTYPE_2D )
//				{
//					DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
//					DescUAV.Texture2D.MipSlice = 0;
//				}
//				else if ( desc.type == TEXTYPE_CUBE )
//				{
//					DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
//					DescUAV.Texture2DArray.MipSlice = 0;
//					DescUAV.Texture2DArray.FirstArraySlice = 0;
//					DescUAV.Texture2DArray.ArraySize = desc.sliceNum * 6;
//				}
//				//dex++
//				else if ( desc.type == TEXTYPE_ARRAY )
//				{
//					DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
//					DescUAV.Texture2DArray.MipSlice = 0;
//					DescUAV.Texture2DArray.FirstArraySlice = 0;
//					DescUAV.Texture2DArray.ArraySize = desc.sliceNum;
//				}
//				//dex--
//
//				res &= GetDevice()->CreateUnorderedAccessView( d3dTex, &DescUAV, &(data.m_pUnorderedAccessView) );
//			}
//
//			//dex++: for texture arrays create separate render target view for each slice
//			if ( desc.type == TEXTYPE_ARRAY )
//			{
//				GPUAPI_ASSERT( 1 == desc.initLevels, TXT( "Add support for mipmaps, as it's done for cubemaps" ) );
//
//				// allocate rtv array
//				{
//					const Uint32 capacity = desc.sliceNum;
//					data.m_RenderTargetViewsArraySize = capacity;
//					data.m_pRenderTargetViewsArray = new ID3DRenderTargetView*[ capacity ];
//					GPUAPI_ASSERT( data.m_RenderTargetViewsArraySize == capacity, TXT( "Out of range" ) );
//				}
//
//				for ( Uint32 i=0; i<desc.sliceNum; ++i )
//				{
//					// describe
//					D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
//					rtvDesc.Format = RTVfmt;
//					rtvDesc.ViewDimension = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
//					rtvDesc.Texture2DArray.MipSlice = 0;
//					rtvDesc.Texture2DArray.FirstArraySlice = i;
//					rtvDesc.Texture2DArray.ArraySize = 1;
//	
//					// create
//					HRESULT res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetViewsArray[i]) );
//					GPUAPI_ASSERT( SUCCEEDED(res) && data.m_pRenderTargetViewsArray[i] != NULL );
//#ifdef GPU_API_DEBUG_PATH
//					if ( SUCCEEDED(res) )
//					{
//						data.m_pRenderTargetViewsArray[i]->SetPrivateData( WKPDID_D3DDebugObjectName, 5, "texAR" );
//					}
//#endif
//				}
//			}
//			else if ( desc.type == TEXTYPE_CUBE )
//			{
//				GPUAPI_ASSERT( desc.initLevels > 0 );
//
//				// allocate rtv array
//				{
//					const Uint32 capacity = desc.sliceNum * 6 * desc.initLevels;
//					data.m_RenderTargetViewsArraySize = capacity;
//					data.m_pRenderTargetViewsArray = new ID3DRenderTargetView*[ capacity ];
//					GPUAPI_ASSERT( data.m_RenderTargetViewsArraySize == capacity, TXT( "Out of range" ) );
//				}
//
//				// per slice
//				for ( Uint32 slice_i=0; slice_i<desc.sliceNum; ++slice_i )
//				{
//					// per face
//					for ( Uint32 face_i=0; face_i<6; ++face_i )
//					{
//						// per mip
//						for ( Uint32 mip_i=0; mip_i<desc.initLevels; ++mip_i )
//						{
//							const int descSliceIndex = slice_i * 6 + face_i;
//							const int entrySliceIndex = CalculateCubemapSliceIndex( desc, slice_i, face_i, mip_i );
//
//							// describe
//							D3D_RENDER_TARGET_VIEW_DESC rtvDesc;
//							rtvDesc.Format = RTVfmt;
//							rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
//							rtvDesc.Texture2DArray.MipSlice = mip_i;
//							rtvDesc.Texture2DArray.FirstArraySlice = descSliceIndex;
//							rtvDesc.Texture2DArray.ArraySize = 1;
//	
//							// create the 
//							HRESULT res = GetDevice()->CreateRenderTargetView( d3dTex, &rtvDesc, &(data.m_pRenderTargetViewsArray[entrySliceIndex]) );
//							GPUAPI_ASSERT( SUCCEEDED(res) && data.m_pRenderTargetViewsArray[entrySliceIndex] != NULL );
//#ifdef GPU_API_DEBUG_PATH
//							if ( SUCCEEDED(res) )
//							{
//								data.m_pRenderTargetViewsArray[entrySliceIndex]->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "texCUBE" );
//							}
//#endif
//						}
//					}
//				}
//			}
//			//dex--
//		}
//
//		if (desc.usage & TEXUSAGE_DepthStencil)
//		{
//	        DXGI_FORMAT DSVfmt = MapDepthStencilView(desc.format);
//
//			DSVfmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
//			SRVfmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
//
//			D3D_DEPTH_STENCIL_VIEW_DESC dsvDesc;
//			dsvDesc.Format = DSVfmt;
//			dsvDesc.Flags = 0;
//			if ( desc.msaaLevel < 1 )
//			{
//				dsvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
//				dsvDesc.Texture2D.MipSlice = 0;
//			}
//			else
//			{
//				dsvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DMS;
//			}
//
//			HRESULT res = GetDevice()->CreateDepthStencilView( d3dTex, &dsvDesc, &(data.m_pDepthStencilView) );
//			GPUAPI_ASSERT( SUCCEEDED(res) && data.m_pDepthStencilView != NULL );
//
//			// Create readonly DSV
//			{
//				D3D_DEPTH_STENCIL_VIEW_DESC dsvDescReadOnly = dsvDesc;
//				GPUAPI_ASSERT ( 0 == dsvDescReadOnly.Flags );
//				dsvDescReadOnly.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
//				dsvDescReadOnly.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
//
//				res = GetDevice()->CreateDepthStencilView( d3dTex, &dsvDescReadOnly, &(data.m_pDepthStencilViewReadOnly) );
//				GPUAPI_ASSERT( SUCCEEDED(res) && data.m_pDepthStencilViewReadOnly != NULL );
//			}
//
//			//dex++: for texture arrays used as depth stencil ( shadowmaps ) create separate DSV for each slice
//			if (desc.usage & TEXUSAGE_DepthStencil && desc.type == TEXTYPE_ARRAY)
//			{
//				GPUAPI_ASSERT( 1 == desc.initLevels, TXT( "Add support for mipmaps, as it's done for cubemaps" ) );
//
//				// allocate rtv array
//				{
//					const Uint32 capacity = desc.sliceNum;
//					data.m_DepthTargetViewsArraySize = capacity;
//					data.m_pDepthStencilViewsArray = new ID3DDepthStencilView*[ capacity ];
//					GPUAPI_ASSERT( data.m_DepthTargetViewsArraySize == capacity, TXT( "Out of range" ) );
//				}
//
//				for ( Uint32 i=0; i<desc.sliceNum; ++i )
//				{
//					// describe
//					D3D_DEPTH_STENCIL_VIEW_DESC rtvDesc;
//					rtvDesc.Format = DSVfmt;
//					rtvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DARRAY;
//					rtvDesc.Flags = 0;
//					rtvDesc.Texture2DArray.MipSlice = 0;
//					rtvDesc.Texture2DArray.FirstArraySlice = i;
//					rtvDesc.Texture2DArray.ArraySize = 1;
//	
//					// create
//					HRESULT res = GetDevice()->CreateDepthStencilView( d3dTex, &rtvDesc, &(data.m_pDepthStencilViewsArray[i]) );
//					GPUAPI_ASSERT( SUCCEEDED(res) && data.m_pDepthStencilViewsArray[i] != NULL );
//				}
//			}
//			//dex--
//		}
//
//		if (desc.usage & TEXUSAGE_Samplable)
//		{
//			D3D_SHADER_RESOURCE_VIEW_DESC srvDesc;
//			srvDesc.Format = SRVfmt;
//
//			if ( desc.type == TEXTYPE_2D )
//			{
//				if ( desc.msaaLevel < 1 )
//				{
//					srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
//					srvDesc.Texture2D.MostDetailedMip = 0;
//					srvDesc.Texture2D.MipLevels = desc.initLevels;
//				}
//				else
//				{
//					srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DMS;
//				}
//			}
//			else if ( desc.type == TEXTYPE_CUBE )
//			{
//				srvDesc.ViewDimension = (desc.sliceNum > 1) ? D3D_SRV_DIMENSION_TEXTURECUBEARRAY : D3D_SRV_DIMENSION_TEXTURECUBE;
//				if (desc.sliceNum > 1)
//				{
//					srvDesc.TextureCubeArray.MostDetailedMip = 0;
//					srvDesc.TextureCubeArray.MipLevels = desc.initLevels;
//					srvDesc.TextureCubeArray.First2DArrayFace = 0;
//					srvDesc.TextureCubeArray.NumCubes = desc.sliceNum;
//				}
//				else
//				{
//					srvDesc.TextureCube.MostDetailedMip = 0;
//					srvDesc.TextureCube.MipLevels = desc.initLevels;
//				}
//			}
//			else if ( desc.type == TEXTYPE_ARRAY )
//			{
//				srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
//				srvDesc.Texture2DArray.MostDetailedMip = 0;
//				srvDesc.Texture2DArray.MipLevels = desc.initLevels;
//				srvDesc.Texture2DArray.FirstArraySlice = 0;
//				srvDesc.Texture2DArray.ArraySize = desc.sliceNum;
//			}
//
//			HRESULT res = GetDevice()->CreateShaderResourceView( d3dTex, &srvDesc, &(data.m_pShaderResourceView) );
//		}
//		//dex--: DX10 and DX11 merge
		// If this is a simple texture, count used memory
		
		if ( data.m_Desc.usage == GpuApi::TEXUSAGE_Samplable  )
		{
			const Uint32 textureSize = CalcTextureSize( texRef );
			dd.m_TextureStats.AddTexture( textureSize, group );
		}

		// Return handle
		return texRef;
	}

	void GetTextureDescFromMemoryFile( const void *memoryFile, Uint32 fileSize, TextureDesc *outDesc /* = NULL */ )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

//		if ( !IsInit() )
//		{
//			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
//			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
//			return;
//		}
//#ifndef NO_TEXTURE_IMPORT
//		bool isCube = false;
//		TextureDesc localDesc;
//		{
//			DirectX::TexMetadata metaData;
//			HRESULT res = DirectX::GetMetadataFromDDSMemory(memoryFile, fileSize, DirectX::DDS_FLAGS_NONE, metaData);
//
//			localDesc.initLevels = static_cast< GpuApi::Uint32 >( metaData.mipLevels );
//			localDesc.format     = GpuApi::Map( metaData.format );
//			localDesc.height     = static_cast< GpuApi::Uint32 >( metaData.height );
//			localDesc.width      = static_cast< GpuApi::Uint32 >( metaData.width );
//			localDesc.type       = GpuApi::TEXTYPE_2D;
//			localDesc.usage      = GpuApi::TEXUSAGE_Samplable;
//			isCube				 = ((DirectX::TEX_MISC_TEXTURECUBE & metaData.miscFlags) != 0);
//		}
//
//		if ( !IsDescSupported( localDesc ) )
//		{
//			return;
//		}
//		if ( outDesc )
//		{
//			*outDesc = localDesc;
//		}
//#endif
	}

	TextureRef CreateTextureFromMemoryFile( const void *memoryFile, Uint32 fileSize, eTextureGroup group, TextureDesc *outDesc )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}

		SDeviceData &dd = GetDeviceData();

		TextureDesc ddsDesc;
		TextureInitData initData;

		if ( !CreateDDSTextureFromMemory( (Uint8*)memoryFile, fileSize, ddsDesc, initData ) )
		{
			GPUAPI_HALT( "DDS texture failed to load" );
			return TextureRef::Null();
		}

		TextureRef result = CreateTexture( ddsDesc, group, &initData );

		CleanupInitData( initData );

		return result;
	}

	TextureRef CreateCubeFaceRenderTarget( const TextureRef &texCube )
	{
		if ( !IsInit() )
		{
			GPUAPI_HALT( "Not init during attempt to create texture" );
			return TextureRef::Null();
		}
			
		if ( !texCube )
		{
			return TextureRef::Null();
		}
			
		// Test whether we've got valid cube texture
		TextureDesc descCube = GetTextureDesc( texCube );
		if ( TEXTYPE_CUBE != descCube.type || !(TEXUSAGE_RenderTarget & descCube.usage) )
		{
			return TextureRef::Null();
		}
			
		// Allocate resource
		SDeviceData &dd = GetDeviceData();
		Uint32 texFaceId = dd.m_Textures.Create( 1 );
		if ( !texFaceId )
		{
			return TextureRef::Null();
		}
		
		GPUAPI_HALT("NOT IMPLEMENTED");
		return TextureRef::Null();

//		// Init resource
//		const STextureData &cubeData = dd.m_Textures.Data( texCube );
//		STextureData &faceData = dd.m_Textures.Data( texFaceId );
//		GPUAPI_ASSERT( 1 == cubeData.m_Desc.initLevels );
//		GPUAPI_ASSERT( (TEXUSAGE_RenderTarget | TEXUSAGE_Samplable) == cubeData.m_Desc.usage );
//		faceData.m_Desc.type		= TEXTYPE_2D;
//		faceData.m_Desc.format		= cubeData.m_Desc.format;
//		faceData.m_Desc.width		= cubeData.m_Desc.width;
//		faceData.m_Desc.height		= cubeData.m_Desc.height;
//		faceData.m_Desc.initLevels	= 1;
//		faceData.m_Desc.usage		= TEXUSAGE_RenderTarget | TEXUSAGE_Samplable;
//		GPUAPI_ASSERT( NULL == faceData.m_pTexture );
//		GPUAPI_ASSERT( NULL != cubeData.m_pTexture );
//
//		D3D11_TEXTURE2D_DESC tex2d_desc;
//		tex2d_desc.Usage = D3D11_USAGE_DEFAULT;
//		ID3D11Texture2D* texture2D = NULL;
//		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
//		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
//
//		tex2d_desc.Width = cubeData.m_Desc.width;
//		tex2d_desc.Height = cubeData.m_Desc.height;
//		tex2d_desc.MipLevels = 1;
//		tex2d_desc.ArraySize = 1;
//		tex2d_desc.Format = Map(cubeData.m_Desc.format);
//		tex2d_desc.SampleDesc.Count = 1;
//		tex2d_desc.SampleDesc.Quality = 0;
//		tex2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
//		tex2d_desc.CPUAccessFlags = 0;
//		tex2d_desc.MiscFlags = 0;
//		
//		// below, the 4 lines are the HACK, we should not create additional texture for cube face
//		// but render to the face directly
//		GPUAPI_ASSERT( SUCCEEDED( GetDevice()->CreateTexture2D( &tex2d_desc, NULL, &texture2D ) ) );
//		GPUAPI_ASSERT( texture2D != NULL );
//
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "cubeFace";
//		texture2D->SetPrivateData( WKPDID_D3DDebugObjectName, 8, debugName );
//#endif
//
//		faceData.m_pTexture = texture2D;
//		
//		rtv_desc.Format = Map( cubeData.m_Desc.format );
//		rtv_desc.Texture2D.MipSlice = 0;
//
//		HRESULT res = GetDevice()->CreateRenderTargetView( faceData.m_pTexture, &rtv_desc, &(faceData.m_pRenderTargetView) );
//		GPUAPI_ASSERT( SUCCEEDED( res ), TXT("couldn't create rtv for cube face") );
//#ifdef GPU_API_DEBUG_PATH
//		if ( SUCCEEDED(res) )
//		{
//			faceData.m_pRenderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 8, "cubeFace" );
//		}
//#endif
//		// Attach parent texture
//		faceData.parentTexId = texCube;
//		AddRef( faceData.parentTexId );
//		
//		// Internal check (every textureDesc being managed should pass this test)
//		GPUAPI_ASSERT( IsDescSupported(faceData.m_Desc) );
//			
//		// Finalize
//		GPUAPI_ASSERT( texFaceId && dd.m_Textures.IsInUse(texFaceId) );
//		return TextureRef( texFaceId );
	}

	Uint32 CalcTextureSize( const TextureDesc &texDesc )
	{
		Uint32 totalPixels = 0;

		// Calculate number of pixels in texture
		// dex_todo: this does not account for alignment, compression, etc....
		const Uint32 levelsCount = texDesc.CalcTargetLevels();
		for ( Uint32 lvl_i=0; lvl_i<levelsCount; ++lvl_i )
		{				
			const Uint32 mipWidth = Red::Math::NumericalUtils::Max< Uint32 >( texDesc.width >> lvl_i, 1 );
			const Uint32 mipHeight = Red::Math::NumericalUtils::Max< Uint32 >( texDesc.height >> lvl_i, 1 );
			totalPixels += mipWidth * mipHeight;
		}

		// Cubemap has 6 faces :)
		GPUAPI_ASSERT( TEXTYPE_2D == texDesc.type || TEXTYPE_CUBE == texDesc.type || TEXTYPE_ARRAY == texDesc.type );
		if ( TEXTYPE_CUBE == texDesc.type )
		{
			totalPixels *= 6;
		}
		if ( TEXTYPE_ARRAY == texDesc.type )
		{
			totalPixels *= texDesc.sliceNum;
		}

		// Calculate 
		const Uint32 totalBits = totalPixels * Utils::GetTextureFormatPixelSize( texDesc.format );
		return totalBits / 8;
	}

	Uint32 CalcTextureSize( const TextureRef &tex )
	{
		if ( tex )
		{
			const TextureDesc &texDesc = GetDeviceData().m_Textures.Data( tex ).m_Desc;
			return CalcTextureSize( texDesc );
		}

		return 0;
	}

	Bool CopyTextureData( const TextureRef& destRef, Uint32 destLevel, const TextureRef& srcRef, Uint32 srcLevel, Uint32 dataSize, Uint32 dstArraySlice, Uint32 srcArraySlice )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;

		//// dataSize parameter is not used in DirectX10 - it is here only to keep the same interface with DirectX9

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(srcRef) );
		//const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		//GPUAPI_ASSERT( srcLevel < srcData.m_Desc.CalcTargetLevels() );
		//GPUAPI_ASSERT( srcData.m_Desc.type == TEXTYPE_2D || srcData.m_Desc.type == TEXTYPE_ARRAY );
		//GPUAPI_ASSERT( srcData.m_pTexture );

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destRef) );
		//const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);
		//GPUAPI_ASSERT( destLevel < destData.m_Desc.CalcTargetLevels() );
		//GPUAPI_ASSERT( destData.m_Desc.type == TEXTYPE_2D || destData.m_Desc.type == TEXTYPE_CUBE );
		//if ( destData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
		//	return false;
		//}
		//if ( srcData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
		//	return false;
		//}

		//Uint32 destSubresource = D3D11CalcSubresource(destLevel, dstArraySlice, destData.m_Desc.initLevels);
		//Uint32 srcSubresource = D3D11CalcSubresource(srcLevel, srcArraySlice, srcData.m_Desc.initLevels);

		//GetDeviceContext()->CopySubresourceRegion( destData.m_pTexture, destSubresource, 0, 0, 0, srcData.m_pTexture, srcSubresource, NULL );
		//return true;
	}

	Bool CopyTextureDataRaw( const TextureRef& destRef, Uint32 destSubresource, const TextureRef& srcRef, Uint32 srcSubresource )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(srcRef) );
		//const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		//GPUAPI_ASSERT( srcSubresource < srcData.m_Desc.CalcTargetLevels() );
		////dex++: extended to texture arrays
		//GPUAPI_ASSERT( srcData.m_Desc.type == TEXTYPE_2D || srcData.m_Desc.type == TEXTYPE_ARRAY );
		////dex--
		//GPUAPI_ASSERT( srcData.m_pTexture );

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destRef) );
		//const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);
		//GPUAPI_ASSERT( destSubresource < destData.m_Desc.CalcTargetLevels() );
		//GPUAPI_ASSERT( destData.m_Desc.type == TEXTYPE_2D || destData.m_Desc.type == TEXTYPE_CUBE );
		//if ( destData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
		//	return false;
		//}
		//if ( srcData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
		//	return false;
		//}

		//GetDeviceContext()->CopySubresourceRegion( destData.m_pTexture, destSubresource, 0, 0, 0, srcData.m_pTexture, srcSubresource, NULL );
		//return true;
	}

	Bool CopyTextureDataRaw( const TextureRef& destRef, const TextureRef& srcRef )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(srcRef) );
		//const STextureData &srcData = GetDeviceData().m_Textures.Data(srcRef);
		//
		////dex++: extended to texture arrays
		//GPUAPI_ASSERT( srcData.m_Desc.type == TEXTYPE_2D || srcData.m_Desc.type == TEXTYPE_ARRAY );
		////dex--
		//GPUAPI_ASSERT( srcData.m_pTexture );

		//GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(destRef) );
		//const STextureData &destData = GetDeviceData().m_Textures.Data(destRef);
	
		//GPUAPI_ASSERT( destData.m_Desc.type == TEXTYPE_2D || destData.m_Desc.type == TEXTYPE_CUBE );
		//if ( destData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
		//	return false;
		//}
		//if ( srcData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
		//	return false;
		//}

		//GetDeviceContext()->CopyResource( destData.m_pTexture, srcData.m_pTexture );
		//return true;
	}

	// read texture data ( staged textures only )
	void* LockLevelRead( const TextureRef& ref, Uint32 level, bool forceLock, Uint32& outPitch )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		//if (data.m_Desc.usage & TEXUSAGE_Staging)
		//{
		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	mapped.pData = NULL;
		//	Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
		//	D3D11_MAP_FLAG flags = forceLock ? (D3D11_MAP_FLAG)0 : D3D11_MAP_FLAG_DO_NOT_WAIT;
		//	HRESULT hRet = GetDeviceContext()->Map( data.m_pTexture, subresource, D3D11_MAP_READ, flags, &mapped );

		//	// We are still waiting for the resource, allowed situation
		//	if ( !forceLock && (hRet == DXGI_ERROR_WAS_STILL_DRAWING) )
		//	{
		//		GPUAPI_ASSERT( !mapped.pData, TXT( "Still waiting for the resource" ) );
		//		return NULL;
		//	}

		//	// Invalid state
		//	if ( !SUCCEEDED( hRet ) )
		//	{
		//		GPUAPI_LOG( TXT( "Failed to lock the texture for read: 0x%X" ), hRet );
		//		return NULL;
		//	}

		//	// Get data
		//	outPitch = mapped.RowPitch;
		//	return mapped.pData;
		//}

		// Invalid resource
		return NULL;
	}

	void* LockLevel( const TextureRef& ref, Uint32 level, Uint32 lockFlags, Uint32& outPitch )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		////dex++: enabled locks on staged textures
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		////dex--
		//{
		//	//HACK DX10 only for 2D now
		//	//dex++: arrays also work
		//	//GPUAPI_ASSERT(data.m_Desc.type == TEXTYPE_2D);
		//	//dex--
		//	//HACK DX10 only mip0 now
		//	GPUAPI_ASSERT(level == 0);

		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	mapped.pData = nullptr;
		//	Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
		//	
		//	HRESULT hr = GetDeviceContext()->Map( data.m_pTexture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
		//	if ( FAILED( hr ) )
		//	{
		//		GPUAPI_HALT( TXT( "Failed to map texture %0x subresource %d. Error code: %d" ), data.m_pTexture, subresource, hr );
		//	}

		//	outPitch = mapped.RowPitch;
		//	return mapped.pData;
		//}
		//else
		{
			return NULL;
		}
	}

	void UnlockLevel( const TextureRef& ref, Uint32 level )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		////dex++: enabled locks on staged textures
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		////dex--
		//{
		//	//HACK DX10 only mip0 now
		//	GPUAPI_ASSERT(level == 0);
		//	Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
		//	GetDeviceContext()->Unmap( data.m_pTexture, subresource );
		//}
	}

	void* LockLevelRaw( const TextureRef& ref, Uint32 subresource, Uint32 lockFlags, Uint32& outPitch )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		//{
		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( data.m_pTexture, subresource, (D3D11_MAP)MapBuffLockFlagsToD3D( lockFlags ), 0, &mapped ) );
		//	outPitch = mapped.RowPitch;
		//	return mapped.pData;
		//}
		//else
		{
			return NULL;
		}
	}

	void UnlockLevelRaw( const TextureRef& ref, Uint32 subresource )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		//{
		//	GetDeviceContext()->Unmap( data.m_pTexture, subresource );
		//}
	}

	void* AllocateTextureBuffer( const TextureRef& ref, Uint32 pitch, Uint32 size, Uint32& outPitch )
	{
		if ( !ref || size < 1 )
		{
			return NULL;
		}

		GPUAPI_ASSERT( IsInit() );

		outPitch = pitch;

		// HACK DX10 locking with offset is not supported, allocating memory for temp buffer
		void* resultPtr = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size, 16 );
		return resultPtr;
	}

	void UpdateTextureData( const TextureRef& ref, Uint32 level, void* textureData, Uint32 srcPitch )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		GPUAPI_ASSERT( level < data.m_Desc.CalcTargetLevels() );
	
		GPUAPI_HALT("NOT IMPLEMENTED");

//		// It should be 2D texture
//		GPUAPI_ASSERT( data.m_Desc.type == TEXTYPE_2D );
//		if ( data.m_pTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
//			return;
//		}
//
//
//#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
//		// HACK DX10 no argb in dx10 we have to swizzle the channels :(
//		// TODO move to BGRA in all the textures that use this format
//		if ( data.m_Desc.format == TEXFMT_R8G8B8A8 && data.m_onLoadSwapRB )
//		{
//			Uint8* colorByte = (Uint8*)textureData;
//
//			Uint32 w = data.m_Desc.width >> level;
//			Uint32 h = data.m_Desc.height >> level;
//
//			Uint8* rowStart = colorByte;
//			for ( Uint32 y = 0; y < h; ++y )
//			{
//				Uint8* texel = rowStart;
//				for ( Uint32 x = 0; x < w; ++x )
//				{
//					Uint8 temp = texel[0];
//					texel[0] = texel[2];
//					texel[2] = temp;
//					texel += 4;
//				}
//				rowStart += srcPitch;
//			}
//		}
//#endif
//
//		Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
//
//		GetDeviceContext()->UpdateSubresource( data.m_pTexture, subresource, NULL, textureData, srcPitch, 0 );
//
//		GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, textureData );
	}

	void UpdateTextureDataAsync( const TextureRef& ref, Uint32 level, void* textureData, Uint32 srcPitch, void* deferredContext )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		GPUAPI_ASSERT( level < data.m_Desc.CalcTargetLevels() );

		GPUAPI_HALT("NOT IMPLEMENTED");

//		// It should be 2D texture
//		GPUAPI_ASSERT( data.m_Desc.type == TEXTYPE_2D );
//		if ( data.m_pTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
//			return;
//		}
//
//
//#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
//		// HACK DX10 no argb in dx10 we have to swizzle the channels :(
//		// TODO move to BGRA in all the textures that use this format
//		if ( data.m_Desc.format == TEXFMT_R8G8B8A8 && data.m_onLoadSwapRB )
//		{
//			Uint8* colorByte = (Uint8*)textureData;
//
//			Uint32 w = data.m_Desc.width >> level;
//			Uint32 h = data.m_Desc.height >> level;
//
//			Uint8* rowStart = colorByte;
//			for ( Uint32 y = 0; y < h; ++y )
//			{
//				Uint8* texel = rowStart;
//				for ( Uint32 x = 0; x < w; ++x )
//				{
//					Uint8 temp = texel[0];
//					texel[0] = texel[2];
//					texel[2] = temp;
//					texel += 4;
//				}
//				rowStart += srcPitch;
//			}
//		}
//#endif
//
//		Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
//		((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( data.m_pTexture, subresource, NULL, textureData, srcPitch, 0 );
//
//		GPU_API_FREE( GpuMemoryPool_Textures, MC_TextureData, textureData );
	}


	void BindTextures( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage )
	{
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );

		for ( Uint32 i=0; i<numTextures; ++i )
		{
			Uint32 sampler_i = startSlot + i;

			if ( textures != nullptr )
			{
				TextureRef currRef = textures[i];
				GPUAPI_ASSERT( !currRef || (GetDeviceData().m_Textures.Data(currRef).m_Desc.usage & TEXUSAGE_Samplable) );

				const STextureData& td = GetDeviceData().m_Textures.Data(currRef);

				OGL_CHK( glBindMultiTextureEXT( GL_TEXTURE0 + i , GL_TEXTURE_2D, td.m_texture ) );
			}
			else
			{
				OGL_CHK( glBindMultiTextureEXT( GL_TEXTURE0 + i , GL_TEXTURE_2D, 0 ) );
			}
			break;
		}
	}

	void BindTextureUAVs( Uint32 startSlot, Uint32 numTextures, const TextureRef* textures )
	{
		for( Uint32 slot = 0; slot < numTextures; ++slot )
		{
			const STextureData& td = GetDeviceData().m_Textures.Data( textures[slot] );

			// Kamil : need to check in code layered UAV's and if there are special mipMapLvl used - seems that now from function definition
			// Kamil : assume we need red/write access mask
			OGL_CHK( glBindImageTexture( slot + startSlot , td.m_texture , 0 , false , 0, MapInternalFormat(td.m_Desc.format) , GL_READ_WRITE ) );
		}
	}

	Bool IsDescSupported( const TextureDesc &desc )
	{
		Bool isSupported = false;
		Utils::TextureFactoryOGL( GetDeviceData().m_Caps, desc, &isSupported, nullptr, nullptr );
		return isSupported;
	}

	static Uint32 CalculatePitch( Uint32 width, eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_R8G8B8A8: return 4 * width;
		case TEXFMT_Float_R32G32B32A32: return 4 * width * sizeof(Float);
		case TEXFMT_BC1: return Red::Math::NumericalUtils::Max< Uint32 >( 8, width * 2 );
		case TEXFMT_BC3: return Red::Math::NumericalUtils::Max< Uint32 >( 16, width * 4 );
		case TEXFMT_BC4: return Red::Math::NumericalUtils::Max< Uint32 >( 8, width * 4 );
		case TEXFMT_BC6H: return Red::Math::NumericalUtils::Max< Uint32 >( 16, width * 4 );
		case TEXFMT_BC7: return Red::Math::NumericalUtils::Max< Uint32 >( 16, width * 4 );
		default: return 0;
		}
	}


#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
	void SetTextureLoadSwapRB( const TextureRef &tex, Bool swapRB )
	{
		if ( !tex )
		{
			return;
		}

		// Get texture data
		STextureData& texData = GetDeviceData().m_Textures.Data( tex );
		texData.m_onLoadSwapRB = swapRB;
	}

	Bool GetTextureLoadSwapRB( const TextureRef &tex )
	{
		if ( !tex )
		{
			return false;
		}

		// Get texture data
		STextureData& texData = GetDeviceData().m_Textures.Data( tex );
		return texData.m_onLoadSwapRB;
	}
#endif

	void LoadTextureData2D( const TextureRef &destTex, Uint32 destTexLevel, const Rect* destRect, Uint32 srcRowPitch, const void *srcMemory )
	{
		if ( !destTex )
		{
			return;
		}

		if ( nullptr == srcMemory )
		{
			GPUAPI_HALT( "Source memory not defined" );
			return;
		}

		GPUAPI_HALT("NOT IMPLEMENTED");

		//// Get texture data
		//STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
		//GPUAPI_ASSERT( TEXTYPE_2D == texData.m_Desc.type || TEXTYPE_ARRAY == texData.m_Desc.type );
		//if ( texData.m_pTexture == nullptr )
		//{
		//	GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
		//	return;
		//}

		//D3D11_BOX destBox;
		//if ( destRect )
		//{
		//	destBox.left = destRect->left;
		//	destBox.top = destRect->top;
		//	destBox.right = destRect->right;
		//	destBox.bottom = destRect->bottom;
		//	destBox.front = 0;
		//	destBox.back = 1;
		//}

		//GetDeviceContext()->UpdateSubresource( texData.m_pTexture, destTexLevel, destRect ? &destBox : NULL, srcMemory, srcRowPitch, 0 );
	}

	void LoadTextureArrayLevelData( const TextureRef& destTex, Uint32 textureIndex, Uint32 mipIndex, Uint32 dataSize, const void* data )
	{
		if ( !destTex || dataSize < 1 || NULL == data )
		{
			return;
		}

		GPUAPI_HALT("NOT IMPLEMENTED");

//		// Get texture data
//		STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
//		GPUAPI_ASSERT( TEXTYPE_ARRAY == texData.m_Desc.type );
//		GPUAPI_ASSERT( mipIndex < texData.m_Desc.CalcTargetLevels() )
//		ID3D11Texture2D* d3dTexture = static_cast<ID3D11Texture2D*>( texData.m_pTexture );
//		if ( d3dTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
//			return;
//		}
//
//		Uint32 mipWidth = texData.m_Desc.width >> mipIndex;
//		Uint32 mipHeight = texData.m_Desc.height >> mipIndex;
//		Uint32 pitch = CalculatePitch( mipWidth, texData.m_Desc.format );
//
//#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
//		if ( texData.m_Desc.format == TEXFMT_R8G8B8A8 && texData.m_onLoadSwapRB ) 
//		{
//			Uint8* colorByte = (Uint8*)data;
//
//			Uint8* rowStart = colorByte;
//			for ( Uint32 y = 0; y < mipHeight; ++y )
//			{
//				Uint8* texel = rowStart;
//				for ( Uint32 x = 0; x < mipWidth; ++x )
//				{
//					Uint8 temp = texel[0];
//					texel[0] = texel[2];
//					texel[2] = temp;
//					texel += 4;
//				}
//				rowStart += pitch;
//			}
//		}
//#endif
//
//		Uint32 subresource = D3D11CalcSubresource( mipIndex, textureIndex, texData.m_Desc.initLevels );
//		GetDeviceContext()->UpdateSubresource( d3dTexture, subresource, NULL, data, pitch, 0 );
	}

	void LoadTextureArrayLevelDataAsync( const TextureRef& destTex, Uint32 textureIndex, Uint32 mipIndex, Uint32 dataSize, const void* data, void* deferredContext )
	{
		if ( !destTex || dataSize < 1 || NULL == data )
		{
			return;
		}

		GPUAPI_HALT("NOT IMPLEMENTED");

//		// Get texture data
//		STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
//		GPUAPI_ASSERT( TEXTYPE_ARRAY == texData.m_Desc.type );
//		GPUAPI_ASSERT( mipIndex < texData.m_Desc.CalcTargetLevels() )
//		ID3D11Texture2D* d3dTexture = static_cast<ID3D11Texture2D*>( texData.m_pTexture );
//		if ( d3dTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Destination texture doesn't exist" ) );
//			return;
//		}
//
//		Uint32 mipWidth = texData.m_Desc.width >> mipIndex;
//		Uint32 mipHeight = texData.m_Desc.height >> mipIndex;
//		Uint32 pitch = CalculatePitch( mipWidth, texData.m_Desc.format );
//
//#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
//		if ( texData.m_Desc.format == TEXFMT_R8G8B8A8 && texData.m_onLoadSwapRB ) 
//		{
//			Uint8* colorByte = (Uint8*)data;
//
//			Uint8* rowStart = colorByte;
//			for ( Uint32 y = 0; y < mipHeight; ++y )
//			{
//				Uint8* texel = rowStart;
//				for ( Uint32 x = 0; x < mipWidth; ++x )
//				{
//					Uint8 temp = texel[0];
//					texel[0] = texel[2];
//					texel[2] = temp;
//					texel += 4;
//				}
//				rowStart += pitch;
//			}
//		}
//#endif
//
//		Uint32 subresource = D3D11CalcSubresource( mipIndex, textureIndex, texData.m_Desc.initLevels );
//		((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( d3dTexture, subresource, NULL, data, pitch, 0 );
	}
	
	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Uint8 *outDataRGBA, Uint32 stride, Bool forceFullAlpha )
	{
		GPUAPI_ASSERT( outDataRGBA );

		if ( !texture || grabWidth < 1 || grabHeight < 1 || stride < 4 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_ERROR( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_R8G8B8A8 != td.m_Desc.format && TEXFMT_R8G8B8X8 != td.m_Desc.format) )
		{
			GPUAPI_ERROR( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		STextureData texData = GetDeviceData().m_Textures.Data(texture);
		TextureDesc desc = GetTextureDesc( texture );

		// TODO moradin these solutions are a bit hacky because they set some states that are not reset after
		if ( desc.usage & ( TEXUSAGE_RenderTarget | TEXUSAGE_BackBuffer ) )
		{
			RenderTargetSetup fakeRTSetup;
			fakeRTSetup.SetColorTarget( 0, texture );
			fakeRTSetup.SetViewportFromTarget( texture );
			SetupRenderTargets( fakeRTSetup );
			glReadPixels( grabX, grabY, grabWidth, grabHeight, MapFormat(TEXFMT_R8G8B8A8), GL_UNSIGNED_BYTE, outDataRGBA );
		}
		else
		{
			if (grabX != 0 || grabY != 0 || grabWidth != desc.width || grabHeight != desc.height)
			{
				GPUAPI_HALT( "Can't grab texture areas yet" );
				return false;
			}
			
			//bind downsampled texture
			glBindTexture(GL_TEXTURE_2D, texData.m_texture);

			//read from bound texture to CPU
			glGetTexImage(GL_TEXTURE_2D, 0, MapFormat(TEXFMT_R8G8B8A8), GL_UNSIGNED_BYTE, outDataRGBA);
		}

		return true;

//		// Create staging texture
//		ID3D11Texture2D *stagingTexture = NULL;
//		D3D11_TEXTURE2D_DESC texDesc;
//		texDesc.Usage = D3D11_USAGE_STAGING;
//		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//		texDesc.ArraySize = 1;
//		texDesc.BindFlags = 0;
//		texDesc.MipLevels = 1;
//		texDesc.MiscFlags = 0;
//		texDesc.SampleDesc.Count = 1;
//		texDesc.SampleDesc.Quality = 0;
//
//		texDesc.Format = Map(td.m_Desc.format);
//		texDesc.Width = td.m_Desc.width;
//		texDesc.Height = td.m_Desc.height;
//
//		HRESULT hr = GetDevice()->CreateTexture2D(&texDesc, 0, &stagingTexture);
//		
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "tempTex";
//		stagingTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
//#endif
//
//		// Copy data
//		if ( td.m_pTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
//			return false;
//		}
//		GetDeviceContext()->CopyResource(stagingTexture, td.m_pTexture);
//
//		// Build lock rect
//		RECT realRect;
//		realRect.left   = grabX;
//		realRect.top    = grabY;
//		realRect.right  = grabX + grabWidth;
//		realRect.bottom = grabY + grabHeight;
//
//		// Lock destination surface
//		D3D11_MAPPED_SUBRESOURCE mappedTexture;
//		if ( !SUCCEEDED( GetDeviceContext()->Map( stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTexture ) ) )
//		{
//			GPUAPI_WARNING( TXT( "Texture pixels grab internal failure : lock rect failed." ) );
//			stagingTexture->Release();
//			return false;
//		}
//
//		Uint8* data = (Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4;
//
//		// Copy data to pixel buffer
//		if ( forceFullAlpha )
//		{
//			for ( Uint32 y=0; y<grabHeight; y++ )
//			{
//				Uint8 *src  = data + y * mappedTexture.RowPitch;
//				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
//				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
//				{
//					dest[0] = src[0];
//					dest[1] = src[1];
//					dest[2] = src[2];
//					dest[3] = 255;
//				}
//			}
//		}
//		else
//		{
//			for ( Uint32 y=0; y<grabHeight; y++ )
//			{
//				Uint8 *src  = data + y * mappedTexture.RowPitch;
//				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
//				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
//				{
//					dest[0] = src[0];
//					dest[1] = src[1];
//					dest[2] = src[2];
//					dest[3] = src[3];
//				}
//			}
//		}
//
//		// Unlock and release
//		GetDeviceContext()->Unmap( stagingTexture, 0 );
//		
//		stagingTexture->Release();
//
//		// Return :)
//		return true;
	}

	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Float *outData )
	{
		GPUAPI_ASSERT( outData );

		if ( !texture || grabWidth < 1 || grabHeight < 1 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_ERROR( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}		

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_Float_R16G16B16A16 != td.m_Desc.format && TEXFMT_Float_R32G32B32A32 != td.m_Desc.format && TEXFMT_Float_R32 != td.m_Desc.format) )
		{
			GPUAPI_ERROR( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;

//		// Create staging texture
//		ID3D11Texture2D *stagingTexture = NULL;
//		D3D11_TEXTURE2D_DESC texDesc;
//		texDesc.Usage = D3D11_USAGE_STAGING;
//		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//		texDesc.ArraySize = 1;
//		texDesc.BindFlags = 0;
//		texDesc.MipLevels = 1;
//		texDesc.MiscFlags = 0;
//		texDesc.SampleDesc.Count = 1;
//		texDesc.SampleDesc.Quality = 0;
//
//		texDesc.Format = Map(td.m_Desc.format);
//		texDesc.Width = td.m_Desc.width;
//		texDesc.Height = td.m_Desc.height;
//
//		HRESULT hr = GetDevice()->CreateTexture2D(&texDesc, 0, &stagingTexture);
//
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "tempTex";
//		stagingTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
//#endif
//
//		// Copy data
//		if ( td.m_pTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
//			return false;
//		}
//		GetDeviceContext()->CopyResource(stagingTexture, td.m_pTexture);
//
//		// Build lock rect
//		RECT realRect;
//		realRect.left   = grabX;
//		realRect.top    = grabY;
//		realRect.right  = grabX + grabWidth;
//		realRect.bottom = grabY + grabHeight;
//
//		// Lock destination surface
//		D3D11_MAPPED_SUBRESOURCE mappedTexture;
//		if ( !SUCCEEDED( GetDeviceContext()->Map( stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTexture ) ) )
//		{
//			GPUAPI_WARNING( TXT( "Texture pixels grab internal failure : lock rect failed." ) );
//			stagingTexture->Release();
//			return false;
//		}
//
//		// Grab the data
//		switch ( td.m_Desc.format )
//		{
//		case TEXFMT_Float_R16G16B16A16:
//			{
//				const Uint16* data = (const Uint16*)((const Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Uint16));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; y++ )
//				{	
//					const Uint16 *src  = (const Uint16*)((Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + 4 * (y * grabWidth);
//					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
//					{
//						dest[0] = Float16Compressor::Decompress( src[0] );
//						dest[1] = Float16Compressor::Decompress( src[1] );
//						dest[2] = Float16Compressor::Decompress( src[2] );
//						dest[3] = 1.f;
//					}
//				}
//			}
//			break;
//
//		case TEXFMT_Float_R32G32B32A32:
//			{
//				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Float));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; y++ )
//				{
//					Float *src  = (Float*)((Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + 4 * (y * grabWidth);
//					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
//					{
//						dest[0] = src[0];
//						dest[1] = src[1];
//						dest[2] = src[2];
//						dest[3] = 1.f;
//					}
//				}
//			}
//			break;
//
//		case TEXFMT_Float_R32:
//			{
//				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * sizeof(Float));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; ++y )
//				{
//					const Float *src  = (const Float*)((const Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + (y * grabWidth);
//					Red::System::MemoryCopy( dest, src, 4 * grabWidth );
//				}
//			}
//			break;
//
//		default:
//			GPUAPI_HALT( TXT( "Format not handled" ) );
//		}
//		
//		// Unlock and release
//		GetDeviceContext()->Unmap( stagingTexture, 0 );
//
//		stagingTexture->Release();
//
//		// Return :)
//		return true;
	}

	Bool SaveTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, const Char* fileName, eTextureSaveFormat format )
	{
#if 0
		//HACK DX10 no texture saving
		if ( !texture || grabWidth < 1 || grabHeight < 1 || !fileName )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}		

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || ( TEXFMT_A8R8G8B8 != td.m_Desc.format && format != SAVE_FORMAT_DDS ) )
		{
			GPUAPI_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		// Create software surface
		IDirect3DSurface9 *softSurf = NULL;
		D3DFORMAT softSurfFormat = D3DFMT_A8R8G8B8;
		
		if ( format == SAVE_FORMAT_DDS )
		{
			if ( td.m_Desc.format == TEXFMT_Float_R32 )
			{
				softSurfFormat = D3DFMT_R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_A32R32G32B32 || td.m_Desc.format == TEXFMT_Float_A16R16G16B16 || td.m_Desc.format == TEXFMT_Float_A2R10G10B10 )
			{
				softSurfFormat = D3DFMT_A32B32G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_G16R16 )
			{
				softSurfFormat = D3DFMT_G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_A8 )
			{
				softSurfFormat = D3DFMT_A8;
			}
			else if ( td.m_Desc.format == TEXFMT_L8 )
			{
				softSurfFormat = D3DFMT_L8;
			}
			else if ( td.m_Desc.format == TEXFMT_A8L8 )
			{
				softSurfFormat = D3DFMT_A8L8;
			}
			else
			{
				GPUAPI_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
				return false;
			}
		}

		// Build lock rect
		RECT realRect;
		realRect.left   = grabX;
		realRect.top    = grabY;
		realRect.right  = grabX + grabWidth;
		realRect.bottom = grabY + grabHeight;

		D3DXIMAGE_FILEFORMAT destFormat = Map( format );

		HRESULT hr = D3DXSaveSurfaceToFile( fileName, destFormat, td.m_pSurface, NULL, &realRect );

		// Return :)
		return SUCCEEDED( hr );
#else
		return false;
#endif
	}

	Bool SaveTextureToMemory( const Uint8* textureData, const size_t textureDataSize, const size_t width, size_t height, const eTextureFormat format, const size_t pitch, const eTextureSaveFormat saveFormat, void** buffer, size_t& size )
	{
//#ifndef NO_TEXTURE_IMPORT
//		DirectX::Image image;
//		image.pixels = const_cast< Uint8* >( textureData );
//		image.rowPitch = pitch;
//		image.slicePitch = textureDataSize;
//		image.width = width;
//		image.height = height;
//		image.format = Map( format );
//
//		DirectX::Image srcImage = image;
//		DirectX::ScratchImage img;
//		if ( DirectX::IsCompressed( Map( format ) ) )
//		{
//			// decompress first
//			GPUAPI_MUST_SUCCEED( DirectX::Decompress( image, Map( TEXFMT_R8G8B8A8 ), img ) );
//			srcImage = *img.GetImage(0, 0, 0);
//		}
//		
//		DirectX::Rect rect;
//		rect.x = 0;
//		rect.y = 0;
//		rect.w = width;
//		rect.h = height;
//		DirectX::ScratchImage* tempImage = new DirectX::ScratchImage();
//		HRESULT hr = tempImage->Initialize2D(Map( TEXFMT_R8G8B8A8 ), width, height, 1, 1);
//		if ( FAILED( hr ) )
//		{
//			GPUAPI_HALT( TXT( "SaveTextureToMemory failed on image initialization." ) );
//			return false;
//		}
//		hr = DirectX::CopyRectangle( srcImage, rect, *tempImage->GetImage(0,0,0), DirectX::TEX_FILTER_DEFAULT, 0, 0 );
//		if ( FAILED( hr ) )
//		{
//			GPUAPI_HALT( TXT( "SaveTextureToMemory failed on copying rectangle." ) );
//			return false;
//		}
//
//		DirectX::Blob blob;
//		switch ( saveFormat )
//		{
//		case SAVE_FORMAT_DDS:
//			hr = DirectX::SaveToDDSMemory(tempImage->GetImages(), tempImage->GetImageCount(), tempImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
//			break;
//		case SAVE_FORMAT_BMP:
//			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), blob );
//			break;
//		case SAVE_FORMAT_JPG:
//			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), blob );
//			break;
//		case SAVE_FORMAT_PNG:
//			hr = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), blob );
//			break;
//		case SAVE_FORMAT_TGA:
//			hr = DirectX::SaveToTGAMemory(*tempImage->GetImage(0,0,0), blob );
//			break;
//		}
//
//		if ( FAILED( hr ) )
//		{
//			GPUAPI_HALT( TXT( "SaveTextureToMemory failed." ) );
//			return false;
//		}
//
//		size = static_cast< size_t >( blob.GetBufferSize() );
//
//		*buffer = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size, 16 );
//		Red::System::MemoryCopy( *buffer, blob.GetBufferPointer(), size );
//
//		blob.Release();
//		tempImage->Release();
//#endif
//		return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	Bool SaveTextureToMemory( const TextureRef &texture, eTextureSaveFormat format, const Rect* sourceRect, void** buffer, Uint32& size )
	{
//#ifndef NO_TEXTURE_IMPORT
//		Uint32 width = sourceRect->right - sourceRect->left;
//		Uint32 height = sourceRect->bottom - sourceRect->top;
//
//		if ( !texture || width < 1 || height < 1 )
//		{
//			return false;
//		}
//
//		// Get internal data
//		SDeviceData  &dd = GetDeviceData();
//		STextureData &td = dd.m_Textures.Data( texture );
//
//		// Test if save is possible for given texture
//		if ( sourceRect->right > (Int32)td.m_Desc.width || sourceRect->bottom > (Int32)td.m_Desc.height )
//		{
//			GPUAPI_WARNING( TXT( "Attempted to save data from invalid area." ) );
//			return false;
//		}		
//
//		//// Test whether we support saving from given texture format
//		//if ( TEXTYPE_2D != td.m_Desc.type || ( TEXFMT_R8G8B8A8 != td.m_Desc.format && format != SAVE_FORMAT_DDS ) )
//		//{
//		//	GPUAPI_WARNING( TXT( "Attempt to save data from unsupported (for grabbing) format detected." ) );
//		//	return false;
//		//}
//
//		DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
//		HRESULT captureRes = DirectX::CaptureTexture(GetDevice(), GetDeviceContext(), td.m_pTexture, *scratchImage);
//		if (captureRes == S_OK)
//		{
//			DirectX::Rect rect;
//			rect.x = 0;
//			rect.y = 0;
//			rect.w = width;
//			rect.h = height;
//			DirectX::ScratchImage* tempImage = new DirectX::ScratchImage();
//			tempImage->Initialize2D(Map( TEXFMT_R8G8B8A8 ), width, height, 1, 1);
//			DirectX::CopyRectangle( *scratchImage->GetImage(0, 0, 0), rect, *tempImage->GetImage(0,0,0), DirectX::TEX_FILTER_DEFAULT, 0, 0 );
//
//			DirectX::Blob blob;
//			HRESULT saveRes;
//			switch(format)
//			{
//			case SAVE_FORMAT_DDS:
//				{
//					saveRes = DirectX::SaveToDDSMemory(tempImage->GetImages(), tempImage->GetImageCount(), tempImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
//				}
//			case SAVE_FORMAT_BMP:
//				{
//					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), blob );
//				}
//			case SAVE_FORMAT_JPG:
//				{
//					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), blob );
//				}
//			case SAVE_FORMAT_PNG:
//				{
//					saveRes = DirectX::SaveToWICMemory(tempImage->GetImages(), tempImage->GetImageCount(),DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), blob );
//				}
//			}
//
//			size = static_cast< GpuApi::Uint32 >( blob.GetBufferSize() );
//
//			*buffer = GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, size, 16 );
//			Red::System::MemoryCopy(*buffer, blob.GetBufferPointer(), size);
//
//			blob.Release();
//			tempImage->Release();
//		}
//		else
//		{
//			GPUAPI_HALT( TXT( "Texture saving failed" ) );
//		}
//		scratchImage->Release();
//#endif
//		// Done
//		return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
	Bool CompressBC6HBC7( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage )
	{
		//D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
		//GetDevice()->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
		//if ( !hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
		//{
		//	GPUAPI_WARNING( TXT( "Sorry your driver and/or video card doesn't support DirectCompute 4.x" ) );
		//	return false;
		//}

		//EncoderBase* encoder = NULL;
		//if ( compressedImage.format == TEXFMT_BC6H )
		//{
		//	encoder = new CGPUBC6HEncoder();
		//}
		//else if ( compressedImage.format == TEXFMT_BC7 )
		//{
		//	encoder = new CGPUBC7Encoder();
		//}
		//else
		//{
		//	GPUAPI_HALT( TXT( "Should never get here." ) );
		//	return false;
		//}

		//GPUAPI_ASSERT( encoder );
		//encoder->Initialize( GetDevice(), GetDeviceContext() );

		//DirectX::Image image;
		//image.format		= Map( srcImage.format );
		//image.width			= srcImage.width;
		//image.height		= srcImage.height;
		//image.pixels		= *const_cast<Uint8**>( srcImage.data );
		//image.rowPitch		= srcImage.rowPitch;
		//image.slicePitch	= srcImage.slicePitch;

		//DirectX::TexMetadata metadata;
		//metadata.arraySize	= 1;
		//metadata.depth		= 1;
		//metadata.dimension	= DirectX::TEX_DIMENSION_TEXTURE2D;
		//metadata.format		= Map( srcImage.format );
		//metadata.height		= srcImage.height;
		//metadata.width		= srcImage.width;
		//metadata.mipLevels	= 1;
		//metadata.miscFlags	= 0;

		//ID3D11Texture2D* sourceTexture = NULL;
		//HRESULT hr = DirectX::CreateTexture( GetDevice(), &image, 1, metadata, (ID3D11Resource**)&sourceTexture );
		//GPUAPI_ASSERT( SUCCEEDED( hr ) );

		//DirectX::Image* cImage = new DirectX::Image();
		//hr = encoder->GPU_EncodeAndReturn( sourceTexture, Map( compressedImage.format ), cImage );
		//if ( SUCCEEDED( hr ) )
		//{
		//	Red::System::MemoryCopy( *compressedImage.data, cImage->pixels, cImage->slicePitch );
		//	compressedImage.width = cImage->width;
		//	compressedImage.height = cImage->height;
		//	compressedImage.rowPitch = cImage->rowPitch;
		//	compressedImage.slicePitch = cImage->slicePitch;
		//}

		//// cleanup
		//free( cImage->pixels ); // memory was allocated inside GPU_EncodeAndReturn
		//delete cImage;
		//cImage = NULL;
		//delete encoder;
		//encoder = NULL;

		//return SUCCEEDED( hr );

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}
#endif

	Bool CompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage )
	{
//#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
//		if ( compressedImage.format == TEXFMT_BC6H || compressedImage.format == TEXFMT_BC7 )
//		{
//			if ( CompressBC6HBC7( srcImage, compressedImage ) )
//			{
//				return true;
//			}
//		}
//#endif
//#ifndef NO_TEXTURE_IMPORT
//		// feed the structure with source data
//		DirectX::Image image;
//		image.format = Map( srcImage.format );
//		image.width = srcImage.width;
//		image.height = srcImage.height;
//		image.pixels = *const_cast<Uint8**>( srcImage.data );
//		image.rowPitch = srcImage.rowPitch;
//		image.slicePitch = srcImage.slicePitch;
//
//		// preform compression
//		DirectX::ScratchImage tempImage;
//		// TODO: integrate OpenMP and use DirectX::TEX_COMPRESS_PARALLEL
//		HRESULT hr = DirectX::Compress( image, Map( compressedImage.format ), DirectX::TEX_COMPRESS_DEFAULT, 1.0f, tempImage );
//		if ( FAILED( hr ) )
//		{
//			GPUAPI_WARNING( TXT( "Unable to compress image" ) );
//			return false;
//		}
//
//		if ( compressedImage.data == nullptr )
//		{
//			GPUAPI_HALT( TXT( "No data in compressed image, compression failed." ) );
//			return false;
//		}
//
//		const DirectX::Image* img = tempImage.GetImage(0, 0, 0);
//		compressedImage.width = img->width;
//		compressedImage.height = img->height;
//		compressedImage.rowPitch = img->rowPitch;
//		compressedImage.slicePitch = img->slicePitch;
//		compressedImage.format = Map( img->format );
//
//		if ( !*compressedImage.data )
//		{
//			*compressedImage.data = (GpuApi::Uint8*)GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, img->slicePitch, 16 );
//		}
//
//		Red::System::MemoryCopy( *compressedImage.data, img->pixels, img->slicePitch );
//#endif
//		return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	Bool DecompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& decompressedImage )
	{
//#ifndef NO_TEXTURE_IMPORT
//		DirectX::Image image;
//		image.format		= Map( srcImage.format );
//		image.width			= srcImage.width;
//		image.height		= srcImage.height;
//		image.pixels		= *const_cast<Uint8**>( srcImage.data );
//		image.rowPitch		= srcImage.rowPitch;
//		image.slicePitch	= srcImage.slicePitch;
//
//		DirectX::ScratchImage tempImage;
//		HRESULT hr = DirectX::Decompress( image, Map( decompressedImage.format ), tempImage );
//		if ( FAILED( hr ) )
//		{
//			GPUAPI_WARNING( TXT( "Unable to compress image" ) );
//			return false;
//		}
//
//		const DirectX::Image* img		= tempImage.GetImage(0, 0, 0);
//		decompressedImage.width			= img->width;
//		decompressedImage.height		= img->height;
//		decompressedImage.format		= Map( img->format );
//		decompressedImage.rowPitch		= img->rowPitch;
//		decompressedImage.slicePitch	= img->slicePitch;
//		
//		if ( !*decompressedImage.data )
//		{
//			 *decompressedImage.data = (GpuApi::Uint8*)GPU_API_ALLOCATE( GpuMemoryPool_Textures, MC_TextureData, img->slicePitch, 16 );
//		}
//		
//		Red::System::MemoryCopy( *decompressedImage.data, img->pixels, img->slicePitch );
//#endif
//		return true;

		GPUAPI_HALT("NOT IMPLEMENTED");
		return false;
	}

	TextureRef GetInternalTexture( eInternalTexture internalTexture )
	{
		if( INTERTEX_Max == internalTexture )
		{
			GPUAPI_HALT( "internal texture index invalid" );
			return TextureRef::Null();
		}
		const TextureRef &ref = GetDeviceData().m_InternalTextures[internalTexture];
		GPUAPI_ASSERT( ref, TXT( "Internal texture reference is invalid" ) );
		return ref;
	}

	void InitInternalTextures( bool assumeRefsPresent )
	{	
		SDeviceData &dd = GetDeviceData();

		// Pre check
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			bool isPresent = !dd.m_InternalTextures[i].isNull();
			GPUAPI_ASSERT( isPresent == assumeRefsPresent );
		}

		// Create resources
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			if ( !dd.m_InternalTextures[i] )
			{
				dd.m_InternalTextures[i] = TextureRef( dd.m_Textures.Create( 1 ) );
				GPUAPI_ASSERT( dd.m_InternalTextures[i] != NULL );
			}
		}

		//// Create internal textures
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Blank2D] ),			GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::BlankTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_FlatNormal2D] ),		GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::FlatNormalTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Default2D] ),		GPUAPI_DEFAULT2D_TEXTURE_SIZE,			Utils::DefaultTextureFill );
		Utils::InitInternalTextureDataCUBE( dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DefaultCUBE] ),		GPUAPI_DEFAULTCUBE_TEXTURE_SIZE,		Utils::DefaultCubeTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DissolvePattern] ),	GPUAPI_DISSOLVE_TEXTURE_SIZE,			Utils::GenerateDissolveTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_PoissonRotation] ),	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE,	Utils::GeneratePoissonRotationTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_SSAORotation] ),		GPUAPI_SSAO_ROTATION_TEXTURE_SIZE,		Utils::GenerateSSAORotationNoise );

		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Blank2D], "BlankTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_FlatNormal2D], "FlatTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Default2D], "DefaultTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DefaultCUBE], "DefaultCube" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DissolvePattern], "DissolvePattern" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_PoissonRotation], "PoissonRotation" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_SSAORotation], "SSAORotation" );

		//// Post check
		//for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		//{
		//	const STextureData &td = dd.m_Textures.Data( dd.m_InternalTextures[i] );
		//	GPUAPI_ASSERT( dd.m_InternalTextures[i] && "Not all internal texture were created!" );
		//	GPUAPI_ASSERT( NULL != td.m_pTexture );
		//	//GPUAPI_ASSERT( (NULL != td.m_pSurface) == (TEXTYPE_2D == td.m_Desc.type) );
		//}
	}

	void ShutInternalTextures( bool dropRefs )
	{
		SDeviceData &dd = GetDeviceData();

		// Release d3d resources

		GPUAPI_HALT("NOT IMPLEMENTED");

		//for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		//{
		//	TextureRef ref = dd.m_InternalTextures[i];
		//	if ( !ref )
		//	{
		//		continue;
		//	}

		//	STextureData &data = dd.m_Textures.Data( ref );
		//	//if ( data.m_pSurface )
		//	//{
		//	//	data.m_pSurface->Release();
		//	//	data.m_pSurface = NULL;
		//	//}
		//	if ( data.m_pTexture )
		//	{
		//		data.m_pTexture->Release();
		//		data.m_pTexture = NULL;
		//	}
		//}

		// Drop resources

		if ( dropRefs )
		{
			for ( Uint32 i=0; i<INTERTEX_Max; ++i )
			{
				SafeRelease( dd.m_InternalTextures[i] );
			}
		}
	}

	//dex++: dynamic texture debug code
	Uint32 GetNumDynamicTextures()
	{
		return GetDeviceData().m_NumDynamicTextures;
	}

	const char* GetDynamicTextureName( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Name;
		}
		else
		{
			return NULL;
		}
	}

	TextureRef GetDynamicTextureRef( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Texture;
		}
		else
		{
			return TextureRef::Null();
		}
	}

	void AddDynamicTexture( TextureRef tex, const char* name )
	{
		if ( GetDeviceData().m_NumDynamicTextures < ARRAY_COUNT( GetDeviceData().m_DynamicTextures ) )
		{
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Name = name;
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Texture = tex;
			GetDeviceData().m_NumDynamicTextures += 1;
		}
	}

	void RemoveDynamicTexture( TextureRef tex )
	{
		for ( Uint32 i=0; i<GetDeviceData().m_NumDynamicTextures; ++i )
		{
			if ( GetDeviceData().m_DynamicTextures[ i ].m_Texture == tex )
			{
				// copy rest of the textures
				for ( Uint32 j=i+1; j<GetDeviceData().m_NumDynamicTextures; ++j )
				{
					GetDeviceData().m_DynamicTextures[j-1] = GetDeviceData().m_DynamicTextures[j];
				}
				
				// remove from list
				GetDeviceData().m_NumDynamicTextures -= 1;
				break;
			}
		}
	}
	//dex--
}
