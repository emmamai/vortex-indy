#include "g_local.h"
#include "curses.h"

//************************************************************************************************
//			Life Drain Curse
//************************************************************************************************

void Cmd_LifeDrain(edict_t *ent)
{
	int		range, radius, talentLevel, cost=LIFE_DRAIN_COST;
	float	duration;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_LifeDrain()\n", ent->client->pers.netname);

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!V_CanUseAbilities(ent, LIFE_DRAIN, cost, true))
		return;

	range = CURSE_DEFAULT_INITIAL_RANGE + CURSE_DEFAULT_ADDON_RANGE * ent->myskills.abilities[LIFE_DRAIN].current_level;
	radius = CURSE_DEFAULT_INITIAL_RADIUS + CURSE_DEFAULT_ADDON_RADIUS * ent->myskills.abilities[LIFE_DRAIN].current_level;
	duration = LIFE_DRAIN_DURATION_BASE + LIFE_DRAIN_DURATION_BONUS * ent->myskills.abilities[LIFE_DRAIN].current_level;

	//Talent: Evil curse
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	if (duration < 1)
		duration = 1;

	CurseRadiusAttack(ent, LIFE_DRAIN, range, radius, duration, true);
	
	//Finish casting the spell
	//ent->client->ability_delay = level.time + LIFE_DRAIN_DELAY;
	ent->myskills.abilities[LIFE_DRAIN].delay = level.time + LIFE_DRAIN_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;

	//Play the spell sound!
	gi.sound(ent, CHAN_ITEM, gi.soundindex("curses/ironmaiden.wav"), 1, ATTN_NORM, 0);// FIXME: change this!
}

void Bleed (edict_t *curse)
{
	int		take;
	edict_t *caster=curse->owner;

	if (curse->atype != BLEEDING)
		return;

	if (level.time < curse->wait)
		return;

	if (!G_ValidTarget(caster, curse->enemy, false))
	{
		// remove the curse if the target dies
		que_removeent(curse->enemy->curses, curse, true);
		return;
	}

	// 33-99% health taken over duration of curse
	take = (curse->enemy->max_health * (0.033 * curse->monsterinfo.level)) / curse->monsterinfo.selected_time;

	//gi.dprintf("target %s take %d health %d/%d level %d time %.1f\n", 
	//	curse->enemy->classname, take, curse->enemy->health, curse->enemy->max_health,
	//	curse->monsterinfo.level, curse->monsterinfo.selected_time);

	// damage limits
	if (take < 1)
		take = 1;
	if (take > 100)
		take = 100;

	T_Damage(curse->enemy, caster, caster, vec3_origin, vec3_origin, 
		vec3_origin, take, 0, DAMAGE_NO_ABILITIES, MOD_LIFE_DRAIN);

	curse->wait = level.time + (GetRandom(3, 10) * FRAMETIME);
}

void LifeDrain (edict_t *curse)
{
	int		take;
	edict_t *caster=curse->owner;

	if (curse->atype != LIFE_DRAIN)
		return;

	if (level.time < curse->wait)
		return;

	if (!G_ValidTarget(caster, curse->enemy, false))
	{
		// remove the curse if the target dies
		que_removeent(curse->enemy->curses, curse, true);
		return;
	}

	take = LIFE_DRAIN_HEALTH;
	// more effective on non-clients (because they have more health)
	if (!curse->enemy->client)
		take *= 2;

	// give caster health
	if (caster->health < caster->max_health)
	{
		caster->health += take;
		if (caster->health > caster->max_health)
			caster->health = caster->max_health;
	}

	// take it away from curse's target
	if (!curse->enemy->client) //deal even MORE damage to non-players.
		T_Damage(curse->enemy, caster, caster, vec3_origin, vec3_origin, 
			vec3_origin, take * 1.5, 0, DAMAGE_NO_ABILITIES, MOD_LIFE_DRAIN);
	else
		T_Damage(curse->enemy, caster, caster, vec3_origin, vec3_origin, 
			vec3_origin, take, 0, DAMAGE_NO_ABILITIES, MOD_LIFE_DRAIN);

	curse->wait = level.time + LIFE_DRAIN_UPDATETIME;
}