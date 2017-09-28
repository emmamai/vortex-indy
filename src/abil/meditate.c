#include "g_local.h"

void drop_make_touchable (edict_t *ent);
void meditate_think (edict_t *self)
{
	self->s.effects &= ~EF_PLASMA;
	self->think = drop_make_touchable;
	self->nextthink = level.time + FRAMETIME;
}

void Cmd_Meditate_f (edict_t *ent)
{
	//Talent: Meditation
	int talentLevel = getTalentLevel(ent, TALENT_MEDITATION);
	gitem_t *power_cube;

	if (talentLevel < 1)
		return;

	if (ent->client->ability_delay > level.time)
		return;

	ent->manacharging = !ent->manacharging;

	if (ent->manacharging)
	{
		gi.cprintf(ent, PRINT_HIGH, "Charging mana.\n");
	}
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "No longer charging mana.\n");
		ent->client->ability_delay = level.time + 2;
	}

	// old meditation
	/*power_cube = FindItem("Power Cube");

	ent->client->pers.inventory[ITEM_INDEX(power_cube)] += 20*talentLevel;*/

	// ent->holdtime = level.time + 2;

	// ent->client->ability_delay = level.time + 2;
}
