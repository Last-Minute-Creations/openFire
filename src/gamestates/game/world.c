#include "gamestates/game/world.h"
#include <ace/managers/log.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/copper.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/palette.h>
#include <ace/utils/custom.h>
#include "gamestates/game/game.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/hud.h"
#include "gamestates/game/turret.h"

// Viewport stuff
tView *g_pWorldView;
tVPort *s_pWorldMainVPort;
tSimpleBufferManager *g_pWorldMainBfr;
tCameraManager *g_pWorldCamera;
tBitMap *s_pTiles;

// Silo highlight
// TODO: struct?
tBob *s_pSiloHighlight;
UBYTE s_ubWasSiloHighlighted;
UBYTE g_ubDoSiloHighlight;
UWORD g_uwSiloHighlightTileY;
UWORD g_uwSiloHighlightTileX;

// Crosshair
tBitMap *s_pCrosshair;

void updateCrosshair(void) {
	UWORD *pSpriteBfr = (UWORD*)s_pCrosshair->Planes[0];
	pSpriteBfr[0] = ((0x2B-4 + g_uwMouseY   ) << 8) | 64-(4>>1) + (g_uwMouseX >> 1);
	pSpriteBfr[1] = ((0x2B-4 + g_uwMouseY+11) << 8) |             (g_uwMouseX & 1);
}

UBYTE worldCreate(void) {
	// Prepare view & viewport
	g_pWorldView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, WORLD_COP_SIZE,
		TAG_DONE
	);
	s_pWorldMainVPort = vPortCreate(0,
		TAG_VPORT_VIEW, g_pWorldView,
		TAG_VPORT_HEIGHT, WORLD_VPORT_HEIGHT,
		TAG_VPORT_BPP, WORLD_BPP,
		TAG_DONE
	);
	g_pWorldMainBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pWorldMainVPort,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, g_uwMapWidth,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, g_uwMapHeight,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, WORLD_COP_VPMAIN_POS,
		TAG_DONE
	);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return 0;
	}
	paletteLoad("data/amidb16.plt", s_pWorldMainVPort->pPalette, 16);
	paletteLoad("data/amidb16.plt", &s_pWorldMainVPort->pPalette[16], 16);
	g_pWorldCamera = g_pWorldMainBfr->pCameraManager;

	hudCreate();

	turretListCreate(128);

	// Load gfx
	s_pCrosshair = bitmapCreateFromFile("data/crosshair.bm");
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	s_pSiloHighlight = bobUniqueCreate("data/silohighlight.bm", "data/silohighlight.msk", 0, 0);

	// Draw map
	mapSetSrcDst(s_pTiles, g_pWorldMainBfr->pBuffer);
	mapRedraw();

	// Crosshair stuff
	UWORD *pSpriteBfr = (UWORD*)s_pCrosshair->Planes[0];
	tCopCmd *pCrossList = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CROSS_POS];
	updateCrosshair();
	copSetWait(&pCrossList[0].sWait, 0, 0);
	copSetMove(&pCrossList[1].sMove, &pSprPtrs[3].uwHi, (ULONG)((UBYTE*)pSpriteBfr) >> 16);
	copSetMove(&pCrossList[2].sMove, &pSprPtrs[3].uwLo, (ULONG)((UBYTE*)pSpriteBfr) & 0xFFFF);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CROSS_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_CROSS_POS],
		3*sizeof(tCopCmd)
	);

	// Initial values
	s_ubWasSiloHighlighted = 0;
	g_ubDoSiloHighlight = 0;
	return 1;
}

void worldDestroy(void) {
	turretListDestroy();

	viewDestroy(g_pWorldView);
	bobUniqueDestroy(s_pSiloHighlight);
	bitmapDestroy(s_pTiles);
	bitmapDestroy(s_pCrosshair);
}

void worldShow(void) {
	viewLoad(g_pWorldView);
}

void worldHide(void) {
	bunkerShow();
}

void worldDraw(void) {
	UBYTE ubPlayer, ubTurret;

	// Silo highlight
	if(g_ubDoSiloHighlight)
		bobDraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer,
			g_uwSiloHighlightTileX << MAP_TILE_SIZE,
			g_uwSiloHighlightTileY << MAP_TILE_SIZE
		);

	// Vehicles
	for(ubPlayer = 0; ubPlayer != g_ubPlayerLimit; ++ubPlayer)
		vehicleDraw(&g_pPlayers[ubPlayer].sVehicle);

	// Turrets
	turretUpdateSprites();

	// Projectiles
	projectileDraw();

	s_ubWasSiloHighlighted = g_ubDoSiloHighlight;
}

void worldUndraw(void) {
	UBYTE ubPlayer;

	// Projectiles
	projectileUndraw();

	// Vehicles
	for(ubPlayer = g_ubPlayerLimit; ubPlayer--;)
		vehicleUndraw(&g_pPlayers[ubPlayer].sVehicle);

	// Silo highlight
	if(s_ubWasSiloHighlighted)
		bobUndraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer
		);
}

void worldProcess(void) {

	// World-specific & steering-irrelevant player input
	worldProcessInput();

	// TODO: Update HUD vport

	// Update main vport
	vPortWaitForEnd(s_pWorldMainVPort);
	worldUndraw();
	if(g_pLocalPlayer->sVehicle.ubLife) {
		UWORD uwLocalX, uwLocalY;
		uwLocalX = g_pLocalPlayer->sVehicle.fX;
		uwLocalY = g_pLocalPlayer->sVehicle.fY;
		cameraCenterAt(g_pWorldCamera, uwLocalX & 0xFFFE, uwLocalY);
	}
	mapUpdateTiles();
	worldDraw();

	if(keyUse(KEY_L))
		copDumpBfr(g_pWorldView->pCopList->pBackBfr);

	updateCrosshair();

	viewProcessManagers(g_pWorldView);
	copProcessBlocks();
}

void worldProcessInput(void) {
	if(keyUse(KEY_C))
		bitmapSaveBmp(g_pWorldMainBfr->pBuffer, s_pWorldMainVPort->pPalette, "bufDump.bmp");
}
