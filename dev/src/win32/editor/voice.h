
#pragma once

#include "MMSystem.h"

class VoiceBase  
{
protected:
	String			m_lastError;
	MMRESULT		m_result;
	
	Uint8*			m_buffer;
	WAVEHDR			m_header;
	WAVEFORMATEX	m_format;

public:
	VoiceBase();
	virtual ~VoiceBase();

	void SetFormat( DWORD nSamplesPerSec, WORD wBitsPerSample, WORD nChannels );

	Bool PrepareBuffer( DWORD ntime );

	Bool CopyBuffer( const VoiceBase* voice, DWORD ntime );

	const String& GetLastError() const;

	Float GetDuration() const;

protected:
	void GetMMResult( MMRESULT res );

	void ClearBuffer();
	Bool CopyBuffer( LPVOID lpBuffer, DWORD ntime );
};

//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK VoiceWaveInProc(
	HWAVEIN hwi,       
	UINT uMsg,         
	DWORD dwInstance,  
	DWORD dwParam1,    
	DWORD dwParam2     
	);

class VoiceRecording : public VoiceBase  
{
	HWAVEIN		m_hWaveIn;

public:
	VoiceRecording();
	virtual ~VoiceRecording();

	Bool Open();
	Bool Close();

	Bool IsOpen() const;

	Bool Record();
	Bool Stop();


	Bool Save( const String& absFilePath );

public:
	Bool InternalWaveInProc();

protected:
	virtual void OnRecordFinished();
};

//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK VoiceWaveOutProc(
	HWAVEOUT hwi,       
	UINT uMsg,         
	DWORD dwInstance,  
	DWORD dwParam1,    
	DWORD dwParam2     
	);

class VoicePlaying : public VoiceBase  
{
	HWAVEOUT	m_hWaveOut;

public:
	VoicePlaying();
	virtual ~VoicePlaying();

	Bool Open();
	Bool Close();
	
	Bool IsOpen() const;

	Bool Play();

	Bool Pause();
	Bool Unpause();

	Uint32 GetTimeMS() const;

	void SetOffsetInSec( Float timeOffset );

	Bool Load( const String& absFilePath );

public:
	Bool InternalWaveOutProc();

protected:
	virtual void OnPlayFinished();
};

class VoicePlayingShot : public VoicePlaying
{
public:
	Bool PlayAndForget( const String& absFilePath, Float offsetInSec = 0.f );

protected:
	virtual void OnPlayFinished();

	void DeleteMe();
};

//////////////////////////////////////////////////////////////////////////

namespace VoiceTest
{
	void VoiceWaveTest();

	void LipsyncTest();
}

//////////////////////////////////////////////////////////////////////////

namespace Voice
{
	void PlayDebugSound( const String& path );
}

//////////////////////////////////////////////////////////////////////////

class EdLipsyncCreator
{
	typedef bool (*LipsyncFunction)( const char* wav, const char* txt, const char* pos, const char* hmm, const char* trig, const char* alex, const char* output, float lipsoffset, bool utf, int smoothit );
	typedef const char* (*LipsyncFunctionError)(  );

	LipsyncFunction		m_function;
	LipsyncFunctionError m_functionError;

	String				m_toolPath;
	StringAnsi			m_outPath;
	String				m_outPathUni;

	StringAnsi			m_pozFile;
	StringAnsi			m_hmmFile;
	StringAnsi			m_trigFile;
	TDynArray< TPair< StringAnsi , StringAnsi > >	m_alexFiles;

public:
	EdLipsyncCreator();

	Bool CreateLipsync( const String& fileNameNoExt, const String& languageId, const String& text, Bool onlyText = false );
	Bool CreateLipsyncAbs( const String& absPath, const String& fileNameNoExt, const String& text, Bool onlyText = false );

	Bool CreateWav( const String& fileNameNoExt, const String& languageId, const String& text );
	Bool CreateWavAbs( const String& fileNameAbs, const String& text );

	Bool CreateWavAna( const String& fileNameAbs, TDynArray< Float >& outData );

	const char* GetLastError() const;

public:

	struct SLineAnaResult
	{
		TDynArray< TPair< Float, Float > >  expressionPoints;
		Float								voiceStartTime;
		Float								voiceEndTime;
	};

	Bool AnalyzeLineDataSync( const String& stringId, const String& languageId, Float lineDuration, SLineAnaResult& out );
	//Bool AnalyzeLineDataAsync( const String& stringId, const String& languageId );	
	Bool GetAnaDataResult( const String& stringId, const String& languageId, SLineAnaResult& out );

	THashMap< TPair< String, String >, SLineAnaResult >		m_analysisResults;
	
public:
	String GetLipsPath( const String& stringId, const String& languageId );
	String GetWavPath( const String& stringId, const String& languageId );

private:
	Bool LoadFunction();
	Bool WriteTextToFile( const StringAnsi& absTextPath, const String& text );

	Bool DoGenerate( const char* wav, const char* txt, const char* pos, const char* hmm, const char* trig, const char* alex, const char* output, float lipsoffset, bool utf = true, int smoothit = 1 );
	const StringAnsi& GetAlexFileForLang( const StringAnsi& languageId );
};

typedef TSingleton< EdLipsyncCreator > SEdLipsyncCreator;

//////////////////////////////////////////////////////////////////////////
