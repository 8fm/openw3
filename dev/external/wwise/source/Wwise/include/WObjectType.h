/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/
 
#pragma once

// Not defined in IWObject to avoid bringing unwanted dependencies in SFLib.
enum WObjectType				// Type	of object.
{
    // Leave type WObjectTypeUnknown the first in the Enum declaration.
	WObjectTypeUnknown = 0,		// Unknown type	of object.
	WProjectType,				// Project object. BE SURE TO UPDATE firstWObjectType IF THIS IS NOT THE FIRST TYPE ANYMORE
	WWorkUnitType,				// Work Unit
	WVirtualFolderType,			// Virtual folder object.
    WActorMixerType,            // Actor-Mixer Object
    WRndSeqContainerType,       // Random Sequence Container object
    WSwitchContainerType,       // Switch Container object
	WLayerContainerType,		// Layer Container object
	WAudioSourceType,			// Audio source	object.
	WSoundType,					// Sound object.
    WEventType,                 // Event object
    WEventActionType,           // Event's Action Object
	WSwitchGroupType,			// Switch Group
	WSwitchType,				// Switch
	WStateGroupType,            // State Group Object
	WStateType,					// State Object
	WSourcePluginType,          // Source Plugin
	WEffectPluginType,          // Effect Plugin
    WSoundBankType,             // SoundBank
    WBusType,                   // Control Bus
	WAuxBusType,				// Auxiliary Bus
	WParamControlType,			// RTPC
	WGameParameterType,			// RTPC Game Parameter
	WObjectSettingAssocType,    // Object Setting Assoc ( for Switch Container )
	WSoundcasterSessionType,	// Soundcaster Session
	WModifierType,              // Modifier
	W2DPathType,				// Path (2D position in time)
	WPositionType,				// Position data
	WDataCurveType,				// Data curve
	WMusicSwitchContainerType,	// A music switch container with transitions
	WMusicRndSeqContainerType,	// A multi-level sequence-random container
	WMusicSegmentType,			// A Interactive Music Segment (Multi-Track)
	WMusicTrackType,			// A track inside a segment
	WMusicPlaylistItemType,		// A playlist item for the Music Random Sequence Container
	WLayerType,					// Layer object (inside a WLayerContainer)
	WLayerAssocType,			// Layer association (inside a WLayer)
	WQueryType,					// Query object
	WSearchCriteriaType,		// Search criteria object
	WMusicTransitionType,		// Music Transition
	WMusicStingerType,			// Music Stinger
	WMusicFadeType,				// Music Fade (in or out)
	WTriggerType,				// Trigger game sync
	WAttenuationType,			// Attenuation data
	WPannerType,				// 2D Panner
	WConversionPluginType,		// Conversion Plugin
	WDialogueEventType,			// Dialogue Event
	WFeedbackBusType,			// Feedback device bus
	WFeedbackNodeType,			// Feedback node
	WFeedbackSourceType,		// Feedback source
	WUserProjectSettingsType,	// User Project Settings
	WPluginInnerObjectType,		// Plugin Inner Object
	WMixingSessionType,			// Mixing Session Object
    WPluginMediaSourceType,     // Plugin Media source Object
	WConversionType,			// Conversion Settings
	WExternalSourceFileType,	// External Source File (for conversion)
	WExternalSourceType,		// External Source (as a sound source)
	WMusicTrackSequenceType,	// Music Track Sequence
	WMusicCueType,				// Music Cue
	WMusicClipType,				// Music Clip

    // Add new types here ...
    numOfWObjectTypes,
	firstWObjectType = WProjectType
};
