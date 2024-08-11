/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/architecture.h"

RED_DISABLE_WARNING_MSC( 4005 )

#define D3D_OVERLOADS 1
#include "DirectXMath.h"
#define XMFLOAT2 DirectX::XMFLOAT2
#define XMFLOAT3 DirectX::XMFLOAT3
#define XMFLOAT4 DirectX::XMFLOAT4

#ifndef RED_PLATFORM_CONSOLE

#include "../../../external/DirectXTex/DirectXTex/DirectXTex.h"

#ifdef _WIN64
	#if defined( _DEBUG )
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/x64/Debug/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/x64/Debug/BC6HBC7EncoderCS.lib" )
		#endif
	#elif defined( RELEASE )
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/x64/Release/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/x64/Release/BC6HBC7EncoderCS.lib" )
		#endif
	#else
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/x64/Profile/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/x64/Profile/BC6HBC7EncoderCS.lib" ) 	
		#endif
	#endif
#else
	#if defined( _DEBUG )
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/Debug/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/Win32/Debug/BC6HBC7EncoderCS.lib" )
		#endif
	#elif defined( RELEASE )
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/Release/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/Win32/Release/BC6HBC7EncoderCS.lib" )
		#endif
	#else
		#pragma comment (lib,  "../../../external/DirectXTex/DirectXTex/Profile/DirectXTex.lib")
		#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
			#pragma comment (lib,  "../../../external/BC6HBC7 DirectCompute Encoder Tool/lib/Win32/Profile/BC6HBC7EncoderCS.lib" )
		#endif
	#endif
#endif

#endif

#if defined( RED_PLATFORM_WINPC )
#	include "d3d11_1.h"
#	include "d3dcompiler.h"
#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "d3dcompiler.lib")
#	pragma comment (lib, "DXGI.lib")			// For enumerating adapters
//#	define GPU_API_DEBUG_PATH
#	ifdef _DEBUG
#		include "Initguid.h"
#		include "DXGIDebug.h"
#	endif
#elif defined( RED_PLATFORM_DURANGO )
#	include "d3d11_x.h"
#	include "d3dcompiler_x.h"
#	include "xg.h"
#	pragma comment (lib, "d3d11_x.lib")
#	pragma comment (lib, "d3dcompiler.lib")
#	pragma comment (lib, "xg_x.lib")
#endif

#pragma comment (lib, "dxguid.lib")

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11PixelShader;
struct ID3D11ShaderResourceView;

#define D3D_RENDER_TARGET_VIEW_DESC D3D11_RENDER_TARGET_VIEW_DESC
#define D3D_DEPTH_STENCIL_VIEW_DESC D3D11_DEPTH_STENCIL_VIEW_DESC
#define D3D_RTV_DIMENSION_TEXTURE2D D3D11_RTV_DIMENSION_TEXTURE2D
#define D3D_RTV_DIMENSION_TEXTURE2DMS D3D11_RTV_DIMENSION_TEXTURE2DMS
#define D3D_RTV_DIMENSION_TEXTURE2DARRAY D3D11_RTV_DIMENSION_TEXTURE2DARRAY
#define D3D_DSV_DIMENSION_TEXTURE2D D3D11_DSV_DIMENSION_TEXTURE2D
#define D3D_DSV_DIMENSION_TEXTURE2DMS D3D11_DSV_DIMENSION_TEXTURE2DMS
#define D3D_DSV_DIMENSION_TEXTURE2DARRAY D3D11_DSV_DIMENSION_TEXTURE2DARRAY
#define D3D_SHADER_RESOURCE_VIEW_DESC D3D11_SHADER_RESOURCE_VIEW_DESC
#define D3D_SRV_DIMENSION_TEXTURE2D D3D11_SRV_DIMENSION_TEXTURE2D
#define D3D_SRV_DIMENSION_TEXTURE2DMS D3D11_SRV_DIMENSION_TEXTURE2DMS
#define D3D_SRV_DIMENSION_TEXTURECUBEARRAY D3D11_SRV_DIMENSION_TEXTURECUBEARRAY
#define D3D_SRV_DIMENSION_TEXTURECUBE D3D11_SRV_DIMENSION_TEXTURECUBE
#define D3D_SRV_DIMENSION_TEXTURE2DARRAY D3D11_SRV_DIMENSION_TEXTURE2DARRAY

#define ID3DResource ID3D11Resource
#define ID3DRenderTargetView ID3D11RenderTargetView
#define ID3DDepthStencilView ID3D11DepthStencilView
#define ID3DShaderResourceView ID3D11ShaderResourceView

namespace GpuApi
{
	namespace Hacks
	{
		ID3D11RenderTargetView*		GetTextureRTV( const TextureRef& ref );
		ID3D11ShaderResourceView*	GetTextureSRV( const TextureRef& ref );
		ID3D11UnorderedAccessView*	GetTextureUAV( const TextureRef& ref );
		ID3D11Texture2D*			GetTexture( const TextureRef& ref );
		ID3D11Buffer*				GetBuffer( const BufferRef& ref );
		ID3D11PixelShader*			GetPixelShader( const ShaderRef& ref );
		ID3D11ComputeShader*		GetComputeShader( const ShaderRef& ref );
		void						UpdateConstantBuffers();

#ifdef RED_PLATFORM_DURANGO
		ID3D11ComputeContextX*		GetComputeContext();
		ID3D11DeviceX*				GetDevice();
		ID3D11DeviceContextX*		GetDeviceContext();
#else
		ID3D11Device*				GetDevice();
		ID3D11DeviceContext*		GetDeviceContext();
#endif
	}
}