/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/core/settings.h"

#if !defined( NO_SECOND_SCREEN )

#include "secondScreenDeviceWatcherDurango.h"
#include "secondScreenDeviceDurango.h"
#include <Windows.h>
#include <Unknwn.h>


using namespace Windows::Foundation;
using namespace Windows::Xbox::SmartGlass;

ref class SecondScreenDeviceWatcherRefClass sealed
{
public:
	SecondScreenDeviceWatcherRefClass()
	{
		m_parent = NULL;
	}
public:
	void Initialize( Platform::IntPtr parent )
	{
		m_parent = (CSecondScreenDeviceWatcherDurango*)((void*)parent);
		m_watcher = ref new SmartGlassDeviceWatcher();
		m_watcher->DeviceAdded += ref new TypedEventHandler<SmartGlassDeviceWatcher^, SmartGlassDevice^>( this, &SecondScreenDeviceWatcherRefClass::OnDeviceAddedDurango );
		m_watcher->DeviceRemoved += ref new TypedEventHandler<SmartGlassDeviceWatcher^, SmartGlassDevice^>( this, &SecondScreenDeviceWatcherRefClass::OnDeviceRemovedDurango );
	}
public:
	void OnDeviceAddedDurango(SmartGlassDeviceWatcher^ sender, SmartGlassDevice^ device)
	{
		if( m_parent != NULL)
		{
			CSecondScreenDevice* secondScreenDevice = new CSecondScreenDeviceDurango( reinterpret_cast<void*>(device),  m_parent->GetDelegate() );
			m_parent->OnDeviceAddedDurango( secondScreenDevice );
		}
	}
	void OnDeviceRemovedDurango(SmartGlassDeviceWatcher^ sender, SmartGlassDevice^ device)
	{
		if( m_parent != NULL)
		{
			CSecondScreenDevice* secondScreenDevice = new CSecondScreenDeviceDurango( reinterpret_cast<void*>(device), m_parent->GetDelegate() );
			m_parent->OnDeviceRemovedDurango( *secondScreenDevice );
			delete secondScreenDevice;
		}
	}

	void OnFindAllCompletionDurango( IAsyncOperation<SmartGlassDeviceCollection^>^ asyncOp, AsyncStatus status )
	{
		UNREFERENCED_PARAMETER( status );

		SmartGlassDeviceCollection^ devices = asyncOp->GetResults();
		UINT devicesSize = devices->Size;
		for( UINT i = 0; i < devicesSize; i++ )
		{
			OnDeviceAddedDurango(m_watcher, devices->GetAt(i));
		}
	}

	void FindAllDevicesAsyncDurango()
	{
		SmartGlassDevice::FindAllAsync()->Completed = ref new AsyncOperationCompletedHandler<SmartGlassDeviceCollection^>( this, &SecondScreenDeviceWatcherRefClass::OnFindAllCompletionDurango );
	}

	
private:
	SmartGlassDeviceWatcher^ m_watcher;
	CSecondScreenDeviceWatcherDurango* m_parent;
};

CSecondScreenDeviceWatcherDurango::CSecondScreenDeviceWatcherDurango( ISecondScreenDeviceDelegate* delegeate ) : CSecondScreenDeviceWatcher( delegeate )
{
	m_watcherDurango = NULL;
	SecondScreenDeviceWatcherRefClass^ secondScreenDeviceWatcherRefClass = ref new  SecondScreenDeviceWatcherRefClass();
	Platform::IntPtr parentPtr = Platform::IntPtr((void*)this);
	secondScreenDeviceWatcherRefClass->Initialize( parentPtr );
	IUnknown* iUnknown = reinterpret_cast<IUnknown*>( secondScreenDeviceWatcherRefClass );
	iUnknown->AddRef();
	m_watcherDurango = (void*)iUnknown;
}

CSecondScreenDeviceWatcherDurango::~CSecondScreenDeviceWatcherDurango()
{
	((IUnknown*)m_watcherDurango)->Release();
	m_watcherDurango = NULL;
}

void CSecondScreenDeviceWatcherDurango::FindAllDevicesAsync()
{
	SecondScreenDeviceWatcherRefClass^ secondScreenDeviceWatcherRefClass = reinterpret_cast<SecondScreenDeviceWatcherRefClass^>( m_watcherDurango );
	secondScreenDeviceWatcherRefClass->FindAllDevicesAsyncDurango();
}

void CSecondScreenDeviceWatcherDurango::OnDeviceAddedDurango( CSecondScreenDevice* device )
{
	if( m_delegate )
	{
		m_delegate->OnDeviceAdded( this, device );
	}
}

void CSecondScreenDeviceWatcherDurango::OnDeviceRemovedDurango( const CSecondScreenDevice& device )
{
	if( m_delegate )
	{
		m_delegate->OnDeviceRemoved( this, device );
	}
}

#endif


