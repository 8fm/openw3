/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderWMVVideo.h"

#if defined(XMV_IS_SUPPORTED)

#ifndef _DEBUG
#  pragma comment (lib, "xmedia2.lib")
#else
#  pragma comment (lib, "xmediad2.lib")
#endif

const Uint32 WMV_BUFFER_SIZE( 128 * 2048 );

CRenderWMVVideo::CRenderWMVVideo( const String& fileName )
    : m_xmvPlayer( NULL )
    , m_xmvReadJob( NULL )
    , m_xaudio2( NULL )
    , m_initialized( false )
    , m_fileName( fileName )
    , m_audioFile( NULL )
    , m_videoFile( NULL )
{
    m_audioFile = GFileManager->CreateFileReader( m_fileName, FOF_Buffered | FOF_AbsolutePath, WMV_BUFFER_SIZE );
    m_videoFile = GFileManager->CreateFileReader( m_fileName, FOF_Buffered | FOF_AbsolutePath, WMV_BUFFER_SIZE );
}

CRenderWMVVideo::~CRenderWMVVideo()
{
    ReleasePlayer();
}

static HRESULT ReadWMVData(
         PVOID context,
         ULONGLONG offset,
         PVOID buffer,
         DWORD bytesToRead,
         DWORD *bytesRead)
{
    IFile* file = static_cast<IFile*>( context );

    CWMVDataReadJob* xmvReadJob = new CWMVDataReadJob( file, buffer, offset, bytesToRead, bytesRead );

    SJobManager::GetInstance().Issue( xmvReadJob );

    while ( ! xmvReadJob->HasEnded() )
    {
    } 

    xmvReadJob->Release();
    xmvReadJob = NULL;

    return S_OK;
}

//! Render video from renderer tick
void CRenderWMVVideo::Render( CRenderFrame* frame )
{
    if ( ! m_xmvPlayer )
    {
        HRESULT hr;

        IXAudio2MasteringVoice* pMasterVoice = NULL;

        // Initialize the IXAudio2 and IXAudio2MasteringVoice interfaces.
        if ( FAILED(hr = XAudio2Create( &m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR ) ) )
        {
            ReleasePlayer();
            return;
        }

        if ( FAILED(hr = m_xaudio2->CreateMasteringVoice( &pMasterVoice, XAUDIO2_DEFAULT_CHANNELS,
                                XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL ) ) )
        {
            ReleasePlayer();
            return;
        }

        XMEDIA_XMV_CREATE_PARAMETERS params;
        ZeroMemory(&params, sizeof (params));

        params.dwFlags = XMEDIA_CREATE_CPU_AFFINITY | XMEDIA_CREATE_SHARE_IO_CACHE;
        params.dwAudioDecoderCpu = 1;
        params.dwVideoDecoderCpu = 1;
        params.dwAudioRendererCpu = 3;
        params.dwVideoRendererCpu = 3;

        params.createType = XMEDIA_CREATE_FROM_USER_IO;
        params.createFromUserIo.pfnAudioStreamReadCallback = ReadWMVData;
        params.createFromUserIo.pfnVideoStreamReadCallback = ReadWMVData;
        params.createFromUserIo.pvAudioStreamContext = m_audioFile;
        params.createFromUserIo.pvVideoStreamContext = m_videoFile;

        hr = XMedia2CreateXmvPlayer( 
            GpuApi::Hacks::GetDevice(),
            m_xaudio2,
            &params,
            &m_xmvPlayer );

        //This may fail if there is no sound track in the file. In such a case 
        //we have to disable audio and explicitly set the video track number
        //(according to XDK)

        if ( FAILED( hr ) )
        {
            params.dwVideoStreamId = 1;
            params.dwAudioStreamId = (DWORD) XMEDIA_STREAM_ID_DONT_USE;
            params.dwFlags = XMEDIA_CREATE_CPU_AFFINITY; 

            hr = XMedia2CreateXmvPlayer( 
                GpuApi::Hacks::GetDevice(),
                m_xaudio2,
                &params,
                &m_xmvPlayer );
        }

        if ( FAILED ( hr ) )
        {
            ReleasePlayer();
            return;
        }

    }

    m_initialized = true;
    DWORD flags = XMEDIA_PLAY_DISABLE_AV_SYNC;
    HRESULT hr = m_xmvPlayer->RenderNextFrame( flags, NULL );

    if ( hr == XMEDIA_W_NO_DATA )
    {
        return;
    }

    if ( FAILED( hr ) || hr == XMEDIA_W_EOF )
    {
        ReleasePlayer();
    }

}

//! Is video valid?
Bool CRenderWMVVideo::IsValid() const
{
    return ( !m_initialized || m_xmvPlayer != NULL ) && m_audioFile && m_videoFile;
}

void CRenderWMVVideo::ReleasePlayer()
{
    m_initialized = true;

    if ( m_xaudio2 )
    {
        m_xaudio2->Release();
        m_xaudio2 = NULL;
    }

    if ( m_xmvPlayer )
    {
        m_xmvPlayer->Release();
        m_xmvPlayer = NULL;
        GpuApi::Hacks::GetDevice()->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    }

    if ( m_audioFile )
    {
        delete m_audioFile;
        m_audioFile = NULL;
    }

    if ( m_videoFile )
    {
        delete m_videoFile;
        m_videoFile = NULL;
    }
}


CWMVDataReadJob::CWMVDataReadJob( IFile* file, void* buffer, ULONGLONG offset, DWORD bytesToRead, DWORD* bytesRead )
    : ILoadJob( JP_Video )
    , m_file( file )
    , m_buffer( buffer )
    , m_offset( offset )
    , m_bytesToRead( bytesToRead )
    , m_bytesRead( bytesRead )
{
    ASSERT( m_file );
    ASSERT( m_bytesRead );
}

CWMVDataReadJob::~CWMVDataReadJob()
{
}

EJobResult CWMVDataReadJob::Process()
{
    if ( ! m_file )
    {
        return JR_Failed;
    }

    m_file->Seek( static_cast<Int32>( m_offset ) );
    Uint32 oldOffset = m_file->GetOffset();
    m_file->Serialize( m_buffer, m_bytesToRead );
    *m_bytesRead = m_file->GetOffset() - oldOffset;
    return JR_Finished;
}

#endif