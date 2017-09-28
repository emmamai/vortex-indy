#include "g_local.h"
#include "curses.h"

#define DEFLECT_INITIAL_DURATION			0
#define DEFLECT_ADDON_DURATION				1.0	
#define DEFLECT_COST						50
#define DEFLECT_DELAY						2.0
// 9 more defines in curses.h

void ProjectileReverseCourse (edict_t *proj)
{
	VectorNegate(proj->movedir, proj->movedir);
	VectorNegate(proj->velocity, proj->velocity);
	vectoangles(proj->velocity, proj->s.angles);
	ValidateAngles(proj->s.angles);
}

void DeflectProjectile (edict_t *self, char *className, float chance, qboolean in_front)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), className)) != NULL)
	{
		// only use non-friendly projectiles
		if (e->owner && e->owner->inuse && OnSameTeam(self, e->owner))
			continue;

		//Talent: Repel
		// only deflect projectiles away from our front
		if (in_front && !infront(self, e))
			continue;

		// this projectiles has already been redirected back at its owner
		if (e->enemy && e->owner && (e->enemy == e->owner))
			continue;

		// calculate projectile speed and compare distance
		// if the speed is greater than the distance remaining, then we are going
		// to hit next frame, so we need to deflect this shot
		if (VectorLength(e->velocity)*FRAMETIME < entdist(self, e)-(G_GetHypotenuse(self->maxs)+1))
			continue;
		
		if (random() > chance)
			continue;

		// redirect projectile towards its owner
		if (e->owner && e->owner->inuse && e->owner->takedamage && visible(e, e->owner))
		{
			e->enemy = e->owner;
			e->owner = self; // take ownership
			ProjectileReverseCourse(e);
		}
	}	
}

void DeflectProjectiles (edict_t *self, float chance, qboolean in_front)
{
	DeflectProjectile(self, "rocket", chance, in_front);
	DeflectProjectile(self, "bolt", chance, in_front);
	DeflectProjectile(self, "bfg blast", chance, in_front);
	DeflectProjectile(self, "magicbolt", chance, in_front);
	DeflectProjectile(self, "grenade", chance, in_front);
	DeflectProjectile(self, "hgrenade", chance, in_front);
	DeflectProjectile(self, "skull", chance, in_front);
	DeflectProjectile(self, "spike", chance, in_front);
	DeflectProjectile(self, "spikey", chance, in_front);
	DeflectProjectile(self, "fireball", chance, in_front);
	DeflectProjectile(self, "plasma bolt", chance, in_front);
	DeflectProjectile(self, "acid", chance, in_front);
}

void deflect_think (edict_t *self)
{
	edict_t *player = G_GetClient(self->enemy);
	//Find my slot
	que_t *slot = NULL;
	slot = que_findtype(self->enemy->curses, NULL, DEFLECT);

	// Blessing self-terminates if the enemy dies or the duration expires
	if (!slot || !que_valident(slot))
	{
		if (player && level.time >= self->monsterinfo.selected_time)
			safe_cprintf(player, PRINT_HIGH, "Deflect has expired\n");

		que_removeent(self->enemy->curses, self, true);
		return;
	}

	// warn player that deflect is about to expire
//	if (player && !(level.framenum % 10) && (level.time >= slot->time - 5))
//		safe_cprintf(player, PRINT_HIGH, "Deflect will expire in %.0f seconds\n", slot->time - level.time);

	//Stick with the target
	VectorCopy(self->enemy->s.origin, self->s.origin);
	gi.linkentity(self);

	DeflectProjectiles(self->enemy, self->random, false);

	//Next think
	self->nextthink = level.time + FRAMETIME;

}

void Cmd_Deflect_f(edict_t *ent)
{
	float duration;
	edict_t *target = ent; // default target is self

	if (!V_CanUseAbilities(ent, DEFLECT, DEFLECT_COST, true))
		return;

	duration = DEFLECT_INITIAL_DURATION + DEFLECT_ADDON_DURATION * ent->myskills.abilities[DEFLECT].current_level;

	// bless the tank instead of the noclipped player
	if (PM_PlayerHasMonster(ent))
		target = target->owner;

	//Blessing self?
	if (Q_strcasecmp(gi.argv(1), "self") == 0)
	{
		if (!curse_add(target, ent, DEFLECT, 0, duration))
		{
			safe_cprintf(ent, PRINT_HIGH, "Unable to bless self.\n");
			return;
		}
		//target = ent;
	}
	else
	{
		target = curse_Attack(ent, DEFLECT, 512.0, duration, false);
	}

	if (target != NULL)
	{
		que_t *slot = NULL;

		//Finish casting the spell
		ent->client->ability_delay = level.time + DEFLECT_DELAY;
		ent->client->pers.inventory[power_cube_index] -= DEFLECT_COST;
	//	ent->myskills.abilities[DEFLECT].delay = level.time + duration + DEFLECT_DELAY;

		//Change the curse think to the deflect think
		slot = que_findtype(target->curses, NULL, DEFLECT);
		if (slot)
		{
			slot->ent->think = deflect_think;
			slot->ent->nextthink = level.time + FRAMETIME;
			slot->ent->random = DEFLECT_INITIAL_PROJECTILE_CHANCE+DEFLECT_ADDON_HITSCAN_CHANCE*ent->myskills.abilities[DEFLECT].current_level;
			if (slot->ent->random > DEFLECT_MAX_PROJECTILE_CHANCE)
				slot->ent->random = DEFLECT_MAX_PROJECTILE_CHANCE;
		}

		//Notify the target
		if (target == ent)
		{
			safe_cprintf(target, PRINT_HIGH, "You have been blessed with deflect for %0.1f seconds!\n", duration);
		}
		else if ((target->client) && !(target->svflags & SVF_MONSTER))
		{
			safe_cprintf(target, PRINT_HIGH, "You have been blessed with deflect for %0.1f seconds!\n\n", duration);
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s with deflect for %0.1f seconds.\n", target->myskills.player_name, duration);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Blessed %s with deflect for %0.1f seconds.\n", V_GetMonsterName(target), duration);
		}

		//Play the spell sound!
		gi.sound(target, CHAN_ITEM, gi.soundindex("curses/prayer.wav"), 1, ATTN_NORM, 0);
	}
}