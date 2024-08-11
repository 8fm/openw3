/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CR4KinectSpeechRecognizerListenerScriptProxyDelegeate
{
public:
	//! check is Kinect supported with current locale
	virtual Bool IsSupported() const = 0;

	//! is Kinect speech recognition enabled
	virtual Bool IsEnabled() const = 0;

	//! turn off/on Kinect speech recognition
	virtual void SetEnabled( const Bool val ) = 0;

};
class CR4KinectSpeechRecognizerListenerScriptProxy: public CObject
{
	DECLARE_ENGINE_CLASS( CR4KinectSpeechRecognizerListenerScriptProxy, CObject, 0 );

public:
	CR4KinectSpeechRecognizerListenerScriptProxy();

	void SetDelegate( CR4KinectSpeechRecognizerListenerScriptProxyDelegeate* scriptProxyDelegate );

private:
	void funcIsSupported( CScriptStackFrame& stack, void* result );
	void funcIsEnabled( CScriptStackFrame& stack, void* result );
	void funcSetEnabled( CScriptStackFrame& stack, void* result );

private:
	CR4KinectSpeechRecognizerListenerScriptProxyDelegeate* m_scriptProxyDelegate;

};

BEGIN_CLASS_RTTI( CR4KinectSpeechRecognizerListenerScriptProxy )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "IsSupported", funcIsSupported );
	NATIVE_FUNCTION( "IsEnabled",   funcIsEnabled );
	NATIVE_FUNCTION( "SetEnabled",  funcSetEnabled );
END_CLASS_RTTI();

#if defined(RED_KINECT)

#include "../../durango/platformDurango/kinectSpeechRecognizer.h"

class CR4KinectSpeechRecognizerListener: public IKinectSpeechRecognizerListener, public CR4KinectSpeechRecognizerListenerScriptProxyDelegeate
{

public:
	CR4KinectSpeechRecognizerListener( CR4KinectSpeechRecognizerListenerScriptProxy* scriptProxy );
	virtual ~CR4KinectSpeechRecognizerListener();

public:

	//! start capture commands
	void Start();
	//! stop capture commands
	void Stop();

	void Update();

	//! Called when need to refresh settings (i.e. change user profile)
	void RefreshSettings();

	//! Generate debug viewport fragments
	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! CR4KinectSpeechRecognizerListenerScriptProxy
public:
	virtual Bool IsSupported() const { return m_supported; }
	virtual Bool IsEnabled() const;
	virtual void SetEnabled(const Bool val);

private:
	virtual void AudioProblem( EKinectSpeechRecognizerAudioProblem audioProblem );
	virtual void RecognizedCommand( SKinectSpeechRecognizedCommand recognizedCommand );
	virtual void HypothesisAvailable( const String& hypothesis );

private:

	void RegisterRegisterCommands( CKinectSpeechRecognizer* kinect );
	void LoadGrammarsFromFiles( CKinectSpeechRecognizer* kinect );

private:
	void OnControllerEvent( const EControllerEventType& event );

private:
	CR4KinectSpeechRecognizerListenerScriptProxy* m_scriptProxy;
	Red::Threads::CMutex						  m_speechRecognizerMutex;

	Uint32										  m_lastAudioProblem;
	Bool										  m_lastAudioProblemSet;

	String										  m_lastHypothesis;
	Bool										  m_lastHypothesisSet;

	Uint32										  m_lastRecognizedCommandSet;
	Uint32										  m_lastRecognizedCommandId;
	TDynArray<String>							  m_lastSemanticNames;
	TDynArray<String>							  m_lastSemanticValues;
	Float										  m_lastConfidenceScore;

	Bool										  m_supported;
	Bool										  m_previouslyEnabled;		// keep track of IsKinectEnabled option changes
	Bool										  m_started;
};

#endif //NO_KINECT
