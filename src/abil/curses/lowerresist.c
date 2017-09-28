#include "g_local.h"
#include "curses.h"

//************************************************************************************************
//			Lower Resist Curse
//************************************************************************************************

void Cmd_LowerResist (edict_t *ent)
{
	int range, radius, talentLevel, cost=LOWER_RESIST_COST;
	float duration;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_LowerResist()\n", ent->client->pers.netname);

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!V_CanUseAbilities(ent, LOWER_RESIST, cost, true))
		return;

	range = LOWER_RESIST_INITIAL_RANGE + LOWER_RESIST_ADDON_RANGE * ent->myskills.abilities[LOWER_RESIST].current_level;
	radius = LOWER_RESIST_INITIAL_RADIUS + LOWER_RESIST_ADDON_RADIUS * ent->myskills.abilities[LOWER_RESIST].current_level;
	duration = LOWER_RESIST_INITIAL_DURATION + LOWER_RESIST_ADDON_DURATION * ent->myskills.abilities[LOWER_RESIST].current_level;

	// evil curse talent
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	if (duration < 1)
		duration = 1;

	CurseRadiusAttack(ent, LOWER_RESIST, range, radius, duration, true);

	//Finish casting the spell
	//ent->client->ability_delay = level.time + LOWER_RESIST_DELAY;
	ent->myskills.abilities[LOWER_RESIST].delay = level.time + LOWER_RESIST_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	//Play the spell sound!
	gi.sound(ent, CHAN_ITEM, gi.soundindex("curses/lowerresist.wav"), 1, ATTN_NORM, 0);

}