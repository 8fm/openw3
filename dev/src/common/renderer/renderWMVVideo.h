/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/


#if defined(XMV_IS_SUPPORTED)

class CWMVDataReadJob;

class CRenderWMVVideo : public IRenderVideo
{
private:
    IXMedia2XmvPlayer* m_xmvPlayer;
    CWMVDataReadJob* m_xmvReadJob;
    IXAudio2* m_xaudio2;
    bool m_initialized;
    String m_fileName;
    IFile* m_audioFile;
    IFile* m_videoFile;

    CTimeCounter timer;

public:
    CRenderWMVVideo( const String& fileName );
    ~CRenderWMVVideo();

	//! Render video from renderer tick
	virtual void Render( CRenderFrame* frame );
    
	//! Is video valid?
	virtual Bool IsValid() const;
    
private:

    void ReleasePlayer();
};

class CWMVDataReadJob : public ILoadJob
{
private:

    IFile* m_file;
    void* m_buffer;
    ULONGLONG m_offset;
    DWORD m_bytesToRead;
    DWORD* m_bytesRead;

protected:
    virtual EJobResult Process();

public:
    CWMVDataReadJob( IFile* file, void* buffer, ULONGLONG offset, DWORD bytesToRead, DWORD* bytesRead );
    virtual ~CWMVDataReadJob();

	virtual const Char* GetDebugName() const override { return TXT("WMVVideoDataRead"); }
};

#endif