/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace MemoryFramework { namespace WinAPI {

///////////////////////////////////////////////////////////////////
// IsFileOpen
//
RED_INLINE Red::System::Bool FileWriter::IsFileOpen() const
{
	return m_fileHandle != INVALID_HANDLE_VALUE;
}

} } }