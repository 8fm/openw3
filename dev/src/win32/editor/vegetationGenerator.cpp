/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "vegetationGenerator.h"
#include "../../common/engine/areaComponent.h"

#define FG_TYPE_GRASS_MIN 0.0f
#define FG_TYPE_GRASS_MAX 35.0f

#define FG_TYPE_LOWBUSH_MIN 8.0f
#define FG_TYPE_LOWBUSH_MAX 26.0f

#define FG_TYPE_BUSH_MIN 22.0f
#define FG_TYPE_BUSH_MAX 50.0f

#define FG_TYPE_LOWTREE_MIN 40.0f
#define FG_TYPE_LOWTREE_MAX 80.0f

#define FG_TYPE_TREE_MIN 75.0f
#define FG_TYPE_TREE_MAX 200.0f

static int CompareByVolume( const void *a, const void* b )
{
	CEdVegetationGeneratorEntry* entryA = * ( CEdVegetationGeneratorEntry** ) a;
	CEdVegetationGeneratorEntry* entryB = * ( CEdVegetationGeneratorEntry** ) b;

	if ( entryA->volumeCapacity > entryB->volumeCapacity ) return -1;
	if ( entryA->volumeCapacity < entryB->volumeCapacity ) return 1;	
	return 0;
}

static int CompareByHeight( const void *a, const void* b )
{
	CEdVegetationGeneratorEntry* entryA = * ( CEdVegetationGeneratorEntry** ) a;
	CEdVegetationGeneratorEntry* entryB = * ( CEdVegetationGeneratorEntry** ) b;

	if ( entryA->baseHeight > entryB->baseHeight ) return -1;
	if ( entryA->baseHeight < entryB->baseHeight ) return 1;	
	return 0;
}

CEdVegetationGenerator::CEdVegetationGenerator()
{	
	m_cellRadius = 2.0f;
	mCellSizeX = 0;
	mCellSizeY = 0;

	m_numIterations = -1;
	m_totalAliveCells = 0;

	m_cellGrowthArea = NULL;	

	m_sunDirectionAtMidday =  Vector3(0.0f, 0.0f, 1.0f);

	m_maxSlopeLimit = 90.0f;				
	m_minSlopeLimit = 0.0f;

	m_minGrowEnergy = 0.0f;		
	m_minSeedEnergy = 10000.0f;	
		
	m_sunEnergy = 100.0f;
	m_energyDiff = 0.0f;	

	additionalCells = 0;
	m_distributionSpeed = 0.2f;
	m_cellSEColorPower = 0.0f;		
}

void CEdVegetationGenerator::SetParams( Float density, Float minWaterD )
{
	cellDensity = density;
	m_waterLevel = minWaterD;
	init();
}

void CEdVegetationGenerator::SetParams( Float sLimit, Float minEnergyT, Float minSeedE, Float minGrowthE, Float minSlopeLimit, Float sunEnergy, Float addCells, Float cellSunColorPower, Float distrSpeed )
{
	m_maxSlopeLimit = sLimit;		
	m_minSlopeLimit = minSlopeLimit;

	m_minGrowEnergy = minGrowthE;		
	m_minSeedEnergy = minSeedE;	
		
	m_sunEnergy = sunEnergy;
	m_energyDiff = minEnergyT;	

	additionalCells = (Uint32) addCells;
	m_distributionSpeed = distrSpeed;

	m_cellSEColorPower = cellSunColorPower;	
}

Vector2 CEdVegetationGenerator::GetAreaSize(void)
{
	if( m_cellGrowthArea )
	{	
		CAreaComponent::TAreaPoints acPoints = m_cellGrowthArea->GetWorldPoints();

		Float minX = 1000000.0f;
		Int32 minXc = -1;
		Float maxX = -1000000.0f;
		Int32 maxXc = -1;

		Float minY = 1000000.0f;
		Int32 minYc = -1;
		Float maxY = -1000000.0f;
		Int32 maxYc = -1;

		for(Uint32 i=0; i<acPoints.Size(); i++)
		{
			if( acPoints[i].X < minX ) 
			{
				minX = acPoints[i].X;
				minXc = i;
			}

			if( acPoints[i].X > maxX ) 
			{
				maxX = acPoints[i].X;
				maxXc = i;
			}

			if( acPoints[i].Y < minY ) 
			{
				minY = acPoints[i].Y;
				minYc = i;
			}			

			if( acPoints[i].Y > maxY ) 
			{
				maxY = acPoints[i].Y;
				maxYc = i;
			}
		}

		mCellBase.X = minX;
		mCellBase.Y = minY;
		mCellBase.Z = 0.0f;

		return Vector2( Abs(maxX - minX), Abs(maxY - minY) );
	}
	else
		return Vector2(0.0f, 0.0f);
}

void CEdVegetationGenerator::init(void)
{	
	Vector2 vSize = GetAreaSize();

	Float sideX = vSize.X*cellDensity/m_cellRadius;
	Float sideY = vSize.Y*cellDensity/m_cellRadius;
	
	mCellSizeX = sideX;
	mCellSizeY = sideY;

	if( !HMap.Empty() ) HMap.Clear();
	HMap.Resize( mCellSizeX );
	
	if( m_SEMap.Empty() )  m_SEMap.Clear();
	m_SEMap.Resize( mCellSizeX );
	
	if( m_WMap.Empty() )  m_WMap.Clear();
	m_WMap.Resize( mCellSizeX );

	if( m_CAMap.Empty() )  m_CAMap.Clear();
	m_CAMap.Resize( mCellSizeX );

	//init height, energy maps
	for(Uint32 i=0; i<mCellSizeX; i++)
		for(Uint32 j=0; j<mCellSizeY; j++) 
		{

			HMap[i].PushBack( Vector3(0.0f, 0.0f, 0.0f) );	
			m_SEMap[i].PushBack( 0.0f );
			m_WMap[i].PushBack( GEngine->GetRandomNumberGenerator().Get< Float >( m_waterLevel, 0.8f * m_waterLevel ) );
			m_CAMap[i].PushBack( 1 );
			//SetCellAlive(i, j, true);					
		}

		//clean borders
		for(Uint32 i=0; i<mCellSizeX; i++) 
		{
			m_WMap[i][0] = 0.0f;		
			m_WMap[i][mCellSizeY-1] = 0.0f;
			SetCellAlive(i, 0, false);
			SetCellAlive(i, mCellSizeY-1, false);
		}
		
		for(Uint32 j=0; j<mCellSizeY; j++) 
		{
			m_WMap[0][j] = 0.0f;	
			m_WMap[mCellSizeX-1][j] = 0.0f;
			SetCellAlive(0, j, false);
			SetCellAlive(mCellSizeX-1, j, false);
		}
}

bool CEdVegetationGenerator::InitBbox( CAreaComponent* ac )
{
	if( m_numIterations < 0 )
	{
		m_cellGrowthArea = ac;

		// calculate sun direction at midday
		CWorld* attachedWorld = ac->GetEntity()->GetLayer()->GetWorld();
		ASSERT( attachedWorld );
		const CGlobalLightingTrajectory& trajectory = attachedWorld->GetEnvironmentParameters().m_globalLightingTrajectory;
		Vector3 sunDir = trajectory.GetLightDirection( GameTime(0, 12, 0, 0) );
		sunDir *= -1.0f;
		sunDir.Normalize();
		m_sunDirectionAtMidday = sunDir;
		return true;
	}
	else
		return false;
}

void CEdVegetationGenerator::CleanGrowthArea(void)
{
	for(Uint32 i=0; i<mCellSizeX; i++)
		for(Uint32 j=0; j<mCellSizeY; j++)
		{
			if( IsCellAlive(i, j) )
			{ 
				if(!m_cellGrowthArea->TestPointOverlap( HMap[i][j] ) ) SetCellAlive(i, j, false);				
			}
		}
}

void CEdVegetationGenerator::Iterate(void)
{
	if( m_numIterations < 0 )
	{
		//IterateCA(5678, 1234);		
		IterateSEMap();
		IterateWMap( 1 );		
		IterateCA();
		IterateHmap();
		CleanGrowthArea();
	}
	else
	{
		//if(m_numIterations < 8) IterateWFill();		
		IterateWMap( 1 );
		IterateCA();
		IterateHmap();
		CleanGrowthArea();
	}
	m_numIterations++;	
}

void CEdVegetationGenerator::IterateWFill(void)
{
	for(Uint32 i=0; i<mCellSizeX; i++)
		for(Uint32 j=0; j<mCellSizeY; j++)
		{
			Float f = 1.45f*m_WMap[i][j];

			if( f>0.0f && f<m_waterLevel ) m_WMap[i][j] = f;
		}
}

Int32 CEdVegetationGenerator::GetNumIterations(void)
{
	return m_numIterations;
}

Float CEdVegetationGenerator::GetCellSize(void)
{
	if(cellDensity > 0.0001) return m_cellRadius/cellDensity;
	else
		return 1.0f;
}

void CEdVegetationGenerator::ResetIterations(bool resetEntryList)
{	
	for(Uint32 i=0; i<mCellSizeX; i++)		
	{
		m_CAMap[i].Clear();
		m_SEMap[i].Clear();
		m_WMap[i].Clear();
		HMap[i].Clear();
	}	

	m_cellRadius = 2.0f;
	mCellSizeX = 0;
	mCellSizeY = 0;

	if( resetEntryList )
	{
		ClearAllEntries();		
	}	
	m_numIterations = -1;	
}

bool CEdVegetationGenerator::IsCellAlive(int x, int y)
{
	if( m_CAMap[x][y] != 0 ) return true;
	else
		return false;
}

void CEdVegetationGenerator::SetCellAlive(int x, int y, bool alive)
{
	if(alive) m_CAMap[x][y] = 1;			
	else
		m_CAMap[x][y] = 0;			
}

void CEdVegetationGenerator::IterateCA(void)
{
	for(Uint32 i=1; i<mCellSizeX-1; i++)
		for(Uint32 j=1; j<mCellSizeY-1; j++)
		{
			Float currEnergy = GetEnergy(i, j, false);

			//lack of energy - kill cell
			if( currEnergy < m_minGrowEnergy ) SetCellAlive(i, j, false);
			else 
			{	//if dead - born new cell
				SetCellAlive(i ,j, true);

				//grow neighbours
				if( i%2 != 0 && j%2 != 0 && currEnergy > m_minSeedEnergy ) //80.0f )
				{
					for(Int32 n=0; n<9; n++)
					{
						Int32 x = (n % 3) - 1;
						Int32 y = (Int32)(n/3) - 1;

						//clean highest neighbours to avoid large tree clusters
						if( x != 0 && y != 0 )
						{
							Float nE = GetEnergy(x+i, y+j, false);
							Float setE = nE + GEngine->GetRandomNumberGenerator().Get< Float >( 2.5f , 5.0f );

							if( nE < 75.0f && setE < m_minSeedEnergy ) SetEnergy( x+i, y+j, setE );
							
							//else
								//if( nE < 10.0f ) SetEnergy( x+i, y+j, nE + FRand( 5.0f, 25.0f ) );
						}
					}
				}
			}
		}
}

void CEdVegetationGenerator::IterateCA(int cellsNeedToStayAlive, int cellsNeedToBeBorn)
{
	for(Uint32 i=1; i<mCellSizeX-1; i++)
		for(Uint32 j=1; j<mCellSizeY-1; j++)
		{			

			Uint32 aliveN = GetNumNeighboursAlive(i, j);
			bool isCurrCellAlive = IsCellAlive(i, j);
						
			Uint32 itFloor = cellsNeedToStayAlive % 10;

			while( itFloor != 0 )
			{
				if( isCurrCellAlive )
				{
					if (aliveN != itFloor ) SetCellAlive(i, j, false);
				}
				else 
					if( aliveN == itFloor ) SetCellAlive(i, j, true);

				if( cellsNeedToStayAlive >= 10 )
				{
					cellsNeedToStayAlive /= 10;
					itFloor = cellsNeedToStayAlive % 10;
				}
				else
					itFloor = 0;
			}

			itFloor = cellsNeedToBeBorn % 10;

			while( itFloor != 0 )
			{
				if( !isCurrCellAlive )
				{
					if (aliveN == itFloor ) SetCellAlive(i, j, true);
				}				

				if( cellsNeedToBeBorn >= 10 )
				{
					cellsNeedToBeBorn /= 10;
					itFloor = cellsNeedToBeBorn % 10;
				}
				else
					itFloor = 0;
			}
		}
}

Uint32 CEdVegetationGenerator::GetNumNeighboursAlive(int x, int y)
{
	int retVal = 0;
	
		retVal =	
			m_CAMap[x-1][y+1]+		m_CAMap[x][y+1]+		m_CAMap[x+1][y+1]+ 
			m_CAMap[x-1][y]+			0.0+				m_CAMap[x+1][y]+
			m_CAMap[x-1][y-1]+		m_CAMap[x][y-1]+		m_CAMap[x+1][y-1];

	return retVal;
}


Float CEdVegetationGenerator::GetNeighboursSlopeV(int x, int y)
{
	Float retVal = 0.0f;

		retVal = (180.f/3.14f)*atan( (HMap[x][y+1].Z-HMap[x][y-1].Z)/GetCellSize() );
	
	return retVal;
}

Float CEdVegetationGenerator::GetNeighboursSlopeH(int x, int y)
{
	Float retVal = 0.0f;
	
	retVal = (180.f/3.14f)*atan( (HMap[x-1][y].Z-HMap[x+1][y].Z)/GetCellSize() );
	
	return retVal;
}
void CEdVegetationGenerator::IterateHmap(void)
{
	//no borders included
	for(Uint32 i=1; i<mCellSizeX-1; i++)
		for(Uint32 j=1; j<mCellSizeY-1; j++)
		{
			//kill cells at slope limit
			Float slopeH = GetNeighboursSlopeH(i, j);
			Float slopeV = GetNeighboursSlopeV(i, j);
			if( IsCellAlive(i, j) )
				if ( ( Abs( slopeH ) > m_maxSlopeLimit || Abs( slopeV ) > m_maxSlopeLimit ) || ( Abs( slopeH ) < m_minSlopeLimit || Abs( slopeV ) < m_minSlopeLimit ) ) SetCellAlive(i, j, false);
		}
}

CAreaComponent* CEdVegetationGenerator::GetCellGrowthArea(void)
{
	if( m_cellGrowthArea ) return m_cellGrowthArea;
	else
		return NULL;
}

Float CEdVegetationGenerator::GetEnergy(Int32 x, Int32 y, bool getSunEnergyOnly)
{	
	Float totalEnergy = m_SEMap[x][y];

	if( !getSunEnergyOnly ) 
	{ 
		totalEnergy *= m_sunEnergy;
		totalEnergy += m_WMap[x][y];

		if( totalEnergy > 99.9f ) totalEnergy = 99.9f;

		if( totalEnergy > m_minGrowEnergy ) return totalEnergy;	
		else
			return -1.0f;
	}
	else
		return totalEnergy;
}

void CEdVegetationGenerator::SetEnergy(Int32 x, Int32 y, Float energy)
{		
	m_WMap[x][y] = energy;	
	if( energy > m_minGrowEnergy ) SetCellAlive(x, y, true);
	else
		SetCellAlive(x, y, false);
}

bool CEdVegetationGenerator::SetEnergyScopeByType( Int32 type, Float scopeMul )
{
	bool ret = false;
	for(Uint32 i=0; i<entryList.Size(); i++)
	{
		if( type == entryList[i]->m_type ) 
		{
			//entryList[i]->energyLevelScope = scopeMul*entryList[i]->energyLevelScopeMax;
			ret = true;
		}
	}	
	return ret;
}
void CEdVegetationGenerator::IterateSEMap(void)
{
	//no borders included
	for(Uint32 i=1; i<mCellSizeX-1; i++)
		for(Uint32 j=1; j<mCellSizeY-1; j++)
		{
			//angle between surface and sun at midday
			Vector3 s;

			s.Y = HMap[i+1][j].Z - HMap[i-1][j].Z;
			s.X = HMap[i][j+1].Z - HMap[i][j-1].Z;

			s.Z = 0.8 * GetCellSize(); //m_cellSEColorPower 

			s.Normalize();

			Float dProd = Vector::Dot3( s, m_sunDirectionAtMidday );
			m_SEMap[i][j] = 0.5f*(dProd + 1.0f);
		}
}

void CEdVegetationGenerator::IterateWMap( Int32 steps )
{	
	for(Int32 r=0; r<steps; r++)
	{	
		//no borders included, Neumann neighbours + center
		for(Uint32 i=1; i<mCellSizeX-1; i++)
			for(Uint32 j=1; j<mCellSizeY-1; j++)
			{	
				Float localMinZ = 1000000.0f;
				Float localMaxZ = -1000000.0f;

				Int32 minX = 0;			
				Int32 minY = 0;

				Int32 maxX = 0;
				Int32 maxY = 0;

				Float baseR = 1.0f;
				Float skipCells = 1.0f;

				if( m_distributionSpeed > 0.15f ) baseR = 10.0f*m_distributionSpeed + m_numIterations;

				if( m_numIterations*m_distributionSpeed > 1.0f )
				{					
					skipCells = (Uint32)(m_numIterations*m_distributionSpeed);
				}

				for(Int32 r=-baseR; r<baseR; r += skipCells)
				{
					Uint32 x = i + r;
					Uint32 y = j + r;

					if( x >= 0 && y >= 0 && x<mCellSizeX && y<mCellSizeY )
					{	
						if( localMinZ > HMap[x][y].Z )
						{
							localMinZ = HMap[x][y].Z;
							minX = x;
							minY = y;
						}

						if( localMaxZ < HMap[x][y].Z )
						{
							localMaxZ = HMap[x][y].Z;
							maxX = x;
							maxY = y;
						}
					}							
				}	
				
				//angle based water flow
				Float z = localMaxZ - localMinZ;
				Float c = 2.0f*GetCellSize();

				Float diff = (1.0f - z/( sqrt(z*z + c*c) ) ) *m_WMap[maxX][maxY];				

				if( diff > 0.0f )
				{					
					if( maxX >= 0 && maxY >= 0 && maxX < (Int32)mCellSizeX && maxY < (Int32)mCellSizeY && ( (maxY != minY || maxX != minX) ) )
					{			
						m_WMap[maxX][maxY] -= 0.25f*diff;
						if( m_WMap[maxX][maxY] > m_energyDiff ) m_WMap[minX][minY] += 0.25f*diff;

						if( m_WMap[maxX][maxY] < m_energyDiff ) m_WMap[maxX][maxY] = m_energyDiff;
						if( m_WMap[minX][minY] > 150.0f ) m_WMap[minX][minY] = 150.0f;					
					}
				}
			}
	}
}

void CEdVegetationGenerator::SortByVolume(void)
{
	qsort( entryList.TypedData(), entryList.Size(), sizeof( CEdVegetationGeneratorEntry* ), &CompareByVolume);	
}

void CEdVegetationGenerator::SortByHeight(void)
{
	qsort( entryList.TypedData(), entryList.Size(), sizeof( CEdVegetationGeneratorEntry* ), &CompareByHeight);	
}
void CEdVegetationGenerator::AddEntry( Float height, Float volume, String name, Int32 species, Int32 index, wxStaticBitmap* bitmap )
{
	CEdVegetationGeneratorEntry* vge = new CEdVegetationGeneratorEntry( height, volume, name, species, index, bitmap );
	if(vge) entryList.PushBack(vge);	
}

void CEdVegetationGenerator::ClearAllEntries( void )
{
	if( !entryList.Empty() ) entryList.ClearPtr();
}

Int32 CEdVegetationGenerator::GetResourceIndex( Int32 x, Int32 y, Vector3 &scale )
{
	Float currEnergy = GetEnergy(x, y, false);
	TDynArray<Float> foundResScale;
	TDynArray<Int32> foundRes;

	Int32 ret = -1;

	for(Int32 i=0; i<entryList.SizeInt(); i++)
	{	
		Float maxE = entryList[i]->energyLevelScopeMax;
		Float minE = entryList[i]->energyLevelScopeMin;
		
		Float avgE = 0.5f*(maxE - minE) + minE;
		Float distFromAvg = Abs( avgE - currEnergy );			
		
		Bool placePlant = false;

		if( currEnergy >= minE && currEnergy <= maxE )
		{
			Float distanceProb = GEngine->GetRandomNumberGenerator().Get< Float >( distFromAvg/(avgE-minE) );

			//further away from avg energy value - less probable to spawn with smaller clustering

			//if( FRand(distFromAvg/(avgE-minE), 1.0f) < entryList[i]->m_spawnEntryClusters )
			if( distanceProb < entryList[i]->m_spawnEntryClusters )
			{
				if(GEngine->GetRandomNumberGenerator().Get< Float >() < entryList[i]->m_spawnDensityRand)
					placePlant = true;				
			}		
		}				

		if(placePlant)
		{
			Float s = (currEnergy - minE)/(maxE - minE);
			s = entryList[i]->scaleMin + s*(entryList[i]->scaleMax - entryList[i]->scaleMin);	

			foundResScale.PushBack( s );
			foundRes.PushBack( entryList[i]->defIndex );			
		}
	}

	//get random plant suitable for growing
	if( !foundRes.Empty() ) 
	{		
		Uint32 rnd = GEngine->GetRandomNumberGenerator().Get< Uint32 >( foundRes.Size() );
		
		ret = foundRes[ rnd ];		

		scale.X = foundResScale[ rnd ];
		scale.Y = foundResScale[ rnd ];
		scale.Z = foundResScale[ rnd ];

		foundRes.Clear();
		foundResScale.Clear();
	}
	return ret;	
}

Int32 CEdVegetationGenerator::GetResourceIndex( Int32 type )
{	
	for(Int32 i=0; i<entryList.SizeInt(); i++)
	{
		if( GetType(i) == type ) return entryList[i]->defIndex;
	}
	return -1;
}

Int32 CEdVegetationGenerator::GetType( Int32 i )
{	
	return entryList[i]->m_type;
}

void CEdVegetationGenerator::SetType( Int32 index, Int32 type )
{
	entryList[index]->SetType(type);
	entryList[index]->SetLevels();
}

void CEdVegetationGenerator::SetEntryDensity( Int32 index, Float density, Float clusters )
{
	entryList[index]->m_spawnDensityRand = density;
	entryList[index]->m_spawnEntryClusters = clusters;
}

void CEdVegetationGenerator::SetEntryScope( Int32 index, Float maxE, Float minE )
{	
	entryList[index]->energyLevelScopeMax = maxE;
	entryList[index]->energyLevelScopeMin = minE;	

	if( minE >= FG_TYPE_GRASS_MIN && maxE < FG_TYPE_GRASS_MAX ) 
	{
		entryList[index]->SetType(-1);
	}
	else if( minE >= FG_TYPE_LOWBUSH_MIN && maxE < FG_TYPE_LOWBUSH_MAX )
	{
		entryList[index]->SetType(0);		
	}
	else if( minE >= FG_TYPE_BUSH_MIN && maxE < FG_TYPE_BUSH_MAX )
	{
		entryList[index]->SetType(1);		
	}
	else if( minE >= FG_TYPE_LOWTREE_MIN && maxE < FG_TYPE_LOWTREE_MAX )
	{
		entryList[index]->SetType(2);		
	}
	else if( minE >= FG_TYPE_TREE_MIN && maxE < FG_TYPE_TREE_MAX )
	{
		entryList[index]->SetType(3);		
	}
}

Float CEdVegetationGenerator::GetEntryScopeMax( Int32 index )
{
	return entryList[index]->energyLevelScopeMax;
}

Float CEdVegetationGenerator::GetEntryScopeMin( Int32 index )
{
	return entryList[index]->energyLevelScopeMin;
}

Float CEdVegetationGenerator::GetEntryDensity( Int32 index )
{
	return entryList[index]->m_spawnDensityRand;
}

Float CEdVegetationGenerator::GetEntryClusters( Int32 index )
{
	return entryList[index]->m_spawnEntryClusters;
}

CEdVegetationGenerator::~CEdVegetationGenerator()
{
	ResetIterations( true );		
}

CEdVegetationGeneratorEntry::CEdVegetationGeneratorEntry( Float height, Float volume, String name, Int32 species, Int32 index, wxStaticBitmap* bitmap )
{	
	baseHeight = height;
	m_name = name;
	volumeCapacity = volume;	
	defIndex = index;
	wBitmap = bitmap;
	m_species = species;
	m_spawnDensityRand = 0.2f;
	m_spawnEntryClusters = 0.75f;

	SetEnergyLevels();
}

void CEdVegetationGeneratorEntry::SetEnergyLevels( void )
{
	if( baseHeight < 1.0f )				//-1 - grass
	{				
		SetType(-1);
	}
	else if( baseHeight < 1.7f )		//0 - low bush
	{				
		SetType(0);
	}
	else if( baseHeight < 2.7f )		//1 - bush
	{
		SetType(1);
	}
	else if( baseHeight < 5.0f )		//2 - low tree
	{
		SetType(2);
	}
	else								//3 - tree
	{
		SetType(3);
	}

	SetLevels();
}

void CEdVegetationGeneratorEntry::SetLevels( void )
{
	if( m_type == -1 ) 
	{		
		energyLevelScopeMax = FG_TYPE_GRASS_MAX;
		energyLevelScopeMin = FG_TYPE_GRASS_MIN;		
	}
	else if( m_type == 0 ) 
	{		
		energyLevelScopeMax = FG_TYPE_LOWBUSH_MAX;
		energyLevelScopeMin = FG_TYPE_LOWBUSH_MIN;		
	}
	else if( m_type == 1 ) 
	{		
		energyLevelScopeMax = FG_TYPE_BUSH_MAX;
		energyLevelScopeMin = FG_TYPE_BUSH_MIN;		
	}
	else if( m_type == 2 )
	{		
		energyLevelScopeMax = FG_TYPE_LOWTREE_MAX;
		energyLevelScopeMin = FG_TYPE_LOWTREE_MIN;		
	}
	else if( m_type == 3 )
	{		
		energyLevelScopeMax = FG_TYPE_TREE_MAX;
		energyLevelScopeMin = FG_TYPE_TREE_MIN;		
	}
}

void CEdVegetationGeneratorEntry::SetType( Int32 type )
{
	m_type = type;
	
	if( type == -1 ) 
	{
		scaleMin = 0.65f;
		scaleMax = 2.5f;		
	}

	if( type == 0 ) 
	{
		scaleMin = 0.7f;
		scaleMax = 1.8f;			
	}

	if( type == 1 ) 
	{
		scaleMin = 0.8f;
		scaleMax = 1.4f;		
	}

	if( type == 2 )
	{
		scaleMin = 0.7f;
		scaleMax = 1.4f;				
	}

	if( type == 3 )
	{
		scaleMin = 0.7f;
		scaleMax = 1.3f;				
	}
}

CEdVegetationGeneratorEntry::~CEdVegetationGeneratorEntry()
{		
	wBitmap = NULL;
}