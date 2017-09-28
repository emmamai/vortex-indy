#include "g_local.h"

#define PURGE_COST		50
#define PURGE_DELAY		5.0

void Cmd_Purge_f (edict_t *ent)
{
	//Talent: Purge
	int talentLevel = getTalentLevel(ent, TALENT_PURGE);

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade this talent before you can use it!\n");
		return;
	}
	
	if (ent->client->ability_delay > level.time)
	{
		safe_cprintf(ent, PRINT_HIGH, "You must wait %.1f seconds before you can use this talent.\n", 
			ent->client->ability_delay-level.time);
		return;
	}
	if (ent->client->pers.inventory[power_cube_index] < PURGE_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need %d cubes before you can use this talent.\n", 
			PURGE_COST-ent->client->pers.inventory[power_cube_index]);
		return;
	}

	//Give them a short period of total immunity
	ent->client->invincible_framenum = level.framenum + 3*talentLevel; //up to 2 seconds at level 5

	//Give them a short period of curse immunity
	ent->holywaterProtection = level.time + talentLevel; //up to 5 seconds at level 5

	//You can only drink 1/sec
	ent->client->ability_delay = level.time + PURGE_DELAY;

	//Remove all curses
	CurseRemove(ent, 0);

	ent->client->pers.inventory[power_cube_index] -= PURGE_COST;
	ent->client->ability_delay = level.time + PURGE_DELAY;

	gi.sound(ent, CHAN_AUTO, gi.soundindex("spells/purification.wav"), 1, ATTN_NORM, 0);
}
