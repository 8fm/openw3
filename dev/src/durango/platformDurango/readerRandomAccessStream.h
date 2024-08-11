#pragma once 

#include <Windows.h>
#include <Unknwn.h>
#include <robuffer.h>

using namespace Windows::Foundation;
using namespace Windows::Xbox::Speech::Recognition;
using namespace Windows::Storage::Streams;


ref class AsyncOperationWithProgressClass sealed : IAsyncOperationWithProgress<IBuffer^, unsigned int>
{
	public: 
		AsyncOperationWithProgressClass( Platform::IntPtr fileHandler, unsigned long long position, unsigned int count, IBuffer^ buffer  );

		//Windows::Foundation::IAsyncOperationWithProgress
	public: 
		virtual	IBuffer^ GetResults();

	public: 
		virtual	property AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ Completed 
		{ 
			AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ get();
			void set (AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ value);
		}
		virtual	property AsyncOperationProgressHandler<IBuffer^, unsigned int>^ Progress 
		{ 
			AsyncOperationProgressHandler<IBuffer^, unsigned int>^ get();
			void set (AsyncOperationProgressHandler<IBuffer^, unsigned int>^ value);
		}

		//Windows::Foundation::IAsyncInfo
	public: 		
		virtual	void Cancel();
		virtual	void Close();

	public: 
		virtual	property HResult ErrorCode 
		{ 
			HResult get();
		}
		virtual	property unsigned int Id 
		{ 
			unsigned int get();
		}
		virtual	property AsyncStatus Status 
		{ 
			AsyncStatus get();
		}

	private: 
		AsyncOperationWithProgressCompletedHandler<IBuffer^, unsigned int>^ m_asyncOperationWithProgressCompletedHandler;
		AsyncOperationProgressHandler<IBuffer^, unsigned int>^ m_asyncOperationProgressHandler;
		HResult		m_operationErrorCode; 
		AsyncStatus	m_operationStatus;
		IBuffer^		m_buffer;
};

ref class InputStreamClass sealed: IInputStream
{
	public: 
		InputStreamClass( Platform::String^ pathToFile, Bool absolutePath );
		//Platform::IDisposable::Dispose() 
		virtual	~InputStreamClass();

		Bool IsValid();

	public: 
		virtual	property unsigned long long Position 
		{ 
			unsigned long long get();
			void set (unsigned long long value);
		}
		virtual	property unsigned long long Size 
		{ 
			unsigned long long get();
		}	
		
	public: 
		//Windows::Storage::Streams::IInputStream
		virtual	IAsyncOperationWithProgress<IBuffer^, unsigned int>^ ReadAsync(IBuffer^ buffer, unsigned int count, InputStreamOptions options);

	private: 
		IFile* m_reader;
		unsigned long long m_position;
		unsigned long long m_size;
};

ref class ReaderRandomAccessStreamClass sealed: IRandomAccessStream
{
	public: 
		ReaderRandomAccessStreamClass( Platform::String^ pathToFile, Bool absolutePath );

		//Platform::IDisposable::Dispose() 
		virtual	~ReaderRandomAccessStreamClass();

		//Windows::Storage::Streams::IRandomAccessStream 
		virtual	IRandomAccessStream^ CloneStream();
		virtual	IInputStream^ GetInputStreamAt(	unsigned long long position	);
		virtual	IOutputStream^ GetOutputStreamAt( unsigned long long position );
		virtual	void Seek( unsigned long long position );

		Bool IsValid();

	public: 
		virtual	property bool CanRead 
		{ 
			bool get();
		}
		virtual	property bool CanWrite 
		{ 
			bool get();
		}
		virtual	property unsigned long long Position 
		{ 
			unsigned long long get();
		}
		virtual	property unsigned long long Size 
		{ 
			unsigned long long get();
			void set (unsigned long long value);
		}

	public: 
		//Windows::Storage::Streams::IInputStream 
		virtual	IAsyncOperationWithProgress<IBuffer^, unsigned int>^ ReadAsync(IBuffer^ buffer, unsigned int count, InputStreamOptions options);

		//Windows::Storage::Streams::IOutputStream 
		virtual IAsyncOperation<bool>^ FlushAsync();
		virtual IAsyncOperationWithProgress<unsigned int, unsigned int>^ WriteAsync( IBuffer^ buffer );

	private:
		InputStreamClass^	m_inputStream;
		Platform::String^	m_pathToFile;
		Bool				m_absolutePath;
};