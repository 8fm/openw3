/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceDatabasePoolEntry::CResourceDatabasePoolEntry()
			: m_dataOffset(0u)
			, m_dataSize(0u)
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceDatabasePoolEntry::CResourceDatabasePoolEntry(Red::System::Uint32 offset, Red::System::Uint32 size)
			: m_dataOffset(offset)
			, m_dataSize(size)
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Uint32 CResourceDatabasePoolEntry::GetOffset() const
		{
			return m_dataOffset;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Uint32 CResourceDatabasePoolEntry::GetSize() const
		{
			return m_dataSize;
		}
	}
}