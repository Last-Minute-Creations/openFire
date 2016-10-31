#include "gamestates/game/bunker.h"
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/game.h>
#include <ace/managers/rand.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include <ace/utils/palette.h>
#include "gamestates/game/game.h"
#include "gamestates/game/world.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/player.h"

// Initial config
#define GFX_SKY_HEIGHT (50)
// Surface gfx:
#define GFX_SURFACE_HEIGHT (32)
#define GFX_SURFACE_SKY_HEIGHT (21)
#define GFX_SURFACE_ALPHA_HEIGHT (6)
// Placement offsets
#define GFX_SURFACE_OFFS (GFX_SKY_HEIGHT - GFX_SURFACE_SKY_HEIGHT)
#define GFX_SHAFT_OFFS (GFX_SURFACE_OFFS + GFX_SURFACE_HEIGHT - GFX_SURFACE_ALPHA_HEIGHT)
// Hangars' Y pos refer to its bottom line
#define HANGAR_HI_Y (GFX_SHAFT_OFFS + 54)
#define HANGAR_LO_Y (GFX_SHAFT_OFFS + 92)
#define HANGAR_SHAFT_DIST 18

#define GFX_LAMP_DELTAY 4

#define GFX_VEHICLE_WIDTH  32
#define GFX_VEHICLE_HEIGHT 16
#define GFX_COLOR_SKY 24

// Bunker gamestate modes
#define BUNKER_MODE_INVALID 0
#define BUNKER_MODE_CHOICE 1
#define BUNKER_MODE_ELEVATOR_TO_VEHICLE 2
#define BUNKER_MODE_VEHICLE_TO_ELEVATOR 3
#define BUNKER_MODE_ELEVATOR_TO_SURFACE 4
#define BUNKER_MODE_MAP 5

#define BUNKER_ANIM_FRAMES 100
#define BUNKER_FADE_FRAMES 16

UBYTE s_ubChoice;
UBYTE s_ubMode;
UWORD s_uwFrameCount;
UWORD s_uwPlatformY;

tView *s_pBunkerView;
tVPort *s_pBunkerVPort;
tSimpleBufferManager *s_pBunkerBfr;

tBitMap *s_pVehiclesBitmap;
tBitmapMask *s_pVehiclesMask;

tBob *s_pPlatform;
tBob *s_pLamp;

tBunkerVehicle s_pVehicles[4];


void bunkerVehiclesResetPos(void) {
	const UWORD uwLeftX = WINDOW_SCREEN_WIDTH/2 - HANGAR_SHAFT_DIST - GFX_VEHICLE_WIDTH;
	const UWORD uwRightX = WINDOW_SCREEN_WIDTH/2 + HANGAR_SHAFT_DIST+2;
	const UWORD uwTopY = HANGAR_HI_Y - GFX_VEHICLE_HEIGHT;
	const UWORD uwBottomY = HANGAR_LO_Y - GFX_VEHICLE_HEIGHT;
	s_pVehicles[0].uwX =  uwLeftX; s_pVehicles[0].uwY =    uwTopY;
	s_pVehicles[1].uwX = uwRightX; s_pVehicles[1].uwY =    uwTopY;
	s_pVehicles[2].uwX =  uwLeftX; s_pVehicles[2].uwY = uwBottomY;
	s_pVehicles[3].uwX = uwRightX; s_pVehicles[3].uwY = uwBottomY;	
}

void bunkerVehiclesCreate(void) {
	UBYTE i;
	
	logBlockBegin("bunkerVehiclesCreate()");
	// Vehicle load & initial display
	// TODO: brown player
	s_pVehiclesBitmap = bitmapCreateFromFile("data/bunker/vehicles_green.bm");
	s_pVehiclesMask = bitmapMaskCreate("data/bunker/vehicles.msk");
	
	// Bobs
	for(i = 0; i != 4; ++i)
		s_pVehicles[i].pBob = bobCreate(
			s_pVehiclesBitmap, s_pVehiclesMask, GFX_VEHICLE_HEIGHT, i
		);
	logBlockEnd("bunkerVehiclesCreate()");
}

void bunkerVehiclesDestroy(void) {
	UBYTE i;
	
	for(i = 0; i != 4; ++i)
		bobDestroy(s_pVehicles[i].pBob);
}

void bunkerCreate(void) {
	tBitMap *pDirt;
	tBob *pSurface, *pHangar;
	tBitmapMask *pMask;
	UWORD x, y;
	UBYTE ubDirtSize;
	UBYTE i;
	
	logBlockBegin("bunkerCreate()");
	s_pBunkerView = viewCreate(V_GLOBAL_CLUT);
	s_pBunkerVPort = vPortCreate(s_pBunkerView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, GAME_BPP, 0);
	s_pBunkerBfr = simpleBufferCreate(s_pBunkerVPort, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, 0);
	if(!s_pBunkerBfr) {
		logWrite("Buffer creation failed");
		gameClose();
		return;
	}
	logWrite("Allocated buffer\n");
	paletteLoad("data/amidb32.plt", s_pBunkerVPort->pPalette, 1 << GAME_BPP);
	
	s_ubChoice = 0;
	
	// Draw bunker bg - dirt
	pDirt = bitmapCreateFromFile("data/bunker/dirt.bm");
	ubDirtSize = pDirt->BytesPerRow << 3;
	for(y = 0; y != WINDOW_SCREEN_HEIGHT; y += ubDirtSize) {
		for(x = 0; x != WINDOW_SCREEN_WIDTH; x += ubDirtSize)
			blitCopyAligned(
				pDirt, 0, (uwRand() & 3)*ubDirtSize,
				s_pBunkerBfr->pBuffer, x, y,
				ubDirtSize, 32
			);
	}
	bitmapDestroy(pDirt);
	
	// Draw sky rectangle
	blitRect(
		s_pBunkerBfr->pBuffer, 0, 0,
		WINDOW_SCREEN_WIDTH, GFX_SURFACE_OFFS, GFX_COLOR_SKY
	);
		
	// Draw shaft & hangars
	pHangar = bobUniqueCreate("data/bunker/hangar.bm", "data/bunker/hangar.msk", 0, 0);
	bobDraw(
		pHangar, s_pBunkerBfr->pBuffer,
		(WINDOW_SCREEN_WIDTH-(pHangar->pBitmap->BytesPerRow<<3))/2, GFX_SHAFT_OFFS
	);
	bobUniqueDestroy(pHangar);
	
	// Draw surface line & silo
	pSurface = bobUniqueCreate("data/bunker/surface.bm", "data/bunker/surface.msk", 32, 0);
	for(x = 0; x != WINDOW_SCREEN_WIDTH; x += 64) {
		bobChangeFrame(pSurface, 1 + ((x & 64) == 64));
		bobDraw(pSurface, s_pBunkerBfr->pBuffer, x, GFX_SURFACE_OFFS);
	}
	bobChangeFrame(pSurface, 0);
	bobDraw(pSurface, s_pBunkerBfr->pBuffer, (WINDOW_SCREEN_WIDTH-64)/2, GFX_SURFACE_OFFS);
	bobUniqueDestroy(pSurface);
	
	// Load lamp & platform bobs
	s_pLamp = bobUniqueCreate("data/bunker/lamp.bm", "data/bunker/lamp.msk", 0, 0);
	s_pPlatform = bobUniqueCreate("data/bunker/platform.bm", "data/bunker/platform.msk", 0, 0);
	
	bunkerVehiclesCreate();
	logBlockEnd("bunkerCreate()");
}

void bunkerProcessChoice() {
	UBYTE ubSelectionChanged;
	UBYTE ubPrevChoice;
	
	// Process player input - selection change
	ubSelectionChanged = 1;
	ubPrevChoice = s_ubChoice;
	if(keyUse(OF_KEY_LEFT) && (s_ubChoice & 1))
		--s_ubChoice;
	else if(keyUse(OF_KEY_RIGHT) && !(s_ubChoice & 1))
		++s_ubChoice;
	else if(keyUse(OF_KEY_BACKWARD) && s_ubChoice < 2)
		s_ubChoice += 2;
	else if(keyUse(OF_KEY_FORWARD) && s_ubChoice >= 2)
		s_ubChoice -= 2;
	else if(keyUse(OF_KEY_ACTION1)) {
		if(g_pLocalPlayer->pVehiclesLeft[s_ubChoice]) {
			if(s_ubChoice > 1)
				s_ubMode = BUNKER_MODE_VEHICLE_TO_ELEVATOR;
			else
				s_ubMode = BUNKER_MODE_ELEVATOR_TO_VEHICLE;
			ubSelectionChanged = 0;
			playerSelectVehicle(g_pLocalPlayer, s_ubChoice);
		}
	}
	else if(keyUse(KEY_M)) {
		// TODO: loadMapPreview();
		// TODO: s_ubMode = BUNKER_MODE_MAP;
	}
	else
		ubSelectionChanged = 0;
	
	// Process animation
	if(ubSelectionChanged) {
		// Undraw lamp, prev & new vehicle
		if(g_pLocalPlayer->pVehiclesLeft[ubPrevChoice])
			bobUndraw(
				s_pVehicles[ubPrevChoice].pBob, s_pBunkerBfr->pBuffer
			);
		if(g_pLocalPlayer->pVehiclesLeft[s_ubChoice])
			bobUndraw(
				s_pVehicles[s_ubChoice].pBob,  s_pBunkerBfr->pBuffer
			);
		bobUndraw(s_pLamp,  s_pBunkerBfr->pBuffer);
		
		// Redraw lamp, prev & new vehicle
		bobDraw(
			s_pLamp, s_pBunkerBfr->pBuffer,
			s_pVehicles[s_ubChoice].uwX,
			s_pVehicles[s_ubChoice].uwY - GFX_LAMP_DELTAY
		);
		if(g_pLocalPlayer->pVehiclesLeft[ubPrevChoice])
			bobDraw(
				s_pVehicles[ubPrevChoice].pBob, s_pBunkerBfr->pBuffer,
				s_pVehicles[ubPrevChoice].uwX, s_pVehicles[ubPrevChoice].uwY
			);
		if(g_pLocalPlayer->pVehiclesLeft[s_ubChoice])
			bobDraw(
				s_pVehicles[s_ubChoice].pBob, s_pBunkerBfr->pBuffer,
				s_pVehicles[s_ubChoice].uwX, s_pVehicles[s_ubChoice].uwY
			);
	}
	// TODO: on timer - update lights on vehicle platform
}

void bunkerProcess(void) {
	
	if (keyUse(KEY_ESCAPE)) {
		gameClose();
		return;
	}
	
	switch(s_ubMode) {
		case BUNKER_MODE_CHOICE:
			bunkerProcessChoice();
			break;
		case BUNKER_MODE_MAP:
			if(keyUse(KEY_RETURN)) {
				// TODO: loadBunkerPreview();
				s_ubMode = BUNKER_MODE_CHOICE;
			}
			break;
		case BUNKER_MODE_ELEVATOR_TO_VEHICLE:
			bobUndraw(s_pPlatform, s_pBunkerBfr->pBuffer);
			--s_uwPlatformY;
			bobDraw(
				s_pPlatform,
				s_pBunkerBfr->pBuffer,
				s_pPlatform->uwPrevX,
				s_uwPlatformY
			);
			if(s_uwPlatformY == HANGAR_HI_Y) {
				s_ubMode = BUNKER_MODE_VEHICLE_TO_ELEVATOR;
			}
			++s_uwFrameCount;
			break;
		case BUNKER_MODE_VEHICLE_TO_ELEVATOR:
			if(s_pVehicles[s_ubChoice].uwX == s_pPlatform->uwPrevX) {
				s_ubMode = BUNKER_MODE_ELEVATOR_TO_SURFACE;
				break;
			}
			bobUndraw(s_pVehicles[s_ubChoice].pBob, s_pBunkerBfr->pBuffer);
			if(s_pVehicles[s_ubChoice].uwX > s_pPlatform->uwPrevX)
				--s_pVehicles[s_ubChoice].uwX;
			else
				++s_pVehicles[s_ubChoice].uwX;
			bobDraw(
				s_pVehicles[s_ubChoice].pBob, s_pBunkerBfr->pBuffer,
				s_pVehicles[s_ubChoice].uwX, s_pVehicles[s_ubChoice].uwY
			);
			++s_uwFrameCount;
			break;
		case BUNKER_MODE_ELEVATOR_TO_SURFACE:
			bobUndraw(s_pVehicles[s_ubChoice].pBob, s_pBunkerBfr->pBuffer);
			bobUndraw(s_pPlatform, s_pBunkerBfr->pBuffer);
			--s_pVehicles[s_ubChoice].uwY;
			--s_uwPlatformY;
			bobDraw(s_pVehicles[s_ubChoice].pBob, s_pBunkerBfr->pBuffer, s_pVehicles[s_ubChoice].uwX, s_pVehicles[s_ubChoice].uwY);
			bobDraw(s_pPlatform, s_pBunkerBfr->pBuffer, s_pPlatform->uwPrevX, s_uwPlatformY);
			if(s_uwFrameCount > BUNKER_ANIM_FRAMES - BUNKER_FADE_FRAMES) {
				// TODO: fade
			}
			if(s_uwFrameCount >= BUNKER_ANIM_FRAMES) {
				bunkerHide();
			}
			++s_uwFrameCount;
			break;
	}
			
	// Animate platform & vehicle carry
	viewProcessManagers(s_pBunkerView);
	copProcessBlocks();
	WaitTOF();
}

void bunkerShow(void) {
	UBYTE i;
	
	logBlockBegin("bunkerShow()");
	// Reset vehicle selection & platform state
	s_ubChoice = 0;
	while(!g_pLocalPlayer->pVehiclesLeft[s_ubChoice] && s_ubChoice < 4)
		++s_ubChoice;
	if(s_ubChoice == 4) {
		// TODO: game over
		s_ubChoice = 0;
	}
	s_ubMode = BUNKER_MODE_CHOICE;
	s_uwFrameCount = 0;
	s_uwPlatformY = HANGAR_LO_Y;
	
	bunkerVehiclesResetPos(); // Needed here for lamp pos
	
	// Draw platform & lamp
	logWrite("Drawing platform & lamp\n");
	bobDraw(
		s_pPlatform, s_pBunkerBfr->pBuffer,
		(WINDOW_SCREEN_WIDTH - (s_pPlatform->pBitmap->BytesPerRow << 3))/2, HANGAR_LO_Y
	);
	bobDraw(
		s_pLamp, s_pBunkerBfr->pBuffer,
		s_pVehicles[s_ubChoice].uwX,
		s_pVehicles[s_ubChoice].uwY - GFX_LAMP_DELTAY
	);
	
	// Draw vehicles (count)
	logWrite("Drawing vehicles\n");
	for(i = 0; i != 4; ++i)
		if(g_pLocalPlayer->pVehiclesLeft[i])
			bobDraw(
				s_pVehicles[i].pBob, s_pBunkerBfr->pBuffer,
				s_pVehicles[i].uwX, s_pVehicles[i].uwY
			);
	
	viewLoad(s_pBunkerView);
	gameChangeLoop(bunkerProcess);
	logBlockEnd("bunkerShow()");
}

void bunkerHide(void) {
	UBYTE i;
	
	logBlockBegin("bunkerHide()");
	bobUndraw(s_pPlatform, s_pBunkerBfr->pBuffer);
	for(i = 0; i != 4; ++i)
		if(g_pLocalPlayer->pVehiclesLeft[i])
			bobUndraw(s_pVehicles[i].pBob, s_pBunkerBfr->pBuffer);
	bobUndraw(s_pLamp, s_pBunkerBfr->pBuffer);
	viewLoad(g_pGameView);
	gameChangeLoop(worldGameLoop);	
	logBlockEnd("bunkerHide()");
}

void bunkerDestroy(void) {
	UBYTE i;
	
	logBlockBegin("bunkerDestroy()");
	viewLoad(0);
	viewDestroy(s_pBunkerView);
	bobUniqueDestroy(s_pLamp);
	bobUniqueDestroy(s_pPlatform);
	// Free vehicles bob
	bunkerVehiclesDestroy();
	bitmapMaskDestroy(s_pVehiclesMask);
	bitmapDestroy(s_pVehiclesBitmap);
	logBlockEnd("BunkerDestroy()");
}