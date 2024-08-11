/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementMap.h"
#include "renderEntityGroup.h"
#include "renderProxy.h"
#include "renderScene.h"
#include "renderProxyMesh.h"

namespace Config
{
	TConfigVar<Float> cvCharacterShadowsFallbackDistance		( "Rendering", "CharacterShadowsFallbackDistance",		8 );
}

CRenderEntityGroup::CRenderEntityGroup()
	: m_next( NULL )
	, m_useHiResShadows( false )
	, m_shadowsEnabled( true )
	, m_lastShadowFrame( -1 )
{}

CRenderEntityGroup::~CRenderEntityGroup()
{
	RED_ASSERT( !HasAnyProxy() );
	UnregisterFromRenderElementMap();
}

IRenderEntityGroup* CRenderInterface::CreateEntityGroup()
{
	return new CRenderEntityGroup();
}

void CRenderEntityGroup::AddProxy( IRenderEntityGroupProxy *proxy )
{
	RED_ASSERT( proxy );
	RED_ASSERT( !HasProxy( proxy ) );

	const Bool origUseShadowDistances = IsUsingShadowDistances();

	// Add proxy to appropriate list
	const eProxyCategory category = GetProxyCategory( *proxy );
	m_proxies[category].PushBackUnique( proxy );

	// Update proxies shadow distance info if needed
	const Bool currUseShadowDistances = IsUsingShadowDistances();
	if ( origUseShadowDistances != currUseShadowDistances )
	{
		UpdateProxiesUseShadowDistances();
	}
	else
	{
		proxy->SetUseShadowDistances( currUseShadowDistances );
	}
}

void CRenderEntityGroup::RemoveProxy( IRenderEntityGroupProxy *proxy )
{
	RED_ASSERT( proxy );
	RED_ASSERT( m_proxies[ GetProxyCategory( *proxy ) ].GetIndex( proxy ) != -1 );

	const Bool origUseShadowDistances = IsUsingShadowDistances();

	// Remove proxy from appropriate list
	const eProxyCategory category = GetProxyCategory( *proxy );
	m_proxies[ category ].Remove( proxy );

	// Mark this proxies not to use shadow distances
	proxy->SetUseShadowDistances( false );

	// Update remaining proxies if needed
	const Bool currUseShadowDistances = IsUsingShadowDistances();
	if ( origUseShadowDistances != currUseShadowDistances )
	{
		UpdateProxiesUseShadowDistances();
	}
}

CRenderEntityGroup::eProxyCategory CRenderEntityGroup::GetProxyCategory( const IRenderEntityGroupProxy &proxy )
{
	return proxy.IsCharacterShadowFallback() ? PROXYCAT_Fallback : PROXYCAT_Main;
}

Bool CRenderEntityGroup::HasAnyProxy() const
{
	for ( Uint32 i=0; i<PROXYCAT_MAX; ++i )
	{
		if ( !m_proxies[i].Empty() )
		{
			return true;
		}
	}

	return false;
}

Bool CRenderEntityGroup::HasProxy( IRenderEntityGroupProxy *proxy ) const
{
	for ( Uint32 i=0; i<PROXYCAT_MAX; ++i )
	{
		if ( m_proxies[i].GetIndex( proxy ) != -1 )
		{
			return true;
		}
	}

	return false;
}

void CRenderEntityGroup::CollectElements( CRenderCollector &collector )
{
	RED_FATAL_ASSERT( collector.m_scene != nullptr , "No m_scene DUH!" );

	const auto ret = m_repeatedFrameTracker.UpdateOncePerFrame( collector.m_scene->GetRepeatedFrameCounter() );
	if ( ret == FUS_AlreadyUpdated )
	{
		return;
	}

	// Hi-res shadows ?
	if ( IsUsingHiResShadows() )
	{
		m_next = collector.m_hiResShadowLists;
		collector.m_hiResShadowLists = this;
	}
}

void CRenderEntityGroup::UpdateOncePerFrame( const CRenderCollector &collector )
{
	// const Uint32 frameIndex = collector.m_scene ? collector.m_scene->GetRepeatedFrameCounter() : collector.m_frameIndex;

	const auto ret = m_frameTracker.UpdateOncePerFrame( collector.m_frameIndex );
	if ( ret == FUS_AlreadyUpdated )
	{
		return;
	}

	UpdateShadowFade( collector , ret == FUS_UpdatedLastFrame );

}

void CRenderEntityGroup::UpdateShadowFade( const CRenderCollector &collector, const Bool wasVisibleLastFrame )
{
	// Test if we should be using fallback shadows
	Bool enableFallbackShadows = true;
	RED_ASSERT( !m_proxies[ PROXYCAT_Fallback ].Empty() );
	if ( !m_proxies[ PROXYCAT_Fallback ].Empty() )
	{
		CRenderProxy_Mesh *meshProxy = m_proxies[PROXYCAT_Fallback][0]->AsMeshProxy();

		RED_ASSERT( meshProxy );
		if ( meshProxy )
		{
			const Vector referencePos = meshProxy->GetLocalToWorld().GetTranslation();

			const CRenderCamera &camera = collector.GetRenderCamera();
			const Float distThresholdSq = Config::cvCharacterShadowsFallbackDistance.Get() * Config::cvCharacterShadowsFallbackDistance.Get();
			const Float distSq = camera.GetPosition().DistanceSquaredTo( referencePos );
			enableFallbackShadows = distSq * camera.GetFOVMultiplier() > distThresholdSq;
		}
	}

	// Update shadow fades
	if ( wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated() )
	{
		const auto &sc = collector.GetDissolveSynchronizer();

		if ( enableFallbackShadows )
		{
			m_shadowFade.FadeOut( sc );
		}
		else
		{
			m_shadowFade.FadeIn( sc );
		}
	}
	else
	{
		if ( enableFallbackShadows )
		{
			m_shadowFade.SetAlphaZero();			
		}
		else
		{
			m_shadowFade.SetAlphaOne();
		}
	}
}

CRenderDissolveAlpha CRenderEntityGroup::GetShadowFadeAlpha( const CRenderDissolveSynchronizer &sc, Bool isShadowFallback ) const
{
	if ( !m_shadowsEnabled )
		return CRenderDissolveAlpha ( 0 );
	CRenderDissolveAlpha dissolveAlpha = m_shadowFade.ComputeAlpha( sc );
	return isShadowFallback ? dissolveAlpha.GetMappedUpperHalfRange().Inverted() : dissolveAlpha.GetMappedLowerHalfRange();
}

void CRenderEntityGroup::UpdateProxiesUseShadowDistances()
{
	// Get info whether we should use shadow distances
	const Bool isUsingShadowDistances = IsUsingShadowDistances();

	// Mark all proxies
	for ( Uint32 category_i=0; category_i<PROXYCAT_MAX; ++category_i )
	{
		TDynArray< IRenderEntityGroupProxy* > &proxies = m_proxies[category_i];
		for ( Uint32 proxy_i=0; proxy_i<proxies.Size(); ++proxy_i )
		{
			proxies[proxy_i]->SetUseShadowDistances( isUsingShadowDistances );
		}
	}
}

void CRenderEntityGroup::RegisterToRenderElementMap()
{
	if ( m_renderElementMap )
	{
		m_renderElementMap->RegisterEntityGroup( this );
	}	
}

void CRenderEntityGroup::UnregisterFromRenderElementMap()
{
	if ( m_renderElementMap ) 
	{
		m_renderElementMap->UnregisterEntityGroup( this );
	}	
}

void CRenderEntityGroup::SetRenderElementMap( CRenderElementMap* renderElementMap )
{
	if ( m_renderElementMap != renderElementMap )
	{
		UnregisterFromRenderElementMap();
		m_renderElementMap = renderElementMap;
		RegisterToRenderElementMap();
	}
}

const Box& CRenderEntityGroup::CalculateBoundingBox()
{
	m_box.Clear();
	for ( Uint32 category_i=0; category_i<PROXYCAT_MAX; ++category_i )
	{
		for ( auto proxy : m_proxies[category_i] )
		{
			m_box.AddBox( proxy->AsBaseProxy()->GetBoundingBox() );
		}
	}
	return m_box;
}

IRenderEntityGroupProxy::IRenderEntityGroupProxy ( IRenderEntityGroup *entityGroup )
	: m_entityGroup( nullptr )
{
	// If we are a part of entity group remember the pointer to it
	if ( entityGroup != nullptr )
	{
		// Keep the link
		m_entityGroup = static_cast< CRenderEntityGroup* >( entityGroup );
		m_entityGroup->AddRef();
	}
}

IRenderEntityGroupProxy::~IRenderEntityGroupProxy ()
{
	// we had and entity group, make sure we got detached
	if ( m_entityGroup != nullptr )
	{
		ASSERT( !m_entityGroup->HasProxy( this ) );
		m_entityGroup->Release();
		m_entityGroup = nullptr;
	}
}

void IRenderEntityGroupProxy::SetEntityGroup( CRenderEntityGroup* group, Bool isAttachedToScene )
{
	if ( group != m_entityGroup )
	{
		// Remove from entity group if we were in one
		if ( m_entityGroup != nullptr )
		{
			RED_ASSERT( isAttachedToScene == m_entityGroup->HasProxy( this ) );
			m_entityGroup->RemoveProxy( this ); //< Not really needed - leaving this just in case
			m_entityGroup->Release();
			m_entityGroup = nullptr;
		}

		// Set new group reference
		m_entityGroup = group;

		// If we are a part of entity group remember the pointer to it
		if ( m_entityGroup != nullptr )
		{
			// Keep the link
			m_entityGroup = static_cast< CRenderEntityGroup* >( m_entityGroup );
			m_entityGroup->AddRef();
			if ( isAttachedToScene )
			{
				m_entityGroup->AddProxy( this );
			}			
		}
	}

	RED_ASSERT( !m_entityGroup || isAttachedToScene == m_entityGroup->HasProxy( this ) );
}

void IRenderEntityGroupProxy::AttachToScene()
{
	// Add to entity group proxy list
	if ( m_entityGroup != nullptr )
	{
		m_entityGroup->AddProxy( this );
	}
}

void IRenderEntityGroupProxy::DetachFromScene()
{	
	// Remove from entity group if we were in one
	if ( m_entityGroup != nullptr )
	{
		m_entityGroup->RemoveProxy( this ); 		
	}
}
