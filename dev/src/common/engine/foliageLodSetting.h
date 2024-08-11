/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

const Int32 FoliageLodCount = 7;

struct SFoliageLODSetting
{
	DECLARE_RTTI_STRUCT( SFoliageLODSetting );
	Float m_minTreeExtentPerLod[ FoliageLodCount ];
};

BEGIN_CLASS_RTTI( SFoliageLODSetting )
	PROPERTY_EDIT_ARRAY( m_minTreeExtentPerLod, TXT( "" ) )  
END_CLASS_RTTI()
