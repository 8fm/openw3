/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "morphedMeshComponent.h"
#include "morphedMeshManagerComponent.h"
#include "renderCommands.h"
#include "mesh.h"
#include "materialDefinition.h"
#include "../core/gatheredResource.h"
#include "../core/dataError.h"
#include "meshSkinningAttachment.h"
#include "world.h"
#include "bitmapTexture.h"
#include "entity.h"
#include "materialInstance.h"

// To do material blending, source and target mesh must use the same material graph. To support more blend materials,
// add more entries to g_blendMaterialMapping.
struct BlendMaterialMapping
{
	String meshMaterialPath;
	CGatheredResource blendMaterial;

	BlendMaterialMapping( const Char* mtlPath, const Char* blendMtlPath )
		: meshMaterialPath( mtlPath )
		, blendMaterial( blendMtlPath, 0 )
	{}

	// Cannot generate, but we don't need. So not implemented. Just preventing warning.
	BlendMaterialMapping();
};

BlendMaterialMapping g_blendMaterialMapping[] = 
{
	BlendMaterialMapping( TXT("engine\\materials\\graphs\\pbr_std.w2mg"), TXT("engine\\materials\\graphs\\morphblend\\pbr_std_morph.w2mg") ),
	BlendMaterialMapping( TXT("engine\\materials\\graphs\\pbr_skin.w2mg"), TXT("engine\\materials\\graphs\\morphblend\\pbr_skin_morph.w2mg") ),
};


// We also need to provide a mapping from source/target mesh material parameters to the blend material parameters...
// If adding more blend materials, will probably need to add more parameters here.

// Parameters in mesh materials.
RED_DEFINE_STATIC_NAME( MorphControlTexture );
RED_DEFINE_STATIC_NAME( VarianceColor );
RED_DEFINE_STATIC_NAME( VarianceOffset );
//RED_DEFINE_STATIC_NAME( Diffuse );					// Already defined in the main names registry
//RED_DEFINE_STATIC_NAME( Translucency );
//RED_DEFINE_STATIC_NAME( Normal );
//RED_DEFINE_STATIC_NAME( Ambient );
RED_DEFINE_STATIC_NAME( SpecularColor );
RED_DEFINE_STATIC_NAME( RSpecScale );
RED_DEFINE_STATIC_NAME( RSpecBase );

// Parameters in blend material.
RED_DEFINE_STATIC_NAME( VarianceColor_Source );
RED_DEFINE_STATIC_NAME( VarianceOffset_Source );
RED_DEFINE_STATIC_NAME( Diffuse_Source );
RED_DEFINE_STATIC_NAME( Translucency_Source );
RED_DEFINE_STATIC_NAME( Normal_Source );
RED_DEFINE_STATIC_NAME( SpecularColor_Source );
RED_DEFINE_STATIC_NAME( RSpecScale_Source );
RED_DEFINE_STATIC_NAME( RSpecBase_Source );
RED_DEFINE_STATIC_NAME( Ambient_Source );
RED_DEFINE_STATIC_NAME( VarianceColor_Target );
RED_DEFINE_STATIC_NAME( VarianceOffset_Target );
RED_DEFINE_STATIC_NAME( Diffuse_Target );
RED_DEFINE_STATIC_NAME( Translucency_Target );
RED_DEFINE_STATIC_NAME( Normal_Target );
RED_DEFINE_STATIC_NAME( SpecularColor_Target );
RED_DEFINE_STATIC_NAME( RSpecScale_Target );
RED_DEFINE_STATIC_NAME( RSpecBase_Target );
RED_DEFINE_STATIC_NAME( Ambient_Target );

struct SBlendMaterialMapping
{
	const CName& fromMaterialParameter;
	const CName& toMaterialParameter;

	SBlendMaterialMapping( const CName& from, const CName& to )
		: fromMaterialParameter( from )
		, toMaterialParameter( to )
	{}

	// Can't be auto-generated. Not implemented. We shouldn't be copying these, and don't need default construction. Just preventing warning.
	SBlendMaterialMapping();
	SBlendMaterialMapping( const SBlendMaterialMapping& );
	const SBlendMaterialMapping& operator=( const SBlendMaterialMapping& );
};
// Parameters from source material into blend material
static const SBlendMaterialMapping blendMaterialSourceMapping[] =
{
	SBlendMaterialMapping( RED_NAME( VarianceColor ),	RED_NAME( VarianceColor_Source )	),
	SBlendMaterialMapping( RED_NAME( VarianceOffset ),	RED_NAME( VarianceOffset_Source )	),
	SBlendMaterialMapping( RED_NAME( Diffuse ),			RED_NAME( Diffuse_Source )			),
	SBlendMaterialMapping( RED_NAME( Translucency ),	RED_NAME( Translucency_Source )		),
	SBlendMaterialMapping( RED_NAME( Normal ),			RED_NAME( Normal_Source )			),
	SBlendMaterialMapping( RED_NAME( SpecularColor ),	RED_NAME( SpecularColor_Source )	),
	SBlendMaterialMapping( RED_NAME( RSpecScale ),		RED_NAME( RSpecScale_Source )		),
	SBlendMaterialMapping( RED_NAME( RSpecBase ),		RED_NAME( RSpecBase_Source )		),
	SBlendMaterialMapping( RED_NAME( Ambient ),			RED_NAME( Ambient_Source )			),
};
// Parameters from target material into blend material
static const SBlendMaterialMapping blendMaterialTargetMapping[] =
{
	SBlendMaterialMapping( RED_NAME( VarianceColor ),	RED_NAME( VarianceColor_Target )	),
	SBlendMaterialMapping( RED_NAME( VarianceOffset ),	RED_NAME( VarianceOffset_Target )	),
	SBlendMaterialMapping( RED_NAME( Diffuse ),			RED_NAME( Diffuse_Target )			),
	SBlendMaterialMapping( RED_NAME( Translucency ),	RED_NAME( Translucency_Target )		),
	SBlendMaterialMapping( RED_NAME( Normal ),			RED_NAME( Normal_Target )			),
	SBlendMaterialMapping( RED_NAME( SpecularColor ),	RED_NAME( SpecularColor_Target )	),
	SBlendMaterialMapping( RED_NAME( RSpecScale ),		RED_NAME( RSpecScale_Target )		),
	SBlendMaterialMapping( RED_NAME( RSpecBase ),		RED_NAME( RSpecBase_Target )		),
	SBlendMaterialMapping( RED_NAME( Ambient ),			RED_NAME( Ambient_Target )			),
};





IMPLEMENT_ENGINE_CLASS( CMorphedMeshComponent );

CMorphedMeshComponent::CMorphedMeshComponent()
	: m_morphSource( nullptr )
	, m_morphTarget( nullptr )
	, m_useControlTexturesForMorph( false )
	, m_morphRatio( 0 )
{
}

void CMorphedMeshComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( CEntity* entity = GetEntity() )
	{
		if ( CMorphedMeshManagerComponent* manager = entity->FindComponent< CMorphedMeshManagerComponent >() )
		{
			manager->OnMorphedMeshAttached( this );
		}
	}
}

void CMorphedMeshComponent::OnDetached( CWorld* world )
{
	if ( CEntity* entity = GetEntity() )
	{
		if ( CMorphedMeshManagerComponent* manager = entity->FindComponent< CMorphedMeshManagerComponent >() )
		{
			manager->OnMorphedMeshDetached( this );
		}
	}

	TBaseClass::OnDetached( world );
}


void CMorphedMeshComponent::SetMorphRatio( Float morphRatio )
{
	if ( m_morphRatio != morphRatio )
	{
		m_morphRatio = morphRatio;
		if ( m_renderProxy != nullptr )
		{
			( new CRenderCommand_UpdateMorphRatio( m_renderProxy, m_morphRatio ) )->Commit();
		}
	}
}


void CMorphedMeshComponent::SetMorphSource( CMesh* mesh )
{
	// Set new mesh
	if ( mesh != m_morphSource.Get() )
	{
		// Set new mesh
		m_morphSource = mesh;
		
		// Update bounding
		OnUpdateBounds();

		// Full recreation
		PerformFullRecreation();
	}
}

void CMorphedMeshComponent::SetMorphTarget( CMesh* mesh )
{
	// Set new mesh
	if ( mesh != m_morphTarget.Get() )
	{
		// Set new mesh
		m_morphTarget = mesh;

		// Update bounding
		OnUpdateBounds();

		// Full recreation
		PerformFullRecreation();
	}
}

void CMorphedMeshComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("morphRatio") )
	{
		( new CRenderCommand_UpdateMorphRatio( m_renderProxy, m_morphRatio ) )->Commit();

		// Skip past CDrawableComponent, we don't need to re-create the proxy.
		CBoundedComponent::OnPropertyPostChange( property );

		return;
	}

	TBaseClass::OnPropertyPostChange( property );

	// We need to update bounds if mesh is changed
	if ( property->GetName() == TXT("morphSource") || property->GetName() == TXT("morphTarget") )
	{
		// Update bounds
		ScheduleUpdateTransformNode();
	}

	if ( property->GetName() == TXT("name") && !m_morphComponentId )
	{
		m_morphComponentId = CName( GetName() );
	}
}

void CMorphedMeshComponent::OnUpdateBounds()
{
	// If we have both meshes, take their combined bounding box. If we just have one mesh, we pass to base class,
	// which will use just the one mesh we give it.
	if ( m_morphSource && m_morphTarget )
	{
		// No skinning, use static bounding box. If we're skinned our bounding box is set by the skinning attachment,
		// so we should do nothing.
		if ( !m_transformParent || !m_transformParent->ToSkinningAttachment() )
		{
			Box boundsUnion = m_morphSource->GetBoundingBox();
			boundsUnion.AddBox( m_morphTarget->GetBoundingBox() );

			// Use default bounding box from mesh
			m_boundingBox = GetLocalToWorld().TransformBox( boundsUnion );
		}
	}
	else
	{
		// Use default implementation
		TBaseClass::OnUpdateBounds();
	}
}


IMaterialDefinition* CMorphedMeshComponent::GetMorphBlendMaterial( Uint32 sourceMaterialIndex, Uint32 targetMaterialIndex ) const
{
	Bool useMorph = sourceMaterialIndex < m_useMorphBlendMaterials.Size() && m_useMorphBlendMaterials[ sourceMaterialIndex ];
	if ( !useMorph )
	{
		return nullptr;
	}

	IMaterial* sourceMaterial = m_morphSource->GetMaterials()[sourceMaterialIndex].Get();
	IMaterial* targetMaterial = m_morphTarget->GetMaterials()[targetMaterialIndex].Get();
	if ( sourceMaterial == nullptr || targetMaterial == nullptr )
	{
		return nullptr;
	}

	IMaterialDefinition* sourceDefinition = sourceMaterial->GetMaterialDefinition();
	IMaterialDefinition* targetDefinition = targetMaterial->GetMaterialDefinition();
	if ( sourceDefinition == nullptr || targetDefinition == nullptr )
	{
		return nullptr;
	}

	String sourceMaterialPath = sourceDefinition->GetDepotPath();
	String targetMaterialPath = targetDefinition->GetDepotPath();


	IMaterialDefinition* blendMaterial = nullptr;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32( g_blendMaterialMapping ); ++i )
	{
		if ( sourceMaterialPath == g_blendMaterialMapping[i].meshMaterialPath && targetMaterialPath == g_blendMaterialMapping[i].meshMaterialPath )
		{
			blendMaterial = g_blendMaterialMapping[i].blendMaterial.LoadAndGet< IMaterialDefinition >();
			break;
		}
	}

	return blendMaterial;
}


IMaterial* CMorphedMeshComponent::CreateBlendMaterialInstance( Uint32 sourceMaterialIndex, Uint32 targetMaterialIndex ) const
{
	if ( m_morphSource == nullptr || m_morphTarget == nullptr )
	{
		return nullptr;
	}

	// Get blend material for this source material
	IMaterial* blendMaterial = GetMorphBlendMaterial( sourceMaterialIndex, targetMaterialIndex );
	if ( blendMaterial == nullptr )
	{
		return nullptr;
	}

	IMaterial* sourceMaterial = m_morphSource->GetMaterials()[sourceMaterialIndex].Get();
	IMaterial* targetMaterial = m_morphTarget->GetMaterials()[targetMaterialIndex].Get();
	if ( sourceMaterial == nullptr || targetMaterial == nullptr )
	{
		return nullptr;
	}


	// Create material instance. Don't compile yet, because we still need to set up parameters.
	CMaterialInstance* blendInstance = new CMaterialInstance( nullptr, blendMaterial, false );


	// First, copy all parameters from the source material. Anything not set up for blending will just use that.
	// Don't recreate the render resource, we've got more work to do.
	blendInstance->CopyParametersFrom( sourceMaterial, false );


	// Temp storage for copying parameters. Not likely going to have a single parameter bigger than a matrix.
	Uint8 paramData[ sizeof( Matrix ) ];

	// Set parameters from source mesh's material
	const IMaterialDefinition* sourceMaterialDefinition = sourceMaterial->GetMaterialDefinition();
	if ( sourceMaterialDefinition != nullptr )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32( blendMaterialSourceMapping ); ++i )
		{
			const CName& fromParam = blendMaterialSourceMapping[i].fromMaterialParameter;
			const CName& toParam = blendMaterialSourceMapping[i].toMaterialParameter;
			Red::MemoryZero( paramData, sizeof(paramData) );
			if ( sourceMaterial->ReadParameterRaw( fromParam, paramData ) )
			{
				// Don't recreate the render resource. That's just unnecessary work, since we'll be setting several parameters.
				blendInstance->WriteParameterRaw( toParam, paramData, false );
			}
		}
	}

	// Set parameters from target mesh's material
	const IMaterialDefinition* targetMaterialDefinition = targetMaterial->GetMaterialDefinition();
	if ( targetMaterialDefinition != nullptr )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32( blendMaterialTargetMapping ); ++i )
		{
			const CName& fromParam = blendMaterialTargetMapping[i].fromMaterialParameter;
			const CName& toParam = blendMaterialTargetMapping[i].toMaterialParameter;
			Red::MemoryZero( paramData, sizeof(paramData) );
			if ( targetMaterial->ReadParameterRaw( fromParam, paramData ) )
			{
				// Don't recreate the render resource. That's just unnecessary work, since we'll be setting several parameters.
				blendInstance->WriteParameterRaw( toParam, paramData, false );
			}
		}
	}

	// Set morph control texture too, if we have one.
	THandle< CBitmapTexture > controlTexture = GetMorphControlTexture( sourceMaterialIndex );
	if ( controlTexture != nullptr )
	{
		blendInstance->WriteParameter( RED_NAME( MorphControlTexture ), controlTexture, false );
	}


	// Set up masking on the blend material. If either source or target material has masking enabled, then the
	// blend material will also have masking enabled.
	CMaterialInstance* sourceInstance = Cast< CMaterialInstance >( sourceMaterial );
	CMaterialInstance* targetInstance = Cast< CMaterialInstance >( targetMaterial );
	const Bool useMasking = ( sourceInstance != nullptr && sourceInstance->IsMaskEnabled() ) || ( targetInstance != nullptr && targetInstance->IsMaskEnabled() );
	blendInstance->SetMaskEnabled( useMasking );

	// NOTE : SetMaskEnabled requires render resource to be recreated after, but at this point we haven't created it, so
	// there's no need to recreate yet. Instead, we just let it get created when it's requested for the morphed render mesh.
	RED_ASSERT( !blendInstance->HasRenderResource(), TXT("Blend material shouldn't have a render resource right now!") );

	return blendInstance;
}


Int32 CMorphedMeshComponent::FindMatchingTargetMaterialIdForBlend( Uint32 sourceMaterialId ) const
{
	const auto& sourceChunks = m_morphSource->GetChunks();
	const auto& targetChunks = m_morphTarget->GetChunks();

	Int32 targetMaterialId = -1;

	for ( Uint32 chunkIdx = 0; chunkIdx < sourceChunks.Size(); ++chunkIdx )
	{
		Uint32 sourceChunkMtl = sourceChunks[chunkIdx].m_materialID;
		if ( sourceChunkMtl == sourceMaterialId && chunkIdx < targetChunks.Size() )
		{
#ifdef NO_EDITOR
			// In the game, we assume there's a one-to-one mapping. Just find the first chunk using the source material,
			// and return the material used by the corresponding chunk in the target mesh.
			targetMaterialId = targetChunks[chunkIdx].m_materialID;
			break;
#else
			// In the editor, we do some extra checks to be sure of the one-to-one mapping.
			Int32 targetChunkMtl = targetChunks[chunkIdx].m_materialID;

			// If this is the first one, store the target material.
			if ( targetMaterialId == -1 )
			{
				targetMaterialId = targetChunkMtl;
			}
			// If we had already found a target material, and this chunk doesn't use it, we don't have a proper mapping.
			else if ( targetMaterialId != targetChunkMtl )
			{
				targetMaterialId = -1;
				break;
			}
#endif
		}
	}

	return targetMaterialId;
}

CMeshTypeResource* CMorphedMeshComponent::GetMeshTypeResource() const
{ 
	return m_morphSource.IsValid() ? m_morphSource.Get() : m_morphTarget.Get(); 
}

#ifndef NO_DATA_VALIDATION

void CMorphedMeshComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	TBaseClass::OnCheckDataErrors( isInTemplate );

	if ( !m_morphSource )
	{
		DATA_HALT( DES_Minor, GetMorphTarget(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' in entity has no source mesh. This will work, but you might as well just use a CMeshComponent."), GetName().AsChar() );
	}
	else
	{
		if ( m_morphSource->GetAutoHideDistance() < 0.01f )
		{
			DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' source mesh has no AutoHideDistance!"), GetName().AsChar() );
		}
		
		if ( m_morphSource->HasFlag( DF_ForceNoAutohide ) )
		{
			DATA_HALT( DES_Minor, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' source mesh is forcing NO AUTOHIDE!"), GetName().AsChar() );
		}

		CMeshTypeResource::TMaterials& sourceMaterials = m_morphSource->GetMaterials();
		for ( auto sourceMaterial : sourceMaterials )
		{
			if ( sourceMaterial && sourceMaterial->GetMaterialDefinition() )
			{
				if ( !sourceMaterial->GetMaterialDefinition()->CanUseOnMorphMeshes() )
				{
					DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' source mesh has material '%ls' that cannot be used on MorphedMesh!"), GetName().AsChar(), sourceMaterial->GetMaterialDefinition()->GetDepotPath().AsChar() );
				}	
			}
		}
	}

	if ( !m_morphTarget )
	{
		DATA_HALT( DES_Minor,GetMorphTarget(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' has no target mesh. This will work, but you might as well just use a CMeshComponent"), GetName().AsChar() );
	}
	else
	{
		if ( m_morphTarget->GetAutoHideDistance() < 0.01f )
		{
			DATA_HALT( DES_Major,GetMorphTarget(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' target mesh has no AutoHideDistance!"), GetName().AsChar() );
		}
	
		if( m_morphTarget->HasFlag( DF_ForceNoAutohide ) )
		{
			DATA_HALT( DES_Minor, GetMorphTarget(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' target mesh is forcing NO AUTOHIDE!"), GetName().AsChar() );
		}

		CMeshTypeResource::TMaterials& targetMaterials = m_morphTarget->GetMaterials();
		for ( auto targetMaterial : targetMaterials )
		{
			if ( targetMaterial && targetMaterial->GetMaterialDefinition() )
			{
				if ( !targetMaterial->GetMaterialDefinition()->CanUseOnMorphMeshes() )
				{
					DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' target mesh has material '%ls' that cannot be used on MorphedMesh!"), GetName().AsChar(), targetMaterial->GetMaterialDefinition()->GetDepotPath().AsChar() );
				}
			}
		}
	}

	if ( m_morphSource && m_morphTarget )
	{
		if ( m_morphSource->GetMaterials().Size() != m_morphTarget->GetMaterials().Size() )
		{
			DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have a different number of materials, material morphing might not worked as desired!"), GetName().AsChar() );
		}

		if ( m_morphSource->CanUseExtraStreams() != m_morphTarget->CanUseExtraStreams() )
		{
			DATA_HALT( DES_Minor, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have inconsistent 'useExtraStreams' usage! May cause problems, especially if you're using a blended material!"), GetName().AsChar() );
		}

#ifndef NO_RESOURCE_IMPORT
		for ( Uint32 i = 0; i < m_morphSource->GetMaterials().Size(); ++i )
		{
			String sourceMaterialName = m_morphSource->GetMaterialNames()[i];

			String sourceMaterialPath = m_morphSource->GetMaterials()[i]->GetMaterialDefinition()->GetDepotPath();
			if ( i < m_useMorphBlendMaterials.Size() && m_useMorphBlendMaterials[i] )
			{
				Int32 targetMaterialId = FindMatchingTargetMaterialIdForBlend( i );
				// Find matching target material.
				if ( targetMaterialId == -1 )
				{
					DATA_HALT( DES_Minor, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' could not find matching target material for source material '%s'."), GetName().AsChar(), sourceMaterialName.AsChar() );
				}
				else
				{
					// We should be able to get a blend material
					if ( GetMorphBlendMaterial( i, targetMaterialId ) == nullptr )
					{
						DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' could not find a compatible blend material for source material '%s'. Check the source and target mesh materials."), GetName().AsChar(), sourceMaterialName.AsChar() );
					}
				}
			}
			else
			{
				// No blend material, so we should have a control texture.
				if ( i >= m_morphControlTextures.Size() || m_morphControlTextures[i] == nullptr )
				{
					DATA_HALT( DES_Minor, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' has a NULL control texture! Default may be used, and will probably look bad!"), GetName().AsChar() );
				}
			}
		}

		if ( m_morphSource->GetNumLODLevels() != m_morphTarget->GetNumLODLevels() )
		{
			DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of LOD levels!"), GetName().AsChar() );
		}

		for ( Uint32 i = 0; i < m_morphSource->GetNumLODLevels() && i < m_morphTarget->GetNumLODLevels(); ++i )
		{
			const CMesh::LODLevel& sourceLevel = m_morphSource->GetMeshLODLevels()[i];
			const CMesh::LODLevel& targetLevel = m_morphTarget->GetMeshLODLevels()[i];
			if ( sourceLevel.m_chunks.Size() != targetLevel.m_chunks.Size() )
			{
				DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of chunks in LOD %d!"), GetName().AsChar(), i );
			}
		}

		const auto& sourceChunks = m_morphSource->GetChunks();
		const auto& targetChunks = m_morphTarget->GetChunks();

		if ( sourceChunks.Size() != targetChunks.Size() )
		{
			DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of chunks!"), GetName().AsChar() );
		}

		for ( Uint32 i = 0; i < sourceChunks.Size() && i < targetChunks.Size(); ++i )
		{
			const auto& sourceChunk = sourceChunks[i];
			const auto& targetChunk = targetChunks[i];

			if ( sourceChunk.m_numIndices != targetChunk.m_numIndices )
			{
				DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of indices in chunk %d!"), GetName().AsChar(), i );
			}

			if ( sourceChunk.m_numVertices != targetChunk.m_numVertices )
			{
				DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of vertices in chunk %d!"), GetName().AsChar(), i );
			}

			if ( sourceChunk.m_numBonesPerVertex != targetChunk.m_numBonesPerVertex )
			{
				DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different number of bones per vertex in chunk %d!"), GetName().AsChar(), i );
			}

			if ( sourceChunk.m_vertexType != targetChunk.m_vertexType )
			{
				DATA_HALT( DES_Major, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' meshes have different vertex types in chunk %d!"), GetName().AsChar(), i );
			}
		}
#endif
	}

	if ( m_forceLODLevel != -1 )
	{
		DATA_HALT( DES_Minor, GetMorphSource(), TXT("Rendering"), TXT("MorphedMeshComponent '%ls' is forcing LOD level, is it intentional?"), GetName().AsChar() );
	}
}

#endif // NO_DATA_VALIDATION
