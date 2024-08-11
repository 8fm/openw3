/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#define DYNAMIC_TAGS_CSV TXT("gameplay\\globals\\dynamic_tags.csv")

class CDynamicTagsContainer
{
public:
	CDynamicTagsContainer();

	// Load all provided 'dynamic' tags mentioned in dynamic_tags.csv
	void LoadDynamicTags();

	//! Add/remove preferred dynamic tags (i.e., when mounting horse to make it unique with this tag)
	void UpdateDynamicTags( CActor* const target, const CName& preferredTag ) const;

private:
	TagList m_dynamicTags;
};