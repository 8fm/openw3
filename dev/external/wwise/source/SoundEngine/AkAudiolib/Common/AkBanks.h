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

//////////////////////////////////////////////////////////////////////
//
// AkBanks.h
//
//////////////////////////////////////////////////////////////////////
#ifndef AK_BANKS_H_
#define AK_BANKS_H_

#include "AkCommon.h"

#define AK_BANK_INVALID_OFFSET (AK_UINT_MAX)

// Version History:
//   1:   Original
//   2:   Added Layers to Layer Containers
//   3:   Remove unused isTransitionEnabled flag
//   4:   Removed Divergence Front/Rear
//   5:   Added Crossfading data to Layers
//   6:   - Moved interpolation type from curves to curve points (for segment-specific interpolation)
//        - Added scaling mode to RTPC curves (for optional dB scaling of volume-related curves curves)
//        - Unifying enum used to specify interpolation in curves and in fade actions
//   7:   Added curve scaling type to radius and obs/occ curves
//   8:   Support more interpolation types in curves. Note that this does not change the format
//        of the bank, only the possible values for interpolation.
//   9:   Support multiple RTPC curves on the same parameter (added curve ID to each RTPC)
//  10:   Add RTPC for environmentals (WG-4485)
//  11:	  Removed unused DeviceID for sources
//  12:   Move FX at beginning of sound (WG-4513)
//  13:   Removed UseState flag from SoundBanks (WG-4597)
//  14:   Cleanup (WG-4716)
//          - Removed bIsDopplerEnabled from AKBKPositioningInfo
//          - Removed bSendOverrideParent, Send, SendModMin and SendModMax from AKBKParameterNodeParams
//  15:   Added State synchronization type.
//  16:   Added Triggers Strings in banks and trigger actions.
//  17:   Added Interactive music bank packaging.
//		  ( all banks gets invalidated since the source content format changed ).
//  18:   Added Random/Sequence support at track level for IM
//  19:   Removed Override Parent in AkStinger
//  20:   Added new positioning information
//	21:	  Added new advanced settings information for kick newest/oldest option
//  22:   Removed old positioning info
//  23:   ---
//  24:	  Fixed Trigger action packaging
//  25:   Added Interactive music track look ahead time.
//  26:   Added Wii compressor for all busses
//  27:   MAJOR BANK REFACTORING
//	28:	  MAJOR BANK REFACTORING( Now hashing strings )	
//	29:	  Changed sources' format bits packaging in accordance to new AkAudioFormat.
//  30:	  Added Feedback Generator
//  31:	  Added Feedback flag in bank header.
//  32:   If the feedback bus isn't set, the feedback properties aren't saved anymore
//  33:   Some plug-ins have additional properties that must be packaged into banks
//	34:   Music tracks: move playlist before base node params (bug fix WG-10631)
//	35:   Changing bank version for 2008.3. (The version should have been incremented in 2008.2.1 for Wii vorbis but didn't, so better late than never).
//	36:   WG-12262 More vorbis changes
//	37:   Reverting Index and Data order in banks allowing to load on a per sound mode.
//	38:   Modified RPTC subscription format
//	39:   Introduced RTPC on Rnd/Seq Duration
//  40:	  Added Game Parameters default values.
//	41:   Changed AkPitchValue from Int32 to Real32.
//	42:   Added Bus Mas duck attenuation.
//  43:	  Added Follow Listener Orientation flag in positionning (WG-9800 3D User-Defined Positioning | Add a "Follow Listener" option.)
//  44:	  Added random range around points in 2D paths. (WG-13292 3D User-Defined Positioning | Add a randomizer on X and Y coordinates for each points)
//	45:   New XMA2 encoder (introduces with new header format structures).
//	46:   Added dialog event "Weighting" property
//  47:   Banks don't contain the source file format anymore (WG-16161)
//  48:   Seek actions (WG-10870)
//  49:   Channel Mask on Bus
//  50:   FX Sharesets
//  51:   Changes in VorbisInfo.h
//  52:   Removed ScalingDB255 interpolation table, changed Layers curve mapping.
//  53:   Removed global states.
//  54:   Added New "Adopt virtual behavior when over limit".
//  55:   SetGameParameter actions (WG-2700)
//  56:   Added Global Voice limitation system.
//  57:   Modified Weigth to go from 0.001 to 100 (was 1-100)
//  58:   Use AkPropID map in hierarchy chunk for props and ranges
//  59:   Use AkPropID map in states
//  60:   Use AkPropID map for basegenparams
//  61:   Use AkPropID map for action props and ranges
//	62:   Removed default value for RTPC on Layers.
//	63:   Removed LFE Separate Control volume.
//	64:   Interactive music markers/cues
//	65:   Interactive music clip automation
//	66:   Busses now with Hashable names.
//	67:   Platform NGP renamed to VitaSW, VitaHW added
//	68:   Auxilliary sends support.
//	69:   Layer crossfades and music clip fades curves are now linear [0-1].
//	70:   Changed table dB scaling computation.
//	71:   Added bus VoiceVolume handling.
//	72:   Migrated to BusVolume system and Adding Duck target prop.
//  73:   Added Initial Delays
//  74:   Added Bus Panning and Bus Channel config.
//  75:   HDR.
//  76:   HDR property fixes.
//  77:   HDR property changes (override analysis, envelope enable).
//	78:	  Changed HDR defaults.
//	79:	  HDR property changes.
//	80:	  Normalization properties change.
//	81:	  Package positioning RTPC.
//	82:	  Remove positioning bits.
//  82:	  Multi switch music switch containers
//  83:   Allow music transition rules to have a list of source/dest nodes.
//  84:   Do not include music transitions in bank if there are none
//  85:   Sort the short id's in the music transitions
//  86:   Split the Wet attenuation curves into a User defined and Game defines sends.
//  87:   Looping Containers Randomizer.
//  88:   Licensing.

#define AK_BANK_READER_VERSION 88

#define AK_DEMOKEY1 0x5c9844b1
#define AK_DEMOKEY2 0x09b96570
#define AK_DEMOKEY3 0xc984963c
#define AK_DEMOKEY4 0x36a024a7 
#define AK_DEMOKEY5 0xdb22f730 

class CAkFileBase;

#pragma pack(push, 1) // pack all structures read from disk

// Could be the ID of the SoundBase object directly, maybe no need to create it.
typedef AkUInt32 AkMediaID; // ID of a specific source

namespace AkBank
{
	static const AkUInt32 BankHeaderChunkID		= AkmmioFOURCC('B', 'K', 'H', 'D');
	static const AkUInt32 BankDataIndexChunkID	= AkmmioFOURCC('D', 'I', 'D', 'X');
	static const AkUInt32 BankDataChunkID		= AkmmioFOURCC('D', 'A', 'T', 'A');
	static const AkUInt32 BankHierarchyChunkID	= AkmmioFOURCC('H', 'I', 'R', 'C');
	static const AkUInt32 BankStrMapChunkID		= AkmmioFOURCC('S', 'T', 'I', 'D');
	static const AkUInt32 BankStateMgrChunkID	= AkmmioFOURCC('S', 'T', 'M', 'G');
	static const AkUInt32 BankEnvSettingChunkID	= AkmmioFOURCC('E', 'N', 'V', 'S');


	struct AkBankHeader
	{
		AkUInt32 dwBankGeneratorVersion;
		AkUInt32 dwSoundBankID;	 // Required for in-memory banks
        AkUInt32 dwLanguageID;   // Wwise Language ID in which the bank was created ( 0 for SFX )
		AkUInt32 bFeedbackInBank;// This bank contain feedback information. (Boolean, but kept at 32 bits for alignment)
		AkUInt32 dwProjectID;	 // ID of the project that generated the bank
	};

	struct AkSubchunkHeader
	{
		AkUInt32 dwTag;			// 4 character TAG
		AkUInt32 dwChunkSize;	// Size of the SubSection in bytes
	};

	// Contents of the BankDataIndexChunkID chunk
	struct MediaHeader
	{
		AkMediaID id;
		AkUInt32 uOffset;
		AkUInt32 uSize;
	};

	struct AkPathHeader
	{
		AkUniqueID	ulPathID;		// Id
		AkInt		iNumVertices;	// How many vertices there are
	};
	//////////////////////////////////////////////////////////////
	// HIRC structures
	//////////////////////////////////////////////////////////////

	enum AKBKHircType
	{
		HIRCType_State			= 1,
		HIRCType_Sound			= 2,
		HIRCType_Action			= 3,
		HIRCType_Event			= 4,
		HIRCType_RanSeqCntr		= 5,
		HIRCType_SwitchCntr		= 6,
		HIRCType_ActorMixer		= 7,
		HIRCType_Bus			= 8,
		HIRCType_LayerCntr		= 9,
		HIRCType_Segment		= 10,
		HIRCType_Track			= 11,
		HIRCType_MusicSwitch	= 12,
		HIRCType_MusicRanSeq	= 13,
		HIRCType_Attenuation	= 14,
		HIRCType_DialogueEvent	= 15,
		HIRCType_FeedbackBus	= 16,
		HIRCType_FeedbackNode	= 17,
		HIRCType_FxShareSet		= 18,
		HIRCType_FxCustom		= 19,
		HIRCType_AuxBus			= 20

		// Note: stored as 8-bit value in AKBKSubHircSection
	};

	enum AKBKStringType
	{
		StringType_None			= 0,
		StringType_Bank         = 1
	};

	struct AKBKSubHircSection
	{
		AkUInt8		eHircType;
		AkUInt32	dwSectionSize;
	};

	enum AKBKSourceType
	{
		SourceType_Data					= 0,
		SourceType_Streaming			= 1,
		SourceType_PrefetchStreaming	= 2,

		SourceType_NotInitialized		= -1
	};

	struct AKBKHashHeader
	{
		AkUInt32 uiType;
		AkUInt32 uiSize;
	};

	struct AKBKPositioningBits
	{
		AKBKPositioningBits()
			:bOverrideParent(false)
			,bIs3DPositioningEnabled(false)
			,bIsPannerEnabled(false)
			,ePanner(Ak2D)
			,ePosSource(AkGameDef)
			,bIsSpatialized(false)
			,bIsDynamic(true)
			,bFollowOrientation(true)
			,bIsLooping(false)
			{};

		//AKBK purpose flag only, the rest is not written or read if the bIs3DPositioningEnabled flag is false
		/////////////////////////////
		AkUInt16	bOverrideParent : 1;			
		AkUInt16	bIs2DPositioningEnabled : 1;		//Both 2D & 3D can be enabled, if attached to RTPC.
		AkUInt16	bIs3DPositioningEnabled : 1;	
		/////////////////////////////

		AkUInt16	bIsPannerEnabled : 1;
		AkUInt16	ePanner : 1;
		AkUInt16	ePosSource : 1;
		AkUInt16	bIsSpatialized : 1;
		AkUInt16	bIsDynamic : 1;
		AkUInt16	bFollowOrientation : 1;
		AkUInt16	bIsLooping : 1;
	};

}//end namespace "AkBank"

#pragma pack(pop)

#endif
