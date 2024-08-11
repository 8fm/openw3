/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

class CEdVegetationGenerator
{
	friend class CEdVegetationTool;
	friend class CEdVegetationToolPanel;
	friend class CEdVegetationGeneratorEntry;

private:

	TDynArray< TDynArray<Uint32> >				m_CAMap;
	TDynArray< TDynArray<Float> >				m_SEMap;
	TDynArray< TDynArray<Float> >				m_WMap;


	Vector3			m_sunDirectionAtMidday;
	Float			m_maxSlopeLimit;
	Float			m_minSlopeLimit;
	Float			m_waterLevel;
	
	Float			m_minGrowEnergy;	
	Float			m_minSeedEnergy;
	
	Float			m_sunEnergy;
	Float			m_energyDiff;
	Float			m_cellRadius;
	Float			m_distributionSpeed;
	Float			m_cellSEColorPower;	

	Int32				m_numIterations;
	CAreaComponent* m_cellGrowthArea;

	void init(void);
	Uint32			m_totalAliveCells;

public:

	Uint32										mCellSizeX;
	Uint32										mCellSizeY;
	Vector3										mCellBase;	
	Float										cellDensity;
	Uint32										additionalCells;	

	TDynArray< TDynArray<Vector3> >				HMap;
	TDynArray< CEdVegetationGeneratorEntry* >	entryList;		

	void Iterate(void);
	Int32 GetNumIterations(void);
	void ResetIterations( bool resetEntryList );
	bool InitBbox( CAreaComponent* ac );

	void SetParams( Float density, Float minWaterD );
	void SetParams( Float sLimit, Float minEnergyT, Float minSeedE, Float minGrowthE, Float minSlopeLimit, Float sunEnergy, Float addCells, Float cellSunColorPower, Float distrSpeed );	

	bool IsCellAlive(int x, int y);
	void IterateCA(int cellsNeedToStayAlive, int cellsNeedToBeBorn);
	void IterateCA(void);

	void SetCellAlive(int x, int y, bool isAlive);
	void IterateHmap(void);
	void IterateWMap( Int32 steps );
	void IterateSEMap(void);
	
	void IterateWFill(void);

	Uint32 GetNumNeighboursAlive(int x, int y);
	void SetEnergy(int x, int y, Float energy);
	Float GetEnergy(int x, int y, bool getSunEnergyOnly);
	Float GetCellSize(void);	
	Float GetNeighboursSlopeH(int x, int y);
	Float GetNeighboursSlopeV(int x, int y);
	
	void CleanGrowthArea(void);

	void SortByVolume(void);
	void SortByHeight(void);

	void AddEntry( Float height, Float volume, String name, Int32 species, Int32 index, wxStaticBitmap* bitmap );
	void ClearAllEntries( void );

	bool SetEnergyScopeByType( Int32 type, Float scopeMul );	
	void SetEntryScope( Int32 index, Float maxE, Float minE );
	void SetEntryDensity( Int32 index, Float density, Float clusters );
	Float GetEntryScopeMax( Int32 index );
	Float GetEntryScopeMin( Int32 index );
	Float GetEntryDensity( Int32 index );
	Float GetEntryClusters( Int32 index );	

	Int32 GetResourceIndex( Int32 x, Int32 y, Vector3 &scale );	
	Int32 GetResourceIndex( Int32 type );

	CAreaComponent*		GetCellGrowthArea(void);	
	Vector2				GetAreaSize(void);

	Int32 GetType( Int32 i );
	void SetType( Int32 index, Int32 type );
	
	CEdVegetationGenerator();
	~CEdVegetationGenerator();

};

class CEdVegetationGeneratorEntry
{

public:

	Int32				m_type;
	String			m_name;
	Float			volumeCapacity;
	Float			baseHeight;

	Int32				m_species;
	String			m_speciesName;
	
	Float			energyLevelScopeMax;
	Float			energyLevelScopeMin;

	Float			scaleMax;
	Float			scaleMin;

	Int32				defIndex;

	Float			m_spawnDensityRand;
	Float			m_spawnEntryClusters;

	inline void save( FILE* f )
	{
		Int32 len = m_name.Size();
		fwrite(&len,sizeof(Int32),1,f);
		fwrite(UNICODE_TO_ANSI( m_name.AsChar() ),sizeof(char)*len,1,f );

		fwrite(&m_type,sizeof(Int32),1,f);
		fwrite(&volumeCapacity,sizeof(Float),1,f);
		fwrite(&baseHeight,sizeof(Float),1,f);
		fwrite(&m_species,sizeof(Int32),1,f);

		len = m_speciesName.Size();
		fwrite(&len,sizeof(Int32),1,f);
		fwrite(UNICODE_TO_ANSI( m_speciesName.AsChar() ),sizeof(char)*len,1,f );

		fwrite(&energyLevelScopeMax,sizeof(Float),1,f);
		fwrite(&energyLevelScopeMin,sizeof(Float),1,f);

		fwrite(&scaleMax,sizeof(Float),1,f);
		fwrite(&scaleMin,sizeof(Float),1,f);

		fwrite(&defIndex,sizeof(Int32),1,f);

		fwrite(&m_spawnDensityRand,sizeof(Float),1,f);
		fwrite(&m_spawnEntryClusters,sizeof(Float),1,f);
	}

	inline void load( FILE* f )
	{
		char buf[1024]={0};
		Int32 len = 0;
		//name was loaded

		fread(&m_type,sizeof(Int32),1,f);
		fread(&volumeCapacity,sizeof(Float),1,f);
		fread(&baseHeight,sizeof(Float),1,f);
		fread(&m_species,sizeof(Int32),1,f);

		fread(&len,sizeof(Int32),1,f);
		fread(buf,sizeof(char)*len,1,f );
		buf[len]=0;
		m_speciesName.Set( ANSI_TO_UNICODE(buf) );

		fread(&energyLevelScopeMax,sizeof(Float),1,f);
		fread(&energyLevelScopeMin,sizeof(Float),1,f);

		fread(&scaleMax,sizeof(Float),1,f);
		fread(&scaleMin,sizeof(Float),1,f);

		fread(&defIndex,sizeof(Int32),1,f);

		fread(&m_spawnDensityRand,sizeof(Float),1,f);
		fread(&m_spawnEntryClusters,sizeof(Float),1,f);
	}

	inline void skip( FILE* f )
	{
		char buf[1024]={0};
		Int32 len = 0;

		fread(buf,sizeof(Int32),1,f);
		fread(buf,sizeof(Float),1,f);
		fread(buf,sizeof(Float),1,f);
		fread(buf,sizeof(Int32),1,f);

		fread(&len,sizeof(Int32),1,f);
		fread(buf,sizeof(char)*len,1,f );

		fread(buf,sizeof(Float),1,f);
		fread(buf,sizeof(Float),1,f);

		fread(buf,sizeof(Float),1,f);
		fread(buf,sizeof(Float),1,f);

		fread(buf,sizeof(Int32),1,f);

		fread(buf,sizeof(Float),1,f);
		fread(buf,sizeof(Float),1,f);
	}

	wxStaticBitmap*	wBitmap;

	void SetEnergyLevels(void);
	void SetType( Int32 type );
	void SetLevels( void );

	CEdVegetationGeneratorEntry( Float height, Float volume, String name, Int32 species, Int32 index, wxStaticBitmap* bitmap );
	~CEdVegetationGeneratorEntry();	
};
