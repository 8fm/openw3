/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _ENGINE_FOLIAGE_RESOURCE_LOADER_H_
#define _ENGINE_FOLIAGE_RESOURCE_LOADER_H_

#include "foliageForward.h"
#include "foliageResource.h"
#include "../core/uniquePtr.h"

namespace Red{ namespace Core{ namespace ResourceManagement{ class CResourcePaths; } } }

class CDepot;
class CResourceLoader;

class CSoftHandleProxy
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:

	CSoftHandleProxy();
	CSoftHandleProxy( const String & filePath );
	CSoftHandleProxy( CFoliageResource * resource );
	RED_MOCKABLE ~CSoftHandleProxy();

	RED_MOCKABLE BaseSoftHandle::EAsyncLoadingResult GetAsync() const;

	RED_MOCKABLE CFoliageResource * Get() const;

	RED_MOCKABLE bool IsLoaded() const; 

	const String& GetPath() const;

private:

	TSoftHandle< CFoliageResource > m_softHandle;
};

class IFoliageResourceLoader
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	virtual FoliageResourceHandle GetResourceHandle( const Vector2 & position ) const = 0;
	virtual void ResourceAcquired( CFoliageResource * resource, const Vector2 & position ) const = 0; 
	virtual void ResourceReleased( CFoliageResource * resource ) const = 0;
	virtual FoliageResourceHandle CreateResource( const Vector2 & position ) const = 0;

protected:

	~IFoliageResourceLoader();
};

class CFoliageResourceLoader : public IFoliageResourceLoader
{
public:

	CFoliageResourceLoader();
	virtual ~CFoliageResourceLoader();

	virtual FoliageResourceHandle GetResourceHandle( const Vector2 & position ) const;
	virtual void ResourceAcquired( CFoliageResource * resource, const Vector2 & position ) const; 
	virtual void ResourceReleased( CFoliageResource * resource ) const;
	virtual FoliageResourceHandle CreateResource( const Vector2 & position ) const;

	// Foliage Loading Optimization. In the best of world, CFoliageResourceLoader should not know about CFoliageCell.
	// For the sake of Witcher3 release, I will overlook this since the interface IFoliageResourceLoader is still
	// unaware of the existence of CFoliageCell. 
	// The whole loading, while good enough for witcher3, might need to be revisited as the streaming saga is mostly over
	// and this class was made with the softhandle streaming iteration design/idea in mind. 
	RED_MOCKABLE void PrefetchAllResource( const CellHandleContainer & container );

	void SetInternalResourcePaths( const Red::Core::ResourceManagement::CResourcePaths * paths );
	void SetInternalHandler( CFoliageResourceHandler * handler );
	void SetInternalDepot( CDepot * depot  );
	void SetInternalResourceLoader( CResourceLoader * loader );

private:

	const Red::Core::ResourceManagement::CResourcePaths * m_resourcePath;
	mutable CFoliageResourceHandler * m_handler;
	CDepot * m_depot;
	CResourceLoader * m_resourceLoader;
};

String GenerateFoliageFilename( const Vector2 & position );

Red::TUniquePtr< CFoliageResourceLoader > CreateFoliageResourceLoader( CFoliageResourceHandler * handler );

#endif 
