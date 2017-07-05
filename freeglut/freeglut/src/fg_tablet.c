/* Tablet support for Linux.
 * Written by John Tsiombikas <nuclear@member.fsf.org>
 *
 * This code supports 3Dconnexion's 6-dof space-whatever devices.
 * It can communicate with either the proprietary 3Dconnexion daemon (3dxsrv)
 * free spacenavd (http://spacenav.sourceforge.net), through the "standard"
 * magellan X-based protocol.
 */


#include <GL/freeglut.h>
#include "fg_internal.h"


/* -- PRIVATE FUNCTIONS --------------------------------------------------- */

extern void fgPlatformInitializeTablet(void);
extern void fgPlatformTabletClose(void);
extern int fgPlatformHasTablet(void);
extern int fgPlatformTabletNumButtons(void);
extern void fgPlatformTabletSetWindow(SFG_Window *window);


int tablet_initialized = 0;

void fgInitialiseTablet(void)
{
    if(tablet_initialized != 0) {
        return;
    }

    fgPlatformInitializeTablet();
}

void fgTabletClose(void)
{
	fgPlatformTabletClose();
}

int fgHasTablet(void)
{
    if(tablet_initialized == 0) {
        fgInitialiseTablet();
        if(tablet_initialized != 1) {
            fgWarning("fgInitialiseTablet failed\n");
            return 0;
        }
    }

    return fgPlatformHasTablet();
}

int fgTabletNumButtons(void)
{
    if(tablet_initialized == 0) {
        fgInitialiseTablet();
        if(tablet_initialized != 1) {
            fgWarning("fgInitialiseTablet failed\n");
            return 0;
        }
    }

    return fgPlatformTabletNumButtons();
}

void fgTabletSetWindow(SFG_Window *window)
{
    if(tablet_initialized == 0) {
        fgInitialiseTablet();
        if(tablet_initialized != 1) {
            return;
        }
    }

    fgPlatformTabletSetWindow(window);
}

