#include "g_local.h"

//3.0 matrix jump
void cmd_mjump(edict_t *ent)
{

	if (ent->holdtime > level.time)
		return; // can't use abilities

	if (HasFlag(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't use this while carrying the flag!\n");
		return;
	}

	if ((!(ent->v_flags & SFLG_MATRIXJUMP)) && (ent->velocity[2] == 0) && (!(ent->v_flags & SFLG_UNDERWATER)))
	{
		item_t *slot;
		int i;
		qboolean found = false;

		//Find item in inventory
		for (i = 3; i < MAX_VRXITEMS; ++i)
		{
			if (ent->myskills.items[i].itemtype & ITEM_GRAVBOOTS)
			{
				slot = &ent->myskills.items[i];
				found = true;
				break;
			}
		}

		//no boots? no jump
		if (!found) return;

		ent->v_flags ^= SFLG_MATRIXJUMP;
		ent->velocity[2] += MJUMP_VELOCITY;

		//Consume a charge
		if (!(ent->myskills.items[i].itemtype & ITEM_UNIQUE))
			ent->myskills.items[i].quantity -= 1;

		//if out of charges, erase the item
		if (ent->myskills.items[i].quantity == 0)
		{
			int count = 0;
			V_ItemClear(&ent->myskills.items[i]);
			//Alert the player
            safe_cprintf(ent, PRINT_HIGH, "Your anti-gravity boots have broken!\n");
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/itembreak.wav"), 1, ATTN_NORM, 0);

			//Check for more boots (backup items)
			for (i = 3; i < MAX_VRXITEMS; ++i)
				if (ent->myskills.items[i].itemtype & ITEM_GRAVBOOTS)
					++count;
			safe_cprintf(ent, PRINT_HIGH, "Boots left: %d.\n", count);

		}
		else gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/gravjump.wav"), 1, ATTN_NORM, 0);
		PlayerNoise(ent, ent->s.origin, PNOISE_SELF);

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}
}
//end matrix jump