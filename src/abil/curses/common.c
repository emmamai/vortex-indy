#include "g_local.h"
#include "curses.h"

//************************************************************************************************
//			Generic Curse functions
//************************************************************************************************

qboolean G_CurseValidTarget (edict_t *self, edict_t *target, qboolean vis, qboolean isCurse)
{
	if (!G_EntIsAlive(target))
		return false;
	// don't target players with invulnerability
	if (target->client && (target->client->invincible_framenum > level.framenum))
		return false;
	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;
	// don't target players in chat-protect
	if (!ptr->value && target->client && (target->flags & FL_CHATPROTECT))
		return false;
	// don't target spawning world monsters
	if (target->activator && !target->activator->client && (target->svflags & SVF_MONSTER) 
		&& (target->deadflag != DEAD_DEAD) && (target->nextthink-level.time > 2*FRAMETIME))
		return false;
	// don't target cloaked players
	if (target->client && target->svflags & SVF_NOCLIENT)
		return false;
	if (vis && !visible(self, target))
		return false;
	if(que_typeexists(target->curses, CURSE_FROZEN))
		return false;
	if (isCurse && (target->flags & FL_GODMODE || OnSameTeam(self, target)))
		return false;
	if (target == self)
		return false;
	
	return true;
}

void curse_think(edict_t *self)
{
	//Find my curse slot
	que_t *slot = NULL;
	slot = que_findtype(self->enemy->curses, NULL, self->atype);

	// curse self-terminates if the enemy dies or the duration expires
	if (!slot || !que_valident(slot))
	{
		que_removeent(self->enemy->curses, self, true);
		return;
	}

	CurseEffects(self->enemy, 10, 242);

	//Stick with the target
	VectorCopy(self->enemy->s.origin, self->s.origin);
	gi.linkentity(self);

	//Next think time
	self->nextthink = level.time + FRAMETIME;

	LifeDrain(self);// 3.5 this must be called last, because it may free the curse ent
	Bleed(self);//4.2
}

//************************************************************************************************

qboolean curse_add(edict_t *target, edict_t *caster, int type, int curse_level, float duration)
{
	edict_t *curse;
	que_t	*slot = NULL;
	
	//Find out if this curse already exists
	slot = que_findtype(target->curses, NULL, type);
	if(slot != NULL)
	{
		//If the current curse in effect has a level greater than the caster's curse level
		//if (slot->ent->owner->myskills.abilities[type].current_level > caster->myskills.abilities[type].current_level)
		if (slot->ent->monsterinfo.level > curse_level)//4.4
			//Can't re-curse this player
			return false;
		else
		{
            //Refresh the curse with the new level/ent/duration
			return que_addent(target->curses, slot->ent, duration);
		}
	}

	/*
	//Talent: Evil curse (improves curse duration)
	talentLevel = getTalentLevel(caster, TALENT_EVIL_CURSE);
	if(talentLevel > 0)
	{
		//Curses only
		if(type != BLESS && type != HEALING)
			duration *= 1.0 + 0.25 * talentLevel;
	}
	*/

	//Create the curse entity
	curse=G_Spawn();
	curse->classname = "curse";
	curse->solid = SOLID_NOT;
	curse->svflags |= SVF_NOCLIENT;
	curse->monsterinfo.level = curse_level;//4.2
	curse->monsterinfo.selected_time = duration;//4.2
	VectorClear(curse->velocity);
	VectorClear(curse->mins);
	VectorClear(curse->maxs);

	//Set curse type, target, and caster
	curse->owner = caster;
	curse->enemy = target;
	curse->atype = type;

	//First think in 1/2 a second
	curse->nextthink = level.time + FRAMETIME;
	curse->think = curse_think;

	//Set origin to target's origin
	VectorCopy(target->s.origin, curse->s.origin);
	gi.linkentity(curse);

	//Try to add the curse to the que
	if (!que_addent(target->curses, curse, duration))
	{
		G_FreeEdict(curse);
		return false;
	}
	return true;
}

//************************************************************************************************

// find valid targets that are within range and field of vision
edict_t *curse_MultiAttack (edict_t *e, edict_t *caster, int type, int range, float duration, qboolean isCurse)
{
	//edict_t *e=NULL;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (caster->s.origin);
	gi.multicast (caster->s.origin, MULTICAST_PVS);

	caster->client->idle_frames = 0; // disable cp/cloak on caster
	caster->client->ability_delay = level.time + 0.2; // avoid spell spam

	while ((e = findreticle(e, caster, range, 20, true)) != NULL)
	{
		if (!G_CurseValidTarget(caster, e, false, isCurse))
			continue;
		// holywater gives immunity to curses for a short time.
		if (e->holywaterProtection > level.time && type != BLESS && type != HEALING)
			continue;
		// don't allow bless on flag carrier
		if ((type == BLESS) && e->client && HasFlag(e))
			continue;
		if (!curse_add(e, caster, type, 0, duration))
			continue;
		return e;
	}
	return NULL;
}

// find a single valid target that is in-range and nearest to the aiming reticle
edict_t *curse_Attack(edict_t *caster, int type, int radius, float duration, qboolean isCurse)
{
	edict_t *e=NULL;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (caster->s.origin);
	gi.multicast (caster->s.origin, MULTICAST_PVS);

	caster->client->idle_frames = 0; // disable cp/cloak on caster
	caster->client->ability_delay = level.time + 0.2; // avoid spell spam

	while ((e = findclosestreticle(e, caster, radius)) != NULL)
	{
		if (!G_CurseValidTarget(caster, e, true, isCurse))
			continue;
	//	if (entdist(caster, e) > radius)
	//		continue;
		//4.0 holywater gives immunity to curses for a short time.
		if (e->holywaterProtection > level.time && type != BLESS && type != HEALING)
			continue;
		// 3.7 don't allow bless on flag carrier
		if ((type == BLESS) && e->client && HasFlag(e))
			continue;
		if (!curse_add(e, caster, type, 0, duration))
			continue;
		return e;
	}
	return NULL;
}

qboolean CanCurseTarget (edict_t *caster, edict_t *target, int type, qboolean isCurse, qboolean vis)
{
	if (!G_EntIsAlive(target))
		return false;
	// don't target players with invulnerability
	if (target->client && (target->client->invincible_framenum > level.framenum))
		return false;
	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;
	// don't target players in chat-protect
	if (!ptr->value && target->client && (target->flags & FL_CHATPROTECT))
		return false;
	// don't target spawning world monsters
	if (target->activator && !target->activator->client && (target->svflags & SVF_MONSTER) 
		&& (target->deadflag != DEAD_DEAD) && (target->nextthink-level.time > 2*FRAMETIME))
		return false;
	// don't target cloaked players
	if (target->client && target->svflags & SVF_NOCLIENT)
		return false;
	if (vis && !visible(caster, target))
		return false;
	if(que_typeexists(target->curses, CURSE_FROZEN))
		return false;
	if (isCurse && (target->flags & FL_GODMODE || OnSameTeam(caster, target)))
		return false;
	if (target == caster)
		return false;
	// holywater gives immunity to curses for a short time.
	if (target->holywaterProtection > level.time && type != BLESS && type != HEALING)
		return false;
	// don't allow bless on flag carrier
	if ((type == BLESS) && target->client && HasFlag(target))
		return false;
	return true;
}

char *GetCurseName (int type)
{
	switch (type)
	{
	case HEALING: return "healing";
	case BLESS: return "bless";
	case DEFLECT: return "deflect";
	case CURSE: return "confuse";
	case LOWER_RESIST: return "lower resist";
	case AMP_DAMAGE: return "amp damage";
	case WEAKEN: return "weaken";
	case LIFE_DRAIN: return "life drain";
	default: return "";
	}
}

void CurseMessage (edict_t *caster, edict_t *target, int type, float duration, qboolean isCurse)
{
	char *curseName = GetCurseName(type);
	char *typeName;

	if (isCurse)
		typeName = "cursed";
	else
		typeName = "blessed";

	//Notify the target
	if ((target->client) && !(target->svflags & SVF_MONSTER))
	{
		safe_cprintf(target, PRINT_HIGH, "**You have been %s with %s for %0.1f second(s)**\n", typeName, curseName, duration);
		if (caster && caster->client)
			safe_cprintf(caster, PRINT_HIGH, "%s %s with %s for %0.1f second(s)\n", typeName, target->myskills.player_name, curseName, duration);
	}
	else if (target->mtype)
	{
		if (PM_MonsterHasPilot(target))
		{
			safe_cprintf(target->activator, PRINT_HIGH, "**You have been %s with %s for %0.1f second(s)**\n", typeName, curseName, duration);
			if (caster && caster->client)
				safe_cprintf(caster, PRINT_HIGH, "%s %s with %s for %0.1f second(s)\n", typeName, target->activator->client->pers.netname, curseName, duration);
			return;
		}

		if (caster && caster->client)
			safe_cprintf(caster, PRINT_HIGH, "%s %s with %s for %0.1f second(s)\n", typeName, V_GetMonsterName(target), curseName, duration);
	}
	else if (caster && caster->client)
		safe_cprintf(caster, PRINT_HIGH, "%s %s with %s for %0.1f second(s)\n", typeName, target->classname, curseName, duration);
}

void CurseRadiusAttack (edict_t *caster, int type, int range, int radius, float duration, qboolean isCurse)
{
	edict_t *e=NULL, *f=NULL;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (caster->s.origin);
	gi.multicast (caster->s.origin, MULTICAST_PVS);

	caster->client->idle_frames = 0;
	caster->client->ability_delay = level.time;// for monster hearing (check if ability was recently used/cast)

	// find a target closest to the caster's reticle
	while ((e = findclosestreticle(e, caster, range)) != NULL)
	{
		if (!CanCurseTarget(caster, e, type, isCurse, true))
			continue;
		if (entdist(caster, e) > range)
			continue;
		if (!infront(caster, e))
			continue;
		if (!curse_add(e, caster, type, 0, duration))
			continue;
		CurseMessage(caster, e, type, duration, isCurse);

		// target anything in-range of this entity
		while ((f = findradius(f, e->s.origin, radius)) != NULL)
		{
			if (!CanCurseTarget(caster, f, type, isCurse, false))
				continue;
			if (f == e)
				continue;
			if (!visible(e, f))
				continue;
			if (!curse_add(f, caster, type, 0, duration))
				continue;
			CurseMessage(caster, f, type, duration, isCurse);
		}

		break;
	}
}

#define CURSE_DIR_UP		1
#define CURSE_DIR_DOWN		2
#define CURSE_DIR_LEFT		3
#define CURSE_DIR_RIGHT		4
#define CURSE_DIR_LU		5
#define CURSE_DIR_LD		6
#define CURSE_DIR_RU		7
#define CURSE_DIR_RD		8
#define CURSE_MOVEMENT		GetRandom(1,4)
#define CURSE_MAX_ROLL		20

void CursedPlayer (edict_t *ent)
{
	int		i;
	vec3_t	forward;

	if (!G_EntIsAlive(ent))
		return;
	if (!ent->client)
		return;
	if (!que_typeexists(ent->curses, CURSE))
	{
		// reset roll angles
		ent->client->ps.pmove.delta_angles[ROLL] = 0;
		return;
	}

	if (level.time > ent->curse_delay)
	{
		ent->curse_dir = GetRandom(1, 8);
		ent->curse_delay = level.time + 2*random();
	}

	// copy current viewing angles
	VectorCopy(ent->client->v_angle, forward);

	// choose which direction to move angles
	switch (ent->curse_dir)
	{
	case 1: forward[PITCH]+=CURSE_MOVEMENT; break; // down
	case 2: forward[PITCH]-=CURSE_MOVEMENT; break; // up
	case 3: forward[YAW]+=CURSE_MOVEMENT; break; // left
	case 4: forward[YAW]-=CURSE_MOVEMENT; break; // right
	case 5: forward[YAW]+=CURSE_MOVEMENT; forward[PITCH]-=CURSE_MOVEMENT; break;
	case 6: forward[YAW]+=CURSE_MOVEMENT; forward[PITCH]+=CURSE_MOVEMENT; break;
	case 7: forward[YAW]-=CURSE_MOVEMENT; forward[PITCH]-=CURSE_MOVEMENT; break;
	case 8: forward[YAW]+=CURSE_MOVEMENT; forward[PITCH]+=CURSE_MOVEMENT; break;
	}

	// change roll angles
	if (ent->curse_dir <= 4)
		forward[ROLL] +=CURSE_MOVEMENT;
	else
		forward[ROLL] -=CURSE_MOVEMENT;
	// don't roll too much
	if ((forward[ROLL] > 0) && (forward[ROLL] > CURSE_MAX_ROLL))
		forward[ROLL] = CURSE_MAX_ROLL;
	else if ((forward[ROLL] < 0) && (forward[ROLL] < -CURSE_MAX_ROLL))
		forward[ROLL] = -CURSE_MAX_ROLL;

	// set view angles 
	for (i = 0 ; i < 3 ; i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(forward[i]-ent->client->resp.cmd_angles[i]);
	VectorCopy(forward, ent->client->ps.viewangles);
	VectorCopy(forward, ent->client->v_angle);
}

void CurseEffects (edict_t *self, int num, int color)
{
	vec3_t start, up, angle;

	if ((level.framenum % 5) != 0)
		return;
	if (!G_EntIsAlive(self))
		return;

	VectorCopy(self->s.angles, angle);
	angle[ROLL] = GetRandom(0, 20) - 10;
	angle[PITCH] = GetRandom(0, 20) - 10;
	AngleCheck(&angle[ROLL]);
	AngleCheck(&angle[PITCH]);

	AngleVectors(angle, NULL, NULL, up);

	// upside-down minisentry
	if (self->owner && (self->mtype == M_MINISENTRY) 
		&& (self->owner->style == SENTRY_FLIPPED))
		VectorMA(self->s.origin, self->mins[2]-16, up, start);
	else
		VectorMA(self->s.origin, self->maxs[2]+16, up, start);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LASER_SPARKS);
	gi.WriteByte(num); // number of sparks
	gi.WritePosition(start);
	gi.WriteDir(up);
	gi.WriteByte(color); // 242 = red, 210 = green, 2 = black
	gi.multicast(start, MULTICAST_PVS);
}

