#include "g_local.h"
#include "organ.h"
#include "spike.h"

#define SPIKER_FRAMES_GROW_START		0
#define SPIKER_FRAMES_GROW_END			12
#define SPIKER_FRAMES_NOAMMO_START		14
#define SPIKER_FRAMES_NOAMMO_END		17
#define SPIKER_FRAMES_REARM_START		19
#define SPIKER_FRAMES_REARM_END			23
#define SPIKER_FRAME_READY				13
#define SPIKER_FRAME_DEAD				18

void spiker_dead (edict_t *self)
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

void spiker_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = SPIKER_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_spikers--;
		cur = self->activator->num_spikers;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
	}

	if (self->health <= self->gib_health || organ_explode(self))
	{
		int n;

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

	self->think = spiker_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = SPIKER_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	self->maxs[2] = 16;
	gi.linkentity(self);
}

void spiker_attack (edict_t *self)
{
	float	dist;
	float	range=SPIKER_INITIAL_RANGE+SPIKER_ADDON_RANGE*self->monsterinfo.level;
	int		speed=SPIKER_INITIAL_SPEED+SPIKER_ADDON_SPEED*self->monsterinfo.level;
	vec3_t	forward, start, end;
	edict_t *e=NULL;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] - 16;

	while ((e = findradius(e, self->s.origin, range)) != NULL)
	{
		if (!G_ValidTarget(self, e, true))
			continue;
		
		// copy target location
		G_EntMidPoint(e, end);

		// calculate distance to target
		dist = distance(e->s.origin, start);

		// move our target point based on projectile and enemy velocity
		VectorMA(end, (float)dist/speed, e->velocity, end);
				
		// calculate direction vector to target
		VectorSubtract(end, start, forward);
		VectorNormalize(forward);

		fire_spike(self, start, forward, self->dmg, 0, speed);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + SPIKER_REFIRE_DELAY;
		self->s.frame = SPIKER_FRAMES_NOAMMO_START;
		gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	}	
}

void NearestNodeLocation (vec3_t start, vec3_t node_loc);
int FindPath(vec3_t start, vec3_t destination);
void spiker_think (edict_t *self)
{
	vec3_t v1,v2;
	edict_t *e=NULL;

	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

	//FIXME: delete this!!!!!!!!!!
	if (!(level.framenum%10))
	{
		NearestNodeLocation(self->s.origin, v1);
		NearestNodeLocation(self->activator->s.origin, v2);
		FindPath(v1, v2);
	}

	if (self->removetime > 0)
	{
		qboolean converted=false;

		if (self->flags & FL_CONVERTED)
			converted = true;

		if (level.time > self->removetime)
		{
			// if we were converted, try to convert back to previous owner
			if (converted && self->prev_owner && self->prev_owner->inuse)
			{
				if (!ConvertOwner(self->prev_owner, self, 0, false))
				{
					organ_remove(self, false);
					return;
				}
			}
			else
			{
				organ_remove(self, false);
				return;
			}
		}
		// warn the converted monster's current owner
		else if (converted && self->activator && self->activator->inuse && self->activator->client 
			&& (level.time > self->removetime-5) && !(level.framenum%10))
				safe_cprintf(self->activator, PRINT_HIGH, "%s conversion will expire in %.0f seconds\n", 
					V_GetMonsterName(self), self->removetime-level.time);	
	}

	// try to attack something
	spiker_attack(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	//if (level.time > self->lasthurt + 1.0) 
	//	M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	// run animation frames
	if (self->monsterinfo.attack_finished - 0.5 > level.time)
	{
		if (self->s.frame != SPIKER_FRAME_READY)
			G_RunFrames(self, SPIKER_FRAMES_NOAMMO_START, SPIKER_FRAMES_NOAMMO_END, false);
	}
	else
	{
		if (self->s.frame != SPIKER_FRAME_READY && self->s.frame != SPIKER_FRAMES_REARM_END)
			G_RunFrames(self, SPIKER_FRAMES_REARM_START, SPIKER_FRAMES_REARM_END, false);
		else
			self->s.frame = SPIKER_FRAME_READY;
	}

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	gi.linkentity(self);
	
	self->nextthink = level.time + FRAMETIME;
}

void spiker_grow (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

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

	self->s.effects |= EF_PLASMA;

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == SPIKER_FRAMES_GROW_END && self->health >= self->max_health)
	{
		self->s.frame = SPIKER_FRAME_READY;
		self->think = spiker_think;
		self->style = 0;//done growing
		return;
	}

	if (self->s.frame == SPIKER_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_NORM, 0);

	if (self->s.frame != SPIKER_FRAMES_GROW_END)
		G_RunFrames(self, SPIKER_FRAMES_GROW_START, SPIKER_FRAMES_GROW_END, false);
}

edict_t *CreateSpiker (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = spiker_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/spiker/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;//Note/Important/Hint: tells MOVETYPE_STEP physics to clip on any solid object (not just walls)
	e->mass = 500;
	e->classname = "spiker";
	e->takedamage = DAMAGE_AIM;
	e->max_health = SPIKER_INITIAL_HEALTH + SPIKER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = SPIKER_INITIAL_DAMAGE + SPIKER_ADDON_DAMAGE * skill_level;

	e->monsterinfo.level = skill_level;
	e->gib_health = -250;
	e->die = spiker_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -24, -24, 0);
	VectorSet(e->maxs, 24, 24, 48);
	e->mtype = M_SPIKER;

	ent->num_spikers++;

	return e;
}

void Cmd_Spiker_f (edict_t *ent)
{
	int		cost = SPIKER_COST;
	edict_t *spiker;
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "spiker", true);
		safe_cprintf(ent, PRINT_HIGH, "Spikers removed\n");
		ent->num_spikers = 0;
		return;
	}

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	if (!V_CanUseAbilities(ent, SPIKER, cost, true))
		return;

	if (ent->num_spikers >= SPIKER_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spikers (%d)\n", SPIKER_MAX_COUNT);
		return;
	}

	spiker = CreateSpiker(ent, ent->myskills.abilities[SPIKER].current_level);
	if (!G_GetSpawnLocation(ent, 100, spiker->mins, spiker->maxs, start))
	{
		ent->num_spikers--;
		G_FreeEdict(spiker);
		return;
	}
	VectorCopy(start, spiker->s.origin);
	VectorCopy(ent->s.angles, spiker->s.angles);
	spiker->s.angles[PITCH] = 0;
	gi.linkentity(spiker);
	spiker->monsterinfo.attack_finished = level.time + 2.0;
	spiker->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Spiker created (%d/%d)\n", ent->num_spikers, SPIKER_MAX_COUNT);

	ent->client->ability_delay = level.time + SPIKER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;
	//ent->holdtime = level.time + SPIKER_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}