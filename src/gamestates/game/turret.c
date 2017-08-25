#include "gamestates/game/turret.h"

#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/custom.h>
#include <ace/libfixmath/fix16.h>
#include "gamestates/game/world.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/player.h"

#define TURRET_SPRITE_HEIGHT  16
#define TURRET_SPRITE_OFFS    ((1 << MAP_TILE_SIZE) - TURRET_SPRITE_HEIGHT)

#define TURRET_MAX_PROCESS_RANGE_Y ((WORLD_VPORT_HEIGHT>>MAP_TILE_SIZE) + 1)

static UBYTE s_ubMaxTurrets;
static tTurret *s_pTurretList;
tBobSource g_sBrownTurretSource, g_sGreenTurretSource;
static UWORD **s_pTurretTiles;                           // Approx. 2KiB

tBitMap *s_pTurretTest;

static tAvg *s_pAvg;

// Copperlist offsets
#define TURRET_COP_INIT_POS    14
#define TURRET_COP_CMDS_START  (TURRET_COP_INIT_POS+3)
#define TURRET_COP_CLEANUP_POS (4728-14-3)

void turretListCreate(UBYTE ubMaxTurrets) {
	int i, t;
	logBlockBegin("turretListCreate(ubMaxTurrets: %hu)", ubMaxTurrets);

	s_ubMaxTurrets = ubMaxTurrets;
	s_pTurretList = memAllocFastClear(ubMaxTurrets * sizeof(tTurret));

	// Tile-based turret list
	// TODO proper dimensions from map
	s_pTurretTiles = memAllocFast(sizeof(UWORD*) * 20);
	for(i = 0; i != 20; ++i) {
		s_pTurretTiles[i] = memAllocFast(sizeof(UWORD)*20);
		memset(s_pTurretTiles[i], 0xFF, sizeof(UWORD)*20);
	}

	// Attach sprites
	// TODO vehicle/turret jitter could be fixed by setting lsbit in ctl
	// accordingly. Updating this block would be needed. Could be easily done in
	// static copperlists.
	tCopCmd *pCopInit = &g_pWorldView->pCopList->pBackBfr->pList[TURRET_COP_INIT_POS];
	copSetWait(&pCopInit[0].sWait, 0x48-3*4 - 3*4, WORLD_VPORT_BEGIN_Y - 1);
	copSetMove(&pCopInit[1].sMove, &custom.spr[1].ctl, (1<<7) | 1);
	copSetMove(&pCopInit[2].sMove, &custom.spr[0].ctl, 1);
	// Same in front bfr
	pCopInit = &g_pWorldView->pCopList->pFrontBfr->pList[TURRET_COP_INIT_POS];
	copSetWait(&pCopInit[0].sWait, 0x48-3*4 - 3*4, WORLD_VPORT_BEGIN_Y - 1);
	copSetMove(&pCopInit[1].sMove, &custom.spr[1].ctl, (1<<7) | 1);
	copSetMove(&pCopInit[2].sMove, &custom.spr[0].ctl, 1);


	// Cleanup block for sprites trimmed from bottom
	tCopCmd *pCopCleanup = &g_pWorldView->pCopList->pBackBfr->pList[TURRET_COP_CLEANUP_POS];
	copSetWait(&pCopCleanup[0].sWait, 0x48-2*4, WORLD_VPORT_BEGIN_Y + WORLD_VPORT_HEIGHT-1);
	copSetMove(&pCopCleanup[1].sMove, &custom.spr[1].ctl, 1);
	copSetMove(&pCopCleanup[2].sMove, &custom.spr[0].ctl, 0);
	pCopCleanup = &g_pWorldView->pCopList->pFrontBfr->pList[TURRET_COP_CLEANUP_POS];
	copSetWait(&pCopCleanup[0].sWait, 0x48-2*4, WORLD_VPORT_BEGIN_Y + WORLD_VPORT_HEIGHT-1);
	copSetMove(&pCopCleanup[1].sMove, &custom.spr[1].ctl, 1);
	copSetMove(&pCopCleanup[2].sMove, &custom.spr[0].ctl, 0);

	s_pTurretTest = bitmapCreateFromFile("data/turrettest.bm");
	s_pAvg = logAvgCreate("turretUpdateSprites()", 50*5);

	// Force blank last line of turret gfx
	for(UWORD uwFrame = 0; uwFrame != VEHICLE_BODY_ANGLE_COUNT; ++uwFrame) {
		blitRect(
			g_sBrownTurretSource.pBitmap,
			0, TURRET_SPRITE_HEIGHT*uwFrame + TURRET_SPRITE_HEIGHT-1,
			16, 1, 0
		);
	}
	blitRect(
		s_pTurretTest, 0, TURRET_SPRITE_HEIGHT-1,
		16, 1, 0
	);

	logBlockEnd("turretListCreate()");
}

void turretListDestroy(void) {
	int i, t;

	logBlockBegin("turretListDestroy()");

	bitmapDestroy(s_pTurretTest);

	memFree(s_pTurretList, s_ubMaxTurrets * sizeof(tTurret));

	// Tile-based turret list
	for(i = 0; i != 20; ++i)
		memFree(s_pTurretTiles[i], sizeof(UWORD)*20);
	memFree(s_pTurretTiles, sizeof(UWORD*) * 20);

	logAvgDestroy(s_pAvg);

	logBlockEnd("turretListDestroy()");
}

UWORD turretCreate(UWORD uwTileX, UWORD uwTileY, UBYTE ubTeam) {
	tTurret *pTurret;
	UBYTE i;

	// Find next free turret
	for(i = 0; i != s_ubMaxTurrets; ++i)
		if(!s_pTurretList[i].ubHp)
			break;
	if(i == s_ubMaxTurrets)
		return TURRET_INVALID;
	pTurret = &s_pTurretList[i];

	// Initial values
	pTurret->uwX = (uwTileX << MAP_TILE_SIZE) + (1 << MAP_TILE_SIZE)/2;
	pTurret->uwY = (uwTileY << MAP_TILE_SIZE) + (1 << MAP_TILE_SIZE)/2;
	pTurret->ubHp = TURRET_MAX_HP;
	pTurret->ubTeam = ubTeam;
	pTurret->ubAngle = ANGLE_90;

	// Add to tile-based list
	s_pTurretTiles[uwTileX][uwTileY] = i;
	return i;
}

void turretDestroy(UWORD uwIdx) {
	UWORD uwTileX, uwTileY;
	tTurret *pTurret;

	if(uwIdx >= s_ubMaxTurrets) {
		logWrite("ERR: turretDestroy() - Index out of range %u\n", uwIdx);
		return;
	}
	pTurret = &s_pTurretList[uwIdx];

	// Remove from tile-based list
	uwTileX = pTurret->uwX >> MAP_TILE_SIZE;
	uwTileY = pTurret->uwY >> MAP_TILE_SIZE;
	s_pTurretTiles[uwTileX][uwTileY] = 0xFFFF;
}

void turretProcess(void) {
	UBYTE ubPlayerIdx, ubTurretIdx;
	tPlayer *pPlayer, *pClosestPlayer;
	UWORD uwClosestDist, uwDist, uwDx, uwDy;
	UBYTE ubDestAngle;
	tTurret *pTurret;

	for(ubTurretIdx = 0; ubTurretIdx != s_ubMaxTurrets; ++ubTurretIdx) {
		pTurret = &s_pTurretList[ubTurretIdx];

		if(!pTurret->ubHp)
			continue;

		// Scan nearby enemies
		uwClosestDist = TURRET_MIN_DISTANCE;
		pClosestPlayer = 0;
		for(ubPlayerIdx = 0; ubPlayerIdx != g_ubPlayerCount; ++ubPlayerIdx) {
			pPlayer = &g_pPlayers[ubPlayerIdx];

			// Same team or not on map?
			if(pPlayer->ubTeam == pTurret->ubTeam || pPlayer->ubState != PLAYER_STATE_DRIVING)
				continue;

			// Calculate distance between turret & player
			uwDx = pPlayer->sVehicle.fX - pTurret->uwX;
			uwDy = pPlayer->sVehicle.fY - pTurret->uwY;
			uwDist = fix16_to_int(fix16_sqrt(fix16_from_int(uwDx*uwDx + uwDy*uwDy)));
			if(uwDist < TURRET_MIN_DISTANCE && uwDist <= uwClosestDist) {
				pClosestPlayer = pPlayer;
				uwClosestDist = uwDist;
			}
		}

		// Anything in range?
		if(!pClosestPlayer)
			continue;
		uwDx = pClosestPlayer->sVehicle.fX - pTurret->uwX;
		uwDy = pClosestPlayer->sVehicle.fY - pTurret->uwY;

		// Determine destination angle
		// calc: ubDestAngle = ((pi + atan2(uwDx, uwDy)) * 64)/(2*pi) * 2
		ubDestAngle = ANGLE_90 + 2 * fix16_to_int(
			fix16_div(
				fix16_mul(
					fix16_add(fix16_pi, fix16_atan2(fix16_from_int(uwDx), fix16_from_int(-uwDy))),
					fix16_from_int(64)
				),
				fix16_pi*2
			)
		);
		if(ubDestAngle >= ANGLE_360)
			ubDestAngle -= ANGLE_360;

		if(pTurret->ubAngle != ubDestAngle) {
			WORD wDelta;
			wDelta = ubDestAngle - pTurret->ubAngle;
			if((wDelta > 0 && wDelta < ANGLE_180) || wDelta + ANGLE_360 < ANGLE_180) {
				// Rotate clockwise
				pTurret->ubAngle += 2;
			}
			else {
				// Rotate anti-clockwise
				pTurret->ubAngle += ANGLE_360 - 2;
			}
			while(pTurret->ubAngle >= ANGLE_360)
				pTurret->ubAngle -= ANGLE_360;
		}
		else {
			// TODO: Fire
			// projectileCreate();
		}
	}
}

/**
 *  Copies data using blitter using A->D channels.
 *  Word count is not flat but rectangular because of blitter internal workings.
 *  It could be converted to flat model but why bother abstraction if copy size
 *  is already almost always result of some kind of multiplication, so this time
 *  you get one for (almost) free!
 *  OpenFire rationale:
 *  One line takes 6*7=42 copper cmds, so 84 words.
 *  4*84/7.16 = 46,9us per line, so filling 15 lines should take 704us.
 *  There are gonna be up to rows on screen, so 4 turet rows will take 2,8ms.
 *  @param pSrc       Source data.
 *  @param pDst       Destination data.
 *  @param uwRowWidth Number of words to copy per 'row', max 64.
 *  @param uwRowCnt   Number of 'rows' to copy. Max 1024
 */
 void copyWithBlitter(UBYTE *pSrc, UBYTE *pDst, UWORD uwRowWidth, UWORD uwRowCnt) {
	// Modulo: 0, 'cuz 2nd line will go from 1st, 3rd from 2nd etc.
	g_sBlitManager.pBlitterSetFn(
		USEA|USED|MINTERM_A, 0,      // bltconX
		0xFFFF, 0xFFFF,              // bltaXwm
		0, 0, 0, 0,                  // bltXmod
		pSrc, 0, 0, pDst,            // bltxpt
		0, 0, 0,                     // bltXdat
		(uwRowCnt << 6) | uwRowWidth // bltsize
	);
}

// Screen is 320x256, tiles are 32x32, hud 64px, so 10x6 tiles on screen
// Assuming that at most half of row tiles may have turret, 5x6 turret tiles
// But also, because of scroll, there'll be 7 turret rows and 6 cols.
void turretUpdateSprites(void) {
	logAvgBegin(s_pAvg);
	tTurret *pTurret;
	UWORD uwSpriteLine;
	const UWORD uwCopperInsCount = 8;
	WORD wCopVPos, wCopHPos;

	// Tiles range to process
	UWORD uwFirstTileX, uwFirstTileY, uwLastTileX, uwLastTileY;
	// Offsets of sprite' beginning from top-left corner
	WORD wSpriteOffsX, wSpriteOffsY;
	// Sprite's pos on screen
	WORD wSpriteBeginOnScreenY, wSpriteEndOnScreenY;
	WORD wSpriteBeginOnScreenX;
	// Sprite's visible lines
	UWORD uwFirstVisibleSpriteLine;
	// Iterators, counters, flags
	UWORD uwTileX, uwTileY;
	UWORD uwTurretsInRow;

	UWORD uwCameraX = g_pWorldCamera->uPos.sUwCoord.uwX;
	UWORD uwCameraY = g_pWorldCamera->uPos.sUwCoord.uwY;
	// This is equivalent to number of cols/rows trimmed by view, if negative
	// OffsX needs to be even 'cuz sprite pos lsbit is always 0
	wSpriteOffsX = ((TURRET_SPRITE_OFFS>>1) - (uwCameraX & ((1 << MAP_TILE_SIZE) -1))) & 0xfffe;
	wSpriteOffsY = (TURRET_SPRITE_OFFS>>1) - (uwCameraY & ((1 << MAP_TILE_SIZE) -1));

	// Tile range to be checked
	// If last sprite begins just at bottom-right end of screen, it garbles
	// HUD's copperlist. So uwLastTileX is trimmed so that it won't try to display
	// last sprite if there is only a bit of it.
	uwFirstTileX = uwCameraX >> MAP_TILE_SIZE;
	uwFirstTileY = uwCameraY >> MAP_TILE_SIZE;
	uwLastTileX  = (uwCameraX + WORLD_VPORT_WIDTH -1-8) >> MAP_TILE_SIZE;
	uwLastTileY  = (uwCameraY + WORLD_VPORT_HEIGHT -1-8) >> MAP_TILE_SIZE;
	UWORD uwCopOffs = TURRET_COP_CMDS_START;
	UWORD uwRowStartCopOffs = 0;
	tCopList *pCopList = g_pWorldView->pCopList;
	tCopCmd *pCmdList = pCopList->pBackBfr->pList;

	// Iterate thru visible tile rows
	UWORD **pTurretPlanes[6];
	for(uwTileY = uwFirstTileY; uwTileY <= uwLastTileY; ++uwTileY) {
		// Determine copperlist & sprite display lines
		wSpriteBeginOnScreenY = wSpriteOffsY + ((uwTileY-uwFirstTileY) << MAP_TILE_SIZE);
		if(wSpriteBeginOnScreenY < 0) {
			// Sprite is trimmed from top
			uwFirstVisibleSpriteLine = -wSpriteOffsY;
			if(uwFirstVisibleSpriteLine >= TURRET_SPRITE_HEIGHT) {
				// Row has only visible lines below screen
				continue;
			}
			wSpriteBeginOnScreenY = 0;
			wSpriteEndOnScreenY = wSpriteOffsY + TURRET_SPRITE_HEIGHT-1;
		}
		else {
			// Sprite is not trimmed on top - may be on bottom
			uwFirstVisibleSpriteLine = 0;
			wSpriteEndOnScreenY = wSpriteBeginOnScreenY + TURRET_SPRITE_HEIGHT-1;
			if(wSpriteEndOnScreenY >= WORLD_VPORT_HEIGHT) {
				// Sprite is trimmed on bottom
				wSpriteEndOnScreenY = WORLD_VPORT_HEIGHT-1;
			}
		}

		// Iterate thru row's visible columns
		uwTurretsInRow = 0;
		uwRowStartCopOffs = uwCopOffs;
		wCopVPos = WORLD_VPORT_BEGIN_Y + wSpriteBeginOnScreenY;
		uwSpriteLine = uwFirstVisibleSpriteLine*(s_pTurretTest->BytesPerRow >> 1);
		// uwSpriteLine = (angleToFrame(pTurret->ubAngle)*TURRET_SPRITE_HEIGHT + uwFirstVisibleSpriteLine)*(g_sBrownTurretSource.pBitmap->BytesPerRow >> 1);
		for(uwTileX = uwFirstTileX; uwTileX <= uwLastTileX; ++uwTileX) {
			// Get turret from tile, skip if there is none
			if(s_pTurretTiles[uwTileX][uwTileY] == 0xFFFF)
				continue;
			pTurret = &s_pTurretList[s_pTurretTiles[uwTileX][uwTileY]];

			// Get proper turret sprite data
			// 0 < wCopHPos < 0xE2 always 'cuz only 8px after 0xE2
			wSpriteBeginOnScreenX = ((uwTileX-uwFirstTileX) << MAP_TILE_SIZE) + wSpriteOffsX;
			wCopHPos = (0x48 + wSpriteBeginOnScreenX/2 - uwCopperInsCount*4);

			// Get turret gfx
			const UWORD **pPlanes = (UWORD**)s_pTurretTest->Planes;
			// const UWORD **pPlanes = (UWORD**)g_sBrownTurretSource.pBitmap->Planes;
			pTurretPlanes[uwTurretsInRow] = pPlanes;

			// Do a WAIT
			// TODO: VPOS could be ignored here and only set on 1st line later
			copSetWait(&pCmdList[uwCopOffs+0].sWait, wCopHPos, wCopVPos);
			// pCmdList[uwCopOffs+0].sWait.bfVE = 0;

			// Add MOVEs - no need setting VPos 'cuz WAIT ensures same line
			UWORD uwSpritePos = 63 + (wSpriteBeginOnScreenX >> 1);
			copSetMove(&pCmdList[uwCopOffs+1].sMove, &custom.spr[0].pos, uwSpritePos);
			copSetMove(&pCmdList[uwCopOffs+2].sMove, &custom.spr[1].pos, uwSpritePos);
			copSetMove(&pCmdList[uwCopOffs+3].sMove, &custom.spr[1].datab, pPlanes[3][uwSpriteLine]);
			copSetMove(&pCmdList[uwCopOffs+4].sMove, &custom.spr[1].dataa, pPlanes[2][uwSpriteLine]);
			copSetMove(&pCmdList[uwCopOffs+5].sMove, &custom.spr[0].datab, pPlanes[1][uwSpriteLine]);
			copSetMove(&pCmdList[uwCopOffs+6].sMove, &custom.spr[0].dataa, pPlanes[0][uwSpriteLine]);
			++uwTurretsInRow;
			// Force 1 empty tile
			++uwTileX;
			uwCopOffs += 7;
		}
		if(!uwTurretsInRow)
			continue;
		WORD wRowsToCopy = wSpriteEndOnScreenY - wSpriteBeginOnScreenY;
		UWORD uwCmdsPerRow = (uwCopOffs-uwRowStartCopOffs);
		if(wRowsToCopy <= 0)
			continue;
		// Copy rows using blitter
		copyWithBlitter(
			(UBYTE*)&pCmdList[uwRowStartCopOffs],	(UBYTE*)&pCmdList[uwCopOffs],
			uwCmdsPerRow, wRowsToCopy*(sizeof(tCopCmd)/sizeof(UWORD))
		);
		WaitBlit();
		// TODO: WAIT could be optimized by copying one with VPOS compare disabled
		// and then re-enabled only in topmost line.
		// NOTE NOTE NOTE This should wait for blitter to finish or at least
		// give it a bit of headstart
		for(UWORD uwRow = 0; uwRow != wRowsToCopy; ++uwRow) {
			uwSpriteLine += s_pTurretTest->BytesPerRow >> 1;
			++wCopVPos;
			for(UWORD i = 0; i != uwTurretsInRow; ++i) {
				// Get turret gfx
				const UWORD **pPlanes = pTurretPlanes[i];

				pCmdList[uwCopOffs+0].sWait.bfWaitY = wCopVPos;                 // WAIT Y
				pCmdList[uwCopOffs+3].sMove.bfValue = pPlanes[3][uwSpriteLine]; // spr1datb
				pCmdList[uwCopOffs+4].sMove.bfValue = pPlanes[2][uwSpriteLine]; // spr1data
				pCmdList[uwCopOffs+5].sMove.bfValue = pPlanes[1][uwSpriteLine]; // spr0datb
				pCmdList[uwCopOffs+6].sMove.bfValue = pPlanes[0][uwSpriteLine]; // spr0data
				uwCopOffs += 7;
			}
		}
	}

	// Jump to cleanup if not completely filled
	if(uwCopOffs < TURRET_COP_CLEANUP_POS) {
		ULONG ulCleanupPos = (ULONG)((void*)&pCmdList[TURRET_COP_CLEANUP_POS]);
		copSetMove(&pCmdList[uwCopOffs+0].sMove, &pCopLc[1].uwHi, ulCleanupPos>>16);
		copSetMove(&pCmdList[uwCopOffs+1].sMove, &pCopLc[1].uwLo, ulCleanupPos & 0xFFFF);
		copSetMove(&pCmdList[uwCopOffs+2].sMove, &custom.copjmp2, 1);
	}

	logAvgEnd(s_pAvg);
	// Avg turretUpdateSprites():   5.357 ms, min:   2.640 ms, max:   7.499 ms
}
