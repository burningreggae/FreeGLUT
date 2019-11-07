/*
* fg_tablet_mswin.c
*/

#include <GL/freeglut.h>
#include <stdlib.h>
#include "../fg_internal.h"


#include "wintab.h"

#define PACKETDATA ( PK_CURSOR | PK_NORMAL_PRESSURE | PK_BUTTONS )
#define PACKETMODE 0
// Tablet control extension defines
#define PACKETEXPKEYS		PKEXT_ABSOLUTE
#define PACKETTOUCHSTRIP	PKEXT_ABSOLUTE
#define PACKETTOUCHRING		PKEXT_ABSOLUTE
#include "pktdef.h"

// Function pointers to Wintab functions exported from wintab32.dll.
typedef UINT ( API * WTINFOA ) ( UINT, UINT, LPVOID );
typedef HCTX ( API * WTOPENA )( HWND, LPLOGCONTEXTA, BOOL );
typedef BOOL ( API * WTGETA ) ( HCTX, LPLOGCONTEXT );
typedef BOOL ( API * WTSETA ) ( HCTX, LPLOGCONTEXT );
typedef BOOL ( API * WTCLOSE ) ( HCTX );
typedef BOOL ( API * WTENABLE ) ( HCTX, BOOL );
typedef BOOL ( API * WTPACKET ) ( HCTX, UINT, LPVOID );
typedef BOOL ( API * WTOVERLAP ) ( HCTX, BOOL );
typedef BOOL ( API * WTSAVE ) ( HCTX, LPVOID );
typedef BOOL ( API * WTCONFIG ) ( HCTX, HWND );
typedef HCTX ( API * WTRESTORE ) ( HWND, LPVOID, BOOL );
typedef BOOL ( API * WTEXTSET ) ( HCTX, UINT, LPVOID );
typedef BOOL ( API * WTEXTGET ) ( HCTX, UINT, LPVOID );
typedef BOOL ( API * WTQUEUESIZESET ) ( HCTX, int );
typedef int  ( API * WTDATAPEEK ) ( HCTX, UINT, UINT, int, LPVOID, LPINT);
typedef int  ( API * WTPACKETSGET ) (HCTX, int, LPVOID);
typedef HMGR ( API * WTMGROPEN ) ( HWND, UINT );
typedef BOOL ( API * WTMGRCLOSE ) ( HMGR );
typedef HCTX ( API * WTMGRDEFCONTEXT ) ( HMGR, BOOL );
typedef HCTX ( API * WTMGRDEFCONTEXTEX ) ( HMGR, UINT, BOOL );

WTINFOA gpWTInfoA = NULL;
WTOPENA gpWTOpenA = NULL;
WTGETA gpWTGetA = NULL;
WTSETA gpWTSetA = NULL;
WTCLOSE gpWTClose = NULL;
WTPACKET gpWTPacket = NULL;
WTENABLE gpWTEnable = NULL;
WTOVERLAP gpWTOverlap = NULL;
WTSAVE gpWTSave = NULL;
WTCONFIG gpWTConfig = NULL;
WTRESTORE gpWTRestore = NULL;
WTEXTSET gpWTExtSet = NULL;
WTEXTGET gpWTExtGet = NULL;
WTQUEUESIZESET gpWTQueueSizeSet = NULL;
WTDATAPEEK gpWTDataPeek = NULL;
WTPACKETSGET gpWTPacketsGet = NULL;
WTMGROPEN gpWTMgrOpen = NULL;
WTMGRCLOSE gpWTMgrClose = NULL;
WTMGRDEFCONTEXT gpWTMgrDefContext = NULL;
WTMGRDEFCONTEXTEX gpWTMgrDefContextEx = NULL;

#define GETPROCADDRESS(type, func) gp##func = (type)GetProcAddress(ghWintab, #func);

HINSTANCE ghWintab = NULL;

void LoadWintab( void )
{
	if ( ghWintab ) return;
	ghWintab = LoadLibraryA( "Wintab32.dll" );

	if ( !ghWintab ) return;

	// Explicitly find the exported Wintab functions in which we are interested.
	// We are using the ASCII, not unicode versions (where applicable).
	GETPROCADDRESS( WTOPENA, WTOpenA );
	GETPROCADDRESS( WTINFOA, WTInfoA );
	GETPROCADDRESS( WTGETA, WTGetA );
	GETPROCADDRESS( WTSETA, WTSetA );
	GETPROCADDRESS( WTPACKET, WTPacket );
	GETPROCADDRESS( WTCLOSE, WTClose );
	GETPROCADDRESS( WTENABLE, WTEnable );
	GETPROCADDRESS( WTOVERLAP, WTOverlap );
	GETPROCADDRESS( WTSAVE, WTSave );
	GETPROCADDRESS( WTCONFIG, WTConfig );
	GETPROCADDRESS( WTRESTORE, WTRestore );
	GETPROCADDRESS( WTEXTSET, WTExtSet );
	GETPROCADDRESS( WTEXTGET, WTExtGet );
	GETPROCADDRESS( WTQUEUESIZESET, WTQueueSizeSet );
	GETPROCADDRESS( WTDATAPEEK, WTDataPeek );
	GETPROCADDRESS( WTPACKETSGET, WTPacketsGet );
	GETPROCADDRESS( WTMGROPEN, WTMgrOpen );
	GETPROCADDRESS( WTMGRCLOSE, WTMgrClose );
	GETPROCADDRESS( WTMGRDEFCONTEXT, WTMgrDefContext );
	GETPROCADDRESS( WTMGRDEFCONTEXTEX, WTMgrDefContextEx );
}

void UnloadWintab( void )
{
	if ( ghWintab )
	{
		FreeLibrary( ghWintab );
		ghWintab = NULL;
	}

	gpWTOpenA			= NULL;
	gpWTClose			= NULL;
	gpWTInfoA			= NULL;
	gpWTPacket			= NULL;
	gpWTEnable			= NULL;
	gpWTOverlap			= NULL;
	gpWTSave			= NULL;
	gpWTConfig			= NULL;
	gpWTGetA			= NULL;
	gpWTSetA			= NULL;
	gpWTRestore			= NULL;
	gpWTExtSet			= NULL;
	gpWTExtGet			= NULL;
	gpWTQueueSizeSet	= NULL;
	gpWTDataPeek		= NULL;
	gpWTPacketsGet		= NULL;
	gpWTMgrOpen			= NULL;
	gpWTMgrClose		= NULL;
	gpWTMgrDefContext	= NULL;
	gpWTMgrDefContextEx = NULL;
}

LOGCONTEXT	glogContext = {0};

AXIS TabletX = {0};
AXIS TabletY = {0};
AXIS TabletZ = {0};

HCTX TabletInit(HWND hWnd)
{
	HCTX hctx = NULL;
	UINT wDevice = 0;
	UINT wExtX = 0;
	UINT wExtY = 0;
	UINT wWTInfoRetVal = 0;

	if ( !gpWTInfoA ) return 0;

	// Set option to move system cursor before getting default system context.
	glogContext.lcOptions |= CXO_SYSTEM;

	// Open default system context so that we can get tablet data
	// in screen coordinates (not tablet coordinates).
	wWTInfoRetVal = gpWTInfoA(WTI_DEFSYSCTX, 0, &glogContext);


	// modify the digitizing region
	//sprintf(glogContext.lcName, "glut%x", hWnd);

	// We process WT_PACKET (CXO_MESSAGES) messages.
	glogContext.lcOptions |= CXO_MESSAGES;
	//glogContext.lcOptions &= ~CXO_SYSTEM;
	{
		UINT extIndex_TouchStrip = 0;
		UINT extIndex_TouchRing = 0;
		UINT extIndex_ExpKeys = 0;
		WTPKT lTouchRing_Mask = 0;
		WTPKT lTouchStrip_Mask = 0;
		WTPKT lExpKeys_Mask = 0;

		UINT i=0, thisTag = 0;
		// Iterate through Wintab extension indices
		for ( ;gpWTInfoA(WTI_EXTENSIONS+i, EXT_TAG, &thisTag); i++)
		{
			// looking for the specified tag
			if (thisTag == WTX_TOUCHSTRIP) extIndex_TouchStrip = i;
			else if (thisTag == WTX_TOUCHRING) extIndex_TouchRing = i;
			else if (thisTag == WTX_EXPKEYS) extIndex_ExpKeys = i;
			else if (thisTag == WTX_EXPKEYS2) extIndex_ExpKeys = i;
		}
		gpWTInfoA(WTI_EXTENSIONS + extIndex_TouchStrip, EXT_MASK, &lTouchStrip_Mask);
		gpWTInfoA(WTI_EXTENSIONS + extIndex_TouchRing, EXT_MASK, &lTouchRing_Mask);
		gpWTInfoA(WTI_EXTENSIONS + extIndex_ExpKeys, EXT_MASK, &lExpKeys_Mask);

		// What data items we want to be included in the tablet packets
		glogContext.lcPktData = PACKETDATA | lTouchStrip_Mask | lTouchRing_Mask | lExpKeys_Mask;
	}

	// Which packet items should show change in value since the last
	// packet (referred to as 'relative' data) and which items
	// should be 'absolute'.
	glogContext.lcPktMode = PACKETMODE;

#if 1

	// This bitfield determines whether or not this context will receive
	// a packet when a value for each packet field changes.  This is not
	// supported by the Intuos Wintab.  Your context will always receive
	// packets, even if there has been no change in the data.
	glogContext.lcMoveMask = PACKETDATA;


	// Which buttons events will be handled by this context.  lcBtnMask
	// is a bitfield with one bit per button.
	glogContext.lcBtnUpMask = glogContext.lcBtnDnMask;

	// Set the entire tablet as active
	wWTInfoRetVal = gpWTInfoA( WTI_DEVICES + 0, DVC_X, &TabletX );
	wWTInfoRetVal = gpWTInfoA( WTI_DEVICES, DVC_Y, &TabletY );
	wWTInfoRetVal = gpWTInfoA( WTI_DEVICES, DVC_Z, &TabletZ );

	glogContext.lcInOrgX = 0;
	glogContext.lcInOrgY = 0;
	glogContext.lcInExtX = TabletX.axMax;
	glogContext.lcInExtY = TabletY.axMax;

	// Guarantee the output coordinate space to be in screen coordinates.
	glogContext.lcOutOrgX = GetSystemMetrics( SM_XVIRTUALSCREEN );
	glogContext.lcOutOrgY = GetSystemMetrics( SM_YVIRTUALSCREEN );
	glogContext.lcOutExtX = GetSystemMetrics( SM_CXVIRTUALSCREEN ); //SM_CXSCREEN );

	// In Wintab, the tablet origin is lower left.  Move origin to upper left
	// so that it coincides with screen origin.
	glogContext.lcOutExtY = -GetSystemMetrics( SM_CYVIRTUALSCREEN );	//SM_CYSCREEN );

	// Leave the system origin and extents as received:
	// lcSysOrgX, lcSysOrgY, lcSysExtX, lcSysExtY
#endif
	// open the region
	// The Wintab spec says we must open the context disabled if we are
	// using cursor masks.
	hctx = gpWTOpenA( hWnd, &glogContext,TRUE );


	return hctx;
}

PACKET pkt_last;
PACKETEXT pktExt_last;

HCTX hCtx = NULL;

void fgPlatformInitializeTablet(void)
{
	HWND hwnd;
	if (!fgStructure.CurrentWindow)
	{
		return;
	}
	hwnd = fgStructure.CurrentWindow->Window.Handle;
	LoadWintab();

	hCtx = TabletInit(hwnd);
	memset(&pkt_last,0x81,sizeof(pkt_last));
	memset(&pktExt_last,0x82,sizeof(pktExt_last));
}

void fgPlatformTabletClose(void)
{
	if (hCtx)
	{
		gpWTClose(hCtx);
		hCtx = 0;
	}
	UnloadWintab();
}

int fgPlatformHasTablet(void)
{
	if (!hCtx) fgPlatformInitializeTablet();
	return hCtx ? 1 : 0;
}

int fgPlatformTabletNumButtons(void)
{
	if (!hCtx) fgPlatformInitializeTablet();
	return glogContext.lcStatus;
}



void fgTabletHandleWinEvent(SFG_Window *window,HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	PACKET pkt;
	PACKETEXT pktExt;

	switch(uMsg)
	{
	case WT_PACKET:
	{
		if (!gpWTPacket || !gpWTPacket((HCTX)lParam, (UINT)wParam, &pkt) ) return;

/*
		fgWarning(
			"pkCursor   : %u\n"
			"pkButtons  : %x\n"
			"pkX        : %d\n"
			"pkY        : %d\n"
			"pkNormalPressure :%d\n",
			pkt.pkCursor,
			pkt.pkButtons,
			pkt.pkX,
			pkt.pkY,
			pkt.pkNormalPressure
			);
*/
		if ( memcmp ( &pkt_last,&pkt,sizeof(pkt_last) ) )
		{
			INVOKE_WCB( *window, TabletButton, (pkt.pkCursor,pkt.pkButtons,pkt.pkNormalPressure,0) );
			memcpy ( &pkt_last,&pkt,sizeof(pkt_last));
		}
	} break;

		case WT_PACKETEXT:
		{
			if (!gpWTPacket || !gpWTPacket((HCTX)lParam, (UINT)wParam, &pktExt) ) return;

			fgWarning("Keys: %d %d %d %d\n",pktExt.pkExpKeys.nTablet, pktExt.pkExpKeys.nControl,pktExt.pkExpKeys.nLocation, pktExt.pkExpKeys.nState);
			fgWarning("Ring: %d %d %d %d\n",pktExt.pkTouchRing.nTablet, pktExt.pkTouchRing.nControl,pktExt.pkTouchRing.nMode, pktExt.pkTouchRing.nPosition);
			fgWarning("Strip: %d %d %d %d\n",pktExt.pkTouchStrip.nTablet, pktExt.pkTouchStrip.nControl,pktExt.pkTouchStrip.nMode, pktExt.pkTouchStrip.nPosition);
		}
		break;


	case WM_ACTIVATE:
		/* if switching in the middle, disable the region */
		//if ( 0 == hCtx ) fgPlatformInitializeTablet();
		if (hCtx && gpWTEnable)
		{
			gpWTEnable(hCtx, GET_WM_ACTIVATE_STATE(wParam, lParam));
			if (hCtx && GET_WM_ACTIVATE_STATE(wParam, lParam))
			{
				if (gpWTOverlap) gpWTOverlap(hCtx, TRUE);
			}
		}
		break;

	}
}
