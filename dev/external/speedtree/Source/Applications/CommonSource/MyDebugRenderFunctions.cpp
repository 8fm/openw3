///////////////////////////////////////////////////////////////////////
//  MyDebugRenderFunctions.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


#ifdef SPEEDTREE_OPENGL

	///////////////////////////////////////////////////////////////////////
	//  Preprocessor

	#include "MyDebugRenderFunctions.h"
	using namespace SpeedTree;


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderAxes

	void CMyDebugRenderFunctions::RenderAxes(const CView& cView)
	{
		BeginFixedPipelineRender(cView);
		{
			glPushAttrib(GL_LINE_BIT);

			glLineWidth(3.0f);

			const st_float32 c_fScalar = 100.0f;
			const Vec3 c_vUp = CCoordSys::UpAxis( ) * 50.0f;
			const Vec3 c_vCameraPos = cView.GetCameraPos( );

			//glTranslatef(c_vUp.x - c_vCameraPos.x, c_vUp.y - c_vCameraPos.y, c_vUp.z - c_vCameraPos.z);
			glTranslatef(c_vCameraPos.x + c_vUp.x, c_vCameraPos.y + c_vUp.y, c_vCameraPos.z + c_vUp.z);

			// right axis
			glColor3f(1.0f, 0.0f, 0.0f);
			glBegin(GL_LINES);
				glVertex3f(0.0f, 0.0f, 0.0f);
				//glVertex3fv(CCoordSys::RightAxis( ) * c_fScalar);
				glVertex3fv(Vec3(1.0f, 0.0f, 0.0f) * c_fScalar);
			glEnd( );

			// out axis
			//if (CCoordSys::IsYAxisUp( ))
			//	glColor3f(0.0f, 0.0f, 1.0f);
			//else
				glColor3f(0.3f, 1.0f, 0.3f);
			glBegin(GL_LINES);
				glVertex3f(0.0f, 0.0f, 0.0f);
				//glVertex3fv(CCoordSys::OutAxis( ) * c_fScalar);
				glVertex3fv(Vec3(0.0f, 1.0f, 0.0f) * c_fScalar);
			glEnd( );

			// up axis
			//if (CCoordSys::IsYAxisUp( ))
			//	glColor3f(0.3f, 1.0f, 0.3f);
			//else
				glColor3f(0.0f, 0.0f, 1.0f);
			glBegin(GL_LINES);
				glVertex3f(0.0f, 0.0f, 0.0f);
				//glVertex3fv(CCoordSys::UpAxis( ) * c_fScalar);
				glVertex3fv(Vec3(0.0f, 0.0f, 1.0f) * c_fScalar);
			glEnd( );

			glPopAttrib( );
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderFrustum

	void CMyDebugRenderFunctions::RenderFrustum(const CView& cView)
	{
		const Vec3* pFrustumPoints = cView.GetFrustumPoints( );

		BeginFixedPipelineRender(cView);
		{
			glPushAttrib(GL_LINE_BIT);
			glLineWidth(2.0f);
			glColor3f(0.4f, 1.0f, 0.4f);

			// near
			glBegin(GL_LINE_LOOP);
				glVertex3fv(pFrustumPoints[0]);
				glVertex3fv(pFrustumPoints[1]);
				glVertex3fv(pFrustumPoints[2]);
				glVertex3fv(pFrustumPoints[3]);
			glEnd( );

			// far
			glBegin(GL_LINE_LOOP);
				glVertex3fv(pFrustumPoints[4]);
				glVertex3fv(pFrustumPoints[5]);
				glVertex3fv(pFrustumPoints[6]);
				glVertex3fv(pFrustumPoints[7]);
			glEnd( );

			// connect
			glBegin(GL_LINES);
				glVertex3fv(pFrustumPoints[0]);
				glVertex3fv(pFrustumPoints[4]);
				glVertex3fv(pFrustumPoints[1]);
				glVertex3fv(pFrustumPoints[5]);
				glVertex3fv(pFrustumPoints[2]);
				glVertex3fv(pFrustumPoints[6]);
				glVertex3fv(pFrustumPoints[3]);
				glVertex3fv(pFrustumPoints[7]);
			glEnd( );

			glPopAttrib( );
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderTreeBoundingBoxes

	void CMyDebugRenderFunctions::RenderTreeBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
	{
		BeginFixedPipelineRender(cView);
		{
			glColor3f(1.0f, 1.0f, 0.0f);

			const T3dTreeInstLodArray2D& aaInstanceLods = cVisibleInstances.Get3dInstanceLods( );
			for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aaInstanceLods.size( )); ++nBaseTree)
			{
				if (!aaInstanceLods[nBaseTree].empty( ))
				{
					// get extents of base tree
					const CTree* pBaseTree = aaInstanceLods[nBaseTree][0].m_pInstance->InstanceOf( );
					const CExtents& cExtents = pBaseTree->GetExtents( );

					// render each instance
					for (st_int32 nInstance = 0; nInstance < st_int32(aaInstanceLods[nBaseTree].size( )); ++nInstance)
					{
						const S3dTreeInstanceLod& sInstanceLod = aaInstanceLods[nBaseTree][nInstance];

						// modify base tree's extents to compensate for instance's transform
						CExtents cInstanceExtents = cExtents;
						cInstanceExtents.Orient(sInstanceLod.m_pInstance->GetUpVector( ), sInstanceLod.m_pInstance->GetRightVector( ));
						cInstanceExtents.Scale(sInstanceLod.m_pInstance->GetScalar( ));
						cInstanceExtents.Translate(sInstanceLod.m_pInstance->GetPos( ));

						DrawBox(cInstanceExtents.Min( ), cInstanceExtents.Max( ), true);
					}
				}
			}
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderTreeCullSpheres

	void CMyDebugRenderFunctions::RenderTreeCullSpheres(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		BeginFixedPipelineRender(cView);
		{
			glColor3f(1.0f, 1.0f, 0.0f);

			const T3dTreeInstLodArray2D& aaInstanceLods = cVisibleInstances.Get3dInstanceLods( );
			for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aaInstanceLods.size( )); ++nBaseTree)
			{
				if (!aaInstanceLods[nBaseTree].empty( ))
				{
					// render each instance
					for (st_int32 nInstance = 0; nInstance < st_int32(aaInstanceLods[nBaseTree].size( )); ++nInstance)
					{
						const S3dTreeInstanceLod& sInstanceLod = aaInstanceLods[nBaseTree][nInstance];
						const Vec3& vGeometricCenter = sInstanceLod.m_pInstance->GetGeometricCenter( );
						st_float32 fCullingRadius = sInstanceLod.m_pInstance->GetCullingRadius( );

						DrawSphere(vGeometricCenter, fCullingRadius);
					}
				}
			}
		}
		EndFixedPipelineRender( );
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderVisibleCellBoundingBoxes

	void CMyDebugRenderFunctions::RenderVisibleCellBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
	{
		BeginFixedPipelineRender(cView);
		{
			for (st_int32 nCell = 0; nCell < st_int32(cVisibleInstances.VisibleCells( ).size( )); ++nCell)
			{
				const CCell* pCell = cVisibleInstances.VisibleCells( )[nCell];

				// color code based on cull status
				if (pCell->GetCullStatus( ) == CS_FULLY_INSIDE_FRUSTUM)
					glColor3f(0.0f, 1.0f, 0.0f);
				else if (pCell->GetCullStatus( ) == CS_INTERSECTS_FRUSTUM)
					glColor3f(1.0f, 1.0f, 0.0f);
				else
					glColor3f(1.0f, 0.0f, 0.0f);

				DrawBox(pCell->GetExtents( ).Min( ), pCell->GetExtents( ).Max( ), true);
			}
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderRoughCellBoundingBoxes

	void CMyDebugRenderFunctions::RenderRoughCellBoundingBoxes(const CView& cView, CVisibleInstancesRender& cVisibleInstances)
	{
		BeginFixedPipelineRender(cView);
		{
			glColor3f(1.0f, 0.0f, 1.0f);
			for (st_int32 nCell = 0; nCell < st_int32(cVisibleInstances.RoughCells( ).size( )); ++nCell)
			{
				const CCell& cCell = cVisibleInstances.RoughCells( )[nCell];

				DrawBox(cCell.GetExtents( ).Min( ), cCell.GetExtents( ).Max( ), true);
			}
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderCellCullSpheres

	void CMyDebugRenderFunctions::RenderCellCullSpheres(const CView& cView, CVisibleInstancesRender& cVisibleInstances)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		BeginFixedPipelineRender(cView);
		{
			for (st_int32 nCell = 0; nCell < st_int32(cVisibleInstances.VisibleCells( ).size( )); ++nCell)
			{
				const CCell* pCell = cVisibleInstances.VisibleCells( )[nCell];

				DrawSphere(pCell->GetExtents( ).GetCenter( ), pCell->GetExtents( ).ComputeRadiusFromCenter3D( ));
			}

			//for (st_int32 nCell = 0; nCell < st_int32(cVisibleInstances.RoughCells( ).size( )); ++nCell)
			//{
			//	const CCell& cCell = cVisibleInstances.RoughCells( )[nCell];
			//	DrawSphere(cCell.GetExtents( ).GetCenter( ), cCell.GetExtents( ).ComputeRadiusFromCenter3D( ));
			//}
		}
		EndFixedPipelineRender( );
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderGrassAsLines

	void CMyDebugRenderFunctions::RenderGrassAsLines(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
	{
		BeginFixedPipelineRender(cView);
		{
			glColor3f(1.0f, 0.0f, 1.0f);

			glPointSize(5.0f);

			// run through all of the grass cells looking to render only those instances that reference pBaseGrass
			const TCellPtrArray& aCells = cVisibleInstances.VisibleCells( );
			for (st_int32 nCell = 0; nCell < st_int32(aCells.size( )); ++nCell)
			{
				glBegin(GL_POINTS);

				// get array of grass instances for this base tree
				const TGrassInstArray& aInstances = aCells[nCell]->GetGrassInstances( );
				for (st_int32 nInstance = 0; nInstance < st_int32(aInstances.size( )); ++nInstance)
				{
					const CGrassInstance& cInstance = aInstances[nInstance];
					glVertex3fv(cInstance.GetPos( ));
					//glVertex3f(pInstance->GetPos( ).x, pInstance->GetPos( ).y, pInstance->GetPos( ).z + 5.0f);
				}

				glEnd( );
			}
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderTerrainNormals

	void CMyDebugRenderFunctions::RenderTerrainNormals(const CView& cView, const CMyTerrain& cTerrain)
	{
		BeginFixedPipelineRender(cView);
		{
			glPushAttrib(GL_LINE_BIT);

			glLineWidth(3.0f);
	
			const st_float32 c_fRadius = 500.0f;
			const st_float32 c_fSpacing = 10.0f;
			const st_float32 c_fNormalSize = 3.0f;
			const Vec3& c_vCamera = cView.GetCameraPos( );

			glBegin(GL_LINES);
			for (st_float32 x = c_vCamera.x - c_fRadius; x < c_vCamera.x + c_fRadius; x += c_fSpacing)
			{
				for (st_float32 y = c_vCamera.y - c_fRadius; y < c_vCamera.y + c_fRadius; y += c_fSpacing)
				{
					st_float32 z = cTerrain.GetHeightFromXY(x, y);

					Vec3 vNormal = cTerrain.GetNormalFromXY(x, y) * c_fNormalSize;

					glColor3f(0.0f, 0.0f, 1.0f);
					glVertex3f(x, y, z);
					glVertex3f(x + vNormal.x, y + vNormal.y, z + vNormal.z);
				}
			}
			glEnd( );

			glPopAttrib( );
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderDistanceCues

	void CMyDebugRenderFunctions::RenderDistanceCues(const CView& cView, const CMyTerrain& cTerrain)
	{
		const st_int32 c_nNumRings = 5;
		const st_int32 c_nRingRes = 100;
		const st_float32 c_nRingInterval = 100.0f;

		BeginFixedPipelineRender(cView);
		{
			glPushAttrib(GL_LINE_BIT);

			glColor3f(0.0f, 0.0f, 1.0f);
			glLineWidth(3.0f);
	
			const Vec3& c_vCamera = cView.GetCameraPos( );

			for (st_int32 nRing = 0; nRing < c_nNumRings; ++nRing)
			{
				st_float32 c_fRadius = c_nRingInterval * (nRing + 1);

				glBegin(GL_LINE_LOOP);
					for (st_int32 i = 0; i < c_nRingRes; ++i)
					{
						st_float32 c_fAngle = i * (c_fTwoPi / c_nRingRes);

						st_float32 x = c_vCamera.x + c_fRadius * cosf(c_fAngle);
						st_float32 y = c_vCamera.y + c_fRadius * sinf(c_fAngle);
						st_float32 z = cTerrain.GetHeightFromXY(x, y);
						glVertex3f(x, y, z);
					}
				glEnd( );
			}

			glPopAttrib( );
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::RenderBillboardSpikes

	void CMyDebugRenderFunctions::RenderBillboardSpikes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
	{
		BeginFixedPipelineRender(cView);
		{
			glPushAttrib(GL_LINE_BIT);
			glColor3f(1.0f, 0.0f, 1.0f);
			glLineWidth(3.0f);
			{
				glBegin(GL_LINES);
				const TCellPtrArray& aVisibleCells = cVisibleInstances.VisibleCells( );
				for (st_int32 nCell = 0; nCell < st_int32(aVisibleCells.size( )); ++nCell)
				{
					const CCell* pCell = aVisibleCells[nCell];

					const TTreeInstConstPtrArray& aInstances = pCell->GetTreeInstances( );

					for (st_int32 i = 0; i < st_int32(aInstances.size( )); ++i)
					{
						const CTreeInstance* pInstance = aInstances[i];

						glVertex3fv(pInstance->GetPos( ));
						Vec3 vUp = pInstance->GetUpVector( ) * 10.0f;
						glVertex3f(pInstance->GetPos( ).x + vUp.x, pInstance->GetPos( ).y + vUp.y, pInstance->GetPos( ).z + vUp.z);
					}
				}
				glEnd( );
			}
			glPopAttrib( );
		}
		EndFixedPipelineRender( );
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::DrawBox

	void CMyDebugRenderFunctions::DrawBox(const st_float32 afMin[3], const st_float32 afMax[3], st_bool bLines)
	{
		if (bLines)
		{
			glBegin(GL_LINE_LOOP);
			glVertex3f(afMin[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMax[1], afMin[2]);
			glVertex3f(afMin[0], afMax[1], afMin[2]);
			glEnd( );

			glBegin(GL_LINE_LOOP);
			glVertex3f(afMin[0], afMin[1], afMax[2]);
			glVertex3f(afMax[0], afMin[1], afMax[2]);
			glVertex3f(afMax[0], afMax[1], afMax[2]);
			glVertex3f(afMin[0], afMax[1], afMax[2]);
			glEnd( );

			glBegin(GL_LINES);
			glVertex3f(afMin[0], afMin[1], afMin[2]);
			glVertex3f(afMin[0], afMin[1], afMax[2]);

			glVertex3f(afMax[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMin[1], afMax[2]);

			glVertex3f(afMax[0], afMax[1], afMin[2]);
			glVertex3f(afMax[0], afMax[1], afMax[2]);

			glVertex3f(afMin[0], afMax[1], afMin[2]);
			glVertex3f(afMin[0], afMax[1], afMax[2]);
			glEnd( );
		}
		else
		{
			glBegin(GL_QUADS);

			// -z
			glVertex3f(afMin[0], afMin[1], afMin[2]);
			glVertex3f(afMin[0], afMax[1], afMin[2]);
			glVertex3f(afMax[0], afMax[1], afMin[2]);
			glVertex3f(afMax[0], afMin[1], afMin[2]);

			// +z
			glVertex3f(afMin[0], afMin[1], afMax[2]);
			glVertex3f(afMax[0], afMin[1], afMax[2]);
			glVertex3f(afMax[0], afMax[1], afMax[2]);
			glVertex3f(afMin[0], afMax[1], afMax[2]);

			// -x
			glVertex3f(afMin[0], afMin[1], afMin[2]);
			glVertex3f(afMin[0], afMin[1], afMax[2]);
			glVertex3f(afMin[0], afMax[1], afMax[2]);
			glVertex3f(afMin[0], afMax[1], afMin[2]);

			// +x
			glVertex3f(afMax[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMax[1], afMin[2]);
			glVertex3f(afMax[0], afMax[1], afMax[2]);
			glVertex3f(afMax[0], afMin[1], afMax[2]);

			// -y
			glVertex3f(afMin[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMin[1], afMin[2]);
			glVertex3f(afMax[0], afMin[1], afMax[2]);
			glVertex3f(afMin[0], afMin[1], afMax[2]);

			// +y
			glVertex3f(afMin[0], afMax[1], afMin[2]);
			glVertex3f(afMin[0], afMax[1], afMax[2]);
			glVertex3f(afMax[0], afMax[1], afMax[2]);
			glVertex3f(afMax[0], afMax[1], afMin[2]);

			glEnd( );
		}
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::DrawSphere

	void CMyDebugRenderFunctions::DrawSphere(const Vec3& /*vCenter*/, st_float32 /*fRadius*/)
	{
		/*#ifdef _WIN32
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix( );
				glTranslatef(vCenter.x, vCenter.y, vCenter.z);
				glutSolidSphere(fRadius, 12, 12);
			glPopMatrix( );
		#endif*/
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::BeginFixedPipelineRender

	void CMyDebugRenderFunctions::BeginFixedPipelineRender(const CView& cView)
	{
		// setup matrix for fixed-function rendering
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(cView.GetProjection( ));
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(cView.GetModelview( ));

		// set up render state
		glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_VERTEX_PROGRAM_ARB);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);

		// matching glPopAttrib appears in ExdFixedPipelineRender() below
	}


	///////////////////////////////////////////////////////////////////////
	//  CMyDebugRenderFunctions::EndFixedPipelineRender

	void CMyDebugRenderFunctions::EndFixedPipelineRender(void)
	{
		glPopAttrib( );
	}

#endif // SPEEDTREE_WIN_OPENGL
