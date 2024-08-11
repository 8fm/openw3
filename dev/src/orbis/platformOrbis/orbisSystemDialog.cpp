/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "orbisSystemDialog.h"

#include <libsysmodule.h>

// Base library
#pragma comment( lib, "libSceCommonDialog_stub_weak.a" )

// User profile dialog
#include <np_profile_dialog.h>
#pragma comment( lib, "libSceNpProfileDialog_stub_weak.a" )

// Help / web browser
#include <web_browser_dialog.h>
#pragma comment( lib, "libSceWebBrowserDialog_stub_weak.a" )

// Error dialog
#include <error_dialog.h>
#pragma comment( lib, "libSceErrorDialog_stub_weak.a" )

// Commerce dialog
#include <np_commerce_dialog.h>
#pragma comment( lib, "libSceNpCommerce_stub_weak.a" )

//////////////////////////////////////////////////////////////////////////
// Load Module
//////////////////////////////////////////////////////////////////////////
template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Load()
{
	return sceSysmoduleLoadModule( SCE_SYSMODULE_NP_PROFILE_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Load()
{
	return sceSysmoduleLoadModule( SCE_SYSMODULE_WEB_BROWSER_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Load()
{
	return sceSysmoduleLoadModule( SCE_SYSMODULE_ERROR_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Load()
{
	return sceSysmoduleLoadModule( SCE_SYSMODULE_NP_COMMERCE );
}

//////////////////////////////////////////////////////////////////////////
// Unload Module
//////////////////////////////////////////////////////////////////////////
template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Unload()
{
	return sceSysmoduleUnloadModule( SCE_SYSMODULE_NP_PROFILE_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Unload()
{
	return sceSysmoduleUnloadModule( SCE_SYSMODULE_WEB_BROWSER_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Unload()
{
	return sceSysmoduleUnloadModule( SCE_SYSMODULE_ERROR_DIALOG );
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Unload()
{
	return sceSysmoduleUnloadModule( SCE_SYSMODULE_NP_COMMERCE );
}

//////////////////////////////////////////////////////////////////////////
// Initialise
//////////////////////////////////////////////////////////////////////////

template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Initialize()
{
	return sceNpProfileDialogInitialize();
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Initialize()
{
	return sceWebBrowserDialogInitialize();
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Initialize()
{
	return sceErrorDialogInitialize();
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Initialize()
{
	return sceNpCommerceDialogInitialize();
}

//////////////////////////////////////////////////////////////////////////
// Terminate
//////////////////////////////////////////////////////////////////////////

template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Terminate()
{
	return sceNpProfileDialogTerminate();
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Terminate()
{
	return sceWebBrowserDialogTerminate();
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Terminate()
{
	return sceErrorDialogTerminate();
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Terminate()
{
	return sceNpCommerceDialogTerminate();
}

//////////////////////////////////////////////////////////////////////////
// Open / Show
//////////////////////////////////////////////////////////////////////////

template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Open( const SceNpProfileDialogParam& params )
{
	return sceNpProfileDialogOpen( &params );
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Open( const SceWebBrowserDialogParam& params )
{
	return sceWebBrowserDialogOpen( &params );
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Open( const SceErrorDialogParam& params )
{
	return sceErrorDialogOpen( &params );
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Open( const SceNpCommerceDialogParam& params )
{
	return sceNpCommerceDialogOpen( &params );
}

//////////////////////////////////////////////////////////////////////////
// IsShown
//////////////////////////////////////////////////////////////////////////

template<>
Bool COrbisSystemDialog< SceNpProfileDialogParam >::IsShown() const
{
	return sceNpProfileDialogUpdateStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING;
}

template<>
Bool COrbisSystemDialog< SceWebBrowserDialogParam >::IsShown() const
{
	return sceWebBrowserDialogUpdateStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING;
}

template<>
Bool COrbisSystemDialog< SceErrorDialogParam >::IsShown() const
{
	return sceErrorDialogGetStatus() == SCE_ERROR_DIALOG_STATUS_RUNNING;
}

template<>
Bool COrbisSystemDialog< SceNpCommerceDialogParam >::IsShown() const
{
	return sceNpCommerceDialogUpdateStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING;
}

//////////////////////////////////////////////////////////////////////////
// Close
//////////////////////////////////////////////////////////////////////////

template<>
Int32 COrbisSystemDialog< SceNpProfileDialogParam >::Close()
{
	return sceNpProfileDialogClose();
}

template<>
Int32 COrbisSystemDialog< SceWebBrowserDialogParam >::Close()
{
	return sceWebBrowserDialogClose();
}

template<>
Int32 COrbisSystemDialog< SceErrorDialogParam >::Close()
{
	return sceErrorDialogClose();
}

template<>
Int32 COrbisSystemDialog< SceNpCommerceDialogParam >::Close()
{
	return sceNpCommerceDialogClose();
}

