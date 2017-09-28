#include "g_local.h"
#include "organ.h"

#define OBSTACLE_INITIAL_HEALTH			0		
#define OBSTACLE_ADDON_HEALTH			145
#define OBSTACLE_INITIAL_DAMAGE			0
#define OBSTACLE_ADDON_DAMAGE			40	
#define OBSTACLE_COST					25
#define OBSTACLE_DELAY					0.5

#define OBSTACLE_FRAMES_GROW_START		0
#define OBSTACLE_FRAMES_GROW_END		9
#define OBSTACLE_FRAME_READY			12
#define OBSTACLE_FRAME_DEAD				13

void obstacle_dead (edict_t *self)
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

void obstacle_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = OBSTACLE_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_obstacle--;
		cur = self->activator->num_obstacle;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
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
	self->s.frame = OBSTACLE_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	self->touch = V_Touch;
	self->maxs[2] = 16;
	gi.linkentity(self);
}

void obstacle_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	organ_touch(self, other, plane, surf);

	if (other && other->inuse && other->takedamage && !OnSameTeam(self->activator, other) 
		&& (level.framenum >= self->monsterinfo.jumpup))
	{
		T_Damage(other, self, self, self->velocity, self->s.origin, 
			plane->normal, self->dmg, 0, 0, MOD_OBSTACLE);
		self->monsterinfo.jumpup = level.framenum + 1;
	}
	else if (other && other->inuse && other->takedamage)
	{
		self->svflags &= ~SVF_NOCLIENT;
		self->monsterinfo.idle_frames = 0;
	}
}

void obstacle_cloak (edict_t *self)
{
	// already cloaked
	if (self->svflags & SVF_NOCLIENT)
	{
		// random chance for obstacle to become uncloaked for a frame
		//if (level.time > self->lasthbshot && random() <= 0.05)
		if (!(level.framenum % 50) && random() > 0.5)
		{
			self->svflags &= ~SVF_NOCLIENT;
			//self->lasthbshot = level.time + 1.0;
		}
		return;
	}

	// cloak after idling for awhile
	if (self->monsterinfo.idle_frames >= self->monsterinfo.nextattack)
		self->svflags |= SVF_NOCLIENT;
}

void obstacle_move (edict_t *self)
{
	// check for ground entity
	if (!M_CheckBottom(self) || self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// check for movement/sliding
	if (!(int)self->velocity[0] && !(int)self->velocity[1] && !(int)self->velocity[2])
	{
		// stick to the ground
		if (self->groundentity && self->groundentity == world)
			self->movetype = MOVETYPE_NONE;
		else
			self->movetype = MOVETYPE_STEP;

		// increment idle frames
		self->monsterinfo.idle_frames++;
	}
	else
	{
		// de-cloak
		self->svflags &= ~SVF_NOCLIENT;
		// reset idle frames
		self->monsterinfo.idle_frames = 0;
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
}

void obstacle_think (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);
	obstacle_cloak(self);
	obstacle_move(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;
}

void obstacle_grow (edict_t *self)
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

	if (self->s.frame == OBSTACLE_FRAMES_GROW_END && self->health >= self->max_health)
	{
		self->style = 0;//done growing
		self->s.frame = OBSTACLE_FRAME_READY;
		self->think = obstacle_think;
		self->touch = obstacle_touch;
		return;
	}

	if (self->s.frame == OBSTACLE_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_NORM, 0);

	if (self->s.frame != OBSTACLE_FRAMES_GROW_END)
		G_RunFrames(self, OBSTACLE_FRAMES_GROW_START, OBSTACLE_FRAMES_GROW_END, false);
}

edict_t *CreateObstacle (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = obstacle_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/obstacle/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "obstacle";
	e->takedamage = DAMAGE_AIM;
	e->max_health = OBSTACLE_INITIAL_HEALTH + OBSTACLE_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = OBSTACLE_INITIAL_DAMAGE + OBSTACLE_ADDON_DAMAGE * skill_level;
	e->monsterinfo.nextattack = 100 - 9 * getTalentLevel(ent, TALENT_PHANTOM_OBSTACLE);
	e->monsterinfo.level = skill_level;
	e->gib_health = -250;
	e->die = obstacle_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -16, -16, 0);
	VectorSet(e->maxs, 16, 16, 40);
	e->mtype = M_OBSTACLE;

	ent->num_obstacle++;

	return e;
}

void Cmd_Obstacle_f (edict_t *ent)
{
	int		cost = OBSTACLE_COST;
	edict_t *obstacle;
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "obstacle", true);
		safe_cprintf(ent, PRINT_HIGH, "Obstacles removed\n");
		ent->num_obstacle = 0;
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

	if (!V_CanUseAbilities(ent, OBSTACLE, cost, true))
		return;

	if (ent->num_obstacle >= OBSTACLE_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of obstacles (%d)\n", OBSTACLE_MAX_COUNT);
		return;
	}

	obstacle = CreateObstacle(ent, ent->myskills.abilities[OBSTACLE].current_level);
	if (!G_GetSpawnLocation(ent, 100, obstacle->mins, obstacle->maxs, start))
	{
		ent->num_obstacle--;
		G_FreeEdict(obstacle);
		return;
	}
	VectorCopy(start, obstacle->s.origin);
	VectorCopy(ent->s.angles, obstacle->s.angles);
	obstacle->s.angles[PITCH] = 0;
	gi.linkentity(obstacle);
	obstacle->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Obstacle created (%d/%d)\n", ent->num_obstacle,OBSTACLE_MAX_COUNT);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + OBSTACLE_DELAY;
	//ent->holdtime = level.time + OBSTACLE_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}