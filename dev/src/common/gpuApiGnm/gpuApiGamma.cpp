/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

//#include "stdio.h"

namespace GpuApi
{
	//--------------------------------------------------------------------------------------
	// Helper functions.
	//--------------------------------------------------------------------------------------
	//Convert to/from different gamma spaces.
	float GammasRGB(float v)
	{
		//if( v <= 0.0031308f )
		//{
		//	return v * 12.92f;
		//}
		//else
		//{
		//	return powf( v, 1.0f / 2.4f ) * 1.055f - 0.055f;
		//}
		return 0.f;
	}

	float DegammasRGB(float v)
	{
		//if( v <= 0.04045f )
		//{
		//	return v / 12.92f;
		//}
		//else
		//{
		//	return powf( ( v + 0.055f ) / 1.055f, 2.4f );
		//}
		return 0.f;
	}

	float GammaTV(float v)
	{
		//if( v <= 0.018f )
		//{
		//	return v * 4.5f;}
		//else
		//{
		//	return powf( v, 0.45f ) * 1.099f - 0.099f;
		//}
		return 0.f;
	}

	float DegammaTV(float v)
	{
		//if( v <= 0.0812f )
		//{
		//	return v / 4.5f;}
		//else
		//{
		//	return powf( ( v + 0.099f ) / 1.099f, 1.0f / 0.45f );
		//}
		return 0.f;
	}

	//template<typename t_type>
	//t_type Min( t_type a, t_type b ) { return a < b ? a : b; }
	//template<typename t_type>
	//t_type Max( t_type a, t_type b ) { return a > b ? a : b; }
	//template<typename t_type>
	//t_type Saturate( t_type a ) { return Min( 1.0f, Max( 0.0f, a ) ); }

	//template<typename t_type, UINT t_N>
	//t_type ConvertFloatToNBits( float f ) { return (t_type) ( ( ( 1 << t_N ) - 1 ) * Saturate( f ) + 0.5f ); }
	//template<typename t_type, UINT t_N>
	//float ConvertNBitsToFloat( t_type i ) { return ( (float) i ) / ( ( 1 << t_N ) - 1 ); }


	//WORD Convert8BitLinearTo10BitsRGB(BYTE v, float g)
	//{
	//	float f = ConvertNBitsToFloat<BYTE, 8>(v);
	//	f = GammasRGB(powf(f,g));
	//	return ConvertFloatToNBits<WORD, 10>(f);
	//}

	//WORD Convert8BitLinearTo10BitsRGBNoTVGamma(BYTE v, float g)
	//{
	//	float f = ConvertNBitsToFloat<BYTE, 8>(v);
	//	f = GammasRGB(DegammaTV(GammasRGB(powf(f,g))));
	//	return ConvertFloatToNBits<WORD, 10>(f);
	//}

	//WORD Convert8BitsRGBTo10BitsRGB(BYTE v, float g)
	//{
	//	float f = ConvertNBitsToFloat<BYTE, 8>(v);
	//	return ConvertFloatToNBits<WORD, 10>(powf(f,g));
	//}

	//WORD Convert8BitsRGBTo10BitsNoTVGamma(BYTE v, float g)
	//{
	//	float f = ConvertNBitsToFloat<BYTE, 8>(v);
	//	f = GammasRGB(DegammaTV(powf(f,g)));
	//	return ConvertFloatToNBits<WORD, 10>(f);
	//}

	//WORD (*pt2Func[])(BYTE,float)=
	//{
	//	Convert8BitLinearTo10BitsRGB,
	//	Convert8BitLinearTo10BitsRGBNoTVGamma,
	//	Convert8BitsRGBTo10BitsRGB,
	//	Convert8BitsRGBTo10BitsNoTVGamma
	//};

	void SetupDeviceGammaMode(eGammaRampType gammaType, float gamma )
	{
		//HACK DX10 this needs the proper swapchain which is in the viewport now so it needs redesign

		//IDXGIOutput* adapter;
		//GetDeviceData().m_swapChain->GetContainingOutput(&adapter);

		//DXGI_GAMMA_CONTROL gammaControl;
		//
		//gammaControl.Scale.Blue = gammaControl.Scale.Green = gammaControl.Scale.Red = 0.f;
		//gammaControl.Offset.Blue = gammaControl.Offset.Green = gammaControl.Offset.Red = 0.f;

		//for(unsigned int i=0; i<1025; i++)
		//{
		//	gammaControl.GammaCurve[i].Blue = gammaControl.GammaCurve[i].Green = gammaControl.GammaCurve[i].Red = powf(i/1025.f, gamma);//(*pt2Func[gammaType])(i, gamma) << 6;
		//}	

		//adapter->SetGammaControl(&gammaControl);
	}
}