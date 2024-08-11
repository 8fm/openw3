/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/physics/physXEngine.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/material.h"


#include "NxApexAsset.h"
#include "NxParamUtils.h"
#include "NxClothingAsset.h"

using namespace physx;
using namespace physx::apex;

/// When importing, this material will be used as default. Assuming the resource exists, this ensures that
/// all parts of the apex actors will have a proper material.
CGatheredResource resDefaultApexMaterial( TXT("engine\\materials\\defaults\\apex.w2mg"), RGF_Startup );

/// Importer for APB files
class CApexImporter : public IImporter
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CApexImporter, IImporter );

public:
	CApexImporter();

	virtual CResource* DoImport( const ImportOptions& options );

private:
	void MapMaterials( CApexResource* resource, TDynArray< String>& materialsArray );
};

BEGIN_ABSTRACT_CLASS_RTTI( CApexImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CApexImporter );

//---

/// Importer for apex cloth
class CApexClothImporter : public CApexImporter
{
	DECLARE_ENGINE_CLASS( CApexClothImporter, CApexImporter, 0 );

public:
	CApexClothImporter()
	{
		m_resourceClass = ClassID< CApexClothResource >();
	}
};

BEGIN_CLASS_RTTI( CApexClothImporter );
	PARENT_CLASS( CApexImporter );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CApexClothImporter );

//---

/// Importer for apex destruction
class CApexDestructionImporter : public CApexImporter
{
	DECLARE_ENGINE_CLASS( CApexDestructionImporter, CApexImporter, 0 );

public:
	CApexDestructionImporter ()
	{
		m_resourceClass = ClassID< CApexDestructionResource >();
	}
};

BEGIN_CLASS_RTTI( CApexDestructionImporter );
	PARENT_CLASS( CApexImporter );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CApexDestructionImporter );
//---

//---

CApexImporter::CApexImporter()
{
	// Importer
	m_formats.PushBack( CFileFormat( TXT("apb"), TXT("Apex Binary File") ) );

	// Load config
	LoadObjectConfig( TXT("User") );
}

CResource* CApexImporter::DoImport( const ImportOptions& options )
{
	// Save
	SaveObjectConfig( TXT("User") );

#ifdef USE_APEX
	// Open file
	IFile* file = GFileManager->CreateFileReader( options.m_sourceFilePath, FOF_Buffered | FOF_AbsolutePath | FOF_DoNotIOManage );
	if ( !file )
	{
		ASSERT( TXT("Couldn't create file reader for Apex resource!") );
		return NULL;
	}

	Uint32 fileSize = static_cast< Uint32 >( file->GetSize() );
	void* buffer = malloc( fileSize );

	file->Serialize( buffer, fileSize );

	NxParameterized::Serializer::DeserializedData data;
	NxApexSDK* apexSdk = NxGetApexSDK();
	physx::PxFileBuf* stream = apexSdk->createMemoryReadStream( buffer, fileSize );
	if (!stream || stream->getOpenMode() != physx::PxFileBuf::OPEN_READ_ONLY) return false;

	NxParameterized::Serializer::SerializeType serType = NxGetApexSDK()->getSerializeType(*stream);
	NxParameterized::Serializer * ser = apexSdk->createSerializer(serType);

	NxParameterized::Serializer::ErrorType error = ser->deserialize(*stream, data); // assume there is one asset in the stream for this case
	
	ser->release();
	apexSdk->releaseMemoryReadStream(*stream);

	if( error != NxParameterized::Serializer::ERROR_NONE)
	{
		GFeedback->ShowError(TXT("Deserialization of APB failed, resource can't be created"));
		return false;
	}

	NxParameterized::Interface *params = data[0];

	CApexResource* res = NULL;

	const char* name = params->className();
	if ( name )
	{
		CClass* resourceClass = NULL;

		// We can use the name of the parameters object to determine what kind of apex resource this is.
		if ( Red::System::StringCompare( name, "ClothingAssetParameters") == 0 )
		{
			resourceClass = ClassID< CApexClothResource >();
		}
		else if ( Red::System::StringCompare( name, "DestructibleAssetParameters" ) == 0 )
		{
			resourceClass = ClassID< CApexDestructionResource >();
		}

		if ( m_resourceClass != resourceClass )
		{
			GFeedback->ShowError(TXT("You cannot import cloth resource as destruction and as opposite. Use correct importer then."));
			return false;
		}

		if ( resourceClass )
		{
			// If we have an existing resource, and it is the same type as what we need, we can reuse it.
			if ( options.m_existingResource && options.m_existingResource->IsA( resourceClass ) )
			{
				res = SafeCast< CApexResource >(options.m_existingResource);
			}
			// Otherwise we must create a new one.
			else
			{
				res = resourceClass->CreateObject< CApexResource >();
			}
		}
	}

	// If we successfully created a resource, we can copy the apex data over.
	if ( res )
	{
		res->m_apexBinaryAsset.Resize(fileSize);
		Red::System::MemoryCopy( res->m_apexBinaryAsset.TypedData(), buffer, fileSize);
	}

	delete file;
	free(buffer);

	// Now that the buffer has been cleaned up, we can die out if we couldn't create a proper resource.
	if ( !res )
	{
		return NULL;
	}

	// If we reused the existing resource, there is a bit of cleanup required.
	if ( res == options.m_existingResource )
	{
#ifndef NO_EDITOR
		res->ReleaseAssetRef( res->m_previewAsset );
		res->m_previewAsset = NULL;
#endif
		res->ReleaseAssetRef( res->m_savedAsset );
		res->m_savedAsset = NULL;

		res->DestroyDefaults();
	}

	NxParameterized::Handle boundsHandle( *params );
	if ( res->IsA< CApexDestructionResource >() )
	{
		NxParameterized::findParam( *params, "bounds", boundsHandle );
	}
	else if ( res->IsA< CApexClothResource >() )
	{
		NxParameterized::findParam( *params, "boundingBox", boundsHandle );

		CApexClothResource* cloth = Cast< CApexClothResource >( res );
		// mark existing or newly created clothing to override lod distance params
		// new asset should create default distances
		Bool createDefaultLODDistance = true;

		// if we have resource already dont override lod distance
		if ( options.m_existingResource != nullptr )
		{
			createDefaultLODDistance = false;
		}

		// Fill graphical LOD info in all cases
		Int32 numLods = 0;
		NxParameterized::getParamArraySize( *params, "graphicalLods", numLods );
		cloth->m_graphicalLodLevelInfo.Resize( numLods );
		for ( Int32 i = 0; i < numLods; ++i )
		{
			SMeshTypeResourceLODLevel& level = cloth->m_graphicalLodLevelInfo[ i ];
			
			// if we had resource dont override lod levels. its already setup. dont delete it
			if( createDefaultLODDistance )
			{
				level.m_distance = CMeshTypeResource::GetDefaultLODDistance( i );
			}
		}

		// get bone count from asset
		Uint32 numBones = 0;
		NxParameterized::getParamU32( *params, "bonesReferenced", numBones );
		physx::PxMat44 boneBindPose;
		const AnsiChar* boneName;

		cloth->m_boneCount = numBones;
		cloth->m_boneNames.Resize( numBones );
		cloth->m_boneMatrices.Resize( numBones );

		for ( Uint32 i = 0; i < numBones; ++i )
		{
			NxParameterized::getParamString( *params, StringAnsi::Printf("bones[%d].name", i ).AsChar(), boneName );
			NxParameterized::getParamMat34( *params, StringAnsi::Printf("bones[%d].bindPose", i ).AsChar(), boneBindPose );
			cloth->m_boneNames[i].Set( ANSI_TO_UNICODE( boneName ) );

			// cloth simulations all happen in local coordinates (local to the attached bone)
			// connect boneBindPose to m_boneMatrices array, but it works with identity. m_bonematrices will be ussed each frame
			// so we will mul by identity which is not good. should think to get rid of this identity somehow.
			cloth->m_boneMatrices[i].Set( Matrix::IDENTITY );
		}

#ifndef NO_EDITOR
		// Clear out any old preset name.
		cloth->m_materialPresetName = String::EMPTY;
#endif
	}
	if ( boundsHandle.isValid() )
	{
		physx::PxBounds3 bounds;
		boundsHandle.getParamBounds3( bounds );
		res->m_boundingBox.Min = Vector(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z, 1);
		res->m_boundingBox.Max = Vector(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z, 1);
	}

	// Find all 'materialNames' array. Cloths and Destructibles have differences in their parameters, and this is nice and general.
	PxU32 numMaterialNamesArrays;
	const NxParameterized::ParamResult* arraysResult = NxParameterized::getParamList( *params, NULL, "materialNames",
		numMaterialNamesArrays, true, false, apexSdk->getParameterizedTraits() );

	TDynArray< String > apexMaterialNames;

	if ( arraysResult )
	{
		// Loop over all materialNames arrays. Depending on the asset type, there may be multiple lists for different LODs or something.
		for ( Uint32 i = 0; i < numMaterialNamesArrays; ++i )
		{
			const NxParameterized::ParamResult& namesResult = arraysResult[i];
			if ( namesResult.mDataType == NxParameterized::TYPE_ARRAY )
			{
				// Grab a handle to the actual materialNames array. We'll iterate over it and make the required changes to the names.
				NxParameterized::Handle namesHandle( namesResult.mHandle );
				for ( Int32 j = 0; j < namesResult.mArraySize; ++j )
				{
					namesHandle.set( j );

					// Modify the material name.
					const char* materialNameAnsi;
					namesHandle.getParamString( materialNameAnsi );

					String materialName = ANSI_TO_UNICODE( materialNameAnsi );

					// If this material has already been added, don't add again.
					apexMaterialNames.PushBackUnique( materialName );

					namesHandle.popIndex();
				}
			}
		}

		NxParameterized::releaseParamList( numMaterialNamesArrays, arraysResult, apexSdk->getParameterizedTraits() );
	}
	MapMaterials( res, apexMaterialNames );

	data.releaseAll();

	return res;
#else
	return 0;
#endif

}


void CApexImporter::MapMaterials( CApexResource* resource, TDynArray< String>& materialNames )
{
	TDynArray< String > oldMaterialNames = resource->m_materialNames;
	CMeshTypeResource::TMaterials oldMaterials = resource->m_materials;

	// m_apexMaterialNames is filled in by the CApexResource after the import is done (when saving).
	resource->m_apexMaterialNames.ClearFast();
	resource->m_materialNames.ClearFast();
	resource->m_materials.ClearFast();

	Uint32 numMaterials = materialNames.Size();

	resource->m_materialNames.Reserve( numMaterials );
	resource->m_materials.Reserve( numMaterials );

	for ( Uint32 i = 0; i < numMaterials; ++i )
	{
		String materialName = materialNames[i];

		// Either find an existing material with the same name (if we're re-importing), or use the default material.
		IMaterial* foundMaterial  = NULL;

		// See if we have an old material with matching name.
		{
			for ( Uint32 j = 0; j < oldMaterialNames.Size(); ++j )
			{
				if ( oldMaterialNames[j] == materialName && oldMaterials[j] )
				{
					foundMaterial = oldMaterials[j].Get();
					break;
				}
			}
		}

		// Try to auto find specialized material
		if ( !foundMaterial )
		{
			// Search for matching shader
			TDynArray< CDiskFile* > materialsFound;
			const String materialFileName = materialName + TXT(".w2mg");
			GDepot->Search( materialFileName, materialsFound );

			// We have found something
			if ( materialsFound.Size() )
			{
				// There are more than one matching material
				if ( materialsFound.Size() > 1 )
				{
					WARN_IMPORTER( TXT("More than 1 material definitions found for material \"%s\". The first match that has been found will be used."), materialFileName.AsChar() );
				}

				// Get material from file
				materialsFound[0]->Load();
				foundMaterial = Cast< IMaterial >( materialsFound[0]->GetResource() );
			}

			// Material not found, use default one
			if ( !foundMaterial )
			{
				foundMaterial = resDefaultApexMaterial.LoadAndGet< IMaterial >();
				ASSERT( foundMaterial, TXT("Failed to load default apex material: %s"), resDefaultApexMaterial.GetPath().ToString().AsChar() );
			}
		}

		resource->m_materialNames.PushBack( materialName );
		resource->m_materials.PushBack( foundMaterial );
	}
}

