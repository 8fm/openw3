#pragma once

#include "sectorDataRawData.h"

// Wrapper for streaming objects
// Wrappers are created only for objects in range the moment they got in range
// Stream() and Unstream() function are called to perform operations on the object.
// Wrapper are destroyed the moment object get out of range, if the object was streamed 
class ISectorDataObject
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_SectorData );

public:
	ISectorDataObject();
	virtual ~ISectorDataObject();

	struct Context
	{
		Context() : m_instantUnload( false ), m_forceStream( false ) { }
		class CDynamicLayer*				m_dynamicLayer;
		class IRenderScene*					m_renderScene;
		class CPhysicsWorld*				m_physicsScene;
		class CClipMap*						m_terrainClipMap;
		class CSectorDataResourceLoader*	m_resourceLoader;
		Bool								m_instantUnload;		// Sometimes we need to unload stuff with no delay (teleports, etc)
		Bool								m_forceStream;			// true if context is used by force stream for point
	};

	enum EResult
	{
		eResult_Finished,				//!< Object was successfully streamed/unstreamed
		eResult_RequiresSync,			//!< Object requires call from synchronous part of the streaming pipeline
		eResult_NotReady,				//!< Object is still not ready
		eResult_Failed,					//!< Object failed creation
	};

	RED_FORCE_INLINE void Setup( const SectorData::PackedObject& object )
	{
		m_packedObject = &object;
	}

	RED_FORCE_INLINE const SectorData::PackedObject& GetObject() const
	{
		return *m_packedObject;
	}

	// Runtime pipeline
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) = 0;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) = 0;

private:
	const SectorData::PackedObject*			m_packedObject;
};

// Templated (a little bit) wrapper for data
template< class T >
class TSectorDataObject : public ISectorDataObject
{
public:
	RED_FORCE_INLINE void Setup( const SectorData::PackedObject& packedObject, const T& packedData )
	{
		ISectorDataObject::Setup( packedObject );
		m_packedData = &packedData;
	}

	RED_FORCE_INLINE const T& GetData() const
	{
		return *m_packedData;
	}

private:
	const T*		m_packedData;
};

// Object wrapper - allows for "unboxing" of packed data into something more useful
class CSectorDataObjectWrapper
{
public:
	CSectorDataObjectWrapper( const class CSectorDataMerged* sourceData );
	~CSectorDataObjectWrapper();

	/// Create object wrapper 
	class ISectorDataObject* CreateObjectWrapper( const Uint32 globalMergedObjectID ) const;

private:
	const class CSectorDataMerged*		m_sourceData;
};

// Helper class holding a reference to resource
class CSectorDataResourceRef : public Red::NonCopyable
{
public:
	CSectorDataResourceRef();
	~CSectorDataResourceRef();

	void Bind( class CSectorDataResourceLoader* loader, SectorData::PackedResourceRef ref );
	void Release();

private:
	class CSectorDataResourceLoader*		m_resources;
	SectorData::PackedResourceRef			m_ref;
};