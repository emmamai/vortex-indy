#include "g_local.h"
#include "curses.h"

//************************************************************************************************
//			Bless (Blessing)
//************************************************************************************************

//************************************************************************************************
//			Bless think
//************************************************************************************************

void Bless_think(edict_t *self)
{
	//Find my slot
	que_t *slot = NULL;
	slot = que_findtype(self->enemy->curses, NULL, BLESS);

	// Blessing self-terminates if the enemy dies or the duration expires
	if (!slot || !que_valident(slot))
	{
		self->enemy->superspeed = false;
		que_removeent(self->enemy->curses, self, true);
		return;
	}

	//Stick with the target
	VectorCopy(self->enemy->s.origin, self->s.origin);
	gi.linkentity(self);

	//give them super speed
	self->enemy->superspeed = true;

	//Next think
	self->nextthink = level.time + FRAMETIME;

}

void Cmd_Bless(edict_t *ent)
{
	int radius;
	float duration, cooldown;
	edict_t *target = NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Bless()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[BLESS].disable)
		return;

	//if (!G_CanUseAbilities(ent, ent->myskills.abilities[BLESS].current_level, BLESS_COST))
	//	return;

	if (!V_CanUseAbilities(ent, BLESS, BLESS_COST, true))
		return;

	radius = SHAMAN_CURSE_RADIUS_BASE + (SHAMAN_CURSE_RADIUS_BONUS * ent->myskills.abilities[BLESS].current_level);
	duration = BLESS_DURATION_BASE + (BLESS_DURATION_BONUS * ent->myskills.abilities[BLESS].current_level);

	//Blessing self?
	if (Q_strcasecmp(gi.argv(1), "self") == 0)
	{
		if (HasFlag(ent))
		{
			safe_cprintf(ent, PRINT_HIGH, "Can't use this while carrying the flag!\n");
			return;
		}

		if (!curse_add(ent, ent, BLESS, 0, duration))
		{
			safe_cprintf(ent, PRINT_HIGH, "Unable to bless self.\n");
			return;
		}
		target = ent;
	}
	else
	{
		target = curse_Attack(ent, BLESS, radius, duration, false);
	}

	if (target != NULL)
	{
		que_t *slot = NULL;

		//Finish casting the spell
		ent->client->ability_delay = level.time + BLESS_DELAY;
		ent->client->pers.inventory[power_cube_index] -= BLESS_COST;

		cooldown = 2.0 * duration;
		if (cooldown > 10.0)
			cooldown = 10.0;

		ent->myskills.abilities[BLESS].delay = level.time + cooldown;

		//Change the curse think to the bless think
		slot = que_findtype(target->curses, NULL, BLESS);
		if (slot)
		{
			slot->ent->think = Bless_think;
			slot->ent->nextthink = level.time + FRAMETIME;
		}

		//Notify the target
		if (target == ent)
		{
			safe_cprintf(target, PRINT_HIGH, "YOU HAVE BEEN BLESSED FOR %0.1f seconds!!\n", duration);
		}
		else if ((target->client) && !(target->svflags & SVF_MONSTER))
		{
			safe_cprintf(target, PRINT_HIGH, "YOU HAVE BEEN BLESSED FOR %0.1f seconds!!\n", duration);
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s for %0.1f seconds.\n", target->myskills.player_name, duration);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s for %0.1f seconds.\n", target->classname, duration);
		}
		//Play the spell sound!
		gi.sound(target, CHAN_ITEM, gi.soundindex("curses/bless.wav"), 1, ATTN_NORM, 0);
	}
}
