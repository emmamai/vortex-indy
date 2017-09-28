#include "g_local.h"
#include "curses.h"

#define CURSE_DELAY				2
#define CURSE_DURATION_BASE		0
#define CURSE_DURATION_BONUS	1
#define CURSE_COST				25

//************************************************************************************************
//			Curse
//************************************************************************************************

void Cmd_Curse(edict_t *ent)
{
	int range, radius, talentLevel, cost=CURSE_COST;
	float duration;
	edict_t *target = NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Curse()\n", ent->client->pers.netname);

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!V_CanUseAbilities(ent, CURSE, cost, true))
		return;

	range = CURSE_DEFAULT_INITIAL_RANGE + CURSE_DEFAULT_ADDON_RANGE * ent->myskills.abilities[CURSE].current_level;
	radius = CURSE_DEFAULT_INITIAL_RADIUS + CURSE_DEFAULT_ADDON_RADIUS * ent->myskills.abilities[CURSE].current_level;
	duration = CURSE_DURATION_BASE + (CURSE_DURATION_BONUS * ent->myskills.abilities[CURSE].current_level);

	//Talent: Evil curse
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	if (duration < 1)
		duration = 1;

	CurseRadiusAttack(ent, CURSE, range, radius, duration, true);

	//Finish casting the spell
	//ent->client->ability_delay = level.time + CURSE_DELAY;
	ent->myskills.abilities[CURSE].delay = level.time + CURSE_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	target = curse_Attack(ent, CURSE, radius, duration, true);

	//Play the spell sound!
	gi.sound(ent, CHAN_ITEM, gi.soundindex("curses/curse.wav"), 1, ATTN_NORM, 0);
	
}