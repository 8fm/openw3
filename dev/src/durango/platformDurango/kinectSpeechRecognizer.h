#pragma once
#if defined( RED_KINECT )
#include "../../common/platformCommon/platformFeature.h"

enum EKinectSpeechRecognizerAudioProblem
{ 
	KAP_NoSignal,
	KAP_TooFast,
	KAP_TooLoud,
	KAP_TooNoisy,
	KAP_TooQuiet,
	KAP_TooSlow
};

struct SKinectSpeechRecognizedCommand
{
	int								m_id;
	THashMap<String, String>		m_semantic;
	String							m_orginalText;
	Float							m_confidenceScore;
};

//! IKinectSpeechRecognizerListener is interface used as callback for Kinect Speech Recognizer listeners
//! IMPORTANT: IKinectSpeechRecognizerListener METHODS CAN BE CALLED FROM ANY THREAD 
class IKinectSpeechRecognizerListener
{
public:
	virtual void AudioProblem( EKinectSpeechRecognizerAudioProblem audioProblem ) = 0;
	virtual void RecognizedCommand( SKinectSpeechRecognizedCommand recognizedCommand  ) = 0;
	virtual void HypothesisAvailable( const String& hypothesis ) = 0;
};

//! This class serves as access point to Kinect speech recognizer.
class CKinectSpeechRecognizer: CPlatformFeature
{
public:
	CKinectSpeechRecognizer();
	virtual ~CKinectSpeechRecognizer();

	Bool IsRecognizerInitialized();

	void RegisterListener( IKinectSpeechRecognizerListener* listener );
	void UnregisterListener( IKinectSpeechRecognizerListener* listener );
	void UnregisterAllListeners();
	
	void RemoveAllGrammars();

	void RegisterCommandRuleWithId( const String& ruleName, Uint32 id );
	Bool LoadGrammarFromFile( const String& name, const String& path ); 
	Bool EnableGrammar( const String& name, const Bool enable );

	void Start();
	void Stop();

private:
	void*						m_kinectSpeechRecognizer;
	String						m_currentLanguage;
	THashMap<String, String>	m_loadedGrammars;
};
#endif