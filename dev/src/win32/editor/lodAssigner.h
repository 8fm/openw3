/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/directory.h"
#include "../../common/engine/meshTypeResource.h"

class CLODAssigner : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CDirectory*						m_directory;

public:
	struct LODData
	{
		Bool						m_modify;
		SMeshTypeResourceLODLevel	m_lodLevel;
	};


	CLODAssigner( wxWindow* parent, CDirectory* dir );
	~CLODAssigner();

	virtual wxString GetShortTitle() { return m_directory->GetDepotPath().AsChar() + wxString(TXT(" - LOD Assigner")); }
	CDirectory* GetDirectory() const { return m_directory; }

protected:
	void OnAssign( wxCommandEvent& event );
	void OnModifyChanged( wxCommandEvent& event );
	void DispatchEditorEvent( const CName& name, IEdEventData* data );


protected:
	void LogDir( CDirectory* dir, Uint32 level, const TDynArray< LODData >& lodData, Bool processSubdirectories, Uint32& processedMeshes, const Uint32& max );
	void ListMeshes( CDirectory* dir, const TDynArray< LODData >& lodData, Uint32& processedMeshes, const Uint32& max );
	void CountMeshes( CDirectory* dir, Bool recursive, Uint32& count );

private:
	void CollectLODData( TDynArray< LODData >& data );

	void UpdateMeshLODs( CMeshTypeResource* mesh, const TDynArray< LODData >& lods );
};
