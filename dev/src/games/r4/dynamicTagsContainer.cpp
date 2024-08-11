/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dynamicTagsContainer.h"
#include "../../common/core/2darray.h"
#include "../../common/core/gatheredResource.h"

CGatheredResource resDynamicTags( DYNAMIC_TAGS_CSV, RGF_Startup );

CDynamicTagsContainer::CDynamicTagsContainer()
{
}

void CDynamicTagsContainer::LoadDynamicTags()
{
	const C2dArray* const dynamicTagsArray = resDynamicTags.LoadAndGet< C2dArray >();
	ASSERT( dynamicTagsArray );

	m_dynamicTags.Clear();

	const Uint32 rows = static_cast< Uint32 >( dynamicTagsArray->GetNumberOfRows() );
	for ( Uint32 i = 0; i < rows; ++i )
	{
		const String dynamicTag = dynamicTagsArray->GetValue( 0, i );
		if ( !dynamicTag.Empty() )
		{
			m_dynamicTags.AddTag( CName( dynamicTag ) );
		}
	}
}

void CDynamicTagsContainer::UpdateDynamicTags( CActor* const target, const CName& preferredTag ) const
{
	CTagManager* const tagManager = GGame->GetActiveWorld()->GetTagManager();

	TagList allTags = target->GetTags();
	Bool tagsModified = false;

	for ( Uint32 i = 0; i < allTags.Size(); ++i )
	{
		const CName& tag = allTags.GetTag( i );
		if ( tag != preferredTag && m_dynamicTags.HasTag( tag ) )
		{
			allTags.SubtractTag( tag );
			tagsModified = true;
		}
	}

	if ( !allTags.HasTag( preferredTag ) )
	{
		allTags.AddTag( preferredTag );
		tagsModified = true;
	}

	if ( tagsModified )
	{
		target->SetTags( allTags );
	}

	TDynArray< CEntity* > entities;
	tagManager->CollectTaggedEntities( preferredTag, entities );

	for ( CEntity* ent : entities )
	{
		if ( CActor* const actor = Cast< CActor >( ent ) )
		{
			if ( actor != target )
			{
				TagList tags = actor->GetTags();
				tags.SubtractTag( preferredTag );
				actor->SetTags( tags );
			}
		}
	}
}