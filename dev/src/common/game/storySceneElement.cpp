#include "build.h"
#include "storySceneElement.h"
#include "storySceneSection.h"
#include "storyScene.h"
#include "storySceneChoice.h"
#include "storySceneDebugger.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/localizationManager.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneElement )

CStorySceneElement::CStorySceneElement()
	: m_approvedDuration( -1.0f )
	, m_isCopy( false )
{
}

IStorySceneElementInstanceData::IStorySceneElementInstanceData( const CStorySceneElement* element, CStoryScenePlayer* player )
	: m_player( player )
	, m_element( element )
	, m_lineSkipped( false )
	, m_currentTime( 0.0f )
	, m_running( false )
	, m_duration( 0.f )
{
	if ( player->GetSceneDebugger() )
	{
		player->GetSceneDebugger()->OnCreatedSceneElementInstanceData( element, this );
	}
}

IStorySceneElementInstanceData::~IStorySceneElementInstanceData()
{	
}

/*

\param locale Locale used for initialization.
\param startTime Start time of element.
\return True if element is ready to play. False - otherwise.

Locale arg is needed because element duration may be different for each locale.
*/
Bool IStorySceneElementInstanceData::Init( const String& locale, Float startTime )
{
	ASSERT( !m_running );
	ASSERT( m_currentTime == 0.f );

	m_duration = m_element->CalculateDuration( locale );
	m_startTime = startTime;

	return OnInit( locale );
}

Bool IStorySceneElementInstanceData::ShouldConfirmSkip() const
{
	return false;
}

Bool IStorySceneElementInstanceData::CanBeSkipped() const
{
	return ( m_element != NULL ) ? m_element->CanBeSkipped() : true;
}

Bool CStorySceneElement::CanBeSkipped() const
{
	// DIALOG_TOMSIN_TODO
	return GetSection()->CanElementBeSkipped();
}

void CStorySceneElement::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Regenerate element ID
	if ( m_elementID.GetLength() == 0 )
	{
		GenerateElementID();
	}
}

String CStorySceneElement::GetFriendlyName() const
{
	if( GetSection() == NULL )
	{
		return TBaseClass::GetFriendlyName();
	}

	String name = GetElementID();
	name += TXT(" in ");
	name += GetSection()->GetFriendlyName();
	return name;
}

void CStorySceneElement::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_test;
}

void CStorySceneElement::OnInitInstance( CStorySceneInstanceBuffer& data ) const
{
	data[ i_test ] = true;
}

void CStorySceneElement::OnReleaseInstance( CStorySceneInstanceBuffer& data ) const
{
	// DIALOG_TOMSIN_TODO - naprawic zeby jak zniana ilosci elementow czy eventow to mozna dalej zrobic release na instancji
	//data[ i_test ] = false;
}

CStorySceneSection* CStorySceneElement::GetSection() const
{
	return FindParent< CStorySceneSection >();
}

/*
Calculates element duration (in seconds) in specified locale.

\param locale Locale for which to calculate element duration.
\return Element duration in seconds.

Locale arg is needed because element duration may be different for each locale.
*/
Float CStorySceneElement::CalculateDuration( const String& /* locale */ ) const
{
	// Instant element. Don't return 0 to proper displaying in timeline
	return 0.2f;
}

/*
Calculates element duration (in seconds) in current locale.

\return Element duration in seconds.
*/
Float CStorySceneElement::CalculateDuration() const
{
	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();
	return CalculateDuration( currentLocale );
}

IStorySceneElementInstanceData* CStorySceneElement::OnStart( CStoryScenePlayer* player ) const
{
	// No instance of this element is created
	return NULL;
}

void CStorySceneElement::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
}

void CStorySceneElement::GenerateElementID()
{
	CStoryScene* scene = FindParent< CStoryScene >();
	if ( scene )
	{
		// Generate unique scene ID
		String elementID = scene->GenerateUniqueElementID();

		// Append with type of the item
		String elementClassName = GetClass()->GetName().AsString();
		m_elementID = elementClassName.StringAfter( TXT("CStoryScene") ) + elementID;
	}
}

Bool CStorySceneElement::MakeCopyUnique()
{
	Bool isSuccess = MakeCopyUniqueImpl();
	if ( isSuccess )
	{
		m_isCopy = false;
	}
	return isSuccess;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
