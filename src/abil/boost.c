#include "g_local.h"

void Cmd_BoostPlayer(edict_t *ent)
{
	edict_t		*other = NULL;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	float		boost_delay = DELAY_BOOST;
	int			talentLevel;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_BoostPlayer()\n", ent->client->pers.netname);

	if (ent->myskills.abilities[BOOST_SPELL].disable)
		return;

	//4.07 can't boost while being hurt
	if (ent->lasthurt+DAMAGE_ESCAPE_DELAY > level.time)
		return;

	if (!V_CanUseAbilities (ent, BOOST_SPELL, COST_FOR_BOOST, true))
		return;

	if (HasFlag(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't use this ability while carrying the flag!\n");
		return;
	}

	if (ent->client->snipertime >= level.time)
	{
		safe_cprintf(ent, PRINT_HIGH, "You can't use boost while trying to snipe!\n");
		return;
	}

	//Talent: Mobility
	talentLevel = getTalentLevel(ent, TALENT_MOBILITY);
	switch(talentLevel)
	{
	case 1:		boost_delay -= 0.15;	break;
	case 2:		boost_delay -= 0.3;		break;
	case 3:		boost_delay -= 0.5;		break;
	default:	//Do nothing
		break;
	}

	ent->client->pers.inventory[power_cube_index] -= COST_FOR_BOOST;
	ent->client->ability_delay = level.time + boost_delay;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  ent->viewheight-8);

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	
	//4.2 player-monster support
	if (PM_PlayerHasMonster(ent))
	{
		other = ent->owner;
		other->monsterinfo.air_frames = 1;// used to turn off velocity restriction
	}
	else
		other = ent;

	other->velocity[0] += forward[0] * 1000;
	other->velocity[1] += forward[1] * 1000;
	other->velocity[2] += forward[2] * 1000;

	if (other->groundentity)
		other->velocity[2] += 300;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	//Talent: Leap Attack
	if (getTalentLevel(ent, TALENT_LEAP_ATTACK) > 0)
	{
		ent->client->boosted = true; 
		gi.sound (ent, CHAN_VOICE, gi.soundindex (va("spells/leapattack%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	}
	else
		gi.sound (ent, CHAN_BODY, gi.soundindex("weapons/rockfly.wav"), 1, ATTN_NORM, 0);
}

void LeapAttack (edict_t *ent)
{
	float	duration;
	vec3_t	point, forward;
	edict_t *e=NULL;

	if (!ent->client->boosted)
		return;

	duration = 0.2 * getTalentLevel(ent, TALENT_LEAP_ATTACK);

	while ((e = findradius (e, ent->s.origin, 128)) != NULL)
	{
		if (!G_ValidTarget(ent, e, true))
			continue;
		if (!infront(ent, e))
			continue;

		G_EntMidPoint(e, point);
		VectorSubtract(point, ent->s.origin, forward);
		T_Damage(e, e, ent, forward, point, vec3_origin, 0, 1000, 0, 0);

		gi.sound (e, CHAN_VOICE, gi.soundindex (va("misc/stun%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
		
		// stun them
		if (e->client)
		{
			e->holdtime = level.time + duration;
			e->client->ability_delay = level.time + duration;
		}
		else
			e->nextthink = level.time + duration;
	}

	ent->client->boosted = false;
}