	///////////////////////////////////////////////////////////////////////
	//  InlinePixel_DeferredOutput.fx
	//
	//	Designed to be included at the end of SpeedTree's deferred pixel shaders,
	//	this code excerpt will handle the per-platform multi-render target output.
	//	It excepts that these values have been declared and assigned prior to
	//	being invoked:
	//
	//		- float3 out_vAlbedo: diffuse color, in our example this also contains
	//							  ambient occlusion
	//
	//		- float out_fAlpha: alpha value of the pixel
	//
	//		- float3 out_vNormal: surface normal (in world space in our example)
	//
	//		- float out_fSpecularMask: used to mask specular lighting, 0.0 = masked, 1.0 = pass
	//
	//		- float out_fSpecularPower: directly related to the specular shininess value as
	//							        defined in the Modeler app, but with (shininess* 126 + 1)
	//									applied
	//
	//		- float out_fTransmissionMask: mask used for out approximate transmission/sub-surface-
	//									   scattering approach, 0.0 = masked, 1.0 = pass

	{
		#if (ST_OPENGL)

			// render target 0
			output_FragmentColor[0].rgb = out_vAlbedo;
			output_FragmentColor[0].a = out_fSpecularPower / 128.0;

			// render target 1
			output_FragmentColor[1].rg = PackNormalIntoFloat2_Stereographic(out_vNormal);
			output_FragmentColor[1].b = out_fSpecularMask;
			output_FragmentColor[1].a = out_fTransmissionMask;

		#elif (!ST_XBOX_360)

// LAVA++
			// render target 0
			sOutput.m_vRenderTarget0.rgb 	= out_vAlbedo.xyz;
       		sOutput.m_vRenderTarget0.a 		= out_fTransmissionMask;

			// render target 1
            sOutput.m_vRenderTarget1.rgb 	= out_vNormal;
			sOutput.m_vRenderTarget1.a 		= out_fSpecularPower / 128.0;
			
            sOutput.m_vRenderTarget2.rgb 	= out_fSpecularMask;
			sOutput.m_vRenderTarget2.a 		= out_encodedGBuffMaterialMask;
			
			//LAVA++
			/*
			//LAVA--
			sOutput.m_Coverage = out_Coverage;
			//LAVA++
			*/
			//LAVA--
			#if (ST_DEFERRED_A2C_ENABLED)
//ACE_TODO: add support for this !!!
				sOutput.m_vNullRenderTarget.rgba = float4(0.0, 0.0, 0.0, out_fAlpha);
			#endif

// LAVA--
			return sOutput;

		#else // Xbox 360

			// render target 0
			sOutput.m_vRenderTargets[0].rgb = out_vAlbedo;
			sOutput.m_vRenderTargets[0].a = out_fSpecularPower / 128.0;

			// render target 1
			sOutput.m_vRenderTargets[1].rg = PackNormalIntoFloat2_Stereographic(out_vNormal);
			sOutput.m_vRenderTargets[1].b = out_fSpecularMask;
			sOutput.m_vRenderTargets[1].a = out_fTransmissionMask;
			
			return sOutput;

		#endif
	}
