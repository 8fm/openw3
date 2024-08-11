/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/
 
#include <map>
#include <AK/Tools/Common/AkLock.h>
class AkMemTracker
{
public:

	AkMemTracker()
	{
	}

	~AkMemTracker()
	{
	}

	void Add( void * ptr, size_t in_size, const char * szFile, int pos )
	{
		m_csLock.Lock();

		MemInfo & info = m_memToInfo[ ptr ];
		info.szFile = szFile;
		info.pos = pos;
		info.ulSize = in_size;

		m_csLock.Unlock();
	}

	void Remove( void * ptr )
	{
		m_csLock.Lock();
		m_memToInfo.erase( ptr );
		m_csLock.Unlock();
	}

	void PrintMemoryLabel(void* ptr)
	{
		m_csLock.Lock();
		MemToInfo::iterator info = m_memToInfo.find(ptr);
		if (info != m_memToInfo.end() )
		{
			char szMsg[ 1024 ];
			if ( info->second.szFile == NULL )
			{
				sprintf( szMsg, "**Memory leak detected in Wwise sound engine: %08X\t%6i\t\tUnknown file\n", (AkUIntPtr) info->first, info->second.ulSize );
			}
			else
			{
				sprintf( szMsg, "** Memory leak detected in Wwise sound engine: %08X\t%6i\t%5i\t%s\n\n", (AkUIntPtr) info->first, info->second.ulSize, info->second.pos, info->second.szFile );
			}
			AKPLATFORM::OutputDebugMsg( szMsg );
		}
		m_csLock.Unlock();
	}

	void DumpToFile()
	{
		m_csLock.Lock();

		FILE* file = fopen( "AkMemDump.txt", "w" );
		fprintf( file, "Address\tSize\tLine\tFile\n" );
		MemToInfo::iterator it = m_memToInfo.begin();
		while ( it != m_memToInfo.end() )
		{
			if ( it->second.szFile == NULL ) 
				fprintf( file, "%08X\t%6i\t\tUnknown file\n", (AkUIntPtr) it->first, it->second.ulSize );
			else
				fprintf( file, "%08X\t%6i\t%5i\t%s\n", (AkUIntPtr) it->first, it->second.ulSize, it->second.pos, it->second.szFile );
			it++;
		}
		fclose(file);

		m_csLock.Unlock();
	}

	struct MemInfo
	{
		const char * szFile;
		int pos;
		size_t ulSize;
	};

	typedef std::map<void *, MemInfo> MemToInfo;
	MemToInfo m_memToInfo;
	CAkLock m_csLock;
};