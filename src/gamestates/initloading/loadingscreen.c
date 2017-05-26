#include "gamestates/initloading/loadingscreen.h"
#include <clib/dos_protos.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>
#include "config.h"
#include "gamestates/initloading/worker.h"
#include "gamestates/game/turret.h"

#define LOADINGSCREEN_PROGRESS_WIDTH 256
#define LOADINGSCREEN_PROGRESS_HEIGHT 32
#define LOADINGSCREEN_COLOR_PROGRESS_OUTLINE 1
#define LOADINGSCREEN_COLOR_PROGRESS_FILL 15
#define LOADINGSCREEN_COLOR_BG 0

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
BYTE g_pLoadProgress[LOADINGSCREEN_BOBSOURCE_COUNT] = {-1};

void loadingScreenCreate(void) {
	// Create View & VPort
	s_pView = viewCreate(V_GLOBAL_CLUT);
	s_pVPort = vPortCreate(
		s_pView,	WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, WINDOW_SCREEN_BPP, 0
	);
	s_pBuffer = simpleBufferCreate(s_pVPort, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, BMF_CLEAR);
	paletteLoad("data/amidb32.plt", s_pVPort->pPalette, 1 << WINDOW_SCREEN_BPP);
	viewLoad(s_pView);

	blitRect(
		s_pBuffer->pBuffer,
		(WINDOW_SCREEN_WIDTH - LOADINGSCREEN_PROGRESS_WIDTH)/2 - 1,
		(WINDOW_SCREEN_HEIGHT - LOADINGSCREEN_PROGRESS_HEIGHT)/2 - 1,
		LOADINGSCREEN_PROGRESS_WIDTH+2,
		LOADINGSCREEN_PROGRESS_HEIGHT+2,
		LOADINGSCREEN_COLOR_PROGRESS_OUTLINE
	);
}

void loadingScreenDestroy() {
	viewLoad(0);
	viewDestroy(s_pView);
}

void loadingScreenSetProgress(UBYTE ubProgress) {
	logBlockBegin("loadingScreenSetProgress(ubProgress: %hu)", ubProgress);
	blitRect(
		s_pBuffer->pBuffer,
		(WINDOW_SCREEN_WIDTH - LOADINGSCREEN_PROGRESS_WIDTH)/2,
		(WINDOW_SCREEN_HEIGHT - LOADINGSCREEN_PROGRESS_HEIGHT)/2,
		(ubProgress*LOADINGSCREEN_PROGRESS_WIDTH)/100,
		LOADINGSCREEN_PROGRESS_HEIGHT,
		LOADINGSCREEN_COLOR_PROGRESS_FILL
	);
	logBlockEnd("loadingScreenSetProgress()");
}

void loadingScreenUpdate(void) {
	static BYTE pPrevFrameProgress[LOADINGSCREEN_BOBSOURCE_COUNT] = {-1};
	static tBobSource *pSources[LOADINGSCREEN_BOBSOURCE_COUNT] = {
		&g_pVehicleTypes[VEHICLE_TYPE_TANK].sMainSource,
		&g_pVehicleTypes[VEHICLE_TYPE_TANK].sAuxSource,
		&g_pVehicleTypes[VEHICLE_TYPE_JEEP].sMainSource,
		&g_sBrownTurretSource
	};
	UBYTE i;
	BYTE bProgress;
	UWORD uwFrameX, uwFrameY;
	for(i = 0; i != LOADINGSCREEN_BOBSOURCE_COUNT+0; ++i) {
		bProgress = g_pLoadProgress[i];
		if(i < LOADINGSCREEN_BOBSOURCE_COUNT) {
			if(pPrevFrameProgress[i] != bProgress) {
				// Draw recently loaded bob source frames
				uwFrameX = (WINDOW_SCREEN_WIDTH-VEHICLE_BODY_WIDTH*LOADINGSCREEN_BOBSOURCE_COUNT)/2 + VEHICLE_BODY_WIDTH*i;
				uwFrameY = (WINDOW_SCREEN_HEIGHT-LOADINGSCREEN_PROGRESS_HEIGHT)/2 - VEHICLE_BODY_HEIGHT - 8;
				// Erase background
				blitRect(
					s_pBuffer->pBuffer,
					uwFrameX, uwFrameY,
					VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT,
					LOADINGSCREEN_COLOR_BG
				);
				// Draw next frame
				blitCopyMask(
					pSources[i]->pBitmap,
					0, VEHICLE_BODY_WIDTH*g_pLoadProgress[i],
					s_pBuffer->pBuffer, uwFrameX, uwFrameY,
					VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT,
					pSources[i]->pMask->pData
				);
				pPrevFrameProgress[i] = bProgress;
			}
		}
		else {
			// Indicate other progress
			logWrite("ERR: Unknown progress type\n");
		}
	}
}
