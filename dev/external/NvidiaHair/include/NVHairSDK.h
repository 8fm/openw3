// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright © 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
#ifndef _NV_HAIR_SDK_H_
#define _NV_HAIR_SDK_H_

#include <d3d11.h>

#define NVHAIR_SDK_VERSION 106
#define NVHAIR_HEADER_SIZE	1024

////////////////////////////////////////////////////////////////////////////////////////
// Proposal for NVHair SDK
////////////////////////////////////////////////////////////////////////////////////////

/// This could be replaced by PhysX structs once we go into Apex
namespace NVHair
{
	/// Use these blank structs to cast from/to your own vector and matrix classes
	struct float2 { 
		float x; float y; 
		float2() {}
		float2(float ix, float iy) : x(ix), y(iy) {}
		float2(const float* pF): x(*pF++), y(*pF++)	{}
		operator float*() { return (float*)this; }
	};

	struct float3 { 
		float x; float y; float z;
		float3() {}
		float3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
		float3(const float* pF): x(*pF++), y(*pF++), z(*pF++)	{}
		inline float& operator[](int i) { return *((float*)this + i); }
		operator float*() { return (float*)this; }
	};

	struct float4 { 
		float x; float y; float z; float w;
		float4() {}
		float4(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}
		float4(const float* pF): x(*pF++), y(*pF++), z(*pF++), w(*pF++)	{}
		inline float& operator[](int i) { return *((float*)this + i); }
		operator float*() { return (float*)this; }
	};

	// dual quaternion
	struct DQ {
		float4	q0; // first quaternion
		float4  q1; // second (dual) quaternion

		DQ() {}
		DQ(const float4 iq0, const float4 iq1) : q0(iq0), q1(iq1) {}
		DQ(float qx, float qy, float qz, float qw, float dqx, float dqy, float dqz, float dqw) :
			q0(qx,qy,qz,qw), q1(dqx, dqy,dqz,dqw) {}
		inline float& operator[](int i) { return *((float*)this + i); }
		operator float*() { return (float*) this; }
	};

	struct matrix4 {
		float data[4][4];
		matrix4() {}
		matrix4(const float* pF) { memcpy(data, pF, sizeof(data)); }
		operator float*() { return (float*)this; }
	};
}

namespace NVHair
{
	typedef unsigned short HairAssetID;
	typedef unsigned short HairInstanceID;

	////////////////////////////////////////////////////////////////////////////////////////
	enum NVHAIR_RESULT
	{
		NVHAIR_OK, // success
		NVHAIR_ERROR_NOT_FOUND,  // file or resource not found 
		NVHAIR_ERROR_DATA_MISMATCH, // input data and what's stored are different
		NVHAIR_ERROR_NOT_INITIALIZED,  // sdk not initialized to run subsequent function calls
		NVHAIR_ERROR_INVALID_ARGS,	// user error in input (out of range, etc.)
		NVHAIR_ERROR_INVALID_OPERATION,	// user error, function cannot be called in current state (feature turned off, etc.)
		NVHAIR_ERROR_INVALID_DATA,	// data given to api is invalid (non-sensical size, etc.)
		NVHAIR_ERROR_OUT_OF_MEMORY, // internal out of memory error
		NVHAIR_ERROR_INTERNAL, // unknown internal error
		NVHAIR_ERROR_VERSION_MISMATCH // version mismatch between cooked asset and runtime
	};

	////////////////////////////////////////////////////////////////////////////////////////
	enum NVHAIR_TEXTURE_TYPE
	{
		NVHAIR_TEXTURE_DENSITY, // hair density map
		NVHAIR_TEXTURE_ROOT_COLOR, // color at the hair root
		NVHAIR_TEXTURE_TIP_COLOR, // color at the hair tip
		NVHAIR_TEXTURE_ROOT_WIDTH,  // width at the hair root
		NVHAIR_TEXTURE_TIP_WIDTH, // width at the hair tip
		NVHAIR_TEXTURE_STIFFNESS,  // stiffness control for simulation
		NVHAIR_TEXTURE_ROOT_STIFFNESS, // stiffness scaling control for simulation
		NVHAIR_TEXTURE_CLUMP_SCALE,  // clumpiness control
		NVHAIR_TEXTURE_CLUMP_NOISE, // clumping noise
		NVHAIR_TEXTURE_WAVE_SCALE, // waviness scale
		NVHAIR_TEXTURE_WAVE_FREQ, // waviness frequency

		NVHAIR_NUM_TEXTURES
	};

	#define NVHAIR_BONENAME_SIZE	128 // maximum allowed size for bone name

	/// descriptor to create hair asset from hair curves, growth mesh, and skinning weights
	struct HairAssetDescriptor
	{
		UINT		m_NumGuideHairs; // number of hair guide hair curves
		UINT		m_NumVertices; // number of total # of cvs in guide curves
		float3*     m_pVertices;   // all the cv data of guide curves
		UINT*		m_pEndIndices; // last vertex index of each hair, size of this array should be 'm_NumGuideHairs'	

		UINT		m_NumFaces; // number of hair triangles, we grow hairs for each mesh triangles
		UINT*		m_pFaceIndices; // triangle indices for hair faces, size must be 3 * m_NbHairFaces
		float2*		m_pFaceUVs;		 // uv values for hair faces, size must be 3 * m_NbHairFaces

		UINT		m_NumBones; // number of bones
		float4*		m_pBoneIndices; // size should be 'm_NumGuideHairs'. each mesh vertex (hair root) can have up to 4 bone indices.
		float4*		m_pBoneWeights; // size should be 'm_NumGuideHairs'. each mesh vertex (hair root) can have up to 4 bone weights.

		char*		m_pBoneNames; // names for each bone used to check if bone names match. buffer size should be at least BONENAME_SIZE * 'm_NumBones'.

		HairAssetDescriptor::HairAssetDescriptor() :
			m_NumGuideHairs(0),
			m_NumVertices(0),		
			m_pVertices(0),
			m_pEndIndices(0),

			m_NumFaces(0),
			m_pFaceIndices(0),
			m_pFaceUVs(0),

			m_NumBones(0),
			m_pBoneIndices(0),
			m_pBoneWeights(0),
			m_pBoneNames(0)
		{
		}
	};

	/// descriptor to control all aspects of hair (can be updated and animated per fame)
	/// all these values could be stored in NxParameterized
	struct HairInstanceDescriptor
	{
		/// hair geometry (density/width/length/clump) controls
		float		m_baseWidth;				// hair width 
		float		m_baseWidthShadowScale;		// additional width scale only for shadow pass
		float		m_clumpNoise;				// probability of each hair gets clumped 
												//	(0 = all hairs get clumped, 1 = clump scale is randomly distributed from 0 to 1)
		float		m_clumpRoundness;			// exponential factor to control roundness of clump shape 
												//	(0 = linear cone, clump scale *= power(t, roundness), where t is normalized distance from the root)
		float		m_clumpScale;				// how clumped each hair face is
		float		m_hairDensity;				// ratio of number of interpolated hairs compared to maximum 
		float		m_lengthNoise;				// length variation noise
		float		m_lengthScale;				// length control for growing hair effect
		float		m_rootWidthScale;			// scale factor for top side of the strand
		float		m_tipWidthScale;			// scale factor for bottom side of the strand
		float		m_waveScale;				// size of waves for hair waviness
		float		m_waveScaleNoise;			// noise factor for the wave scale
		float		m_waveFreq;					// wave frequency (1.0 = one sine wave along hair length)
		float		m_waveFreqNoise;			// noise factor for the wave frequency

		/// lod controls
		float		m_lodStart;					// distance (in scene unit) to camera where fur will start fading out (by reducing density)
		float		m_lodEnd;					// distance (in scene unit) to camera where fur will completely disappear (and stop simulating)
		bool		m_colorizeLOD;				// show LOD factor in color (works only when non-custom pixel shader is used)
		bool		m_useDynamicLOD;			// automatically adjust thickness where fur is located between lodStart and lodEnd

		/// shading controls
		/// note - all the rendering parameters will be ignored when custom user buffer is used for pixel shader.
		/// In that case, it is user's responsibility to match rendering control parameters to his/her shader.
		float		m_alpha;					// alpha value used for all hairs
	    float		m_ambientScale;				// ambient factor
		float4		m_baseColor;				// base color (when color textures are not used)
		float		m_diffuseBlend;				// blend factor between tangent based hair lighting vs normal based skin lighting (0 = all tangent, 1 = all normal)
		float4		m_diffuseColor;				// diffuse lighting color (when specular textures are not used)
		float		m_diffuseScale;				// diffuse factor
		float		m_rootDarken;				// darkening around the root
		float		m_shadowSigma;				// shadow absorption factor
		float4		m_specularColor;			// specular lighing color (when specular textures are not used)		
		float		m_specularPrimary;			// primary specular factor
		float		m_specularSecondary;		// secondary specular factor
		float		m_specularSecondaryOffset;	// secondary highlight shift offset along tangents
		float		m_specularPowerPrimary;		// primary specular power exponent
		float		m_specularPowerSecondary;	// secondary specular power exponent		
		bool		m_useTextures;				// use textures 
		bool		m_useShadows;				// turn on/off shadow
		bool		m_useShading;				// turn on/off lighting (when off, just show base colors)
		float4		m_hdrScale;					// additional color scale to control HDR exposure

		// simulation control
		bool		m_simulate;					// whether to turn on/off simulation
		float		m_damping;					// damping to slow down hair motion
		float3		m_gravity;					// gravity force vector
		float		m_stiffness;				// stiffness to restore to skinned rest shape for hairs
		float		m_rootStiffness;			// attenuation of stiffness away from the root (stiffer at root, weaker toward tip)
		float		m_windNoise;				// strength of wind noise
		float3		m_wind;						// vector force for main wind direction
		float		m_backStopRadius;			// radius of backstop collision
		float		m_collisionOffset;			// additional body offset for hair/body collision

		bool		m_useGPUSkinning;			// use gpu skinning for growth mesh

		// default values
		HairInstanceDescriptor() :

			// default geometry parameters
			m_baseWidth(3.0f),
			m_baseWidthShadowScale(10.0f),
			m_clumpNoise(0.0f),
			m_clumpRoundness(1.0f),
			m_clumpScale(0.0f),
			m_hairDensity(0.5f), 
			m_lengthNoise(1.0f),
			m_lengthScale(1.0f),
			m_rootWidthScale(1.0f),
			m_tipWidthScale(0.1f),
			m_waveScale(0.0f),
			m_waveScaleNoise(0.5f),
			m_waveFreq(3.0f),
			m_waveFreqNoise(0.5f),

			// default LOD parameters
			m_lodStart(200.0f), 
			m_lodEnd(500.0f), 
			m_colorizeLOD(false),
			m_useDynamicLOD(false),

			// default shading parameters
			m_alpha(1.0f), 
			m_ambientScale(0.3f),
			m_baseColor(0.5f, 0.5f, 0.5f, 1.0f),
			m_diffuseBlend(1.0f),
			m_diffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
			m_diffuseScale(1.0f),
			m_rootDarken(0.0f),
			m_shadowSigma(0.2f),
			m_specularColor(0.8f, 0.9f, 1.0f, 1.0f),
			m_specularPrimary(0.1f),
			m_specularSecondary(0.05f),
			m_specularSecondaryOffset(0.1f),
			m_specularPowerPrimary(100.0f),
			m_specularPowerSecondary(20.0f),
			m_useTextures(true),
			m_useShadows(true),
			m_useShading(true),
			m_hdrScale(1.0f, 1.0f, 1.0f, 1.0f),

			// default simulation parameters
			m_simulate(true),
			m_backStopRadius(0.0f),
			m_collisionOffset(0.0f),
			m_damping(0.0f),
			m_gravity(0.0f, 0.0f, -50.0f),
			m_windNoise(0.0f),
			m_wind(0.0f, 0.0f, 0.0f),
			m_stiffness(0.5f),
			m_rootStiffness(0.5f),
			m_useGPUSkinning(true)
			{
			}
	};

	// statistics for optimization and profiling (more to be added)
	struct HairStats
	{
		int		m_numCharacters;
		int		m_totalHairs;
		int		m_totalFaces;
		float	m_averageDensity;	
		float	m_averageHairsPerFace;
	};

	// hair shadow callback function can render additional objects onto body->hair shadow map
	typedef void (__cdecl * HAIR_SHADOW_CALLBACK)(ID3D11DeviceContext* pd3dContext, const HairInstanceID hairInstanceID, const matrix4* pViewProjection, const matrix4* pViewMatrix);

	// This abstract class provides all the API/asset management for hair
	// Note - could be further split into cooker vs runtime
	class HairSDK
	{
	public:
		///////////////////////////////////////////////////////////////////////////////////////////////
		virtual NVHAIR_RESULT Release(void) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Cooker and preprocessing of assets
		///////////////////////////////////////////////////////////////////////////////////////////////
		virtual NVHAIR_RESULT CookHairData(const HairAssetDescriptor& desc, void **serializedBuffer, size_t &bufferSize, unsigned short targetNbPointsPerHair = 5) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Asset management
		///////////////////////////////////////////////////////////////////////////////////////////////
		// load hair asset descriptor from cooked raw data
		virtual NVHAIR_RESULT LoadHairAsset(const void* rawData, HairAssetID *assetID) = 0;

		// destory/release hair asset data
		virtual NVHAIR_RESULT FreeHairAsset( const HairAssetID ) = 0;

		// get number of bones used in the specified hair asset
		virtual NVHAIR_RESULT GetBoneSize( const HairAssetID assetID, UINT* boneSize ) = 0;

		// get name of a bone used in the specified hair asset. 
		virtual NVHAIR_RESULT GetBoneName( const HairAssetID assetID, const UINT boneID, const char** pBoneName) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Renderer initialization and cleanup
		///////////////////////////////////////////////////////////////////////////////////////////////
		virtual NVHAIR_RESULT InitRenderResources( ID3D11Device *device, D3D11_COMPARISON_FUNC = D3D11_COMPARISON_GREATER ) = 0;
		virtual NVHAIR_RESULT FreeRenderResources() = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Hair instance creation and cleanup
		///////////////////////////////////////////////////////////////////////////////////////////////
		virtual NVHAIR_RESULT CreateHairInstance( HairAssetID hairAssetID, ID3D11Device *device, const HairInstanceDescriptor *, HairInstanceID* newInstanceID) = 0;
		virtual NVHAIR_RESULT FreeHairInstance( HairInstanceID ) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Animation and Simulation
		///////////////////////////////////////////////////////////////////////////////////////////////
		// [OPTIONAL] update render/simulation parameters 
		virtual NVHAIR_RESULT UpdateInstanceDescriptor( HairInstanceID, HairInstanceDescriptor &descriptor) = 0;

		// update skinning data by matrices
		virtual NVHAIR_RESULT UpdateSkinningMatrices( HairInstanceID, const UINT numBones, const matrix4 *boneMatrices ) = 0;

		// [OPTIONAL] update skinning data by dual quaternions 
		virtual NVHAIR_RESULT UpdateSkinningDQs( HairInstanceID, const UINT numBones, const DQ *dqs ) = 0;

		// step simulation for one frame.  GPU skinning also happens here.
		// When worldReference is set, simulation happens in the space.  
		//		This is useful when world containing hair itself is moving, but motion due to moving world is not desired and needs to be canceled out. 
		virtual NVHAIR_RESULT StepSimulation( ID3D11DeviceContext* pd3dContext, float timeStepSize = 1.0f / 60.0f, const matrix4* worldRefrence = 0) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Renderer control
		///////////////////////////////////////////////////////////////////////////////////////////////
		// set camera 
		virtual NVHAIR_RESULT SetViewProjection(const matrix4 *view,const matrix4 *proj) = 0;

		// set light direction for shadow and pixel shader
		virtual NVHAIR_RESULT SetLightDir(const float3* lightDir) = 0;

		// render hair instance. if modelToWorld is set, hair will be transformed by the matrix during rendering.
		virtual NVHAIR_RESULT RenderHairs( ID3D11DeviceContext* pd3dContext, const HairInstanceID, const matrix4 *modelToWorld = 0) = 0;

		// [OPTIONAL] set shadow callback function to add additional rendering that affects body(world)->hair shadow
		// The shadow call back can use any drawing primitive, but should not change render state/shader, etc.
		// Only shadow depth map will be affected by this call.
		virtual NVHAIR_RESULT SetHairShadowCallback(HAIR_SHADOW_CALLBACK pCallBackFunc = 0) = 0;
		
		// [OPTIONAL] set user created pixel shader to replace the default hair shader.  
		// If useUserDefinedBuffers is true, all the constant buffers and shader resources will be ignored,
		// and users have to set them before calling Render function.
		virtual NVHAIR_RESULT SetPixelShader( ID3D11PixelShader* pPixelShader = 0, bool useUserDefinedBuffers = false) = 0;	

		// [OPTIONAL] set texture resource for the hair instance
		virtual NVHAIR_RESULT SetTextureResource(ID3D11Device* pd3dDevice, const HairInstanceID, NVHAIR_TEXTURE_TYPE t, ID3D11Resource* pResource) = 0;

		// [OPTIONAL] set texture resource for the hair instance using SRV
		virtual NVHAIR_RESULT SetTextureSRV(const HairInstanceID, NVHAIR_TEXTURE_TYPE t, ID3D11ShaderResourceView* pResource) = 0;

		// [OPTIONAL] Helper method to create an ID3D11Resource texture from a memory copy of dds format
		virtual ID3D11Resource* CreateTextureFromMemory(ID3D11Device* pd3dDevice,void* pBuffer, size_t bufferSize) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// For integration of hair->body shadows
		///////////////////////////////////////////////////////////////////////////////////////////////

		// [OPTIONAL] returns currently rendered hair shadow maps.  This returns shadow map used for last RenderHairs() call.
		virtual NVHAIR_RESULT GetShadowShaderResource(ID3D11ShaderResourceView** ppShadowSRV) = 0;

		// [OPTIONAL] returns shadow matrices that may be needed to integrate hair->body shadows for last rendered hair instance.
		// If pointer is NULL, the corresponding matrix is not updated.
		// lightView: view matrix for shadow light (world -> light space)
		// lightViewProj: view matrix * camera projection for shadow camera (world->camera space)
		// lightViewProjClip2Tex:  world->shadow map texture space
		virtual NVHAIR_RESULT GetShadowMatrices(matrix4* lightView, matrix4* lightViewProj, matrix4* lightViewProjClip2Tex) = 0;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// [OPTIONAL] Profiling and Debugging
		///////////////////////////////////////////////////////////////////////////////////////////////
		// render guide hairs for debugging
		virtual NVHAIR_RESULT RenderGuideHairs( ID3D11DeviceContext* pd3dContext, const HairInstanceID, const matrix4 *modelToWorld = 0) = 0;

		// render growth mesh for debugging
		virtual NVHAIR_RESULT RenderGrowthMesh( ID3D11DeviceContext* pd3dContext, const HairInstanceID, const matrix4 *modelToWorld = 0) = 0;

		// gather statistics
		virtual NVHAIR_RESULT ComputeStats(ID3D11DeviceContext* pd3dContext, HairStats *stats) = 0;

		// visualize shadow map used for body->hair, hair->hair shadows.  This uses shadow map from last RenderHairs() call.
		virtual NVHAIR_RESULT VisualizeShadowMap(ID3D11DeviceContext* pd3dContext) = 0;
	};

	// Use this function to load HairSDK from dll
	inline HairSDK* LoadHairSDK(const char* dllPath,unsigned int version = NVHAIR_SDK_VERSION)
	{
		typedef HairSDK* (__cdecl * GET_HAIR_SDK_PROC)(unsigned int version);

		HMODULE hairDLLModule = LoadLibraryA(dllPath);

		if (hairDLLModule)
		{
			GET_HAIR_SDK_PROC pCreateProc = (GET_HAIR_SDK_PROC)GetProcAddress(hairDLLModule, "createHairSDK");

			if (!pCreateProc)
			{
				FreeLibrary( hairDLLModule);
				hairDLLModule = NULL;
			}
			else
				return pCreateProc(version);
		}

		return 0;
	}

	// Use this function to create HairSDK from statically linked lib
	HairSDK* CreateHairSDK(unsigned int version);

} // namespace NVHair

#endif