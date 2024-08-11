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
// AkFxIDs.h
//
// Definition of sw effects IDs
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_FX_ID_H_
#define _AK_FX_ID_H_
//====================================================================================================
// 12 bits for Vendor ID | 4 bits for Effect type | 16 bit for effect ID
//====================================================================================================
const AkPluginID AK_VENDOR_ID_MASK					0xFFF00000;
const AkPluginID AK_EFFECT_TYPE_MASK				0x000F0000;
const AkPluginID AK_EFFECT_ID_MASK					0x0000FFFF;
#define GET_VENDOR_ID(_id_)						((_id_) & AK_VENDOR_ID_MASK)
#define GET_EFFECT_TYPE(_id_)					((_id_) & AK_EFFECT_TYPE_MASK) 
#define GET_EFFECT_ID(_id_)						((_id_) & AK_EFFECT_ID_MASK)

// vendor
const AkPluginID AK_VENDORID_RESERVED				= 0x00000000;
const AkPluginID AK_VENDORID_AUDIOKINETIC			= 0x00100000;
#define IS_AK_EFFECT(_id_)						(((_id_) & AK_VENDORID_AUDIOKINETIC) != 0)
#define ISNOT_AK_EFFECT(_id_)					(((_id_) & AK_VENDORID_AUDIOKINETIC) == 0)

// sync & async
const AkPluginID AK_EFFECT_TYPE_ASYNCHRONOUS		= 0x00000000;
const AkPluginID AK_EFFECT_TYPE_SYNCHRONOUS			= 0x00080000;
#define IS_SYNC_EFFECT(_id_)					(((_id_) & AK_EFFECT_TYPE_SYNCHRONOUS) != 0)
#define IS_ASYNC_EFFECT(_id_)					(((_id_) & AK_EFFECT_TYPE_SYNCHRONOUS) == 0)

// insert
const AkPluginID AK_EFFECT_TYPE_INSERT				= 0x00000000;
#define IS_INSERT_EFFECT(_id_)					(((_id_) & AK_EFFECT_TYPE_INSERT) != 0)
#define ISNOT_INSERT_EFFECT(_id_)				(((_id_) & AK_EFFECT_TYPE_INSERT) == 0)

// source
const AkPluginID AK_EFFECT_TYPE_SOURCE				= 0x00010000;
#define IS_SOURCE_EFFECT(_id_)					(((_id_) & AK_EFFECT_TYPE_SOURCE) != 0)
#define ISNOT_SOURCE_EFFECT(_id_)				(((_id_) & AK_EFFECT_TYPE_SOURCE) == 0)

// mixer
const AkPluginID AK_EFFECT_TYPE_MIXER				= 0x00020000;
#define IS_MIXER_EFFECT(_id_)					(((_id_) & AK_EFFECT_TYPE_MIXER) != 0)
#define ISNOT_MIXER_EFFECT(_id_)				(((_id_) & AK_EFFECT_TYPE_MIXER) == 0)

// effect id build macros
#define AK_SYNC_AK_EFFECT_ID(_type_,_id_)			(AK_VENDORID_AUDIOKINETIC | AK_EFFECT_TYPE_SYNCHRONOUS | (_type_) | (_id_))
#define AK_ASYNC_AK_EFFECT_ID(_type_,_id_)			(AK_VENDORID_AUDIOKINETIC | (_type_) | (_id_))
#define AK_SYNC_EFFECT_ID(_vendor_,_type_,_id_)		((_vendor_) | AK_EFFECT_TYPE_SYNCHRONOUS | (_type_) | (_id_))
#define AK_ASYNC_EFFECT_ID(_vendor_,_type_,_id_)	((_vendor_) | (_type_) | (_id_))

const AkPluginID AK_EFFECTID_LOPASS					= 0x00000000;
const AkPluginID AK_EFFECTID_SINE					= 0x00000001;
const AkPluginID AK_EFFECTID_SILENCE				= 0x00000002;

const AkPluginID AK_SYNC_LOW_PASS					= AK_SYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_INSERT,AK_EFFECTID_LOPASS);
const AkPluginID AK_ASYNC_LOW_PASS					= AK_ASYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_INSERT,AK_EFFECTID_LOPASS);
const AkPluginID AK_SYNC_SINE						= AK_SYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_SOURCE,AK_EFFECTID_SINE);
const AkPluginID AK_ASYNC_SINE						= AK_ASYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_SOURCE,AK_EFFECTID_SINE);
const AkPluginID AK_SYNC_SILENCE					= AK_SYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_SOURCE,AK_EFFECTID_SILENCE);
const AkPluginID AK_ASYNC_SILENCE					= AK_ASYNC_AK_EFFECT_ID(AK_EFFECT_TYPE_SOURCE,AK_EFFECTID_SILENCE);

#endif // _AK_FX_ID_H_
