void organ_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);
qboolean organ_checkowner (edict_t *self);
void organ_restoreMoveType (edict_t *self);
void organ_remove (edict_t *self, qboolean refund);
void organ_removeall (edict_t *ent, char *classname, qboolean refund);
qboolean organ_explode (edict_t *self);
void spiker_dead (edict_t *self);