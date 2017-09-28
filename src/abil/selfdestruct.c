#include "g_local.h"

void Cmd_SelfDestruct_f(edict_t *self)
{
	int damage;

	if (!V_CanUseAbilities(self, SELFDESTRUCT, 0, true))
		return;

	damage = self->myskills.abilities[SELFDESTRUCT].current_level * SELFDESTRUCT_BONUS + SELFDESTRUCT_BASE;

	// do the damage
	T_RadiusDamage(self, self, damage, self, SELFDESTRUCT_RADIUS, MOD_SELFDESTRUCT);
	T_Damage(self, self, self, vec3_origin, self->s.origin, vec3_origin, damage * 0.6, 0, 0, MOD_SELFDESTRUCT);

	// GO BOOM!
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->client->ability_delay = level.time + 1;
	return;
}
