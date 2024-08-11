/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_EXPORT_H__
#define __VERCON_EXPORT_H__

#ifdef VERSIONCONTROL_EXPORTS
#	define RED_VC_DLL_C extern "C" __declspec( dllexport )
#	define RED_VC_DLL __declspec( dllexport )
#else
#	define RED_VC_DLL_C extern "C" __declspec( dllimport )
#	define RED_VC_DLL __declspec( dllimport )
#endif

#endif // __VERCON_EXPORT_H__
