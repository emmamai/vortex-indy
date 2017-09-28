#include "g_local.h"

#define MIND_ABSORB_RADIUS_BASE		256
#define MIND_ABSORB_RADIUS_BONUS	0
#define MIND_ABSORB_AMOUNT_BASE		0
#define MIND_ABSORB_AMOUNT_BONUS	10

//************************************************************************************************
//			Mind Absorb (passive skill)
//************************************************************************************************

void MindAbsorb(edict_t *ent) 
{  
	edict_t *target = NULL;  
	int radius;  
	int take;  
	int total;
	int abilityLevel = ent->myskills.abilities[MIND_ABSORB].current_level;   
	
	if(ent->myskills.abilities[MIND_ABSORB].disable)   
		return;   
	if (!V_CanUseAbilities(ent, MIND_ABSORB, 0, false))   
		return;   //Cloaking and chat protected players can't steal anything
	if ((ent->flags & FL_CHATPROTECT) || (ent->svflags & SVF_NOCLIENT))   
		return;   
	
	take = MIND_ABSORB_AMOUNT_BASE + (MIND_ABSORB_AMOUNT_BONUS * abilityLevel);  
	radius = MIND_ABSORB_RADIUS_BASE + (MIND_ABSORB_RADIUS_BONUS * abilityLevel);   
	
	// scan for targets  
	while ((target = findclosestradius(target, ent->s.origin, radius)) != NULL)  
	{   
		if (target == ent)    
			continue;   
		if (!G_ValidTarget(ent, target, true))    
			continue;   
		
		total = 0;
		
		if (target->client)
		{
			if (target->client->pers.inventory[power_cube_index] < take)
				total += target->client->pers.inventory[power_cube_index];
			else
				total += take;

			target->client->pers.inventory[power_cube_index] -= total;

			// a bit of amnesia too
			target->client->ability_delay = level.time + 0.1 * abilityLevel;  
		}
		else
		{
			if (target->health < take)
				total += target->health;
			else
				total += take;
		}

		//Cap cube count to max cubes 
		if (ent->client->pers.inventory[power_cube_index] + total < MAX_POWERCUBES(ent))
			ent->client->pers.inventory[power_cube_index] += total;
		else if (ent->client->pers.inventory[power_cube_index] < MAX_POWERCUBES(ent))
			ent->client->pers.inventory[power_cube_index] = MAX_POWERCUBES(ent); 

		// those powercubes hurt!  
		T_Damage(target, ent, ent, vec3_origin, vec3_origin, vec3_origin, total, 0, DAMAGE_NO_ARMOR, MOD_MINDABSORB);
	}  
}