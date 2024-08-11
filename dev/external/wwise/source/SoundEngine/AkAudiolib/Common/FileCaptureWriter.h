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

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkCaptureMgr.h"
#include <AK/SoundEngine/Common/IAkStreamMgr.h>

class FileCaptureWriterFactory : public IAkCaptureWriterFactory
{
public:
	static FileCaptureWriterFactory* Instance();
	virtual IAkCaptureWriter* CreateWriter();
};

class FileCaptureWriter : public IAkCaptureWriter
{
public:
	FileCaptureWriter();
	~FileCaptureWriter();

	virtual AKRESULT StartCapture( const AkOSChar* in_CaptureFileName, AkWAVEFileHeader* in_pHeader );
	virtual AKRESULT StopCapture( AkWAVEFileHeader* in_pHeader );
	virtual AKRESULT PassSampleData( void* in_pData, AkUInt32 in_size );
	virtual void Destroy();

private:
	AkOSChar m_szFileName[MAXCAPTURENAMELENGTH];
	AkUInt32 m_uCaptureStreamDataSize;			// Data size capture counter (bytes)
	AK::IAkStdStream *m_pCaptureStream;			// Output stream for capture
};
