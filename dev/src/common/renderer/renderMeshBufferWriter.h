/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/mathUtils.h"
#include "../core/pagedMemoryWriter.h"
#include "../redMath/float16compressor.h"

/// Pointer based data buffer writer
/// This is used by Cooker, so everything must remain inline here (wcc doesn't link with renderer project)
class CRenderMeshBufferWriterPaged : public GpuApi::VertexPacking::IBufferWriter
{
private:
	CPagedMemoryBuffer*		m_buffer;

public:
	CRenderMeshBufferWriterPaged( CPagedMemoryBuffer& buffer )
		: m_buffer( &buffer )
	{}

	virtual Uint32 GetOffset() const
	{
		return m_buffer->GetTotalSize();
	}

	virtual void Align( Uint32 size )
	{
		if ( size > 1 )
		{
			Uint32 baseOffset = GetOffset();
			Uint32 offset = GetOffset();

			offset += (size-1);
			offset -= ( offset % size );

			while ( offset > baseOffset )
			{
				Uint8 zero = 0;
				m_buffer->Append( &zero, sizeof(zero) );
				baseOffset += 1;
			}
		}
	}

	virtual void WriteFloat( Float data )
	{
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteUnsignedByte( Uint8 data )
	{
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteUnsignedByteN( Float data )
	{
		if ( data <= 0.0f )
		{
			WriteUnsignedByte( 0 );
		}
		else if ( data >= 1.0f )
		{
			WriteUnsignedByte( 255 );
		}
		else
		{
			Uint8 val = (Uint8) ( data * 255.0f );
			m_buffer->Append( &val, sizeof(val) );
		}
	}

	virtual void WriteSignedByteN( Float data )
	{
		if ( data <= -1.0f )
		{
			WriteUnsignedByte( 0 );
		}
		else if ( data >= 1.0f )
		{
			WriteUnsignedByte( 255 );
		}
		else
		{
			Uint8 val = (Uint8) ( 127.0f + data * 127.0f );
			m_buffer->Append( &val, sizeof(val) );
		}
	}

	virtual void WriteUnsignedShort( Uint16 data )
	{
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteUnsignedShortN( Float data )
	{
		if ( data <= 0.0f )
		{
			WriteUnsignedShort( 0 );
		}
		else if ( data >= 1.0f )
		{
			WriteUnsignedShort( NumericLimits< Uint16 >::Max() );
		}
		else
		{
			Uint16 val = (Uint16)( data * NumericLimits< Uint16 >::Max() );
			m_buffer->Append( &val, sizeof(val) );
		}
	}

	virtual void WriteUnsignedInt( Uint32 data )
	{
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteFloat16( Float f )
	{
		Uint16 data = Float16Compressor::Compress( f );
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteColor( Uint32 data )
	{
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteDec3N( Float x, Float y, Float z )
	{
		int vx = (int) ::Clamp < Float >( x * 1023.0f, 0.0f, 1023.0f );
		int vy = (int) ::Clamp < Float >( y * 1023.0f, 0.0f, 1023.0f );
		int vz = (int) ::Clamp < Float >( z * 1023.0f, 0.0f, 1023.0f );
		int vw = 1;
		Uint32 data = ((vx&1023)+((vy&1023)<<10)+((vz&1023)<<20)+((vw&3)<<30));
		m_buffer->Append( &data, sizeof(data) );
	}

	virtual void WriteDec4N( Float x, Float y, Float z, Float w )
	{
		int vx = (int) ::Clamp < Float >( x * 1023.0f, 0.0f, 1023.0f );
		int vy = (int) ::Clamp < Float >( y * 1023.0f, 0.0f, 1023.0f );
		int vz = (int) ::Clamp < Float >( z * 1023.0f, 0.0f, 1023.0f );
		int vw = (int) ::Clamp < Float >( w * 3.0f, 0.0f, 3.0f );
		Uint32 data = ((vx&1023)+((vy&1023)<<10)+((vz&1023)<<20)+((vw&3)<<30));
		m_buffer->Append( &data, sizeof(data) );
	}
};
