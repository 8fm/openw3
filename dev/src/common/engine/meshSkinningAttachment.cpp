/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshSkinningAttachment.h"
#include "renderCommands.h"
#include "clothComponent.h"
#include "renderSkinningData.h"
#include "skeletonProvider.h"
#include "staticMeshComponent.h"
#include "furComponent.h"
#include "renderProxy.h"
#include "component.h"
#include "entity.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CMeshSkinningAttachment );

MeshSkinningAttachmentSpawnInfo::MeshSkinningAttachmentSpawnInfo()
	: SkinningAttachmentSpawnInfo()
{
	m_attachmentClass = ClassID< CMeshSkinningAttachment >();
}

CMeshSkinningAttachment::CMeshSkinningAttachment()
	: m_skinningData( nullptr )
{
	m_isMeshSkinningAttachment = true;
}

CMeshSkinningAttachment::~CMeshSkinningAttachment()
{
}

void CMeshSkinningAttachment::OnFinalize()
{
	if ( m_skinningData != nullptr )
	{
		( new CRenderCommand_ReleaseRenderObjectOnRenderThread( m_skinningData ) )->Commit();
		SAFE_RELEASE( m_skinningData );
	}
}

void CMeshSkinningAttachment::Break()
{
	if ( m_skinningData != nullptr )
	{
		( new CRenderCommand_ReleaseRenderObjectOnRenderThread( m_skinningData ) )->Commit();
		SAFE_RELEASE( m_skinningData );
	}
	
	CSkinningAttachment::Break();
}

const struct SMeshSkeletonCacheEntryData* CMeshSkinningAttachment::GetCachedData() const
{
	// the child should be mesh component
	CMeshTypeComponent* meshComponent = Cast< CMeshTypeComponent >( GetChild() );
	//RED_ASSERT( meshComponent != nullptr, TXT("Mesh skinning attachment is not connected to mesh component. Oh well.") );
	if ( !meshComponent )
		return nullptr;

	// the mesh should be there
	CMeshTypeResource* mesh = meshComponent->GetMeshTypeResource();
	//RED_ASSERT( mesh != nullptr, TXT("Mesh skinning attachment is connected to mesh component with no mesh. Oh well.") );
	if ( !mesh )
		return nullptr;

	// the parent should be a valid node
	CNode* parentNode = GetParent();
	//RED_ASSERT( parentNode != nullptr, TXT("Mesh skinning attachment is not connected to skeleton provider. Oh well.") );
	if ( !parentNode )
		return nullptr;

	// the parent node should contain a skeleton provider
	const ISkeletonDataProvider* provider = parentNode->QuerySkeletonDataProvider();
	//RED_ASSERT( provider != nullptr, TXT("Mesh skinning attachment is connected to node with no skeleton provider. Oh well.") );
	if ( !provider )
		return nullptr;

	return mesh->GetSkeletonMappingCache().GetMappingEntry( provider );
}

const TDynArray< Int32 >& CMeshSkinningAttachment::GetBoneMapping() const
{
	RED_FATAL_ASSERT( SIsMainThread(), "Call only from main thread" );

	const auto& data = GetCachedMappingTable();

	static TDynArray< Int32 > dupa;
	dupa.Resize( data.Size() );

	for ( Uint32 i=0; i<data.Size(); ++i )
		dupa[i] = (Int16) data[i];

	return dupa;
}

const TDynArray< Int16, MC_SkinningMapping >& CMeshSkinningAttachment::GetCachedMappingTable() const
{
	const struct SMeshSkeletonCacheEntryData* data = GetCachedData();
	if ( data )
		return data->m_boneMapping;

	static TDynArray< Int16, MC_SkinningMapping > emptyMapping;
	return emptyMapping;
}

Bool CMeshSkinningAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// static meshes shouldn't be skinned so disable skinning attachment here
	if ( child->IsA< CStaticMeshComponent > () )
	{
		if( !GIsCooker ) { WARN_ENGINE( TXT("Attachment wont be created on static mesh '%ls'."), child->GetName().AsChar() ); }
		return false;
	}
	// Child component should be skinned mesh component
	if ( !child->IsA< CMeshTypeComponent >() )
	{
		if( !GIsCooker ) { WARN_ENGINE( TXT("Unable to create mesh skinning attachment because '%ls' is not a CMeshTypeComponent"), child->GetName().AsChar() ); }
		return false;
	}

	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ))
	{
		return false;
	}

	// Created
	return true;
}

Uint32 CMeshSkinningAttachment::GetSkinningMatricesAndBox( void* skinningMatrices, Box& outBoxMS, const Matrix* rigMatrices, const Float* vertexEpsilons )
{
	// Get the cached bone mapping
	const auto* data = GetCachedData();
	if ( !data )
		return 0;

	// Initialize shit
	ISkeletonDataProvider::SBonesData		bonesData( outBoxMS );
	bonesData.m_boneIndices					= data->m_boneMapping.TypedData();
	bonesData.m_numBones					= data->m_boneMapping.Size();
	bonesData.m_rigMatrices					= rigMatrices;
	bonesData.m_vertexEpsilons				= vertexEpsilons;
	bonesData.m_outMatricesArray			= skinningMatrices;
	bonesData.m_outMatricesType				= m_skinningData->GetMatrixType();
	GetSkeletonDataProvider()->GetBoneMatricesAndBoundingBoxModelSpace( bonesData );

	// Return number of skinning matrices generated
	return bonesData.m_numBones;
}

void CMeshSkinningAttachment::RecreateSkinningData()
{
	SAFE_RELEASE( m_skinningData );

	// Get the cached bone mapping
	const auto* data = GetCachedData();
	if ( data )
	{
		const Uint32 numMatrices = data->m_boneMapping.Size();

		if ( m_child->IsA< CClothComponent >() )
		{
			m_skinningData = GRender->CreateNonRenderSkinningBuffer( numMatrices );
		}
		else if ( CFurComponent* furCmp = Cast< CFurComponent >( m_child ) )
		{
			if ( furCmp->IsUsingFur() )
			{
				m_skinningData = GRender->CreateSkinningBuffer( numMatrices, false );
			}
			else
			{
				m_skinningData = GRender->CreateSkinningBuffer( numMatrices, true );
			}
		}
		else
		{
			m_skinningData = GRender->CreateSkinningBuffer( numMatrices, true );
		}

		// update hack data
		if ( m_skinningData )
		{
			if ( -1 != data->m_mappedBoneIndexHead )
			{
				if ( CMeshComponent* meshComponent = Cast< CMeshComponent >( GetChild() ) )
				{
					const CMeshTypeResource* mesh = meshComponent->GetMeshTypeResource();
					const Matrix* rigMatrices = mesh->GetBoneRigMatrices();

					const Matrix& headMat = rigMatrices[ data->m_mappedBoneIndexHead ];
					Matrix headMatInv = headMat.Inverted();

					m_skinningData->SetCustomHeadData( headMatInv.GetTranslation(), headMat.GetAxisY(), -headMat.GetAxisX() );
				}
			}
		
			m_skinningData->SetCustomMatrixIndex( SDCM_EyeOrientationLeft, data->m_mappedBoneIndexEyeLeft );
			m_skinningData->SetCustomMatrixIndex( SDCM_EyeOrientationRight, data->m_mappedBoneIndexEyeRight );
			m_skinningData->SetCustomMatrixIndex( SDCM_HeadTransformation, data->m_mappedBoneIndexHead );
		}
	}
}


void CMeshSkinningAttachment::DiscardSkinningData()
{
	SAFE_RELEASE( m_skinningData );
}


void CMeshSkinningAttachment::UpdateTransformWithoutSkinningData( const Box& boxMS, SMeshSkinningUpdateContext& skinningContext )
{
	RED_ASSERT( GetChild()->IsA< CMeshTypeComponent >() );
	CMeshTypeComponent* meshTypeComponent = static_cast< CMeshTypeComponent* >( GetChild() );
	IRenderProxy* proxy = meshTypeComponent->GetRenderProxy();
	const CNode* parentNode = GetParent();

	if ( GRender && parentNode && proxy )
	{
		meshTypeComponent->OnUpdateTransformWithoutSkinning( boxMS, parentNode->GetLocalToWorld(), skinningContext );
	}
}

void CMeshSkinningAttachment::UpdateTransformAndSkinningData( Box& outBoxMS, SMeshSkinningUpdateContext& skinningContext )
{
	RED_ASSERT( GetChild()->IsA< CMeshTypeComponent >() );
	CMeshTypeComponent* meshTypeComponent = static_cast< CMeshTypeComponent* >( GetChild() );

	const CMeshTypeResource* mesh = meshTypeComponent->GetMeshTypeResource();
	IRenderProxy* proxy = meshTypeComponent->GetRenderProxy();

	const auto* mappingData = GetCachedData();

	if ( mappingData && mesh && mesh->GetBoneCount() && proxy && GRender )
	{
		const Uint32 numMeshBones = mappingData->m_boneMapping.Size();
		RED_FATAL_ASSERT( numMeshBones == mesh->GetBoneCount(), "Skeleton mesh mapping does not match the mesh" );

		// Make sure skinning data bone count matches skeleton mapping bone count (fur case)
		if ( m_skinningData && m_skinningData->GetMatrixCount() != numMeshBones )
		{
			m_skinningData->Release();
			m_skinningData = nullptr;
		}

		// Recreate skinning data if needed
		if ( !m_skinningData )
		{
			RecreateSkinningData();
		}

		// Update the skinning data
		if ( m_skinningData )
		{
			const Matrix* rigMatrices = mesh->GetBoneRigMatrices();
			const Float* vertexEpsilons = mesh->GetBoneVertexEpsilons();

			void* dataToFill = m_skinningData->GetWriteData();

			CNode* parentNode = GetParent();
			if ( parentNode )
			{
				const ISkeletonDataProvider* provider = parentNode->QuerySkeletonDataProvider();

				// We can fill
				Box boxMS( Box::RESET_STATE );
				{
					//PC_SCOPE( GetSkinningMatricesAndBox );
					GetSkinningMatricesAndBox( dataToFill, boxMS, rigMatrices, vertexEpsilons );
				}

				outBoxMS.AddBox( boxMS );

				// Update mesh skinning
				{
					//PC_SCOPE( OnUpdateSkinning );
					meshTypeComponent->OnUpdateSkinning( provider, m_skinningData, boxMS, parentNode->GetLocalToWorld(), skinningContext );
				}
			}
		}
	}
}

Bool CMeshSkinningAttachment::IsSkinningMappingValid() const
{
	Bool isAttValid = false;
	const auto* mappingData = GetCachedData();
	if ( mappingData )
	{
		for ( const auto index : mappingData->m_boneMapping )
		{
			if ( index != -1 && index != 0 )
			{
				isAttValid = true;
				break;
			}
		}
	}

	return isAttValid;
}
