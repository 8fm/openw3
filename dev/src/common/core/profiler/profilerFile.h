#ifndef _PROFILER_FILE_H_
#define _PROFILER_FILE_H_

//////////////////////////////////////////////////////////////////////////
// headers
#include "instrumentedFunction.h"


//////////////////////////////////////////////////////////////////////////
// pre-declarations
class CMemoryFileWriter;


//////////////////////////////////////////////////////////////////////////
// declarations
namespace NewRedProfiler
{
    //////////////////////////////////////////////////////////////////////////

    class ProfilerFile
    {
    public:
        ProfilerFile();
        ~ProfilerFile();

        // Uses data as point of reference ( no deletion on dtor )
        void SetDataForSaving( const Uint64* buff, Uint32 buffSize, const InstrumentedFunction** functions, Uint32 functionsCount, const Uint8* buffSignalStrings, Uint32 buffSignalStringsSize, Uint32 signalStringsCount );
        Bool SaveToFile( const Char* filename ) const;
		Bool SaveToBuffer( CMemoryFileWriter* buffer ) const;

    private:

        void ClearBuffers();

        const Uint64*                       m_buffer;
        Uint32                              m_bufferSize;

        const InstrumentedFunction**		m_functions;
		Uint32								m_functionsCount;

		const Uint8*						m_buffSignalStrings;
		Uint32								m_buffSignalStringsSize;
		Uint32								m_signalStringsCount;

        static const Int16 FILE_VERSION;
        static const Int32 FILE_SIGNATURE;
    };

    //////////////////////////////////////////////////////////////////////////

} // namespace NewRedProfiler

//////////////////////////////////////////////////////////////////////////

#endif
