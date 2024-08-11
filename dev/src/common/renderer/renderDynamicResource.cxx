/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderResourceIterator.h"
#include "renderTexture.h"

Red::Threads::CLightMutex		 IDynamicRenderResource::s_resourceMutex;
IDynamicRenderResource*	IDynamicRenderResource::s_allResources = NULL;

IDynamicRenderResource::IDynamicRenderResource()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( s_resourceMutex );

	// Add to resource list
	if ( s_allResources ) s_allResources->m_prevResource = &m_nextResource;
	m_nextResource = s_allResources;
	m_prevResource = &s_allResources;
	s_allResources = this;
}

IDynamicRenderResource::~IDynamicRenderResource()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( s_resourceMutex );

	// Unlink
	if ( m_nextResource ) m_nextResource->m_prevResource = m_prevResource;
	if ( m_prevResource ) *m_prevResource = m_nextResource;
	m_prevResource = NULL;
	m_nextResource = NULL;
}


void IDynamicRenderResource::DestroySelfIfRefCountZero()
{
	RED_FATAL_ASSERT( IsRefCountZero(), "" );
	delete this;
}


void IDynamicRenderResource::GetAllResources( TDynArray< IDynamicRenderResource* >& allResources )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( s_resourceMutex );

	for ( IDynamicRenderResource* cur=IDynamicRenderResource::s_allResources; cur; cur=cur->m_nextResource )
	{
		allResources.PushBack( cur );
		cur->AddRef();
	}
}

