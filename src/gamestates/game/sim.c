#include "gamestates/game/sim.h"
#include <ace/types.h>
#include "gamestates/game/player.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/world.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/team.h"

#include <ace/managers/key.h>

void simPlayerVehicle(tPlayer *pPlayer) {
	tVehicle *pVehicle;
	UWORD uwVx, uwVy, uwVTileX, uwVTileY;
	UWORD uwSiloDx, uwSiloDy;
	UBYTE ubTileType;

	pVehicle = &pPlayer->sVehicle;

	uwVx = pVehicle->fX;
	uwVy = pVehicle->fY;
	uwVTileX = uwVx >> MAP_TILE_SIZE;
	uwVTileY = uwVy >> MAP_TILE_SIZE;
	ubTileType = g_pMap[uwVTileX][uwVTileY].ubIdx;

	// Drowning
	if(ubTileType == MAP_LOGIC_WATER) {
		playerLoseVehicle(pPlayer);
		return;
	}

	// Process standing on silos
	g_ubDoSiloHighlight = 0;
	if(ubTileType == MAP_LOGIC_SPAWN1 || ubTileType == MAP_LOGIC_SPAWN2) {
		if(0) {
			// TODO if one of them is standing on moving platform, destroy it
			//      but don't destroy vehicles which are descending/ascending
		}
		else if(
			(pPlayer->ubTeam == TEAM_GREEN && ubTileType == MAP_LOGIC_SPAWN1) ||
			(pPlayer->ubTeam == TEAM_BROWN && ubTileType == MAP_LOGIC_SPAWN2)
		) {
			// Standing on own, unoccupied silo
			uwSiloDx = uwVx & ((1 << MAP_TILE_SIZE) - 1);
			uwSiloDy = uwVy & ((1 << MAP_TILE_SIZE) - 1);
			if(uwSiloDx > 12 && uwSiloDx < 18 && uwSiloDy > 12 && uwSiloDy < 18) {
				// Standing on bunkerable position
				if(pPlayer == g_pLocalPlayer) {
					// If one of them is local player, save data for highlight draw
					g_ubDoSiloHighlight = 1;
					g_uwSiloHighlightTileX = uwVTileX;
					g_uwSiloHighlightTileY = uwVTileY;
				}
				// Hide in bunker
				if(pPlayer->sSteerRequest.ubAction1 && g_ubDoSiloHighlight) {
					playerHideInBunker(pPlayer);
					return;
				}
			}
		}
	}

	// Calculate vehicle positions based on steer requests
	switch(pPlayer->ubCurrentVehicleType) {
		case VEHICLE_TYPE_TANK:
			vehicleSteerTank(pVehicle, &pPlayer->sSteerRequest);
			break;
		case VEHICLE_TYPE_JEEP:
			vehicleSteerJeep(pVehicle, &pPlayer->sSteerRequest);
			break;
	}
}

/**
 * Updates players' state machine.
 */
void simPlayers(void) {
	UBYTE ubPlayer;
	tPlayer *pPlayer;

	for(ubPlayer = 0; ubPlayer != g_ubPlayerLimit; ++ubPlayer) {
		pPlayer = &g_pPlayers[ubPlayer];
		switch(pPlayer->ubState) {
			case PLAYER_STATE_OFF:
				continue;
			case PLAYER_STATE_DEAD:
				if(pPlayer->uwCooldown) {
					--pPlayer->uwCooldown;
				}
				else {
					pPlayer->ubState = PLAYER_STATE_BUNKERED;
					worldHide();
				}
				continue;
			case PLAYER_STATE_SURFACING:
				if(!pPlayer->uwCooldown)
					pPlayer->ubState = PLAYER_STATE_DRIVING;
				else
					--pPlayer->uwCooldown;
				continue;
			case PLAYER_STATE_DRIVING:
				simPlayerVehicle(pPlayer);
				continue;
		}
	}
}

void simProjectiles(void) {
	projectileProcess();
}

void simTurrets(void) {
	// TODO simTurrets
}
