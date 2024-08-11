/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Voicetag selection
class CVoiceTagListSelection : public CListSelection
{
public:
	CVoiceTagListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};