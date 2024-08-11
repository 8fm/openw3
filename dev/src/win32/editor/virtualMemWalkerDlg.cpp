#include "build.h"
#include "virtualMemWalkerDlg.h"

BEGIN_EVENT_TABLE( CEdVirtualMemWalkerDlg, wxDialog )
	EVT_MOUSEWHEEL( CEdVirtualMemWalkerDlg::OnMouseWheel )
END_EVENT_TABLE()

namespace 
{
	wxColor cUsedBlock (128, 205, 247);
	wxColor cUnusedBlock(194, 224, 150);
	wxColor cReservedBlock(206, 183, 168);
	wxColor cMaxFreeBlock(223, 239, 199);
	wxColor cDefault(253, 244, 255);
	wxColor cHeapUsedBlock(14, 36, 188);
	wxColor cHeapFreeBlock(80, 112, 33);

	Uint64 virtualMemoryMaxFreeBlockLen = 0;

#ifdef separate_wmv_thread
	wxPoint lastQueryPoint;

	const Uint32 MAX_HEAPS = 16;
	Uint32 heapMaxFreeBlockLen [ MAX_HEAPS ];

	const Uint32 MEMORY_BUFFER_LEN = 65535;
	UCHAR queryMemoryBuffer [ MEMORY_BUFFER_LEN ];

	Uint32 selectedHeapBlockStart = 0;
	Uint32 queryAddress = 0;
	Uint32 realQueryAddress = 0;
	Uint32 bytesOfMemoryRead = 0;
#endif

	Float scaleFactor = 1.0;
	Uint32 timerInterval = 500;

	void DrawBlock ( wxDC* dc, const wxRect& rect, Int32& nStartX, Int32& nStartY, Int32& nLength)
	{
		Int32 length = rect.width - nLength;
		while (nStartX + nLength > rect.width)
		{
			dc->DrawLine( nStartX, nStartY, rect.x + rect.width, nStartY );
			nLength -= (rect.x + rect.width - nStartX);
			nStartX = rect.x;
			nStartY += 1;
		}
		dc->DrawLine( nStartX, nStartY, nStartX + nLength, nStartY );
		nStartX += nLength;
	}

	void VirtualMemWalk ( wxWindow* window )
	{

		wxMemoryDC dc;
		wxBitmap bmp; bmp.Create(1024, 768);
		dc.SelectObject(bmp);

		// Offset for the text pane
		const int nVerticalOffset = 20;

		wxRect r = window->GetClientRect();
		ASSERT(r.IsEmpty() == false);

		wxBrush bg( wxColour( 255.0f * 0, 255.0f * 0, 255.0f * 0 ) );
		dc.SetBrush( bg );
		dc.DrawRectangle(r);

		r.y += nVerticalOffset;
		int nCurrentX = r.x;
		int nCurrentY = r.y;
		Uint64 nUsedBytes = 0; Uint64 nUnUsedBytes = 0; Uint64 nReservedBytes = 0;
		int nUsedBlocks = 0; int nUnUsedBlocks = 0; int nReservedBlocks = 0;

		int VirtualMemstatus = 0;

		wxPen usedPen ( cUsedBlock );
		wxPen reservedPen ( cReservedBlock);
		wxPen freePen ( cUnusedBlock);
		wxPen maxPen ( cMaxFreeBlock);
		wxPen defaultPen ( cDefault);
		wxPen heapUsedPen ( cHeapUsedBlock);
		wxPen heapFreePen ( cHeapFreeBlock);

		SYSTEM_INFO sInfo;
		ZeroMemory(&sInfo, sizeof(sInfo));
		GetSystemInfo( &sInfo );

		unsigned int nPageSize = sInfo.dwPageSize;
		unsigned int nCurrAddress = reinterpret_cast<unsigned int>( sInfo.lpMinimumApplicationAddress );
		unsigned int nMinVMAddress = nCurrAddress;
		unsigned int nMaxAddress = reinterpret_cast<unsigned int>( sInfo.lpMaximumApplicationAddress );

		unsigned int nNumUsedBlocks = 0;
		unsigned int nTotalUsedVirtualMem = 0;
		unsigned int nMaxUsedBlock = 0;

		unsigned int nNumFreeBlocks = 0;
		unsigned int nTotalFreeVirtualMem = 0;
		unsigned int nMaxFreeBlock = 0;

		wxPen* pen = NULL;

		int nMaxFreeBlockX = 0;
		int nMaxFreeBlockY = 0;
		int nMaxFreeBlockLen = 0;
		while( nCurrAddress < nMaxAddress )
		{
			MEMORY_BASIC_INFORMATION sMemInfo;

			if ( VirtualQuery( reinterpret_cast<void*>( nCurrAddress ), &sMemInfo, sizeof( sMemInfo ) ) )
			{
				if ( sMemInfo.State & MEM_FREE )
				{
					++nUnUsedBlocks;

					nUnUsedBytes += sMemInfo.RegionSize;

					if ( sMemInfo.RegionSize > nMaxFreeBlock )
					{
						nMaxFreeBlock = sMemInfo.RegionSize;
						nMaxFreeBlockX = nCurrentX;
						nMaxFreeBlockY = nCurrentY;
						nMaxFreeBlockLen = sMemInfo.RegionSize / (1024 * 4);
					}

					pen = &freePen;
				}
				else if ( sMemInfo.State & MEM_COMMIT )
				{
					++nUsedBlocks;

					nUsedBytes += sMemInfo.RegionSize;

					if ( sMemInfo.RegionSize > nMaxUsedBlock )
						nMaxUsedBlock = sMemInfo.RegionSize;

					pen = &usedPen;
				}
				else if ( sMemInfo.State & MEM_RESERVE )
				{
					++nNumUsedBlocks;

					nUsedBytes += sMemInfo.RegionSize;

					if ( sMemInfo.RegionSize > nMaxUsedBlock )
						nMaxUsedBlock = sMemInfo.RegionSize;

					pen = &usedPen;
				}


				nCurrAddress += sMemInfo.RegionSize;
			}
			else
				nCurrAddress += nPageSize;

			dc.SetPen(*pen);
			int nLen = sMemInfo.RegionSize / nPageSize * scaleFactor;

			// Let's draw the block
			DrawBlock( &dc, r, nCurrentX, nCurrentY, nLen);
		}

		// Largest free memory block
		virtualMemoryMaxFreeBlockLen = nMaxFreeBlockLen * scaleFactor / nPageSize;

		dc.SetPen(maxPen);
		DrawBlock(&dc, r, nMaxFreeBlockX, nMaxFreeBlockY, nMaxFreeBlockLen);

		// mc_todo: cannot enable this part on the same thread as main app

#ifdef separate_vmw_thread
		HANDLE pHeaps[ MAX_HEAPS ];

		Int32 numHeaps = GetProcessHeaps( MAX_HEAPS, pHeaps );
		GetProcessHeap();

		unsigned int nHeapUsedBytes = 0;
		unsigned int nHeapUnUsedBytes = 0;
		unsigned int nNumHeapBlocks = 0;
		unsigned int nNumUncommitedRanges = 0;
		unsigned int nSelectedHeapBlockStart = 0;
		unsigned int nSelectedHeapBlockLength = 0;
		Uint32 heapIndex = -1;

		for( Int32 i = 0; i < numHeaps; ++i )
		{
			HANDLE nCurrHeap = pHeaps[i];
			if (pHeaps[i] == GetProcessHeap())
				heapIndex = i;

			unsigned int nMinHeapAddress = 0xFFFFFFFF;
			unsigned int nMaxHeapAddress = 0;

			unsigned int nNumRegions = 0;
			unsigned int nTotalRegionsOverhead = 0;
			unsigned int nTotalRegionsCommitted = 0;
			unsigned int nTotalRegionsUncommitted = 0;
			unsigned int nMaxRegionCommitted = 0;
			unsigned int nMaxRegionUncommitted = 0;


			unsigned int nTotalInBlocks = 0;
			unsigned int nMaxBlock = 0;

			unsigned int nTotalInUncommitedRanges = 0;
			unsigned int nMaxUncommitedRange = 0;

			HeapLock( nCurrHeap );

			PROCESS_HEAP_ENTRY sEntry;
			sEntry.lpData = NULL;

			while( HeapWalk( nCurrHeap, &sEntry ) )
			{
				wxPen* pPen = NULL;

				if ( (unsigned int)sEntry.lpData < nMinHeapAddress )
					nMinHeapAddress = (unsigned int)sEntry.lpData;

				if ( (unsigned int)sEntry.lpData + sEntry.cbData > nMaxHeapAddress )
					nMaxHeapAddress = (unsigned int)sEntry.lpData + sEntry.cbData;

				if ( sEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) 
				{
					++nNumHeapBlocks;
					nHeapUsedBytes += sEntry.cbData;

					nTotalInBlocks += sEntry.cbData;
					if ( sEntry.cbData > nMaxBlock )
						nMaxBlock = sEntry.cbData;

					if ( sEntry.lpData &&
						(unsigned int)sEntry.lpData < queryAddress &&
						((unsigned int)sEntry.lpData + sEntry.cbData) > queryAddress)
					{
						LPVOID pBufferStart = sEntry.lpData;
						if ( RIM_IS_KEY_DOWN( IK_Shift ) )
						{
							pBufferStart = (LPVOID)queryAddress;
							realQueryAddress = queryAddress;
							nSelectedHeapBlockLength = nPageSize;
						}
						else
						{
							nSelectedHeapBlockLength = sEntry.cbData;
							realQueryAddress = (unsigned int)sEntry.lpData;
						}

						HANDLE hThisProcess = GetCurrentProcess();
						SIZE_T nNumBytesRead = 0;
						ReadProcessMemory(hThisProcess, pBufferStart, queryMemoryBuffer, MEMORY_BUFFER_LEN, &nNumBytesRead);
						bytesOfMemoryRead = nNumBytesRead;
						nSelectedHeapBlockStart = (unsigned int)pBufferStart;
					}


					pPen = &heapUsedPen;
				} else
					if ( sEntry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE )
					{
						++nNumUncommitedRanges;
						nHeapUnUsedBytes += sEntry.cbData;

						nTotalInUncommitedRanges += sEntry.cbData;
						if ( sEntry.cbData > nMaxUncommitedRange )
							nMaxUncommitedRange = sEntry.cbData;

						if ( sEntry.lpData &&
							(unsigned int)sEntry.lpData < queryAddress &&
							((unsigned int)sEntry.lpData + sEntry.cbData) > queryAddress)
						{
							bytesOfMemoryRead = 0;
						}

						pPen = &heapFreePen;
					} else
						if ( sEntry.wFlags & PROCESS_HEAP_REGION )
						{
							++nNumRegions;

							nTotalRegionsOverhead += sEntry.cbOverhead;
							nTotalRegionsCommitted += sEntry.Region.dwCommittedSize;
							nTotalRegionsUncommitted += sEntry.Region.dwUnCommittedSize;

							if ( sEntry.Region.dwCommittedSize > nMaxRegionCommitted )
								nMaxRegionCommitted = sEntry.Region.dwCommittedSize;

							if ( sEntry.Region.dwUnCommittedSize > nMaxRegionUncommitted )
								nMaxRegionUncommitted = sEntry.Region.dwUnCommittedSize;

							if ( sEntry.lpData &&
								(unsigned int)sEntry.lpData < queryAddress &&
								((unsigned int)sEntry.lpData + sEntry.cbData) > queryAddress)
							{
								bytesOfMemoryRead = 0;
							}
						} 


						if (pPen)
						{
							dc.SetPen(*pen);
							int nStartX = (((int)sEntry.lpData - (nMinVMAddress)) / nPageSize);
							int nStartY = r.y;
							int nRectWidth = r.width;
							while (nStartX > nRectWidth)
							{
								++nStartY;
								nStartX -= nRectWidth;
							}

							int nLength = sEntry.cbData / nPageSize;
							DrawBlock(&dc, r, nStartX, nStartY, nLength);
						}
			}

			// Najwiêkszy blok (KB)
			heapMaxFreeBlockLen [i] = nMaxBlock / (1024);

			HeapUnlock( nCurrHeap );
		}
#endif

		// Drawing the text
		dc.SetFont(wxFont(9, wxMODERN, wxNORMAL, wxBOLD, 0));
		r = window->GetClientRect();
		r.x += 10;
		r.width -= 10;
		int nTextWidth = (r.width) / 4;
		dc.SetTextBackground(*wxBLACK);
		dc.SetTextForeground(cUsedBlock);
		String str;
		str = String::Printf(TXT("Memory in use: %d MB"), (int)(nUsedBytes /(1024*1024)));
		r.height = r.y + 20;
		dc.DrawText(str.AsChar(), r.GetLeftTop());
		dc.SetTextForeground(cUnusedBlock);
		str = String::Printf(TXT("Free memory: %d MB"), (int)(nUnUsedBytes/(1024*1024)));
		r.x += nTextWidth;
		dc.DrawText(str.AsChar(), r.GetTopLeft());
		dc.SetTextForeground(cReservedBlock);
		str = String::Printf(TXT("Reserved memory: %d MB"), (int)(nReservedBytes/(1024*1024)));
		r.x += nTextWidth;
		dc.DrawText(str.AsChar(), r.GetTopLeft());
		dc.SetTextForeground(cMaxFreeBlock);
		str = String::Printf(TXT("Largest free memory block: %d MB"), nMaxFreeBlock / (1024*1024));
		r.x += nTextWidth;
		dc.DrawText(str.AsChar(), r.GetTopLeft());

#ifdef separate_vmw_thread

		// Let's draw selected block
		dc.SetPen(defaultPen);
		wxRect rClipBox = window->GetClientRect();
		rClipBox.y += nVerticalOffset;
		int nSelHeapStartX = (selectedHeapBlockStart - nMinVMAddress) / nPageSize;
		int nSelHeapStartY = rClipBox.y;
		while (nSelHeapStartX > rClipBox.width)
		{
			nSelHeapStartX -= rClipBox.width;
			++nSelHeapStartY;
		}
		int nLen = nSelectedHeapBlockLength / nPageSize;
		DrawBlock(&dc, rClipBox, nSelHeapStartX, nSelHeapStartY, nLen);

		dc.SetTextForeground(cHeapUsedBlock);
		str = String::Printf("Heap Blocks used: %d (%d MB)", nNumHeapBlocks, (int)(nHeapUsedBytes /(1024*1024)));
		r.x = 10;
		r.y += 15;
		r.height = r.y + 20;
		dc.DrawText(str.AsChar(), &r);
		dc.SetTextColor(cHeapFreeBlock);
		str = String::Printf("Heap blocks unused: %d (%d MB)", nNumUncommitedRanges, (int)(nHeapUnUsedBytes/(1024*1024)));
		r.x += nTextWidth;
		dc->DrawText(str.AsChar(), &r, DT_NOCLIP);

		dc.SetTextForeground(cDefault);
		str = String::Format("Memory under cursor: 0x%08X", 
			realQueryAddress);
		r.x += nTextWidth;
		dc.DrawText(str.AsChar(), r.GetTopLeft());

		queryAddress = nMinVMAddress + lastQueryPoint.x * nPageSize + ((lastQueryPoint.y - nVerticalOffset) * rClipBox.width * nPageSize);

		if (CVirtualMemWalkerDlg::m_bMainThreadPaused)
		{
			pDC->SetTextColor(cDefault);
			r.left += nTextWidth;
			pDC->DrawText(DjiString("Main thread paused"), &r, DT_NOCLIP);
		}
#endif


		wxClientDC cdc(window);
		cdc.Blit(0, 0, 1024, 768, &dc, 0, 0);
		return;
	}
};

CEdVirtualMemWalkerDlg::CEdVirtualMemWalkerDlg( wxWindow* parent ) :
	m_taskEvent( 0, 10 )
	, m_taskDone( false )
	, m_taskTimer( this, wxID_ANY )
{
	
	Create( parent, -1, wxString::FromAscii("Virtual Memory Walker"), wxDefaultPosition, wxSize (1024, 768), wxDEFAULT_DIALOG_STYLE );

	// Timer event
	Connect( m_taskTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( CEdVirtualMemWalkerDlg::OnTaskTimer ) );

	m_taskTimer.Start(timerInterval);
}

CEdVirtualMemWalkerDlg::~CEdVirtualMemWalkerDlg()
{
}

void CEdVirtualMemWalkerDlg::OnShow( wxShowEvent& event )
{
	if (event.IsShown())
		m_taskTimer.Start(timerInterval);
	else 
		m_taskTimer.Stop();
}


void CEdVirtualMemWalkerDlg::OnClose( wxCommandEvent& event )
{
	// Do nothing
	Show(false);
}


DWORD CEdVirtualMemWalkerDlg::ReadFunc( LPVOID lpvThreadParam )
{
	CEdVirtualMemWalkerDlg* dlg = ( CEdVirtualMemWalkerDlg* )lpvThreadParam;

	// Task waiting loop
	for ( ;; )
	{
		// Wait for tasks
		dlg->m_taskEvent.Acquire();
	}

	// Never reached
	return 1;
}

void CEdVirtualMemWalkerDlg::OnTaskTimer( wxTimerEvent& event )
{
	VirtualMemWalk(this);
}

void CEdVirtualMemWalkerDlg::OnMouseWheel( wxMouseEvent& event )
{
	Float delta = 0.0f;
	if ( event.GetWheelRotation() != 0 )
	{
		delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	}
	scaleFactor += delta / 100.f;
	scaleFactor = Min(Max(scaleFactor, 0.1f), 10.f);
	VirtualMemWalk(this);
}
