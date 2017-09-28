#include "g_local.h"

void BecomeExplosion2 (edict_t *self);
void Cmd_CorpseExplode(edict_t *ent)
{
	int		damage, min_dmg, max_dmg, slvl;
	float	fraction, radius;
	vec3_t	start, end, forward, right, offset;
	trace_t	tr;
	edict_t *e=NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_CorpseExplode()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[CORPSE_EXPLODE].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[CORPSE_EXPLODE].current_level, COST_FOR_CORPSEEXPLODE))
		return;

	slvl = ent->myskills.abilities[CORPSE_EXPLODE].current_level;
	radius = CORPSE_EXPLOSION_INITIAL_RADIUS + CORPSE_EXPLOSION_ADDON_RADIUS * slvl;

	// calculate starting position
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorMA(start, CORPSE_EXPLOSION_MAX_RANGE, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);

	while ((e = findclosestradius (e, tr.endpos, CORPSE_EXPLOSION_SEARCH_RADIUS)) != NULL)
	{
		if (!G_EntExists(e))
			continue;
		if (e->health > 0)
			continue;
		if (e->max_health < 1)
			continue;

		// kill the corpse
		T_Damage(e, e, ent, vec3_origin, e->s.origin, vec3_origin, 10000, 0, DAMAGE_NO_PROTECTION, MOD_CORPSEEXPLODE);

		// inflict damage
		fraction = 0.1 * GetRandom(5, 10);
		damage = fraction * e->max_health;
		
		// calculate min/max damage range
		min_dmg = CORPSE_EXPLOSION_INITIAL_DAMAGE + CORPSE_EXPLOSION_ADDON_DAMAGE * slvl;
		max_dmg = 5 * min_dmg;

		if (damage < min_dmg)
			damage = min_dmg;
		else if (damage > max_dmg)
			damage = max_dmg;

		T_RadiusDamage (e, ent, damage, NULL, radius, MOD_CORPSEEXPLODE);

		//gi.dprintf("fraction %.1f, damage %d, max_health: %d\n", fraction, damage, e->max_health);

		//Spells like corpse explode shouldn't display 10000 damage, so show the corpse damage instead
		ent->client->ps.stats[STAT_ID_DAMAGE] = damage;

		gi.sound(e, CHAN_ITEM, gi.soundindex("spells/corpseexplodecast.wav"), 1, ATTN_NORM, 0);
		ent->client->pers.inventory[power_cube_index] -= COST_FOR_CORPSEEXPLODE;
		ent->client->ability_delay = level.time + DELAY_CORPSEEXPLODE;
		safe_cprintf(ent, PRINT_HIGH, "Corpse exploded for %d damage!\n", damage);

		//decino: explosion effect
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_GRENADE_EXPLOSION);
		gi.WritePosition (e->s.origin);
		gi.multicast (e->s.origin, MULTICAST_PVS);

		//decino: shoot 6 gibs that deal damage
		//az: todo, more copypaste.
		/*for (i = 0; i < 10; i++)
		{
			e->s.angles[YAW] += 36;
			AngleCheck(&e->s.angles[YAW]);

			AngleVectors(e->s.angles, forward, NULL, up);
			fire_gib(ent, e->s.origin, forward, dmg, 0, 1000);
		}*/

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;

		break;
	}
}

void Vampire_Think(edict_t *self)
{
	if (self->myskills.abilities[VAMPIRE].current_level > 5)
	{
		if (!level.daytime && self->deadflag != DEAD_DEAD && self->solid != SOLID_NOT)
		{
			if (!self->client->lowlight){
				self->client->ps.rdflags |= RDF_IRGOGGLES;
				self->client->lowlight = true;
			}
		}
		else
		{
			if (self->client->lowlight){
				self->client->ps.rdflags &= ~RDF_IRGOGGLES;
				self->client->lowlight = false;
			}
		}
	}
}