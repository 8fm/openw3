#include "build.h"
#include "storySceneAbstractLine.h"

IMPLEMENT_ENGINE_CLASS( CAbstractStorySceneLine );

RED_DEFINE_STATIC_NAME(comment);

CAbstractStorySceneLine::CAbstractStorySceneLine()
	: CStorySceneElement()
{
	// Create the user comment

	m_comment.AssignIndex();
}

void CAbstractStorySceneLine::SetVoiceTag( CName newValue )
{
	if ( m_voicetag != newValue )
	{
		m_voicetag = newValue; 
		OnVoicetagChanged();
	}
}

Bool CAbstractStorySceneLine::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == CNAME(comment) )
	{
		CLocalizedContent *locCont = *( CLocalizedContent **) readValue.GetData();

		ASSERT( locCont->GetIndex() != 0 && TXT("Localization Content index equals zero!") );
		m_comment.SetIndex( locCont->GetIndex() );


		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Bool CAbstractStorySceneLine::MakeCopyUniqueImpl()
{
	m_comment.MakeUniqueCopy();
	
	return true;
}
