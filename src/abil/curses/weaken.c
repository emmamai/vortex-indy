#include "g_local.h"
#include "curses.h"

#define WEAKEN_DELAY			2
#define WEAKEN_DURATION_BASE	0
#define WEAKEN_DURATION_BONUS	1.0
#define WEAKEN_COST				50
// Four additional defines in curses.h

//************************************************************************************************
//			Weaken Curse
//************************************************************************************************

void Cmd_Weaken(edict_t *ent)
{
	int range, radius, talentLevel, cost=WEAKEN_COST;
	float duration;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Weaken()\n", ent->client->pers.netname);

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!V_CanUseAbilities(ent, WEAKEN, cost, true))
		return;

	range = CURSE_DEFAULT_INITIAL_RANGE + CURSE_DEFAULT_ADDON_RANGE * ent->myskills.abilities[WEAKEN].current_level;
	radius = CURSE_DEFAULT_INITIAL_RADIUS + CURSE_DEFAULT_ADDON_RADIUS * ent->myskills.abilities[WEAKEN].current_level;
	duration = WEAKEN_DURATION_BASE + (WEAKEN_DURATION_BONUS * ent->myskills.abilities[WEAKEN].current_level);

	//Talent: Evil curse
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	if (duration < 1)
		duration = 1;

	CurseRadiusAttack(ent, WEAKEN, range, radius, duration, true);
	
	//Finish casting the spell
	//ent->client->ability_delay = level.time + WEAKEN_DELAY;
	ent->myskills.abilities[WEAKEN].delay = level.time + WEAKEN_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	//Play the spell sound!
	gi.sound(ent, CHAN_ITEM, gi.soundindex("curses/weaken.wav"), 1, ATTN_NORM, 0);
}
