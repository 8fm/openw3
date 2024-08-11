/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EAnselState
{
	EAS_Off							= 0,
	EAS_FirstFrameOfActivation,
	EAS_Activating,
	EAS_Active,
	EAS_FirstFrameOfDeactivation,
	EAS_Deactivating,
	EAS_FirstFrameAfterDeactivation,
};
