#include "g_local.h"

void TeleportPlayer (edict_t *player)
{
	int dist = 512;
	vec3_t forward, start, end;
	trace_t tr;

	//GHz: Calculate end point and trace
	VectorCopy(player->s.origin, start);
	start[2]++;
	AngleVectors (player->s.angles, forward, NULL, NULL);
	//GHz: Keep trying until we get a valid spot
	while (dist > 0){
		VectorMA(start, dist, forward, end);
		tr = gi.trace(start, player->mins, player->maxs, end, player, MASK_SOLID);
		if (!(tr.contents & MASK_SOLID) && tr.fraction == 1.0)
		{
			player->s.event = EV_PLAYER_TELEPORT;
			VectorCopy(tr.endpos, player->s.origin);

			//GHz: Hold in place briefly
			player->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
			player->client->ps.pmove.pm_time = 14;
			//player->client->respawn_time = level.time;

			dist = 0;//GHz: We found a spot, so we're done
			VectorClear(player->velocity);
			gi.linkentity(player);
		}
		else
			dist -= 10;//GHz: No luck, so try further back
	}

}

qboolean ValidTeleportSpot (edict_t *ent, vec3_t spot)
{
	vec3_t	start, forward, right;
	trace_t	tr;

	AngleVectors(ent->s.angles, forward, right, NULL);

	// check above
	VectorCopy(spot, start);
	start[2] += 128;
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check left
	VectorMA(spot, -128, right, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check right
	VectorMA(spot, 128, right, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check forward
	VectorMA(spot, 128, forward, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check behind
	VectorMA(spot, -128, forward, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	return true;
}

void TeleportForward (edict_t *ent)
{
	int		dist = 512;
	vec3_t	angles, offset, forward, right, start, end;
	trace_t	tr;

	if (!G_EntIsAlive(ent))
		return;
	//4.07 can't teleport while being hurt
	if (ent->lasthurt+DAMAGE_ESCAPE_DELAY > level.time)
		return;
	if (HasFlag(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't use this while carrying the flag!\n");
		return;
	}

	if (!V_CanUseAbilities(ent, TELEPORT, TELEPORT_COST, true))
		return;
	
	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	RemoveAllDrones(ent, false);
	RemoveHellspawn(ent);
	RemoveMiniSentries(ent);
//	VortexRemovePlayerSummonables(ent);

	// get forward vector
	if (!ent->client)
	{
		VectorCopy(ent->s.angles, angles);
		angles[PITCH] = 0;
		AngleVectors(angles, forward, NULL, NULL);
	}
	else
	{
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 0, 7,  ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	}
	// get starting position
	VectorCopy(ent->s.origin, start);
	start[2]++;

	// in CTF, you can't teleport thru solid objects
	/*
	if (ctf->value)
	{
		VectorMA(start, dist, forward, end);
		tr = gi.trace(start, ent->mins, ent->maxs, end, ent, MASK_SHOT);

		ent->s.event = EV_PLAYER_TELEPORT;
		VectorCopy(tr.endpos, ent->s.origin);
		VectorCopy(tr.endpos, ent->s.old_origin);
		VectorClear(ent->velocity);
		gi.linkentity(ent);
		ent->client->pers.inventory[power_cube_index]-=TELEPORT_COST;
		ent->client->ability_delay = level.time + 1;
		return;
	}
	*/

	// keep trying to teleport until there is no room left
	while (dist > 0)
	{
		edict_t *teleport_target;

		//4.2 support player-monsters
		if (PM_PlayerHasMonster(ent))
			teleport_target = ent->owner;
		else
			teleport_target = ent;

		VectorMA(start, dist, forward, end);

		// if there is a nearby sky brush, we shouldn't teleport there
		if (!ValidTeleportSpot(teleport_target, end))
		{
			// try further back
			dist -= 8;
			continue;
		}

		tr = gi.trace(end, teleport_target->mins, teleport_target->maxs, end, teleport_target, MASK_SHOT);

		// is this a valid position?
		if (!(tr.contents & MASK_SHOT) && !(gi.pointcontents(end) & MASK_SHOT) && !tr.allsolid)
		{
			teleport_target->s.event = EV_PLAYER_TELEPORT;
			VectorCopy(end, teleport_target->s.origin);
			VectorCopy(end, teleport_target->s.old_origin);
			VectorClear(teleport_target->velocity);
			gi.linkentity(teleport_target);
			ent->client->pers.inventory[power_cube_index]-=TELEPORT_COST;

			// 3.14 prevent teleport+nova spam
			ent->client->ability_delay = level.time + (ctf->value?2.0:1.0);
			return;
		}
		else
		{
			// try further back
			dist-=8;
		}
	}
}