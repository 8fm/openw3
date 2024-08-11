
#pragma once

class CEdAnimationListExporter
{
	struct AnimsetItem
	{
		String					m_path;
		CSkeletalAnimationSet*	m_animset;

		AnimsetItem() : m_animset( NULL ) {}
	};

private:
	wxWindow*					m_parent;
	TDynArray< AnimsetItem >	m_items;

public:
	CEdAnimationListExporter( wxWindow* parent );
	~CEdAnimationListExporter();

	void ExportToCSV();

private:
	void CollectAllAnimsets();
	void LoadAllAnimsets();

	String GetCSVFilePath() const;
	void WriteToCSV( const String& path );
};
