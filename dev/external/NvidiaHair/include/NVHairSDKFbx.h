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
#ifndef _NV_HAIR_FBX_EXTENSION_H_
#define _NV_HAIR_FBX_EXTENSION_H_

#include <NVHairSDK.h>

////////////////////////////////////////////////////////////////////////////////////////
// Helper functions to fill in hair descriptor from FBX
////////////////////////////////////////////////////////////////////////////////////////
namespace NVHairExt
{
	struct SkinData
	{
		UINT				m_NumBones;
		NVHair::float4*		m_pBoneIndices;
		NVHair::float4*		m_pBoneWeights;
		NVHair::matrix4*	m_pMatrices;
		NVHair::DQ*			m_pDQs;
		char*				m_pBoneNames;

	public:
		SkinData() : 
			m_NumBones(0),
			m_pBoneIndices(nullptr),
			m_pBoneWeights(nullptr),
			m_pBoneNames(nullptr),
			m_pMatrices(nullptr),
			m_pDQs(nullptr)		
			{}
	};
	
	struct MeshDesc
	{
		UINT				m_NumVertices;
		UINT				m_NumTriangles;

		NVHair::float3*		m_pVertices;
		NVHair::float3*		m_pOriginalVertices; 
		UINT*				m_pIndices;
		NVHair::float2*		m_pTexCoords;
	public:
		MeshDesc() :
			m_NumVertices(0),
			m_NumTriangles(0),
			m_pVertices(nullptr),
			m_pOriginalVertices(nullptr),
			m_pIndices(nullptr),
			m_pTexCoords(nullptr)
			{}
	};

	class HairExtSDK
	{
	public:
		/// initialize fbx loader.  
		/// when convertScale is set to true, we apply unit conversion provided by fbxsdk.
		virtual bool Initialize(const char* fbxFileName, bool convertScale = true) = 0;

		/// Get global frame range and fps information from fbx.
		virtual bool GetGlobalSettings(float* pStarfFrame, float* pEndFrame, float *pFps) = 0;

		/// find guide hairs and growth mesh from fbx, and create hair asset data.
		virtual bool CreateHairAsset(const char* guideName, const char* growthMeshName, NVHair::HairAssetDescriptor &hairDesc, bool convertScale = false, bool swizzle = false) = 0;

		/// cook guide hairs to internal serialized binary format.
		/// use the runtime function (HairSDK::LoadHairAsset) to deserialize/load this binary data.
		virtual bool CookHairData(const NVHair::HairAssetDescriptor& desc, void **userBuffer,  size_t &bufferSize, unsigned short targetNbPointsPerHair) = 0;

		/*
		 * Use functions below to apply bone mapping between the ones from fbx file and the ones actually used in games.
		 * If bone mapping is applied, the ones from fbx that are not used in newly mapped bones are discarded and ignored.
		 * The UpdateSkinning* functions in HairSDK should match remappend bones in skinning data update.
		 */

		/// Set bone mapping relationship. This must be called before the 'CreateHairAsset' or 'InitializeSkinData' call.
		/// numBones - number of bones in the new mapping
		/// boneNames - bone names used for the new mapping
		/// fbxPrefix - prefix generated from fbx exporter such as 'wolf:'.  We discard this in matching.
		virtual bool SetBoneRemapping(int numBones, char** boneNames, const char* fbxPrefix = 0) = 0;

		/// set rest pose matrices, these matrices get applied to rest geometry during cooking time
		virtual bool SetRestPoseMatrices(int numBones, NVHair::matrix4* pMatrices) = 0;

		/*
		 *	 Use functions below to create mesh and deformation.
		 *	 Note these functions are not meant to be used in real product.
		 *	 Mesh skinning should be typically done with GPU skinning, and the function below only uses CPU implementations.
		 *	 These are provided for prototype implementation and verification purpose only.
		 */

		/// get skinning data from the mesh node
		virtual bool InitializeSkinData( const char* meshName, SkinData& pSkinningDataToUpdate) = 0;

		/// update skining matrices at time (frametime) t
		virtual bool UpdateSkinData( const char* meshName, float t, SkinData& pSkinningDataToUpdate) = 0;

		/// create mesh descriptor from fbx mesh node
		virtual bool CreateMeshDescriptor(const char* meshName, MeshDesc &meshDesc) = 0;

		/// apply skinning matrices and update mesh vertices
		/// When useDQ is set, it uses dual quaternion skinning instead.
		virtual bool UpdateMeshVertices( const SkinData &skinData, UINT numVertices, const NVHair::float3* pOriginalVertices, 
			NVHair::float3* pVertices, bool useDQ = false) = 0;		

		/// shutdown sdk and release all the memory
		virtual bool Release(void) = 0;
	};
	
	// Use this function to load HairExtSDK from dll
	inline HairExtSDK* LoadHairExtSDK(const char* dllPath, unsigned int version)
	{
		typedef HairExtSDK* (__cdecl * GET_HAIR_EXT_SDK_PROC)(unsigned int version);

		HMODULE DLLModule = LoadLibraryA(dllPath);

		if (DLLModule)
		{
			GET_HAIR_EXT_SDK_PROC pCreateProc = (GET_HAIR_EXT_SDK_PROC)GetProcAddress(DLLModule, "createHairExtSDK");

			if (!pCreateProc)
			{
				FreeLibrary( DLLModule);
				DLLModule = NULL;
			}
			else
				return pCreateProc(version);
		}

		return 0;
	}

	// Use this function to create HairExtSDK from statically linked lib
	HairExtSDK* CreateHairExtSDK(unsigned int version);

} // namespace NVHairExt
#endif 
