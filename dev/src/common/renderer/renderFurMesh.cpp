/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderFurMesh.h"
#include "renderInterface.h"
#include "../engine/furMeshResource.h"


#ifdef USE_NVIDIA_FUR


IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( CRenderFurMesh_Hairworks );


CRenderFurMesh_Hairworks::CRenderFurMesh_Hairworks()
	: m_assetID( GFSDK_HairAssetID_NULL )
{
}

CRenderFurMesh_Hairworks::~CRenderFurMesh_Hairworks()
{
	if ( m_assetID != GFSDK_HairAssetID_NULL )
	{
		GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
		if ( hairSDK )
		{
			hairSDK->FreeHairAsset( m_assetID );
		}
	}
}


CRenderFurMesh_Hairworks* CRenderFurMesh_Hairworks::Create( const CFurMeshResource* furResource, Uint64 partialRegistrationHash )
{
	RED_FATAL_ASSERT( furResource != nullptr, "Should not be calling CRenderHairworksAsset::Create with no resource" );

	TScopedRenderResourceCreationObject< CRenderFurMesh_Hairworks > createdRenderFur ( partialRegistrationHash );
	createdRenderFur.InitResource( new CRenderFurMesh_Hairworks() );

	GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
	if ( !hairSDK )
	{
		RED_LOG_ERROR( CNAME( CRenderProxy_Fur ), TXT( "HairWorks library not initialized" ) );
		return nullptr;
	}

	// Create the hair descriptor
	GFSDK_HairAssetDescriptor assetDesc;
	furResource->CreateHairAssetDesc( &assetDesc );

	// Load the hair asset
	GFSDK_HAIR_RETURNCODES loadResult = hairSDK->CreateHairAsset( assetDesc, &createdRenderFur->m_assetID );

	// Clean up the stuff that was allocated in CreateHairAssetDesc
	delete [] assetDesc.m_pVertices;
	delete [] assetDesc.m_pEndIndices;
	delete [] assetDesc.m_pFaceIndices;
	delete [] assetDesc.m_pFaceUVs;
	delete [] assetDesc.m_pBoneIndices;
	delete [] assetDesc.m_pBoneWeights;
	delete [] assetDesc.m_pBoneSpheres;
	delete [] assetDesc.m_pBoneCapsuleIndices;
	delete [] assetDesc.m_pPinConstraints;
	delete [] assetDesc.m_pBoneNames;
	delete [] assetDesc.m_pBindPoses;
	delete [] assetDesc.m_pBoneParents;


	RED_ASSERT( loadResult == GFSDK_RETURN_OK, TXT("Fur creation failed") );
	if ( loadResult != GFSDK_RETURN_OK )
	{
		return nullptr;
	}

	return createdRenderFur.RetrieveSuccessfullyCreated();
}


CName CRenderFurMesh_Hairworks::GetCategory() const
{
	return RED_NAME( RenderFurMesh );
}

String CRenderFurMesh_Hairworks::GetDisplayableName() const
{
	return TXT("Hairworks asset");
}

void CRenderFurMesh_Hairworks::OnDeviceLost()
{
}

void CRenderFurMesh_Hairworks::OnDeviceReset()
{
}


#endif // USE_NVIDIA_FUR


IRenderResource* CRenderInterface::UploadFurMesh( const CFurMeshResource* fur )
{
	RED_ASSERT( fur != nullptr, TXT("Cannot create Hairworks asset from null resource!") );
	if ( fur == nullptr )
	{
		return nullptr;
	}

#ifdef USE_NVIDIA_FUR
	CRenderFurMesh_Hairworks* renderFur = nullptr;

	if ( CanUseResourceCache() )
	{
		const Uint64 hash = CRenderFurMesh_Hairworks::CalcResourceHash( fur );
		if ( CRenderFurMesh_Hairworks::ResourceCacheRequestPartialCreate( hash, renderFur ) )
		{
			renderFur = CRenderFurMesh_Hairworks::Create( fur, hash );
		}
	}
	else
	{
		renderFur = CRenderFurMesh_Hairworks::Create( fur, 0 );
	}

	return renderFur;

#else
	ERR_RENDERER( TXT("Fur not supported. Cannot create render resource.") );
	return nullptr;
#endif
}
