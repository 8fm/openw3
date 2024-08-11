/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#if defined( RED_KINECT )

#include "kinectSpeechRecognizer.h"
#include "readerRandomAccessStream.h"
#include "../../common/core/depot.h"

#include <Windows.h>
#include <Unknwn.h>
#include <collection.h> 

using namespace Platform::Collections; 
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::Xbox::Speech::Recognition;


ref class KinectSpeechRecognizerClass sealed
{
public:
	KinectSpeechRecognizerClass();

	virtual ~KinectSpeechRecognizerClass();

	Bool Initialize( Platform::IntPtr parent );

	Bool IsRecognizerInitialized();

	void RegisterCommandRuleWithId( Platform::String^ ruleName, int id );

	void RegisterListener( Platform::IntPtr listener );
	void UnregisterListener( Platform::IntPtr listener );
	void UnregisterAllListeners();

	void Start();
	void Stop();

	void RemoveAllGrammars();
	bool LoadGrammarFromFile( Platform::String^ name, Platform::String^ path, Bool absolutePath ); 
	bool EnableGrammar( Platform::String^ name, bool enable ); 

	
	//! m_CrraToken
	void ContinuousRecognitionResultAvailable( SpeechRecognizer^ reco, ContinuousSpeechRecognitionResultEventArgs^ args );
	//! m_CrscToken
	void ContinuousRecognitionStatusChanged( SpeechRecognizer^ reco, ContinuousSpeechRecognitionStatusEventArgs^ args );
	//! m_HaToken
	void HypothesisAvailable( SpeechRecognizer^ reco, SpeechHypothesisResultEventArgs^ args );
	//! m_ApoToken
	void AudioProblemOccurred( SpeechRecognizer^ reco, SpeechAudioProblemOccurredEventArgs^ args );

		
	bool IsLanguageSupported( Platform::String^ redKitLanguageCode );

	Platform::String^ GetCurrentLanguage() { return m_currentLanguage; }

private:
	//! if local is not supported return nullptr
	Platform::String^ GetSupportedLocal( SpeechRecognizerInformation^ speechRecognizerInformation );
	void CreateRecognizer();
	void AttachRecognizerEventHandlers();
	//! if can not set then return false and set default Xbox SpeechRecognizer
	bool SetRecognizer( Platform::String^ redKitLanguageCode );

private:
	CKinectSpeechRecognizer*	m_parent;
	SpeechRecognizer^			m_reco;
	Platform::String^			m_currentLanguage;
	bool                m_isSpeechRunning;
	bool                m_isSpeechClosing;
	HANDLE              m_speechClosedEvent;

	Windows::Foundation::EventRegistrationToken  m_CrraToken;
	Windows::Foundation::EventRegistrationToken  m_CrscToken;
	Windows::Foundation::EventRegistrationToken  m_HaToken;
	Windows::Foundation::EventRegistrationToken  m_ApoToken;

	SpeechRecognizerInformation^				 m_defaultSpeechRecognizer;
	
	//! list of languages supported by the game
	Platform::Collections::Vector<Platform::String^>^ m_supportedLanguages;

	//! Speech Recognizer to RedKit language code map
	Platform::Collections::Map<Platform::String^, Platform::String^>^	 m_languageMapper;
	//! this mapping combine few locals with the one grammar file
	//! if local is not present in this map it has mapping 1-1
	Platform::Collections::Map<Platform::String^, Platform::String^>^	 m_localMapper;

	//! top rule id from *.grxml to enum value
	Platform::Collections::Map<Platform::String^, int>^				 m_ruleMapper;

	Platform::Collections::Vector< Platform::IBox<Platform::IntPtr>^ >^	m_speechRecognizerListener;
	Red::Threads::CMutex												m_speechRecognizerMutex;
};

KinectSpeechRecognizerClass::KinectSpeechRecognizerClass() :
	m_isSpeechClosing( false ),
	m_isSpeechRunning( false ),
	m_speechClosedEvent( INVALID_HANDLE_VALUE ),
	m_reco( nullptr ),
	m_parent( NULL )
{
	
	m_supportedLanguages = ref new Platform::Collections::Vector<Platform::String^>();

	m_supportedLanguages->Append( TXT("DE") );
	m_supportedLanguages->Append( TXT("EN") );
	m_supportedLanguages->Append( TXT("GB") );
	m_supportedLanguages->Append( TXT("ES") );
	m_supportedLanguages->Append( TXT("ESMX") );
	m_supportedLanguages->Append( TXT("FR") );
	m_supportedLanguages->Append( TXT("IT") );
	m_supportedLanguages->Append( TXT("JP") );
	m_supportedLanguages->Append( TXT("BR") );
		
	m_languageMapper = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();
	m_languageMapper->Insert(TXT("de-DE"), TXT("DE") );
	m_languageMapper->Insert(TXT("en-AU"), TXT("EN") );
	m_languageMapper->Insert(TXT("en-CA"), TXT("EN") );
	m_languageMapper->Insert(TXT("en-GB"), TXT("GB") );
	m_languageMapper->Insert(TXT("en-US"), TXT("EN") );
	m_languageMapper->Insert(TXT("es-ES"), TXT("ES") );
	m_languageMapper->Insert(TXT("fr-CA"), TXT("FR") );
	m_languageMapper->Insert(TXT("fr-FR"), TXT("FR") );
	m_languageMapper->Insert(TXT("it-IT"), TXT("IT") );
	m_languageMapper->Insert(TXT("ja-JP"), TXT("JP") );
	m_languageMapper->Insert(TXT("pt-BR"), TXT("BR") );

	m_localMapper = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();
	m_localMapper->Insert(TXT("en-AU"), TXT("en-US") );
	m_localMapper->Insert(TXT("en-CA"), TXT("en-US") );
	m_localMapper->Insert(TXT("fr-CA"), TXT("fr-FR") );
	m_localMapper->Insert(TXT("es-MX"), TXT("es-ES") );
	
	m_ruleMapper = ref new Platform::Collections::Map<Platform::String^, int>();

	m_speechRecognizerListener = ref new Platform::Collections::Vector< Platform::IBox<Platform::IntPtr>^ >();
}

void KinectSpeechRecognizerClass::RegisterCommandRuleWithId( Platform::String^ ruleName, int id )
{
	m_ruleMapper->Insert( ruleName, id );
}

KinectSpeechRecognizerClass::~KinectSpeechRecognizerClass()
{
	if ( m_reco != nullptr )
	{
		m_isSpeechClosing = true;

		m_reco->StopContinuousRecognition();

		if ( m_speechClosedEvent != INVALID_HANDLE_VALUE )
		{
			if ( m_isSpeechRunning )
			{
				static const DWORD  SPEECH_CLOSE_TIMEOUT = 5000;
				HRESULT _hr_ = ( ( WaitForSingleObject( m_speechClosedEvent, SPEECH_CLOSE_TIMEOUT) != WAIT_OBJECT_0 ) ? HRESULT_FROM_WIN32( GetLastError() ) : S_OK ); 
				if( FAILED( _hr_ ) )
				{
					RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Kinect speech recodnizer WaitForSingleObject timed out posible future crash!!!" ) );
				}
			}

			CloseHandle( m_speechClosedEvent );
			m_speechClosedEvent = INVALID_HANDLE_VALUE;
		}
		m_reco->AudioProblemOccurred					-= m_ApoToken;
		m_reco->ContinuousRecognitionResultAvailable	-= m_CrraToken;
		m_reco->ContinuousRecognitionStatusChanged		-= m_CrscToken;
		m_reco->HypothesisAvailable						-= m_HaToken;
	}
}

Bool KinectSpeechRecognizerClass::Initialize( Platform::IntPtr parent )
{
	m_parent = (CKinectSpeechRecognizer*)((void*)parent);
	
	CreateRecognizer();
	if ( m_reco == nullptr )
		return false;

	AttachRecognizerEventHandlers();
	return true;
}

void KinectSpeechRecognizerClass::CreateRecognizer()
{
	try
	{
		m_reco = ref new SpeechRecognizer();
		if ( !m_reco )
			return;

		m_defaultSpeechRecognizer = m_reco->GetRecognizer();
		Platform::String^ localName = GetSupportedLocal( m_defaultSpeechRecognizer );
		if ( !m_languageMapper->HasKey( localName ) )
		{
			m_reco = nullptr;
			return;
		}

		Platform::String^ defaultLanguage = m_languageMapper->Lookup( localName );
		if ( !SetRecognizer( defaultLanguage ) )
		{
			m_reco = nullptr;
			return;
		}
	}
	catch ( Platform::Exception^ ex )
	{
		if( ex->HResult == 0x800455BC )
		{
			RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "The requested language is not supported." ) );
		}		
		m_reco = nullptr;
	}
}

void KinectSpeechRecognizerClass::AttachRecognizerEventHandlers()
{
	m_speechClosedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	HRESULT _hr_ = ( ( m_speechClosedEvent == NULL ) ? HRESULT_FROM_WIN32( GetLastError() ) : S_OK ); 
	if( FAILED( _hr_ ) )
	{
		RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Kinect speech recognizer can not create Event object, speech recognizer will not start !!!" ) );
		return;
	}

	// We create an event handler to receive information about any recognition 
	// problems:
	//
	//  ref new TypedEventHandler<SpeechRecognizer^, SpeechAudioProblemOccurredEventArgs^>();
	//
	// We want to direct this to our AudioProblemOccurred() member function, but 
	// can't do this directly as Sample isn't derived from a WinRT type. Instead
	// we supply a lambda (which can be bound as an event handler) and use a
	// lambda capture to allow access to the "this" pointer of our Sample instance.
	m_ApoToken = m_reco->AudioProblemOccurred += ref new TypedEventHandler<SpeechRecognizer^, SpeechAudioProblemOccurredEventArgs^>( 
		[this]( SpeechRecognizer^ reco, SpeechAudioProblemOccurredEventArgs^ args )
	{
		this->AudioProblemOccurred( reco, args );
	}
	);

	// Event handler that will be notified when something has been recognized
	m_CrraToken = m_reco->ContinuousRecognitionResultAvailable += ref new TypedEventHandler<SpeechRecognizer^, ContinuousSpeechRecognitionResultEventArgs^>( 
		[this]( SpeechRecognizer^ reco, ContinuousSpeechRecognitionResultEventArgs^ args )
	{
		this->ContinuousRecognitionResultAvailable( reco, args );
	}
	);

	// This event handler picks up status changes in the recognizer, including errors, being started / stopped and so on
	m_CrscToken = m_reco->ContinuousRecognitionStatusChanged += ref new TypedEventHandler<SpeechRecognizer^, ContinuousSpeechRecognitionStatusEventArgs^>( 
		[this]( SpeechRecognizer^ reco, ContinuousSpeechRecognitionStatusEventArgs^ args )
	{
		this->ContinuousRecognitionStatusChanged( reco, args );
	}
	);

	// This event handler fires when a new hypothesis is available
	m_HaToken = m_reco->HypothesisAvailable += ref new TypedEventHandler<SpeechRecognizer^, SpeechHypothesisResultEventArgs^>( 
		[this]( SpeechRecognizer^ reco, SpeechHypothesisResultEventArgs^ args )
	{
		this->HypothesisAvailable( reco, args );
	}
	);

	// We can tell the recognizer to pause when a new result is available, but
	// we're not worried about that here.
	m_reco->PauseOnContinuousRecognitionResultAvailable = false;
}

Bool KinectSpeechRecognizerClass::IsRecognizerInitialized()
{
	return ( m_reco != nullptr );
}

//--------------------------------------------------------------------------------------
// Name: AudioProblemOccurred()
// Desc: Event handler for recognition problems.
//--------------------------------------------------------------------------------------

void KinectSpeechRecognizerClass::AudioProblemOccurred( SpeechRecognizer^ /*reco*/, SpeechAudioProblemOccurredEventArgs^ args )
{
	unsigned int speechRecognizerListenerCount = 0;
	if( m_parent )
	{
		switch ( args->Problem )
		{
		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::None:
			//  Note: SpeechRecognitionAudioProblem::None may be raised frequently, e.g. when
			//  recognition completes successfully. Just ignore it.
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::NoSignal:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_NoSignal );						
			}
			m_speechRecognizerMutex.Release();
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::TooFast:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_TooFast );						
			}
			m_speechRecognizerMutex.Release();
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::TooLoud:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_TooLoud );						
			}
			m_speechRecognizerMutex.Release();
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::TooNoisy:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_TooNoisy );						
			}
			m_speechRecognizerMutex.Release();
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::TooQuiet:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_TooQuiet );						
			}
			m_speechRecognizerMutex.Release();
			break;

		case Windows::Xbox::Speech::Recognition::SpeechRecognitionAudioProblem::TooSlow:
			m_speechRecognizerMutex.Acquire();
			speechRecognizerListenerCount = m_speechRecognizerListener->Size;
			for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
			{
				((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->AudioProblem( KAP_TooSlow );						
			}
			m_speechRecognizerMutex.Release();
			break;

		default:
			RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Audio Problem: Unknown Problem" ) );
			break;
	
		}
	}
}


//--------------------------------------------------------------------------------------
// Name: ContinuousRecognitionResultAvailable()
// Desc: Called when a result is available from continuous recognition
//--------------------------------------------------------------------------------------

void KinectSpeechRecognizerClass::ContinuousRecognitionResultAvailable( SpeechRecognizer^ reco, ContinuousSpeechRecognitionResultEventArgs^ args )
{
	if ( args->Result != nullptr && args->Result->Details != nullptr )
	{
		auto result = args->Result;

		if ( result != nullptr )
		{
			Windows::Xbox::Speech::Recognition::SpeechRecognitionConfidence     recoTextConfidence = result->TextConfidence;

			if (   recoTextConfidence != Windows::Xbox::Speech::Recognition::SpeechRecognitionConfidence::Rejected
				&& recoTextConfidence != Windows::Xbox::Speech::Recognition::SpeechRecognitionConfidence::Low )
			{
				// Assign results->Text to a local Platform::String^ in order to ensure
				// its lifetime while we access its internal buffer.
				SKinectSpeechRecognizedCommand recognizedCommand;
				recognizedCommand.m_orginalText = result->Text->Data();
				// Use this value to threshold whether to accept or reject a
				// recognition result.
				recognizedCommand.m_confidenceScore = result->Details->ConfidenceScore;
				if( m_ruleMapper->HasKey( result->RuleName ) )
				{
					recognizedCommand.m_id = m_ruleMapper->Lookup(result->RuleName);

					//  Let's look at semantics, if they have been provided
					if ( args->Result->Semantics != nullptr && args->Result->Semantics->Size > 0 )
					{
						auto semantics = args->Result->Semantics;

						//  Multiple semantics can be returned during a single recognition event
						for ( auto i = semantics->First(); i->HasCurrent; i->MoveNext() )
						{
							String semanticNameString( i->Current->Value->Name->Data() );

							//  Retrieve the value of the semantic directly as a string using
							//  the ValueAsString property.
							String semanticValueString( i->Current->Value != nullptr ? i->Current->Value->ValueAsString->Data() : L"<none>" );

							//Float semanticConfidence = i->Current->Value->ConfidenceScore;
							recognizedCommand.m_semantic.Set( semanticNameString, semanticValueString );					
						}
					}
					m_speechRecognizerMutex.Acquire();
					unsigned int speechRecognizerListenerCount = m_speechRecognizerListener->Size;
					for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
					{
						((IKinectSpeechRecognizerListener*)( (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value ))->RecognizedCommand( recognizedCommand );						
					}
					m_speechRecognizerMutex.Release();
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Can not find rule name (%s) in rule mapper !!!" ), result->RuleName->Begin() );
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------
// Name: ContinuousRecognitionStatusChanged()
// Desc: Called when the status of continuous recognition has changed
//--------------------------------------------------------------------------------------

void KinectSpeechRecognizerClass::ContinuousRecognitionStatusChanged( SpeechRecognizer^ reco, ContinuousSpeechRecognitionStatusEventArgs^ args )
{
	String statusText = TXT("Unknown");

	switch( args->Status )
	{
	case Windows::Xbox::Speech::Recognition::ContinuousSpeechRecognitionStatus::Completed:
		statusText = TXT("Completed");
		m_isSpeechRunning = false;
		break;

	case Windows::Xbox::Speech::Recognition::ContinuousSpeechRecognitionStatus::Error:
		{
			Char errorBuffer[ 256 ];
			swprintf_s( errorBuffer, L"Error (0x%08X)", (UINT32)( args->ErrorCode.Value ) );
			statusText = errorBuffer;
		}
		m_isSpeechRunning = false;
		break;

	case Windows::Xbox::Speech::Recognition::ContinuousSpeechRecognitionStatus::InProgress:
		statusText = TXT("In Progress");
		m_isSpeechRunning = true;
		break;

	case Windows::Xbox::Speech::Recognition::ContinuousSpeechRecognitionStatus::Paused:
		statusText = TXT("Paused");
		m_isSpeechRunning = true;
		break;

	case Windows::Xbox::Speech::Recognition::ContinuousSpeechRecognitionStatus::Stopped:
		statusText = TXT("Stopped");
		m_isSpeechRunning = false;
		break;

	default:
		m_isSpeechRunning = false;
		break;
	}

	RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Speech Recognition Status: %s" ), statusText.AsChar() );

	//  Check whether main loop has requested shutdown
	if ( m_isSpeechClosing && !m_isSpeechRunning )
	{
		BOOL setShutdown = SetEvent( m_speechClosedEvent );
		if( setShutdown == false )
		{
			HRESULT _hr_ = HRESULT_FROM_WIN32( GetLastError() );
			if( FAILED( _hr_ ) )
			{
				RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Kinect speech recodnizer SetEvent faild!!!" ) );
			}
		}
	}
}


//--------------------------------------------------------------------------------------
// Name: HypothesisAvailable()
// Desc: Called when a new hypothesis is available from continuous recognition
//--------------------------------------------------------------------------------------

void KinectSpeechRecognizerClass::HypothesisAvailable( SpeechRecognizer^ reco, SpeechHypothesisResultEventArgs^ args )
{
	auto hypothesisText = args->Hypothesis->Text;
	m_speechRecognizerMutex.Acquire();
	unsigned int speechRecognizerListenerCount = m_speechRecognizerListener->Size;
	for( unsigned int listenerIndex = 0; listenerIndex < speechRecognizerListenerCount; ++listenerIndex )
	{
		void* voidPtr = (void*)m_speechRecognizerListener->GetAt(listenerIndex)->Value;
		IKinectSpeechRecognizerListener* kinectSpeechRecognizerListener = (IKinectSpeechRecognizerListener*)voidPtr;
		kinectSpeechRecognizerListener->HypothesisAvailable( hypothesisText->Data() );						
	}
	m_speechRecognizerMutex.Release();
}

void KinectSpeechRecognizerClass::RegisterListener( Platform::IntPtr listener )
{
	m_speechRecognizerMutex.Acquire();
	m_speechRecognizerListener->Append( ref new Platform::Box<Platform::IntPtr>(listener));
	m_speechRecognizerMutex.Release();
}
void KinectSpeechRecognizerClass::UnregisterListener( Platform::IntPtr listener )
{
	m_speechRecognizerMutex.Acquire();
	unsigned int index = 0;
	if( m_speechRecognizerListener->IndexOf( ref new Platform::Box<Platform::IntPtr>(listener), &index ) )
	{
		m_speechRecognizerListener->RemoveAt( index );
	}
	m_speechRecognizerMutex.Release();
}
void KinectSpeechRecognizerClass::UnregisterAllListeners()
{
	m_speechRecognizerMutex.Acquire();
	m_speechRecognizerListener->Clear();
	m_speechRecognizerMutex.Release();
}

void KinectSpeechRecognizerClass::Start()
{
	if( m_reco )
	{
		m_reco->StartContinuousRecognition();
	}
}

void KinectSpeechRecognizerClass::Stop()
{
	if( m_reco )
	{
		m_reco->StopContinuousRecognition();
	}
}

void KinectSpeechRecognizerClass::RemoveAllGrammars()
{
	if( m_reco )
	{
		m_reco->Grammars->Clear();
	}
}

bool KinectSpeechRecognizerClass::LoadGrammarFromFile( Platform::String^ name, Platform::String^ path, Bool absolutePath ) 
{
	if( m_reco )
	{
		ReaderRandomAccessStreamClass^ streamGrammar = ref new ReaderRandomAccessStreamClass( path, absolutePath );
		if( streamGrammar->IsValid() == true )
		{
			m_reco->Grammars->AddGrammarFromStream( name, streamGrammar );
			return true;
		}
	}
	return false;
}

bool KinectSpeechRecognizerClass::EnableGrammar( Platform::String^ name, bool enable )
{
	if( m_reco && m_reco->Grammars->HasKey( name ) )
	{
		m_reco->Grammars->Lookup(name)->Enabled = enable;
		return true;
	}
	return false;
}

bool KinectSpeechRecognizerClass::IsLanguageSupported( Platform::String^ redKitLanguageCode )
{
	unsigned int index;
	return m_supportedLanguages->IndexOf( redKitLanguageCode, &index );
}

bool KinectSpeechRecognizerClass::SetRecognizer( Platform::String^ redKitLanguageCode )
{
	if( m_reco && redKitLanguageCode && IsLanguageSupported( redKitLanguageCode ) )
	{
		Platform::String^ supportedLocal = GetSupportedLocal( m_defaultSpeechRecognizer );

		if( supportedLocal != nullptr )
		{
			if( Platform::String::CompareOrdinal( m_defaultSpeechRecognizer->Language, supportedLocal ) == 0 ) //! if local is different it mean that defaultrecognizer do not support our grammars
			{
				Platform::String^ defaultLanguage = m_languageMapper->Lookup( supportedLocal );
				if( Platform::String::CompareOrdinal( defaultLanguage, redKitLanguageCode ) == 0 )
				{
					m_reco->SetRecognizer( m_defaultSpeechRecognizer );
					m_currentLanguage = defaultLanguage;
					RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Speech Recognizer : %s"), m_defaultSpeechRecognizer->Language->Begin() );
					return true;
				}
			}			
		}

		unsigned int recognizersCount = InstalledSpeechRecognizers::All->Size;
		for ( unsigned int index = 0; index < recognizersCount; index++ )
		{
			SpeechRecognizerInformation^ speechRecognizerInformation = InstalledSpeechRecognizers::All->GetAt( index );
			
			supportedLocal = GetSupportedLocal( speechRecognizerInformation );

			if( supportedLocal != nullptr )
			{
				if( Platform::String::CompareOrdinal( speechRecognizerInformation->Language, supportedLocal ) == 0 ) //! if local is different it mean that speechRecognizerInformation do not support our grammars
				{
					Platform::String^ language = m_languageMapper->Lookup( supportedLocal );
					if( Platform::String::CompareOrdinal( language, redKitLanguageCode ) == 0 )
					{
						m_reco->SetRecognizer( speechRecognizerInformation );
						m_currentLanguage = language;
						RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Speech Recognizer : %s"), speechRecognizerInformation->Language->Begin() );
						return true;
					}	
				}
		
			}
		}

		m_reco->SetRecognizer( m_defaultSpeechRecognizer );
		RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Speech Recognizer : %s"), m_defaultSpeechRecognizer->Language->Begin() );
	}
	return false;
}

Platform::String^ KinectSpeechRecognizerClass::GetSupportedLocal( SpeechRecognizerInformation^ speechRecognizerInformation )
{
	Platform::String^ supportedLocal = speechRecognizerInformation->Language;

	if( m_localMapper->HasKey( speechRecognizerInformation->Language )  )
	{
		supportedLocal = m_localMapper->Lookup( speechRecognizerInformation->Language );
	}

	if( m_languageMapper->HasKey( supportedLocal ) )
	{
		return supportedLocal;
	}
	 return nullptr;
}

CKinectSpeechRecognizer::CKinectSpeechRecognizer() : CPlatformFeature( PF_Kinect )
{	
	Platform::IntPtr parentPtr = Platform::IntPtr((void*)this);
	KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = ref new KinectSpeechRecognizerClass();
	if ( kinectSpeechRecognizerClass->Initialize( parentPtr ) )
	{
		m_currentLanguage = kinectSpeechRecognizerClass->GetCurrentLanguage()->Data();
	}
	
	m_kinectSpeechRecognizer = reinterpret_cast<void*>( kinectSpeechRecognizerClass );
	if( m_kinectSpeechRecognizer != NULL )
	{
		IUnknown* iUnknown = static_cast<IUnknown*>( m_kinectSpeechRecognizer );
		iUnknown->AddRef();
	}
}

CKinectSpeechRecognizer::~CKinectSpeechRecognizer()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		IUnknown* iUnknown = static_cast<IUnknown*>( m_kinectSpeechRecognizer );
		iUnknown->Release();
	}
}

Bool CKinectSpeechRecognizer::IsRecognizerInitialized()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		return kinectSpeechRecognizerClass->IsRecognizerInitialized();
	}
	return false;
}

void CKinectSpeechRecognizer::Start()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->Start();
	}
}

void CKinectSpeechRecognizer::Stop()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->Stop();
	}
}

void CKinectSpeechRecognizer::RegisterCommandRuleWithId( const String& ruleName, Uint32 id )
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->RegisterCommandRuleWithId( ref new Platform::String( ruleName.AsChar() ), id );
	}
}

Bool CKinectSpeechRecognizer::LoadGrammarFromFile( const String& name, const String& path )
{
	if( m_kinectSpeechRecognizer != NULL && m_loadedGrammars.KeyExist( name ) == false )
	{
		String absoluteFilePath = GFileManager->GetBaseDirectory() + TXT("xdkconfig\\kinect\\");
		absoluteFilePath += m_currentLanguage;
		absoluteFilePath += TXT("\\");
		absoluteFilePath += path;

		absoluteFilePath = absoluteFilePath.ToLower();
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		if(    kinectSpeechRecognizerClass->IsLanguageSupported( ref new Platform::String( m_currentLanguage.AsChar() ) )
			&& kinectSpeechRecognizerClass->LoadGrammarFromFile( ref new Platform::String( name.AsChar() ), ref new Platform::String( absoluteFilePath.AsChar() ), true ) )
		{
			m_loadedGrammars[name] = path;
			return true;
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "Can not load grammar: %s"), absoluteFilePath.AsChar() );
		}
		
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizer ), TXT( "KinectSpeechRecognizer not initialized or %s is loaded!!!"), name.AsChar() );
	}
	return false;
}

Bool CKinectSpeechRecognizer::EnableGrammar( const String& name, const Bool enable )
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		return kinectSpeechRecognizerClass->EnableGrammar( ref new Platform::String( name.AsChar() ), enable );
	}
	return false;
}

void CKinectSpeechRecognizer::RemoveAllGrammars()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );

		kinectSpeechRecognizerClass->RemoveAllGrammars();

		m_loadedGrammars.Clear();
	}
}

void CKinectSpeechRecognizer::RegisterListener( IKinectSpeechRecognizerListener* listener )
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->RegisterListener( Platform::IntPtr((void*)listener) );
	}
}

void CKinectSpeechRecognizer::UnregisterListener( IKinectSpeechRecognizerListener* listener )
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->UnregisterListener( Platform::IntPtr((void*)listener) );
	}
}

void CKinectSpeechRecognizer::UnregisterAllListeners()
{
	if( m_kinectSpeechRecognizer != NULL )
	{
		KinectSpeechRecognizerClass^ kinectSpeechRecognizerClass = reinterpret_cast<KinectSpeechRecognizerClass^>( m_kinectSpeechRecognizer );
		kinectSpeechRecognizerClass->UnregisterAllListeners();
	}
}

#endif