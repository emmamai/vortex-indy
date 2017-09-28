#include "g_local.h"


void V_Push (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float maxvel = 300;

	// our activator or ally can push us
	if (other && other->inuse && other->client && self->activator && self->activator->inuse 
		&& (other == self->activator || IsAlly(self->activator, other)))
	{
		vec3_t forward, right, offset, start;

		// don't push if we are standing on this entity
		if (other->groundentity && other->groundentity == self)
			return;
		
		self->movetype_prev = self->movetype;
		self->movetype_frame = level.framenum + 5;
		self->movetype = MOVETYPE_STEP;

		AngleVectors (other->client->v_angle, forward, right, NULL);
		VectorScale (forward, -3, other->client->kick_origin);
		VectorSet(offset, 0, 7,  other->viewheight-8);
		P_ProjectSource (other->client, other->s.origin, offset, forward, right, start);

		VectorMA(self->velocity, 50, forward, self->velocity);

		// cap maximum velocity
		if (self->velocity[0] > maxvel)
			self->velocity[0] = maxvel;
		if (self->velocity[0] < -maxvel)
			self->velocity[0] = -maxvel;
		if (self->velocity[1] > maxvel)
			self->velocity[1] = maxvel;
		if (self->velocity[1] < -maxvel)
			self->velocity[1] = -maxvel;
		if (self->velocity[2] > maxvel)
			self->velocity[2] = maxvel;
		if (self->velocity[2] < -maxvel)
			self->velocity[2] = -maxvel;
	}
}

// touch function for all the gloom stuff
void organ_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(ent, other, plane, surf);

	// don't push or heal something that's already dead or invalid
	if (!ent || !ent->inuse || !ent->takedamage || ent->health < 1)
		return;

	V_Push(ent, other, plane, surf);

	if (G_EntIsAlive(other) && other->client && OnSameTeam(ent, other) 
		&& other->client->pers.inventory[power_cube_index] >= 5
		&& level.time > ent->lasthurt + 0.5 && ent->health < ent->max_health
		&& level.framenum > ent->monsterinfo.regen_delay1)
	{
		ent->health_cache += (int)(0.5 * ent->max_health);
		ent->monsterinfo.regen_delay1 = level.framenum + 10;
		other->client->pers.inventory[power_cube_index] -= 5;
	}
}

// generic remove function for all the gloom stuff
void organ_remove (edict_t *self, qboolean refund)
{
	if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
		return;

	if (self->mtype == M_COCOON)
	{
		// restore cocooned entity
		if (self->enemy && self->enemy->inuse)
		{
			self->enemy->movetype = self->count;
			self->enemy->svflags &= ~SVF_NOCLIENT;
			self->enemy->flags &= FL_COCOONED;//4.4
		}
	}

	if (self->activator && self->activator->inuse)
	{
		if (!self->monsterinfo.slots_freed)
		{
			if (self->mtype == M_OBSTACLE)
				self->activator->num_obstacle--;
			else if (self->mtype == M_SPIKER)
				self->activator->num_spikers--;
			else if (self->mtype == M_HEALER)
				self->activator->healer = NULL;
			else if (self->mtype == M_COCOON)
				self->activator->cocoon = NULL;
			else if (self->mtype == M_GASSER)
				self->activator->num_gasser--;
			else if (self->mtype == M_SPIKEBALL)
				self->activator->num_spikeball--;

			self->monsterinfo.slots_freed = true;
		}

		if (refund)
			self->activator->client->pers.inventory[power_cube_index] += (self->health / self->max_health) * self->monsterinfo.cost;
	}

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

qboolean organ_checkowner (edict_t *self)
{
	qboolean remove = false;

	// make sure activator exists
	if (!self->activator || !self->activator->inuse)
		remove = true;
	// make sure player activator is alive
	else if (self->activator->client && (self->activator->health < 1 
		|| (self->activator->client->respawn_time > level.time) 
		|| self->activator->deadflag == DEAD_DEAD))
		remove = true;

	if (remove)
	{
		organ_remove(self, false);
		return false;
	}

	return true;
}

void organ_restoreMoveType (edict_t *self)
{
	if (self->movetype_prev && level.framenum >= self->movetype_frame)
	{
		self->movetype = self->movetype_prev;
		self->movetype_prev = 0;
	}
}

void organ_removeall (edict_t *ent, char *classname, qboolean refund)
{
	edict_t *e = NULL;

	while((e = G_Find(e, FOFS(classname), classname)) != NULL) 
	{
		if (e && e->activator && e->activator->inuse && (e->activator == ent) && !RestorePreviousOwner(e))
			organ_remove(e, refund);
	}
}

//Talent: Exploding Bodies
qboolean organ_explode (edict_t *self)
{
	int damage, radius, talentLevel = getTalentLevel(self->activator, TALENT_EXPLODING_BODIES);

	if (talentLevel < 1 || self->style)
		return false;

	// cause the damage
	damage = radius = 100 * talentLevel;
	if (radius > 200)
		radius = 200;
	T_RadiusDamage (self, self, damage, self, radius, MOD_CORPSEEXPLODE);

	gi.sound (self, CHAN_VOICE, gi.soundindex (va("spells/corpse_explode%d.wav", GetRandom(1, 6))), 1, ATTN_NORM, 0);
	return true;
}