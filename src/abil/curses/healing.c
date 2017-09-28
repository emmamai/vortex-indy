#include "g_local.h"
#include "curses.h"

#define HEALING_DELAY			2
#define HEALING_DURATION_BASE	10.5	//allow for 10 "ticks"
#define HEALING_DURATION_BONUS	0
#define HEALING_COST			50
#define HEALING_HEAL_BASE		0
#define HEALING_HEAL_BONUS		1

//************************************************************************************************
//			Healing (Blessing)
//************************************************************************************************


void Healing_think(edict_t *self)
{
	//Find my slot
	que_t *slot = NULL;	
	int heal_amount = HEALING_HEAL_BASE + HEALING_HEAL_BONUS * self->owner->myskills.abilities[HEALING].current_level;
	float cooldown = 1.0;

	slot = que_findtype(self->enemy->curses, NULL, HEALING);

	// Blessing self-terminates if the enemy dies or the duration expires
	if (!slot || !que_valident(slot))
	{
		que_removeent(self->enemy->curses, self, true);
		return;
	}

	//Stick with the target
	VectorCopy(self->enemy->s.origin, self->s.origin);
	gi.linkentity(self);

	//Next think time
	self->nextthink = level.time + cooldown;

	//Heal the target's armor
	if (!self->enemy->client)
	{
		//Check to make sure it's a monster
		if (!self->enemy->mtype)
			return;
		
		heal_amount = self->enemy->max_health * (0.01 * self->owner->myskills.abilities[HEALING].current_level); // 1% healed per level
		
		if (heal_amount > 100)
			heal_amount = 100;

		//Heal the momster's health
		self->enemy->health += heal_amount;
		if (self->enemy->health > self->enemy->max_health)
			self->enemy->health = self->enemy->max_health;

		if (self->enemy->monsterinfo.power_armor_type)
		{
			heal_amount = self->enemy->monsterinfo.power_armor_power * (0.01 * self->owner->myskills.abilities[HEALING].current_level); // 1% healed per level
			
			if (heal_amount > 100)
				heal_amount = 100;

			//Heal the monster's armor
			self->enemy->monsterinfo.power_armor_power += heal_amount;
			if (self->enemy->monsterinfo.power_armor_power > self->enemy->monsterinfo.max_armor)
				self->enemy->monsterinfo.power_armor_power = self->enemy->monsterinfo.max_armor;
		}
	}
	else
	{
		if (self->enemy->health < MAX_HEALTH(self->enemy))
		{
			//Heal health
			self->enemy->health += heal_amount;
			if (self->enemy->health > MAX_HEALTH(self->enemy))
				self->enemy->health = MAX_HEALTH(self->enemy);
		}

		if (self->enemy->client->pers.inventory[body_armor_index] < MAX_ARMOR(self->enemy))
		{
			//Heal armor
			heal_amount *= 0.5; // don't heal as much armor
			if (heal_amount < 1)
				heal_amount = 1;
			self->enemy->client->pers.inventory[body_armor_index] += heal_amount;
			if (self->enemy->client->pers.inventory[body_armor_index] > MAX_ARMOR(self->enemy))
				self->enemy->client->pers.inventory[body_armor_index] = MAX_ARMOR(self->enemy);
		}
	}
}

void Cmd_Healing(edict_t *ent)
{
	int radius;
	float duration;
	edict_t *target = NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Healing()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[HEALING].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[HEALING].current_level, HEALING_COST))
		return;

	radius = BLESSING_RADIUS_BASE + (BLESSING_RADIUS_BONUS * ent->myskills.abilities[HEALING].current_level);
	duration = HEALING_DURATION_BASE + (HEALING_DURATION_BONUS * ent->myskills.abilities[HEALING].current_level);

	//Blessing self?
	if (Q_strcasecmp(gi.argv(1), "self") == 0)
	{
		if (!curse_add(ent, ent, HEALING, 0, duration))
		{
			safe_cprintf(ent, PRINT_HIGH, "Unable to bless self.\n");
			return;
		}
		target = ent;
	}
	else
	{
		target = curse_Attack(ent, HEALING, radius, duration, false);
	}
	if (target != NULL)
	{
		que_t *slot = NULL;
		//Finish casting the spell
		ent->client->ability_delay = level.time + HEALING_DELAY;
		ent->client->pers.inventory[power_cube_index] -= HEALING_COST;

		//Change the curse think to the healing think
		slot = que_findtype(target->curses, NULL, HEALING);
		if (slot)
		{
			slot->ent->think = Healing_think;
			slot->ent->nextthink = level.time + FRAMETIME;
		}

		//Notify the target
		if (target == ent)
		{
			safe_cprintf(target, PRINT_HIGH, "YOU HAVE BEEN BLESSED WITH %0.1f seconds OF HEALING!!\n", duration);
		}
		else if ((target->client) && !(target->svflags & SVF_MONSTER))
		{
			safe_cprintf(target, PRINT_HIGH, "YOU HAVE BEEN BLESSED WITH %0.1f seconds OF HEALING!!\n", duration);
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s with healing for %0.1f seconds.\n", target->myskills.player_name, duration);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s with healing for %0.1f seconds.\n", target->classname, duration);
		}
		//Play the spell sound!
		gi.sound(target, CHAN_ITEM, gi.soundindex("curses/prayer.wav"), 1, ATTN_NORM, 0);
	}
}