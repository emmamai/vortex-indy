#include "g_local.h"

#define CRIPPLE_COST			25
#define CRIPPLE_DELAY			2.0
#define CRIPPLE_RANGE			256
#define CRIPPLE_MAX_DAMAGE		500

void CrippleAttack (edict_t *ent)
{
	int		damage;
	vec3_t	end;
	trace_t	tr;
	edict_t	*e=NULL;

	while ((e = findclosestreticle(e, ent, CRIPPLE_RANGE)) != NULL)
	{
		if (!G_ValidTarget(ent, e, true))
			continue;
		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;

		damage = e->health * (1-(1/(1+0.2*ent->myskills.abilities[CRIPPLE].current_level)));
		if (damage > CRIPPLE_MAX_DAMAGE)
			damage = CRIPPLE_MAX_DAMAGE;
		T_Damage(e, ent, ent, vec3_origin, e->s.origin, vec3_origin, damage, 0, DAMAGE_NO_ABILITIES, MOD_CRIPPLE);

		VectorCopy(e->s.origin, end);
		end[2] += 8192;
		tr = gi.trace(e->s.origin, NULL, NULL, end, e, MASK_SHOT);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_MONSTER_HEATBEAM);
		gi.WriteShort (e-g_edicts);
		gi.WritePosition (e->s.origin);
		gi.WritePosition (tr.endpos);
		gi.multicast (e->s.origin, MULTICAST_PVS);
		break;
	}

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	
	gi.sound (ent, CHAN_WEAPON, gi.soundindex("spells/eleccast.wav"), 1, ATTN_NORM, 0);
	ent->client->ability_delay = level.time + CRIPPLE_DELAY;
	ent->client->pers.inventory[power_cube_index]-=CRIPPLE_COST;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

void Cmd_Cripple_f (edict_t *ent)
{
	int	ability_level=ent->myskills.abilities[CRIPPLE].current_level;

	if (!G_CanUseAbilities(ent, ability_level, CRIPPLE_COST))
		return;
	if (ent->myskills.abilities[CRIPPLE].disable)
		return;

	//3.0 amnesia disables cripple
	if (que_findtype(ent->curses, NULL, AMNESIA) != NULL)
		return;
	CrippleAttack(ent);
}