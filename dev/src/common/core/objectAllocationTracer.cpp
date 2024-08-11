/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "objectMap.h"
#include "objectAllocationTracer.h"
#include "fileSys.h"
#include "resource.h"
#include "diskFile.h"
#include "fileStringWriter.h"
#include "resource.h"
#include "configVar.h"
#include "scopedPtr.h"
#include "version.h"

#include "../redSystem/stringWriter.h"

namespace Config
{
	// everything goes to the hacks in GC
	TConfigVar<Bool> cvDumpObjectDetails( "Memory/GC/Hacks", "DumpObjectDetails", true );
	TConfigVar<Bool> cvDumpObjectCallstacks( "Memory/GC/Hacks", "DumpObjectCallstacks", false );
	TConfigVar<String> cvReportFilePrefix( "Memory/GC/Hacks", "ObjectDumpFilePrefix", TXT("leaks") );
	TConfigVar<String> cvReportFilePath( "Memory/GC/Hacks", "ObjectDumpDirectory", TXT("") ); // default output used if not specified

} // Config

namespace Helper
{
	//---

	class CObjectGCLeakCollector
	{
	public:
		CObjectGCLeakCollector()
		{
			const Uint32 numClasses = SRTTI::GetInstance().GetIndexedClasses().Size();
			m_perClass.Resize( numClasses );
			for ( Uint32 i=0; i<numClasses; ++i )
				m_perClass[i] = 0;

			m_totalObjectsLeaked = 0;
			m_totalNativeMemLeaked = 0;
			m_totalScriptMemLeaked = 0;
		}

		void Add( CObject* object )
		{
			const Uint32 classIndex = object->GetClassIndex();
			m_perClass[ classIndex ] += 1;

			m_totalObjectsLeaked += 1;
			m_totalNativeMemLeaked += object->GetClass()->GetSize();
			m_totalScriptMemLeaked += object->GetClass()->GetScriptDataSize();
		}

		void Dump( Red::CAnsiStringFileStream& file ) const
		{
			struct ClassInfo
			{
				const CClass*		m_class;
				Uint32				m_count;
				Uint32				m_size;
			};

			TDynArray< ClassInfo > perClassInfo;
			for ( Uint32 i=0; i<m_perClass.Size(); ++i )
			{
				if ( m_perClass[i] )
				{
					const CClass* objectClass = SRTTI::GetInstance().GetIndexedClasses()[i];
					const Uint32 classMem = objectClass->GetSize() + objectClass->GetScriptDataSize();

					ClassInfo info;
					info.m_class = objectClass;
					info.m_count = m_perClass[i];
					info.m_size = m_perClass[i] * classMem;
					perClassInfo.PushBack( info );
				}
			}

			::Sort( perClassInfo.Begin(), perClassInfo.End(), []( const ClassInfo& a, const ClassInfo& b ) { return a.m_size > b.m_size; } );

			file.Appendf( "Objects leaked: %d\r\n", m_totalObjectsLeaked );
			file.Appendf( "Native memory leaked: %d bytes (%1.2fKB)\r\n", m_totalNativeMemLeaked, m_totalNativeMemLeaked / 1024.0f );
			file.Appendf( "Script memory leaked: %d bytes (%1.2fKB)\r\n", m_totalScriptMemLeaked, m_totalScriptMemLeaked / 1024.0f );
			file.Appendf( "\r\n" );

			file.Appendf( "Per class breakdown:\r\n" );
			for ( Uint32 i=0; i<perClassInfo.Size(); ++i )
			{
				const auto& info = perClassInfo[i];
				file.Appendf( "   %7.2f KB, %7d objects, %ls\r\n", info.m_size / 1024.0, info.m_count, info.m_class->GetName().AsChar() );
			}
			file.Appendf( "\r\n" );	
		}

	private:
		TDynArray< Uint32, MC_Temporary >		m_perClass;
		TDynArray< String, MC_Temporary >		m_resources;
		Uint32									m_totalObjectsLeaked;
		Uint32									m_totalNativeMemLeaked;
		Uint32									m_totalScriptMemLeaked;
	};

	//----

	CObjectAllocationTracker::CObjectAllocationTracker()
	{
		m_callstacks.Resize( GObjectsMap->GetMaxObjectIndex() );
	}

	void CObjectAllocationTracker::RegisterObject( const Uint32 objectIndex )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// we are not tracking the leaks
		if ( !Config::cvDumpObjectCallstacks.Get() )
			return;

		// resize of needed
		while ( objectIndex >= m_callstacks.Size() )
			m_callstacks.Resize( m_callstacks.Size() * 2 );

		// create new callstack
		RED_ASSERT( m_callstacks[objectIndex] == nullptr, TXT("Object ID conflict, Callstack not traced") );
		void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ObjectMap, sizeof(ObjectCallstack) );
		ObjectCallstack* callstack = new (mem) ObjectCallstack();
		m_callstacks[ objectIndex ] = callstack;
	}

	void CObjectAllocationTracker::UnregisterObject( const Uint32 objectIndex )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// release the callstack - always
		if ( m_callstacks[ objectIndex ] )
		{
			ObjectCallstack* callstack = m_callstacks[ objectIndex ];
			callstack->~ObjectCallstack();

			RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, callstack );
			m_callstacks[ objectIndex ] = nullptr;
		}
	}

	/*static*/ void CObjectAllocationTracker::AssembleFilePath( String& outFilePath )
	{
		// use depot path if possible
		String outputPath;
		if ( !Config::cvReportFilePath.Get().Empty() )
		{
			outputPath = Config::cvReportFilePath.Get();
			if ( !outputPath.EndsWith( TXT("\\") ) )
				outputPath += TXT("\\");
		}
		else
		{
			outputPath = GFileManager->GetBaseDirectory() + TXT("leaks\\");
		}

		// time stamp
		Red::DateTime time;
		Red::Clock::GetInstance().GetLocalTime(time);

		// assemble file path
		outFilePath = outputPath;
		outFilePath += String::Printf( TXT("%s_[%02d_%02d_%04d]_[%02d_%02d_%02d].txt"), 
			Config::cvReportFilePrefix.Get().AsChar(),
			time.GetDay(), time.GetMonth(), time.GetYear(),
			time.GetHour(), time.GetMinute(), time.GetSecond() );
	}

	bool CObjectAllocationTracker::DumpCallstackInfo( const TDynArray<CObject*, MC_Engine>& objects ) const
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// compute maximum object index
		Uint32 maxObjectIndex = 0;
		for ( CObject* object : objects )
			maxObjectIndex = Max( maxObjectIndex, object->GetObjectIndex() );

		// bit flags marking if object is a true leak
		TDynArray< Uint8 > leakedObjectsMask;
		leakedObjectsMask.Resize( maxObjectIndex+1 );
		Red::MemoryZero( leakedObjectsMask.Data(), leakedObjectsMask.DataSize() );

		// mark current objects as leaks
		for ( CObject* object : objects )
			leakedObjectsMask[ object->GetObjectIndex() ] = 1;

		// collect only true leaks - the fact that the child is gone when parent is released is NOT a leak cause
		TDynArray< CObject* > finalLeaks;
		CObjectGCLeakCollector objectCollector;
		for ( CObject* object : objects )
		{
			Bool isTrueLeak = true;
			CObject* parent = object->GetParent();
			if ( parent )
			{
				// we are not a leak unless we are parented to an object that is not released
				isTrueLeak = false;
				while ( parent != nullptr )
				{
					Uint8 isParentLeaked = 0;
					if ( parent->GetObjectIndex() < leakedObjectsMask.Size() )
						isParentLeaked = leakedObjectsMask[ parent->GetObjectIndex() ];

					if ( isParentLeaked == 0 )
					{
						isTrueLeak = true;
						break;
					}

					parent = parent->GetParent();
				}
			}
			else
			{
				// a quarantined resources is NOT a leak
				if ( object->IsA< CResource >() )
				{
					CResource* res = static_cast< CResource* >( object );
					if ( res && res->GetFile() && res->GetFile()->IsQuarantined() )
					{
						isTrueLeak = false;
					}
				}
			}

			// only collect true leaks
			if ( isTrueLeak )
			{
				objectCollector.Add( object );
				finalLeaks.PushBack( object );
			}
		}

		// no true leaks - no reason for a report
		if ( finalLeaks.Empty() )
			return false;

		// get the file path
		String absoluteFilePath;
		AssembleFilePath( absoluteFilePath );

		Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath ) );
		if ( !file )
		{
			WARN_CORE( TXT("Failed to open dump file '%ls'"), absoluteFilePath.AsChar() );
			return false;
		}

		WARN_CORE( TXT("Saving object dump to file '%ls'"), absoluteFilePath.AsChar() );

		// create buffered string writer and configure it to output to the file
		Red::CAnsiStringFileWriter writer( file.Get() );
		Red::CAnsiStringFileStream stream( writer );

		// time stamp
		Red::DateTime time;
		Red::Clock::GetInstance().GetLocalTime(time);

		// header
		stream.Appendf( "FULL LEAK REPORT\r\n" );
		stream.Appendf( " Generated on: %02d/%02d/%04d %02d:%02d:%02d\r\n", time.GetDay(), time.GetMonth(), time.GetYear(), time.GetHour(), time.GetMinute(), time.GetSecond() );
		stream.Appendf( " App: %s\r\n", RED_EXPAND_AND_STRINGIFY( PROJECT_PLATFORM ) );
		stream.Appendf( " Version: %s\r\n", APP_VERSION_BUILD );
		stream.Appendf( " CL: %s\r\n", APP_LAST_P4_CHANGE );
		stream.Appendf( " Details: %s\r\n", Config::cvDumpObjectDetails.Get() ? "Yes" : "No" );
		stream.Appendf( " Callstack: %s\r\n", Config::cvDumpObjectCallstacks.Get() ? "Yes" : "No" );
		stream.Appendf( "\r\n" );
		
		// resource leaks
		{
			TDynArray< CResource* > quarantinedResources;
			TDynArray< CResource* > leakedResources;

			// filter out the resource that would have been freed anyway
			for ( CObject* object : objects )
			{
				if ( object->IsA< CResource >() )
				{
					CResource* res = static_cast< CResource* >( object );
					if ( res && res->GetFile() && !res->GetParent() )
					{
						if ( res->GetFile()->IsQuarantined() )
						{
							quarantinedResources.PushBack( res );
						}
						else
						{
							leakedResources.PushBack( res );
						}
					}
				}
			}

			// report leaks
			stream.Appendf( "Leaked resources (%d):\r\n", leakedResources.Size() );
			for ( Uint32 i=0; i<leakedResources.Size(); ++i )
			{
				stream.Appendf( "   Leaked: '%s'\r\n", UNICODE_TO_ANSI( leakedResources[i]->GetFile()->GetDepotPath().AsChar() ) );
			}
			stream.Appendf( "\r\n" );

			// report quarantine (it's OK)
			stream.Appendf( "Quarantined resources (%d):\r\n", quarantinedResources.Size() );
			for ( Uint32 i=0; i<quarantinedResources.Size(); ++i )
			{
				stream.Appendf( "   Quarantined: '%s'\r\n", UNICODE_TO_ANSI( quarantinedResources[i]->GetFile()->GetDepotPath().AsChar() ) );
			}
			stream.Appendf( "\r\n" );
		}

		// normal leaks
		objectCollector.Dump( stream );

		// write details
		if ( Config::cvDumpObjectDetails.Get() )
		{
			stream.Appendf( "\r\n" );
			stream.Appendf( "Details of each leaked object allocation:\r\n" );
			stream.Appendf( "\r\n" );

			// write information about each object
			for ( CObject* object : finalLeaks )
			{
				// object details
				stream.Appendf( "Object: 0x%08llX\r\n", (Uint64)object );
				stream.Appendf( "Class: %s\r\n", UNICODE_TO_ANSI( object->GetClass()->GetName().AsChar() ) );

				// resource path (in case of resources)
				if ( object->IsA< CResource >() )
				{
					CResource* res = static_cast< CResource* >( object );
					if ( res->GetFile() )
					{
						stream.Appendf( "Path: %s\r\n", UNICODE_TO_ANSI( res->GetFile()->GetDepotPath().AsChar() ) );
					}
					else
					{
						stream.Appendf( "Path: Unknown\r\n" );
					}
				}

				// fine grain details
				stream.Appendf( "Index: %d\r\n", object->GetObjectIndex() );
				stream.Appendf( "Flags: 0x%X\r\n", object->GetFlags() );

				// parents
				CObject* parent = object->GetParent();
				while ( parent != nullptr )
				{
					stream.Appendf( "Parent: 0x%08llX (index %d, class %s)\r\n", 
						(Uint64)parent, parent->GetObjectIndex(), UNICODE_TO_ANSI( parent->GetClass()->GetName().AsChar() ) );
					
					// resource path (in case of resources)
					if ( parent->IsA< CResource >() )
					{
						CResource* res = static_cast< CResource* >( parent );
						if ( res->GetFile() )
						{
							stream.Appendf( "ParentResource: %s\r\n", UNICODE_TO_ANSI( res->GetFile()->GetDepotPath().AsChar() ) );
						}
					}

					// go to the next parent
					parent = parent->GetParent();
				}

				// callstack
				if ( Config::cvDumpObjectCallstacks.Get() )
				{
					const Uint32 index = object->GetObjectIndex();
					ObjectCallstack* callstack = (index < m_callstacks.Size()) ? m_callstacks[ index ] : nullptr;
					if ( !callstack )
					{
						stream.Appendf( "Callstack: Unknown\r\n" );
					}
					else
					{
						stream.Appendf( "Callstack:\r\n" );

						const Uint32 numLevels = callstack->m_callstack.GetCallstackDepth();
						for ( Uint32 i=0; i<numLevels; ++i )
						{
							Char callstackBuffer[ 1024 ];
							callstack->m_callstack.GetAsString( i, callstackBuffer, ARRAY_COUNT_U32(callstackBuffer) );

							stream.Appendf( "   %s\r\n", UNICODE_TO_ANSI(callstackBuffer) );
						}
					}
				}

				// separator
				stream.Appendf( "\r\n" );
				stream.Appendf( "-------------------------------------------------------\r\n" );
				stream.Appendf( "\r\n" );
			}
		}

		// saved
		return true;
	}

} // Helper