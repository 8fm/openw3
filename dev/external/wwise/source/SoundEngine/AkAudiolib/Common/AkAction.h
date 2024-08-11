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
// AkAction.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_H_
#define _ACTION_H_

#include "AkIndexable.h"
#include "AkModifiers.h"
#include "AkParameters.h"
#include "AkPropBundle.h"

//        Define                 Bitfield exclusive
#define ACTION_TYPE_USE_OBJECT			0x0001

//        Define                 Bitfield exclusive
#define ACTION_TYPE_SCOPE_MASK			0x002E

#define ACTION_TYPE_SCOPE_ELEMENT		0x0002
#define ACTION_TYPE_SCOPE_ALL			0x0004
#define ACTION_TYPE_SCOPE_ALL_EXCEPT	0x0008
#define ACTION_TYPE_SCOPE_EVENT			0x0010
#define ACTION_TYPE_SCOPE_BUS			0x0020

//       Define                  Bitfield NON-exclusive

#define ACTION_TYPE_ACTION				0xFF00
#define ACTION_TYPE_STOP				0x0100
#define ACTION_TYPE_PAUSE				0x0200
#define ACTION_TYPE_RESUME				0x0300
#define ACTION_TYPE_PLAY				0x0400
#define ACTION_TYPE_PLAYANDCONTNUE		0x0500

#define ACTION_TYPE_MUTE				0x0600	
#define ACTION_TYPE_UNMUTE				0x0700
#define ACTION_TYPE_SETPITCH			0x0800
#define ACTION_TYPE_RESETPITCH			0x0900
#define ACTION_TYPE_SETVOLUME			0x0A00
#define ACTION_TYPE_RESETVOLUME			0x0B00
#define ACTION_TYPE_SETBUSVOLUME		0x0C00
#define ACTION_TYPE_RESETBUSVOLUME		0x0D00
#define ACTION_TYPE_SETLPF				0x0E00
#define ACTION_TYPE_RESETLPF			0x0F00
#define ACTION_TYPE_USESTATE			0x1000
#define ACTION_TYPE_UNUSESTATE			0x1100
#define ACTION_TYPE_SETSTATE			0x1200
#define ACTION_TYPE_SETGAMEPARAMETER	0x1300
#define ACTION_TYPE_RESETGAMEPARAMETER	0x1400

#define ACTION_TYPE_STOPEVENT			0x1500
#define ACTION_TYPE_PAUSEEVENT			0x1600
#define ACTION_TYPE_RESUMEEVENT			0x1700

#define ACTION_TYPE_DUCK				0x1800
#define ACTION_TYPE_SETSWITCH			0x1900
#define ACTION_TYPE_BYPASSFX			0x1A00
#define ACTION_TYPE_RESETBYPASSFX		0x1B00
#define ACTION_TYPE_BREAK				0x1C00
#define ACTION_TYPE_TRIGGER				0x1D00

#define ACTION_TYPE_SEEK				0x1E00

#define ACTION_TYPE_ILLEGAL_MASK		0xFFFF
///////////////////////////////////////////////////////////////////
// ACTION_TYPE_ILLEGAL_MASK
//Important notice.
// ACTION_TYPE_************
// Nothing can exceed 0xFFFF
// No action type is allowed to define more than 16 bits, to guard
// the usage of AKACTIONTYPE_NUM_STORAGE_BIT.
///////////////////////////////////////////////////////////////////

class CAkRegisteredObj;
class CAkParameterNodeBase;

//List of all types of actions
enum AkActionType
{
//             LEGEND
//
//	_E   = ElementSpecific
//	_ALL = Global scope(all elements)
//	_AE  = All except
//	_O   = Object Specific
//  _M	 = Main object is affected (AudioNode Definition)

	AkActionType_None				= 0x0000,
	AkActionType_SetState			= ACTION_TYPE_SETSTATE	 | ACTION_TYPE_SCOPE_ALL,

	AkActionType_BypassFX_M			= ACTION_TYPE_BYPASSFX	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_BypassFX_O			= ACTION_TYPE_BYPASSFX	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetBypassFX_M	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetBypassFX_O	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetBypassFX_ALL	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetBypassFX_ALL_O= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetBypassFX_AE	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetBypassFX_AE_O	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetSwitch			= ACTION_TYPE_SETSWITCH  | ACTION_TYPE_USE_OBJECT,

	AkActionType_UseState_E			= ACTION_TYPE_USESTATE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_UnuseState_E		= ACTION_TYPE_UNUSESTATE | ACTION_TYPE_SCOPE_ELEMENT,

	AkActionType_Play				= ACTION_TYPE_PLAY		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_PlayAndContinue	= ACTION_TYPE_PLAYANDCONTNUE | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_E				= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Stop_E_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_ALL			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Stop_ALL_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_AE			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Stop_AE_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,	

	AkActionType_Pause_E			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Pause_E_O			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,	
												
	AkActionType_Pause_ALL			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL,		
	AkActionType_Pause_ALL_O		= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,	
												
	AkActionType_Pause_AE			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Pause_AE_O			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Resume_E			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Resume_E_O			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Resume_ALL			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Resume_ALL_O		= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Resume_AE			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Resume_AE_O		= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Break_E			= ACTION_TYPE_BREAK		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Break_E_O			= ACTION_TYPE_BREAK		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Mute_M				= ACTION_TYPE_MUTE		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Mute_O				= ACTION_TYPE_MUTE		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_M			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Unmute_O			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_ALL			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Unmute_ALL_O		= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_AE			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Unmute_AE_O		= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_SetVolume_M		= ACTION_TYPE_SETVOLUME	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetVolume_O		= ACTION_TYPE_SETVOLUME	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetVolume_M		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetVolume_O		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetVolume_ALL	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetVolume_ALL_O	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetVolume_AE		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetVolume_AE_O	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetPitch_M			= ACTION_TYPE_SETPITCH	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetPitch_O			= ACTION_TYPE_SETPITCH	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetPitch_M		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetPitch_O		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,	

	AkActionType_ResetPitch_ALL		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetPitch_ALL_O	= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetPitch_AE		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL_EXCEPT,	
	AkActionType_ResetPitch_AE_O	= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetLPF_M			= ACTION_TYPE_SETLPF	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetLPF_O			= ACTION_TYPE_SETLPF	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLPF_M			= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetLPF_O			= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLPF_ALL		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetLPF_ALL_O		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetLPF_AE		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetLPF_AE_O		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetBusVolume_M		= ACTION_TYPE_SETBUSVOLUME	 | ACTION_TYPE_SCOPE_ELEMENT,

	AkActionType_ResetBusVolume_M	= ACTION_TYPE_RESETBUSVOLUME | ACTION_TYPE_SCOPE_ELEMENT,

	AkActionType_ResetBusVolume_ALL	= ACTION_TYPE_RESETBUSVOLUME | ACTION_TYPE_SCOPE_ALL,

	AkActionType_ResetBusVolume_AE	= ACTION_TYPE_RESETBUSVOLUME | ACTION_TYPE_SCOPE_ALL_EXCEPT,

	AkActionType_StopEvent			= ACTION_TYPE_STOPEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_PauseEvent			= ACTION_TYPE_PAUSEEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_ResumeEvent		= ACTION_TYPE_RESUMEEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Duck				= ACTION_TYPE_DUCK | ACTION_TYPE_SCOPE_BUS,

	AkActionType_Trigger			= ACTION_TYPE_TRIGGER,
	AkActionType_Trigger_O			= ACTION_TYPE_TRIGGER | ACTION_TYPE_USE_OBJECT,

	AkActionType_Seek_E				= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Seek_E_O			= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,	

	AkActionType_Seek_ALL			= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ALL,	
	AkActionType_Seek_ALL_O			= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,	
	
	AkActionType_Seek_AE			= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ALL_EXCEPT,	
	AkActionType_Seek_AE_O			= ACTION_TYPE_SEEK		| ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetGameParameter	= ACTION_TYPE_SETGAMEPARAMETER | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetGameParameter_O	= ACTION_TYPE_SETGAMEPARAMETER | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_ResetGameParameter	= ACTION_TYPE_RESETGAMEPARAMETER | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetGameParameter_O	= ACTION_TYPE_RESETGAMEPARAMETER | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT


//               LEGEND
//
//	_E   = ElementSpecific
//	_ALL = Global scope(all elements)
//	_AE  = All except
//	_O   = Object Specific
//  _M	 = Main object is affected (AudioNode Definition)

#define AKACTIONTYPE_NUM_STORAGE_BIT 16 // Internal storage restriction, for internal use only.
};

struct AkPendingAction;

//Action object
class CAkAction : public CAkIndexable
{
public:

	//Thread safe version of the constructor
	static CAkAction* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Constructor
	CAkAction(
		AkActionType in_eActionType,	//Type of action
		AkUniqueID in_ulID
		);

	//Destructor
	virtual ~CAkAction();

	AkForceInline AKRESULT Init(){ AddToIndex(); return AK_Success; }

	//Get the Action type of the current action
	//
	// Return - AkActionType - Type of action
	AkForceInline AkActionType ActionType() { return (AkActionType)m_eActionType; }

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	//Set the element ID associated to the Action
	virtual void SetElementID(
		WwiseObjectIDext in_Id
		);

	//Get The element ID associated to the action
	AkForceInline AkUniqueID ElementID() { return m_ulElementID; }

	virtual AKRESULT SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax );
	virtual AKRESULT SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax );

	// Get the Curve type of the transition
	AkForceInline AkCurveInterpolation CurveType() { return (AkCurveInterpolation) m_eFadeCurve; }

	// Sets the curve type of the transition
	AkForceInline void CurveType(
		const AkCurveInterpolation in_eCurveType //Curve type
		)
	{
		m_eFadeCurve = in_eCurveType;
	}

	AkTimeMs GetTransitionTime();
	AkInt32 GetDelayTime();

	// Execute the Action
	// Must be called only by the AudioThread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		) = 0;

	void AddToIndex();
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	AKRESULT SetInitialValues(AkUInt8* pData, AkUInt32 ulDataSize);
	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	CAkParameterNodeBase* GetAndRefTarget();

	CAkAction *pNextLightItem;	//For AkListBareLight

protected:

	template<class T_VALUE>
	AkForceInline void ApplyRange( AkPropID in_ePropID, T_VALUE & io_value )
	{
#ifdef AKPROP_TYPECHECK
		AKASSERT( typeid( T_VALUE ) == *g_AkPropTypeInfo[ in_ePropID ] );
#endif
		RANGED_MODIFIERS<T_VALUE> * pRange = (RANGED_MODIFIERS<T_VALUE> *) m_ranges.FindProp( in_ePropID );
		if ( pRange )
			io_value += RandomizerModifier::GetMod( *pRange );
	}

	bool IsBusElement()
	{ 
		// Update this flag from the proxy and banks, otherwise currently init to false always.
		return m_bIsBusElement != 0;
	}

	AkPropBundle<AkPropValue> m_props;
	AkPropBundle< RANGED_MODIFIERS<AkPropValue> > m_ranges;

	AkUniqueID  m_ulElementID;	// Associated element	
    AkUInt16    m_eActionType;	// Type of action
    AkUInt8		m_eFadeCurve	     : AKCURVEINTERPOLATION_NUM_STORAGE_BIT;
    AkUInt8		m_bWasLoadedFromBank : 1; // Only used for play action, but stored here for bit alignment savings.
	AkUInt8		m_bIsBusElement      : 1;
};
#endif
