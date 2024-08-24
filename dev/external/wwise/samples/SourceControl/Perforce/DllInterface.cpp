//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Perforce.h"

namespace
{
	// Perforce - {F56BDC98-5067-4719-86A4-83CD509C54B7}
	static const GUID k_guidPerforceSourceControlID = 
	{ 0xf56bdc98, 0x5067, 0x4719, { 0x86, 0xa4, 0x83, 0xcd, 0x50, 0x9c, 0x54, 0xb7 } };
};

void __stdcall AkSourceControlGetPluginIDList ( AK::Wwise::ISourceControl::PluginIDList& out_rPluginIDList )
{
	if ( PerforceSourceControl::IsAvalaible() )
		out_rPluginIDList.AddTail( k_guidPerforceSourceControlID );
}

void __stdcall AkSourceControlGetPluginInfo ( const GUID& in_rguidPluginID, AK::Wwise::ISourceControl::PluginInfo& out_rPluginInfo )
{
	if ( in_rguidPluginID == k_guidPerforceSourceControlID )
	{
		PerforceSourceControl::GetPluginInfo( out_rPluginInfo );
	}
}

AK::Wwise::ISourceControl* __stdcall AkSourceControlCreatePlugin ( const GUID& in_rguidPluginID )
{
	AK::Wwise::ISourceControl* pSourceControl = NULL;

	if ( in_rguidPluginID == k_guidPerforceSourceControlID )
	{
		pSourceControl =  new PerforceSourceControl();
	}

	return pSourceControl;
}
