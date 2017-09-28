#include "g_local.h"

void Cmd_AmmoStealer_f(edict_t *ent)
{
	edict_t		*other = NULL;
	int			shells=0, bullets=0, rockets=0, slugs=0, cells=0;
	float		steal_base;	//Base multiplier for ammo steal
	int			skill_level;
	qboolean	foundtarget = false;
	vec3_t		start;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_AmmoStealer_f()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[AMMO_STEAL].disable)
		return;

	//3.0 new ammo steal algorithm, more efficient less buggy (no ammo lost during steal)
	if (!G_CanUseAbilities(ent, ent->myskills.abilities[AMMO_STEAL].current_level, COST_FOR_STEALER))
		return;

	G_GetSpawnLocation(ent, 512, vec3_origin, vec3_origin, start);

	//Search for targets
	while ((other = findradius(other, start, 128)) != NULL)
	{
		if (other == ent)
			continue;
		if (!other->inuse)
			continue;
		if (!other->takedamage)
			continue;
		if (other->solid == SOLID_NOT)
			continue;
		if (!other->client)
			continue;
		if (OnSameTeam(ent, other))
			continue;
		if (!visible(ent, other))
			continue;

		//A target has been found
		foundtarget = true;

		//Calculate the steal multiplier (random amount for each target)
		skill_level = ent->myskills.abilities[AMMO_STEAL].current_level;
		//min = 6.25%/lvl, max = 7.14%/lvl
		steal_base = GetRandom((int)(skill_level / 16.0 * 100.0), (int)(skill_level / 14.0 * 100.0)) / 100.0;
		if (steal_base > 0.9)	steal_base = 0.9; //max at 90%

		//Start stealing ammo
		shells	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]	* steal_base;
		bullets += other->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))]	* steal_base;
		rockets += other->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))]	* steal_base;
		slugs	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))]		* steal_base;
		cells	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))]		* steal_base;
		
		//This next part does the same thing as subtracting the amount stolen from the target
		other->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))]	*= (1 - steal_base);

		//Play the spell sound!
		gi.sound(ent, CHAN_ITEM, gi.soundindex("spells/telekinesis.wav"), 1, ATTN_NORM, 0);

		//Notify the two clients
		safe_cprintf(other, PRINT_HIGH, "%s just stole some of your ammo!\n", ent->client->pers.netname);
		safe_cprintf(ent, PRINT_HIGH, "You just stole %d%% of %s's ammo!\n", (int)(steal_base * 100), other->client->pers.netname);

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}
	//Add what we stole from the other people. :)
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] += shells;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] += bullets;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] += rockets;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))] += slugs;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))] += cells;
	Check_full(ent);//Make sure we are not over our limits

	if(foundtarget)
	{
		ent->client->ability_delay = level.time + DELAY_AMMOSTEAL;
		ent->client->pers.inventory[power_cube_index] -= COST_FOR_STEALER;
	}
}