/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxy.h"
#include "renderScene.h"
#include "renderElementMap.h"

#if 0
LONG GNumProxies = 0;

class ProxyList
{
public:
	Red::Threads::CMutex			m_mutex;
	TDynArray< IRenderProxyBase* >	m_allProxies;

public:
	ProxyList() {};
	~ProxyList() {};

	void Add( IRenderProxyBase* base )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		m_allProxies.PushBack( base );
	}

	void Release( IRenderProxyBase* base )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		m_allProxies.Remove( base );
	}

	void PrintList()
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

		for ( Uint32 i=0; i<m_allProxies.Size(); ++i )
		{
			IRenderProxyBase* base = m_allProxies[i];
			LOG_RENDERER( TXT("Proxy[%i]: 0x%X, id %i"), i, base, base->m_proxyGenerationID );
		}
	}

public:
	static ProxyList GProxyList;
};

ProxyList ProxyList::GProxyList;

void PrintProxyList()
{
	ProxyList::GProxyList.PrintList();
}

#endif

IRenderProxyBase::IRenderProxyBase( ERenderProxyType type, const RenderProxyInitInfo& initInfo )
	: IRenderProxy( type )
	, m_scene( nullptr )
	, m_localToWorld( initInfo.ExtractLocalToWorld() )
	, m_boundingBox( initInfo.ExtractBoundingBox() )
	, m_registrationRefCount( 0 )
	, m_autoHideDistance( 2000.0f )
	, m_autoHideDistanceSquared( 4000000.0f )
	, m_entryID( 0 )
{
	m_boundingBox.Min.W = 1.0f;
	m_boundingBox.Max.W = 1.0f;
}

IRenderProxyBase::~IRenderProxyBase()
{
	RED_ASSERT( m_registrationRefCount.GetValue() == 0, TXT("Destroying RenderProxyDrawable which has not been unregistered from RenderElementMap") );
	RED_ASSERT( !m_scene );
}

void IRenderProxyBase::AttachToScene( CRenderSceneEx* scene )
{
	RED_ASSERT( !m_scene, TXT("Attaching to null scene is invalid") );

	// Link to scene
	m_scene = scene;
	m_frameTracker.SetFrameIndex( scene->GetLastAllocatedFrame() );

	AddRef();
}

void IRenderProxyBase::DetachFromScene( CRenderSceneEx* scene )
{
	RED_ASSERT( m_scene == scene, TXT("Attached to another scene") );

	// Unlink from scene
	m_scene = nullptr;
	Release();
}

const EFrameUpdateState IRenderProxyBase::UpdateOncePerFrame( const CRenderCollector& collector )
{
	return m_frameTracker.UpdateOncePerFrame( collector.m_frameIndex );
}


void IRenderProxyBase::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	// Update internal data - bounding box, location
	m_localToWorld = localToWorld;
	m_boundingBox = boundingBox;
	m_boundingBox.Min.W = 1.0f;
	m_boundingBox.Max.W = 1.0f;

	// Relink in scene
	if ( m_scene )
	{
		m_scene->RelinkProxy( this );
	}
}

void IRenderProxyBase::RelinkTransformOnly( const Matrix& localToWorld )
{
	// Update internal data - bounding box, location
	m_localToWorld = localToWorld;
	// Make sure bounding box won't stay off screen when proxy is moving in and out
	m_boundingBox.AddPoint( m_localToWorld.GetTranslation() );

	// Relink in scene
	if ( m_scene )
	{
		m_scene->RelinkProxy( this );
	}
}

void IRenderProxyBase::RelinkBBoxOnly( const Box& boundingBox )
{
	m_boundingBox = boundingBox;
	m_boundingBox.Min.W = 1.0f;
	m_boundingBox.Max.W = 1.0f;

	// Relink in scene
	if ( m_scene )
	{
		m_scene->RelinkProxy( this );
	}
}

void IRenderProxyBase::UpdateHitProxyID( const CHitProxyID& id )
{

}

void IRenderProxyBase::UpdateSelection( Bool isSelected)
{

}

Bool IRenderProxyBase::Register( CRenderElementMap* reMap )
{
	if ( !reMap )
	{
		return false;
	}

	return reMap->Register( this );
}

Bool IRenderProxyBase::Unregister( CRenderElementMap* reMap, Bool deferredUnregister /*=false*/ )
{
	if ( !reMap )
	{
		return false;
	}

	return reMap->Unregister( this, deferredUnregister );
}
