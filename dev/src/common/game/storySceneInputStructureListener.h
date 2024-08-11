#pragma once

class CStorySceneInput;

class IStorySceneInputStructureListener
{
public:
	virtual void OnNameChanged( CStorySceneInput* sender ) {}
	virtual void OnLinksChanged( CStorySceneInput* sender ) {}
};