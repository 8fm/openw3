/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CJobActionAnimSelection : public CListSelection
{
protected:
	CJobActionBase *m_jobAction;

public:
	CJobActionAnimSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};
