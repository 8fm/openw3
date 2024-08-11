/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "renderProxy.h"
#include "../engine/umbraStructures.h"
#include "../core/idAllocator.h"

// forward declarations
class CRenderCollector;
class CRenderDynamicDecalChunk;
class CRenderDynamicDecal;
class IRenderProxyBase;
class IRenderProxyDrawable;
struct SMergedShadowCascades;
struct SShadowEntityProxyCuller;
class ExtraOcclusionQueryResults;
class CRenderEntityGroup;
class IShadowmapQuery;

#ifndef RED_FINAL_BUILD
#define PROFILE_CODE_ENABLED
#endif // RED_FINAL_BUILD

#ifdef PROFILE_CODE_ENABLED
struct SRenderElementMapStats
{
	Uint32 m_dynamicMeshes;
	Uint32 m_particles;
	Uint32 m_apex;
	Uint32 m_flares;
	Uint32 m_fur;
	Uint32 m_nonBakedStripes;
	Uint32 m_bakedStripes;
	Uint32 m_meshesNotInObjectCache;
	Uint32 m_bakedPointLights;
	Uint32 m_nonBakedPointLights;
	Uint32 m_bakedSpotLights;
	Uint32 m_nonBakedSpotLights;
	Uint32 m_nonBakedDecals;
	Uint32 m_staticMeshes;
	Uint32 m_bakedDecals;
	Uint32 m_bakedDimmers;
	Uint32 m_nonBakedDimmers;
};
#endif // PROFILE_CODE_ENABLED


struct SExtraSceneCollection
{
	TDynArray< const IRenderProxyBase* >	m_proxies;
	TDynArray< const CRenderDynamicDecal* >	m_dynamicDecals;
};

enum ERenderElementMapEntryFlags
{
	REMEF_Valid						= FLAG( 0 ),
	REMEF_Dynamic					= FLAG( 1 ),
	REMEF_Static					= FLAG( 2 ),
	REMEF_Pass_RegularCollection	= FLAG( 3 ),
	REMEF_Pass_DeferredCollection	= FLAG( 4 ),
	REMEF_CharacterProxy			= FLAG( 5 ),
	REMEF_SkipOcclusionTest			= FLAG( 6 ),
	REMEF_PastAutoHide				= FLAG( 7 ),
	REMEF_FadingOut					= FLAG( 8 ),
	REMEF_ShadowCaster				= FLAG( 9 ),
	REMEF_VolumeMesh				= FLAG( 10 ),		// Is this a volume mesh
	REMEF_NonStaticVolumeMesh		= FLAG( 11 )		// Is this a volume mesh that didn't make it into the statics list
};

// values measured in the center of novigrad city with all layers loaded and extra buffer added
#ifndef NO_EDITOR
// 8 * 64 * 1024 * sizeof(SRenderElementMapEntry) = 33554432b = 32768kB = 32MB
#define MAX_REGISTERED_PROXIES 8 * 64 * 1024
#define MAX_REGISTERED_STATIC_PROXIES 8 * 48 * 1024
#else
#define MAX_REGISTERED_PROXIES 64 * 1024
#define MAX_REGISTERED_STATIC_PROXIES 48 * 1024
#endif

#define MAX_COLLECTED_PROXIES 64 * 1024

class CRenderElementMap
{
public:

	RED_ALIGNED_STRUCT(SRenderElementMapEntry,64)
	{
		ERenderProxyType		m_type;						//4
		Float					m_autoHideDistanceSquared;	//4
		Uint32					m_umbraObjectId;			//4
		CRenderProxyTypeFlag	m_typeFlag;					//4
		IRenderProxyBase*		m_proxy;					//8
		Vector3					m_refPos;					//12
		Vector3					m_boundingBoxMin;			//12
		Vector3					m_boundingBoxMax;			//12
		Uint16					m_flags;					//2
		Uint8					m_renderMask;				//1
		Uint8					m_testLastFrame : 4;		//4b
#ifdef USE_ANSEL
		Bool					m_skipCollection : 1;		//1b
#endif // USE_ANSEL

		RED_FORCE_INLINE SRenderElementMapEntry()
		{
			Red::System::MemorySet( this, 0, sizeof( SRenderElementMapEntry ) );
		}

		RED_FORCE_INLINE Bool HasFlag( ERenderElementMapEntryFlags flag ) const	{ return ( m_flags & flag ) != 0;	}
		RED_FORCE_INLINE void SetFlag( ERenderElementMapEntryFlags flag )		{ m_flags |= flag;					}
		RED_FORCE_INLINE void ClearFlag( ERenderElementMapEntryFlags flag )		{ m_flags &= ~flag;					}
	};
	static_assert( sizeof( SRenderElementMapEntry ) == 64, "Invalid size of RenderElementMap entry, this structure is cache optimized, do not tamper with it" );

public:
#ifdef USE_UMBRA
	RED_INLINE void SetObjectCache( TObjectCache& objectCache )			{ m_objectCache = Move( objectCache );		}
#endif // USE_UMBRA

	RED_INLINE Uint32 GetStaticProxiesCount() const						{ return m_staticProxies.Size();			}
	RED_INLINE Uint32 GetDynamicDecalsCount() const						{ return m_dynamicDecalChunks.Size();		}

	RED_INLINE void RegisterEntityGroup( CRenderEntityGroup* group )	{ m_characterEntities.PushBackUnique( group ); }
	RED_INLINE void UnregisterEntityGroup( CRenderEntityGroup* group )	{ m_characterEntities.RemoveFast( group ); }

#ifdef PROFILE_CODE_ENABLED
	RED_INLINE const SRenderElementMapStats& GetStats() const			{ return m_stats; }
#endif // PROFILE_CODE_ENABLED

public:
	CRenderElementMap();
	~CRenderElementMap();

	// Collect visible proxies, without modifying any cached distances etc.
	void CollectProxiesNoModify( const ExtraOcclusionQueryResults* occlusionResults, CRenderFrame* frame, SExtraSceneCollection& collector );

	void CollectStaticProxies( CRenderCollector& collector );
	void CollectNonStaticProxies( CRenderCollector& collector );
	void CollectShadowProxies( SMergedShadowCascades& cascades );
	
	Bool Register( IRenderProxyBase* proxy );
	Bool Unregister( IRenderProxyBase* proxy, Bool deferredUnregister = false );

	void RegisterDynamicDecalChunk( CRenderDynamicDecalChunk* chunk );
	void UnregisterDynamicDecalChunk( CRenderDynamicDecalChunk* chunk );

	void UpdateProxyInfo( const Uint32 id, const Vector& referencePosition, const Box& boundingBox );

	// Set whether a proxy is fading out or not. Only has an effect for non-character dynamic objects, to allow them to be collected
	// while fading out.
	void SetProxyFadingOut( const Uint32 id, const Bool fading );

	// Set whether a proxy is past its auto-hide distance or not. Only has an effect for non-character dynamic objects. When a proxy
	// has been flagged as past auto-hide, it will have a distance check performed and not be collected if it's too far away. If it
	// is not flagged, no distance check is done.
	void SetProxyPastAutoHide( const Uint32 id, const Bool pastAutoHide );

#ifdef USE_UMBRA
	void FeedVisibleObjects( TVisibleChunksIndices& visibleObjectsIndices, Int32 objectsCount );
	void BuildRemapArray( const Umbra::TomeCollection* tomeCollection, const TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIdToIndexMap );

#ifndef NO_EDITOR
	void DumpVisibleMeshes( const TLoadedComponentsMap& componentsMap, const String& path );
#endif // NO_EDITOR
#endif //USE_UMBRA

public:
	void Collect( const Box& box, CRenderProxyTypeFlag proxyTypeFlags, TDynArray< IRenderProxyBase* >& proxies, Uint32 flags = REMEF_Valid ) const;
	void Collect( const CFrustum& frustum, CRenderProxyTypeFlag proxyTypeFlags, TDynArray< IRenderProxyBase* >& proxies, Uint32 flags = REMEF_Valid ) const;

	void Collect( IShadowmapQuery** queries, Uint32 numQueries, CRenderProxyTypeFlag proxyTypeFlags, Uint32 flags = REMEF_Valid ) const;

private:
	enum CollectionFlags
	{
		COLLECT_Static			= FLAG( 0 ),
		COLLECT_NonStatic		= FLAG( 1 )
	};

#ifdef USE_UMBRA
	template< typename TCollector >
	void CollectProxiesGeneric( TCollector& collector, Uint8 flags );
#endif // USE_UMBRA

	template< typename TCollector >
	void CollectProxiesGenericFrustum( TCollector& collector, Uint8 flags );

	TDynArray< CRenderEntityGroup* >		m_characterEntities;
	THashMap< Uint32, Uint32 >				m_staticProxies;
	TDynArray< CRenderDynamicDecalChunk* >	m_dynamicDecalChunks;
	TDynArray< Uint32 >						m_statics;

	THashSet< Uint32 >						m_nonStaticVolumes;					// Indices in m_entries with REMEF_NonStaticVolumeMesh

	Uint8									m_entriesMemoryPlaceholder[ ( 1 + MAX_REGISTERED_PROXIES ) * sizeof( SRenderElementMapEntry ) ];
	SRenderElementMapEntry*					m_entries;

	IDAllocator< MAX_REGISTERED_PROXIES >	m_idAllocator;
	Uint32									m_lastEntryId;

#ifdef USE_UMBRA
	const Umbra::TomeCollection*			m_tomeCollection;
#endif // USE_UMBRA

	TObjectCache							m_objectCache;

	TVisibleChunksIndices					m_visibleObjectsIndices;
	Int32									m_visibleObjectsCount;

	TObjectIDToIndexMap						m_objectIdToIndexMap;

#ifdef PROFILE_CODE_ENABLED
	SRenderElementMapStats					m_stats;
	void UpdateStats( const SRenderElementMapEntry& entry, Int32 value );
#endif // PROFILE_CODE_ENABLED
};
