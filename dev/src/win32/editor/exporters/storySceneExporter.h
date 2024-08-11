/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CStoryScene;
class CStorySceneSection;
class CStorySceneLine;
class CStorySceneFlowCondition;
class CStorySceneFlowSwitch;
class CStorySceneChoice;
class CStorySceneComment;

class CStorySceneExporter
{
public:
	CStorySceneExporter();
	virtual ~CStorySceneExporter() {}

	virtual Bool IsBatchedExport() { return false; }

	virtual void ExportScene( CStoryScene* scene );
	virtual void ExportSceneSection( CStoryScene* storyScene, const CStorySceneSection* section );
	virtual void ExportFlowCondition( CStoryScene* storyScene, const CStorySceneFlowCondition* flowCondition ) {};
	virtual void ExportFlowSwitch( CStoryScene* storyScene, const CStorySceneFlowSwitch* flowCondition ){};

protected:
	virtual void DoBeginExport() {}
	virtual void DoEndExport() {}
	virtual void DoExportEmptyLine() {}
	virtual void DoExportStorySceneChoice( const CStorySceneChoice* sectionChoice, CStoryScene* storyScene ) {}
	virtual void DoExportStorySceneComment( const CStorySceneComment* sceneComment, CStoryScene* storyScene, Bool isLastElement = false ) {}
	virtual void DoExportSceneSectionHeader( const CStorySceneSection* section, CStoryScene* storyScene ) {}
	virtual void DoExportStorySceneLine( const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine = false ) {}
};
