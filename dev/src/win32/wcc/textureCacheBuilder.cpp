/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/class.h"
#include "../../common/core/depot.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/rttiSystem.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/cubeTexture.h"
#include "../../common/engine/textureArray.h"
#include "../../common/engine/layer.h"
#include "../../common/engine/envProbeComponent.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheBuilderCommandlet.h"
#include "textureCacheCooker.h"
#include "textureCookerNonResource.h"
#include "baseCacheBuilder.h"

//--------------------

class CTextureCacheBuilder : public IResourceBasedCacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CTextureCacheBuilder );

public:
	CTextureCacheBuilder();
	~CTextureCacheBuilder();

	// interface
	virtual void GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const override;
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) override;

	virtual Bool Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess ) override;
	virtual Bool ProcessFile( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, const String& depotPath ) override;
	virtual Bool ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource ) override;

	virtual Bool Save() override;

	// interface for description
	virtual const Char* GetName() const override { return TXT("textures"); }
	virtual const Char* GetDescription() const override { return TXT("Compressed textures"); }

private:

	Bool ProcessBitmapTexture( CBitmapTexture* texture );
	Bool ProcessTextureArray( CTextureArray* texture );
	Bool ProcessCubeTexture( CCubeTexture* texture );
	Bool ProcessLayer( CLayer* layer );


	class CTextureCacheCooker*			m_textureCache;
	ECookingPlatform					m_platform;
};

BEGIN_CLASS_RTTI( CTextureCacheBuilder )
	PARENT_CLASS( IResourceBasedCacheBuilder );
END_CLASS_RTTI()

//--------------------

IMPLEMENT_ENGINE_CLASS( CTextureCacheBuilder );

CTextureCacheBuilder::CTextureCacheBuilder()
	: m_textureCache( nullptr )
	, m_platform( PLATFORM_None )
{
}

CTextureCacheBuilder::~CTextureCacheBuilder()
{
	delete m_textureCache;
	m_textureCache = nullptr;
}

void CTextureCacheBuilder::GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const
{
	// generic bitmap resources
	outExtensionList.PushBack( ResourceExtension< CBitmapTexture >() );
	outExtensionList.PushBack( ResourceExtension< CCubeTexture >() );
	outExtensionList.PushBack( ResourceExtension< CTextureArray >() );

	// layers - only for envmaps
	outExtensionList.PushBack( ResourceExtension< CLayer >() );

	// rouge files customs ;]
	outExtensionList.PushBack( TXT("dds") );
	outExtensionList.PushBack( TXT("png") );
	outExtensionList.PushBack( TXT("jpg") );
	outExtensionList.PushBack( TXT("jpeg") );
}

Bool CTextureCacheBuilder::Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions )
{
	// initialize base cooker
	if ( !IResourceBasedCacheBuilder::Initialize( outputFilePath, platform, additonalOptions) )
		return false;

	// setup platform
	if ( platform != PLATFORM_PC 
#ifndef WCC_LITE
		&& platform != PLATFORM_PS4 && platform != PLATFORM_XboxOne 
#endif
		)
	{
		ERR_WCC( TXT("Texture cooking for selected platform is not supported") );
		return false;
	}

	// create texture cache in the output location
	m_textureCache = new CWccTextureCacheCooker();
	if ( !m_textureCache->AttachToFile(outputFilePath ) )
	{
		ERR_WCC( TXT("Failed to create/attach texture cooker to file '%ls'"), outputFilePath.AsChar() );
		return false;
	}

	m_platform = platform;
	return true;
}


Bool CTextureCacheBuilder::Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess )
{
	CTextureCacheCooker* curTextureCacheCooker = GTextureCacheCooker;
	GTextureCacheCooker = m_textureCache;

	Bool res = IResourceBasedCacheBuilder::Process( db, filesToProcess );

	GTextureCacheCooker = curTextureCacheCooker;

	return res;
}


Bool CTextureCacheBuilder::ProcessFile( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, const String& depotPath )
{
	// special cases for raw textures - dds/png
	const String fileExtension = StringHelpers::GetFileExtension( depotPath );
	if ( fileExtension == TXT("png") || fileExtension == TXT("dds") || fileExtension == TXT("jpg") || fileExtension == TXT("jpeg") )
	{
		if ( !CookNonResourceTexture( depotPath, m_platform ) )
		{
			ERR_WCC( TXT("Failed to cook non resource texture '%ls'"), depotPath.AsChar() );
			return false;
		}

		return true;
	}

	return IResourceBasedCacheBuilder::ProcessFile( db, dbId, depotPath );
}


Bool CTextureCacheBuilder::ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource )
{
	if ( CBitmapTexture* bitmapTexture = Cast< CBitmapTexture >( resource ) )
	{
		return ProcessBitmapTexture( bitmapTexture );
	}
	else if ( CCubeTexture* cubeTexture = Cast< CCubeTexture >( resource ) )
	{
		return ProcessCubeTexture( cubeTexture );
	}
	else if ( CTextureArray* textureArray = Cast< CTextureArray >( resource ) )
	{
		return ProcessTextureArray( textureArray );
	}
	else if ( CLayer* layer = Cast< CLayer >( resource ) )
	{
		return ProcessLayer( layer );
	}

	ERR_WCC( TXT("Unrecognized resource type: %ls"), resource->GetClass()->GetName().AsChar() );
	return false;
}


Bool CTextureCacheBuilder::Save()
{
	if ( m_textureCache )
	{
		LOG_WCC( TXT("Flushing texture cache...") );

		m_textureCache->Flush();
		delete m_textureCache;
		m_textureCache = nullptr;

		LOG_WCC( TXT("Texture cache closed") );
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////


Bool CTextureCacheBuilder::ProcessBitmapTexture( CBitmapTexture* texture )
{
	if ( GTextureCacheCooker == nullptr )
	{
		RED_HALT( "No TextureCacheCooker" );
		return false;
	}

	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( m_platform );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return false;
	}

	const Uint16 residentMipMap	= texture->GetResidentMipForCookingPlatform( m_platform );
	const Uint16 firstMip		= texture->GetHighestMipForCookingPlatform( m_platform );

	if ( residentMipMap == 0 )
	{
		return true;
	}

	CTextureBakerSourceBitmapTexture source( texture, firstMip );
	GTextureCacheCooker->StoreTextureData( texture->CalcTextureCacheKey(), texture->GetDepotPath(), source, CookFunction );


	return true;
}


//////////////////////////////////////////////////////////////////////////


Bool CTextureCacheBuilder::ProcessTextureArray( CTextureArray* texture )
{
	if ( GTextureCacheCooker == nullptr )
	{
		RED_HALT( "No TextureCacheCooker" );
		return false;
	}

	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( m_platform );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return false;
	}

	// Check for any data errors. We don't need a fallback like in CTextureArray::OnCook(), because this is just for the extra
	// streaming. If a cooked texture requests streaming and we don't have it, it'll just get a null data source. Which it needs
	// to handle anyways.
	{
		String failReason;
		if ( !texture->CanCook( failReason ) )
		{
			ERR_ENGINE( TXT("%ls"), failReason.AsChar() );
			// Still return true, so we don't interrupt the cooking process.
			return true;
		}
	}

	const Uint16 residentMipMap = texture->GetResidentMipForCookingPlatform( m_platform );
	const Uint16 firstMip		= texture->GetHighestMipForCookingPlatform( m_platform );

	if ( residentMipMap == 0 )
	{
		return true;
	}

	CTextureBakerSourceTextureArray source( texture, firstMip );
	GTextureCacheCooker->StoreTextureData( texture->CalcTextureCacheKey(), texture->GetDepotPath(), source, CookFunction );

	return true;
}


//////////////////////////////////////////////////////////////////////////


Bool CTextureCacheBuilder::ProcessCubeTexture( CCubeTexture* texture )
{
	if ( GTextureCacheCooker == nullptr )
	{
		RED_HALT( "No TextureCacheCooker" );
		return false;
	}

	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( m_platform );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return false;
	}

	const Uint32 residentMipMap = texture->GetResidentMipForCookingPlatform( m_platform );

	// If full cube is resident, we have nothing to do.
	if ( residentMipMap == 0 )
	{
		return true;
	}

	CCubeTexture::CookedData fullCachedCube;
	if ( texture->ShouldRecreateForCook() )
	{
		texture->GenerateCachedData( fullCachedCube );
	}
	else
	{
		fullCachedCube = texture->GetCookedData();
	}

	CTextureBakerSourceCubeTexture source( fullCachedCube, 0 );
	GTextureCacheCooker->StoreTextureData( texture->CalcTextureCacheKey(), texture->GetDepotPath(), source, CookFunction );

	return true;
}


//////////////////////////////////////////////////////////////////////////


Bool CTextureCacheBuilder::ProcessLayer( CLayer* layer )
{
	// This stuff was mostly stolen from CLayerCooker. Basically, just need to go through all entities, find all
	// CEnvProbeComponents, and add those to the texture cache.
	//
	// We could have done this in the component's OnCook() and store it in the resource in a DeferredDataBuffer or something,
	// but we'd like to be able to use in-place texture creation on these (which we can't do with DeferredDataBuffer), and don't want
	// to load it initially (which GpuDataBuffer does)

	struct SScopedEntityStreaming : private Red::System::NonCopyable
	{
		CEntity* m_entity;
		// 	SEntityStreamingState streamingState;

		SScopedEntityStreaming( CEntity* entity )
			: m_entity( entity )
		{
			if ( m_entity )
			{
				m_entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
				m_entity->ForceFinishAsyncResourceLoads();
			}
		}

		~SScopedEntityStreaming()
		{
			if ( m_entity )
			{
				m_entity->UpdateStreamedComponentDataBuffers();
				m_entity->UpdateStreamingDistance();
				SEvents::GetInstance().ProcessPendingEvens(); // avoid OOM!
			}
		}
	};


	TDynArray< CEntity* > entitiesToAdd;
	Uint32 numExtractedMeshComponents = 0;

	// Remove empty component pointers from entity
	for ( CEntity* entity : layer->GetEntities() )
	{
		if ( ! entity )
		{
			continue;
		}

		{
			SScopedEntityStreaming scopedStreaming( entity );

			// Process components
			for ( ComponentIterator< CEnvProbeComponent > iter( entity ); iter; ++iter )
			{
				( *iter )->DumpToTextureCache( m_platform );
			}
		}
	}

	return true;
}
