//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AK/Wwise/SourceControl/ISourceControl.h"
#include "SamplePlugin.h"

namespace
{
	// This is the ID of the plug-in. 
	// This ID must be changed if you use this sample as a template project

	// {BD47DF98-1700-4542-A33F-E44705C25AF0}
	static const GUID s_guidSamplePluginID = 
	{ 0xbd47df98, 0x1700, 0x4542, { 0xa3, 0x3f, 0xe4, 0x47, 0x5, 0xc2, 0x5a, 0xf0 } };
}

//
//  Exported functions
//

void __stdcall AkSourceControlGetPluginIDList ( AK::Wwise::ISourceControl::PluginIDList& out_rPluginIDList )
{
	out_rPluginIDList.AddTail( s_guidSamplePluginID );
}

void __stdcall AkSourceControlGetPluginInfo ( const GUID& in_rguidPluginID, AK::Wwise::ISourceControl::PluginInfo& out_rPluginInfo )
{
	if ( in_rguidPluginID == s_guidSamplePluginID )
	{
		SamplePlugin::GetPluginInfo( out_rPluginInfo );
	}
}

AK::Wwise::ISourceControl* __stdcall AkSourceControlCreatePlugin ( const GUID& in_rguidPluginID )
{
	AK::Wwise::ISourceControl* pSourceControl = NULL;

	if ( in_rguidPluginID == s_guidSamplePluginID )
	{
		pSourceControl =  new SamplePlugin();
	}

	return pSourceControl;
}