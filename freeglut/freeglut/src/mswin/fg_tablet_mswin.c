/*
* fg_tablet_mswin.c
*/

#include <GL/freeglut.h>
#include <stdlib.h>
#include "../fg_internal.h"


#include "msgpack.h"
#include "wintab.h"

#define PACKETNAME FULL
#define FULLPACKETDATA	(   PK_X | PK_Y | PK_Z | PK_BUTTONS | PK_SERIAL_NUMBER \
	| PK_CONTEXT \
	| PK_STATUS  \
	| PK_TIME    \
	| PK_CHANGED \
	| PK_CURSOR \
	| PK_NORMAL_PRESSURE \
	| PK_TANGENT_PRESSURE /* the scroll wheel on wacom airbrush */\
	| PK_ORIENTATION \
	| PK_ROTATION \
	)
#define FULLPACKETMODE	0 /* ALL FIELDS ABSOLUTE */
#define FULLPACKETFKEYS PKEXT_ABSOLUTE
#define FULLPACKETTILT	PKEXT_ABSOLUTE
#define FULLPACKETTOUCHSTRIP PKEXT_ABSOLUTE
#define FULLPACKETTOUCHRING PKEXT_ABSOLUTE
#define FULLPACKETEXPKEYS2 PKEXT_ABSOLUTE

#include "pktdef.h"
#undef PACKETNAME

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

HINSTANCE ghWintab = NULL;

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

BOOL LoadWintab( void )
{
	//	ghWintab = LoadLibraryA(  "C:\\dev\\mainline\\Wacom\\Win\\Win32\\Debug\\Wacom_Tablet.dll" );
	//	ghWintab = LoadLibraryA(  "C:\\dev\\mainline\\Wacom\\Win\\Win32\\Debug\\Wintab32.dll" );	
	ghWintab = LoadLibraryA( "Wintab32.dll" );

	if ( !ghWintab )
	{
		return FALSE;
	}

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

	return TRUE;
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
	gpWTSave				= NULL;
	gpWTConfig			= NULL;
	gpWTGetA				= NULL;
	gpWTSetA				= NULL;
	gpWTRestore			= NULL;
	gpWTExtSet			= NULL;
	gpWTExtGet			= NULL;
	gpWTQueueSizeSet	= NULL;
	gpWTDataPeek		= NULL;
	gpWTPacketsGet		= NULL;
	gpWTMgrOpen			= NULL;
	gpWTMgrClose		= NULL;
	gpWTMgrDefContext = NULL;
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
	sprintf(glogContext.lcName, "glut_digi%x", hWnd);

	// We process WT_PACKET (CXO_MESSAGES) messages.
	glogContext.lcOptions |= CXO_MESSAGES;

	// What data items we want to be included in the tablet packets
	glogContext.lcPktData = FULLPACKETDATA;

	// Which packet items should show change in value since the last
	// packet (referred to as 'relative' data) and which items
	// should be 'absolute'.
	glogContext.lcPktMode = FULLPACKETMODE;

	// This bitfield determines whether or not this context will receive
	// a packet when a value for each packet field changes.  This is not
	// supported by the Intuos Wintab.  Your context will always receive
	// packets, even if there has been no change in the data.
	glogContext.lcMoveMask = FULLPACKETDATA;

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

	// open the region
	// The Wintab spec says we must open the context disabled if we are 
	// using cursor masks.  
	hctx = gpWTOpenA( hWnd, &glogContext, FALSE );


	return hctx;
}

extern int tablet_initialized;
HCTX hCtx = NULL;

void fgPlatformInitializeTablet(void)
{
	HWND hwnd;
	tablet_initialized = 1;
	if (!fgStructure.CurrentWindow)
	{
		tablet_initialized = 0;
		return;
	}
	hwnd = fgStructure.CurrentWindow->Window.Handle;
	LoadWintab();

	hCtx = TabletInit(hwnd);
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
	return hCtx != 0 ? 1 : 0;
}

int fgPlatformTabletNumButtons(void)
{
	return 0;
}

void fgPlatformTabletSetWindow(SFG_Window *window)
{
	return;
}


void fgTabletHandleWinEvent(SFG_Window *window,HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	FULLPACKET pkt;
	switch(uMsg)
	{
	case WT_PACKET:
	{
		if (!gpWTPacket || !gpWTPacket((HCTX)lParam, (UINT)wParam, &pkt) ) return;
		fgWarning(
			"pkStatus  : %x\n"
			"pkTime    : %u\n",
			"pkChanged : %u\n"
			"pkSerialNumber : %u\n"
			"pkCursor   : %u\n"
			"pkButtons  : %x\n"
			"pkX        : %d\n"
			"pkY        : %d\n"
			"pkZ        : %d\n"
			"pkNormalPressure :%d\n"
			"pkTangentPressure : %d\n"
			"orAzimuth : %d\n"
			"orAltitude : %d\n"
			"orTwist : %d\n"
			"roPitch : %d\n"
			"roRoll  : %d\n"
			"roYaw   : %d\n"
			"pkFKeys   : %u\n"
			"tiltX    : %d\n"
			"tiltY    : %d\n",
			pkt.pkStatus,
			pkt.pkTime,
			pkt.pkChanged,
			pkt.pkSerialNumber,
			pkt.pkCursor,
			pkt.pkButtons,
			pkt.pkX,
			pkt.pkY,
			pkt.pkZ,
			pkt.pkNormalPressure,
			pkt.pkTangentPressure,
			pkt.pkOrientation.orAzimuth,
			pkt.pkOrientation.orAltitude,
			pkt.pkOrientation.orTwist,
			pkt.pkRotation.roPitch,
			pkt.pkRotation.roRoll,
			pkt.pkRotation.roYaw,
			pkt.pkFKeys,
			pkt.pkTilt.tiltX,
			pkt.pkTilt.tiltY
			);

			INVOKE_WCB( *window, TabletMotion, (pkt.pkX,pkt.pkY) );

		}
		break;

	case WM_ACTIVATE:
		/* if switching in the middle, disable the region */
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
