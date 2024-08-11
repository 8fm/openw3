/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageCell.h"
#include "foliageResourceLoader.h"

CFoliageCell::CFoliageCell()
	:	m_loader( nullptr ),
		m_loadingResult( BaseSoftHandle::ALR_InProgress )
{}

CFoliageCell::~CFoliageCell()
{
	if( IsResourceLoaded() )
	{
		m_loader->ResourceReleased( m_resourceHandle->Get() );
	}
}

void CFoliageCell::Tick()
{
	if( IsResourceValid() && m_loadingResult != BaseSoftHandle::ALR_Loaded )
	{
		if( m_loadingResult == BaseSoftHandle::ALR_Failed )
		{
			RED_WARNING_ONCE( false, "Failed to load foliage cell file '%ls'", GenerateFoliageFilename( m_position ).AsChar() );
		}
		else
		{
			m_loadingResult = m_resourceHandle->GetAsync();
			if( m_loadingResult == BaseSoftHandle::ALR_Loaded )
			{
				CFoliageResource * foliageResource = m_resourceHandle->Get();
				m_loader->ResourceAcquired( foliageResource, m_position );
			}
		}
	}
}

void CFoliageCell::Setup( const Vector2 & postion, const IFoliageResourceLoader * loader )
{
	m_position = postion;
	m_loader = loader;
	m_resourceHandle = m_loader->GetResourceHandle( m_position );
	if( m_resourceHandle && m_resourceHandle->IsLoaded() )
	{
		m_loadingResult = BaseSoftHandle::ALR_Loaded;
		m_loader->ResourceAcquired( m_resourceHandle->Get(), m_position );
	}
}

CFoliageResource * CFoliageCell::AcquireFoliageResource()
{
	if( !m_resourceHandle )
	{
		m_resourceHandle = m_loader->CreateResource( m_position );
		m_loadingResult = BaseSoftHandle::ALR_Loaded;
		m_loader->ResourceAcquired( m_resourceHandle->Get(), m_position );
	}

	return m_resourceHandle->Get();
}

CFoliageResource * CFoliageCell::GetFoliageResource() const
{
	if( IsResourceLoaded() )
	{
		return m_resourceHandle->Get();
	}

	return nullptr;
}

Vector2 CFoliageCell::GetWorldCoordinate() const
{
	return m_position;
}

bool CFoliageCell::IsResourceValid() const
{
	return m_resourceHandle;
}

const String & CFoliageCell::GetPath() const
{
	return IsResourceValid() ? m_resourceHandle->GetPath() : String::EMPTY;
}

void CFoliageCell::Wait()
{
	if( IsResourceValid() )
	{
		while( !IsResourceLoaded() && m_loadingResult != BaseSoftHandle::ALR_Failed )
		{
			Red::Threads::SleepOnCurrentThread( 1 );
			Tick();
		}
	}
}

bool CFoliageCell::IsResourceLoaded() const
{
	return IsResourceValid() && m_loadingResult == BaseSoftHandle::ALR_Loaded;
}

bool CFoliageCell::IsLoading() const
{
	return IsResourceValid() && m_loadingResult == BaseSoftHandle::ALR_InProgress;
}

CellHandle CreateFoliageCell( const Vector2 & position, const IFoliageResourceLoader * loader )
{
	CellHandle handle( new CFoliageCell );  
	handle->Setup( position, loader );
	return handle;
}
