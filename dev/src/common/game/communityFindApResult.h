/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// namespace CommunitySystem
//{
	enum EFindAPResult
	{
		FAPR_Success,				// action point has been found
		FAPR_NoFreeAP,				// there is at least one good AP, but none is free
		FAPR_NoCategoryAP,			// there is no AP with specified category
		FAPR_LayerNotLoaded,		// chosen layer exists, but it's not loaded
		FAPR_LayerNotFound,			// cannot find layer with specified path (maybe casing or typographic error)
		FAPR_TimetabEmpty,			// cannot find AP because timetable is empty
		FAPR_TimetabEmptyCategory,	// timetable exists, but it has empty action category field
		FAPR_NoCandidates,			// there is no free AP that meets requirements
		FAPR_Default,				// there was no find AP call so far
		FAPR_UnknownError			// we don't know what happened
	};
//}
