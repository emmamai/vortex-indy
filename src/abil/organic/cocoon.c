#include "g_local.h"
#include "organ.h"

#define COCOON_FRAMES_IDLE_START	73//36
#define COCOON_FRAMES_IDLE_END		87//50
#define COCOON_FRAME_STANDBY		89//52
#define COCOON_FRAMES_GROW_START	90//53
#define COCOON_FRAMES_GROW_END		108//72

#define COCOON_INITIAL_HEALTH		0
#define COCOON_ADDON_HEALTH			100
#define COCOON_INITIAL_DURATION		50		// number of frames enemy will remain in cocoon
#define COCOON_ADDON_DURATION		0
#define COCOON_MINIMUM_DURATION		50
#define COCOON_INITIAL_FACTOR		1.0
#define COCOON_ADDON_FACTOR			0.05
#define COCOON_INITIAL_TIME			30.0
#define COCOON_ADDON_TIME			1.5
#define COCOON_COST					50
#define COCOON_DELAY				1.0

void cocoon_dead (edict_t *self)
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

void cocoon_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->cocoon = NULL;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by %s\n", attacker->client->pers.netname);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by a %s\n", V_GetMonsterName(attacker));
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by a %s\n", attacker->classname);
	}

	// restore cocooned entity
	if (self->enemy && self->enemy->inuse && !self->deadflag)
	{
		self->enemy->movetype = self->count;
		self->enemy->svflags &= ~SVF_NOCLIENT;
		self->enemy->flags &= FL_COCOONED;

		if (self->enemy->client)
		{
			self->enemy->holdtime = level.time + 0.5;
			self->enemy->client->ability_delay = level.time + 0.5;
		}
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

	self->think = cocoon_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = COCOON_FRAME_STANDBY;
	self->movetype = MOVETYPE_TOSS;
	self->maxs[2] = 0;
	gi.linkentity(self);
}

void cocoon_cloak (edict_t *self)
{
	// cloak disabled or not upgraded
	if (self->monsterinfo.jumpdn == -1)
		return;

	// don't cloak if we're holding the flag carrier!
	if (self->enemy->client && (self->enemy->client->pers.inventory[flag_index] || 
		self->enemy->client->pers.inventory[flag_index] || 
		self->enemy->client->pers.inventory[flag_index]))
		return;

	// already cloaked
	if (self->svflags & SVF_NOCLIENT)
		return;
	
	// cloak after jumpdn frames of attacking
	if (self->monsterinfo.jumpup >= self->monsterinfo.jumpdn)
		self->svflags |= SVF_NOCLIENT;
}

void cocoon_attack (edict_t *self)
{
	int		frames;
	float	time;
	vec3_t	start;

	if (!G_EntIsAlive(self->enemy))
	{
		if (self->enemy)
			self->enemy = NULL;
		return;
	}
	
	cocoon_cloak(self);

	if (level.framenum >= self->monsterinfo.nextattack)
	{
		int		heal;
		float	duration = COCOON_INITIAL_TIME + COCOON_ADDON_TIME * self->monsterinfo.level;
		float	factor = COCOON_INITIAL_FACTOR + COCOON_ADDON_FACTOR * self->monsterinfo.level;

		// give them a damage/defense bonus for awhile
		self->enemy->cocoon_time = level.time + duration;
		self->enemy->cocoon_factor = factor;

		if (self->enemy->client)
			safe_cprintf(self->enemy, PRINT_HIGH, "You have gained a damage/defense bonus of +%.0f%c for %.0f seconds\n", 
				(factor * 100) - 100, '%', duration); 
		
		//4.4 give some health
		heal = self->enemy->max_health * (0.25 + (0.075 * self->monsterinfo.level));
		if (self->enemy->health < self->enemy->max_health)
		{
			self->enemy->health += heal;
			if (self->enemy->health > self->enemy->max_health)
				self->enemy->health = self->enemy->max_health;
		}
		
		
		//Talent: Phantom Cocoon - decloak when entity emerges
		self->svflags &= ~SVF_NOCLIENT; 
		self->monsterinfo.jumpup = 0;

		self->enemy->svflags &= ~SVF_NOCLIENT;
		self->enemy->movetype = self->count;
		self->enemy->flags &= ~FL_COCOONED;//4.4
	//	self->owner = self->enemy;
		self->enemy = NULL;
		self->s.frame = COCOON_FRAME_STANDBY;
		self->maxs[2] = 0;//shorten bbox
		self->touch = V_Touch;
		return;
	}

	if (!(level.framenum % 10) && self->enemy->client)
		safe_cprintf(self->enemy, PRINT_HIGH, "You will emerge from the cocoon in %d second(s)\n", 
			(int)((self->monsterinfo.nextattack - level.framenum) / 10));

	time = level.time + FRAMETIME;

	// hold target in-place
	if (!strcmp(self->enemy->classname, "drone"))
	{
		self->enemy->monsterinfo.pausetime = time;
		self->enemy->monsterinfo.stand(self->enemy);
	}
	else
		self->enemy->holdtime = time;

	// keep morphed players from shooting
	if (PM_MonsterHasPilot(self->enemy))
	{
		self->enemy->owner->client->ability_delay = time;
		self->enemy->owner->monsterinfo.attack_finished = time;
	}
	else
	{
		// no using abilities!
		if (self->enemy->client)
			self->enemy->client->ability_delay = time;
		//if(strcmp(self->enemy->classname, "spiker") != 0)
		if (self->enemy->mtype == M_SPIKER)
			self->enemy->monsterinfo.attack_finished = time;
	}
	
	// move position
	VectorCopy(self->s.origin, start);
	start[2] += fabs(self->enemy->mins[2]) + 1;
	VectorCopy(start, self->enemy->s.origin);
	gi.linkentity(self->enemy);
	
	// hide them
	self->enemy->svflags |= SVF_NOCLIENT;

	frames = COCOON_INITIAL_DURATION + COCOON_ADDON_DURATION * self->monsterinfo.level;
	if (frames < COCOON_MINIMUM_DURATION)
		frames = COCOON_MINIMUM_DURATION;

	/*if (M_Regenerate(self->enemy, frames, 1, true, true, false, &self->monsterinfo.regen_delay2) && (level.time > self->msg_time))
	{
		gi.sound(self, CHAN_AUTO, gi.soundindex("organ/healer1.wav"), 1, ATTN_NORM, 0);
		self->msg_time = level.time + 1.5;
	}
	*/

	self->monsterinfo.jumpup++;//Talent: Phantom Cocoon - keep track of attack frames
}

void cocoon_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int frames;

	V_Touch(ent, other, plane, surf);
	
	if (!ent->groundentity || ent->groundentity != world)
		return;
	if (level.framenum < ent->monsterinfo.nextattack)
		return;
	if (!G_ValidTargetEnt(other, true) || !OnSameTeam(ent, other))
		return;
	if (other->mtype == M_SPIKEBALL)//4.4
		return;
	if (other->movetype == MOVETYPE_NONE)
		return;

	ent->enemy = other;

	frames = COCOON_INITIAL_DURATION + COCOON_ADDON_DURATION * ent->monsterinfo.level;
	if (frames < COCOON_MINIMUM_DURATION)
		frames = COCOON_MINIMUM_DURATION;
	ent->monsterinfo.nextattack = level.framenum + frames;

	// don't let them move (or fall out of the map)
	ent->count = other->movetype;
	other->movetype = MOVETYPE_NONE;
	other->flags |= FL_COCOONED;//4.4

	if (other->client)
		safe_cprintf(other, PRINT_HIGH, "You have been cocooned for %d seconds\n", (int)(frames / 10));
}

void cocoon_think (edict_t *self)
{
	trace_t tr;

	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	cocoon_attack(self);

	if (level.time > self->lasthurt + 1.0)
		M_Regenerate(self, 300, 10,  1.0, true, false, false, &self->monsterinfo.regen_delay1);

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

	if (self->s.frame == COCOON_FRAME_STANDBY)
	{
		vec3_t maxs;

		VectorCopy(self->maxs, maxs);
		maxs[2] = 40;

		tr = gi.trace(self->s.origin, self->mins, maxs, self->s.origin, self, MASK_SHOT);
		if (tr.fraction == 1.0)
		{
			//self->touch = cocoon_touch;
			self->maxs[2] = 40;
			self->s.frame++;
		}
		return;
	}
	else if (self->s.frame > COCOON_FRAME_STANDBY && self->s.frame < COCOON_FRAMES_GROW_END)
	{
		G_RunFrames(self, COCOON_FRAMES_GROW_START, COCOON_FRAMES_GROW_END, false);
	}
	else if (self->s.frame == COCOON_FRAMES_GROW_END)
	{
		self->style = 0; //done growing
		self->touch = cocoon_touch;
		self->s.frame = COCOON_FRAMES_IDLE_START;
	}
	else
		G_RunFrames(self, COCOON_FRAMES_IDLE_START, COCOON_FRAMES_IDLE_END, false);
}

edict_t *CreateCocoon (edict_t *ent, int skill_level)
{
	//Talent: Phantom Cocoon
	int talentLevel = getTalentLevel(ent, TALENT_PHANTOM_COCOON);

	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = cocoon_think;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/cocoon/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "cocoon";
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = COCOON_INITIAL_HEALTH + COCOON_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	
	//Talent: Phantom Cocoon - frames before cloaking
	if (talentLevel > 0)
		e->monsterinfo.jumpdn = 50 - 8 * talentLevel;
	else
		e->monsterinfo.jumpdn = -1; // cloak disabled

	e->gib_health = -200;
	e->s.frame = COCOON_FRAME_STANDBY;
	e->die = cocoon_die;
	e->touch = V_Touch;
	VectorSet(e->mins, -50, -50, -40);
	VectorSet(e->maxs, 50, 50, 40);
	e->mtype = M_COCOON;

	ent->cocoon = e;

	return e;
}

void Cmd_Cocoon_f (edict_t *ent)
{
	edict_t *cocoon;
	vec3_t	start;

	if (ent->cocoon && ent->cocoon->inuse)
	{
		organ_remove(ent->cocoon, true);
		safe_cprintf(ent, PRINT_HIGH, "Cocoon removed\n");
		return;
	}

	if (!V_CanUseAbilities(ent, COCOON, COCOON_COST, true))
		return;

	cocoon = CreateCocoon(ent, ent->myskills.abilities[COCOON].current_level);
	if (!G_GetSpawnLocation(ent, 100, cocoon->mins, cocoon->maxs, start))
	{
		ent->cocoon = NULL;
		G_FreeEdict(cocoon);
		return;
	}
	VectorCopy(start, cocoon->s.origin);
	VectorCopy(ent->s.angles, cocoon->s.angles);
	cocoon->s.angles[PITCH] = 0;
	gi.linkentity(cocoon);
	cocoon->monsterinfo.cost = COCOON_COST;

	safe_cprintf(ent, PRINT_HIGH, "Cocoon created\n");
	gi.sound(cocoon, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_STATIC, 0);

	ent->client->pers.inventory[power_cube_index] -= COCOON_COST;
	ent->client->ability_delay = level.time + COCOON_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}