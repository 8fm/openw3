#include "build.h"
#include "storySceneComment.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneComment )
IMPLEMENT_ENGINE_CLASS( CStorySceneQuestChoiceLine )
RED_DEFINE_STATIC_NAME( commentText );

CStorySceneComment::CStorySceneComment(void)
{
	// m_commentText = CreateObject< CLocalizedContent >( this );
}

IStorySceneElementInstanceData* CStorySceneComment::OnStart( CStoryScenePlayer* player ) const
{
	return new StorySceneCommentInstanceData( this, player );
}

void CStorySceneComment::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}

Bool CStorySceneComment::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == CNAME(commentText) )
	{
		CLocalizedContent *locCont = *( CLocalizedContent **) readValue.GetData();
		
		ASSERT( locCont->GetIndex() != 0 && TXT("Localization Content index equals zero!") );
		m_commentText.SetIndex( locCont->GetIndex() );

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Bool CStorySceneComment::MakeCopyUniqueImpl()
{
	m_commentText.MakeUniqueCopy();
	return true;
}

StorySceneCommentInstanceData::StorySceneCommentInstanceData( const CStorySceneComment* comment, CStoryScenePlayer* player )
	: IStorySceneElementInstanceData( comment, player )
{
}

Bool StorySceneCommentInstanceData::OnTick( Float timeDelta )
{
	return true;
}
