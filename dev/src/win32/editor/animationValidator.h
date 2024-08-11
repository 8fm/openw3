
class CAnimationValidator
{
public:
	CAnimationValidator( Bool andFixSourceAnimData = false );
	
	void Execute( const TDynArray< CDirectory* > dirs, Uint32& numProblemsFound );
	void DumpErrorsToFile( String filename, CDirectory* saveDirectory );

private:
	String m_curentFilePath;
	THashMap< String, TDynArray< String > > m_errors; // grouped by filepath
	Bool m_fixSourceAnimData;
	Bool m_changedAnything;
	Bool m_markAnimSetDirtyWhenFixing;

	void ValidateSkeleton( const CSkeleton* skeleton );
	void ValidateAnimations( const TDynArray< CSkeletalAnimationSetEntry* >& animationSets );
	void ValidateAnimation( const CSkeletalAnimation* animation);

	void ReportError( const String& error );
};