#include "g_local.h"

void Cmd_Antigrav_f (edict_t *ent)
{
	if ((!ent->inuse) || (!ent->client))
		return;
	if(ent->myskills.abilities[ANTIGRAV].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[ANTIGRAV].current_level, 0))
		return;

	if (ent->antigrav == true)
	{
		safe_cprintf(ent, PRINT_HIGH, "Antigrav disabled.\n");
		ent->antigrav = false;
		return;
	}

	if(ent->myskills.abilities[ANTIGRAV].disable)
		return;

	if (HasFlag(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't use this ability while carrying the flag!\n");
		return;
	}

	//3.0 amnesia disables
	if (que_findtype(ent->curses, NULL, AMNESIA) != NULL)
		return;

	safe_cprintf(ent, PRINT_HIGH, "Antigrav enabled.\n");
	ent->antigrav= true;
}