/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "renderTerrainShadows.h"
#include "renderShadowManager.h"
#include "renderTextureArray.h"
#include "renderProxyPointLight.h"
#include "renderProxySpotLight.h"
#include "renderProxyDimmer.h"
#include "renderCollector.h"
#include "renderPostProcess.h"
#include "renderPostFx.h"
#include "renderShaderPair.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbe.h"
#include "renderEnvProbeManager.h"
#include "../engine/dimmerComponent.h"
#include "../engine/lightComponent.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

#define TILE_SIZE						16
#define TILE_SIZE_INTERIOR_FULLRES		(TILE_SIZE * WEATHER_VOLUMES_SIZE_DIV)

namespace Config
{
	TConfigVar< Bool > cvEnableDeferredLightsStencil( "Rendering", "EnableDeferredLightsStencil", true );
}

struct DimmerParams
{
	Vector viewToLocal0;
	Vector viewToLocal1;
	Vector viewToLocal2;
	Float normalScaleX;	
	Float normalScaleY;	
	Float normalScaleZ;
	Uint32 fadeAlphaAndInsideMarkFactor;
	Float outsideMarkFactor;
	Float insideAmbientLevel;
	Float outsideAmbientLevel;
	Float marginFactor;

	static Float PackInsideAmbientLevel( Float ambientLevel )
	{
		return Clamp( 1 - ambientLevel, 0.0001f, 1.f );
	}

	static Uint32 PackInsideAmbientLevelExplicitValue( Float fadeAlpha, Float insideValue )
	{
		Uint32 value = 0;
		value += (Uint32)(0xffff * Clamp( fadeAlpha, 0.f, 1.f ));
		value <<= 16;
		value += (Uint32)(0xffff * Clamp( insideValue, 0.f, 1.f ));
		return value;
	}

	static Uint32 PackInsideAmbientLevel( Float fadeAlpha, Float ambientLevel )
	{
		return PackInsideAmbientLevelExplicitValue( fadeAlpha, PackInsideAmbientLevel( ambientLevel ) );
	}

	void Pack( const CRenderFrameInfo &info, const Matrix &viewToWorld, const CRenderProxy_Dimmer& proxy )
	{
		const Matrix worldToLocal = proxy.GetLocalToWorld().FullInverted(); // ace_optimize: full inverted to handle scaling 
		const Matrix localToWorld = proxy.GetLocalToWorld();
		const Matrix viewToLocal = viewToWorld * worldToLocal;
		const Vector normalScale = localToWorld.GetScale33() / worldToLocal.GetPreScale33();

		viewToLocal0 = viewToLocal.GetColumn( 0 );
		viewToLocal1 = viewToLocal.GetColumn( 1 );
		viewToLocal2 = viewToLocal.GetColumn( 2 );

		normalScaleX = normalScale.X;
		normalScaleY = normalScale.Y;
		normalScaleZ = normalScale.Z;

		Float ambientLevel = proxy.GetAmbientLevel();
		if ( proxy.IsMarker() && DIMMERTYPE_InsideArea == proxy.GetDimmerType() && -1 == ambientLevel )
		{
			ambientLevel = info.m_worldRenderSettings.m_interiorDimmerAmbientLevel;
		}

		const Float fadeAlpha = proxy.GetFadeAlphaCombined();

		const Float clampedAmbientLevel = Clamp( Lerp( fadeAlpha, 1.f, ambientLevel ), 0.f, 1.f );

		Bool isInsideMarker = proxy.IsMarker() && DIMMERTYPE_InsideArea == proxy.GetDimmerType();
		fadeAlphaAndInsideMarkFactor = isInsideMarker ? PackInsideAmbientLevel( fadeAlpha, clampedAmbientLevel ) : PackInsideAmbientLevelExplicitValue( fadeAlpha, 0 );
		outsideMarkFactor = proxy.IsMarker() && DIMMERTYPE_OutsideArea ==  proxy.GetDimmerType() ? fadeAlpha / Max( 0.0001f, proxy.GetMarginFactor() ) : 0.f;
		insideAmbientLevel  = 1.f - (DIMMERTYPE_OutsideArea != proxy.GetDimmerType() && !isInsideMarker ? clampedAmbientLevel : 1.f);
		outsideAmbientLevel = 1.f - (DIMMERTYPE_InsideArea != proxy.GetDimmerType() && 0 == outsideMarkFactor ? clampedAmbientLevel : 1.f);
		marginFactor = proxy.GetMarginFactor() >= 0.f ? 1.f / Max( 0.0001f, proxy.GetMarginFactor() ) : -1;
	}
};

struct LightParams
{
	Vector positionAndRadius;
	Vector positionAndRadiusCull;
	Vector direction;
	Vector colorAndType;
	Vector params; // in case of spot, spot params
	Vector params2;
	Vector staticShadowmapRegion;
	Uint32 dynamicShadowmapRegions[6];
	Float shadowProjection[2];
	
	Float PackTypeAndMask( Uint32 lightUsageMask, const Bool spotFlag, const Bool cameraLightFlag )
	{
		if ( (LUM_IsInteriorOnly & lightUsageMask) == (LUM_IsExteriorOnly & lightUsageMask) )
		{
			lightUsageMask |= LUM_IsInteriorOnly | LUM_IsExteriorOnly;
		}

		lightUsageMask <<= 1;
		COMPILE_ASSERT( SHADER_LIGHT_USAGE_MASK_INTERIOR == (LUM_IsInteriorOnly << 1) );
		COMPILE_ASSERT( SHADER_LIGHT_USAGE_MASK_EXTERIOR == (LUM_IsExteriorOnly << 1) );
		
		if ( spotFlag )
		{
			lightUsageMask |= ( 1 << 0 ); // Setting first bit to ONE - spot light flag
		}
		else
		{
			lightUsageMask &= ~( 1 << 0 ); // Setting first bit to ZERO - point light
		}

		RED_ASSERT( !(lightUsageMask & SHADER_LIGHT_USAGE_MASK_CAMERA_LIGHT) );
		lightUsageMask |= (cameraLightFlag ? SHADER_LIGHT_USAGE_MASK_CAMERA_LIGHT : 0);
		
		Float lightUsageMaskAsFloat = 0;
		COMPILE_ASSERT( sizeof(lightUsageMaskAsFloat) == sizeof(lightUsageMask) );
		lightUsageMaskAsFloat = reinterpret_cast< Float& >( lightUsageMask );
	
		return lightUsageMaskAsFloat;
	}

	Bool UnpackTypeIsSpot() const
	{
		return 0 != (reinterpret_cast< const Uint32& >( colorAndType.W ) & 1);
	}

	Float PackAttenuationCustomValue( Float attenuation )
	{
		float att = Clamp( attenuation, 0.f, 1.f );
		return att * att;
	}

	Float PackAttenuation( const class IRenderProxyLight& proxy )
	{
		return PackAttenuationCustomValue( proxy.GetAttenuation() );
	}

	// helper for packing camera light
	Bool PackCameraLight( const CRenderCamera &camera, const SCameraLightModifiers &lightModifier, const CEnvCameraLightParametersAtPoint &lightParams, const Vector &interiorLightColor, Float interiorAmount, Vector *outInOutPatchColors )
	{
		const Float radius = lightParams.GetRadius( lightModifier );
		if ( radius <= 0 )
		{
			return false;
		}

		Vector finalColor ( 0, 0, 0, 1 );
		if ( outInOutPatchColors )
		{
			const Vector finalColor0 = lightParams.GetColor( lightModifier, interiorLightColor, 0 );
			const Vector finalColor1 = lightParams.GetColor( lightModifier, interiorLightColor, 1 );
			if ( 0 == finalColor0.X && 0 == finalColor0.Y && 0 == finalColor0.Z && 0 == finalColor1.X && 0 == finalColor1.Y && 0 == finalColor1.Z )
			{
				return false;
			}

			outInOutPatchColors[0] = finalColor0;
			outInOutPatchColors[1] = finalColor1;

			finalColor = Vector ( 0, 0, 0, 1 );
		}
		else
		{
			finalColor = lightParams.GetColor( lightModifier, interiorLightColor, interiorAmount );
			if ( 0 == finalColor.X && 0 == finalColor.Y && 0 == finalColor.Z )
			{
				return false;
			}
		}

		Vector lightPos;
		{
			Vector cameraFwd, cameraUp, cameraRight;
			CRenderCamera::CalcCameraVectors( EulerAngles ( 0, 0, camera.GetRotation().Yaw ), cameraFwd, cameraRight, cameraUp );
			Vector offset = lightParams.GetOffsetForwardRightUp( lightModifier );
			lightPos = camera.GetPosition() + cameraFwd * offset.X + cameraRight * offset.Y + cameraUp * offset.Z;
		}

		positionAndRadius.Set4( lightPos.X, lightPos.Y, lightPos.Z, radius );
		positionAndRadiusCull = positionAndRadius;
		colorAndType.Set4( finalColor.X, finalColor.Y, finalColor.Z, PackTypeAndMask( 0, false, true ));
		params = Vector::ZEROS;
		params2.Set4( PackAttenuationCustomValue( lightParams.GetAttenuation( lightModifier ) ), 0, 0, 0 );

		// dynamic shadows
		memset( dynamicShadowmapRegions, 0, sizeof(dynamicShadowmapRegions) );
		shadowProjection[0] = 0;
		shadowProjection[1] = 0;

		// optional static shadows
		staticShadowmapRegion.Set4( 0.0f, 0.0f, 0.0f, 0.0f );

		//
		direction.Set4( 0.0f, 0.0f, 0.0f, 0.0f );		

		return true;
	}

	Bool UnpackIsCastingShadow() const
	{
		return staticShadowmapRegion.X > 0 || params2.Z > 0;
	}

	Bool UnpackIsCameraLight() const
	{
		return 0 != (reinterpret_cast< const Uint32& >( colorAndType.W ) & SHADER_LIGHT_USAGE_MASK_CAMERA_LIGHT);
	}
	
	static Uint32 BuildShadowmapRegion( const CRenderShadowDynamicRegion* region )
	{
		if ( !region || !region->GetSize() )
		{
			return 0;
		}

		RED_ASSERT( region->GetOffsetX() < 1024 );
		RED_ASSERT( region->GetOffsetY() < 1024 );
		RED_ASSERT( region->GetSize() < 1024 );
		RED_ASSERT( region->GetSlice() < 2 );

		Uint32 packedRegion = 0;
		packedRegion = region->GetSlice();
		packedRegion = region->GetOffsetX() + (packedRegion << 10);
		packedRegion = region->GetOffsetY() + (packedRegion << 10);
		packedRegion = region->GetSize() + (packedRegion << 10);

		RED_ASSERT( 0 != packedRegion );
		return packedRegion;
	}

	static void BuildPackedShadowProjection( Float outPackedProjection[2], Float degAngle, Float nearPlane, Float farPlane )
	{
		Matrix shadowProjectionMatrix;
		shadowProjectionMatrix.BuildPerspectiveLH( DEG2RAD(degAngle), 1.f, nearPlane, farPlane );

		outPackedProjection[0] = shadowProjectionMatrix.V[2].Z;
		outPackedProjection[1] = shadowProjectionMatrix.V[3].Z;
	}

	// helper packing for point light
	void PackPoint( const Vector& finalPos, const Vector& finalColor, Bool allowShadows, const class CRenderProxy_PointLight& proxy )
	{
		positionAndRadius.Set4( finalPos.X, finalPos.Y, finalPos.Z, proxy.GetRadius() );
		positionAndRadiusCull = positionAndRadius;
		colorAndType.Set4( finalColor.X, finalColor.Y, finalColor.Z, PackTypeAndMask( proxy.GetLightUsageMask(), false, false ));
		params = Vector::ZEROS;
		params2.Set4( PackAttenuation( proxy ), 0, 0, 0 );

		if ( allowShadows && (proxy.GetCurrentShadowResolution() > 0) )
		{
			BuildPackedShadowProjection( shadowProjection, 90.f, 0.05f, proxy.GetRadius() );

			params2.Y = 1.f;

			Bool hasShadowedSide = false;
			for ( Uint32 side_i=0; side_i<6; ++side_i )
			{
				Uint32 packedRegion = BuildShadowmapRegion( proxy.GetDynamicShadowRegion( side_i ) );
				dynamicShadowmapRegions[side_i] = packedRegion;
				hasShadowedSide = hasShadowedSide || 0!=packedRegion;
			}

			params2.Z = hasShadowedSide ? 1.f : 0.f;
		}
		else
		{	
			memset( dynamicShadowmapRegions, 0, sizeof(dynamicShadowmapRegions) );
			shadowProjection[0] = 0;
			shadowProjection[1] = 0;
		}

		// optional static shadows
		if ( proxy.GetStaticShadowCube() != NULL )
		{
			staticShadowmapRegion.Set4( 1.0f, (Float)proxy.GetStaticShadowCube()->GetIndex(), 0.0f, 0.0f );
		}
		else
		{
			staticShadowmapRegion.Set4( 0.0f, 0.0f, 0.0f, 0.0f );
		}

		//
		direction.Set4( 0.0f, 0.0f, 0.0f, proxy.GetCurrentShadowAlpha() );
	}

	Float UnpackSpotOuterRadAngle() const
	{
		ASSERT( UnpackTypeIsSpot() );
		return acos( params.X );
	}

	// helper packing for spot light
	void PackSpot( const Vector& finalPos, const Vector& finalColor, const class CRenderProxy_SpotLight& proxy, Bool renderShadows )
	{
		positionAndRadius.Set4( finalPos.X, finalPos.Y, finalPos.Z, proxy.GetRadius() );
		direction = proxy.GetLocalToWorld().TransformVector( Vector(0,1,0)).Normalized3();
		colorAndType.Set4( finalColor.X, finalColor.Y, finalColor.Z, PackTypeAndMask( proxy.GetLightUsageMask(), true, false ));
		staticShadowmapRegion.Set4( 0.0f, 0.0f, 0.0f, 0.0f );
		params2.Set4( PackAttenuation( proxy ), 0, 0, 0 );

		// do not test distance to point light center
		Vector testingPoint = finalPos + (direction * proxy.GetRadius() * 0.5f );

		//tempshit, math bullshit, especially not correct if angle gets --> 180
		{
			const Vector pointOne = finalPos;
			const Vector pointTwo = finalPos + (direction * proxy.GetRadius());

			Vector origin = (pointOne + pointTwo)*0.5f;
			origin.W = origin.DistanceTo( pointOne );

			Vector origin2 = finalPos + (direction * proxy.GetRadius() * 1.1f );
			origin2.W = proxy.GetRadius() * 1.3f;

			positionAndRadiusCull = Lerp<Vector>( proxy.m_outerAngle / 180.0f, origin, origin2 );

			// Get cone params
			const Float innerAngleCos = cos( 0.5f * DEG2RAD( proxy.m_innerAngle ) );
			const Float outerAngleCos = cos( 0.5f * DEG2RAD( proxy.m_outerAngle ) );
			params = Vector( outerAngleCos, 1.0f / ( innerAngleCos - outerAngleCos ), -outerAngleCos / ( innerAngleCos - outerAngleCos ), proxy.m_softness );
		}

		// does this spot have any shadows ?
		const CRenderShadowDynamicRegion* region = proxy.GetDynamicShadowsRegion();
		if ( renderShadows && (proxy.GetCurrentShadowResolution() > 0) && (region != NULL) )
		{
			BuildPackedShadowProjection( shadowProjection, proxy.m_outerAngle, 0.05f, proxy.GetRadius() );
			dynamicShadowmapRegions[0] = BuildShadowmapRegion( region );

			params2.Y = 1.f / MTan( 0.5f * DEG2RAD( proxy.m_outerAngle ) );
			params2.Z = 1;

			const EulerAngles cameraRotation = proxy.GetLocalToWorld().ToEulerAngles();
			Matrix mat;// = CRenderCamera::CalcWorldToTexture( finalPos, cameraRotation, proxy.m_outerAngle, 1.0f, 0.05f, proxy.GetRadius() );
			{
				// the magic axis conversion crap
				Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );

				// compute view space matrix
				Matrix rotMatrix = cameraRotation.ToMatrix().Transposed();
				Matrix transMatrix = Matrix::IDENTITY;
				Matrix worldToView = (transMatrix * rotMatrix) * axesConversion;

				mat = worldToView.Transposed();				
			}

			Vector v0 = mat.GetAxisX();
			Vector v1 = mat.GetAxisY();
			Vector v2 = mat.GetAxisZ();

			COMPILE_ASSERT( sizeof(Uint32) == sizeof(v0.X) );
			dynamicShadowmapRegions[1] = reinterpret_cast< Uint32& >( v1.X );
			dynamicShadowmapRegions[2] = reinterpret_cast< Uint32& >( v1.Y );
			dynamicShadowmapRegions[3] = reinterpret_cast< Uint32& >( v1.Z );

			dynamicShadowmapRegions[4] = 0;
			dynamicShadowmapRegions[5] = 0;
		}
		else
		{
			memset( dynamicShadowmapRegions, 0, sizeof(dynamicShadowmapRegions) );
			shadowProjection[0] = 0;
			shadowProjection[1] = 0;
		}

		// shadow fade
		direction.SetW( proxy.GetCurrentShadowAlpha() );
	}
};

struct ComputeConstantBuffer
{
	// dir light
	Vector lightDir;
	Vector lightColor;
	Vector lightSpecularColorAndShadowAmount;	// ace_todo: remove this (unused)
	Vector lightTweaks;

	// global shadows
	Matrix mShadowTransform;
	Vector vShadowOffsetsX;
	Vector vShadowOffsetsY; 
	Vector vShadowHalfSizes;
	Vector vShadowParams[ MAX_CASCADES ];
	Vector vShadowPoissonOffsetAndBias;
	Vector vShadowTextureSize;
	Vector vShadowFadeScales;
	Uint32 iShadowQuality; // 0-PCF 1-PCSS, etc
	Vector vSpeedTreeShadowParams[ MAX_CASCADES ];

	// ambient
	Vector ambientColorTop;						// ace_todo: remove this (unused)
	Vector ambientColorBottom;					// ace_todo: remove this (unused)
	Vector ambientColorFront;					// ace_todo: remove this (unused)
	Vector ambientColorBack;					// ace_todo: remove this (unused)
	Vector ambientColorSide;					// ace_todo: remove this (unused)

	// sky
	Vector _skyColor;
	Vector _skyColorHorizon;
	Vector _sunColorHorizon;
	Vector _sunBackHorizonColor;
	Vector _sunColorSky;
	
	//
	Vector interiorParams;
	Vector interiorRangeParams;
	
	// terrain shadows
	Uint32 iNumTerrainShadowRegions;
	Uint32 iNumTerrainTextureRegions;
	Vector vTerrainShadowsParams;
	Vector vTerrainShadowsParams2;
	Vector vTerrainShadowsParams3;
	Vector vTerrainShadowsWindows[5];
	Vector vTerrainTextureWindows[5];

	Vector histogramParams;		//< ace_todo: remove this. not used anymore (reject threshold is being set by custom cs constants)

	//
	Vector ambientShadowAmount;				//< ace_fix: remove

	Vector lightColorLightSide;				// ace_todo: move this near lightColor
	Vector lightColorLightOppositeSide;		// ace_todo: move this near lightColor
	Vector vShadowDepthRanges;

	Vector characterLightParams;
	Vector characterEyeBlicksColor;
	Vector fresnelGainParams;

	// KEEP AT THE END !

	// lights!
	Uint32 lightNum;
	LightParams lightParams[CRenderInterface::TILED_DEFERRED_LIGHTS_CAPACITY];

	// dimmers
	Uint32 dimmerNum;
	DimmerParams dimmerParams[CRenderInterface::TILED_DEFERRED_DIMMERS_CAPACITY];
};

Uint32 CRenderInterface::GetTileDeferredConstantsDataSize()
{
	return sizeof( ComputeConstantBuffer );
}

Bool CRenderInterface::HasTiledDeferredConstantsPatch() const
{
	return !m_cameraLightPatches.Empty(); 
}

void CRenderInterface::FlushTiledDeferredConstants( Bool allowPatch )
{
	if ( !m_computeConstantBuffer || !m_computeConstantShadowBuffer )
	{
		return;
	}

#ifndef RED_PLATFORM_ORBIS
	if ( !m_computeRawConstantBuffer )
	{
		return;
	}
#endif

	GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

	if ( m_cameraLightPatches.Empty() || !allowPatch )
	{
		void* pConstantData = GpuApi::LockBuffer( m_computeConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(ComputeConstantBuffer) );
		if ( !pConstantData )
		{
			return;
		}

		ASSERT( sizeof(ComputeConstantBuffer) == m_computeConstantShadowBuffer.GetSize() );
		Red::System::MemoryCopy(pConstantData, m_computeConstantShadowBuffer.Get(), m_computeConstantShadowBuffer.GetSize() );
		GpuApi::UnlockBuffer( m_computeConstantBuffer );
	}
	else
	{	
	#ifdef RED_PLATFORM_ORBIS
		GpuApi::BufferRef bufferToUpdate = m_computeConstantBuffer;
	#else
		GpuApi::BufferRef bufferToUpdate = m_computeRawConstantBuffer;
	#endif

		// Update raw constant buffer
		{
			void* pConstantData = GpuApi::LockBuffer( bufferToUpdate, GpuApi::BLF_Discard, 0, sizeof(ComputeConstantBuffer) );
			if ( !pConstantData )
			{
				return;
			}

			ASSERT( sizeof(ComputeConstantBuffer) == m_computeConstantShadowBuffer.GetSize() );
			Red::System::MemoryCopy(pConstantData, m_computeConstantShadowBuffer.Get(), m_computeConstantShadowBuffer.GetSize() );
			GpuApi::UnlockBuffer( bufferToUpdate );
		}

		// Patch camera lights
		for ( Uint32 patch_i=0; patch_i<m_cameraLightPatches.Size(); ++patch_i )
		{
			PC_SCOPE_RENDER_LVL1( PatchConstantBuffer );

			const SCameraLightPatch &currPatch = m_cameraLightPatches[patch_i];
		
			struct SConstantBuffer
			{
				Vector patchParams;
				Vector value0;
				Vector value1;
			};

			// bind params
			SConstantBuffer constBuffer;
			constBuffer.patchParams.Set4( (Float)currPatch.byteOffset, 0, 0, 0 );
			constBuffer.value0 = currPatch.color0;
			constBuffer.value1 = currPatch.color1;

			GpuApi::SetComputeShaderConsts( constBuffer );

			// bind textures and buffers
			const GpuApi::TextureRef texInteriorFactor = GetSurfaces()->GetRenderTargetTex( RTN_CameraInteriorFactor );
			BindBufferUAV( bufferToUpdate, 0 );
			GpuApi::BindTextures( 0, 1, &texInteriorFactor, GpuApi::ComputeShader );

			// perform patching
			if ( m_shaderPatchCameraInteriorFactor )
			{
				GetRenderer()->m_shaderPatchCameraInteriorFactor->Dispatch( 1, 1, 1 );
			}

			// unbind stuff
			GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
		}

		// Copy patched buffer into constant buffer
	#ifndef RED_PLATFORM_ORBIS
		RED_ASSERT( bufferToUpdate == m_computeRawConstantBuffer );
		GpuApi::CopyBuffer( m_computeConstantBuffer, 0, m_computeRawConstantBuffer, 0, sizeof(ComputeConstantBuffer) );
	#endif
	}
}

void CRenderInterface::ExportTiledDeferredConstants( void *outData ) const
{
	PC_SCOPE_RENDER_LVL1( ExportTiledDeferredConstants );

	RED_ASSERT( m_computeConstantShadowBuffer );
	Red::System::MemoryCopy( outData, m_computeConstantShadowBuffer.Get(), m_computeConstantShadowBuffer.GetSize() );
	RED_FATAL_ASSERT( m_cameraLightPatches.Empty(), "CameraLightPatches not supported at this moment - see also ImportTiledDeferredConstants" );
}

void CRenderInterface::ImportTiledDeferredConstants( const void *data )
{
	PC_SCOPE_RENDER_LVL1( ImportTiledDeferredConstants );

	RED_ASSERT( m_computeConstantShadowBuffer );
	Red::System::MemoryCopy( m_computeConstantShadowBuffer.Get(), data, m_computeConstantShadowBuffer.GetSize() );

	m_cameraLightPatches.ClearFast(); //< camera light patches are not being exported, so just clear them
}

void CRenderInterface::InitTiledDeferredConstants()
{
	if ( !m_computeConstantBuffer )
	{
		m_computeConstantBuffer = GpuApi::CreateBuffer( sizeof(ComputeConstantBuffer), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_computeConstantBuffer, "tiled cbuffer" );
	}

#ifndef RED_PLATFORM_ORBIS
	if ( !m_computeRawConstantBuffer )
	{
		const Uint32 rawSize = sizeof(ComputeConstantBuffer);
		RED_ASSERT( !(rawSize % 4) );

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_elementCount = rawSize / 4;

		m_computeRawConstantBuffer = GpuApi::CreateBuffer( rawSize, GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		GpuApi::SetBufferDebugPath( m_computeRawConstantBuffer, "tiled raw cbuffer" );
	}
#endif

	{
		// ace_todo: create this somewhere else. for instance what about 
		// a situation when user changes desktop resolution with editor enabled?
#if MICROSOFT_ATG_DYNAMIC_SCALING
		const Uint32 maxWidth = Max<Uint32>( CRenderEnvProbeManager::GetMaxProbeTypeResolution(), 1920 );
		const Uint32 maxHeight = Max<Uint32>( CRenderEnvProbeManager::GetMaxProbeTypeResolution(), 1080 );
#else
		const Uint32 maxWidth = Max<Uint32>( CRenderEnvProbeManager::GetMaxProbeTypeResolution(), GetSurfaces()->GetWidth() );
		const Uint32 maxHeight = Max<Uint32>( CRenderEnvProbeManager::GetMaxProbeTypeResolution(), GetSurfaces()->GetHeight() );
#endif
		if ( !m_computeTileLights )
		{
			const Uint32 numElements = (maxWidth/TILE_SIZE + 1) * (maxHeight/TILE_SIZE + 1) * CRenderInterface::MAX_LIGHTS_PER_TILE;
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_elementCount = numElements;
			m_computeTileLights = GpuApi::CreateBuffer( 4 * numElements, GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		}

		if ( !m_computeTileDimmers )
		{
			const Uint32 numElements = (maxWidth/TILE_SIZE_INTERIOR_FULLRES + 1) * (maxHeight/TILE_SIZE_INTERIOR_FULLRES + 1) * CRenderInterface::MAX_DIMMERS_PER_TILE;
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_elementCount = numElements;
			m_computeTileDimmers = GpuApi::CreateBuffer( 4 * numElements, GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		}
	}

	// Allocate shadow buffer
	if ( m_computeConstantShadowBuffer.GetSize() == 0 )
	{
		m_computeConstantShadowBuffer = Red::CreateUniqueBuffer( sizeof( ComputeConstantBuffer ), 16, MC_RenderData );
	}
	ASSERT( m_computeConstantShadowBuffer.GetSize() == sizeof( ComputeConstantBuffer ) );
}

void CRenderInterface::CalculateTiledDeferredConstants( CRenderCollector& collector, const CCascadeShadowResources &cascadeShadowResources, Bool flush )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants );

	const Float cameraInteriorFactor = collector.m_scene ? collector.m_scene->GetDelayedCameraInteriorFactor() : 0.f;

	const CRenderCollector::CRenderCollectorData *rdata = collector.m_renderCollectorData;
	const CRenderFrameInfo &info = collector.GetRenderFrameInfo();

	CalculateTiledDeferredConstants_Various( collector.GetRenderFrameInfo() );
	CalculateTiledDeferredConstants_CascadeShadows( collector.GetRenderFrameInfo(), &collector.m_cascades, cascadeShadowResources );
	CalculateTiledDeferredConstants_TerrainShadows( collector.GetRenderFrameInfo(), collector.m_scene );
	CalculateTiledDeferredConstants_Lights( &info, info.m_camera.GetPosition(), info.m_worldRenderSettings, info.m_envParametersArea.m_colorGroups, rdata ? rdata->m_lights.Size() : 0, rdata ? rdata->m_lights.TypedData() : 0, collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Shadows ), true, cameraInteriorFactor );
	CalculateTiledDeferredConstants_Dimmers( collector.GetRenderFrameInfo(), rdata ? rdata->m_dimmers.Size() : 0, rdata ? rdata->m_dimmers.TypedData() : 0 );

	if ( flush )
	{
		FlushTiledDeferredConstants();
	}
}

void CRenderInterface::CalculateTiledDeferredConstants_Various( const CRenderFrameInfo &info )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants_Various );

	ComputeConstantBuffer* cbuff = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );

	const CEnvGlobalLightParametersAtPoint& globalLightParams = info.m_envParametersArea.m_globalLight;

	// global light 
	cbuff->lightColor = info.m_baseLightingParameters.m_sunLightDiffuse;
	cbuff->lightColorLightSide = info.m_baseLightingParameters.m_sunLightDiffuseLightSide;
	cbuff->lightColorLightOppositeSide = info.m_baseLightingParameters.m_sunLightDiffuseLightOppositeSide;	
	cbuff->lightSpecularColorAndShadowAmount = Vector::ZEROS;
	cbuff->lightDir = info.m_baseLightingParameters.m_lightDirection;
	const Float billboardsGlobalBleed = info.m_envParametersArea.m_speedTree.m_billboardsLightBleed.GetScalarClampMin( 0.f );
	cbuff->lightTweaks = Vector ( 1.f - Clamp( billboardsGlobalBleed, 0.f, 1.f ), Clamp( billboardsGlobalBleed, 0.f, 1.f ), 0, 0 );
	cbuff->characterLightParams = Vector ( 
		globalLightParams.m_charactersLightingBoostAmbientLight.GetScalarClampMin( 0.f ), 		
		globalLightParams.m_charactersLightingBoostAmbientShadow.GetScalarClampMin( 0.f ),
		globalLightParams.m_charactersLightingBoostReflectionLight.GetScalarClampMin( 0.f ), 
		globalLightParams.m_charactersLightingBoostReflectionShadow.GetScalarClampMin( 0.f ) );

	{
		const Vector blicksColor = globalLightParams.m_charactersEyeBlicksColor.GetColorScaledGammaToLinear( true );
		cbuff->characterEyeBlicksColor = Vector ( blicksColor.X, blicksColor.Y, blicksColor.Z, globalLightParams.m_charactersEyeBlicksShadowedScale.GetScalarClampMin( 0.f ) );
	}

	// 
	cbuff->ambientShadowAmount = Vector::ONES;

	// base lighting
	{
		cbuff->ambientColorTop = Vector ( 1, 0, 1, 0 );
		cbuff->ambientColorBottom = Vector ( 1, 0, 1, 0 );
		cbuff->ambientColorFront = Vector ( 1, 0, 1, 0 );
		cbuff->ambientColorBack = Vector ( 1, 0, 1, 0 );
		cbuff->ambientColorSide = Vector ( 1, 0, 1, 0 );
	}

	// sky
	cbuff->_skyColor = Vector::ZEROS;
	cbuff->_skyColorHorizon= Vector::ZEROS;
	cbuff->_sunColorHorizon= Vector::ZEROS;
	cbuff->_sunBackHorizonColor = Vector::ZEROS;
	cbuff->_sunColorSky = Vector::ZEROS;	

	//
	{
		const Float fadeStart = Max( 0.f, info.m_worldRenderSettings.m_interiorVolumesFadeStartDist );
		const Float fadeRange = Max( 0.001f, info.m_worldRenderSettings.m_interiorVolumesFadeRange );
		const Float encodeRange = Max( 0.001f, info.m_worldRenderSettings.m_interiorVolumesFadeEncodeRange );
		cbuff->interiorParams = Vector ( Clamp( info.m_worldRenderSettings.m_interiorDimmerAmbientLevel, 0.f, 1.f ), DimmerParams::PackInsideAmbientLevel( info.m_worldRenderSettings.m_interiorDimmerAmbientLevel ), info.m_worldRenderSettings.m_interiorVolumeSmoothExtent, 1.f / Max( 0.001f, info.m_worldRenderSettings.m_interiorVolumeSmoothExtent * info.m_worldRenderSettings.m_interiorVolumeSmoothRemovalRange ) );
		cbuff->interiorRangeParams = Vector( fadeStart, 1.f / fadeRange, encodeRange, 1.f / encodeRange );
	}

	cbuff->histogramParams = Vector( 
		0.0f, 
		0.0f,
		0.0f,
		0.0f );

	cbuff->fresnelGainParams.Set4( Clamp( info.m_worldRenderSettings.m_fresnelScaleLights, 0.f, 1.f ), Clamp( info.m_worldRenderSettings.m_fresnelScaleEnvProbes, 0.f, 1.f ), Max( 0.f, info.m_worldRenderSettings.m_fresnelRoughnessShape ), Max( 0.f, info.m_worldRenderSettings.m_fresnelRoughnessShape ) );
}

void CRenderInterface::CalculateTiledDeferredConstants_CascadeShadows( const CRenderFrameInfo &info, const SMergedShadowCascades *mergedShadowCascades, const CCascadeShadowResources &cascadeShadowResources )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants_CascadeShadows );

	ComputeConstantBuffer* cbuff = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );

	if ( NULL != mergedShadowCascades )
	{
		const SMergedShadowCascades &cascades = *mergedShadowCascades;

		const Uint32 numCascades = cascades.m_numCascades;
		ASSERT( cascades.m_numCascades <= info.m_requestedNumCascades );
		ASSERT( numCascades <= MAX_CASCADES );

		// Get base camera settings
		//dex++: cascades are now owned by collector and not allocate dynamically
		const Vector basePos		= cascades.m_cascades[ 0 ].m_camera.GetPosition();
		const Vector baseRight		= cascades.m_cascades[ 0 ].m_camera.GetCameraRight();
		const Vector baseUp			= cascades.m_cascades[ 0 ].m_camera.GetCameraUp();
		const Matrix baseTransform	= cascades.m_cascades[ 0 ].m_camera.GetWorldToScreen();
		//dex--

		// Calculate sub pixel offset for Poisson offsets
		const Float cameraSubPixelOffsetX = info.m_camera.GetSubpixelOffsetX();
		const Float cameraSubPixelOffsetY = info.m_camera.GetSubpixelOffsetY();
		Float subPixelOffsetXY = cameraSubPixelOffsetY * info.m_width * POISSON_ROTATION_TEXTURE_SIZE + cameraSubPixelOffsetX * POISSON_ROTATION_TEXTURE_SIZE;

		// calculate adjusted filter scales
		const Float cascadeSize0 = Max<Float>(1.0f, cascades.m_cascades[0].m_camera.GetZoom() );
		const Float cascadeSize1 = Max<Float>(1.0f, cascades.m_cascades[1].m_camera.GetZoom() );
		const Float cascadeSize2 = Max<Float>(1.0f, cascades.m_cascades[2].m_camera.GetZoom() );
		const Float cascadeSize3 = Max<Float>(1.0f, cascades.m_cascades[3].m_camera.GetZoom() );

		// Minimal filter texel size prevents from subpixel filtering - sampling disk cannot be 
		// inside one texel, bcoz this will give regular linear sampling pattern.
		const Float minFilterTexelSize = SMergedShadowCascades::MIN_FILTER_SIZE / static_cast<Float>( cascadeShadowResources.GetResolution() );

		Vector filterScales;
		filterScales.X = Max<Float>( minFilterTexelSize , info.m_cascadeFilterSizes[0] / cascadeSize0 );
		filterScales.Y = Max<Float>( minFilterTexelSize , info.m_cascadeFilterSizes[1] / cascadeSize1 );
		filterScales.Z = Max<Float>( minFilterTexelSize , info.m_cascadeFilterSizes[2] / cascadeSize2 );
		filterScales.W = Max<Float>( minFilterTexelSize , info.m_cascadeFilterSizes[3] / cascadeSize3 );
		Vector speedTreeFilterScales;
		speedTreeFilterScales.X = Max<Float>( minFilterTexelSize , info.m_speedTreeCascadeFilterSizes[0] / cascadeSize0 );
		speedTreeFilterScales.Y = Max<Float>( minFilterTexelSize , info.m_speedTreeCascadeFilterSizes[1] / cascadeSize1 );
		speedTreeFilterScales.Z = Max<Float>( minFilterTexelSize , info.m_speedTreeCascadeFilterSizes[2] / cascadeSize2 );
		speedTreeFilterScales.W = Max<Float>( minFilterTexelSize , info.m_speedTreeCascadeFilterSizes[3] / cascadeSize3 );
		Vector speedTreeShadowGradients;
		speedTreeShadowGradients.X = info.m_speedTreeShadowGradient / Max( 0.001f, cascades.m_cascades[0].m_camera.GetFarPlane() - cascades.m_cascades[0].m_camera.GetNearPlane() );
		speedTreeShadowGradients.Y = info.m_speedTreeShadowGradient / Max( 0.001f, cascades.m_cascades[1].m_camera.GetFarPlane() - cascades.m_cascades[1].m_camera.GetNearPlane() );
		speedTreeShadowGradients.Z = info.m_speedTreeShadowGradient / Max( 0.001f, cascades.m_cascades[2].m_camera.GetFarPlane() - cascades.m_cascades[2].m_camera.GetNearPlane() );
		speedTreeShadowGradients.W = info.m_speedTreeShadowGradient / Max( 0.001f, cascades.m_cascades[3].m_camera.GetFarPlane() - cascades.m_cascades[3].m_camera.GetNearPlane() );
		
		// Initialize the constants that will be set to mark the cascades placement
		COMPILE_ASSERT( MAX_CASCADES == 4 ); // Change following code if you change this
		Float paramOffsetsRight[MAX_CASCADES]	= { 0.f, 0.f, 0.f, 0.f };
		Float paramOffsetsUp[MAX_CASCADES]		= { 0.f, 0.f, 0.f, 0.f };
		Float paramHalfSizes[MAX_CASCADES]		= { 999, 999, 999, 999 };
		Float paramDepthRanges[MAX_CASCADES]	= { 1.f, 1.f, 1.f, 1.f };

		// Calculate the constants
		//dex++: cascades are now owned by collector and not allocate dynamically
		const CRenderCamera &baseShadowCamera = cascades.m_cascades[0].m_camera;
		//dex--
		for ( Uint32 i=0; i<numCascades; ++i )
		{
			//dex++: cascades are now owned by collector and not allocate dynamically
			const CRenderCamera &camera = cascades.m_cascades[i].m_camera;
			//dex--
			Vector pos = camera.GetPosition() - basePos;
			paramOffsetsRight[i] = Vector::Dot3( baseRight, pos );
			paramOffsetsUp[i]	 = Vector::Dot3( baseUp,    pos );
			paramHalfSizes[i]	 = 0.5f * camera.GetZoom();
			paramDepthRanges[i]	 = camera.GetFarPlane() - camera.GetNearPlane();
		}

		// Normalize parameters
		Float paramsScale = 1.f / paramHalfSizes[0];
		for ( Uint32 i=0; i<numCascades; ++i )
		{
			paramOffsetsRight[i]	*= paramsScale;
			paramOffsetsUp[i]		*= paramsScale;
			paramHalfSizes[i]		*= paramsScale;
		}

		// Calculate the index of the last cascade
		const Uint32 fadeCascadeIndex = (Uint32) Max<Int32>( 0, (Int32)numCascades - 1 );

		// Setup fade range
		const Float fadeRangeInner = paramHalfSizes[ fadeCascadeIndex ] * (100.f / Max<Uint32>( 1, cascadeShadowResources.GetResolution() ) );
		const Float fadeRangeOuter = 0.05f;

		// Finalize constants
		const Float poissonOffset = 0.5f + subPixelOffsetXY;
		
		cbuff->mShadowTransform = baseTransform;
		cbuff->vShadowTextureSize = Vector( (Float)cascadeShadowResources.GetResolution(), 1.0f / Max<Uint32>( 1, cascadeShadowResources.GetResolution() ), 0.0f, 0.0f );
		cbuff->vShadowOffsetsX	= Vector ( paramOffsetsRight[0],paramOffsetsRight[1],	paramOffsetsRight[2], paramOffsetsRight[3] );
		cbuff->vShadowOffsetsY	= Vector ( paramOffsetsUp[0],	paramOffsetsUp[1],		paramOffsetsUp[2],    paramOffsetsUp[3]  );
		cbuff->vShadowHalfSizes	= Vector ( paramHalfSizes[0],	paramHalfSizes[1],		paramHalfSizes[2],    paramHalfSizes[3]  );

		cbuff->vShadowFadeScales = Vector( cascades.m_cascades[0].m_edgeFade, cascades.m_cascades[1].m_edgeFade, cascades.m_cascades[2].m_edgeFade, cascades.m_cascades[3].m_edgeFade );

		for( Uint32 i = 0; i < 4; ++i )
		{
			const Float halfSegSize = paramHalfSizes[i] > FLT_EPSILON ? 2.0f * paramHalfSizes[i] : 1.0f;

			// CPU precompute. This will allow use MAD in shader
			cbuff->vShadowParams[i].X =      - ( paramOffsetsRight[i] - paramHalfSizes[i] ) / halfSegSize;	// shadow map center.x
			cbuff->vShadowParams[i].Y = 1.0f + ( paramOffsetsUp[i]	  - paramHalfSizes[i] ) / halfSegSize;	// shadow map center.y. Take care of 1-y thing
			cbuff->vShadowParams[i].Z = 1.0f / halfSegSize;
			cbuff->vShadowParams[i].W = filterScales.A[i];

			cbuff->vSpeedTreeShadowParams[i] = Vector( speedTreeFilterScales.A[i], speedTreeShadowGradients.A[i], 0, 0 );
		}

		cbuff->vShadowPoissonOffsetAndBias = Vector ( poissonOffset, 0, (Float)numCascades, 0 );
		cbuff->vShadowDepthRanges = paramDepthRanges;

		// shadow quality depends on the Performance Plataform
		cbuff->iShadowQuality = Config::cvCascadeShadowQuality.Get();
	}
	else
	{
		const Uint32 numCascades = 0;

		cbuff->mShadowTransform = Matrix::IDENTITY;
		cbuff->vShadowTextureSize = Vector( 1.f, 1.0f / (Float)1.f, 0.0f, 0.0f );
		cbuff->vShadowOffsetsX = Vector::ZEROS;
		cbuff->vShadowOffsetsY = Vector::ZEROS;
		cbuff->vShadowHalfSizes= Vector::ZEROS;
		cbuff->vShadowFadeScales = Vector::ONES;
		cbuff->vShadowParams[0] = Vector::ZEROS;
		cbuff->vShadowParams[1] = Vector::ZEROS;
		cbuff->vShadowParams[2] = Vector::ZEROS;
		cbuff->vShadowParams[3] = Vector::ZEROS;
		cbuff->vShadowPoissonOffsetAndBias = Vector ( 0, 0, (Float)numCascades, 0 );
		cbuff->vSpeedTreeShadowParams[0] = Vector::ZEROS;
		cbuff->vSpeedTreeShadowParams[1] = Vector::ZEROS;
		cbuff->vSpeedTreeShadowParams[2] = Vector::ZEROS;
		cbuff->vSpeedTreeShadowParams[3] = Vector::ZEROS;

		// shadow quality depends on the Performance Plataform
		cbuff->iShadowQuality = Config::cvCascadeShadowQuality.Get();
	}
}

void CRenderInterface::CalculateTiledDeferredConstants_Lights( const CRenderFrameInfo *cameraLightsInfo, const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Uint32 totalLights, const IRenderProxyLight * const *lights, Bool allowShadows, Bool usedDistaneLights, Float cameraInteriorFactor )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants_Lights );

	ComputeConstantBuffer* lightsData = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );

	//
	Uint32 numLights = 0;

	// Add camera lights
	m_cameraLightPatches.ClearFast();
	if ( nullptr != cameraLightsInfo && cameraLightsInfo->IsShowFlagOn( SHOW_Lights ) && cameraLightsInfo->IsShowFlagOn( SHOW_CameraLights ) )
	{
		const CRenderFrameInfo &info = *cameraLightsInfo;
		const CEnvCameraLightsSetupParametersAtPoint &cameraLightsSetup = info.m_envParametersArea.m_cameraLightsSetup;
		const SCameraLightsModifiersSetup &modifiersSetup = info.m_cameraLightsModifiersSetup;

		for ( Uint32 type_i=0; type_i<ECLT_MAX; ++type_i )
		{
			const CEnvCameraLightParametersAtPoint* lightParams0 = nullptr;
			const CEnvCameraLightParametersAtPoint* lightParams1 = nullptr;

			Float interiorAmount = 0.f;
			Vector lightInteriorColor0 = Vector::ZEROS;
			Vector lightInteriorColor1 = Vector::ZEROS;
			Float intensityScale = 1.f;
			Bool useBufferPatching = false;
			switch ( type_i )
			{
			case ECLT_Scene:
				lightParams0 = &cameraLightsSetup.m_sceneLight0;
				lightParams1 = &cameraLightsSetup.m_sceneLight1;
				lightInteriorColor0 = cameraLightsSetup.m_sceneLightColorInterior0.GetColorScaledGammaToLinear( true );
				lightInteriorColor1 = cameraLightsSetup.m_sceneLightColorInterior1.GetColorScaledGammaToLinear( true );
				interiorAmount = Clamp( cameraInteriorFactor, 0.f, 1.f );
				useBufferPatching = true;
				break;
			case ECLT_Gameplay:
				lightParams0 = &cameraLightsSetup.m_gameplayLight0;
				lightParams1 = &cameraLightsSetup.m_gameplayLight1;
				intensityScale = Lerp( Clamp( info.m_gameplayCameraLightsFactor, 0.f, 1.f ), info.m_envParametersArea.m_cameraLightsSetup.m_playerInInteriorLightsScale.GetScalarClampMin( 0.f ), 1.f );
				break;
			case ECLT_DialogScene:
				lightParams0 = &cameraLightsSetup.m_dialogLight0;
				lightParams1 = &cameraLightsSetup.m_dialogLight1;
				break;
			case ECLT_Interior:
				lightParams0 = &cameraLightsSetup.m_interiorLight0;
				lightParams1 = &cameraLightsSetup.m_interiorLight1;
				break;
			default:
				RED_ASSERT( !"Invalid" );
				continue;
			}

			RED_ASSERT( !(1 != intensityScale && useBufferPatching) && "Not supported - patched lights will be overwritten later anyway so we'll loost the intensity scale" );

			const SCameraLightsTypeModifier &typeModifier = modifiersSetup.m_modifiersByType[ type_i ];

			Vector inOutPatchColors[2];

			//#define ENABLE_DEBUG_PATCHING

			#ifdef ENABLE_DEBUG_PATCHING
			{
				if ( ECLT_Gameplay != type_i )
				{
					continue;
				}
				useBufferPatching = true;
				lightInteriorColor0 = Vector::ONES;
				lightInteriorColor1 = Vector::ONES;
			}
			#endif

			if ( intensityScale > 0 &&
				 numLights < CRenderInterface::TILED_DEFERRED_LIGHTS_CAPACITY && 
				 lightsData->lightParams[ numLights ].PackCameraLight( info.m_camera, typeModifier.m_lightModifier0, *lightParams0, lightInteriorColor0, interiorAmount, useBufferPatching ? inOutPatchColors : nullptr ) )
			{
				if ( useBufferPatching )
				{
					#ifdef ENABLE_DEBUG_PATCHING
					inOutPatchColors[0] = Vector( 100.f, 0.f, 0.f, 1.f );
					inOutPatchColors[1] = Vector( 0.f, 100.f, 0.f, 1.f );
					#endif

					m_cameraLightPatches.Grow( 1 );
					m_cameraLightPatches.Back().byteOffset = (Uint32)( reinterpret_cast<Uint64>( &lightsData->lightParams[numLights].colorAndType ) - reinterpret_cast<Uint64>( lightsData ) );
					m_cameraLightPatches.Back().color0 = inOutPatchColors[0];
					m_cameraLightPatches.Back().color1 = inOutPatchColors[1];
				}

				if ( 1 != intensityScale )
				{
					lightsData->lightParams[ numLights ].colorAndType.X *= intensityScale;
					lightsData->lightParams[ numLights ].colorAndType.Y *= intensityScale;
					lightsData->lightParams[ numLights ].colorAndType.Z *= intensityScale;
				}

				++numLights;
			}

			if ( intensityScale > 0 &&
				 numLights < CRenderInterface::TILED_DEFERRED_LIGHTS_CAPACITY && 
				 lightsData->lightParams[ numLights ].PackCameraLight( info.m_camera, typeModifier.m_lightModifier1, *lightParams1, lightInteriorColor1, interiorAmount, useBufferPatching ? inOutPatchColors : nullptr ) )
			{
				if ( useBufferPatching )
				{
					#ifdef ENABLE_DEBUG_PATCHING
					inOutPatchColors[0] = Vector( 100.f, 0.f, 0.f, 1.f );
					inOutPatchColors[1] = Vector( 0.f, 100.f, 0.f, 1.f );
					#endif

					m_cameraLightPatches.Grow( 1 );
					m_cameraLightPatches.Back().byteOffset = (Uint32)( reinterpret_cast<Uint64>( &lightsData->lightParams[numLights].colorAndType ) - reinterpret_cast<Uint64>( lightsData ) );
					m_cameraLightPatches.Back().color0 = inOutPatchColors[0];
					m_cameraLightPatches.Back().color1 = inOutPatchColors[1];
				}

				if ( 1 != intensityScale )
				{
					lightsData->lightParams[ numLights ].colorAndType.X *= intensityScale;
					lightsData->lightParams[ numLights ].colorAndType.Y *= intensityScale;
					lightsData->lightParams[ numLights ].colorAndType.Z *= intensityScale;
				}

				++numLights;
			}
		}
	}

	// Check all lights collected
	for ( Uint32 light_i=0; light_i<totalLights; ++light_i )
	{
		// out of buffer space
		if ( numLights >= CRenderInterface::TILED_DEFERRED_LIGHTS_CAPACITY )
		{
			break;
		}

		// process light
		const IRenderProxyLight *light = lights[light_i];
		if ( light->GetDistanceInfo( cameraPosition, worldRenderSettings, colorGroupsParams, usedDistaneLights ).VisibleNear() )
		{
			// Always evaluate light position and color
			const Vector finalColor = light->GetFinalColor( cameraPosition, worldRenderSettings, colorGroupsParams, usedDistaneLights );
			const Vector finalPos = light->GetFinalPosition();

			// Point light - it can be packed as a point or as up to 6 spots
			if ( light->GetType() == RPT_PointLight )
			{
				const CRenderProxy_PointLight& pointLight = *static_cast< const CRenderProxy_PointLight* >( light );
				LightParams& param = lightsData->lightParams[ numLights++ ];
				param.PackPoint( finalPos, finalColor, allowShadows, pointLight );
			}
			else if ( light->GetType() == RPT_SpotLight )
			{
				LightParams& param = lightsData->lightParams[ numLights++ ];
				param.PackSpot( finalPos, finalColor, *static_cast<const CRenderProxy_SpotLight*>( light ), allowShadows );
			}
		}
	}

	// Save the light count
	lightsData->lightNum = numLights;
}

void CRenderInterface::CalculateTiledDeferredConstants_TerrainShadows( const CRenderFrameInfo &info, const CRenderSceneEx *scene )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants_TerrainShadows );

	ComputeConstantBuffer* cbuff = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );

	//dex++: terrain shadows integration
	// Since in editor we are rendering more than one scene and we have a valid terrain
	// data only in one scene here's the code to check if the scene we are rendering to is the 
	// original scene with the terrain.
	if ( scene && scene->GetTerrainShadows() && scene->GetTerrainShadows()->IsVisible() && scene->GetTerrainShadows()->IsValidForScene( scene ) )
	{
		// compute the terrain windows and save in the shader constat buffer
		{
			cbuff->iNumTerrainShadowRegions = 0;
			const Uint32 numRegions = Min<Uint32>( 5, scene->GetTerrainShadows()->GetShadowTextureWindows().Size() );
			for ( Uint32 i=0; i<numRegions; ++i )
			{
				const CRenderTerrainShadows::Window& wnd = scene->GetTerrainShadows()->GetShadowTextureWindows()[i];
				if ( wnd.m_isValid )
				{
					cbuff->iNumTerrainShadowRegions = i+1;
					cbuff->vTerrainShadowsWindows[i].X = wnd.m_worldRect.Min.X;
					cbuff->vTerrainShadowsWindows[i].Y = wnd.m_worldRect.Min.Y;
					cbuff->vTerrainShadowsWindows[i].Z = 1.0f / ( wnd.m_worldRect.Max.X -  wnd.m_worldRect.Min.X );
					cbuff->vTerrainShadowsWindows[i].W = 1.0f / ( wnd.m_worldRect.Max.Y - wnd.m_worldRect.Min.Y );
					cbuff->vTerrainTextureWindows[i] = wnd.m_validTexels;
				}
				else
				{
					break;
				}
			}
		}

		// setup terrain texture windows
		{
			cbuff->iNumTerrainTextureRegions = 0;
			const Uint32 numRegions = Min<Uint32>( 5, scene->GetTerrainShadows()->GetTerrainTextureWindows().Size() );
			for ( Uint32 i=0; i<numRegions; ++i )
			{
				const CRenderTerrainShadows::Window& wnd = scene->GetTerrainShadows()->GetTerrainTextureWindows()[i];
				if ( wnd.m_isValid )
				{
					cbuff->iNumTerrainTextureRegions = i+1;
					cbuff->vTerrainShadowsWindows[i].X = wnd.m_worldRect.Min.X;
					cbuff->vTerrainShadowsWindows[i].Y = wnd.m_worldRect.Min.Y;
					cbuff->vTerrainShadowsWindows[i].Z = 1.0f / ( wnd.m_worldRect.Max.X -  wnd.m_worldRect.Min.X );
					cbuff->vTerrainShadowsWindows[i].W = 1.0f / ( wnd.m_worldRect.Max.Y - wnd.m_worldRect.Min.Y );
					cbuff->vTerrainTextureWindows[i] = wnd.m_validTexels;
				}
				else
				{
					break;
				}
			}
		}

		// shadow parameters
		cbuff->vTerrainShadowsParams.X = 1.0f / Max<Float>( info.m_terrainShadowsBaseSmoothing, 0.001f );
		cbuff->vTerrainShadowsParams.Y = info.m_terrainShadowsDistance;
		cbuff->vTerrainShadowsParams.Z = -(info.m_terrainShadowsDistance - info.m_terrainShadowsFadeRange );
		cbuff->vTerrainShadowsParams.W = 1.0f / info.m_terrainShadowsFadeRange;

		// terrain shadow mesh fading params
		cbuff->vTerrainShadowsParams2.X = -Max<Float>( 0.0f, info.m_terrainMeshShadowDistance - info.m_terrainMeshShadowFadeRange );
		cbuff->vTerrainShadowsParams2.Y = 1.0f / Max<Float>( 0.01f, info.m_terrainMeshShadowFadeRange );
		cbuff->vTerrainShadowsParams2.Z = 1.0f / Max<Float>( 0.01f, info.m_terrainShadowsTerrainDistanceSoftness );
		cbuff->vTerrainShadowsParams2.W = 1.0f / Max<Float>( 0.01f, info.m_terrainShadowsMeshDistanceSoftness );

		// terrain heightmap scale
		cbuff->vTerrainShadowsParams3 = scene->GetTerrainShadows()->GetTerrainTextureRange();
	}
	else 
	{
		// No terrain shadows data
		cbuff->iNumTerrainShadowRegions = 0;
		cbuff->iNumTerrainTextureRegions = 0;
		cbuff->vTerrainShadowsParams = Vector::ZEROS;
		cbuff->vTerrainShadowsParams2 = Vector::ZEROS;
		cbuff->vTerrainShadowsParams3 = Vector::ZEROS;
	}
	//dex--
}

void CRenderInterface::CalculateTiledDeferredConstants_Dimmers( const CRenderFrameInfo &info, Uint32 totalDimmers, const CRenderProxy_Dimmer * const *dimmers )
{
	PC_SCOPE_RENDER_LVL1( CalculateTiledDeferredConstants_Dimmers );

	ComputeConstantBuffer* dimmersData = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );

	const Matrix &viewToWorld = info.m_camera.GetViewToWorld();

	// Get number of dimmers
	// Limit by our capacity, and additionally get rid of the dimmers from the back of the buffer, which are fully faded out
	// (we're maintaining some range in order to prevent popping when the dimmers buffer is used at full capacity).
	Uint32 numDimmers = Min<Uint32>( totalDimmers, ARRAY_COUNT(dimmersData->dimmerParams) );
	while ( numDimmers > 0 && 0 == dimmers[numDimmers-1]->GetFadeAlphaCombined() )
	{
		--numDimmers;
	}

	// Pack dimmers
	for ( Uint32 dimmer_i=0; dimmer_i<numDimmers; ++dimmer_i )
	{
		const CRenderProxy_Dimmer *dimmer = dimmers[dimmer_i];

		dimmersData->dimmerParams[dimmer_i].Pack( info, viewToWorld, *dimmer );
	}

	dimmersData->dimmerNum = numDimmers;
}

using namespace PostProcessUtilities;

void CRenderInterface::RenderGlobalShadowMask( CRenderCollector& collector, const CCascadeShadowResources &cascadeShadowResources, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texGBuffer1, const GpuApi::TextureRef &texGBuffer2, const GpuApi::TextureRef &texDepthBuffer )
{
	PC_SCOPE_RENDER_LVL1( RenderGlobalShadowMask );

	const CRenderFrameInfo& info = collector.GetRenderFrameInfo();

	ASSERT( GpuApi::GetTextureDesc( texGBuffer1 ).msaaLevel == GpuApi::GetTextureDesc( texDepthBuffer ).msaaLevel );
	const Bool useMSAAShaders = GpuApi::GetTextureDesc( texGBuffer1 ).msaaLevel > 1;

	const Bool renderCloudsShadow = info.IsCloudsShadowVisible();	

	GpuApi::RenderTargetSetup rtSetupLocal;
	rtSetupLocal.SetColorTarget( 0, texRenderTarget );
	rtSetupLocal.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupLocal );

	GpuApi::TextureRef gbufferTexRefs[3] = { texDepthBuffer, texGBuffer1, texGBuffer2 };
	GpuApi::BindTextures( 0, ARRAY_COUNT(gbufferTexRefs), gbufferTexRefs, GpuApi::PixelShader );

	GpuApi::TextureRef cascadeArray = cascadeShadowResources.GetDepthStencilArrayRead();
	GpuApi::BindTextures( 8, 1, &cascadeArray, GpuApi::PixelShader ); 
	
	if ( collector.GetScene() && collector.GetScene()->GetTerrainShadows() && collector.GetScene()->GetTerrainShadows()->GetShadowTexture() )
	{
		GpuApi::TextureRef terrainShadows = collector.GetScene()->GetTerrainShadows()->GetShadowTexture();
		GpuApi::BindTextures( 19, 1, &terrainShadows, GpuApi::PixelShader );
	}
	else
	{
		GpuApi::BindTextures( 19, 1, nullptr, GpuApi::PixelShader );
	}

	if ( collector.GetScene() && collector.GetScene()->GetTerrainShadows() && collector.GetScene()->GetTerrainShadows()->GetTerrainTexture() )
	{
		GpuApi::TextureRef terrainTex = collector.GetScene()->GetTerrainShadows()->GetTerrainTexture();
		GpuApi::BindTextures( 15, 1, &terrainTex, GpuApi::PixelShader );
	}
    else
    {
        GpuApi::BindTextures( 15, 1, nullptr, GpuApi::PixelShader );
    }

	GetRenderer()->BindGlobalSkyShadowTextures( info, GpuApi::PixelShader );	

	GpuApi::SetSamplerStatePreset( 8, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

	GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::PixelShader );

	{
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GlobalShadow );

		CRenderShaderPair* shader = nullptr;

#if 0
		shader = useMSAAShaders ? GetRenderer()->m_shaderGlobalShadow_MSAA : GetRenderer()->m_shaderGlobalShadow;
#else
		if ( useMSAAShaders )
		{
			shader = 
#ifdef USE_ANSEL
				isAnselSessionActive ? GetRenderer()->m_shaderGlobalShadow_MSAA_BlendedCascades :
#endif // USE_ANSEL				
				GetRenderer()->m_shaderGlobalShadow_MSAA;
		}
		else
		{
			shader = 
#ifdef USE_ANSEL
				isAnselSessionActive ? GetRenderer()->m_shaderGlobalShadowBlendedCascades :
#endif // USE_ANSEL
				GetRenderer()->m_shaderGlobalShadow;
		}
#endif
		RED_FATAL_ASSERT( shader != nullptr, "" );
		if ( shader )
		{
			GetPostProcess()->m_drawer->DrawQuad( shader );
		}
	}

	GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

	GpuApi::BindTextures( 0, ARRAY_COUNT(gbufferTexRefs), nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 8, 1, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 19, 1, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 14, 2, nullptr, GpuApi::PixelShader );
	
	// Render all of the entities that require hi-res shadows - this uses forward rendering
	if ( info.IsShowFlagOn( SHOW_HiResEntityShadows ) )
	{
		PC_SCOPE_RENDER_LVL0( HiResEntityShadows );
		
		GpuApi::RenderTargetSetup rtSetupLocalWithDepth = rtSetupLocal;
		ASSERT( rtSetupLocalWithDepth.depthTarget.isNull() );
		rtSetupLocalWithDepth.SetDepthStencilTarget( texDepthBuffer );

		collector.RenderHiResEntityShadows( rtSetupLocalWithDepth );
	}
}

void CRenderInterface::RenderTiledDeferredEnvProbe( const CRenderFrameInfo& info, CRenderEnvProbe &envProbe, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texGBuffer0, const GpuApi::TextureRef &texGBuffer1, const GpuApi::TextureRef &texShadowMask, const GpuApi::TextureRef &texDepthBuffer, Uint32 faceIndex )
{
	// Unbind rendertargets
	GpuApi::SetupBlankRenderTargets();

	// Render
	{
		GpuApi::BindTextureUAVs( 0, 1, &texRenderTarget );
		GpuApi::BindBufferUAV( m_computeTileLights, 2 );
		GpuApi::BindBufferUAV( m_computeTileDimmers, 3 );
		
		//FIXME: the textures bound here are spread out, it would be much faster to bind them in one function call
		GpuApi::TextureRef gbufferRefs[3] = { texDepthBuffer, texGBuffer0, texGBuffer1 };
		GpuApi::BindTextures( 0, 3, &(gbufferRefs[0]), GpuApi::ComputeShader );
		
		// Bind envProbe related resources
		GpuApi::TextureRef envProbeTextureArray[2] = { GetEnvProbeManager()->GetAmbientEnvProbeAtlas(), GetEnvProbeManager()->GetReflectionEnvProbeAtlas() };
		GpuApi::BindTextures( 6, 2, &(envProbeTextureArray[0]), GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 6, GpuApi::SAMPSTATEPRESET_ClampLinearMipNoBias, GpuApi::ComputeShader );

		GpuApi::TextureRef shadowRefs[2] = { m_shadowManager->GetDynamicAtlas(), m_shadowManager->GetStaticAtlas() };
		GpuApi::BindTextures( 9, 2, &(shadowRefs[0]), GpuApi::ComputeShader );
		GpuApi::BindTextures( 16, 1, &texShadowMask, GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::ComputeShader );
		BindSharedConstants( GpuApi::ComputeShader );

		GpuApi::SetSamplerStatePreset( 8, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );

		// Setup envprobe params
		{
			struct SConstantBuffer
			{
				Vector envProbeDimmerFactorAndLightScale;
				Vector envProbeFaceIndex;
			};

			SConstantBuffer constBuffer;
			constBuffer.envProbeDimmerFactorAndLightScale = Vector ( Clamp( envProbe.GetProbeParams().m_genParams.m_dimmerFactor, 0.f, 1.f ), Max( 0.f, envProbe.GetProbeParams().m_genParams.m_lightScaleGlobal ), envProbe.GetProbeParams().m_genParams.GetLightScaleLocals( info.m_gameTime ), 0.f );
			constBuffer.envProbeFaceIndex = Vector ( (Float)faceIndex, 0.f, 0.f, 0.f );
			GpuApi::SetComputeShaderConsts( constBuffer );
		}

		{
			static int gridSize = TILE_SIZE;
			CRenderShaderCompute *shader = GetRenderer()->m_shaderLightsCompute_EnvProbeGen;
			shader->Dispatch( (info.m_width+(gridSize-1)) / gridSize, (info.m_height+(gridSize-1)) / gridSize, 1 );
		}

		GpuApi::BindTextures( 5, 1, nullptr, GpuApi::ComputeShader );

		GpuApi::BindTextureUAVs( 0, 4, nullptr );
		GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 2 );
		GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 3 );
		GpuApi::BindTextures( 0, 3, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 5, 3, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 9, 3, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 16, 1, nullptr, GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		UnbindSharedConstants( GpuApi::ComputeShader );
	}
}

void CRenderInterface::RenderTiledDeferred( const CRenderFrameInfo& info, EPostProcessCategoryFlags postprocessFlags, ERenderTargetName rtnTarget )
{
	PC_SCOPE_RENDER_LVL1( RenderTiledDeferred );

	ASSERT( !IsMSAAEnabled( info ) );
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();

	//
	const Bool isPureDeferredEnabled = info.IsShowFlagOn( SHOW_UseRegularDeferred );

	//
	GpuApi::SetupBlankRenderTargets();

	// Render interior mask
	{
		PC_SCOPE_RENDER_LVL1( RenderInteriorMask );

		GpuApi::TextureRef uavRefs[1] = { surfaces->GetRenderTargetTex( RTN_FinalColor ) };
		GpuApi::BindTextureUAVs( 0, 1, &(uavRefs[0]) );
		GpuApi::BindBufferUAV( m_computeTileDimmers, 1 );

		GpuApi::TextureRef gbufferRefs[2] = { surfaces->GetDepthBufferTex(), surfaces->GetRenderTargetTex( RTN_InteriorVolume ) };
		GpuApi::BindTextures( 0, 2, &(gbufferRefs[0]), GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::ComputeShader );
		BindSharedConstants( GpuApi::ComputeShader );

		{
			const int gridSize = TILE_SIZE_INTERIOR_FULLRES;
			const int numHoriz = (info.m_width+(gridSize-1)) / gridSize;
			const int numVert = (info.m_height+(gridSize-1)) / gridSize;

#ifdef USE_ANSEL
			// HACKY, skip dimmers culling in Ansel mode, fixes black screen-space stripes
			// connected with projection matrix and building frustums for clusters
			CRenderShaderCompute* interiorCompute = isAnselSessionActive ? GetRenderer()->m_shaderInteriorCompute_NoCulling : GetRenderer()->m_shaderInteriorCompute;
			RED_FATAL_ASSERT( interiorCompute, "InteriorCompute shader missing!" );
			interiorCompute->Dispatch( numHoriz, numVert, 1 );
#else
			GetRenderer()->m_shaderInteriorCompute->Dispatch( numHoriz, numVert, 1 );
#endif // USE_ANSEL
		}

		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		UnbindSharedConstants( GpuApi::ComputeShader );

		GpuApi::BindTextureUAVs( 0, 4, nullptr );
		GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 1 );
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::ComputeShader );
	}

	// Combine interior mask
	{
		PC_SCOPE_RENDER_LVL1( CombineInteriorMask );

		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		GpuApi::RenderTargetSetup rtSetupMerge;
		rtSetupMerge.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ) );
		rtSetupMerge.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetupMerge );

		GpuApi::TextureRef shadowRefs[] =	{ 
			surfaces->GetDepthBufferTex(), 
			surfaces->GetRenderTargetTex( RTN_FinalColor ),			
		};

		GpuApi::BindTextures( 0, ARRAY_COUNT(shadowRefs), shadowRefs, GpuApi::PixelShader );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::PixelShader );

		GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)info.m_width, (Float)info.m_height, 0, 0 ) );

		const Int32 halfWidthMax = Max( 1, ((Int32)info.m_width / WEATHER_VOLUMES_SIZE_DIV) ) - 1;
		const Int32 halfHeightMax = Max( 1, ((Int32)info.m_height / WEATHER_VOLUMES_SIZE_DIV) ) - 1;
		const Int32 fullWidthMax = halfWidthMax * WEATHER_VOLUMES_SIZE_DIV;
		const Int32 fullHeightMax = halfHeightMax * WEATHER_VOLUMES_SIZE_DIV;
		GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)halfWidthMax, (Float)halfHeightMax, (Float)fullWidthMax, (Float)fullHeightMax ) );

		// Draw
		{
			CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet_BlueAlpha );
			GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderInteriorCombine );
		}

		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

		GpuApi::BindTextures( 0, ARRAY_COUNT(shadowRefs), nullptr, GpuApi::PixelShader );
	}

	// Unbind rendertargets
	GpuApi::SetupBlankRenderTargets();

	// Cull lights
	if ( isPureDeferredEnabled )
	{
		PC_SCOPE_RENDER_LVL1( LightsCulling );

		GpuApi::BindBufferUAV( m_computeTileLights, 0 );
		GpuApi::TextureRef gbufferRefs[1] = {	surfaces->GetDepthBufferTex() };
		GpuApi::BindTextures( 0, 1, &(gbufferRefs[0]), GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::ComputeShader );
		BindSharedConstants( GpuApi::ComputeShader );

		{
			const int gridSize = TILE_SIZE;
			GetRenderer()->m_shaderLightsCulling_Deferred->Dispatch( (info.m_width+(gridSize-1)) / gridSize, (info.m_height+(gridSize-1)) / gridSize, 1 );
		}

		GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
		
		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		UnbindSharedConstants( GpuApi::ComputeShader );
	}

	// Render color
	{
		PC_SCOPE_RENDER_LVL1( RenderLightBuffers );
		GpuApi::TextureRef uavRefs[1] = { surfaces->GetRenderTargetTex( rtnTarget ) };
		GpuApi::BindTextureUAVs( 0, 1, &(uavRefs[0]) );
		GpuApi::BindBufferUAV( m_computeTileLights, 2 );
		
		//FIXME: the textures bound here are spread out, it would be much faster to bind them in one function call

		GpuApi::TextureRef gbufferRefs[3] = {	surfaces->GetDepthBufferTex(), 
												surfaces->GetRenderTargetTex( RTN_GBuffer0 ), 
												surfaces->GetRenderTargetTex( RTN_GBuffer1 ),
		};
		GpuApi::BindTextures( 0, 3, &(gbufferRefs[0]), GpuApi::ComputeShader );

		GpuApi::BindTextureStencil( 3, surfaces->GetDepthBufferTex(), GpuApi::ComputeShader );

		GpuApi::TextureRef gbuffer2Ref = surfaces->GetRenderTargetTex( RTN_GBuffer2 );
		GpuApi::BindTextures( 4, 1, &gbuffer2Ref, GpuApi::ComputeShader );

		// Bind envProbe related resources
		GpuApi::TextureRef envProbeTextureArray[2] = { GetEnvProbeManager()->GetAmbientEnvProbeAtlas(), GetEnvProbeManager()->GetReflectionEnvProbeAtlas() };
		GpuApi::BindTextures( 6, 2, &(envProbeTextureArray[0]), GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 6, GpuApi::SAMPSTATEPRESET_ClampLinearMipNoBias, GpuApi::ComputeShader );

		GpuApi::TextureRef shadowRefs[2] = { m_shadowManager->GetDynamicAtlas(), m_shadowManager->GetStaticAtlas() };
		GpuApi::BindTextures( 9, 2, &(shadowRefs[0]), GpuApi::ComputeShader );

		GpuApi::TextureRef moreRefs[1] = { surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ) };
		GpuApi::BindTextures( 16, 1, &(moreRefs[0]), GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::ComputeShader );
		BindSharedConstants( GpuApi::ComputeShader );

		GpuApi::SetSamplerStatePreset( 8, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, GpuApi::ComputeShader );
		GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );

		if ( isPureDeferredEnabled )
		{
			const int gridSize = TILE_SIZE;
			GetRenderer()->m_shaderLightsCompute_Deferred->Dispatch( (info.m_width+(gridSize-1)) / gridSize, (info.m_height+(gridSize-1)) / gridSize, 1 );
		}
		else
		{
			const int gridSize = TILE_SIZE;
			GetRenderer()->m_shaderLightsCompute->Dispatch( (info.m_width+(gridSize-1)) / gridSize, (info.m_height+(gridSize-1)) / gridSize, 1 );
		}

		GpuApi::BindTextureUAVs( 0, 4, nullptr );
		GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 2 );
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 4, 4, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 9, 3, nullptr, GpuApi::ComputeShader );
		GpuApi::BindTextures( 16, 1, nullptr, GpuApi::ComputeShader );

		GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		UnbindSharedConstants( GpuApi::ComputeShader );
	}

	// Render deferred lights
	if ( isPureDeferredEnabled )
	{
		PC_SCOPE_RENDER_LVL1( LightsDeferred );

		ComputeConstantBuffer* lightsData = static_cast<ComputeConstantBuffer*>( m_computeConstantShadowBuffer.Get() );
		if ( lightsData->lightNum > 0 )
		{
			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
			
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnTarget ) );
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex(), -1, true );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef texRefs[] = { 
				surfaces->GetDepthBufferTex(),
				surfaces->GetRenderTargetTex( RTN_GBuffer0 ),
				surfaces->GetRenderTargetTex( RTN_GBuffer1 ),
				surfaces->GetRenderTargetTex( RTN_GBuffer2 ),
				surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ),
			};
			GpuApi::BindTextures( 0, ARRAY_COUNT(texRefs), texRefs, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(texRefs), surfaces->GetDepthBufferTex(), GpuApi::PixelShader );

			GpuApi::TextureRef shadowRefs[2] = { m_shadowManager->GetDynamicAtlas(), m_shadowManager->GetStaticAtlas() };
			GpuApi::BindTextures( 9, 2, &(shadowRefs[0]), GpuApi::PixelShader );

			GpuApi::SetSamplerStatePreset( 8, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::ComputeShader );
			GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, GpuApi::ComputeShader );
			GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::ComputeShader );

			GpuApi::BindConstantBuffer( 13, m_computeConstantBuffer, GpuApi::PixelShader );
			
			Uint32 drawContextRefValue = -1;
			GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_DeferredLights;
			if ( Config::cvEnableDeferredLightsStencil.Get() )
			{
				drawContextRefValue = LC_ForwardShaded;
				drawContext = GpuApi::DRAWCONTEXT_DeferredLightsSkipStencilForward;
			}

			CGpuApiScopedDrawContext scopedDrawContext ( drawContext, drawContextRefValue );
			
			const Bool isCameraLightModEnabled = fabsf( info.m_envParametersArea.m_cameraLightsSetup.m_cameraLightsNonCharacterScale.GetScalar() - 1.f ) > 0.001f;

			// Spot lights
			{
				Bool prevIsCastingShadow = false;
				Bool prevNeedsCameraLightMod = false;
				GetRenderer()->m_shaderDeferredLightSpot->Bind();
				for ( Uint32 light_i=0; light_i<lightsData->lightNum; ++light_i )
				{
					const LightParams &currLight = lightsData->lightParams[light_i];
					if ( !currLight.UnpackTypeIsSpot() )
					{
						continue;
					}

					Matrix localToWorld;
					localToWorld.BuildFromDirectionVector( currLight.direction );
					localToWorld.SetScale33( Vector ( currLight.positionAndRadius.W, currLight.positionAndRadius.W, currLight.positionAndRadius.W, 1.f ) );
					localToWorld.SetTranslation( Vector ( currLight.positionAndRadius.X, currLight.positionAndRadius.Y, currLight.positionAndRadius.Z, 1.f ) );

					GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );
					GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector( currLight.UnpackSpotOuterRadAngle(), 0, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector( (Float)light_i, 0, 0, 0 ) );

					const Bool currIsCastingShadow = currLight.UnpackIsCastingShadow();
					const Bool currNeedsCameraLightMod = isCameraLightModEnabled && currLight.UnpackIsCameraLight();
					if ( currIsCastingShadow != prevIsCastingShadow || currNeedsCameraLightMod != prevNeedsCameraLightMod )
					{
						prevIsCastingShadow = currIsCastingShadow;
						prevNeedsCameraLightMod = currNeedsCameraLightMod;
						(currIsCastingShadow ? (currNeedsCameraLightMod ? GetRenderer()->m_shaderDeferredLightCamLightModSpotShadow : GetRenderer()->m_shaderDeferredLightSpotShadow) : (currNeedsCameraLightMod ? GetRenderer()->m_shaderDeferredLightCamLightModSpot : GetRenderer()->m_shaderDeferredLightSpot))->Bind();
					}

					GetRenderer()->GetDebugDrawer().DrawUnitCone();
				}
			}

			// Point lights
			{
				Bool prevIsCastingShadow = false;
				Bool prevNeedsCameraLightMod = false;
				GetRenderer()->m_shaderDeferredLightPoint->Bind();
				for ( Uint32 light_i=0; light_i<lightsData->lightNum; ++light_i )
				{
					const LightParams &currLight = lightsData->lightParams[light_i];
					if ( currLight.UnpackTypeIsSpot() )
					{
						continue;
					}

					Matrix localToWorld;
					localToWorld.SetIdentity();
					localToWorld.SetScale33( Vector ( currLight.positionAndRadius.W, currLight.positionAndRadius.W, currLight.positionAndRadius.W, 1.f ) );
					localToWorld.SetTranslation( Vector ( currLight.positionAndRadius.X, currLight.positionAndRadius.Y, currLight.positionAndRadius.Z, 1.f ) );				

					GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector( (Float)light_i, 0, 0, 0 ) );

					const Bool currIsCastingShadow = currLight.UnpackIsCastingShadow();
					const Bool currNeedsCameraLightMod = isCameraLightModEnabled && currLight.UnpackIsCameraLight();
					if ( currIsCastingShadow != prevIsCastingShadow || currNeedsCameraLightMod != prevNeedsCameraLightMod )
					{
						prevIsCastingShadow = currIsCastingShadow;
						prevNeedsCameraLightMod = currNeedsCameraLightMod;
						(currIsCastingShadow ? (currNeedsCameraLightMod ? GetRenderer()->m_shaderDeferredLightCamLightModPointShadow : GetRenderer()->m_shaderDeferredLightPointShadow) : (currNeedsCameraLightMod ? GetRenderer()->m_shaderDeferredLightCamLightModPoint : GetRenderer()->m_shaderDeferredLightPoint))->Bind();
					}

					GetRenderer()->GetDebugDrawer().DrawUnitSphere();
				}
			}

			GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

			GpuApi::BindTextures( 9, 2, nullptr, GpuApi::PixelShader );

			GpuApi::BindTextures( 0, ARRAY_COUNT(texRefs), nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( ARRAY_COUNT(texRefs), 1, nullptr, GpuApi::PixelShader );
		}
	}
}

void CRenderInterface::BindForwardConsts( const CRenderFrameInfo& info, const CCascadeShadowResources &cascadeShadowResources, CRenderSurfaces *surfaces, Bool bindEnvProbesResources, GpuApi::eShaderType shaderTarget )
{

	const Uint32 constReg = 13;

	// do we need that here anyway?
	// do we need all the shader targets here for that?
	GetRenderer()->BindGlobalSkyShadowTextures( info, shaderTarget );

	/*
	// cloud shadow texture
	{
		const Bool renderCloudsShadow = info.IsCloudsShadowVisible();		
		CRenderTextureArray *cloudsRenderTexture = info.m_envParametersDayPoint.m_cloudsShadowTexture.Get< CRenderTextureArray >();

		if ( renderCloudsShadow && cloudsRenderTexture )
		{
			switch(shaderTarget)
			{
			case GpuApi::PixelShader:
				cloudsRenderTexture->Bind( 14, RST_PixelShader );
				break;
			case GpuApi::VertexShader :
				cloudsRenderTexture->Bind( 14, RST_VertexShader );
				break;
			case GpuApi::ComputeShader :
				cloudsRenderTexture->Bind( 14, RST_ComputeShader );
				break;
			case GpuApi::GeometryShader :
				cloudsRenderTexture->Bind( 14, RST_GeometryShader );
				break;
			case GpuApi::DomainShader :
				cloudsRenderTexture->Bind( 14, RST_DomainShader );
				break;
			case GpuApi::HullShader :
				cloudsRenderTexture->Bind( 14, RST_HullShader );
				break;
			case GpuApi::ShaderTypeMax :
				cloudsRenderTexture->Bind( 14, RST_Max );
				break;
			default:
				break;
			}
		}
		else
		{
			GpuApi::BindTextures( 14, 1, nullptr, shaderTarget );
		}

		GpuApi::SetSamplerStatePreset( 14, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, shaderTarget);
	}
	*/

	// Setting for PS
	GpuApi::SetSamplerStatePreset( 8, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, shaderTarget );
	GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, shaderTarget );
	GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, shaderTarget );
	GpuApi::BindConstantBuffer( constReg, m_computeConstantBuffer, shaderTarget );

	GpuApi::TextureRef shadowRefs[3] = { cascadeShadowResources.GetDepthStencilArrayRead(), m_shadowManager->GetDynamicAtlas(), m_shadowManager->GetStaticAtlas() };
	GpuApi::BindTextures( 8, 3, &(shadowRefs[0]), shaderTarget);

	// terrain shadows
	if ( m_collector.GetScene() && m_collector.GetScene()->GetTerrainShadows() )
	{
		GpuApi::TextureRef terrainShadowRef = m_collector.GetScene()->GetTerrainShadows()->GetShadowTexture();
		GpuApi::BindTextures( 19, 1, &terrainShadowRef, shaderTarget );
	}

	// bind shadow buffer
	GpuApi::TextureRef shadowBuffer = surfaces ? surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ) : GpuApi::TextureRef::Null();
	GpuApi::BindTextures( PSSMP_GlobalShadowAndSSAO, 1, &shadowBuffer, shaderTarget );
	
	if ( shaderTarget == GpuApi::PixelShader)
	{
		GpuApi::TextureRef sceneDepth = surfaces ? surfaces->GetRenderTargetTex( RTN_GBuffer3Depth ) : GpuApi::TextureRef::Null();
		GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, shaderTarget );
	}

	GpuApi::SetSamplerStatePreset( PSSMP_SceneDepth, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

	GpuApi::BindBufferSRV( m_computeTileLights, 20, shaderTarget );
	GpuApi::BindBufferSRV( m_computeTileDimmers, 21, shaderTarget);

	if ( bindEnvProbesResources )
	{
		GpuApi::TextureRef envProbeRefs[2] = { GetEnvProbeManager()->GetAmbientEnvProbeAtlas(), GetEnvProbeManager()->GetReflectionEnvProbeAtlas() };
		GpuApi::BindTextures( 22, 2, &(envProbeRefs[0]), shaderTarget );
	}
	else
	{
		GpuApi::BindTextures( 22, 2, nullptr, shaderTarget );
	}

	GpuApi::TextureRef volumes = surfaces ? surfaces->GetRenderTargetTex( RTN_InteriorVolume ) : GpuApi::TextureRef::Null();			
	GpuApi::BindTextures( VOLUME_TEXTURE_SLOT, 1, &volumes, shaderTarget );	

	GpuApi::SetSamplerStatePreset( 11 , GpuApi::SAMPSTATEPRESET_ClampLinearMipNoBias, shaderTarget );
}

void CRenderInterface::UnbindForwardConsts( const CRenderFrameInfo& info, GpuApi::eShaderType shaderTarget  )
{
	const Uint32 constReg = 13;

	GpuApi::BindConstantBuffer( constReg, GpuApi::BufferRef::Null(), shaderTarget );
	GpuApi::BindTextures( 8, 4, nullptr, shaderTarget );
	
	// shadows
	GpuApi::BindTextures( 19, 1, nullptr, shaderTarget );
	GpuApi::BindTextures( 14, 1, nullptr, shaderTarget );

	GpuApi::BindBufferSRV( GpuApi::BufferRef::Null(), 20, shaderTarget );
	GpuApi::BindBufferSRV( GpuApi::BufferRef::Null(), 21, shaderTarget );
	GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, shaderTarget );
	GpuApi::BindTextures( PSSMP_GlobalShadowAndSSAO, 1, nullptr, shaderTarget);

	GpuApi::BindTextures( 22, 2, nullptr, shaderTarget);
	GpuApi::BindTextures( VOLUME_TEXTURE_SLOT, 1, nullptr, shaderTarget );	
}
