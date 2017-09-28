#include "g_local.h"
#include "curses.h"

//************************************************************************************************
//			Disables all skills, called when target is cursed with amnesia
//************************************************************************************************

void V_DisableAllSkills(edict_t *ent)
{
	//jetpack
	ent->client->thrusting = 0;
	//grapple hook
	ent->client->hook_state = HOOK_READY;
	//power screen
	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	//superspeed
	ent->superspeed = false;
	ent->antigrav = false;
	//Disable all auras
	AuraRemove(ent, 0);
}

//************************************************************************************************
//			Amnesia Curse
//************************************************************************************************

void Cmd_Amnesia(edict_t *ent)
{
	int radius, talentLevel, cost=AMNESIA_COST;
	float duration;
	edict_t *target = NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Amnesia()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[AMNESIA].disable)
		return;

	//Talent: Cheaper Curses
	if ((talentLevel = getTalentLevel(ent, TALENT_CHEAPER_CURSES)) > 0)
		cost *= 1.0 - 0.1 * talentLevel;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[AMNESIA].current_level, cost))
		return;

	radius = SHAMAN_CURSE_RADIUS_BASE + (SHAMAN_CURSE_RADIUS_BONUS * ent->myskills.abilities[AMNESIA].current_level);
	duration = AMNESIA_DURATION_BASE + (AMNESIA_DURATION_BONUS * ent->myskills.abilities[AMNESIA].current_level);

	//Talent: Evil curse
	talentLevel = getTalentLevel(ent, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
		duration *= 1.0 + 0.25 * talentLevel;

	target = curse_Attack(ent, AMNESIA, radius, duration, true);
	if (target != NULL)
	{
		//Finish casting the spell
		ent->client->ability_delay = level.time + AMNESIA_DELAY;
		ent->client->pers.inventory[power_cube_index] -= cost;

		//disable some abilities if the player was using them
		V_DisableAllSkills(ent);

		//Notify the target
		if ((target->client) && !(target->svflags & SVF_MONSTER))
		{
			safe_cprintf(target, PRINT_HIGH, "YOU HAVE BEEN CURSED WITH AMNESIA!! (%0.1f seconds)\n", duration);
			safe_cprintf(ent, PRINT_HIGH, "Cursed %s with amnesia for %0.1f seconds.\n", target->myskills.player_name, duration);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Cursed %s with amnesia for %0.1f seconds.\n", target->classname, duration);
		}
		//Play the spell sound!
		gi.sound(target, CHAN_ITEM, gi.soundindex("curses/amnesia.wav"), 1, ATTN_NORM, 0);
	}
}
