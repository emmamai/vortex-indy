#include "g_local.h"
#include "organ.h"

#define HEALER_INITIAL_HEALTH		100
#define HEALER_ADDON_HEALTH			40
#define HEALER_FRAMES_GROW_START	0
#define HEALER_FRAMES_GROW_END		15
#define HEALER_FRAMES_START			16
#define HEALER_FRAMES_END			26
#define HEALER_FRAME_DEAD			4
#define HEALER_COST					50
#define HEALER_DELAY				1.0

void healer_heal (edict_t *self, edict_t *other)
{
	float	value;
	//int	talentLevel = getTalentLevel(self->activator, TALENT_SUPER_HEALER);
	qboolean regenerate = false;

	// (apple)
	// In the healer's case, checking for the health 
	// self->health >= 1 should be enough,
	// but used G_EntIsAlive for consistency.
	if (G_EntIsAlive(other) && G_EntIsAlive(self) && OnSameTeam(self, other) && other != self)
	{
		int frames = 5000 / (15 * self->monsterinfo.level); // idk how much would this change tbh

		value = 1.0 + 0.1 * getTalentLevel(self->activator, TALENT_SUPER_HEALER);
		// regenerate health
		if (M_Regenerate(other, frames, 1, value, true, false, false, &other->monsterinfo.regen_delay2))
			regenerate = true;

		// regenerate armor
		/*
		if (talentLevel > 0)
		{
			frames *= (3.0 - (0.4 * talentLevel)); // heals 100% armor in 5 seconds at talent level 5
			if (M_Regenerate(other, frames, 10,  1.0, false, true, false, &other->monsterinfo.regen_delay3))
				regenerate = true;
		}*/
		
		// play healer sound if health or armor was regenerated
		if (regenerate && (level.time > self->msg_time))
		{
			gi.sound(self, CHAN_AUTO, gi.soundindex("organ/healer1.wav"), 1, ATTN_NORM, 0);
			self->msg_time = level.time + 1.5;
		}
	}
}

void healer_attack (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, 96)) != NULL)
		healer_heal(self, e);
}

void healer_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	if (level.time > self->lasthurt + 1.0)
		M_Regenerate(self, 300, 10, 1.0, true, false, false, &self->monsterinfo.regen_delay1);

	G_RunFrames(self, HEALER_FRAMES_START, HEALER_FRAMES_END, false);

	self->nextthink = level.time + FRAMETIME;
}

void healer_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(ent, other, plane, surf);
	healer_heal(ent, other);	
}

void healer_grow (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == HEALER_FRAMES_GROW_END)
	{
		self->think = healer_think;
		self->touch = healer_touch;
		self->style = 0;// done growing
		return;
	}

	if (self->s.frame == HEALER_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_STATIC, 0);

	G_RunFrames(self, HEALER_FRAMES_GROW_START, HEALER_FRAMES_GROW_END, false);
}

void healer_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void healer_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->healer = NULL;
		self->monsterinfo.slots_freed = true;
	
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by %s\n", attacker->client->pers.netname);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by a %s\n", V_GetMonsterName(attacker));
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by a %s\n", attacker->classname);
	}

	if (self->health <= self->gib_health || organ_explode(self))
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = healer_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = HEALER_FRAME_DEAD;
	self->maxs[2] = 4;
	gi.linkentity(self);
}

edict_t *CreateHealer (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = healer_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/healer/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "healer";
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = HEALER_INITIAL_HEALTH + HEALER_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	e->gib_health = -200;
	e->die = healer_die;
	e->touch = V_Touch;
	VectorSet(e->mins, -28, -28, 0);
	VectorSet(e->maxs, 28, 28, 32);
	e->mtype = M_HEALER;
	ent->healer = e;

	return e;
}

void Cmd_Healer_f (edict_t *ent)
{
	edict_t *healer;
	vec3_t	start;

	if (ent->healer && ent->healer->inuse)
	{
		organ_remove(ent->healer, true);
		safe_cprintf(ent, PRINT_HIGH, "Healer removed\n");
		return;
	}

	if (!V_CanUseAbilities(ent, HEALER, HEALER_COST, true))
		return;

	healer = CreateHealer(ent, ent->myskills.abilities[HEALER].current_level);
	if (!G_GetSpawnLocation(ent, 100, healer->mins, healer->maxs, start))
	{
		ent->healer = NULL;
		G_FreeEdict(healer);
		return;
	}
	VectorCopy(start, healer->s.origin);
	VectorCopy(ent->s.angles, healer->s.angles);
	healer->s.angles[PITCH] = 0;
	gi.linkentity(healer);
	healer->monsterinfo.cost = HEALER_COST;

	ent->client->ability_delay = level.time + HEALER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= HEALER_COST;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}