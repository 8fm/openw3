/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/textureArray.h"

struct MeshTextureInfo;
struct SMeshTextureArrayInfo;
struct SMeshChunk;
enum EMeshVertexType : CEnum::TValueType;

enum ECmpTexture
{
	CmpByRefCount,
	CmpByDataSize,
	CmpByDimensions,
	CmpByDepotPath,
	CmpByFileName,
};

namespace MeshStatsNamespace
{
	const Double CalcChunkArea( const CMesh* mesh, Uint32 chunkIndex );
	const Double CalcChunkAverageTriangleArea( const CMesh* mesh, Uint32 chunkIndex );
	wxString MemSizeToText( Uint32 memSize );
	wxString RawCountToText( Uint32 count );
	Uint32 CalcMeshRenderDataSize( CMesh* mesh );
	Uint32 CalcMeshLodRenderDataSize( const CMesh* mesh, Uint32 lodIndex );
	Uint32 CalcMeshRawDataSize( CMesh* mesh );
	Uint32 CalcMeshCollisionDataSize( CMesh* mesh );
	Uint32 CalcTextureDataSize( const CBitmapTexture* texture );
	wxString MeshVertexTypeToName( EMeshVertexType vt );

	void GatherTextureArraysUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< SMeshTextureArrayInfo* >& textures );
	void GatherTextureArraysUsedByMesh( const CMesh* mesh, TDynArray< SMeshTextureArrayInfo* >& textures );

	void GatherTexturesUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< MeshTextureInfo* >& textures );
	void GatherTexturesUsedByMesh( const CMesh* mesh, TDynArray< MeshTextureInfo* >& textures );

	void GatherCubeNamesUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< String >& cubeNames );
	void GatherCubeNamesUsedByMesh( CMesh* mesh, TDynArray< String >& cubeNames );

	wxString ExtractTextureThumbnail( CBitmapTexture* texture );
	void CalcChunkSkinningHistogram( const SMeshChunk& chunk, Uint32& s1, Uint32& s2, Uint32& s3, Uint32& s4 );
	void CalcChunkSkinningUsedBones( const SMeshChunk& chunk, TDynArray< Uint32 >& allUsedBones );
	wxArrayString GetCmpTextureFriendlyNames();
};

struct SMeshTextureArrayInfo
{
	THandle< CTextureArray >		m_textureArray;
	TDynArray< Uint32 >				m_usedByMaterials;

	SMeshTextureArrayInfo( const CTextureArray* textureArray )
		: m_textureArray( textureArray )
	{
		/* intentionally empty */
	}

	static int CmpFuncByDepotPath( const SMeshTextureArrayInfo* elem0, const SMeshTextureArrayInfo* elem1 )
	{
		const String& elem0Path = elem0->m_textureArray->GetFile()->GetDepotPath();
		const String& elem1Path = elem1->m_textureArray->GetFile()->GetDepotPath();

		if ( elem0Path < elem1Path ) 
		{
			return -1;
		}
		else if( elem0Path > elem1Path )
		{
			return 1;
		}
		return 0;
	}
};

struct MeshTextureInfo
{
	CBitmapTexture*			m_texture;
	wxString				m_thumbnailFile;
	Uint32					m_dataSize;
	TDynArray< Uint32 >		m_usedByMaterials;
	TDynArray< Uint32 >		m_isUsedByMeshes;
	Uint32					m_refCount;
	Uint32					m_anchorCounter;

	MeshTextureInfo( CBitmapTexture* texture )
		: m_texture( texture )
		, m_dataSize( MeshStatsNamespace::CalcTextureDataSize( texture ) )
		, m_refCount( 1 )
	{
	}

	static ECmpTexture comparer1, comparer2;

	static int DoubleCompare( const void* elem0, const void* elem1 )
	{
		int result = 0;

		switch( comparer1 )
		{
		case CmpByDataSize:
			result = CmpFuncByDataSize(elem0, elem1);
			break;
		case CmpByRefCount:
			result = CmpFuncByRefCount(elem0, elem1);
			break;		
		case CmpByDimensions:
			result = CmpFuncByDimensions(elem0, elem1);
			break;
		case CmpByFileName:
			result = CmpFuncByFileName(elem0, elem1);
			break;
		case CmpByDepotPath:
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
			case CmpByDataSize:
				return CmpFuncByDataSize(elem0, elem1);
			case CmpByRefCount:
				return CmpFuncByRefCount(elem0, elem1);	
			case CmpByDimensions:
				return CmpFuncByDimensions(elem0, elem1);
			case CmpByFileName:
				return CmpFuncByFileName(elem0, elem1);
			case CmpByDepotPath:
				return CmpFuncByDepotPath(elem0, elem1);
			}
		}

		return 0;
	}

	static int CmpFuncByDataSize( const void* elem0, const void* elem1 )
	{
		const MeshTextureInfo* header0 = *(const MeshTextureInfo**)elem0;
		const MeshTextureInfo* header1 = *(const MeshTextureInfo**)elem1;
		if ( header0->m_dataSize < header1->m_dataSize ) return 1;
		if ( header0->m_dataSize > header1->m_dataSize ) return -1;
		return 0;
	}

	static int CmpFuncByRefCount( const void* elem0, const void* elem1 )
	{
		const MeshTextureInfo* header0 = *(const MeshTextureInfo**)elem0;
		const MeshTextureInfo* header1 = *(const MeshTextureInfo**)elem1;
		if ( header0->m_refCount < header1->m_refCount ) return -1;
		if ( header0->m_refCount > header1->m_refCount ) return 1;
		return 0;
	}

	static int CmpFuncByDimensions( const void* elem0, const void* elem1 )
	{
		const MeshTextureInfo* header0 = *(const MeshTextureInfo**)elem0;
		const MeshTextureInfo* header1 = *(const MeshTextureInfo**)elem1;
		if (( header0->m_texture->GetWidth() * header0->m_texture->GetHeight()) < (header1->m_texture->GetWidth() * header1->m_texture->GetHeight())) return 1;
		if (( header0->m_texture->GetWidth() * header0->m_texture->GetHeight()) > (header1->m_texture->GetWidth() * header1->m_texture->GetHeight())) return -1;
		return 0;
	}

	static int CmpFuncByFileName( const void* elem0, const void* elem1 )
	{
		const MeshTextureInfo* header0 = *(const MeshTextureInfo**)elem0;
		const MeshTextureInfo* header1 = *(const MeshTextureInfo**)elem1;
		if ( header0->m_texture->GetFile()->GetFileName() < header1->m_texture->GetFile()->GetFileName() ) return -1;
		if ( header0->m_texture->GetFile()->GetFileName() > header1->m_texture->GetFile()->GetFileName() ) return 1;
		return 0;
	}

	static int CmpFuncByDepotPath( const void* elem0, const void* elem1 )
	{
		const MeshTextureInfo* header0 = *(const MeshTextureInfo**)elem0;
		const MeshTextureInfo* header1 = *(const MeshTextureInfo**)elem1;
		if ( header0->m_texture->GetFile()->GetDepotPath() < header1->m_texture->GetFile()->GetDepotPath() ) return -1;
		if ( header0->m_texture->GetFile()->GetDepotPath() > header1->m_texture->GetFile()->GetDepotPath() ) return 1;
		return 0;
	}
};


struct ProblemList
{
	wxString	problemsCode;
	wxString	warningCode;

	void EmitProblem( wxChar* format, ... )
	{
		// Format
		va_list arglist;
		va_start(arglist, format);
		wxChar formattedBuf[ 4096 ];
		Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist );

		// Emit
		problemsCode += wxT("<font color=\"#C00000\"><b>");
		problemsCode += formattedBuf;
		problemsCode += wxT("</b></font><br>");
	}

	void EmitWarning( wxChar* format, ... )
	{
		// Format
		va_list arglist;
		va_start(arglist, format);
		wxChar formattedBuf[ 4096 ];
		Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist );

		// Emit
		warningCode += wxT("<font color=\"#C0C000\"><b>");
		warningCode += formattedBuf;
		warningCode += wxT("</b></font><br>");
	}
};

struct MeshTextureGroupInfo
{
	CName	m_name;
	Uint32	m_dataSize;
	Uint32	m_numTextures;

	static int CmpFumc( const void* elem0, const void* elem1 )
	{
		const MeshTextureGroupInfo* header0 = *(const MeshTextureGroupInfo**)elem0;
		const MeshTextureGroupInfo* header1 = *(const MeshTextureGroupInfo**)elem1;
		if ( header0->m_dataSize < header1->m_dataSize ) return 1;
		if ( header0->m_dataSize > header1->m_dataSize ) return -1;
		return 0;
	}
};
