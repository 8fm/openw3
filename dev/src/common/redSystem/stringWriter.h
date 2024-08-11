/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "types.h"
#include "crt.h"
#include "utility.h"

namespace Red
{
	namespace System
	{
		namespace StringWriterStream
		{
			// Stream that is not a stream - it will prevent data from being flushed 
			template< typename CH >
			class ConstantSizeStream : public NonCopyable
			{
			public:
				static ConstantSizeStream& GetInstance();
				bool Flush( const Bool forced, const CH* data, const Red::System::Uint32 count );
			};

			// NULL stream - will consume data without doing anything with it
			template< typename CH >
			class NullStream : public NonCopyable
			{
			public:
				static NullStream& GetInstance();
				bool Flush( const Bool forced, const CH* data, const Red::System::Uint32 count );
			};

		} // StringWriterStream

		// Basic string writer
		template< typename CH, typename StreamClass = StringWriterStream::ConstantSizeStream< CH > >
		class StringWriter : public NonCopyable
		{
		public:
			StringWriter( CH* bufer, const Red::System::Uint32 bufferSize, StreamClass& sc = StreamClass::GetInstance() );
			~StringWriter();

			// reset stream
			void Reset();

			// flush to stream out
			void Flush();

			// put char in the stream
			void Put( const CH ch );

			// append string (or part of it)
			void Append( const CH* str, const Red::System::Uint32 length = (Red::System::Uint32)-1 );

			// append formated string
			void Appendf( const CH* str, ... );

		private:
			void FlushWithFail();

			static const Red::System::Uint32 FormattedBufferSize = 1024;

			Red::System::Uint32		m_max;
			Red::System::Uint32		m_pos;
			CH*						m_buf;
			Red::System::Bool		m_full;

			StreamClass&			m_stream;
		};

		// Stack based stream writer
		template< typename CH, Red::System::Uint32 BufferSize, typename StreamClass = StringWriterStream::ConstantSizeStream< CH > >
		class StackStringWriter : public StringWriter< CH, StreamClass >
		{
		public:
			static_assert( BufferSize > 1, "Need to be able to put at least one char in the buffer" ); 

			StackStringWriter( StreamClass& sc = StreamClass::GetInstance() )
				: StringWriter< CH, StreamClass >( m_localBuf, BufferSize, sc )
			{}

			RED_INLINE const CH* AsChar() const
			{
				return m_localBuf;
			}

		private:
			CH	m_localBuf[ BufferSize ];
		};

	} // System
} // Red

#include "stringWriter.inl"