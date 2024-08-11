
#include "build.h"
#include "voice.h"

#ifndef FindResource
#	ifdef UNICODE
#		define FindResource  FindResourceW
#	else
#		define FindResource  FindResourceA
#	endif // !UNICODE
#else
#	warning "The FindResource redefinition isn't necessary anymore, please remove it"
#endif

#include "sapi.h"
#include "sphelper.h"

#include "../../common/core/depot.h"
#include "../../common/engine/localizationManager.h"

#define LIPSYNCOFFSET 0.07f

// =================================================================================================
namespace {
// =================================================================================================

/*
Preprocesses text so it doesn't contain code points that are problematic for Annosoft Lipsync SDK.

\param text Text that is to be preprocessed.
\param result (out) Preprocessed text.

Annosoft Lipsync SDK (version 4.0.0.0) has problems with certain Unicode code points. The problem is
that ConvertTextForAlignment() function (which is a part of Annosoft Lipsync SDK and which is used by
our lipsync dll) causes SEH exception to be raised when text contains said code points. The solution
is to replace these with code points that will produce the same (or almost the same) results and will
not cause any problems for Annosoft Lipsync SDK.
*/
void PreprocessTextForAnnosoftLipsyncSDK( const String& text, String& result )
{
	// TODO: We might consider normalizing the text to one of Unicode normalization forms
	/// (see http://unicode.org/reports/tr15/#Norm_Forms). However, we don't know if such
	// normalized text will be properly handled by Annosoft Lipsync SDK.

	result = text;

	String src;
	String dst;

	// Replace each HORIZONTAL ELLIPSIS (U+2026) with three full stops.
	src = L"\u2026";
	dst = L"...";
	result.ReplaceAll( src, dst, false );

	// Replace each LATIN SMALL LIGATURE OE (U+0153) with oe.
	// Note that replacing ligatures in such way is safe - see http://www.unicode.org/faq/ligature_digraph.html.
	src = L"\u0153";
	dst = L"oe";
	result.ReplaceAll( src, dst, false );

	// Replace each RIGHT SINGLE QUOTATION MARK (U+2019) with plain old apostrophe (U+0027).
	src = L"\u2019";
	dst = L"'";
	result.ReplaceAll( src, dst, false );
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

VoiceBase::VoiceBase()
	: m_buffer( NULL )
{
	SetFormat( 48000, 16, 1 );
}

VoiceBase::~VoiceBase()
{
	ClearBuffer();
}

void VoiceBase::ClearBuffer()
{
	if ( m_buffer )
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}
}

Bool VoiceBase::PrepareBuffer( DWORD ntime )
{
	ClearBuffer();

	DWORD length = m_format.nSamplesPerSec * m_format.nChannels * m_format.wBitsPerSample * (DWORD)ntime / 8;

	m_buffer = new Uint8[ length ];

	m_header.lpData = (char*)m_buffer;
	m_header.dwBufferLength = length;
	m_header.dwBytesRecorded = 0;
	m_header.dwUser = 0;
	m_header.dwFlags = 0;
	m_header.reserved = 0;
	m_header.lpNext = 0;

	return true;
}

void VoiceBase::GetMMResult( MMRESULT res )
{
	switch ( res )
	{
	case MMSYSERR_ALLOCATED: 
		m_lastError = TXT("Specified resource is already allocated.");
		break;

	case MMSYSERR_BADDEVICEID:
		m_lastError = TXT("Specified device identifier is out of range.");
		break;

	case MMSYSERR_NODRIVER:
		m_lastError = TXT("No device driver is present. ");
		break;

	case MMSYSERR_NOMEM:
		m_lastError = TXT("Unable to allocate or lock memory. ");
		break;

	case WAVERR_BADFORMAT:
		m_lastError = TXT("Attempted to open with an unsupported waveform-audio format.");
		break;

	case WAVERR_UNPREPARED:
		m_lastError = TXT("The buffer pointed to by the pwh parameter hasn't been prepared. ");
		break;

	case WAVERR_SYNC:
		m_lastError = TXT("The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag. ");
		break;

	case WAVERR_STILLPLAYING:
		m_lastError = TXT("The buffer pointed to by the pwh parameter is still in the queue.");
		break;

	case MMSYSERR_NOTSUPPORTED:
		m_lastError = TXT("Specified device is synchronous and does not support pausing. ");
		break;

	case MMSYSERR_NOERROR:
		break;

	default:
		m_lastError = TXT("Unspecified error");
	}
}

const String& VoiceBase::GetLastError() const
{
	return m_lastError;
}

Float VoiceBase::GetDuration() const
{
	return (Float)( ( m_header.dwBufferLength / m_format.nChannels ) / ( m_format.wBitsPerSample / 8 ) ) / m_format.nSamplesPerSec;
}

Bool VoiceBase::CopyBuffer( LPVOID lpBuffer, DWORD ntime )
{
	DWORD length = m_format.nSamplesPerSec * m_format.nChannels * m_format.wBitsPerSample * ntime / 8;
	
	Red::System::MemoryCopy( m_buffer, lpBuffer, length );

	return true;
}

Bool VoiceBase::CopyBuffer( const VoiceBase* voice, DWORD ntime )
{
	return CopyBuffer( voice->m_buffer, ntime );
}

void VoiceBase::SetFormat( DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels )
{
	m_format.cbSize = 0;
	m_format.wFormatTag = WAVE_FORMAT_PCM;
	m_format.nChannels = nChannels;
	m_format.nSamplesPerSec = nSamplesPerSec;
	m_format.wBitsPerSample = wBitsPerSample;
	m_format.nBlockAlign = nChannels * wBitsPerSample / 8;
	m_format.nAvgBytesPerSec = nSamplesPerSec * nChannels * wBitsPerSample / 8;
}

//////////////////////////////////////////////////////////////////////////

VoiceRecording::VoiceRecording()
	: m_hWaveIn( NULL )
{
	
}

VoiceRecording::~VoiceRecording()
{
	if ( IsOpen() )
	{
		Close();
	}
}

Bool VoiceRecording::Record()
{
	// 1. Prepare
	m_result = waveInPrepareHeader( m_hWaveIn, &m_header, sizeof(WAVEHDR) );
	GetMMResult( m_result );
	
	if ( m_result != MMSYSERR_NOERROR )
	{
		return false;
	}

	// 2. Add buffer
	m_result = waveInAddBuffer( m_hWaveIn, &m_header, sizeof(WAVEHDR) );
	GetMMResult( m_result );

	if ( m_result != MMSYSERR_NOERROR )
	{
		return false;
	}

	// 3. Start
	m_result = waveInStart( m_hWaveIn );
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Bool VoiceRecording::Stop()
{
	m_result = waveInStop( m_hWaveIn );
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Bool VoiceRecording::Open()
{
	if ( IsOpen() )
	{
		return false;
	}

	m_result = waveInOpen( &m_hWaveIn, (UINT)WAVE_MAPPER, &m_format, (DWORD)VoiceWaveInProc, (DWORD)this, CALLBACK_FUNCTION );
	GetMMResult( m_result );

	if ( m_result != MMSYSERR_NOERROR )
	{
		m_hWaveIn = NULL;

		return false;
	}
	else
	{
		return true;
	}
}

Bool VoiceRecording::Save( const String& absFilePath )
{
	HANDLE hFile = ::CreateFileW( absFilePath.AsChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD bytesWritten;
	WriteFile( hFile, "RIFF", 4, &bytesWritten, NULL);
	DWORD dwReturn = 36 + m_header.dwBytesRecorded;
	WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 
	WriteFile(hFile, "WAVE", 4, &bytesWritten, NULL); 

	WriteFile(hFile, "fmt ", 4, &bytesWritten, NULL); 
	dwReturn = 16; 
	WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 

	WriteFile(hFile, &m_format.wFormatTag, 2, &bytesWritten, NULL); 
	WriteFile(hFile, &m_format.nChannels, 2, &bytesWritten, NULL); 
	WriteFile(hFile, &m_format.nSamplesPerSec, 4, &bytesWritten, NULL); 
	dwReturn = m_format.nSamplesPerSec * m_format.nChannels * (m_format.wBitsPerSample / 8); 
	WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 
	dwReturn = m_format.nChannels * (m_format.wBitsPerSample / 8); 
	WriteFile(hFile, &dwReturn, 2, &bytesWritten, NULL); 
	WriteFile(hFile, &m_format.wBitsPerSample, 2, &bytesWritten, NULL); 

	WriteFile(hFile, "data", 4, &bytesWritten, NULL); 
	WriteFile(hFile, &m_header.dwBytesRecorded, 4, &bytesWritten, NULL); 
	WriteFile(hFile, m_header.lpData, m_header.dwBytesRecorded, &bytesWritten, NULL); 
	
	CloseHandle(hFile); 

	return true;
}

Bool VoiceRecording::Close()
{
	//m_result = waveInReset( m_hWaveIn );
	//GetMMResult( m_result );
	//if ( m_result != MMSYSERR_NOERROR )
	//	return false;

	m_result = waveInClose( m_hWaveIn );
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Bool VoiceRecording::IsOpen() const
{
	return m_hWaveIn != NULL;
}

Bool VoiceRecording::InternalWaveInProc()
{
	m_result = waveInUnprepareHeader( m_hWaveIn, &m_header, sizeof(WAVEHDR) );
	GetMMResult( m_result );

	OnRecordFinished();

	return m_result == MMSYSERR_NOERROR;
}

BOOL CALLBACK VoiceWaveInProc( HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	if ( uMsg == WIM_DATA )
	{
		VoiceRecording* voice = ( VoiceRecording* ) dwInstance;

		return voice->InternalWaveInProc() == MMSYSERR_NOERROR ? TRUE : FALSE;
	}

	return TRUE;
}

void VoiceRecording::OnRecordFinished()
{
	//...
}

//////////////////////////////////////////////////////////////////////////

VoicePlaying::VoicePlaying()
	: m_hWaveOut( NULL )
{
	
}

VoicePlaying::~VoicePlaying()
{
	if ( IsOpen() )
	{
		Close();
	}
}

Bool VoicePlaying::Play()
{
	m_result = waveOutPrepareHeader( m_hWaveOut, &m_header, sizeof(WAVEHDR) );
	GetMMResult( m_result );

	if ( m_result != MMSYSERR_NOERROR )
	{
		return false;
	}

	m_result = waveOutWrite( m_hWaveOut, &m_header, sizeof(WAVEHDR) );	
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Bool VoicePlaying::Pause()
{
	m_result = waveOutPause( m_hWaveOut );	
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Bool VoicePlaying::Unpause()
{
	m_result = waveOutRestart( m_hWaveOut );	
	GetMMResult( m_result );

	return m_result == MMSYSERR_NOERROR;
}

Uint32 VoicePlaying::GetTimeMS() const
{
	MMTIME mmtime;
	mmtime.wType = TIME_MS;

	MMRESULT res = waveOutGetPosition( m_hWaveOut, &mmtime, sizeof(mmtime) );	

	Uint32 time = (Uint32)mmtime.u.ms;

	return res == MMSYSERR_NOERROR ? time : 0;
}

void VoicePlaying::SetOffsetInSec( Float timeOffset )
{
	DWORD length = ( m_format.nSamplesPerSec * m_format.nChannels * m_format.wBitsPerSample * timeOffset ) / 8;

	m_header.lpData = (char*)m_buffer + length;
	m_header.dwBufferLength = m_header.dwBufferLength - length;
}

Bool VoicePlaying::Open()
{
	if ( IsOpen() )
	{
		return false;
	}

	m_result = waveOutOpen( &m_hWaveOut, WAVE_MAPPER , &m_format, (DWORD)VoiceWaveOutProc, (DWORD)this, CALLBACK_FUNCTION );
	GetMMResult( m_result );

	if ( m_result != MMSYSERR_NOERROR )
	{
		m_hWaveOut = NULL;
		return false;
	}
	else
	{
		return true;
	}
}

Bool VoicePlaying::Close()
{
	//m_result=waveOutReset( m_hWaveOut );
	//GetMMResult(m_result);
	//if ( m_result != MMSYSERR_NOERROR )
	//	return false;

	m_result = waveOutClose( m_hWaveOut );
	GetMMResult( m_result );

	m_hWaveOut = NULL;

	return m_result == MMSYSERR_NOERROR;
}

Bool VoicePlaying::IsOpen() const
{
	return m_hWaveOut != NULL;
}

Bool VoicePlaying::Load( const String& absFilePath )
{
	HANDLE hFile = ::CreateFileW( absFilePath.AsChar(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
	
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		return false;
	}	

	SetFilePointer( hFile, 12, NULL, FILE_BEGIN );

	DWORD bytesRead;
	DWORD dw;

	ReadFile(hFile, &dw, 4, &bytesRead, NULL);  
	ReadFile(hFile, &dw, 4, &bytesRead, NULL);  

	ReadFile(hFile, &m_format.wFormatTag, 2, &bytesRead, NULL); 
	ReadFile(hFile, &m_format.nChannels, 2, &bytesRead, NULL); 
	ReadFile(hFile, &m_format.nSamplesPerSec, 4, &bytesRead, NULL); 

	ReadFile(hFile, &dw, 4, &bytesRead, NULL);  
	ReadFile(hFile, &dw, 2, &bytesRead, NULL); 

	ReadFile(hFile, &m_format.wBitsPerSample, 2, &bytesRead, NULL); 

	ReadFile(hFile, &dw, 4, &bytesRead, NULL); 

	DWORD bufferSize;
	ReadFile(hFile, &bufferSize, 4, &bytesRead, NULL);

	{
		ClearBuffer();

		m_buffer = new Uint8[ bufferSize ];

		m_header.lpData = (char*)m_buffer;
		m_header.dwBufferLength = bufferSize;
		m_header.dwBytesRecorded = 0;
		m_header.dwUser = 0;
		m_header.dwFlags = 0;
		m_header.reserved = 0;
		m_header.lpNext = 0;
	}

	ReadFile(hFile, m_header.lpData, bufferSize, &bytesRead, NULL); 

	CloseHandle( hFile ); 

	return true;
}

Bool VoicePlaying::InternalWaveOutProc()
{
	m_result = waveOutUnprepareHeader( m_hWaveOut, &m_header, sizeof(WAVEHDR) );
	GetMMResult( m_result );

	OnPlayFinished();

	return m_result == MMSYSERR_NOERROR;
}

BOOL CALLBACK VoiceWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if ( uMsg == WOM_DONE )
	{
		VoicePlaying* voice = (VoicePlaying*) dwInstance;

		return voice->InternalWaveOutProc() ? TRUE : FALSE;
	}

	return TRUE;
}

void VoicePlaying::OnPlayFinished()
{
	//...
}

Bool VoicePlayingShot::PlayAndForget( const String& absFilePath, Float offsetInSec )
{
	if ( !Load( absFilePath ) )
	{
		DeleteMe();
		return false;
	}

	if ( offsetInSec > 0.f )
	{
		SetOffsetInSec( offsetInSec );
	}

	if ( !Open() )
	{
		DeleteMe();
		return false;
	}

	if ( !Play() )
	{
		DeleteMe();
		return false;
	}

	return true;
}

void VoicePlayingShot::OnPlayFinished()
{
	DeleteMe();
}

void VoicePlayingShot::DeleteMe()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////

namespace VoiceTest
{

void VoiceWaveTest()
{
	VoiceRecording record;
	VoicePlaying play;

	if( !record.PrepareBuffer(10) )    //prepare buffer for recording 10 seconds.
	{
		LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	if( !record.Open() )
	{
		LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	if( !play.PrepareBuffer(10) )    //prepare buffer for playing of 10 seconds of data
	{
		LOG_EDITOR( TXT("%s"), play.GetLastError().AsChar() );
		ASSERT( 0 );
	}
	
	if ( !play.Open() )
	{
		LOG_EDITOR( TXT("%s"), play.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	if ( record.IsOpen() )
	{
		if ( !record.Record() )
		{
			LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
			ASSERT( 0 );
		}
	}
	else
	{
		LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	Sleep( 3.f * 1000.f );

	if ( !record.Stop() )
	{
		LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	if ( !record.Save( TXT("c:\\recording.wav") ) )
	{
		LOG_EDITOR( TXT("%s"), record.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	Sleep( 5.f * 1000.f );

	VoicePlayingShot* shot = new VoicePlayingShot();
	shot->PlayAndForget( TXT("c:\\recording.wav"), 1.5f );

	/*if( !play.CopyBuffer( &record, 10 ) )
	{
		LOG_EDITOR( TXT("%s"), play.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	if ( play.IsOpen() )
	{
		if( !play.Play() )
		{
			LOG_EDITOR( TXT("%s"), play.GetLastError().AsChar() );
			ASSERT( 0 );
		}
	}
	else
	{
		LOG_EDITOR( TXT("%s"), play.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	Sleep( 12.f * 1000.f );

	VoicePlaying play2;

	if ( play2.Load( TXT("c:\\recording.wav") ) )
	{
		if ( !play2.Open() )
		{
			LOG_EDITOR( TXT("%s"), play2.GetLastError().AsChar() );
			ASSERT( 0 );
		}

		if( !play2.Play() )
		{
			LOG_EDITOR( TXT("%s"), play2.GetLastError().AsChar() );
			ASSERT( 0 );
		}
	}
	else
	{
		LOG_EDITOR( TXT("%s"), play2.GetLastError().AsChar() );
		ASSERT( 0 );
	}

	LOG_EDITOR( TXT("d. %f"), play2.GetDuration() );

	for ( Uint32 i=0; i<5*5; i++ )
	{
		Sleep( 0.2f * 1000.f );
		LOG_EDITOR( TXT("p. %u"), play2.GetTimeMS() );
	}

	VoicePlayingShot* shot = new VoicePlayingShot();
	shot->PlayAndForget( TXT("c:\\recording.wav") );

	Sleep( 0.4f * 1000.f );

	VoicePlayingShot* shot2 = new VoicePlayingShot();
	shot2->PlayAndForget( TXT("c:\\recording.wav") );*/
}

void LipsyncTest()
{
	SEdLipsyncCreator::GetInstance().CreateLipsync( TXT("VO_LUI1_001514_0162"), TXT("pl"), TXT("dsfdfsfds") );
}

}

namespace Voice
{

void PlayDebugSound( const String& path )
{
	::PlaySoundW( path.AsChar(), NULL, SND_FILENAME|SND_ASYNC );
}

}

EdLipsyncCreator::EdLipsyncCreator()
	: m_function( NULL )
{

}

Bool EdLipsyncCreator::LoadFunction()
{
	GDepot->GetAbsolutePath( m_toolPath );
	m_outPath = UNICODE_TO_ANSI( m_toolPath.AsChar() );
	m_outPath += "speech\\";
	m_outPathUni = m_toolPath + TXT("speech\\");

	m_toolPath = m_toolPath.StringBefore( TXT("\\"), true );
	m_toolPath = m_toolPath.StringBefore( TXT("\\"), true );

	m_toolPath += TXT("\\bin\\tools\\anno_lipsync");

	if( !SetDllDirectoryW( m_toolPath.AsChar() ) )
	{
		return false;
	}

	HINSTANCE handle = NULL;

	#if defined( RED_PLATFORM_WIN32 )
		handle = LoadLibraryW( TXT("re_lipsync_32.dll") );
	#elif defined( RED_PLATFORM_WIN64 )
		handle = LoadLibraryW( TXT("re_lipsync_x64.dll") );
	#endif

	if( !handle )
	{
		SetDllDirectory( NULL );

		return false;
	}

	m_function = (LipsyncFunction)GetProcAddress( handle, "create_lips_file" );
	m_functionError = (LipsyncFunctionError)GetProcAddress( handle, "get_last_error" );

	SetDllDirectory( NULL );

	StringAnsi ansiPath = UNICODE_TO_ANSI( m_toolPath.AsChar() );

	m_pozFile = ansiPath + "\\lipsync.poz";
	m_hmmFile = ansiPath + "\\anno_lipsync.hmm";
	m_trigFile = ansiPath + "\\anno_lipsync.trig";

	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "EN", ansiPath + "\\anno_lipsync.alex" );
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "FR", ansiPath + "\\french.alex" );
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "DE", ansiPath + "\\german.alex" );
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "PL", ansiPath + "\\polish.alex" );
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "RU", ansiPath + "\\russian.alex" );	
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "JP", ansiPath + "\\Japanese.alex" );
	new ( m_alexFiles ) TPair< StringAnsi, StringAnsi  >( "BR", ansiPath + "\\portuguese.alex" );

	return m_function!= NULL;	
}

const StringAnsi& EdLipsyncCreator::GetAlexFileForLang( const StringAnsi& _languageId )
{
	StringAnsi languageId = _languageId.ToUpper();
	for ( Uint32 i = 0; i < m_alexFiles.Size(); i++ )
	{
		if ( m_alexFiles[i].m_first == languageId )
		{
			return m_alexFiles[i].m_second;
		}
	}
	LOG_EDITOR( TXT("Couldnt find .alex file for %s language "), ANSI_TO_UNICODE( languageId.AsChar() ) );
	return m_alexFiles.Size() > 0 ? m_alexFiles[0].m_second : StringAnsi::EMPTY;
}

String EdLipsyncCreator::GetWavPath( const String& stringId, const String& languageId )
{
	if ( !m_function && !LoadFunction() )
	{
		return String::EMPTY;
	}

	StringAnsi fileName = UNICODE_TO_ANSI( stringId.AsChar() );
	StringAnsi lang = UNICODE_TO_ANSI( languageId.AsChar() );

	StringAnsi inputFileWav = m_outPath + lang + "\\audio\\" + fileName + ".wav";

	return ANSI_TO_UNICODE( inputFileWav.AsChar() );
}

String EdLipsyncCreator::GetLipsPath( const String& stringId, const String& languageId )
{
	if ( !m_function && !LoadFunction() )
	{
		return String::EMPTY;
	}

	StringAnsi fileName = UNICODE_TO_ANSI( stringId.AsChar() );
	StringAnsi lang = UNICODE_TO_ANSI( languageId.AsChar() );

	StringAnsi inputFileLips = m_outPath + lang + "\\lipsync\\" + fileName + ".re";

	return ANSI_TO_UNICODE( inputFileLips.AsChar() );
}

Bool EdLipsyncCreator::WriteTextToFile( const StringAnsi& absTextPath, const String& text )
{
	return GFileManager->SaveStringToFileWithUTF8( ANSI_TO_UNICODE( absTextPath.AsChar() ), text );
}

Bool EdLipsyncCreator::CreateLipsyncAbs( const String& absPath, const String& fileNameNoExt, const String& text, Bool onlyText /*= false */ )
{
	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	StringAnsi lang = UNICODE_TO_ANSI( language.AsChar() );

	if ( lang.Empty() || ( !m_function && !LoadFunction() ) )
	{
		return false;
	}

	StringAnsi fileNameAnsi = UNICODE_TO_ANSI( fileNameNoExt.AsChar() );
	StringAnsi absPathAnsi = UNICODE_TO_ANSI( absPath.AsChar() );

	StringAnsi inputFileWav =	absPathAnsi + "\\audio\\" + fileNameAnsi + ".wav";
	StringAnsi inputFileTxt =	absPathAnsi + "\\text\\" + fileNameAnsi + ".txt";
	StringAnsi outputFileLips = absPathAnsi + "\\lipsync\\" + fileNameAnsi + ".re";

	String preprocessedText;
	PreprocessTextForAnnosoftLipsyncSDK( text, preprocessedText );

	if ( !WriteTextToFile( inputFileTxt, preprocessedText ) )
	{
		return false;
	}

	if ( onlyText )
	{
		return true;
	}

	Bool ret = false;

	size_t chr;
	StringAnsi suff = fileNameAnsi;
	if ( fileNameAnsi.FindCharacter( '_', chr ) )
	{
		suff = fileNameAnsi.LeftString( chr );
	}

	StringAnsi ansiPath = UNICODE_TO_ANSI( m_toolPath.AsChar() );
	StringAnsi pozFile = ansiPath + "\\poses\\" + suff + ".poz";

	if ( !GFileManager->FileExist( ANSI_TO_UNICODE( pozFile.AsChar() ) ) )
	{
		pozFile = m_pozFile;
	}

	ret = DoGenerate( inputFileWav.AsChar(), inputFileTxt.AsChar(), pozFile.AsChar(), m_hmmFile.AsChar(), m_trigFile.AsChar(), GetAlexFileForLang( lang ).AsChar(), outputFileLips.AsChar(), LIPSYNCOFFSET );

	return ret;
}

Bool EdLipsyncCreator::CreateLipsync( const String& fileNameNoExt, const String& languageId, const String& text, Bool onlyText )
{
	if ( !m_function && !LoadFunction() )
	{
		return false;
	}

	StringAnsi fileNameAnsi = UNICODE_TO_ANSI( fileNameNoExt.AsChar() );
	StringAnsi lang = UNICODE_TO_ANSI( languageId.AsChar() );

	StringAnsi inputFileWav =	m_outPath + lang + "\\audio\\" + fileNameAnsi + ".wav";
	StringAnsi inputFileTxt =	m_outPath + lang + "\\text\\" + fileNameAnsi + ".txt";
	StringAnsi outputFileLips = m_outPath + lang + "\\lipsync\\" + fileNameAnsi + ".re";

	String preprocessedText;
	PreprocessTextForAnnosoftLipsyncSDK( text, preprocessedText );

	if ( !WriteTextToFile( inputFileTxt, preprocessedText ) )
	{
		return false;
	}

	if ( onlyText )
	{
		return true;
	}

	bool ret = false;

	size_t chr;
	StringAnsi suff = fileNameAnsi;
	if( fileNameAnsi.FindCharacter('_', chr) )
	{
		suff = fileNameAnsi.LeftString(chr);
	}

	StringAnsi ansiPath = UNICODE_TO_ANSI( m_toolPath.AsChar() );
	StringAnsi pozFile = ansiPath + "\\poses\\" + suff + ".poz";

	if ( !GFileManager->FileExist( ANSI_TO_UNICODE( pozFile.AsChar() ) ) )
	{
		pozFile = m_pozFile;
	}

	ret = DoGenerate( inputFileWav.AsChar(), inputFileTxt.AsChar(), pozFile.AsChar(), m_hmmFile.AsChar(), m_trigFile.AsChar(), GetAlexFileForLang( lang ).AsChar(), outputFileLips.AsChar(), LIPSYNCOFFSET );

	return ret;
}

Bool EdLipsyncCreator::DoGenerate( const char* wav, const char* txt, const char* pos, const char* hmm, const char* trig, const char* alex, const char* output, float lipsoffset, bool utf, int smoothit )
{
	Bool ret = false;
	__try
	{
		ret = m_function( wav, txt, pos, hmm, trig, alex, output, lipsoffset, utf, smoothit );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = false;
	}

	return ret;
}

const char* EdLipsyncCreator::GetLastError() const
{
	return m_functionError ? m_functionError() : NULL;
}

/*
Uses text-to-speech (Microsoft Speech API) to synthesize specified text into a wav file.

\param fileNameNoExt Name of resulting wav file, without extension.
\param languageId String specifying language, e.g. "en".
\param text Text to be spoken.
\return True - success, false - failure.

CreateWav() vs CreateWavAbs()
CreateWavAbs() creates wav file at arbitrary location. CreateWav() creates wav file at location defined by the game.
*/
Bool EdLipsyncCreator::CreateWav( const String& fileNameNoExt, const String& languageId, const String& text )
{
	String fileWav = m_outPathUni + languageId + TXT("\\audio\\") + fileNameNoExt + TXT(".wav");

	return CreateWavAbs( fileWav, text );
}

namespace
{
	HRESULT CopyDataFromStreamToBuffer( IStream* iStream, TDynArray< Uint8 >& data, WORD blockAlign )
	{
		if ( iStream )
		{
			TDynArray< Uint8 > chunk( 256 );

			ULONG read = 0;

			LARGE_INTEGER zero = {0};
			HRESULT hr = iStream->Seek( zero, STREAM_SEEK_SET, NULL );

			if( !SUCCEEDED( hr ) )
			{
				return hr;
			}

			while ( 1 ) 
			{
				hr = iStream->Read( chunk.TypedData(), chunk.Size()*sizeof(Uint8), &read );

				data.PushBack( chunk );

				if( !SUCCEEDED( hr ) )
				{
					return hr;
				}

				const Bool eos = (S_FALSE == hr);

				if ( 0 != read % blockAlign ) 
				{
					return -1;
				}

				if ( eos )
				{
					break;
				}
			}

			return hr;
		}

		return -1;
	}

	Bool DumpBufferToFile( const String& fileNameAbs, const TDynArray< Uint8 >& data, WAVEFORMATEX& fmt )
	{
		HANDLE hFile = ::CreateFileW( fileNameAbs.AsChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == INVALID_HANDLE_VALUE )
		{
			return false;
		}

		DWORD bytesWritten;
		WriteFile( hFile, "RIFF", 4, &bytesWritten, NULL);
		DWORD dwReturn = 36 + data.Size();
		WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 
		WriteFile(hFile, "WAVE", 4, &bytesWritten, NULL); 

		WriteFile(hFile, "fmt ", 4, &bytesWritten, NULL); 
		dwReturn = 16; 
		WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 

		WriteFile(hFile, &fmt.wFormatTag, 2, &bytesWritten, NULL); 
		WriteFile(hFile, &fmt.nChannels, 2, &bytesWritten, NULL); 
		WriteFile(hFile, &fmt.nSamplesPerSec, 4, &bytesWritten, NULL); 
		dwReturn = fmt.nSamplesPerSec * fmt.nChannels * (fmt.wBitsPerSample / 8); 
		WriteFile(hFile, &dwReturn, 4, &bytesWritten, NULL); 
		dwReturn = fmt.nChannels * (fmt.wBitsPerSample / 8); 
		WriteFile(hFile, &dwReturn, 2, &bytesWritten, NULL); 
		WriteFile(hFile, &fmt.wBitsPerSample, 2, &bytesWritten, NULL); 

		DWORD dataSize = (DWORD)data.Size();
		WriteFile(hFile, "data", 4, &bytesWritten, NULL); 
		WriteFile(hFile, &dataSize, 4, &bytesWritten, NULL); 
		WriteFile(hFile, data.TypedData(), dataSize, &bytesWritten, NULL); 

		CloseHandle(hFile);

		return true;
	}
}

/*
Uses text-to-speech (Microsoft Speech API) to synthesize specified text into a wav file.

\param fileNameAbs Absolute path specifying location and name of resulting wav file.
\param text Text to be spoken.
\return True - success, false - failure.

CreateWav() vs CreateWavAbs()
CreateWavAbs() creates wav file at arbitrary location. CreateWav() creates wav file at location defined by the game.
*/
Bool EdLipsyncCreator::CreateWavAbs( const String& fileNameAbs, const String& text )
{
	ISpVoice*	pVoice = NULL;
	ISpStream*	pStream = NULL;
	IStream*	iStream = NULL;

	if ( FAILED( ::CoInitialize(NULL) ) )
	{
		return false;
	}

	HRESULT hr;

	hr = CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice );
	hr = CoCreateInstance( CLSID_SpStream, nullptr, CLSCTX_ALL, __uuidof(ISpStream), (void**)&pStream );

	#define CHECK_EXIT if(!SUCCEEDED(hr)) { if ( pVoice ) pVoice->Release(); if ( pStream ) pStream->Release(); if ( iStream ) iStream->Release(); return false; }

	CHECK_EXIT;

	// VERSION A works but annosoft can not read wav with extra data inside. So I stream data into memory and save wav using my functions - VERSION B
	/* VERSION A
	{
		CSpStreamFormat	cAudioFmt;

		hr = cAudioFmt.AssignFormat( SPSF_48kHz16BitMono );

		CHECK_EXIT;

		hr = SPBindToFile( fileNameAbs.AsChar(), SPFM_CREATE_ALWAYS, &pStream, &cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr() );
	}*/

	// VERSION B
	WAVEFORMATEX fmt = {};
	{
		fmt.wFormatTag = WAVE_FORMAT_PCM;
		fmt.nChannels = 1;
		fmt.nSamplesPerSec = 48000;
		fmt.wBitsPerSample = 16;
		fmt.cbSize = 0;
		fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
		fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
	}

	hr = -1;

	iStream = SHCreateMemStream( NULL, 0 );
	if ( iStream )
	{
		hr = pStream->SetBaseStream( iStream, SPDFID_WaveFormatEx, &fmt );
	}

	CHECK_EXIT;

	hr = pVoice->SetOutput( pStream, TRUE );
	
	CHECK_EXIT;

	hr = pVoice->Speak( text.AsChar(), SPF_DEFAULT, NULL );

	CHECK_EXIT;

	TDynArray< Uint8 > data;
	hr = CopyDataFromStreamToBuffer( iStream, data, fmt.nBlockAlign );

	CHECK_EXIT;

	#undef CHECK_EXIT

	hr = pStream->Close();

	pStream->Release();
	pVoice->Release();

	::CoUninitialize();

	DumpBufferToFile( fileNameAbs, data, fmt );

	return true;
}

#undef FindResource

Bool EdLipsyncCreator::CreateWavAna( const String& fileNameAbs, TDynArray< Float >& outData )
{
	if ( !GFileManager->FileExist( fileNameAbs ) )
	{
		return false;
	}

	SECURITY_ATTRIBUTES securityAttribs; 
	securityAttribs.nLength = sizeof(SECURITY_ATTRIBUTES); 
	securityAttribs.bInheritHandle = TRUE; 
	securityAttribs.lpSecurityDescriptor = NULL; 

	STARTUPINFOW startupInfo;
	ZeroMemory( &startupInfo, sizeof( STARTUPINFO ) );
	startupInfo.cb = sizeof( STARTUPINFO );

	PROCESS_INFORMATION processInformation;    
	ZeroMemory( &processInformation, sizeof( PROCESS_INFORMATION ) );

	String absPath;
	GDepot->GetAbsolutePath( absPath );
	absPath = absPath.StringBefore( TXT("\\"), true );
	absPath = absPath.StringBefore( TXT("\\"), true );

	const String processDirectory = absPath + TXT("\\bin\\tools\\voiceAnalyzer\\");
	const String tempFilePath = processDirectory + TXT("temp.csv");
	const String appPath = processDirectory + TXT("voiceAnalyzer.exe");
	const String commandLine = String::Printf( TXT("%s %s %s"), appPath.AsChar(), fileNameAbs.AsChar(), tempFilePath.AsChar() );

	WCHAR processCommandline[ 512 ];
	wcscpy_s( processCommandline, ARRAYSIZE(processCommandline), commandLine.AsChar() );

	WCHAR processAppPath[ 512 ];
	wcscpy_s( processAppPath, ARRAYSIZE(processAppPath), appPath.AsChar() );

	GFileManager->DeleteFile( tempFilePath );

	if ( !CreateProcessW( NULL, processCommandline, NULL, NULL, true, CREATE_NO_WINDOW, NULL, processDirectory.AsChar(), &startupInfo, &processInformation ) )
	{
		return false;
	}

	WaitForSingleObject( processInformation.hProcess, INFINITE );

	IFile *file = GFileManager->CreateFileReader( tempFilePath, FOF_AbsolutePath );
	if ( file )
	{
		String fileData;
		if( GFileManager->LoadFileToString( file, fileData ) && !fileData.Empty() )
		{
			const TDynArray< String > tokens = fileData.Split( TXT("\n") );
			for ( Uint32 i=0; i<tokens.Size(); ++i )
			{
				const String& token = tokens[ i ];
				if ( !token.Empty() )
				{
					Float valFloat = 0.f;


					//temp fix for scientific notation in csv file
					if ( token.ContainsCharacter( 'e', true  ) )
					{
						Float power;
						const TDynArray< String > sciTokens = token.Split( TXT("e") );
						VERIFY( FromString( sciTokens[0], valFloat ) );
						VERIFY( FromString( sciTokens[1], power ) );	
						valFloat = Red::Math::MPow( valFloat, power );
					}
					else
					{
						VERIFY( FromString( token, valFloat ) );
					}					
					outData.PushBack( valFloat );
				}
			}
		}

		delete file;

		return !fileData.Empty();
	}

	return false;
}

Bool EdLipsyncCreator::AnalyzeLineDataSync( const String& stringId, const String& languageId, Float lineDuration, SLineAnaResult& out )
{
	if ( GetAnaDataResult( stringId, languageId, out ) )
	{
		return true;
	}

	const String pathWav = GetWavPath( stringId, languageId );
	TDynArray< Float > data;
	if ( ! SEdLipsyncCreator::GetInstance().CreateWavAna( pathWav, data ) )
	{
		return false;
	}

	SLineAnaResult& ref = m_analysisResults.GetRef( TPair<String,String>( stringId, languageId ) );

	Uint32 lastLocalMin = 0;
	Uint32 dataSize = data.Size() - 1;

	for ( Uint32 i = 1; i < dataSize; ++i )
	{	
		if (  data[i-1] > data[i] && data[i+1] > data[i] )//local min
		{
			lastLocalMin = i;
		}
		else if (  data[i-1] < data[i] && data[i+1] < data[i] )//local max
		{			
			Float slopeLength	= i - lastLocalMin;
			Float slopeValue	= data[i] - data[lastLocalMin];

			Float weight = ( data[i]* Max<Float>(data[i] - data[lastLocalMin], 0.f ) ); // / Float( i - lastLocalMin );
			Float pos = Float( i )/data.Size();

			ref.expressionPoints.PushBack( TPair< Float, Float >( pos, weight ) );
		}
	}
	struct CDataComparer
	{
		Bool m_sortByPos;
		CDataComparer( Bool sortByPos ) : m_sortByPos( sortByPos )
		{}
		Bool operator()( const TPair< Float, Float >& x, const TPair< Float, Float >& y ) const
		{
			return m_sortByPos ? x.m_first > y.m_first :  x.m_second > y.m_second;
		}
	};

	TDynArray< TPair< Float, Float > >  resCopy = ref.expressionPoints;
	ref.expressionPoints.ClearFast();


	Float firstTimeWindowStart = 0.f;
	Float timeWindow = 1.f;
	Float secondTimeWindowStart = -timeWindow / 2.f;

	TPair< Float, Float > highestVal1( 0.0f, -1.0f );
	TPair< Float, Float > highestVal2(0.0f, -1.0f );

	for( Uint32 i = 0; i < resCopy.Size(); i ++ )
	{
		Float markerPosition = resCopy[i].m_first * lineDuration;
		if ( ( i == resCopy.Size() -1 ) || ( markerPosition > firstTimeWindowStart + timeWindow  ) )
		{
			ref.expressionPoints.PushBackUnique( highestVal1 );
			highestVal1 = TPair< Float, Float >( 0.f, -1.f );
			firstTimeWindowStart += timeWindow;
		}
		if ( ( i == resCopy.Size() -1 ) || ( markerPosition > secondTimeWindowStart + timeWindow )  )
		{
			ref.expressionPoints.PushBackUnique( highestVal2 );
			highestVal2 = TPair< Float, Float >( 0.f, -1.f );
			secondTimeWindowStart += timeWindow;
		}

		if ( highestVal1.m_second < resCopy[i].m_second )
		{
			highestVal1 = resCopy[i];
		}
		if ( highestVal2.m_second < resCopy[i].m_second )
		{
			highestVal2 = resCopy[i];
		}			
	}

	Sort( ref.expressionPoints.Begin(), ref.expressionPoints.End(), CDataComparer( false ) );
	ref.expressionPoints.Resize( Min<Uint32>( ref.expressionPoints.Size(), 9 ) );

	out = ref;
	return true;	
}

Bool EdLipsyncCreator::GetAnaDataResult( const String& stringId, const String& languageId, SLineAnaResult& out )
{
	auto anaReslut = m_analysisResults.Find( TPair<String,String>( stringId, languageId ) );
	if ( anaReslut != m_analysisResults.End() )
	{
		out = (*anaReslut).m_second;
		return true;
	}
	return false;
}
