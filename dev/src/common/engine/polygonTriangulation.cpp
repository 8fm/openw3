#include "build.h"

#include <list>
#include <algorithm>
#include <set>

using namespace std;

#include "polygonTriangulation.h"

#define TPPL_VERTEXTYPE_REGULAR 0
#define TPPL_VERTEXTYPE_START	1
#define TPPL_VERTEXTYPE_END		2
#define TPPL_VERTEXTYPE_SPLIT	3
#define TPPL_VERTEXTYPE_MERGE	4

#define TPPL_CCW 1
#define TPPL_CW -1

CPartitionPoly::CPartitionPoly()
{ 
	hole = false;
	numpoints = 0;
	points = NULL;
}

CPartitionPoly::~CPartitionPoly()
{
	if (NULL != points)
	{
		delete [] points;
	}
}

void CPartitionPoly::Clear()
{
	if (NULL != points)
	{
		delete [] points;
	}

	hole = false;
	numpoints = 0;
	points = NULL;
}

void CPartitionPoly::Init(Int32 numpoints)
{
	Clear();

	this->numpoints = numpoints;
	points = new CPartitionVertex[numpoints];
}

void CPartitionPoly::Triangle(CPartitionVertex &p1, CPartitionVertex &p2, CPartitionVertex &p3)
{
	Init(3);
	points[0] = p1;
	points[1] = p2;
	points[2] = p3;
}

CPartitionPoly::CPartitionPoly(const CPartitionPoly &src)
{
	hole = src.hole;
	numpoints = src.numpoints;
	points = new CPartitionVertex[numpoints];
	memcpy(points, src.points, numpoints*sizeof(CPartitionVertex));
}

CPartitionPoly& CPartitionPoly::operator=(const CPartitionPoly &src)
{
	Clear();
	hole = src.hole;
	numpoints = src.numpoints;
	points = new CPartitionVertex[numpoints];
	memcpy(points, src.points, numpoints*sizeof(CPartitionVertex));
	return *this;
}

Int32 CPartitionPoly::GetOrientation()
{
	Int32 i1,i2;
	TPartitionFloat area = 0;
	for(i1=0; i1<numpoints; i1++)
	{
		i2 = i1+1;
		if (i2 == numpoints)
		{
			i2 = 0;
		}

		area += points[i1].x * points[i2].y - points[i1].y * points[i2].x;
	}

	if(area>0) return TPPL_CCW;
	if(area<0) return TPPL_CW;

	return 0;
}

void CPartitionPoly::SetOrientation(Int32 orientation)
{
	Int32 polyorientation = GetOrientation();
	if (polyorientation&&(polyorientation!=orientation))
	{
		Invert();
	}
}

void CPartitionPoly::Invert()
{
	Int32 i;
	CPartitionVertex *invpoints;

	invpoints = new CPartitionVertex[numpoints];
	for (i=0;i<numpoints;i++)
	{
		invpoints[i] = points[numpoints-i-1];
	}

	delete [] points;
	points = invpoints;
}

CPartitionVertex CPartitionEngine::Normalize(const CPartitionVertex &p)
{
	CPartitionVertex r;
	TPartitionFloat n = sqrt(p.x*p.x + p.y*p.y);

	if (n!=0)
	{
		r = p/n;
	}
	else
	{
		r.x = 0;
		r.y = 0;
	}

	return r;
}

TPartitionFloat CPartitionEngine::Distance(const CPartitionVertex &p1, const CPartitionVertex &p2)
{
	TPartitionFloat dx,dy;
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	return(sqrt(dx*dx + dy*dy));
}

Int32 CPartitionEngine::Intersects(CPartitionVertex &p11, CPartitionVertex &p12, CPartitionVertex &p21, CPartitionVertex &p22)
{
	if ((p11.x == p21.x)&&(p11.y == p21.y)) return 0;
	if ((p11.x == p22.x)&&(p11.y == p22.y)) return 0;
	if ((p12.x == p21.x)&&(p12.y == p21.y)) return 0;
	if ((p12.x == p22.x)&&(p12.y == p22.y)) return 0;

	CPartitionVertex v1ort,v2ort,v;
	TPartitionFloat dot11,dot12,dot21,dot22;

	v1ort.x = p12.y-p11.y;
	v1ort.y = p11.x-p12.x;

	v2ort.x = p22.y-p21.y;
	v2ort.y = p21.x-p22.x;

	v = p21-p11;
	dot21 = v.x*v1ort.x + v.y*v1ort.y;
	v = p22-p11;
	dot22 = v.x*v1ort.x + v.y*v1ort.y;

	v = p11-p21;
	dot11 = v.x*v2ort.x + v.y*v2ort.y;
	v = p12-p21;
	dot12 = v.x*v2ort.x + v.y*v2ort.y;

	if (dot11*dot12 > 0) return 0;
	if (dot21*dot22 > 0) return 0;

	return 1;
}

bool CPartitionEngine::IsConvex(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3)
{
	TPartitionFloat tmp;
	tmp = (p3.y-p1.y)*(p2.x-p1.x)-(p3.x-p1.x)*(p2.y-p1.y);
	if(tmp>0) return 1;
	else return 0;
}

bool CPartitionEngine::IsReflex(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3)
{
	TPartitionFloat tmp;
	tmp = (p3.y-p1.y)*(p2.x-p1.x)-(p3.x-p1.x)*(p2.y-p1.y);
	if(tmp<0) return 1;
	else return 0;
}

bool CPartitionEngine::IsInside(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3, CPartitionVertex &p)
{
	if(IsConvex(p1,p,p2)) return false;
	if(IsConvex(p2,p,p3)) return false;
	if(IsConvex(p3,p,p1)) return false;
	return true;
}

bool CPartitionEngine::InCone(CPartitionVertex &p1, CPartitionVertex &p2, CPartitionVertex &p3, CPartitionVertex &p)
{
	bool convex;

	convex = IsConvex(p1,p2,p3);

	if (convex)
	{
		if(!IsConvex(p1,p2,p)) return false;
		if(!IsConvex(p2,p3,p)) return false;
		return true;
	}
	else
	{
		if(IsConvex(p1,p2,p)) return true;
		if(IsConvex(p2,p3,p)) return true;
		return false;
	}
}

bool CPartitionEngine::InCone(PartitionVertex *v, CPartitionVertex &p)
{
	CPartitionVertex p1,p2,p3;

	p1 = v->previous->p;
	p2 = v->p;
	p3 = v->next->p;

	return InCone(p1,p2,p3,p);
}

void CPartitionEngine::UpdateVertexReflexity(PartitionVertex *v)
{
	PartitionVertex *v1,*v3;
	v1 = v->previous;
	v3 = v->next;
	v->isConvex = !IsReflex(v1->p,v->p,v3->p);	
}

void CPartitionEngine::UpdateVertex(PartitionVertex *v, PartitionVertex *vertices, Int32 numvertices)
{
	Int32 i;
	PartitionVertex *v1,*v3;
	CPartitionVertex vec1,vec3;

	v1 = v->previous;
	v3 = v->next;

	v->isConvex = IsConvex(v1->p,v->p,v3->p);

	vec1 = Normalize(v1->p - v->p);
	vec3 = Normalize(v3->p - v->p);
	v->angle = vec1.x*vec3.x + vec1.y*vec3.y;

	if (v->isConvex)
	{
		v->isEar = true;
		for (i=0;i<numvertices;i++)
		{
			if ((vertices[i].p.x==v->p.x)&&(vertices[i].p.y==v->p.y)) continue;
			if ((vertices[i].p.x==v1->p.x)&&(vertices[i].p.y==v1->p.y)) continue;
			if ((vertices[i].p.x==v3->p.x)&&(vertices[i].p.y==v3->p.y)) continue;

			if (IsInside(v1->p,v->p,v3->p,vertices[i].p))
			{
				v->isEar = false;
				break;
			}
		}
	}
	else
	{
		v->isEar = false;
	}
}

Int32 CPartitionEngine::Triangulate(CPartitionPoly *poly, list<CPartitionPoly> *triangles)
{
	Int32 i,j,k,gap,n;
	DPState **dpstates;
	CPartitionVertex p1,p2,p3,p4;
	Int32 bestvertex;
	TPartitionFloat weight,minweight=0,d1,d2;
	Diagonal diagonal,newdiagonal;
	list<Diagonal> diagonals;
	CPartitionPoly triangle;
	Int32 ret = 1;

	n = poly->GetNumPoints();
	dpstates = new DPState *[n];
	for(i=1;i<n;i++) {
		dpstates[i] = new DPState[i];
	}

	//init states and visibility
	for(i=0;i<(n-1);i++)
	{
		p1 = poly->GetPoint(i);
		for(j=i+1;j<n;j++)
		{
			dpstates[j][i].visible = true;
			dpstates[j][i].weight = 0;
			dpstates[j][i].bestvertex = -1;

			if(j!=(i+1))
			{
				p2 = poly->GetPoint(j);
				
				//visibility check
				if (i==0) 
				{
					p3 = poly->GetPoint(n-1);
				}
				else 
				{
					p3 = poly->GetPoint(i-1);
				}

				if (i==(n-1)) 
				{
					p4 = poly->GetPoint(0);
				}
				else
				{
					p4 = poly->GetPoint(i+1);
				}

				if (!InCone(p3,p1,p4,p2))
				{
					dpstates[j][i].visible = false;
					continue;
				}

				if (j==0) 
				{
					p3 = poly->GetPoint(n-1);
				}
				else
				{
					p3 = poly->GetPoint(j-1);
				}

				if (j==(n-1)) 
				{
					p4 = poly->GetPoint(0);
				}
				else
				{
					p4 = poly->GetPoint(j+1);
				}

				if (!InCone(p3,p2,p4,p1))
				{
					dpstates[j][i].visible = false;
					continue;
				}

				for (k=0;k<n;k++)
				{
					p3 = poly->GetPoint(k);
					if (k==(n-1)) 
					{
						p4 = poly->GetPoint(0);
					}
					else
					{
						p4 = poly->GetPoint(k+1);
					}

					if (Intersects(p1,p2,p3,p4))
					{
						dpstates[j][i].visible = false;
						break;
					}
				}
			}
		}
	}

	dpstates[n-1][0].visible = true;
	dpstates[n-1][0].weight = 0;
	dpstates[n-1][0].bestvertex = -1;

	for (gap = 2; gap<n; gap++)
	{
		for (i=0; i<(n-gap); i++)
		{
			j = i+gap;
			if (!dpstates[j][i].visible) 
			{
				continue; 
			}

			bestvertex = -1;
			for (k=(i+1);k<j;k++)
			{
				if (!dpstates[k][i].visible) continue; 
				if (!dpstates[j][k].visible) continue;

				if (k<=(i+1)) 
				{
					d1=0;
				}
				else 
				{
					d1 = Distance(poly->GetPoint(i),poly->GetPoint(k));
				}

				if (j<=(k+1)) 
				{
					d2=0;
				}
				else 
				{
					d2 = Distance(poly->GetPoint(k),poly->GetPoint(j));
				}

				weight = dpstates[k][i].weight + dpstates[j][k].weight + d1 + d2;

				if ((bestvertex == -1)||(weight<minweight))
				{
					bestvertex = k;
					minweight = weight;
				}
			}

			if (bestvertex == -1)
			{
				for(i=1;i<n;i++) {
					delete [] dpstates[i];
				}
				delete [] dpstates;
				return 0;
			}
			
			dpstates[j][i].bestvertex = bestvertex;
			dpstates[j][i].weight = minweight;
		}
	}

	newdiagonal.index1 = 0;
	newdiagonal.index2 = n-1;
	diagonals.push_back(newdiagonal);
	while (!diagonals.empty())
	{
		diagonal = *(diagonals.begin());
		diagonals.pop_front();
		bestvertex = dpstates[diagonal.index2][diagonal.index1].bestvertex;
		if (bestvertex == -1)
		{
			ret = 0;		
			break;
		}

		triangle.Triangle(poly->GetPoint(diagonal.index1),poly->GetPoint(bestvertex),poly->GetPoint(diagonal.index2));
		triangles->push_back(triangle);

		if (bestvertex > (diagonal.index1+1))
		{
			newdiagonal.index1 = diagonal.index1;
			newdiagonal.index2 = bestvertex;
			diagonals.push_back(newdiagonal);
		}

		if (diagonal.index2 > (bestvertex+1))
		{
			newdiagonal.index1 = bestvertex;
			newdiagonal.index2 = diagonal.index2;
			diagonals.push_back(newdiagonal);
		}
	}

	for (i=1;i<n;i++)
	{
		delete [] dpstates[i];
	}

	delete [] dpstates;
	return ret;
}

void CPartitionEngine::UpdateState(Int32 a, Int32 b, Int32 w, Int32 i, Int32 j, DPState2 **dpstates)
{
	Diagonal newdiagonal;
	list<Diagonal> *pairs;
	Int32 w2;

	w2 = dpstates[a][b].weight;
	if (w>w2) 
	{
		return;
	}
	
	pairs = &(dpstates[a][b].pairs);
	newdiagonal.index1 = i;
	newdiagonal.index2 = j;

	if (w<w2)
	{
		pairs->clear();
		pairs->push_front(newdiagonal);
		dpstates[a][b].weight = w;
	}
	else
	{
		if ((!pairs->empty())&&(i <= pairs->begin()->index1))
		{
			return;
		}

		while ((!pairs->empty())&&(pairs->begin()->index2 >= j)) 
		{
			pairs->pop_front();
		}

		pairs->push_front(newdiagonal);
	}
}

void CPartitionEngine::TypeA(Int32 i, Int32 j, Int32 k, PartitionVertex *vertices, DPState2 **dpstates)
{
	list<Diagonal> *pairs;
	list<Diagonal>::iterator iter,lastiter;
	Int32 top;
	Int32 w;

	if (!dpstates[i][j].visible)
	{
		return;
	}

	top = j;
	w = dpstates[i][j].weight;
    if (k-j > 1)
	{
		if (!dpstates[j][k].visible)
		{
			return;
		}
		w += dpstates[j][k].weight + 1;
    }

	if (j-i > 1)
	{
		pairs = &(dpstates[i][j].pairs);
		iter = pairs->end();
		lastiter = pairs->end();
		while (iter!=pairs->begin())
		{
			iter--;
			if (!IsReflex(vertices[iter->index2].p,vertices[j].p,vertices[k].p))
			{
				lastiter = iter;
			}
			else
			{
				break;
			}
		}

		if (lastiter == pairs->end()) 
		{
			w++;
		}
		else
		{
			if (IsReflex(vertices[k].p,vertices[i].p,vertices[lastiter->index1].p))
			{
				w++;
			}
			else
			{
				top = lastiter->index1;
			}
		}
	}

	UpdateState(i,k,w,top,j,dpstates);
}

void CPartitionEngine::TypeB(Int32 i, Int32 j, Int32 k, PartitionVertex *vertices, DPState2 **dpstates)
{
	list<Diagonal> *pairs;
	list<Diagonal>::iterator iter,lastiter;
	Int32 top;
	Int32 w;

	if (!dpstates[j][k].visible)
	{
		return;
	}

	top = j;
	w = dpstates[j][k].weight;
	
	if (j-i > 1)
	{
		if (!dpstates[i][j].visible)
		{
			return;
		}

		w += dpstates[i][j].weight + 1;
	}

	if (k-j > 1)
	{
		pairs = &(dpstates[j][k].pairs);

		iter = pairs->begin();
		if ((!pairs->empty())&&(!IsReflex(vertices[i].p,vertices[j].p,vertices[iter->index1].p)))
		{
			lastiter = iter;
			while (iter!=pairs->end())
			{
				if (!IsReflex(vertices[i].p,vertices[j].p,vertices[iter->index1].p))
				{
					lastiter = iter;
					iter++;
				}
				else
				{
					break;
				}
			}

			if (IsReflex(vertices[lastiter->index2].p,vertices[k].p,vertices[i].p))
			{
				w++;
			}
			else
			{
				top = lastiter->index2;
			}
		}
		else
		{
			w++;
		}
	}

	UpdateState(i,k,w,j,top,dpstates);
}

Int32 CPartitionEngine::ConvexPartition_Optimal(CPartitionPoly *poly, list<CPartitionPoly> *parts)
{
	CPartitionVertex p1,p2,p3,p4;
	PartitionVertex *vertices;
	DPState2 **dpstates;
	Int32 i,j,k,n,gap;
	list<Diagonal> diagonals,diagonals2;
	Diagonal diagonal,newdiagonal;
	list<Diagonal> *pairs,*pairs2;
	list<Diagonal>::iterator iter,iter2;
	Int32 ret;
	CPartitionPoly newpoly;
	list<Int32> indices;
	list<Int32>::iterator iiter;
	bool ijreal,jkreal;

	n = poly->GetNumPoints();
	vertices = new PartitionVertex[n];

	dpstates = new DPState2 *[n];
	for (i=0;i<n;i++)
	{
		dpstates[i] = new DPState2[n];
	}

	//init vertex information
	for (i=0;i<n;i++)
	{
		vertices[i].p = poly->GetPoint(i);
		vertices[i].isActive = true;

		if (i==0)
		{
			vertices[i].previous = &(vertices[n-1]);
		}
		else
		{
			vertices[i].previous = &(vertices[i-1]);
		}

		if (i==(poly->GetNumPoints()-1))
		{
			vertices[i].next = &(vertices[0]);
		}
		else
		{
			vertices[i].next = &(vertices[i+1]);
		}
	}

	for (i=1;i<n;i++)
	{
		UpdateVertexReflexity(&(vertices[i]));
	}

	//init states and visibility
	for (i=0;i<(n-1);i++)
	{
		p1 = poly->GetPoint(i);
		for (j=i+1;j<n;j++)
		{
			dpstates[i][j].visible = true;
			if (j==i+1)
			{
				dpstates[i][j].weight = 0;
			}
			else
			{
				dpstates[i][j].weight = 2147483647;
			}

			if (j!=(i+1))
			{
				p2 = poly->GetPoint(j);
				
				if (!InCone(&vertices[i],p2))
				{
					dpstates[i][j].visible = false;
					continue;
				}

				if(!InCone(&vertices[j],p1))
				{
					dpstates[i][j].visible = false;
					continue;
				}

				for(k=0;k<n;k++)
				{
					p3 = poly->GetPoint(k);
					if (k==(n-1))
					{
						p4 = poly->GetPoint(0);
					}
					else
					{
						p4 = poly->GetPoint(k+1);
					}

					if (Intersects(p1,p2,p3,p4))
					{
						dpstates[i][j].visible = false;
						break;
					}
				}
			}
		}
	}

	for (i=0;i<(n-2);i++)
	{
		j = i+2;

		if (dpstates[i][j].visible)
		{
			dpstates[i][j].weight = 0;
			newdiagonal.index1 = i+1;
			newdiagonal.index2 = i+1;
			dpstates[i][j].pairs.push_back(newdiagonal);
		}
	}

	dpstates[0][n-1].visible = true;
	vertices[0].isConvex = false; //by convention

	for (gap=3; gap<n; gap++)
	{
		for (i=0;i<n-gap;i++)
		{
			if (vertices[i].isConvex)
			{
				continue;
			}

			k = i+gap;

			if (dpstates[i][k].visible)
			{
				if (!vertices[k].isConvex)
				{
					for (j=i+1;j<k;j++)					
					{
						TypeA(i,j,k,vertices,dpstates);
					}

				}
				else
				{
					for (j=i+1;j<(k-1);j++)
					{
						if (vertices[j].isConvex)
						{
							continue;				
						}

						TypeA(i,j,k,vertices,dpstates);
					}

					TypeA(i,k-1,k,vertices,dpstates);
				}
			}
		}

		for (k=gap;k<n;k++)
		{
			if (vertices[k].isConvex)
			{
				continue;
			}

			i = k-gap;

			if ((vertices[i].isConvex) && (dpstates[i][k].visible))
			{
				TypeB(i,i+1,k,vertices,dpstates);

				for(j=i+2;j<k;j++)
				{
					if (vertices[j].isConvex)
					{
						continue;				
					}

					TypeB(i,j,k,vertices,dpstates);
				}
			}
		}
	}

	//recover solution
	ret = 1;
	newdiagonal.index1 = 0;
	newdiagonal.index2 = n-1;
	diagonals.push_front(newdiagonal);
	while (!diagonals.empty())
	{
		diagonal = *(diagonals.begin());
		diagonals.pop_front();
		if ((diagonal.index2 - diagonal.index1) <= 1)
		{
			continue;
		}

		pairs = &(dpstates[diagonal.index1][diagonal.index2].pairs);
		if (pairs->empty())
		{
			ret = 0;
			break;
		}

		if (!vertices[diagonal.index1].isConvex)
		{
			iter = pairs->end();
			iter--;
			j = iter->index2;
			newdiagonal.index1 = j;
			newdiagonal.index2 = diagonal.index2;
			diagonals.push_front(newdiagonal);
			if ((j - diagonal.index1)>1)
			{
				if (iter->index1 != iter->index2)
				{
					pairs2 = &(dpstates[diagonal.index1][j].pairs);
					while (1)
					{
						if (pairs2->empty())
						{
							ret = 0;
							break;
						}

						iter2 = pairs2->end();
						iter2--;
						if (iter->index1 != iter2->index1)
						{
							pairs2->pop_back();
						}
						else
						{
							break;
						}
					}

					if (ret == 0)
					{
						break;
					}
				}

				newdiagonal.index1 = diagonal.index1;
				newdiagonal.index2 = j;
				diagonals.push_front(newdiagonal);
			}
		}
		else
		{
			iter = pairs->begin();
			j = iter->index1;
			newdiagonal.index1 = diagonal.index1;
			newdiagonal.index2 = j;
			diagonals.push_front(newdiagonal);

			if ((diagonal.index2 - j) > 1)
			{
				if (iter->index1 != iter->index2)
				{
					pairs2 = &(dpstates[j][diagonal.index2].pairs);
					while (1)
					{
						if (pairs2->empty())
						{
							ret = 0;
							break;
						}

						iter2 = pairs2->begin();
						if (iter->index2 != iter2->index2)
						{
							pairs2->pop_front();
						}
						else
						{
							break;
						}
					}
					if (ret == 0)
					{
						break;
					}
				}
				newdiagonal.index1 = j;
				newdiagonal.index2 = diagonal.index2;
				diagonals.push_front(newdiagonal);
			}
		}
	}

	if (ret == 0)
	{
		for (i=0;i<n;i++)
		{
			delete [] dpstates[i];
		}

		delete [] dpstates;
		delete [] vertices;
		return ret;
	}

	newdiagonal.index1 = 0;
	newdiagonal.index2 = n-1;
	diagonals.push_front(newdiagonal);
	while (!diagonals.empty())
	{
		diagonal = *(diagonals.begin());
		diagonals.pop_front();
		if ((diagonal.index2 - diagonal.index1) <= 1)
		{
			continue;
		}
		
		indices.clear();
		diagonals2.clear();
		indices.push_back(diagonal.index1);
		indices.push_back(diagonal.index2);
		diagonals2.push_front(diagonal);

		while (!diagonals2.empty())
		{
			diagonal = *(diagonals2.begin());
			diagonals2.pop_front();
			if ((diagonal.index2 - diagonal.index1) <= 1)
			{
				continue;
			}

			ijreal = true;
			jkreal = true;
			pairs = &(dpstates[diagonal.index1][diagonal.index2].pairs);
			if (!vertices[diagonal.index1].isConvex)
			{
				iter = pairs->end();
				iter--;
				j = iter->index2;
				if (iter->index1 != iter->index2)
				{
					ijreal = false;
				}
			}
			else
			{
				iter = pairs->begin();
				j = iter->index1;
				if (iter->index1 != iter->index2)
				{
					jkreal = false;
				}
			}

			newdiagonal.index1 = diagonal.index1;
			newdiagonal.index2 = j;
			if (ijreal)
			{
				diagonals.push_back(newdiagonal);
			}
			else
			{
				diagonals2.push_back(newdiagonal);
			}

			newdiagonal.index1 = j;
			newdiagonal.index2 = diagonal.index2;
			if (jkreal)
			{
				diagonals.push_back(newdiagonal);
			}
			else
			{
				diagonals2.push_back(newdiagonal);
			}

			indices.push_back(j);
		}

		indices.sort();
		newpoly.Init((Int32)indices.size());
		k=0;

		for (iiter = indices.begin();iiter!=indices.end();iiter++)
		{
			newpoly[k] = vertices[*iiter].p;
			k++;
		}

		parts->push_back(newpoly);
	}

	for(i=0;i<n;i++)
	{
		delete [] dpstates[i];
	}

	delete [] dpstates;
	delete [] vertices;
	return ret;
}

int CPartitionEngine::ConvexPartition_Forced(CPartitionPoly *poly, list<CPartitionPoly> *parts)
{
	list<CPartitionPoly> triangles;
	list<CPartitionPoly>::iterator iter1,iter2;
	CPartitionPoly *poly1,*poly2;
	CPartitionPoly newpoly;
	CPartitionVertex d1,d2,p1,p2,p3;
	Int32 i11,i12,i21,i22,i13,i23,j,k;
	Bool isdiagonal;
	Int32 numreflex;

	//check if the poly is already convex
	numreflex = 0;
	for (i11=0;i11<poly->GetNumPoints();i11++)
	{
		if (i11==0)
			i12 = poly->GetNumPoints()-1;
		else
			i12=i11-1;

		if (i11==(poly->GetNumPoints()-1))
			i13=0;
		else
			i13=i11+1;

		if (IsReflex(poly->GetPoint(i12),poly->GetPoint(i11),poly->GetPoint(i13)))
		{
			numreflex = 1;
			break;
		}
	}

	if(numreflex == 0)
	{
		parts->push_back(*poly);
		return 1;
	}

	if (!Triangulate(poly,&triangles))
		return 0;

	for(iter1 = triangles.begin(); iter1 != triangles.end(); iter1++)
	{
		poly1 = &(*iter1);

		for (i11=0;i11<poly1->GetNumPoints();i11++)
		{
			d1 = poly1->GetPoint(i11);
			i12 = (i11+1)%(poly1->GetNumPoints());
			d2 = poly1->GetPoint(i12);

			isdiagonal = false;
			for (iter2 = iter1; iter2 != triangles.end(); iter2++)
			{
				if (iter1 == iter2) continue;
				poly2 = &(*iter2);

				for (i21=0;i21<poly2->GetNumPoints();i21++)
				{
					if ((d2.x != poly2->GetPoint(i21).x)||(d2.y != poly2->GetPoint(i21).y))
						continue;

					i22 = (i21+1)%(poly2->GetNumPoints());
					if ((d1.x != poly2->GetPoint(i22).x)||(d1.y != poly2->GetPoint(i22).y))
						continue;

					isdiagonal = true;
					break;
				}

				if (isdiagonal)
					break;
			}

			if (!isdiagonal)
				continue;

			p2 = poly1->GetPoint(i11);
			if (i11 == 0)
				i13 = poly1->GetNumPoints()-1;
			else
				i13 = i11-1;

			p1 = poly1->GetPoint(i13);
			if (i22 == (poly2->GetNumPoints()-1))
				i23 = 0;
			else
				i23 = i22+1;
			p3 = poly2->GetPoint(i23);

			if (!IsConvex(p1,p2,p3))
				continue;

			p2 = poly1->GetPoint(i12);
			if (i12 == (poly1->GetNumPoints()-1))
				i13 = 0;
			else
				i13 = i12+1;

			p3 = poly1->GetPoint(i13);
			if (i21 == 0)
				i23 = poly2->GetNumPoints()-1;
			else
				i23 = i21-1;
			p1 = poly2->GetPoint(i23);

			if (!IsConvex(p1,p2,p3))
				continue;

			newpoly.Init(poly1->GetNumPoints()+poly2->GetNumPoints()-2);
			k = 0;
			for (j=i12;j!=i11;j=(j+1)%(poly1->GetNumPoints()))
			{
				newpoly[k] = poly1->GetPoint(j);
				k++;
			}
			for (j=i22;j!=i21;j=(j+1)%(poly2->GetNumPoints()))
			{
				newpoly[k] = poly2->GetPoint(j);
				k++;
			}

			triangles.erase(iter2);
			*iter1 = newpoly;
			poly1 = &(*iter1);
			i11 = -1;

			continue;
		}
	}

	for (iter1 = triangles.begin(); iter1 != triangles.end(); iter1++)
	{
		parts->push_back(*iter1);
	}

	return 1;
}