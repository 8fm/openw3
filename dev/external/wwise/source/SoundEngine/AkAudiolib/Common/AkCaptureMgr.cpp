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

#include "stdafx.h"
#include "AkCaptureMgr.h"
#include "string.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "FileCaptureWriter.h"
#include "PlatformAudiolibDefs.h"

AkCaptureMgr* AkCaptureMgr::Instance()
{
	// Classic singleton
	static AkCaptureMgr sTheCaptureMgr;	
	return &sTheCaptureMgr;
}

AkCaptureMgr::AkCaptureMgr():
m_pWriterFactory(NULL)
{
}

AkCaptureMgr::~AkCaptureMgr()
{
}

void AkCaptureMgr::SetWriter( IAkCaptureWriterFactory* in_pWriter )
{
	m_pWriterFactory = in_pWriter;
}

IAkCaptureWriterFactory* AkCaptureMgr::GetWriter()
{
	if (m_pWriterFactory == NULL)
	{
		//If there is no writer factory, default to disk
		m_pWriterFactory = FileCaptureWriterFactory::Instance();
	}
	return m_pWriterFactory;
}

AkCaptureFile* AkCaptureMgr::StartCapture( const AkOSChar* in_CaptureFileName, AkUInt32 in_uNumChannels, AkUInt32 in_uSampleRate, AkUInt32 in_uBitsPerSample, AkCaptureFile::FormatTag in_formatTag )
{
	IAkCaptureWriter *pWriter = GetWriter()->CreateWriter();
	if (pWriter == NULL)
		return NULL;

	AkCaptureFile *pFile = NULL;
	AkNew2(pFile, g_DefaultPoolId, AkCaptureFile, AkCaptureFile(pWriter));
	if (pFile == NULL)
	{
		pWriter->Destroy();
		return NULL;
	}
	
	AKRESULT eResult = pFile->StartCapture(in_CaptureFileName, in_uNumChannels, in_uSampleRate, in_uBitsPerSample, in_formatTag);
	if (eResult != AK_Success)
	{
		AkDelete(g_DefaultPoolId, pFile);
		pWriter->Destroy();
		pFile = NULL;
	}

	return pFile;
}

AkCaptureFile::AkCaptureFile(IAkCaptureWriter* in_pWriter)
	: m_uDataSize(0)
	, m_pWriter(in_pWriter)
{
	memset((void*)&m_Header, 0, sizeof(AkWAVEFileHeader));
}

AkCaptureFile::~AkCaptureFile()
{
}

AKRESULT AkCaptureFile::StartCapture(const AkOSChar* in_Name, 
									 AkUInt32 in_uNumChannels, 
									 AkUInt32 in_uSampleRate, 
									 AkUInt32 in_uBitsPerSample, 
									 FormatTag in_formatTag)
{
	AkUInt32 uBlockAlign = (in_uNumChannels * in_uBitsPerSample) / 8;	
	m_Header.RIFF.ChunkId = AkPlatformRiffChunkId;
	m_Header.RIFF.dwChunkSize = 0XFFFFFFFF;//sizeof(AkWAVEFileHeader) + in_uDataSize - 8;
	m_Header.uWAVE = WAVEChunkId;
	m_Header.fmt.ChunkId = fmtChunkId;
	m_Header.fmt.dwChunkSize = sizeof(WaveFormatEx);
	m_Header.fmtHeader.wFormatTag = in_formatTag; // PCM
	m_Header.fmtHeader.nChannels = (AkUInt16)in_uNumChannels;				
	m_Header.fmtHeader.nSamplesPerSec = in_uSampleRate;			
	m_Header.fmtHeader.nAvgBytesPerSec = in_uSampleRate * uBlockAlign; 
	m_Header.fmtHeader.nBlockAlign = (AkUInt16)uBlockAlign;
	m_Header.fmtHeader.wBitsPerSample = (AkUInt16)in_uBitsPerSample;
	m_Header.fmtHeader.cbSize = 0;
	m_Header.data.ChunkId = dataChunkId;	
	m_Header.data.dwChunkSize = 0XFFFFFFFF;//in_uDataSize; 

	return m_pWriter->StartCapture(in_Name, &m_Header);
}

AKRESULT AkCaptureFile::StopCapture()
{
	if (m_pWriter == NULL)
		return AK_Success;	//Not capturing, so not an error.

	m_Header.RIFF.dwChunkSize = sizeof(AkWAVEFileHeader) + m_uDataSize - 8;
	m_Header.data.dwChunkSize = m_uDataSize; 
	AKRESULT eResult = m_pWriter->StopCapture(&m_Header);
	m_pWriter->Destroy();
	m_pWriter = NULL;

	AkDelete(g_DefaultPoolId, this);
	return eResult;
}

AKRESULT AkCaptureFile::PassSampleData( void* in_pData, AkUInt32 in_size )
{
	if (m_pWriter == NULL)
		return AK_Fail;
	AKRESULT eResult = m_pWriter->PassSampleData(in_pData, in_size);
	if (eResult == AK_Success)
		m_uDataSize += in_size;	

	return eResult;
}
