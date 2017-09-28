#ifndef CURSES_H

#define BLESSING_RADIUS_BASE	512
#define BLESSING_RADIUS_BONUS	0

#define CURSE_DEFAULT_INITIAL_RADIUS	256
#define CURSE_DEFAULT_ADDON_RADIUS		0
#define CURSE_DEFAULT_INITIAL_RANGE		512
#define CURSE_DEFAULT_ADDON_RANGE		0

#define AMP_DAMAGE_MULT_BASE		1.5
#define AMP_DAMAGE_MULT_BONUS		0.1

#define BLESS_BONUS				1.5		//Damage / Defense
#define BLESS_MAGIC_BONUS		1.5

#define DEFLECT_INITIAL_HITSCAN_CHANCE		0.1
#define DEFLECT_ADDON_HITSCAN_CHANCE		0.023
#define DEFLECT_INITIAL_PROJECTILE_CHANCE	0.1
#define DEFLECT_ADDON_PROJECTILE_CHANCE		0.023
#define DEFLECT_MAX_PROJECTILE_CHANCE		0.33
#define DEFLECT_MAX_HITSCAN_CHANCE			0.33
#define DEFLECT_HITSCAN_ABSORB_BASE			1.0
#define DEFLECT_HITSCAN_ABSORB_ADDON		0
#define DEFLECT_HITSCAN_ABSORB_MAX			1.0

#define LOWER_RESIST_INITIAL_FACTOR			0.25
#define LOWER_RESIST_ADDON_FACTOR			0.025

#define WEAKEN_MULT_BASE		1.25
#define WEAKEN_MULT_BONUS		0.025
#define WEAKEN_SLOW_BASE		0
#define WEAKEN_SLOW_BONUS		0.1

qboolean G_CurseValidTarget (edict_t *self, edict_t *target, qboolean vis, qboolean isCurse);
void curse_think(edict_t *self);
qboolean curse_add(edict_t *target, edict_t *caster, int type, int curse_level, float duration);
edict_t *curse_MultiAttack (edict_t *e, edict_t *caster, int type, int range, float duration, qboolean isCurse);
edict_t *curse_Attack(edict_t *caster, int type, int radius, float duration, qboolean isCurse);
qboolean CanCurseTarget (edict_t *caster, edict_t *target, int type, qboolean isCurse, qboolean vis);
char *GetCurseName (int type);
void CurseMessage (edict_t *caster, edict_t *target, int type, float duration, qboolean isCurse);
void CurseRadiusAttack (edict_t *caster, int type, int range, int radius, float duration, qboolean isCurse);
void CursedPlayer (edict_t *ent);
void CurseEffects (edict_t *self, int num, int color);

void Cmd_AmpDamage(edict_t *ent);
void Cmd_Weaken(edict_t *ent);
void Cmd_Slave(edict_t *ent);
void Cmd_Amnesia(edict_t *ent);
void Cmd_Curse(edict_t *ent);
void Cmd_Bless(edict_t *ent);
void Cmd_Healing(edict_t *ent);
void MindAbsorb(edict_t *ent);
void Cmd_LifeDrain(edict_t *ent);
void LifeDrain (edict_t *ent);
void Bleed (edict_t *curse);
void Cmd_LowerResist (edict_t *ent);

#define CURSES_H
#endif