/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/core/settings.h"
#include "../../common/core/jsonReader.h"

#if !defined( NO_SECOND_SCREEN )
#include "secondScreenDeviceDurango.h"
#include "../../common/redSystem/log.h"
#include <Windows.h>
#include <Unknwn.h>

const size_t CSecondScreenDevice::s_maxMessageLength = 65532;

#define MAX_MESSAGES_PER_DEVICE 8

using namespace Windows::Foundation;
using namespace Windows::Xbox::SmartGlass;
using namespace Windows::Data::Json;

class CSecondScreenSendMessageJob : public CTask
{
public:
	CSecondScreenSendMessageJob( void* pHtmlSurface, const String& message ): m_IAsyncAction(nullptr)
	{
		m_pHtmlSurface = pHtmlSurface;
		if( m_pHtmlSurface != NULL )
		{
			IUnknown* iUnknown = static_cast<IUnknown*>( m_pHtmlSurface );
			iUnknown->AddRef();
		}
		m_message = message;
	}

	~CSecondScreenSendMessageJob()
	{
		if( m_pHtmlSurface != NULL )
		{
			IUnknown* iUnknown = static_cast<IUnknown*>( m_pHtmlSurface );
			iUnknown->Release();
		}
	}

protected:
	//! Process the job, is called from job thread
	void Run()
	{
		m_IAsyncAction = nullptr;
		//PC_SCOPE( CSecondScreenSendMessageJob_Run );
		if( m_pHtmlSurface != NULL )
		{
			try
			{
				SmartGlassHtmlSurface^ pHtmlSurface = reinterpret_cast<SmartGlassHtmlSurface^>(m_pHtmlSurface);
				m_IAsyncAction = pHtmlSurface->SubmitMessageAsync( ref new Platform::String(m_message.AsChar()) );
				
			}
			catch (Platform::COMException^ e)
	 		{
				RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Can not send message, SmartGlassHtmlSurface throws an exception %x with message %s" ), e->HResult, e->Message->Begin() );
	 		}
		}		
	}

public:
	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT( "CSecondScreenSendMessageJob" ); };

	//! Get debug color
	virtual Uint32 GetDebugColor() const { return Color::LIGHT_BLUE.ToUint32(); };

	bool IsSubmitMessageAsyncStarted()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Started )
			{
				return true;
			}
		}
		return false;
	}

	bool IsSubmitMessageAsyncCompleted()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Completed )
			{
				return true;
			}
		}
		return false;
	}

	bool IsSubmitMessageAsyncCanceled()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Canceled )
			{
				return true;
			}
		}
		return false;
	}

	bool IsSubmitMessageAsyncError()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Error )
			{
				return true;
			}
		}
		return false;
	}

	Int32 GetSubmitMessageAsyncErrorCode()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Error )
			{
				return  m_IAsyncAction->ErrorCode.Value;
			}
		}
		return S_OK;
	}

	void CancelSubmitMessageAsync()
	{
		if( m_IAsyncAction != nullptr )
		{
			if( m_IAsyncAction->Status == AsyncStatus::Started )
			{
				m_IAsyncAction->Cancel();
			}
		}
	}

private:
	String m_message;
	void* m_pHtmlSurface;
	IAsyncAction^ m_IAsyncAction;
};

ref class SecondScreenDeviceRefClass sealed
{
public:
	SecondScreenDeviceRefClass()
	{
		m_parent = NULL;
	}

	virtual ~SecondScreenDeviceRefClass()
	{
		TDynArray<CSecondScreenSendMessageJob*>::iterator end = m_tasks.End();
		for( TDynArray<CSecondScreenSendMessageJob*>::iterator iter = m_tasks.Begin(); iter != end; ++iter )
		{
			CSecondScreenSendMessageJob* task = *iter;
			if( task != NULL )
			{
				if( task->IsSubmitMessageAsyncStarted() )
				{
					task->CancelSubmitMessageAsync();
				}
				
				if( !task->IsFinished() )
				{
					task->TryCancel();
				}	
				task->Release();
			}
		}
		m_tasks.Clear();
	}
	
	void Initialize( SmartGlassDevice^ pDevice, Platform::IntPtr parent )
	{
		m_parent = (CSecondScreenDeviceDurango*)((void*)parent);

		//Get the HtmlSurface
		SmartGlassHtmlSurface^ pHtmlSurface = pDevice->HtmlSurface;

		//Set the Mode to be Html Surface.
		pDevice->SetActiveSurfaceAsync( pHtmlSurface );  

		//Wire up message received event
		pHtmlSurface->MessageReceived += ref new TypedEventHandler<SmartGlassHtmlSurface^,  SmartGlassMessageReceivedEventArgs^>(this, &SecondScreenDeviceRefClass::OnMessageReceivedDurango);

		m_pSmartGlassDevice = pDevice;

	}
	
	bool CompareDevices( SecondScreenDeviceRefClass^ rhl )
	{
		return (m_pSmartGlassDevice->Id == rhl->m_pSmartGlassDevice->Id);
	}

	bool SendMessageAsyncDurango( Platform::String^ message )
	{
		//PC_SCOPE( SecondScreenDeviceRefClass_SendMessageAsyncDurango );
 		bool success = false;
		if( m_tasks.Size() < MAX_MESSAGES_PER_DEVICE )
		{
			String messageNativeString = message->Begin();
			CSecondScreenSendMessageJob* task = new ( CTask::Root ) CSecondScreenSendMessageJob( reinterpret_cast<void*>( m_pSmartGlassDevice->HtmlSurface ), messageNativeString ); 			
			m_tasks.PushBack( task );
			GTaskManager->Issue( *task );
			success = true;
		}
		if( success == false )
		{
			TDynArray<CSecondScreenSendMessageJob*>::iterator end = m_tasks.End();
			for( TDynArray<CSecondScreenSendMessageJob*>::iterator iter = m_tasks.Begin(); iter != end; ++iter )
			{
				CSecondScreenSendMessageJob* task = *iter;
				if( task != NULL && task->IsFinished() )
				{
					if( !task->IsSubmitMessageAsyncStarted() )
					{
						if( task->IsSubmitMessageAsyncError() )
						{
							RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "SubmitMessageAsync, end with error %x" ), task->GetSubmitMessageAsyncErrorCode() );
						}
						task->Release();
						String messageNativeString = message->Begin();
						task = new ( CTask::Root ) CSecondScreenSendMessageJob( reinterpret_cast<void*>( m_pSmartGlassDevice->HtmlSurface ), messageNativeString ); 			
						*iter = task;
						GTaskManager->Issue( *task );
						success = true;
						break;
					}
					
				}
			}
		}				
		
		return success;
	}

	void OnMessageReceivedDurango( SmartGlassHtmlSurface^ pHtmlSurface, SmartGlassMessageReceivedEventArgs^ pArgs )
	{
		if( m_parent )
		{
			CJSONReaderUTF16 reader;	
			if( reader.Read( pArgs->Message->Data() ) == true ) // VALIDATE MESSAGE XR-011
			{
				m_parent->OnMessageReceived( pArgs->Message->Data() );	
			}
		}
	}

private:
	SmartGlassDevice^               m_pSmartGlassDevice;
	CSecondScreenDeviceDurango*		m_parent;
	TDynArray<CSecondScreenSendMessageJob*>	m_tasks;
};


CSecondScreenDeviceDurango::CSecondScreenDeviceDurango( void* secondScreenDeviceDurango, ISecondScreenDeviceDelegate* _delegate ): CSecondScreenDevice( _delegate )
{
	Platform::IntPtr parentPtr = Platform::IntPtr((void*)this);
	SecondScreenDeviceRefClass^ secondScreenDeviceRefClass = ref new SecondScreenDeviceRefClass();
	if( secondScreenDeviceDurango != NULL )
	{
		secondScreenDeviceRefClass->Initialize( reinterpret_cast<SmartGlassDevice^>(secondScreenDeviceDurango), parentPtr );

		m_secondScreenDeviceDurango = reinterpret_cast<void*>( secondScreenDeviceRefClass );
		if( m_secondScreenDeviceDurango != NULL )
		{
			IUnknown* iUnknown = static_cast<IUnknown*>( m_secondScreenDeviceDurango );
			iUnknown->AddRef();
		}
	}
	
}

CSecondScreenDeviceDurango::~CSecondScreenDeviceDurango()
{
	if( m_secondScreenDeviceDurango != NULL )
	{
		IUnknown* iUnknown = static_cast<IUnknown*>( m_secondScreenDeviceDurango );
		iUnknown->Release();
	}
}

Bool CSecondScreenDeviceDurango::SendMessageAsync( const Char* message ) const
{
	if( m_secondScreenDeviceDurango && message != NULL && message[0] != 0 )
	{
		SecondScreenDeviceRefClass^ secondScreenDeviceRefClass = reinterpret_cast<SecondScreenDeviceRefClass^>( m_secondScreenDeviceDurango ); 
		return secondScreenDeviceRefClass->SendMessageAsyncDurango( ref new Platform::String(message) );
	}
	return false;
}

void CSecondScreenDeviceDurango::OnMessageReceived( const Char* message )
{
	if( m_delegate )
	{
		m_delegate->OnMessageReceived( *this, message );
	}
}

bool CSecondScreenDeviceDurango::operator==( const CSecondScreenDevice& rhs )
{
	SecondScreenDeviceRefClass^ device_lhs =  reinterpret_cast<SecondScreenDeviceRefClass^>( m_secondScreenDeviceDurango ); 
	SecondScreenDeviceRefClass^ device_rhs =  reinterpret_cast<SecondScreenDeviceRefClass^>( ((const CSecondScreenDeviceDurango*)(&rhs))->m_secondScreenDeviceDurango ); 	
	return device_lhs->CompareDevices(device_rhs);
}

#endif

