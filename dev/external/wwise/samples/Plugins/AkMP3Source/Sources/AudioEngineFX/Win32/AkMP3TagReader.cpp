//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "AkMP3TagReader.h"

#define XING_FRAMES_FLAG	1
#define XING_BYTES_FLAG		2
#define XING_TOC_FLAG		4
#define XING_VBR_SCALE_FLAG	8

#define SAMPLES_PER_MPEG1_FRAME	1152
#define SAMPLES_PER_MPEG2_FRAME 576

struct AkID3V1Tag
{
	char tag[ 3 ]; // 'TAG'
	char title[ 30 ];
	char artist[ 30 ];
	char album[ 30 ];
	char year[ 4 ];
	char comment[ 30 ];
	unsigned char genre;
};

#define SIZEOF_ID3V1	128

struct AkMP3FrameHeader
{
	// 'Unpacked' version of all mp3 frame header fields.
	AkUInt32 uVersion;
	AkUInt32 uLayer;
	AkUInt32 uProtection;
	AkUInt32 uBitRate;
	AkUInt32 uSampleRate;
	AkUInt32 uPadding;
	AkUInt32 uPrivate;
	AkUInt32 uChannelMode;
	AkUInt32 uModeExtension;
	AkUInt32 uCopyright;
	AkUInt32 uOriginal;
	AkUInt32 uEmphasis;

	bool Decode( AkUInt8 * in_pHeader ) // And validate
	{
		if ( in_pHeader[0] != 0xff || ( in_pHeader[1] & 0xe0 ) != 0xe0 )
			return false;

		uVersion = ( in_pHeader[1] >> 3 ) & 3;
		if ( uVersion == 1 ) // reserved
			return false;

		uLayer = ( in_pHeader[1] >> 1 ) & 3;
		if ( uLayer != 1 ) // only support MPEG Layer III
			return false;

		uProtection = in_pHeader[1] & 1;

		uBitRate = ( in_pHeader[2] >> 4 ) & 0xf;
		if ( uBitRate == 0 || uBitRate == 15 ) // do not allow free format, bad bitrate
			return false;

		uSampleRate = ( in_pHeader[2] >> 2 ) & 3;
		if ( uSampleRate == 3 ) // reserved
			return false;

		uPadding = ( in_pHeader[2] >> 1 ) & 1;
		uPrivate = in_pHeader[2] & 1;
		uChannelMode = ( in_pHeader[3] >> 6 ) & 3;
		uModeExtension = ( in_pHeader[3] >> 4 ) & 3;
		uCopyright = ( in_pHeader[3] >> 3 ) & 1;
		uOriginal = ( in_pHeader[3] >> 2 ) & 1;

		uEmphasis = in_pHeader[3] & 3;
		if ( uEmphasis == 2 ) // reserved
			return false;

		return true;
	}

	AkUInt32 GetBitRate()
	{
		static const AkUInt32 s_aBitRatesMPEG1[ 16 ] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 };
		static const AkUInt32 s_aBitRatesMPEG2[ 16 ] = { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, 0 };

		switch ( uVersion )
		{
			case 0: // MPEG Version 2.5 (later extension of MPEG 2)
			case 2: // MPEG Version 2 (ISO/IEC 13818-3)
				return s_aBitRatesMPEG2[ uBitRate ];

			case 3: // MPEG Version 1 (ISO/IEC 11172-3)
				return s_aBitRatesMPEG1[ uBitRate ];

			default:
				return 0;
		}
	}

	AkUInt32 GetNumChannels()
	{
		return ( uChannelMode == 3 ) ? 1 : 2;
	}

	AkUInt32 GetSampleRate()
	{
		static const AkUInt32 s_aSampleRatesMPEG1[ 4 ] = { 44100, 48000, 32000, 0 };
		static const AkUInt32 s_aSampleRatesMPEG2[ 4 ] = { 22050, 24000, 16000, 0 };
		static const AkUInt32 s_aSampleRatesMPEG2_5[ 4 ] = { 11025, 12000, 8000, 0 };

		switch ( uVersion )
		{
		case 0: // MPEG Version 2.5 (later extension of MPEG 2)
			return s_aSampleRatesMPEG2_5[ uSampleRate ];

		case 2: // MPEG Version 2 (ISO/IEC 13818-3)
			return s_aSampleRatesMPEG2[ uSampleRate ];

		case 3: // MPEG Version 1 (ISO/IEC 11172-3)
			return s_aSampleRatesMPEG1[ uSampleRate ];

		default: // reserved
			return 0;
		}
	}
};

#define MAX_FRAME_SEARCH 4096

static AkUInt8 * AkMP3SyncToFirstFrame( AkUInt8 * in_pFile, AkMP3FrameHeader & io_hdr )
{
	AkUInt8 * pSearchEnd = in_pFile + MAX_FRAME_SEARCH;
	while ( in_pFile < pSearchEnd )
	{
		if ( io_hdr.Decode( in_pFile ) )
			return in_pFile;

		++in_pFile;
	}

	return NULL;
}

static AkUInt32 ReadUInt24( AkUInt8 * in_pInt )
{
	return in_pInt[2] | ( in_pInt[1] << 8 ) | ( in_pInt[0] << 16 );
}

static AkUInt32 ReadSynchSafeUInt32( AkUInt8 * in_pInt )
{
	return in_pInt[3] | ( in_pInt[2] << 7 ) | ( in_pInt[1] << 14 ) | ( in_pInt[0] << 21 );
}

static bool IsEqualFrameID3( AkUInt8 * in_pFrameID, DWORD in_dwFrameID )
{
	return in_pFrameID[ 0 ] == ( ( in_dwFrameID >> 16 ) & 0xff )
		&& in_pFrameID[ 1 ] == ( ( in_dwFrameID >> 8 ) & 0xff )
		&& in_pFrameID[ 2 ] == ( in_dwFrameID  & 0xff );
}

static bool IsEqualFrameID4( AkUInt8 * in_pFrameID, DWORD in_dwFrameID )
{
	return in_pFrameID[ 0 ] == ( in_dwFrameID >> 24 )
		&& in_pFrameID[ 1 ] == ( ( in_dwFrameID >> 16 ) & 0xff )
		&& in_pFrameID[ 2 ] == ( ( in_dwFrameID >> 8 ) & 0xff )
		&& in_pFrameID[ 3 ] == ( in_dwFrameID  & 0xff );
}

static void ReadTextField( AkUInt8 * in_pText, AkInt32 in_iInTextLen, LPWSTR out_pText, AkInt32 in_iOutTextLen )
{
	AkUInt32 uEncoding = *in_pText++;
	in_iInTextLen--;
	switch ( uEncoding )
	{
	case 0: // ISO-8859-1 [ISO-8859-1] (codepage 1252 under Windows). Terminated with $00 in ID3V2.4, not necessarily terminated in ID3V2.3.
		{
			AkInt32 iWritten = ::MultiByteToWideChar( 1252, 0, (char *) in_pText, in_iInTextLen, out_pText, in_iOutTextLen );
			if ( iWritten < in_iOutTextLen )
				out_pText[ iWritten ] = 0; // make sure string is properly terminated
		}
		break;

	case 1: // UTF-16 [UTF-16] encoded Unicode [UNICODE] with BOM. Terminated with $00 00 in ID3V2.4, not necessarily terminated in ID3V2.3.
		if ( in_pText[0] == 0xff && in_pText[1] == 0xfe )
		{
			// Little-Endian unicode text: simple copy
			WCHAR * pInText = (WCHAR *) ( in_pText + 2 );
			in_iInTextLen = ( in_iInTextLen - 2 ) >> 1;
			WCHAR * pInTextEnd = pInText + in_iInTextLen;
			WCHAR * pOutTextEnd = out_pText + in_iOutTextLen;

			while ( ( *pInText != 0 ) && ( pInText < pInTextEnd ) && ( out_pText < pOutTextEnd ) )
				*out_pText++ = *pInText++;

			// ensure string is properly terminated
			if ( out_pText >= pOutTextEnd )
				pOutTextEnd[-1] = 0; 
			else
				*out_pText = 0;
			break;
		}
		else
		{
			in_pText += 2; // skip BOM and fall back to UTF-16BE codepath
			in_iInTextLen -= 2;
		}
		// no break

	case 2: // UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM. Terminated with $00 00 in ID3V2.4, not necessarily terminated in ID3V2.3.
		{
			WCHAR * pInText = (WCHAR *) in_pText;
			in_iInTextLen = in_iInTextLen >> 1;
			WCHAR * pInTextEnd = pInText + in_iInTextLen;
			WCHAR * pOutTextEnd = out_pText + in_iOutTextLen;

			while ( ( *pInText != 0 ) && ( pInText < pInTextEnd ) && ( out_pText < pOutTextEnd ) )
				*out_pText++ = _byteswap_ushort( *pInText++ );

			// ensure string is properly terminated
			if ( out_pText >= pOutTextEnd )
				pOutTextEnd[-1] = 0; 
			else
				*out_pText = 0;
		}
		break;

	case 3: // UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated with $00 in ID3V2.4, not necessarily terminated in ID3V2.3.
		{
			AkInt32 iWritten = ::MultiByteToWideChar( CP_UTF8, 0, (char *) in_pText, in_iInTextLen, out_pText, in_iOutTextLen );
			if ( iWritten < in_iOutTextLen )
				out_pText[ iWritten ] = 0; // make sure string is properly terminated
		}
		break;
	}
}

bool AkMP3InfoRead( LPCTSTR in_tszFilename, AkMP3BaseInfo & out_info, AkMP3Tag * out_pTag /* = NULL */ )
{
	if ( out_pTag )
	{
		out_pTag->wszAlbum[0] = 0;
		out_pTag->wszArtist[0] = 0;
		out_pTag->wszTrack[0] = 0;
	}

	HANDLE hFile = ::CreateFile( in_tszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 0 );
	if ( !hFile )
		return false;

	HANDLE hMap = ::CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
	if ( !hMap )
	{
		::CloseHandle( hFile );
		return false;
	}

	AkUInt8 * pFile = (AkUInt8 *) ::MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );
	if ( !pFile )
		goto error_result;

	bool bTagRead = false;

	__try // memory-mapped file needs exception handling
	{
		AkUInt8 * pFirstFrame = pFile;

		// ID3V2 Tag
		// 10 first bytes is the ID3 header.

		AkUInt8 * pID3 = pFile;
		while ( IsEqualFrameID3( pID3, 'ID3' ) )
		{
			AkUInt32 uID3MajorVersion = pID3[ 3 ];
			AkUInt32 uID3RevisionNumber = pID3[ 4 ];

			if ( uID3MajorVersion == 2 )
			{
				AkUInt32 uUnsynch = pID3[ 5 ] >> 7;
				AkUInt32 uCompress = pID3[ 5 ] >> 6;

				AkUInt32 uSize = ReadSynchSafeUInt32( pID3 + 6 ); // sum of the byte length of the extended header, the padding and the frames after unsynchronisation.

				if ( out_pTag ) // requested tag info ?
				{
					pID3 += 10; 

					AkUInt8 * pID3DataStart = pID3;

					while ( ( pID3 + 6 - pID3DataStart ) < (int) uSize )
					{
						AkUInt32 uID3FrameSize = ReadUInt24( pID3 + 3 );
						if ( IsEqualFrameID3( pID3, 0 ) || uID3FrameSize == 0 )
							break; // bail out -- we hit padding

						if ( IsEqualFrameID3( pID3, 'TT2' ) )
						{
							ReadTextField( pID3 + 6, uID3FrameSize, out_pTag->wszTrack, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}
						else if ( IsEqualFrameID3( pID3, 'TAL' ) )
						{
							ReadTextField( pID3 + 6, uID3FrameSize, out_pTag->wszAlbum, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}
						else if ( IsEqualFrameID3( pID3, 'TP1' ) )
						{
							ReadTextField( pID3 + 6, uID3FrameSize, out_pTag->wszArtist, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}

						pID3 += 6;
						pID3 += uID3FrameSize;
					}
				}

				pFirstFrame += 10 + uSize;
			} 
			else if ( uID3MajorVersion == 3 || uID3MajorVersion == 4 )
			{
				// Finish processing header

				AkUInt32 uUnsynch = pID3[ 5 ] >> 7;
				AkUInt32 uExtHdr = ( pID3[ 5 ] >> 6 ) & 1;
				AkUInt32 uExperimental = ( pID3[ 5 ] >> 5 ) & 1;
				AkUInt32 uFooter = ( uID3MajorVersion == 4 ) ? ( ( pID3[ 5 ] >> 4 ) & 1 ) : 0;

				AkUInt32 uSize = ReadSynchSafeUInt32( pID3 + 6 ); // sum of the byte length of the extended header, the padding and the frames after unsynchronisation.

				if ( out_pTag ) // requested tag info ?
				{
					pID3 += 10; 

					AkUInt8 * pID3DataStart = pID3;

					// Process extended header

					if ( uExtHdr )
					{
						// Just skip it for now

						AkUInt32 uExtHdrSize = ReadSynchSafeUInt32( pID3 );
						pID3 += uExtHdrSize;
					}

					while ( ( pID3 + 10 - pID3DataStart ) < (int) uSize )
					{
						AkUInt32 uID3FrameSize = ReadSynchSafeUInt32( pID3 + 4 );
						if ( IsEqualFrameID4( pID3, 0 ) || uID3FrameSize == 0 )
							break; // bail out -- we hit padding

						// Read format flags %0h00kmnp
						AkUInt32 uGrouping = ( pID3[ 9 ] >> 6 ) & 1;
						AkUInt32 uCompression = ( pID3[ 9 ] >> 3 ) & 1;
						AkUInt32 uEncryption = ( pID3[ 9 ] >> 2 ) & 1;
						AkUInt32 uFrameUnsynch = ( pID3[ 9 ] >> 1 ) & 1;
						AkUInt32 uDataLengthID = pID3[ 9 ] & 1;

						if ( IsEqualFrameID4( pID3, 'TIT2' ) )
						{
							ReadTextField( pID3 + 10, uID3FrameSize, out_pTag->wszTrack, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}
						else if ( IsEqualFrameID4( pID3, 'TALB' ) )
						{
							ReadTextField( pID3 + 10, uID3FrameSize, out_pTag->wszAlbum, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}
						else if ( IsEqualFrameID4( pID3, 'TPE1' ) )
						{
							ReadTextField( pID3 + 10, uID3FrameSize, out_pTag->wszArtist, AK_MP3_TAG_STRING_SIZE );
							bTagRead = true;
						}

						pID3 += 10;
						pID3 += uID3FrameSize;
					} 
				}

				pFirstFrame += 10 + uSize;
			}

			pID3 = pFirstFrame;
		}

		// Extract basic information from first frame header

		AkMP3FrameHeader header;
		pFirstFrame = AkMP3SyncToFirstFrame( pFirstFrame, header );
		if ( !pFirstFrame )
			goto error_result;

		out_info.uSampleRate = (AkUInt16) header.GetSampleRate();
		out_info.uNumChannels = (AkUInt16) header.GetNumChannels();
		out_info.msDuration = 0;
		out_info.uBitRate = header.GetBitRate();
		out_info.uFirstFrameOffset = (AkUInt32) ( pFirstFrame - pFile );
		out_info.uMPEGStreamSize = 0;
		out_info.bToc = false;

		// Determine offset in frame of Xing/Lame data, test for presence

		AkUInt8 * pXing;
		if ( header.uVersion & 1 ) // MPEG-1
		{
			if ( header.GetNumChannels() == 1 )
				pXing = pFirstFrame + 21;
			else // stereo
				pXing = pFirstFrame + 36;
		}
		else // MPEG-2 / MPEG-2.5
		{
			if ( header.GetNumChannels() == 1 )
				pXing = pFirstFrame + 13;
			else // stereo
				pXing = pFirstFrame + 21;
		}

		bool bXing = false, bLame = false;
		if ( pXing[ 0 ] == 'X' && pXing[ 1 ] == 'i' && pXing[ 2 ] == 'n' && pXing[ 3 ] == 'g' )
			bXing = true;
		if ( pXing[ 0 ] == 'I' && pXing[ 1 ] == 'n' && pXing[ 2 ] == 'f' && pXing[ 3 ] == 'o' )
		{
			bXing = true;
			bLame = true;
		}

		if ( bXing )
		{
			pXing += 4;

			AkUInt32 uXingFlags = _byteswap_ulong( *((AkUInt32 *) pXing ) );
			pXing += 4;

			if ( uXingFlags & XING_FRAMES_FLAG )
			{
				AkUInt32 uFrames = _byteswap_ulong( *((AkUInt32 *) pXing ) );
				
				if ( header.uVersion & 1 ) // MPEG-1
					out_info.msDuration = ( 20 * uFrames * SAMPLES_PER_MPEG1_FRAME ) / ( out_info.uSampleRate / 50 );
				else
					out_info.msDuration = ( 20 * uFrames * SAMPLES_PER_MPEG2_FRAME ) / ( out_info.uSampleRate / 50 );

				pXing += 4;
			}

			if ( uXingFlags & XING_BYTES_FLAG )
			{
				out_info.uMPEGStreamSize = _byteswap_ulong( *((AkUInt32 *) pXing ) );
				pXing += 4;
			}

			if ( uXingFlags & XING_TOC_FLAG )
			{
				out_info.bToc = true;

				for ( AkUInt32 i = 0; i < 100; ++i )
					out_info.aToc[ i ] = pXing[ i ];
				pXing += 100;
			}
		}

		DWORD dwFileSize = ::GetFileSize( hFile, NULL );

		if ( out_info.uMPEGStreamSize == 0 )
			out_info.uMPEGStreamSize = dwFileSize - out_info.uFirstFrameOffset;

		if ( out_info.msDuration == 0 ) // no Xing frame info -- assume CBR and calculate duration directly based on bitrate
		{
			AkUInt32 uBitRate = header.GetBitRate();
			out_info.msDuration = out_info.uMPEGStreamSize / ( uBitRate / 8 );
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
	   // Failed to read from the view.
		goto error_result;
	}	
	
	if ( pFile ) 
		::UnmapViewOfFile( pFile );

	::CloseHandle( hMap );

	// Read ID3V1 (but only if there was no ID3V2 and we require tags)

	if ( out_pTag && !bTagRead )
	{
		DWORD dwReturn = ::SetFilePointer( hFile, -SIZEOF_ID3V1, NULL, FILE_END );

		AkID3V1Tag id3v1tag;

		if ( ::ReadFile( hFile, &id3v1tag, SIZEOF_ID3V1, &dwReturn, NULL ) )
		{
			if ( dwReturn == SIZEOF_ID3V1
				&& id3v1tag.tag[0] == 'T' 
				&& id3v1tag.tag[1] == 'A' 
				&& id3v1tag.tag[2] == 'G' )
			{
				AkInt32 iWritten;

				iWritten = ::MultiByteToWideChar( 1252, 0, id3v1tag.album, 30, out_pTag->wszAlbum, AK_MP3_TAG_STRING_SIZE );
				if ( iWritten < AK_MP3_TAG_STRING_SIZE )
					out_pTag->wszAlbum[ iWritten ] = 0; // make sure string is properly terminated

				iWritten = ::MultiByteToWideChar( 1252, 0, id3v1tag.artist, 30, out_pTag->wszArtist, AK_MP3_TAG_STRING_SIZE );
				if ( iWritten < AK_MP3_TAG_STRING_SIZE )
					out_pTag->wszArtist[ iWritten ] = 0;

				iWritten = ::MultiByteToWideChar( 1252, 0, id3v1tag.title, 30, out_pTag->wszTrack, AK_MP3_TAG_STRING_SIZE );
				if ( iWritten < AK_MP3_TAG_STRING_SIZE )
					out_pTag->wszTrack[ iWritten ] = 0;
			}
		}
	}

	::CloseHandle( hFile );

	return true;

error_result:

	::ZeroMemory( &out_info, sizeof( out_info ) );

	if ( pFile ) 
		 UnmapViewOfFile( pFile );

	::CloseHandle( hMap );
	::CloseHandle( hFile );

	return false;
}