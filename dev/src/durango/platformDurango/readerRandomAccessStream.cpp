/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "readerRandomAccessStream.h"
#include "../../common/core/depot.h"

AsyncOperationWithProgressClass::AsyncOperationWithProgressClass( Platform::IntPtr fileHandler, unsigned long long position, unsigned int count, IBuffer^ buffer )
{	
	m_buffer = buffer;

	m_asyncOperationWithProgressCompletedHandler = nullptr;
	m_asyncOperationProgressHandler = nullptr;

	bool initializeSuccess = false;

	IFile* file = reinterpret_cast< IFile* >( (void*)fileHandler );
	if( file != NULL && m_buffer != nullptr )
	{
		if( (position + count) <= file->GetSize() )
		{	
			IUnknown* pUnk = reinterpret_cast<IUnknown*>(m_buffer);

			IBufferByteAccess* pBufferByteAccess = nullptr;

			HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess));

			if( hr == S_OK )
			{
				byte* byteBuffor = nullptr;
				pBufferByteAccess->Buffer( &byteBuffor );

				if( byteBuffor )
				{
					file->Seek( position );
					file->Serialize( byteBuffor, count );
					m_buffer->Length = count;

					initializeSuccess = true;					
				}
				pBufferByteAccess->Release();
			}
		}
	}

	if( initializeSuccess )
	{
		m_operationErrorCode.Value	= S_OK;
		m_operationStatus			= AsyncStatus::Completed;
	}
	else
	{
		m_operationErrorCode.Value	= E_FAIL;
		m_operationStatus			= AsyncStatus::Error;
	}
}

IBuffer^ AsyncOperationWithProgressClass::GetResults()
{
	return m_buffer;
}

AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ AsyncOperationWithProgressClass::Completed::get()
{
	return m_asyncOperationWithProgressCompletedHandler;
}

void AsyncOperationWithProgressClass::Completed::set( AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ value )
{
	m_asyncOperationWithProgressCompletedHandler = value;
	if( m_asyncOperationWithProgressCompletedHandler != nullptr && m_operationStatus == AsyncStatus::Completed )
	{
		m_asyncOperationWithProgressCompletedHandler->Invoke( this, AsyncStatus::Completed  );
	}
}

AsyncOperationProgressHandler<IBuffer^, unsigned int>^ AsyncOperationWithProgressClass::Progress::get()
{
	return m_asyncOperationProgressHandler;
}

void AsyncOperationWithProgressClass::Progress::set( AsyncOperationProgressHandler<IBuffer^, unsigned int>^ value )
{
	m_asyncOperationProgressHandler = value;
}

void AsyncOperationWithProgressClass::Cancel()
{

}

void AsyncOperationWithProgressClass::Close()
{

}

HResult AsyncOperationWithProgressClass::ErrorCode::get()
{
	return m_operationErrorCode;
}

unsigned int AsyncOperationWithProgressClass::Id::get()
{
	return 0;
}

AsyncStatus AsyncOperationWithProgressClass::Status::get()
{
	return m_operationStatus;
}

InputStreamClass::InputStreamClass( Platform::String^ pathToFile, Bool absolutePath  )
{
	if( absolutePath )
	{
		m_reader = GFileManager->CreateFileReader( pathToFile->Begin(), FOF_AbsolutePath|FOF_Buffered );
	}
	else
	{
		m_reader = GFileManager->CreateFileReader( pathToFile->Begin(), FOF_Buffered );
	}

	if( m_reader )
	{
		m_size = m_reader->GetSize();
	}
	else
	{
		m_size = 0;
	}
	m_position = 0;
}

InputStreamClass::~InputStreamClass()
{
	if( m_reader)
		delete m_reader;
}

Bool InputStreamClass::IsValid()
{
	return m_reader != NULL;
}

IAsyncOperationWithProgress<IBuffer^, unsigned int>^ InputStreamClass::ReadAsync(IBuffer^ buffer, unsigned int count, InputStreamOptions options)
{
	return ref new AsyncOperationWithProgressClass( m_reader, m_position, count, buffer );
}

unsigned long long InputStreamClass::Position::get()
{
	return m_position;
}

void InputStreamClass::Position::set( unsigned long long value )
{
	m_position = value;
}

unsigned long long InputStreamClass::Size::get()
{
	return m_size;
}

ReaderRandomAccessStreamClass::~ReaderRandomAccessStreamClass()
{

}

ReaderRandomAccessStreamClass::ReaderRandomAccessStreamClass( Platform::String^ pathToFile, Bool absolutePath )
{
	m_absolutePath = absolutePath;
	m_pathToFile = pathToFile;
	m_inputStream = ref new InputStreamClass( m_pathToFile, absolutePath );
}

IRandomAccessStream^ ReaderRandomAccessStreamClass::CloneStream()
{
	return nullptr;
}

IInputStream^ ReaderRandomAccessStreamClass::GetInputStreamAt( unsigned long long position )
{
	InputStreamClass^ inputStream = ref new InputStreamClass( m_pathToFile, m_absolutePath );
	inputStream->Position = position;
	return inputStream;
}

IOutputStream^ ReaderRandomAccessStreamClass::GetOutputStreamAt( unsigned long long position )
{
	return nullptr;
}

void ReaderRandomAccessStreamClass::Seek( unsigned long long position )
{
	if( m_inputStream )
	{
		m_inputStream->Position = position;
	}
}

Bool ReaderRandomAccessStreamClass::IsValid()
{
	if( m_inputStream )
	{
		return m_inputStream->IsValid();
	}
	return false;
}

bool ReaderRandomAccessStreamClass::CanRead::get()
{
	return true;
}

bool ReaderRandomAccessStreamClass::CanWrite::get()
{
	return false;
}

unsigned long long ReaderRandomAccessStreamClass::Position::get()
{
	if( m_inputStream )
	{
		return m_inputStream->Position;
	}
	return 0;
}

unsigned long long ReaderRandomAccessStreamClass::Size::get()
{
	if( m_inputStream )
	{
		return m_inputStream->Size;
	}
	return 0;
}

void ReaderRandomAccessStreamClass::Size::set( unsigned long long size )
{

}

IAsyncOperationWithProgress<IBuffer^, unsigned int>^ ReaderRandomAccessStreamClass::ReadAsync(IBuffer^ buffer, unsigned int count, InputStreamOptions options)
{
	return m_inputStream->ReadAsync( buffer, count, options );
}

IAsyncOperation<bool>^ ReaderRandomAccessStreamClass::FlushAsync()
{
	return nullptr;
}

IAsyncOperationWithProgress<unsigned int, unsigned int>^ ReaderRandomAccessStreamClass::WriteAsync( IBuffer^ buffer )
{
	return nullptr;
}
