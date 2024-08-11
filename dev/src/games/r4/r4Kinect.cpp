#include "build.h"
#include "r4Kinect.h"
#include "../../common/core/configVar.h"
#include "../../common/engine/inputDeviceManager.h"

namespace Config
{
	TConfigVar<Bool> cvIsKinectEnabled( "Kinect", "Kinect", true, eConsoleVarFlag_Save );
}

IMPLEMENT_ENGINE_CLASS( CR4KinectSpeechRecognizerListenerScriptProxy );

RED_DEFINE_STATIC_NAME( OnAudioProblem );
RED_DEFINE_STATIC_NAME( OnRecognizedCommand );
RED_DEFINE_STATIC_NAME( OnHypothesisAvailable );
RED_DEFINE_STATIC_NAME( OnListenerRegistered );

CR4KinectSpeechRecognizerListenerScriptProxy::CR4KinectSpeechRecognizerListenerScriptProxy(): m_scriptProxyDelegate( nullptr )
{

}

void CR4KinectSpeechRecognizerListenerScriptProxy::SetDelegate( CR4KinectSpeechRecognizerListenerScriptProxyDelegeate* scriptProxyDelegate )
{
	m_scriptProxyDelegate = scriptProxyDelegate;
}

void CR4KinectSpeechRecognizerListenerScriptProxy::funcIsSupported( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool supported = false;

	if( m_scriptProxyDelegate )
		supported = m_scriptProxyDelegate->IsSupported();

	RETURN_BOOL( supported );
}

void CR4KinectSpeechRecognizerListenerScriptProxy::funcIsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool enabled = false;

	if( m_scriptProxyDelegate )
		enabled = m_scriptProxyDelegate->IsEnabled();

	RETURN_BOOL( enabled );
}

void CR4KinectSpeechRecognizerListenerScriptProxy::funcSetEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	if( m_scriptProxyDelegate )
		m_scriptProxyDelegate->SetEnabled( enable );
}


#if defined(RED_KINECT)

#include "../../common/engine/localizationManager.h"
#include "../../common/platformCommon/platform.h"
#include "../../durango/platformDurango/kinectSpeechRecognizer.h"
#include "../../common/engine/renderFrame.h"

#if !defined( RED_CONFIGURATION_FINAL ) 
#define KINECT_DEBUG_PAGE_DISPLAY
#endif
#ifdef KINECT_DEBUG_PAGE_DISPLAY
static String debugPageKinectDisplay_AudioProblemString;
static String debugPageKinectDisplay_RecognizedCommandString;
static String debugPageKinectDisplay_ConfidenceScore;
static String debugPageKinectDisplay_HypothesisAvailableString;
static Red::Threads::CMutex debugPageKinectDisplayMutex;
static Red::System::Timer* debugPageKinectDisplay_timer = NULL;
#define KINECT_INIT_DISPLAY_TIMER if( debugPageKinectDisplay_timer == NULL ) debugPageKinectDisplay_timer = new Red::System::Timer();
#define KINECT_RESET_DISPLAY_TIMER if( debugPageKinectDisplay_timer != NULL ){ delete debugPageKinectDisplay_timer; debugPageKinectDisplay_timer = new Red::System::Timer(); }
#endif

enum EKinectSpeechRecognizerCommands
{
	KC_TopLevel,	//! quick save, quick load
	KC_Magic,		//! use sign
	KC_Inventory,	//! use item/potion form inventory
	KC_Menu			//! open map/inventory/etc...
};

CR4KinectSpeechRecognizerListener::CR4KinectSpeechRecognizerListener( CR4KinectSpeechRecognizerListenerScriptProxy* scriptProxy ):
	m_scriptProxy( NULL ), 
	m_lastAudioProblemSet( false ), 
	m_lastRecognizedCommandSet( false ), 
	m_lastHypothesisSet( false ),
	m_started( false ),
	m_supported( false )
{
	CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
	if( kinect )
	{
		if( kinect->IsRecognizerInitialized() )
		{
			kinect->RegisterListener( this );
			m_supported = true;
			RegisterRegisterCommands( kinect );
			LoadGrammarsFromFiles( kinect );
		}
	}

	if( m_supported == false )
		Config::cvIsKinectEnabled.Set( false );

	m_previouslyEnabled = Config::cvIsKinectEnabled.Get();

	m_scriptProxy = scriptProxy;

	if( m_scriptProxy )
		m_scriptProxy->SetDelegate( this );

	Bool call = CallFunction( m_scriptProxy, CNAME( OnListenerRegistered ) );
	RED_WARNING( call, "Can not call OnListenerRegistered" );

	GEngine->GetInputDeviceManager()->RegisterListener( &CR4KinectSpeechRecognizerListener::OnControllerEvent, this );
}

CR4KinectSpeechRecognizerListener::~CR4KinectSpeechRecognizerListener()
{
	if( m_scriptProxy )
		m_scriptProxy->SetDelegate( NULL );

	CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
	if( kinect )
	{
		kinect->UnregisterListener( this );
		kinect->RemoveAllGrammars();
	}
}

void CR4KinectSpeechRecognizerListener::RegisterRegisterCommands( CKinectSpeechRecognizer* kinect )
{
	kinect->RegisterCommandRuleWithId( TXT("toplevel"), KC_TopLevel );
	kinect->RegisterCommandRuleWithId( TXT("magic_rule"), KC_Magic );
	kinect->RegisterCommandRuleWithId( TXT("inventory_rule"), KC_Inventory );
	kinect->RegisterCommandRuleWithId( TXT("menu_rule"), KC_Menu );
}

void CR4KinectSpeechRecognizerListener::LoadGrammarsFromFiles( CKinectSpeechRecognizer* kinect )
{
	if( m_supported )
	{
		if( kinect->LoadGrammarFromFile( TXT("scripted_grammar_cast"), TXT("reco_cast.cfg") ) )
			kinect->EnableGrammar( TXT("scripted_grammar_cast"), true );

		if( kinect->LoadGrammarFromFile( TXT("scripted_grammar_open"), TXT("reco_open.cfg") ) )
			kinect->EnableGrammar( TXT("scripted_grammar_open"), true );
	}
}

void CR4KinectSpeechRecognizerListener::Start()
{	
	if( Config::cvIsKinectEnabled.Get() )
	{
		CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
		if( kinect )
			kinect->Start();
		
	}
	m_started = true;	
}

void CR4KinectSpeechRecognizerListener::Stop()
{
	if( Config::cvIsKinectEnabled.Get() )
	{
		CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
		if( kinect )
			kinect->Stop();		
	}
	m_started = false;
}

void CR4KinectSpeechRecognizerListener::Update()
{
	m_speechRecognizerMutex.Acquire();
	if( m_scriptProxy )
	{	
		if( m_lastAudioProblemSet)
		{
			m_lastAudioProblem = false;
			Bool call = CallFunction( m_scriptProxy, CNAME( OnAudioProblem ), m_lastAudioProblem );
			RED_WARNING( call, "Can not call OnAudioProblem" );
		}

		if( m_lastHypothesisSet )
		{
			m_lastHypothesisSet  = false;
			Bool call = CallFunction( m_scriptProxy, CNAME( OnHypothesisAvailable ), m_lastHypothesis );
			RED_WARNING( call , "Can not call OnHypothesisAvailable" );
		}

		if( m_lastRecognizedCommandSet )
		{
			m_lastRecognizedCommandSet = false;
			bool call = CallFunction( m_scriptProxy, CNAME( OnRecognizedCommand ), m_lastRecognizedCommandId, m_lastSemanticNames, m_lastSemanticValues, m_lastConfidenceScore );
			RED_WARNING( call, "Can not call OnRecognizedCommand" );
		}
	}
	m_speechRecognizerMutex.Release();
}

void CR4KinectSpeechRecognizerListener::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
#ifdef KINECT_DEBUG_PAGE_DISPLAY
	if( debugPageKinectDisplayMutex.TryAcquire() )
	{	
		KINECT_INIT_DISPLAY_TIMER;

		if( debugPageKinectDisplay_timer->GetSeconds() > 5.0 )
		{
			KINECT_RESET_DISPLAY_TIMER;
			debugPageKinectDisplay_AudioProblemString.Clear();
			debugPageKinectDisplay_RecognizedCommandString.Clear();
			debugPageKinectDisplay_ConfidenceScore.Clear();
			debugPageKinectDisplay_HypothesisAvailableString.Clear();
		}
		frame->AddDebugScreenText( 840, 88, String::Printf( TXT("Kinect Audio Problem:           %s"), debugPageKinectDisplay_AudioProblemString.AsChar() ),        0, false, Color(255,0,0), Color(0,0,0) );	
		frame->AddDebugScreenText( 840, 99, String::Printf( TXT("Kinect Recognized Command:  %s (%s)"), debugPageKinectDisplay_RecognizedCommandString.AsChar(), debugPageKinectDisplay_ConfidenceScore.AsChar() ), 0, false, Color(0,255,0), Color(0,0,0) );	
		frame->AddDebugScreenText( 840, 110, String::Printf( TXT("Kinect Hypothesis Available:   %s"), debugPageKinectDisplay_HypothesisAvailableString.AsChar() ), 0, false, Color(0,255,0), Color(0,0,0) );	
		debugPageKinectDisplayMutex.Release();
	}
#endif
}

void CR4KinectSpeechRecognizerListener::AudioProblem( EKinectSpeechRecognizerAudioProblem audioProblem )
{
#ifdef KINECT_DEBUG_PAGE_DISPLAY
	String audioProblemString;
#endif
	switch ( audioProblem )
	{
	case KAP_NoSignal:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "No Signal" );
#endif
		break;

	case KAP_TooFast:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Too Fast" );
#endif
		break;

	case KAP_TooLoud:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Too Loud" );
#endif
		break;

	case KAP_TooNoisy:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Too Noisy" );
#endif
		break;

	case KAP_TooQuiet:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Too Quiet" );
#endif
		break;

	case KAP_TooSlow:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Too Slow" );
#endif
		break;

	default:
#ifdef KINECT_DEBUG_PAGE_DISPLAY
		audioProblemString = TXT( "Unknown Problem" );
#endif;
		break;
	}
#ifdef KINECT_DEBUG_PAGE_DISPLAY
	debugPageKinectDisplayMutex.Acquire();
	KINECT_RESET_DISPLAY_TIMER;
	debugPageKinectDisplay_AudioProblemString = audioProblemString;
	debugPageKinectDisplayMutex.Release();
#endif
	m_speechRecognizerMutex.Acquire();
	m_lastAudioProblem = audioProblem;
	m_lastAudioProblemSet = true;
	m_speechRecognizerMutex.Release();
}

void CR4KinectSpeechRecognizerListener::RecognizedCommand( SKinectSpeechRecognizedCommand recognizedCommand  )
{
#ifdef KINECT_DEBUG_PAGE_DISPLAY
	debugPageKinectDisplayMutex.Acquire();
	KINECT_RESET_DISPLAY_TIMER;
	debugPageKinectDisplay_RecognizedCommandString = recognizedCommand.m_orginalText;
	debugPageKinectDisplay_ConfidenceScore = ToString( recognizedCommand.m_confidenceScore );
	debugPageKinectDisplayMutex.Release();
#endif
	m_speechRecognizerMutex.Acquire();

	m_lastSemanticNames.Clear();
	recognizedCommand.m_semantic.GetKeys( m_lastSemanticNames ) ;

	m_lastSemanticValues.Clear();
	recognizedCommand.m_semantic.GetValues( m_lastSemanticValues ) ;

	m_lastConfidenceScore = recognizedCommand.m_confidenceScore ;

	m_lastRecognizedCommandId = (Uint32)recognizedCommand.m_id;

	m_lastRecognizedCommandSet = true;

	m_speechRecognizerMutex.Release();

}

void CR4KinectSpeechRecognizerListener::HypothesisAvailable( const String& hypothesis )
{
#ifdef KINECT_DEBUG_PAGE_DISPLAY
	debugPageKinectDisplayMutex.Acquire();
	KINECT_RESET_DISPLAY_TIMER;
	debugPageKinectDisplay_HypothesisAvailableString = hypothesis;
	debugPageKinectDisplayMutex.Release();
#endif
	m_speechRecognizerMutex.Acquire();
	m_lastHypothesis = hypothesis;
	m_lastHypothesisSet = true;	
	m_speechRecognizerMutex.Release();

}

void CR4KinectSpeechRecognizerListener::SetEnabled( Bool val )
{
	if( m_previouslyEnabled != val )
	{
		if( IsSupported() == true )
		{
			Config::cvIsKinectEnabled.Set( val );
			m_previouslyEnabled = Config::cvIsKinectEnabled.Get();
			if( Config::cvIsKinectEnabled.Get() == true && m_started == true )
			{
				CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
				if( kinect )
					kinect->Start();
			}
			else if( Config::cvIsKinectEnabled.Get() == false && m_started == true )
			{
				CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
				if( kinect )
					kinect->Stop();
			}
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( KinectSpeechRecognizerListener ), TXT( "Can not enable speech recognition, Kinect not supported with current locale !!!") );
		}
	}
}

Bool CR4KinectSpeechRecognizerListener::IsEnabled() const
{
	return Config::cvIsKinectEnabled.Get();
}

void CR4KinectSpeechRecognizerListener::RefreshSettings()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Refreshing kinect settings should be done on main thread" );

	m_speechRecognizerMutex.Acquire();
	if( m_supported == true )
	{
		SetEnabled( Config::cvIsKinectEnabled.Get() );
	}
	m_speechRecognizerMutex.Release();
}

void CR4KinectSpeechRecognizerListener::OnControllerEvent( const EControllerEventType& event )
{
	if( Config::cvIsKinectEnabled.Get() == true && m_started == true )
	{
		CKinectSpeechRecognizer* kinect = (CKinectSpeechRecognizer*)CPlatform::GetFeature(PF_Kinect);
		if( kinect )
		{
			switch (event)
			{
			case EControllerEventType::CET_Disconnected:
				kinect->Stop();
				break;
			case EControllerEventType::CET_Reconnected:				
				kinect->Start();
				break;
			default:
				break;
			}
		}
	}
}

#endif //NO_KINECT

