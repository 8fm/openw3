/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../memoryFileWriter.h"
#include "profilerFile.h"
#include "../file.h"
#include "../fileSys.h"
#include "../hashmap.h"


//////////////////////////////////////////////////////////////////////////

namespace NewRedProfiler
{
    //////////////////////////////////////////////////////////////////////////

    const Int16 ProfilerFile::FILE_VERSION = 0x0005;
    const Int32 ProfilerFile::FILE_SIGNATURE = 0x66600000 | FILE_VERSION;

    //////////////////////////////////////////////////////////////////////////

    ProfilerFile::ProfilerFile()
        :   m_buffer( nullptr ),
			m_bufferSize( 0 ),
			m_functions( nullptr ),
			m_functionsCount( 0 ),
			m_buffSignalStrings( nullptr ),
			m_buffSignalStringsSize( 0 ),
			m_signalStringsCount( 0 )
    {
    }

    //////////////////////////////////////////////////////////////////////////

    ProfilerFile::~ProfilerFile()
    {
        ClearBuffers();
    }

    //////////////////////////////////////////////////////////////////////////

    void ProfilerFile::ClearBuffers()
    {
        m_functions				= nullptr;
        m_buffer				= nullptr;
        m_bufferSize			= 0;
		m_buffSignalStrings		= nullptr;
		m_buffSignalStringsSize = 0;
		m_signalStringsCount	= 0;
    }

    //////////////////////////////////////////////////////////////////////////

    void ProfilerFile::SetDataForSaving( const Uint64* buff, Uint32 buffSize, const InstrumentedFunction** functions, Uint32 functionsCount, const Uint8* buffSignalStrings, Uint32 buffSignalStringsSize , Uint32 signalStringsCount )
    {
        if( buff == nullptr || buffSize <= 0 || functions == nullptr )
		{
            return;
		}

        ClearBuffers();

        m_buffer				= buff;
        m_bufferSize			= buffSize;
        m_functions				= functions;
		m_functionsCount		= functionsCount;
		m_buffSignalStrings		= buffSignalStrings;
		m_buffSignalStringsSize = buffSignalStringsSize;
		m_signalStringsCount	= signalStringsCount;
    }
	
    //////////////////////////////////////////////////////////////////////////

	Bool ProfilerFile::SaveToFile( const Char* fileName ) const
	{
		if ( m_buffer == nullptr || m_bufferSize == 0 )
		{
			return false;
		}

		if (  m_functions == nullptr || m_functionsCount == 0  )
		{
			return false;
		}


		if ( fileName == nullptr )
		{
			return false;
		}

		IFile* writer = GFileManager->CreateFileWriter( fileName, FOF_AbsolutePath | FOF_Buffered | FOF_Append );
		if ( writer )
		{
			// Write signature
			writer->Serialize( (void*)(&FILE_SIGNATURE), sizeof(Int32) );

			// Write signal functions
			Uint8* currentPosBuffSignalStrings = (Uint8*)m_buffSignalStrings;

			TDynArray< const char* > stringsArray;
			THashMap< Uint32, Uint32 > stringIndexToStringArrayIndexMap;
			
			Uint32 stringIndex = 0;
			while( currentPosBuffSignalStrings < m_buffSignalStrings + m_buffSignalStringsSize )
			{
				size_t stringArrayIndex = stringsArray.Size();
				for( size_t i = 0; i < stringsArray.Size(); i++ )
				{
					if( Red::System::StringCompare( stringsArray[(Int32)i], (const char*)currentPosBuffSignalStrings ) == 0 )
					{
						stringArrayIndex = i;
						break;
					}
				}

				if( stringArrayIndex == stringsArray.Size() ) // add new string to array
				{
					stringsArray.PushBack( (const char*) currentPosBuffSignalStrings );
				}

				stringIndexToStringArrayIndexMap.Insert( stringIndex, (Uint32)stringArrayIndex );

				stringIndex++;

				const Uint32 length = (Uint32)strlen( (const char*)currentPosBuffSignalStrings );
				currentPosBuffSignalStrings += length + 1;
			}

			Uint32 stringArraySize = stringsArray.Size();
			writer->Serialize( &stringArraySize, sizeof(Uint32) );

			for ( size_t stringIndex = 0 ; stringIndex < stringsArray.Size(); stringIndex++ )
			{
				const Uint32 length = (Uint32)strlen( stringsArray[(Int32)stringIndex] );
				writer->Serialize( (void*)(&length), sizeof(Uint32) );
				writer->Serialize( (void*)stringsArray[(Int32)stringIndex], length );
			}

			Uint32 mapSize = stringIndexToStringArrayIndexMap.Size();
			writer->Serialize( &mapSize, sizeof(Uint32) );

			for ( THashMap< Uint32, Uint32 >::iterator it=stringIndexToStringArrayIndexMap.Begin(); it!=stringIndexToStringArrayIndexMap.End(); ++it )
			{
				writer->Serialize( &it->m_first, sizeof(Uint32) );
				writer->Serialize( &it->m_second, sizeof(Uint32) );
			}

			// Write buffer size
			writer->Serialize( (void*)(&m_bufferSize), sizeof(Uint32) );

			// Write buffer
			writer->Serialize( (void*)m_buffer, m_bufferSize );

			// Write function signatures
			writer->Serialize( (void*)(&m_functionsCount), sizeof(Uint32) );

			for( Uint32 i=0; i<m_functionsCount; ++i)
			{
				const InstrumentedFunction* fun = m_functions[i];
				ASSERT( fun );

				const Uint32 length = (Uint32)strlen(fun->m_name);
				Bool enable = fun->m_enabled.GetValue();
				writer->Serialize( (void*)(&enable), sizeof(Bool) );
				writer->Serialize( (void*)(&length), sizeof(Uint32) );
				writer->Serialize( (void*)fun->m_name, length );
			}
						
			delete writer;
		}

		return true;
	}

	Bool ProfilerFile::SaveToBuffer( CMemoryFileWriter* writer ) const
	{
		if ( writer )
		{
			// Write signature
			writer->Serialize( (void*)(&FILE_SIGNATURE), sizeof(Int32) );

			// Write signal functions
			Uint8* currentPosBuffSignalStrings = (Uint8*)m_buffSignalStrings;

			TDynArray< const char* > stringsArray;
			THashMap< Uint32, Uint32 > stringIndexToStringArrayIndexMap;

			Uint32 stringIndex = 0;
			while( currentPosBuffSignalStrings < m_buffSignalStrings + m_buffSignalStringsSize )
			{
				size_t stringArrayIndex = stringsArray.Size();
				for( size_t i = 0; i < stringsArray.Size(); i++ )
				{
					if( Red::System::StringCompare( stringsArray[(Int32)i], (const char*)currentPosBuffSignalStrings ) == 0 )
					{
						stringArrayIndex = i;
						break;
					}
				}

				if( stringArrayIndex == stringsArray.Size() ) // add new string to array
				{
					stringsArray.PushBack( (const char*) currentPosBuffSignalStrings );
				}

				stringIndexToStringArrayIndexMap.Insert( stringIndex, (Uint32)stringArrayIndex );

				stringIndex++;

				const Uint32 length = (Uint32)strlen( (const char*)currentPosBuffSignalStrings );
				currentPosBuffSignalStrings += length + 1;
			}

			Uint32 stringArraySize = stringsArray.Size();
			writer->Serialize( &stringArraySize, sizeof(Uint32) );

			for ( size_t stringIndex = 0 ; stringIndex < stringsArray.Size(); stringIndex++ )
			{
				const Uint32 length = (Uint32)strlen( stringsArray[(Int32)stringIndex] );
				writer->Serialize( (void*)(&length), sizeof(Uint32) );
				writer->Serialize( (void*)stringsArray[(Int32)stringIndex], length );
			}

			Uint32 mapSize = stringIndexToStringArrayIndexMap.Size();
			writer->Serialize( &mapSize, sizeof(Uint32) );

			for ( THashMap< Uint32, Uint32 >::iterator it=stringIndexToStringArrayIndexMap.Begin(); it!=stringIndexToStringArrayIndexMap.End(); ++it )
			{
				writer->Serialize( &it->m_first, sizeof(Uint32) );
				writer->Serialize( &it->m_second, sizeof(Uint32) );
			}

			// Write buffer size
			writer->Serialize( (void*)(&m_bufferSize), sizeof(Uint32) );

			// Write buffer
			writer->Serialize( (void*)m_buffer, m_bufferSize );

			// Write function signatures
			writer->Serialize( (void*)(&m_functionsCount), sizeof(Uint32) );

			for( Uint32 i=0; i<m_functionsCount; ++i)
			{
				const InstrumentedFunction* fun = m_functions[i];
				ASSERT( fun );

				const Uint32 length = (Uint32)strlen(fun->m_name);
				Bool enable = fun->m_enabled.GetValue();
				writer->Serialize( (void*)(&enable), sizeof(Bool) );
				writer->Serialize( (void*)(&length), sizeof(Uint32) );
				writer->Serialize( (void*)fun->m_name, length );
			}

			writer->Close();

			return true;
		}

		// def result
		return false;
	}

    //////////////////////////////////////////////////////////////////////////

} // namespace NewRedProfiler
 
