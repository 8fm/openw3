#include "build.h"

#include "file.h"
#include "compressedNumSerializer.h"

IFile& operator<<( IFile& file, CCompressedNumSerializer c )
{
	Uint32 noSign = Abs( c.m_num );

	Uint8 byt = (Uint8)((( c.m_num > 0 )?(0):(0x80) ) | ( (noSign > 0x3F) ? (0x40 | (noSign & 0x3F) ) : (noSign & 0x3F) ));
	file.Serialize( &byt, 1 );
	Uint8 firstByte = byt;

	c.m_num = 0;
	(Uint32&)c.m_num = byt & 0x3F;

	// larger or equal than 2 ^ 6
	if ( byt & 0x40 )
	{
		// more bytes than 1
		noSign >>= 6;

		byt = (Uint8)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
		file.Serialize( &byt, 1 );
		(Uint32&)c.m_num |= ( byt & 0x7F )<< 6;

		// larger than 2 ^ 13
		if ( byt & 0x80 )
		{
			// more bytes than 2
			noSign >>= 7;

			byt = (Uint8)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
			file.Serialize( &byt, 1 );
			(Uint32&)c.m_num |= ( byt & 0x7F )<< 13;

			// larger than 2 ^ 20
			if ( byt & 0x80 )
			{
				// more bytes than 3
				noSign >>= 7;

				byt = (Uint8)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
				file.Serialize( &byt, 1 );
				(Uint32&)c.m_num |= ( byt & 0x7F )<< 20;

				// larger than 2 ^ 27
				if ( byt & 0x80 )
				{
					// more bytes than 4
					noSign >>= 7;

					byt = (Uint8)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
					file.Serialize( &byt, 1 );
					(Uint32&)c.m_num |= ( byt & 0x7F )<< 27;
				}
			}
		}
	}
	c.m_num = firstByte & 0x80 ? -c.m_num : c.m_num;

//	ASSERT( file.IsReader() || oldVal == c.m_num );

	return file;
}
