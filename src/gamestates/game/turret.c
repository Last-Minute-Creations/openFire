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
static tCopBlock *s_pTurretCopBlocks[TURRET_MAX_PROCESS_RANGE_Y][TURRET_SPRITE_HEIGHT]; // Approx. 10KiB
static tCopBlock *s_pInitCopBlock, *s_pCleanupCopBlock;

tBitMap *s_pTurretTest;

static tAvg *s_pAvg;

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
	s_pInitCopBlock = copBlockCreate(
		g_pWorldView->pCopList, 2, 0x48-3*4 - 3*4, WORLD_VPORT_BEGIN_Y - 1
	);
	copMove(g_pWorldView->pCopList, s_pInitCopBlock, &custom.spr[1].ctl, (1<<7) | 1);
	copMove(g_pWorldView->pCopList, s_pInitCopBlock, &custom.spr[0].ctl, 1);

	// Cleanup block for sprites trimmed from bottom
	s_pCleanupCopBlock = copBlockCreate(
		g_pWorldView->pCopList, 2, 0x48-2*4, WORLD_VPORT_BEGIN_Y + WORLD_VPORT_HEIGHT
	);
	copMove(g_pWorldView->pCopList, s_pCleanupCopBlock, &custom.spr[1].ctl, 1);
	copMove(g_pWorldView->pCopList, s_pCleanupCopBlock, &custom.spr[0].ctl, 1);

	// CopBlocks for turret display
	for(t = 0; t != TURRET_MAX_PROCESS_RANGE_Y; ++t) {
		for(i = 0; i != TURRET_SPRITE_HEIGHT; ++i)
			s_pTurretCopBlocks[t][i] = copBlockCreate(
				g_pWorldView->pCopList, WORLD_VPORT_WIDTH/8,
				0x48-3*4,WORLD_VPORT_BEGIN_Y + (t << MAP_TILE_SIZE) + i - 1
			);
	}

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

	// CopBlocks for turret display
	for(t = 0; t != TURRET_MAX_PROCESS_RANGE_Y; ++t) {
		for(i = 0; i != TURRET_SPRITE_HEIGHT; ++i)
			copBlockDestroy(g_pWorldView->pCopList, s_pTurretCopBlocks[t][i]);
	}
	copBlockDestroy(g_pWorldView->pCopList, s_pInitCopBlock);
	copBlockDestroy(g_pWorldView->pCopList, s_pCleanupCopBlock);

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

// Screen is 320x256, tiles are 32x32, hud 64px, so 10x6 tiles on screen
// Assuming that at most half of row tiles may have turret, 5x6 turret tiles
// But also, because of scroll, there'll be 7 turret rows and 6 cols.
void turretUpdateSprites(void) {
	logAvgBegin(s_pAvg);
	UWORD uwCopBlockIdx = 0;
	tCopList *pCopList = g_pWorldView->pCopList;
	tCopBlock *pCopBlock;
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
	UWORD uwFirstVisibleSpriteLine, uwLastVisibleSpriteLine; 
	// Iterators, counters, flags
	UWORD uwTileX, uwTileY, uwScreenLine;
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
	// TODO uwLastTileY could be decreased a bit too to (-1 := -8?) 'cuz of
	// 8 blank lines before sprite.
	uwFirstTileX = uwCameraX >> MAP_TILE_SIZE;
	uwFirstTileY = uwCameraY >> MAP_TILE_SIZE;
	uwLastTileX  = (uwCameraX + WORLD_VPORT_WIDTH -1-8) >> MAP_TILE_SIZE;
	uwLastTileY  = (uwCameraY + WORLD_VPORT_HEIGHT -1) >> MAP_TILE_SIZE;

	// Iterate thru visible tile rows
	for(uwTileY = uwFirstTileY; uwTileY <= uwLastTileY; ++uwTileY) {
		// Determine copperlist & sprite display lines
		wSpriteBeginOnScreenY = wSpriteOffsY + ((uwTileY-uwFirstTileY) << MAP_TILE_SIZE);
		if(wSpriteBeginOnScreenY < 0) {
			// Sprite is trimmed from top
			uwFirstVisibleSpriteLine = -wSpriteOffsY;
			if(uwFirstVisibleSpriteLine >= TURRET_SPRITE_HEIGHT) {
				// Row has only visible lines below turret sprite
				// Disable copper rows
				// TODO shouldn't be needed 'cuz it should cope under full load anyway
				UWORD uwSpriteLine;
				for(uwSpriteLine = 0; uwSpriteLine < TURRET_SPRITE_HEIGHT; ++uwSpriteLine)
					s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine]->ubDisabled = 1;
				continue;
			}
			
			uwLastVisibleSpriteLine = TURRET_SPRITE_HEIGHT-1;
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
				uwLastVisibleSpriteLine = wSpriteEndOnScreenY - wSpriteBeginOnScreenY;
			}
			else {
				// Sprite is not trimmed on bottom
				uwLastVisibleSpriteLine = TURRET_SPRITE_HEIGHT-1;
			}
		}

		// Iterate thru row's visible columns
		uwTurretsInRow = 0;
		for(uwTileX = uwFirstTileX; uwTileX <= uwLastTileX; ++uwTileX) {
			// Get turret from tile, skip if there is none
			if(s_pTurretTiles[uwTileX][uwTileY] == 0xFFFF)
				continue;
			pTurret = &s_pTurretList[s_pTurretTiles[uwTileX][uwTileY]];
			
			// Update turret sprites
			wSpriteBeginOnScreenX = ((uwTileX-uwFirstTileX) << MAP_TILE_SIZE) + wSpriteOffsX;
			wCopVPos = WORLD_VPORT_BEGIN_Y;
			wCopHPos = (0x48 + wSpriteBeginOnScreenX/2 - uwCopperInsCount*4);
			// always > 0, always < 0xE2 'cuz only 8px after 0xE2
			if(!uwTurretsInRow) {
				// Reset CopBlock cmd count
				for(uwScreenLine = wSpriteBeginOnScreenY; uwScreenLine <= wSpriteEndOnScreenY; ++uwScreenLine) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwScreenLine-wSpriteBeginOnScreenY];
					pCopBlock->ubDisabled = 0;
					pCopBlock->uwCurrCount = 0;
					
					// TODO then first turret without own WAIT cmd?
					// Will be fixed with static copperlists
					copBlockWait(pCopList, pCopBlock, wCopHPos - 2*4, wCopVPos + uwScreenLine);
				}
				// If sprite was trimmed from top, disable remaining copBlock lines
				while(uwScreenLine <= wSpriteBeginOnScreenY+15) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwScreenLine-wSpriteBeginOnScreenY];
					pCopBlock->ubDisabled = 1;
					++uwScreenLine;
				}
			}
			
			for(uwScreenLine = wSpriteBeginOnScreenY; uwScreenLine <= wSpriteEndOnScreenY; ++uwScreenLine) {
				const UWORD **pPlanes = (UWORD**)s_pTurretTest->Planes;
				// const UWORD **pPlanes = (UWORD**)g_sBrownTurretSource.pBitmap->Planes;
				pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwScreenLine-wSpriteBeginOnScreenY];
				// Do a WAIT
				tCopWaitCmd *pWaitCmd = (tCopWaitCmd*)&pCopBlock->pCmds[pCopBlock->uwCurrCount];
				copSetWait(pWaitCmd, wCopHPos, wCopVPos + uwScreenLine);
				// pWaitCmd->bfVE = 0; // VPOS could be ignored here
				++pCopBlock->uwCurrCount;
				// Add MOVEs
				uwSpriteLine = (uwFirstVisibleSpriteLine + uwScreenLine - wSpriteBeginOnScreenY)*(s_pTurretTest->BytesPerRow >> 1);
				// uwSpriteLine = (angleToFrame(pTurret->ubAngle)*TURRET_SPRITE_HEIGHT + uwFirstVisibleSpriteLine + uwScreenLine - wSpriteBeginOnScreenY)*(g_sBrownTurretSource.pBitmap->BytesPerRow >> 1);
				UWORD uwSpritePos = 63 + (wSpriteBeginOnScreenX >> 1); // No need for VPos 'cuz WAIT ensures same line
				copMove(pCopList, pCopBlock, &custom.spr[0].pos, uwSpritePos);
				copMove(pCopList, pCopBlock, &custom.spr[1].pos, uwSpritePos);
				copMove(pCopList, pCopBlock, &custom.spr[1].datab, pPlanes[3][uwSpriteLine]);
				copMove(pCopList, pCopBlock, &custom.spr[1].dataa, pPlanes[2][uwSpriteLine]);
				copMove(pCopList, pCopBlock, &custom.spr[0].datab, pPlanes[1][uwSpriteLine]);
				copMove(pCopList, pCopBlock, &custom.spr[0].dataa, pPlanes[0][uwSpriteLine]);
				// Avg turretUpdateSprites():  14.034 ms, min:  28.770 ms, max:  32.772 ms
			}
			++uwTurretsInRow;
			// Force 1 empty tile
			++uwTileX;
		}
		if(!uwTurretsInRow) {
			// Disable copper rows
			UWORD uwSpriteLine;
			for(uwSpriteLine = 0; uwSpriteLine < TURRET_SPRITE_HEIGHT; ++uwSpriteLine)
				s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine]->ubDisabled = 1;
		}
	}
	pCopList->ubStatus = STATUS_REORDER;
	logAvgEnd(s_pAvg);
}
