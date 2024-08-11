/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "communityValidator.h"


Bool CCommunityValidator::Validate( const CCommunity *community, String &errorMsg /* out */ )
{
	// Initialize
	m_isValid = true;
	m_errorMessages.Clear();

	const TDynArray< CSTableEntry > &communityTable = community->GetCommunityTable();
	const TDynArray< CSStoryPhaseTimetableEntry > &storyPhaseTimetab = community->GetStoryPhaseTimetable();
	
	TSet< CName > definedTimetableNames;

	// Fill in story phases names
	for ( TDynArray< CSStoryPhaseTimetableEntry >::const_iterator sptEntry = storyPhaseTimetab.Begin();
		  sptEntry != storyPhaseTimetab.End();
		  ++sptEntry )
	{
		definedTimetableNames.Insert( sptEntry->m_name );
	}

	// Community table
	
	if ( communityTable.Empty() )
	{
		AddErrorMsg( TXT("Empty community table.") );
	}

	for ( TDynArray< CSTableEntry >::const_iterator csTableEntry = communityTable.Begin();
		  csTableEntry != communityTable.End();
		  ++csTableEntry )
	{
		if ( csTableEntry->m_entities.Empty() )
		{
			AddErrorMsg( TXT("Empty entities.") );
		}

		for ( TDynArray< CSEntitiesEntry >::const_iterator entitiyEntry = csTableEntry->m_entities.Begin();
			  entitiyEntry != csTableEntry->m_entities.End();
			  ++entitiyEntry )
		{
			if ( entitiyEntry->m_entityTemplate == NULL )
			{
				AddErrorMsg( TXT("Empty entity template.") );
			}
		}

		for ( TDynArray< CSStoryPhaseEntry >::const_iterator storyPhases = csTableEntry->m_storyPhases.Begin();
			  storyPhases != csTableEntry->m_storyPhases.End();
			  ++storyPhases )
		{
			if ( definedTimetableNames.Find( storyPhases->m_timetableName ) == definedTimetableNames.End() )
			{
				AddErrorMsg( String::Printf( TXT("Reference to not defined timetable: %s"), storyPhases->m_timetableName.AsString() ) );
			}

			/*if ( csTableEntry->m_isBackground && storyPhases->m_alwaysSpawned )
			{
				AddErrorMsg( String::Printf( TXT("Entity entry: flags 'is background' and 'always spawned' are both active. (timetable: %s)"),
					storyPhases->m_timetableName.AsString() ) );
			}*/
		}
	}

	// Story phase timetable

	if ( storyPhaseTimetab.Empty() )
	{
		AddErrorMsg( TXT("Empty story phase timetable.") );
	}

	for ( TDynArray< CSStoryPhaseTimetableEntry >::const_iterator sptEntry = storyPhaseTimetab.Begin();
		  sptEntry != storyPhaseTimetab.End();
		  ++sptEntry )
	{
		if ( sptEntry->m_name == CName::NONE )
		{
			AddErrorMsg( TXT("Emtpy story phase timetable name.") );
		}
		if ( sptEntry->m_actionCategies.Empty() )
		{
			AddErrorMsg( TXT("Emtpy action categories.") );
		}
		Float lastActionCategoryTime = -1.0f;
		for ( TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >::const_iterator actionCategory = sptEntry->m_actionCategies.Begin();
			  actionCategory != sptEntry->m_actionCategies.End();
			  ++actionCategory )
		{
			if ( actionCategory->m_actions.Empty() )
			{
				AddErrorMsg( TXT("Empty action category.") );
			}

			if ( actionCategory->m_time.GetSeconds() <= lastActionCategoryTime )
			{
				AddErrorMsg( TXT("Action category time is less or equal than the previous action category time.") );
			}
			lastActionCategoryTime = actionCategory->m_time.GetSeconds();

			for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = actionCategory->m_actions.Begin();
				  action != actionCategory->m_actions.End();
				  ++action )
			{
				if ( action->m_layerName.m_layerName == CName::NONE )
				{
					AddErrorMsg( TXT("Empty layer name") );
				}
				if ( action->m_actionCategories.Empty() )
				{
					AddErrorMsg( TXT("Action categories empty.") );
				}
				for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategories = action->m_actionCategories.Begin();
					  actionCategories != action->m_actionCategories.End();
					  ++actionCategories )
				{
					if ( actionCategories->m_name == CName::NONE )
					{
						AddErrorMsg( TXT("Action category name is empty.") );
					}
					if ( actionCategories->m_weight < 0.0f )
					{
						AddErrorMsg( TXT("Action category weight less than zero.") );
					}
				}
			}
		}
	}

	if ( !m_isValid )
	{
		errorMsg = GetErrorMsg();
	}
	return m_isValid;
}

void CCommunityValidator::AddErrorMsg( const String &errorMsg )
{
	m_isValid = false;
	m_errorMessages.PushBack( errorMsg );
}

String CCommunityValidator::GetErrorMsg()
{
	String res;

	for ( TDynArray< String >::const_iterator errMsg = m_errorMessages.Begin();
		  errMsg != m_errorMessages.End();
		  ++errMsg )
	{
		res += *errMsg + TXT("\n");
	}

	return res;
}
