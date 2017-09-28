#include "g_local.h"

void lightningbolt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_EntExists(other))
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, 0, MOD_LIGHTNING);
	G_FreeEdict(self);
}

void lightningbolt_think (edict_t *self)
{
	// free self if our time is up
	if (level.time >= self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	// rotate animation frames
	self->s.frame++;
	if (self->s.frame > 2)
		self->s.frame = 0;

	//if (!(level.framenum%2))
	//	gi.dprintf("%f\n", VectorLength(self->velocity));

	self->nextthink = level.time + FRAMETIME;
}

void fire_lightningbolt (edict_t *ent, vec3_t start, vec3_t dir, float speed, int damage)
{
	edict_t *bolt;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// create bolt
	bolt = G_Spawn();
	bolt->s.modelindex = gi.modelindex ("models/proj/beam/tris.md2");
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_TAGTRAIL;
	//bolt->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = ent;
	bolt->think = lightningbolt_think;
	bolt->touch = lightningbolt_touch;
	bolt->nextthink = level.time + FRAMETIME;
	bolt->delay = level.time + 10;
	bolt->dmg = damage;
	bolt->classname = "lightning bolt";	
	gi.linkentity (bolt);

	/*
	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/
	
	gi.sound(ent, CHAN_ITEM, gi.soundindex("spells/zap2.wav"), 1, ATTN_NORM, 0);
	//ent->client->ability_delay = level.time+BOLT_DELAY;
}

void G_SpawnParticleTrail (vec3_t start, vec3_t end, int particles, int color)
{
	int		i, spacing;
	float	dist;
	vec3_t	dir, org;

	VectorCopy(start, org);
	VectorSubtract(end, start, dir);
	dist = VectorLength(dir);
	VectorNormalize(dir);

	spacing = particles+11;

	// particle effects
	for (i=0; i<(int)dist; i+=spacing)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_LASER_SPARKS);
		gi.WriteByte (particles);
		gi.WritePosition (org);
		gi.WriteDir (dir);
		gi.WriteByte (color);
		gi.multicast (org, MULTICAST_PVS);

		// move starting position forward
		VectorMA(org, spacing, dir, org);
	}
}

#define CLIGHTNING_MAX_HOPS			4
#define CLIGHTNING_COLOR			15
#define CLIGHTNING_PARTICLES		3
#define CLIGHTNING_INITIAL_DMG		50
#define CLIGHTNING_ADDON_DMG		15
#define CLIGHTNING_INITIAL_AR		256
#define CLIGHTNING_ADDON_AR			0
#define CLIGHTNING_INITIAL_HR		256
#define CLIGHTNING_ADDON_HR			0
#define CLIGHTNING_DELAY			0.3
#define CLIGHTNING_DMG_MOD			1.25

#define CL_CHECK_AND_ATTACK			1		// check if target is valid, then attack
#define CL_CHECK					2		// check if the target is valid
#define CL_ATTACK					3		// attack only

qboolean ChainLightning_Attack (edict_t *ent, edict_t *target, int damage, int mode)
{
	qboolean result=false;


	if (mode != CL_ATTACK)
	{
		if (G_ValidTargetEnt(target, false) && !OnSameTeam(ent, target))
			result = true;
	}
	else
		result = true;

	if (result && mode != CL_CHECK)
	{
		//gi.dprintf("CL did %d damage at %d\n", damage, level.framenum);
		// deal damage
		T_Damage(target, ent, ent, vec3_origin, target->s.origin, vec3_origin, 
			damage, damage, DAMAGE_ENERGY, MOD_LIGHTNING);
	}

	return result;
}

void ChainLightning (edict_t *ent, vec3_t start, vec3_t aimdir, int damage, int attack_range, int hop_range)
{
	int		i=0, hops=CLIGHTNING_MAX_HOPS;
	vec3_t	end;
	trace_t	tr;
	edict_t	*target=NULL;
	edict_t	*prev_ed[CLIGHTNING_MAX_HOPS]; // list of entities we've previously hit
	qboolean	found=false;

	memset(prev_ed, 0, CLIGHTNING_MAX_HOPS*sizeof(prev_ed[0]));

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// play sound
	gi.sound(ent, CHAN_ITEM, gi.soundindex("spells/thunderbolt.wav"), 1, ATTN_NORM, 0);

	// randomize damage
	//damage = GetRandom((int)(0.5*damage), damage);

	// get ending position
	VectorMA(start, attack_range, aimdir, end);

	// trace from attacker to ending position
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// is this a non-world entity?
	if (tr.ent && tr.ent != world)
	{
		// try to attack it
		if (ChainLightning_Attack(ent, tr.ent, damage, CL_CHECK_AND_ATTACK))
		{
			// damage is modified with each hop
			damage *= CLIGHTNING_DMG_MOD; 

			prev_ed[0] = tr.ent;
		}
		else
			return; // give up
	}
	
	
	// spawn particle trail
	G_SpawnParticleTrail(start, tr.endpos, CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

	// we didn't find an entity to jump from
	while (!prev_ed[0] && hops > 0)
	{
		hops--;

		VectorCopy(tr.endpos, start);

		// bounce away from the wall
		VectorMA(aimdir, (-2 * DotProduct(aimdir, tr.plane.normal)), tr.plane.normal, aimdir);
		VectorMA(start, attack_range, aimdir, end);
			
		// trace
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);

		// spawn particle trail
		G_SpawnParticleTrail(start, tr.endpos, CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

		// we hit nothing, give up
		if (tr.fraction == 1.0)
			return;

		// we hit an entity
		if (tr.ent && tr.ent != world)
		{
			// try to attack
			if (ChainLightning_Attack(ent, tr.ent, damage, CL_CHECK_AND_ATTACK))
			{
				// damage is modified with each hop
				damage *= CLIGHTNING_DMG_MOD; 

				prev_ed[0] = tr.ent;
				break;
			}
			else
				return;// give up
		}
		//FIXME: if we didn't hit an entity, shouldn't we modify damage again?
	}
	
	// we never hit a valid target, so give up
	if (!prev_ed[0])
		return;

	// find nearby targets and bounce between them
	while ((i<CLIGHTNING_MAX_HOPS-1) && ((target = findradius(target, prev_ed[i]->s.origin, hop_range)) != NULL))
	{
		if (target == prev_ed[0])
			continue;

		// try to attack, if successful then add entity to list
		if (ChainLightning_Attack(ent, target, 0, CL_CHECK) && visible(prev_ed[i], target))
		{
			ChainLightning_Attack(ent, target, damage, CL_ATTACK);

			// damage is modified with each hop
			damage *= CLIGHTNING_DMG_MOD; 

			G_SpawnParticleTrail(prev_ed[i]->s.origin, target->s.origin, 
				CLIGHTNING_PARTICLES, CLIGHTNING_COLOR);

			prev_ed[++i] = target;
		}
	}
}

void Cmd_ChainLightning_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int damage=CLIGHTNING_INITIAL_DMG+CLIGHTNING_ADDON_DMG*ent->myskills.abilities[LIGHTNING].current_level;
	int attack_range=CLIGHTNING_INITIAL_AR+CLIGHTNING_ADDON_AR*ent->myskills.abilities[LIGHTNING].current_level;
	int hop_range=CLIGHTNING_INITIAL_HR+CLIGHTNING_ADDON_HR*ent->myskills.abilities[LIGHTNING].current_level;
	int cost=CLIGHTNING_COST*cost_mult;
	vec3_t start, forward, right, offset;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[LIGHTNING].current_level, cost))
		return;
	if (ent->myskills.abilities[LIGHTNING].disable)
		return;

	damage *= skill_mult;

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	ChainLightning(ent, start, forward, damage, attack_range, hop_range);

	ent->client->ability_delay = level.time + CLIGHTNING_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;
}