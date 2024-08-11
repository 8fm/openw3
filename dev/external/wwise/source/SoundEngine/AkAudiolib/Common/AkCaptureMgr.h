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


#ifndef _AK_CAPTURE_MGR_H_
#define _AK_CAPTURE_MGR_H_
#include "AkFileParserBase.h"

#define CAPTURE_MAX_WRITERS 4
#define	MAXCAPTURENAMELENGTH 256

#pragma pack(1)
struct AkWAVEFileHeader
{
	AkChunkHeader		RIFF;		// AkFourcc	ChunkId: FOURCC('RIFF')
									// AkUInt32	dwChunkSize: Size of file (in bytes) - 8
	AkFourcc			uWAVE;		// FOURCC('WAVE')
	AkChunkHeader		fmt;		// AkFourcc	ChunkId: FOURCC('fmt ')
									// AkUInt32	dwChunkSize: Total size (in bytes) of fmt chunk's content
	WaveFormatEx		fmtHeader;	// AkUInt16	wFormatTag: 0x0001 for linear PCM etc.
									// AkUInt16	nChannels: Number of channels (1=mono, 2=stereo etc.)
									// AkUInt32	nSamplesPerSec: Sample rate (e.g. 44100)
									// AkUInt32	nAvgBytesPerSec: nSamplesPerSec * nBlockAlign
									// AkUInt16 nBlockAlign: nChannels * nBitsPerSample / 8 for PCM
									// AkUInt16 wBitsPerSample: 8, 16, 24 or 32
									// AkUInt16 cbSize:	Size of extra chunk of data, after end of this struct
	AkChunkHeader		data;		// AkFourcc	ChunkId: FOURCC('data')
									// AkUInt32	dwChunkSize: Total size (in bytes) of data chunk's content
	// Sample data goes after this..
};
#pragma pack()

class IAkCaptureWriter
{
public:
	virtual AKRESULT StartCapture( const AkOSChar* in_CaptureFileName, AkWAVEFileHeader* in_pHeader) = 0;
	virtual AKRESULT StopCapture( AkWAVEFileHeader* in_pHeader ) = 0;
	virtual AKRESULT PassSampleData( void* in_pData, AkUInt32 in_size ) = 0;
	virtual void Destroy() = 0;
};

class IAkCaptureWriterFactory
{
public:
	virtual IAkCaptureWriter* CreateWriter() = 0;
};

class AkCaptureFile
{
public:
	enum FormatTag {Int16=1, Float=3};	//From WAV format definitions	

	AkCaptureFile(IAkCaptureWriter* in_pWriter);
	~AkCaptureFile();

	AKRESULT StartCapture(const AkOSChar* in_Name, 
		AkUInt32 in_uNumChannels, 
		AkUInt32 in_uSampleRate, 
		AkUInt32 in_uBitsPerSample, 
		FormatTag in_formatTag);

	AKRESULT StopCapture();
	AKRESULT PassSampleData( void* in_pData, AkUInt32 in_size );

private:

	AkWAVEFileHeader	m_Header;
	AkUInt32			m_uDataSize;
	IAkCaptureWriter*	m_pWriter;
};

class AkCaptureMgr
{
public:
	static AK_FUNC( AkCaptureMgr*, Instance )();

	void SetWriter( IAkCaptureWriterFactory* in_pWriter );
	IAkCaptureWriterFactory* GetWriter();
	
	AkCaptureFile* StartCapture(const AkOSChar* in_CaptureFileName, 
								AkUInt32 in_uNumChannels, 
								AkUInt32 in_uSampleRate, 
								AkUInt32 in_uBitsPerSample, 
								AkCaptureFile::FormatTag in_formatTag);
private:

	AkCaptureMgr();
	~AkCaptureMgr();
	
	IAkCaptureWriterFactory *m_pWriterFactory;
};

#endif
