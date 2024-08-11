/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CDrawableComponent;
class CRenderFrame;


/// Color meshes based on their collision type
class CMeshColoringSchemeCollisionType : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeCollisionType();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
private:
	Vector m_colorDisabled;
	Vector m_colorStatic;
	Vector m_colorStaticWalkable;
	Vector m_colorStaticMetaobstacle;
	Vector m_colorDynamic;
	Vector m_colorWalkable;
	Vector m_colorImmediate;
};

/// Color meshes based on type
class CMeshColoringSchemeEntityType : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeEntityType();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
private:
	Vector m_colorEntityTemplate;
	Vector m_colorMesh;
	Vector m_colorStaticMesh;
	Vector m_colorRigidMesh;
	Vector m_colorDestructionMesh;
	Vector m_colorClothMesh;
};

/// Color meshes based on their collision type
class CMeshColoringSchemeShadows : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeShadows();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
private:
	Vector m_colorCastShadows;
	Vector m_colorNoShadows;
	Vector m_colorShadowsFromLocalLightsOnly;
};

/// Color meshes based on sound material
class CMeshColoringSchemeSoundMaterial : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeSoundMaterial();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
private:
	Vector m_colorNone;
	Vector m_colorDirt;
	Vector m_colorStone;
	Vector m_colorWood;
	Vector m_colorWatershallow;
	Vector m_colorWaterdeep;
	Vector m_colorMud;
	Vector m_colorGrass;
	Vector m_colorMetal;
};

/// Color meshes based on is it used for sound occlusion or not
class CMeshColoringSchemeSoundOccl : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeSoundOccl();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
};

/// Color meshes based on rendering LOD
class CMeshColoringSchemeRendering : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeRendering();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );

private:
	Vector m_colorLOD0;
	Vector m_colorLOD1;
	Vector m_colorLOD2;
	Vector m_colorLOD3;
	Vector m_colorLOD4;
};

/// Color meshes based on streaming LOD
class CMeshColoringSchemeStreaming : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeStreaming();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
private:
	Vector m_colorNone;
	Vector m_colorLOD0;
};

/// Color meshes based on layer build tag
class CMeshColoringSchemeLayerBuildTag : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeLayerBuildTag();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );
};

/// Color meshes based on number of chunks
class CMeshColoringSchemeChunks : public IMeshColoringScheme
{
public:
	CMeshColoringSchemeChunks();
	virtual Vector GetMeshSelectionColor( const CDrawableComponent* drawComp ) const;
	virtual void GenerateEditorFragments( CRenderFrame* frame );

private:
	Vector m_colorMaxChunks;
	Vector m_colorMinChunks;	
};
