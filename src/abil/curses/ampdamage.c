#include "g_local.h"
#include "curses.h"

#define AMP_DAMAGE_DELAY			2
#define AMP_DAMAGE_DURATION_BASE	0
#define AMP_DAMAGE_DURATION_BONUS	1.0
#define AMP_DAMAGE_COST				50
// Two additional defines in curses.h

//************************************************************************************************
//			Amp Damage Curse
//************************************************************************************************

void Cmd_AmpDamage(edict_t *ent)
{
	int range, radius, talentLevel, cost=AMP_DAMAGE_COST;
	float duration;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_AmpDamage()\n", ent->client->pers.netname);

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!V_CanUseAbilities(ent, AMP_DAMAGE, cost, true))
		return;

	range = CURSE_DEFAULT_INITIAL_RANGE + CURSE_DEFAULT_ADDON_RANGE * ent->myskills.abilities[AMP_DAMAGE].current_level;
	radius = CURSE_DEFAULT_INITIAL_RADIUS + CURSE_DEFAULT_ADDON_RADIUS * ent->myskills.abilities[AMP_DAMAGE].current_level;
	duration = AMP_DAMAGE_DURATION_BASE + AMP_DAMAGE_DURATION_BONUS * ent->myskills.abilities[AMP_DAMAGE].current_level;

	//Talent: Evil curse
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	if (duration < 1)
		duration = 1;

	CurseRadiusAttack(ent, AMP_DAMAGE, range, radius, duration, true);
	
	//Finish casting the spell
	//ent->client->ability_delay = level.time + AMP_DAMAGE_DELAY;
	ent->myskills.abilities[AMP_DAMAGE].delay = level.time + AMP_DAMAGE_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	//Play the spell sound!
	gi.sound(ent, CHAN_ITEM, gi.soundindex("curses/amplifydamage.wav"), 1, ATTN_NORM, 0);
}
