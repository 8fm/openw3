/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "meshStats.h"
#include <wx/fdrepdlg.h>
#include "../../common/engine/mesh.h"

struct MeshTextureInfo;

enum ECmpMeshes
{
	CmpMeshesByRefCount,
	CmpMeshesByDataSize,
	CmpMeshesByChunks,
	CmpMeshesByDepotPath,
	CmpMeshesByFileName,
};

wxArrayString GetCmpMeshesFriendlyNames();


struct MeshInfo
{
	CMesh*					m_mesh;
	Uint32					m_refCount;
	Uint32					m_dataSize;
	Uint32					m_index;

	MeshInfo( CMesh* mesh, Uint32 index )
		: m_mesh( mesh )
		, m_refCount( 1 )
		, m_dataSize( MeshStatsNamespace::CalcMeshRenderDataSize(mesh) )
		, m_index( index )
	{
	}

	static ECmpMeshes comparer1, comparer2;

	static int DoubleCompare( const void* elem0, const void* elem1 )
	{
		int result = 0;

		switch( comparer1 )
		{
		case CmpMeshesByDataSize:
			result = CmpFuncByDataSize(elem0, elem1);
			break;
		case CmpMeshesByRefCount:
			result = CmpFuncByRefCount(elem0, elem1);
			break;		
		case CmpMeshesByChunks:
			result = CmpFuncByChunks(elem0, elem1);
			break;
		case CmpMeshesByFileName:
			result = CmpFuncByFileName(elem0, elem1);
			break;
		case CmpMeshesByDepotPath:
			result = CmpFuncByDepotPath(elem0, elem1);
			break;
		}

		if ( result != 0 )
		{
			return result;
		}
		else
		{
			switch( comparer2 )
			{
			case CmpMeshesByDataSize:
				return CmpFuncByDataSize(elem0, elem1);
			case CmpMeshesByRefCount:
				return CmpFuncByRefCount(elem0, elem1);	
			case CmpMeshesByChunks:
				return CmpFuncByChunks(elem0, elem1);
			case CmpMeshesByFileName:
				return CmpFuncByFileName(elem0, elem1);
			case CmpMeshesByDepotPath:
				return CmpFuncByDepotPath(elem0, elem1);
			}
		}

		return 0;
	}

	static int CmpFuncByRefCount( const void* elem0, const void* elem1 )
	{
		const MeshInfo* header0 = *(const MeshInfo**)elem0;
		const MeshInfo* header1 = *(const MeshInfo**)elem1;
		if ( header0->m_refCount < header1->m_refCount ) return -1;
		if ( header0->m_refCount > header1->m_refCount ) return 1;
		return 0;
	}

	static int CmpFuncByDataSize( const void* elem0, const void* elem1 )
	{
		const MeshInfo* header0 = *(const MeshInfo**)elem0;
		const MeshInfo* header1 = *(const MeshInfo**)elem1;
		if ( header0->m_dataSize > header1->m_dataSize ) return -1;
		if ( header0->m_dataSize < header1->m_dataSize ) return 1;
		return 0;
	}

	static int CmpFuncByChunks( const void* elem0, const void* elem1 )
	{
		const MeshInfo* header0 = *(const MeshInfo**)elem0;
		const MeshInfo* header1 = *(const MeshInfo**)elem1;
		if ( header0->m_mesh->CountLODChunks(0) > header1->m_mesh->CountLODChunks(0)) return 1;
		if ( header0->m_mesh->CountLODChunks(0) < header1->m_mesh->CountLODChunks(0) ) return -1;
		return 0;
	}

	static int CmpFuncByFileName( const void* elem0, const void* elem1 )
	{
		const MeshInfo* header0 = *(const MeshInfo**)elem0;
		const MeshInfo* header1 = *(const MeshInfo**)elem1;
		if ( header0->m_mesh->GetFile()->GetFileName() < header1->m_mesh->GetFile()->GetFileName() ) return -1;
		if ( header0->m_mesh->GetFile()->GetFileName() > header1->m_mesh->GetFile()->GetFileName() ) return 1;

		return 0;
	}

	static int CmpFuncByDepotPath( const void* elem0, const void* elem1 )
	{
		const MeshInfo* header0 = *(const MeshInfo**)elem0;
		const MeshInfo* header1 = *(const MeshInfo**)elem1;
		if ( header0->m_mesh->GetFile()->GetDepotPath() < header1->m_mesh->GetFile()->GetDepotPath() ) return -1;
		if ( header0->m_mesh->GetFile()->GetDepotPath() > header1->m_mesh->GetFile()->GetDepotPath() ) return 1;
		return 0;
	}
};

enum ECmpEntities
{
	CmpEntitiesByAttachedMeshes,
	CmpEntitiesByName,
	CmpEntitiesByRefCount,
	CmpEntitiesByError,
};

struct EntityInfo
{
	const CEntity*		m_entity;
	Uint32				m_refCount;
	TDynArray< Uint32 >	m_usedMeshes;
	String				m_errorMessage;

	EntityInfo( const CEntity* entity, Uint32 meshIndex, const String& errorMessage )
		: m_entity( entity )
		, m_refCount( 1 )
		, m_errorMessage( errorMessage )
	{
		m_usedMeshes.PushBack( meshIndex );
	}

	static ECmpEntities comparer1, comparer2;

	static int DoubleCompare( const void* elem0, const void* elem1 )
	{
		int result = 0;

		switch( comparer1 )
		{
		case CmpEntitiesByAttachedMeshes:
			result = CmpFuncByAttachedMeshes(elem0, elem1);
			break;
		case CmpEntitiesByName:
			result = CmpFuncByName(elem0, elem1);
			break;		
		case CmpEntitiesByRefCount:
			result = CmpFuncByRefCount(elem0, elem1);
			break;
		case CmpEntitiesByError:
			result = CmpFuncByError(elem0, elem1);
			break;
		}

		if ( result != 0 )
		{
			return result;
		}
		else
		{
			switch( comparer2 )
			{
			case CmpEntitiesByAttachedMeshes:
				return CmpFuncByAttachedMeshes(elem0, elem1);
				break;
			case CmpEntitiesByName:
				return CmpFuncByName(elem0, elem1);
				break;		
			case CmpEntitiesByRefCount:
				return CmpFuncByRefCount(elem0, elem1);
				break;
			case CmpEntitiesByError:
				result = CmpFuncByError(elem0, elem1);
				break;
			}
		}

		return 0;
	}

	static int CmpFuncByAttachedMeshes( const void* elem0, const void* elem1 )
	{
		const EntityInfo* header0 = *(const EntityInfo**)elem0;
		const EntityInfo* header1 = *(const EntityInfo**)elem1;
		if ( header0->m_usedMeshes.Size() < header1->m_usedMeshes.Size() ) return -1;
		if ( header0->m_usedMeshes.Size() > header1->m_usedMeshes.Size() ) return 1;

		return 0;
	}

	static int CmpFuncByRefCount( const void* elem0, const void* elem1 )
	{
		const EntityInfo* header0 = *(const EntityInfo**)elem0;
		const EntityInfo* header1 = *(const EntityInfo**)elem1;
		if ( header0->m_refCount < header1->m_refCount ) return -1;
		if ( header0->m_refCount > header1->m_refCount ) return 1;

		return 0;
	}

	static int CmpFuncByError( const void* elem0, const void* elem1 )
	{
		const EntityInfo* header0 = *(const EntityInfo**)elem0;
		const EntityInfo* header1 = *(const EntityInfo**)elem1;
		if ( header0->m_errorMessage.Size() < header1->m_errorMessage.Size() ) return 1;
		if ( header0->m_errorMessage.Size() > header1->m_errorMessage.Size() ) return -1;

		return 0;
	}

	static int CmpFuncByName( const void* elem0, const void* elem1 )
	{
		const EntityInfo* header0 = *(const EntityInfo**)elem0;
		const EntityInfo* header1 = *(const EntityInfo**)elem1;
		if ( header0->m_entity->GetDisplayName() < header1->m_entity->GetDisplayName() ) return -1;
		if ( header0->m_entity->GetDisplayName() > header1->m_entity->GetDisplayName() ) return 1;

		return 0;
	}
};

class CEdMeshStatsViewerTool : public wxFrame
{
	DECLARE_EVENT_TABLE();


private:

	static int							m_refCount;

	wxHtmlWindow						*m_htmlViewEntities;
	wxHtmlWindow						*m_htmlViewMeshesCollapsed;
	wxHtmlWindow						*m_htmlViewMeshes;
	wxHtmlWindow						*m_htmlViewTextures;
	wxHtmlWindow						*m_htmlViewAtlases;
	wxHtmlWindow						*m_htmlViewFoliage;

	wxString							m_entitiesHtml;
	wxString							m_meshesHtml;
	wxString							m_meshesCollapsedHtml;
	wxString							m_texturesHtml;
	wxString							m_atlasesHtml;
	wxString							m_foliageHtml;

	wxNotebook							*m_notebook;
	wxChoice							*m_listMeshes1;
	wxChoice							*m_listMeshes2;
	wxChoice							*m_listTextures1;
	wxChoice							*m_listTextures2;
	wxChoice							*m_listEntities1;
	wxChoice							*m_listEntities2;

	wxFindReplaceDialog					*m_wxFindDlg;
	wxFindReplaceData					*m_wxFindData;
	Uint32								m_findLastIndex;				//!< Used in find next/prev
	Uint32								m_findPageIndex;				//!< On which page find was started

	TDynArray<String>					m_findEntriesEntities;			//!< lowercase helper arrays for finding
	TDynArray<String>					m_findEntriesMeshes;			//!<
	TDynArray<String>					m_findEntriesTextures;			//!<


	TEdShortcutArray					m_shortcuts;


	THashMap<String, MeshTextureInfo*>	m_allUsedTextures;
	THashMap<CBitmapTexture*, MeshTextureInfo*>	m_allUsedAtlasTextures;	
	TDynArray<MeshInfo*>				m_allMeshes;
	TDynArray<MeshInfo*>				m_allMeshesUnsorted;
	TDynArray<EntityInfo*>				m_allEntities;
	TDynArray<String>				m_allCubeNames;
	Uint32								m_textureAnchorCounter;			//!< Helper counter for generating texture anchors




	/// stats
	Uint32								m_totalDifferentMeshes;
	Uint32								m_totalChunks;
	Uint32								m_totalMeshData;

	Uint32								m_totalDifferentMeshesOccuringOnce;
	Uint32								m_totalChunksOccuringOnce;
	Uint32								m_totalMeshDataOccuringOnce;

	Uint32								m_potentialDuplicateEntityCount;

	Uint32								m_totalTextures;
	Uint32								m_totalTextureData;

	Uint32								m_totalTexturesOccuringOnce;
	Uint32								m_totalTextureDataOccuringOnce;	

	Uint32								m_totalAtlasTexturesLoaded;
	Uint32								m_totalAtlasTexturesUsed;
	Uint32								m_totalAtlasData;
	Uint32								m_totalAtlasTextureData;

private:
	void RefreshHtmlControl();
	
	//! Generate info about mesh in m_meshes table - index is it's index in m_meshes
	void GenerateMeshInfo(const Int32 index, String &output, String &outputCollapsed );

	void GenerateTextureInfo( MeshTextureInfo* info, String &output );
	
	void GenerateEntityInfo( EntityInfo* info, String &output );

	void Select(String s);

	void Goto(String s);

	void SelectEntity(String s);

	void GotoEntity(String s);

	TEdShortcutArray* GetAccelerators();

	void OnRefresh( wxCommandEvent& event );
	void OnLinkClicked( wxHtmlLinkEvent& event );
	void EnumAllMeshes();
	void EnumAllFoliage();
	void SaveHtml( wxCommandEvent& event );
	void ScrollToSelection( wxCommandEvent& event );

	void OnFindOpen( wxCommandEvent& event );

	void OnFind( wxFindDialogEvent& event );

public:
	static int GetRefCount(){return m_refCount; };
	CEdMeshStatsViewerTool(wxWindow* parent);
	~CEdMeshStatsViewerTool();
};