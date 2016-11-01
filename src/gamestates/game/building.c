#include <gamestates/game/building.h>

#define BUILDING_FIRST 1
#define BUILDING_LAST 255
#define BUILDING_HP_WALL 100
#define BUILDING_HP_AMMO 30
#define BUILDING_HP_FUEL 30
#define BUILDING_HP_FLAG 50

tBuildingManager s_sBuildingManager;
const UBYTE s_pBuildingHps[4] = {
	BUILDING_HP_WALL,
	BUILDING_HP_AMMO,
	BUILDING_HP_FUEL,
	BUILDING_HP_FLAG
};

void buildingManagerReset(void) {
	UBYTE ubIdx;
	s_sBuildingManager.ubLastIdx = BUILDING_LAST;
	
	for(ubIdx = BUILDING_FIRST; ubIdx != BUILDING_LAST; ++ubIdx) {
		s_sBuildingManager.pBuildings[ubIdx].ubHp = 0;
	}
}

UBYTE buildingAdd(UBYTE ubType) {
	UBYTE ubIdx;
	
	if(s_sBuildingManager.ubLastIdx == BUILDING_LAST)
		ubIdx = BUILDING_FIRST;
	else
		ubIdx = s_sBuildingManager.ubLastIdx;
	while(ubIdx != s_sBuildingManager.ubLastIdx) {
		if(!s_sBuildingManager.pBuildings[ubIdx].ubHp) {
			s_sBuildingManager.pBuildings[ubIdx].ubHp = s_pBuildingHps[ubType];
			s_sBuildingManager.ubLastIdx = ubIdx;
			return ubIdx;
		}
		
		if(ubIdx == BUILDING_LAST)
			ubIdx = BUILDING_FIRST; // Will be incremented by loop
		else
			++ubIdx;
	}
		
	return BUILDING_INVALID;
}

UBYTE buildingDamage(UBYTE ubIdx, UBYTE ubDamage) {
	if(s_sBuildingManager.pBuildings[ubIdx].ubHp <= ubDamage) {
		s_sBuildingManager.pBuildings[ubIdx].ubHp = 0;
		return BUILDING_DESTROYED;
	}
	s_sBuildingManager.pBuildings[ubIdx].ubHp -= ubDamage;
	return BUILDING_DAMAGED;
}