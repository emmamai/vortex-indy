#include "g_local.h"
#include "organ.h"

#define GASSER_FRAMES_ATTACK_START		1	
#define GASSER_FRAMES_ATTACK_END		6
#define GASSER_FRAMES_REARM_START		7
#define GASSER_FRAMES_REARM_END			8
#define GASSER_FRAMES_IDLE_START		9
#define GASSER_FRAMES_IDLE_END			12
#define GASSER_FRAME_DEAD				6

#define GASSER_RANGE					128
#define GASSER_REFIRE					5.0
#define GASSER_INITIAL_DAMAGE			0
#define GASSER_ADDON_DAMAGE				10
#define GASSER_INITIAL_HEALTH			100
#define GASSER_ADDON_HEALTH				50
#define GASSER_INITIAL_ATTACK_RANGE		100
#define GASSER_ADDON_ATTACK_RANGE		0
#define GASSER_COST						25
#define GASSER_DELAY					1.0

#define GASCLOUD_FRAMES_GROW_START		10
#define GASCLOUD_FRAMES_GROW_END		18
#define GASCLOUD_FRAMES_IDLE_START		19
#define GASCLOUD_FRAMES_IDLE_END		23

#define GASCLOUD_POISON_DURATION		10.0
#define GASCLOUD_POISON_FACTOR			0.1

void poison_think (edict_t *self)
{
	if (!G_EntIsAlive(self->enemy) || !G_EntIsAlive(self->activator) || (level.time > self->delay))
	{
		G_FreeEdict(self);
		return;
	}

	if (level.framenum >= self->monsterinfo.nextattack)
	{
		T_Damage(self->enemy, self, self->activator, vec3_origin, self->enemy->s.origin, vec3_origin, self->dmg, 0, 0, self->style);
		self->monsterinfo.nextattack = level.framenum + floattoint(self->random);
		self->random *= 1.25;
	}

	self->nextthink = level.time + FRAMETIME;
}

void CreatePoison (edict_t *ent, edict_t *targ, int damage, float duration, int meansOfDeath)
{
	edict_t *e;

	e = G_Spawn();
	e->activator = ent;
	e->solid = SOLID_NOT;
	e->movetype = MOVETYPE_NOCLIP;
	e->svflags |= SVF_NOCLIENT;
	e->classname = "poison";
	e->delay = level.time + duration;
	e->owner = e->enemy = targ;
	e->random = 1; // starting refire delay (in frames)
	e->dmg = damage;
	e->mtype = POISON;
	e->style = meansOfDeath;
	e->think = poison_think;
	e->nextthink = level.time + FRAMETIME;
	VectorCopy(targ->s.origin, e->s.origin);
	gi.linkentity(e);

	if (!que_addent(targ->curses, e, duration))
		G_FreeEdict(e);
}

void gascloud_sparks (edict_t *self, int num, int radius)
{
	int		i;
	vec3_t	start;

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	for (i=0; i<num; i++)
	{
		VectorCopy(self->s.origin, start);
		start[0] += crandom() * GetRandom(0, radius);
		start[1] += crandom() * GetRandom(0, radius);
		start[2] += crandom() * GetRandom(0, radius);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(GetRandom(200, 209)); // color
		gi.multicast(start, MULTICAST_PVS);
	}
}

void gascloud_runframes (edict_t *self)
{
	if (level.time > self->delay - 0.8)
	{
		G_RunFrames(self, GASCLOUD_FRAMES_GROW_START, GASCLOUD_FRAMES_GROW_END, true);
		self->s.effects |= EF_SPHERETRANS;
	}
	else if (self->s.frame < GASCLOUD_FRAMES_GROW_END)
		G_RunFrames(self, GASCLOUD_FRAMES_GROW_START, GASCLOUD_FRAMES_GROW_END, false);
}

void gascloud_attack (edict_t *self)
{
	que_t	*slot=NULL;
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (G_ValidTarget(self, e, true))
		{
			// otherwise, update the attack frequency to once per server frame
			if ((slot = que_findtype(e->curses, NULL, POISON)) != NULL)
			{
				slot->ent->random = 1;
				slot->ent->monsterinfo.nextattack = level.framenum + 1;
				slot->ent->delay = level.time + self->radius_dmg;
				slot->time = level.time + self->radius_dmg;
			}
			// if target is not already poisoned, create poison entity
			else
				CreatePoison(self->activator, e, (int)(GASCLOUD_POISON_FACTOR*self->dmg), self->radius_dmg, MOD_GAS);
			T_Damage(e, self, self->activator, vec3_origin, e->s.origin, vec3_origin, (int)((1.0-GASCLOUD_POISON_FACTOR)*self->dmg), 0, 0, MOD_GAS);
		}
		else if (e && e->inuse && e->takedamage && visible(self, e) && !OnSameTeam(self, e))
			T_Damage(e, self, self->activator, vec3_origin, e->s.origin, vec3_origin, self->dmg, 0, 0, MOD_GAS);
	}
}

void gascloud_move (edict_t *self)
{
	trace_t tr;
	vec3_t	start;

	VectorCopy(self->s.origin, start);
	start[2]++;
	tr = gi.trace(self->s.origin, NULL, NULL, start, NULL, MASK_SOLID);
	if (tr.fraction == 1.0)
	{
		self->s.origin[2]++;
		gi.linkentity(self);
	}
}

void gascloud_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator) || level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}
	
	gascloud_sparks(self, 3, (int)self->dmg_radius);
	gascloud_attack(self);
	gascloud_move(self);
	gascloud_runframes(self);
	
	self->nextthink = level.time + FRAMETIME;	
}

void SpawnGasCloud (edict_t *ent, vec3_t start, int damage, float radius, float duration)
{
	edict_t *e;

	e = G_Spawn();
	e->activator = ent->activator;
	e->solid = SOLID_NOT;
	e->movetype = MOVETYPE_NOCLIP;
	e->classname = "gas cloud";
	e->delay = level.time + duration;
	e->radius_dmg = GASCLOUD_POISON_DURATION; // poison effect duration
	e->dmg = damage;
	e->dmg_radius = radius;
	e->think = gascloud_think;
	e->s.modelindex = gi.modelindex ("models/objects/smokexp/tris.md2");
	e->s.skinnum = 1;
	e->s.effects |= EF_PLASMA;
	e->nextthink = level.time + FRAMETIME;
	VectorCopy(start, e->s.origin);
	VectorCopy(ent->s.angles, e->s.angles);
	gi.linkentity(e);
}

void fire_acid (edict_t *self, vec3_t start, vec3_t aimdir, int projectile_damage, float radius, 
				int speed, int acid_damage, float acid_duration);

void gasser_acidattack (edict_t *self)
{
	float	dist;
	float	range=384;
	int		speed=600;
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

		fire_acid(self, self->s.origin, forward, 200, 64, speed, 20, 10.0);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + 2.0;
		self->s.frame = GASSER_FRAMES_ATTACK_END-2;
		//gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	}	
}

void gasser_attack (edict_t *self)
{
	vec3_t start;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] + 8;
	SpawnGasCloud(self, start, self->dmg, self->dmg_radius, 4.0);

	self->s.frame = GASSER_FRAMES_ATTACK_START+2;
	self->monsterinfo.attack_finished = level.time + GASSER_REFIRE;
}

qboolean gasser_findtarget (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, GASSER_RANGE)) != NULL)
	{
		if (!G_ValidTarget(self, e, true))
			continue;
		self->enemy = e;
		return true;
	}
	self->enemy = NULL;
	return false;
}

void gasser_think (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	if (gasser_findtarget(self))
		gasser_attack(self);
	//gasser_acidattack(self);

	if (self->s.frame < GASSER_FRAMES_ATTACK_END)
		G_RunFrames(self, GASSER_FRAMES_ATTACK_START, GASSER_FRAMES_ATTACK_END, false);
	else if (self->s.frame < GASSER_FRAMES_REARM_END && level.time > self->monsterinfo.attack_finished - 0.2)
		G_RunFrames(self, GASSER_FRAMES_REARM_START, GASSER_FRAMES_REARM_END, false);
	else if (level.time > self->monsterinfo.attack_finished)
	{
		gascloud_sparks(self, 1, 32);
		G_RunFrames(self, GASSER_FRAMES_IDLE_START, GASSER_FRAMES_IDLE_END, false);
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

	self->nextthink = level.time + FRAMETIME;
}

void gasser_grow (edict_t *self)
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

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->s.effects |= EF_PLASMA;

	if (self->health >= self->max_health)
	{
		self->style = 0;//done growing
		self->think = gasser_think;
	}

	self->nextthink = level.time + FRAMETIME;
}

void gasser_dead (edict_t *self)
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

void gasser_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = GASSER_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_gasser--;
		cur = self->activator->num_gasser;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
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

	self->think = gasser_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = GASSER_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	gi.linkentity(self);
}

edict_t *CreateGasser (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = gasser_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/gas/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;// tells physics to clip on any solid object (not just walls)
	e->mass = 500;
	e->classname = "gasser";
	e->takedamage = DAMAGE_AIM;
	e->max_health = GASSER_INITIAL_HEALTH + GASSER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = GASSER_INITIAL_DAMAGE + GASSER_ADDON_DAMAGE * skill_level;
	e->dmg_radius = GASSER_INITIAL_ATTACK_RANGE + GASSER_ADDON_ATTACK_RANGE * skill_level;

	e->monsterinfo.level = skill_level;
	e->gib_health = -250;
	e->s.frame = GASSER_FRAMES_IDLE_START;
	e->die = gasser_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -8, -8, -5);
	VectorSet(e->maxs, 8, 8, 15);
	e->mtype = M_GASSER;

	ent->num_gasser++;

	return e;
}

void Cmd_Gasser_f (edict_t *ent)
{
	int		cost = GASSER_COST;
	edict_t *gasser;
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "gasser", true);
		safe_cprintf(ent, PRINT_HIGH, "Gassers removed\n");
		ent->num_gasser = 0;
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

	if (!V_CanUseAbilities(ent, GASSER, cost, true))
		return;

	if (ent->num_gasser >= GASSER_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of gassers (%d)\n", GASSER_MAX_COUNT);
		return;
	}

	gasser = CreateGasser(ent, ent->myskills.abilities[GASSER].current_level);
	if (!G_GetSpawnLocation(ent, 100, gasser->mins, gasser->maxs, start))
	{
		ent->num_gasser--;
		G_FreeEdict(gasser);
		return;
	}
	VectorCopy(start, gasser->s.origin);
	VectorCopy(ent->s.angles, gasser->s.angles);
	gasser->s.angles[PITCH] = 0;
	gi.linkentity(gasser);
	gasser->monsterinfo.attack_finished = level.time + 1.0;
	gasser->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Gasser created (%d/%d)\n", ent->num_gasser, GASSER_MAX_COUNT);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + GASSER_DELAY;
	//ent->holdtime = level.time + GASSER_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}