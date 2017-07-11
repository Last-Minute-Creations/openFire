#include "gamestates/game/turret.h"

#include <ace/managers/memory.h>
#include <ace/managers/key.h>
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

void turretListCreate(UBYTE ubMaxTurrets) {
	int i, t;
	logBlockBegin("turretListCreate(ubMaxTurrets: %hu)", ubMaxTurrets);

	s_ubMaxTurrets = ubMaxTurrets;
	s_pTurretList = memAllocFast(ubMaxTurrets * sizeof(tTurret));
	// Flat turret list
	for(i = 0; i != s_ubMaxTurrets; ++i) {
		s_pTurretList[i].pBob = bobCreate(
			g_sBrownTurretSource.pBitmap, g_sBrownTurretSource.pMask,
			TURRET_BOB_SIZE, 0
		);
		s_pTurretList[i].pBob->ubFlags = BOB_FLAG_NODRAW;
	}
	// Tile-based turret list
	// TODO proper dimensions
	s_pTurretTiles = memAllocFast(sizeof(UWORD*) * 20);
	for(i = 0; i != 20; ++i) {
		s_pTurretTiles[i] = memAllocFast(sizeof(UWORD)*20);
		memset(s_pTurretTiles[i], 0xFF, sizeof(UWORD)*20);
	}

	// CopBlocks for turret display
	// TODO: more precise copper instruction count?
	for(t = 0; t != TURRET_MAX_PROCESS_RANGE_Y; ++t) {
		for(i = 0; i != TURRET_SPRITE_HEIGHT; ++i)
			s_pTurretCopBlocks[t][i] = copBlockCreate(
				g_pWorldView->pCopList, WORLD_VPORT_WIDTH/8,
				0xE2-3*4,WORLD_VPORT_BEGIN_Y + (t << MAP_TILE_SIZE) + i - 1
			);
	}

	logBlockEnd("turretListCreate()");
}

void turretListDestroy(void) {
	int i, t;

	logBlockBegin("turretListDestroy()");

	for(i = 0; i != s_ubMaxTurrets; ++i) {
		bobDestroy(s_pTurretList[i].pBob);
	}
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
	bobChangeFrame(pTurret->pBob, angleToFrame(pTurret->ubAngle));
	pTurret->pBob->ubFlags = BOB_FLAG_START_DRAWING;

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
	pTurret->pBob->ubFlags = BOB_FLAG_STOP_DRAWING;

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
			// TODO uncomment
			if(/*pPlayer->ubTeam == pTurret->ubTeam ||*/ pPlayer->ubState != PLAYER_STATE_DRIVING)
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
		// ubDestAngle = ((pi + atan2(uwDx, uwDy)) * 64)/(2*pi) * 2
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
			// TODO: Rotate turret into enemy position
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
			bobChangeFrame(pTurret->pBob, angleToFrame(pTurret->ubAngle));
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
	UWORD uwCopBlockIdx = 0;
	tCopList *pCopList = g_pWorldView->pCopList;
	tCopBlock *pCopBlock;
	UWORD uwDebug = 0;

	if(keyUse(KEY_L)) {
		uwDebug = 1;
		logBlockBegin("turretUpdateSprites");
	}

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
	UWORD uwTileX, uwTileY, uwSpriteLine;
	UWORD uwPrevTurretIs2TilesAway;
	UWORD uwTurretsInRow;

	UWORD uwCameraX = g_pWorldCamera->uPos.sUwCoord.uwX;
	UWORD uwCameraY = g_pWorldCamera->uPos.sUwCoord.uwY;
	// This is equivalent to number of cols/rows trimmed by view, if negative
	wSpriteOffsX = (TURRET_SPRITE_OFFS>>1) - (uwCameraX & ((1 << MAP_TILE_SIZE) -1));
	wSpriteOffsY = (TURRET_SPRITE_OFFS>>1) - (uwCameraY & ((1 << MAP_TILE_SIZE) -1));

	// Tile range to be checked
	uwFirstTileX = uwCameraX >> MAP_TILE_SIZE;
	uwFirstTileY = uwCameraY >> MAP_TILE_SIZE;
	uwLastTileX  = (uwCameraX + WORLD_VPORT_WIDTH -1) >> MAP_TILE_SIZE;
	uwLastTileY  = (uwCameraY + WORLD_VPORT_HEIGHT -1) >> MAP_TILE_SIZE;

	if(uwDebug)
		logWrite("Camera pos %u,%u, so scanning tiles: %u,%u to %u,%u\n", uwCameraX, uwCameraY, uwFirstTileX, uwFirstTileY, uwLastTileX, uwLastTileY);

	// Iterate thru visible tile rows
	for(uwTileY = uwFirstTileY; uwTileY <= uwLastTileY; ++uwTileY) {
		// Determine copperlist & sprite display lines
		wSpriteBeginOnScreenY = wSpriteOffsY + ((uwTileY-uwFirstTileY) << MAP_TILE_SIZE);
		if(wSpriteBeginOnScreenY < 0) {
			// Sprite is trimmed from top
			uwFirstVisibleSpriteLine = -wSpriteOffsY;
			uwLastVisibleSpriteLine = TURRET_SPRITE_HEIGHT-1;
			wSpriteBeginOnScreenY = 0;
			wSpriteEndOnScreenY = wSpriteOffsY + TURRET_SPRITE_HEIGHT;
		}
		else {
			// Sprite is not trimmed on top - may be on bottom
			uwFirstVisibleSpriteLine = 0;
			wSpriteEndOnScreenY = wSpriteBeginOnScreenY + TURRET_SPRITE_HEIGHT-1;
			if(wSpriteEndOnScreenY >= WORLD_VPORT_HEIGHT) {
				// Sprite is trimmed on bottom
				uwLastVisibleSpriteLine = TURRET_SPRITE_HEIGHT + wSpriteEndOnScreenY - (WORLD_VPORT_HEIGHT-1);
				wSpriteEndOnScreenY = WORLD_VPORT_HEIGHT-1;
			}
			else {
				// Sprite is not trimmed on bottom
				uwLastVisibleSpriteLine = TURRET_SPRITE_HEIGHT-1;
			}
		}

		// Iterate thru row's visible columns
		uwTurretsInRow = 0;
		uwPrevTurretIs2TilesAway = 0;
		if(uwDebug) {
			logWrite("Y tile: %u, line: %u\n", uwTileY, wSpriteBeginOnScreenY);
		}
		for(uwTileX = uwFirstTileX; uwTileX <= uwLastTileX; ++uwTileX) {
			// No turret?
			if(s_pTurretTiles[uwTileX][uwTileY] == 0xFFFF) {
				uwPrevTurretIs2TilesAway = 0;
				continue;
			}

			// TODO sprites should alternate
			// Update turret sprites
			wSpriteBeginOnScreenX = ((uwTileX-uwFirstTileX) << MAP_TILE_SIZE) + wSpriteOffsX;
			if(uwDebug) {
				logWrite("Found turret at %u,%u, onscreen pos: %d,%d\n", uwTileX, uwTileY, wSpriteBeginOnScreenX, wSpriteBeginOnScreenY);
			}
			if(!uwTurretsInRow) {
				// Reset CopBlock cmd count
				if(uwDebug)
					logWrite("Copper lines %d,%d are enabled\n", wSpriteBeginOnScreenY, wSpriteEndOnScreenY);
				for(uwSpriteLine = wSpriteBeginOnScreenY; uwSpriteLine <= wSpriteEndOnScreenY; ++uwSpriteLine) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine-wSpriteBeginOnScreenY];
					pCopBlock->ubDisabled = 0;
					pCopBlock->uwCurrCount = 0;
					
					copBlockWait(pCopList, pCopBlock, 0xE2 - 3*4, WORLD_VPORT_BEGIN_Y + uwSpriteLine - 1);
				}
				if(uwDebug) {
					if(uwSpriteLine <= wSpriteBeginOnScreenY+15)
						logWrite("Trailing offscreen copper lines %d,%d are disabled\n", uwSpriteLine, wSpriteBeginOnScreenY+15);
					else
						logWrite("No trailing offscreen copper lines\n");
				}
				while(uwSpriteLine <= wSpriteBeginOnScreenY+15) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine-wSpriteBeginOnScreenY];
					pCopBlock->ubDisabled = 1;
					++uwSpriteLine;
				}
			}
			//if(uwPrevTurretIs2TilesAway) {
			if(0) {
				for(uwSpriteLine = wSpriteBeginOnScreenY; uwSpriteLine <= wSpriteEndOnScreenY; ++uwSpriteLine) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine-wSpriteBeginOnScreenY];
					// Add directly MOVEs 'cuz there's no time for anything else
					copMove(pCopList, pCopBlock, &custom.spr[0].pos, (0x44 + (wSpriteBeginOnScreenY << 8)) |wSpriteBeginOnScreenX);
					copMove(pCopList, pCopBlock, &custom.spr[0].datab, 0xFFFF);
					copMove(pCopList, pCopBlock, &custom.spr[0].dataa, 0x8001);
				}
			}
			else {
				WORD wCopVPos = WORLD_VPORT_BEGIN_Y;
				WORD wCopHPos = (0x48 + (wSpriteBeginOnScreenX/2 - 4*4) & 0xfffe);
				if(wCopHPos < 0) {
					wCopHPos += 0xE2;
					--wCopVPos;
				}
				else if(wCopHPos >= 0xE2)
					wCopHPos -= 0xE2;
				for(uwSpriteLine = wSpriteBeginOnScreenY; uwSpriteLine <= wSpriteEndOnScreenY; ++uwSpriteLine) {
					pCopBlock = s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine-wSpriteBeginOnScreenY];
					// Do a WAIT
					// FIXME copper coords are not screen coords
					// VPOS could be ignored here 
					tCopWaitCmd *pWaitCmd = (tCopWaitCmd*)&pCopBlock->pCmds[pCopBlock->uwCurrCount];
					copSetWait(pWaitCmd, wCopHPos, wCopVPos + uwSpriteLine);
					// pWaitCmd->bfVE = 0;
					++pCopBlock->uwCurrCount;
					// Add MOVEs
					// copMove(
					// 	pCopList, pCopBlock, &custom.spr[0].pos,
					// 	((44 + wSpriteBeginOnScreenY) << 8) | ((0x48 + wSpriteBeginOnScreenX) >> 1)
					// );
					copMove(
						pCopList, pCopBlock, &custom.spr[0].pos,
						/*((44 + wSpriteBeginOnScreenY) << 8) |*/ (63 + (wSpriteBeginOnScreenX >> 1))
					);
					if(uwSpriteLine < wSpriteEndOnScreenY) {
						copMove(pCopList, pCopBlock, &custom.spr[0].datab, 0xFFFF);
						copMove(pCopList, pCopBlock, &custom.spr[0].dataa, 0x8001);
					}
					else {
						copMove(pCopList, pCopBlock, &custom.spr[0].datab, 0);
						copMove(pCopList, pCopBlock, &custom.spr[0].dataa, 0);
					}
				}
			}
			++uwTurretsInRow;
			// Force 1 empty tile
			++uwTileX;
			uwPrevTurretIs2TilesAway = 1;
		}
		if(!uwTurretsInRow) {
			// Disable copper rows
			if(uwDebug)
				logWrite("All copper lines are disabled 'cuz no turret\n");
			for(uwSpriteLine = 0; uwSpriteLine < TURRET_SPRITE_HEIGHT; ++uwSpriteLine)
				s_pTurretCopBlocks[uwTileY-uwFirstTileY][uwSpriteLine]->ubDisabled = 1;
		}
	}
	pCopList->ubStatus = STATUS_REORDER;
	if(uwDebug) {
		logWrite(
			"Processed %hu rows: %hu,%hu; max range: %hu\n",
			(uwLastTileY - uwFirstTileY + 1), uwFirstTileY, uwLastTileY,
			TURRET_MAX_PROCESS_RANGE_Y
		);
		logBlockEnd("turretUpdateSprites()");
	}
}

void turretDrawAll(void) {
	turretUpdateSprites();
	return;
	WORD uwTurret;
	tTurret *pTurret;
	for(uwTurret = 0; uwTurret != s_ubMaxTurrets; ++uwTurret) {
		pTurret = &s_pTurretList[uwTurret];
		bobDraw(
			pTurret->pBob,
			g_pWorldMainBfr->pBuffer,
			pTurret->uwX - TURRET_BOB_SIZE/2,
			pTurret->uwY - TURRET_BOB_SIZE/2
		);
	}

}

void turretUndrawAll(void) {
	return;
	UWORD uwTurret;
	for(uwTurret = 0; uwTurret != s_ubMaxTurrets; ++uwTurret) {
		bobUndraw(
			s_pTurretList[uwTurret].pBob,
			g_pWorldMainBfr->pBuffer
		);
	}
}
