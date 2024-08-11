#pragma once

#if defined(__ANDROID__) || defined(__GNUC__)
	#ifdef ANSEL_SDK_EXPORTS
	#define ANSEL_SDK_API extern "C" __attribute__ ((visibility ("default")))
	#define ANSEL_SDK_CLASS_API  __attribute__ ((visibility ("default")))
	#else
	#define ANSEL_SDK_API 
	#define ANSEL_SDK_CLASS_API
	#endif
#else
	#ifdef ANSEL_SDK_EXPORTS
	#define ANSEL_SDK_API extern "C" __declspec(dllexport)
	#define ANSEL_SDK_CLASS_API __declspec(dllexport)
	#else
	#define ANSEL_SDK_API extern "C" __declspec(dllimport)
	#define ANSEL_SDK_CLASS_API __declspec(dllimport)
	#endif
#endif
