#include "g_local.h"

void OpenSpecialUpgradeMenu (edict_t *ent, int lastline);
void OpenMultiUpgradeMenu (edict_t *ent, int lastline, int page, int generaltype); // 3.17

//************************************************************************************************
//************************************************************************************************

int GetAbilityUpgradeCost(int index)
{
	switch(index)
	{
		//Abilities that cost 2 points		
		//case FREEZE_SPELL:
		case SCANNER:
		case DOUBLE_JUMP:
		case JETPACK:
		case ANTIGRAV:
		case FLASH:
		case ID:				return 2;			
		//Abilities that cost 3 points
		case CLOAK:
		case WORLD_RESIST:
		case BULLET_RESIST:
		case SHELL_RESIST:
		case ENERGY_RESIST:
		case PIERCING_RESIST:
		case BLACKHOLE:
		case TELEPORT:
		case BOOST_SPELL:
		case SPLASH_RESIST:		return 3;
		//Abilities that cost 4 points		
		case CREATE_QUAD:
		case CREATE_INVIN:
		case SUPER_SPEED:
		case WEAPON_KNOCK:		
		case MORPH_MASTERY:	
		case TOTEM_MASTERY:				
		case SHIELD:			return 4;
		default:				return 1;
	}
}


//************************************************************************************************

void upgradeSpecialMenu_handler(edict_t *ent, int option)
{
	int cost = GetAbilityUpgradeCost(option-1);

	//are we navigating the menu?
	switch (option)
	{
	case 502:	OpenUpgradeMenu(ent);	return;
	case 500:	closemenu(ent);			return;
	}

	//We are upgrading
	if (ent->myskills.speciality_points < cost)
	{
		//You need 1 points? let's fix that:
		if (cost > 1)
			safe_cprintf(ent, PRINT_HIGH, va("You need %d points to upgrade this ability.\n", cost));
		else safe_cprintf(ent, PRINT_HIGH, va("You need %d point to upgrade this ability.\n", cost));
		return;
	}
	if (ent->myskills.abilities[option-1].level < ent->myskills.abilities[option-1].max_level || ent->myskills.administrator > 20)
	{
		ent->myskills.speciality_points -= cost;
		ent->myskills.abilities[option-1].level++;
		ent->myskills.abilities[option-1].current_level++;
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, va("You have already reached the maximum level in this skill. (%d)\n", 
			ent->myskills.abilities[option-1].max_level));
		return;
	}
	// refresh the menu
	OpenSpecialUpgradeMenu(ent, ent->client->menustorage.currentline);
}

//************************************************************************************************

void OpenSpecialUpgradeMenu(edict_t *ent, int lastline)
{
	int i;
	int total_lines = 7;
	if (!ShowMenu(ent))
        return;
	clearmenu(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);
	for (i = 0; i < MAX_ABILITIES; i++)
	{
		upgrade_t *upgrade;
		int num = i + 1;
		char buffer[30];

		upgrade = &ent->myskills.abilities[i];

		if((upgrade->disable) || (upgrade->general_skill) || (upgrade->hidden))
			continue;

		//Create ability menu string
		strcpy(buffer, GetAbilityString(i));
		strcat(buffer, ":");
		padRight(buffer, 15);
		addlinetomenu(ent, va("%d. %s %d[%d]", total_lines-6, buffer, upgrade->level, upgrade->current_level), num);
		total_lines++;
	}
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, "Previous Menu", 502);
	addlinetomenu(ent, "Exit", 500);
	setmenuhandler(ent, upgradeSpecialMenu_handler);
	if (!lastline)
		ent->client->menustorage.currentline = total_lines-1;
	else
		ent->client->menustorage.currentline = lastline;
	showmenu(ent);

	ent->client->menustorage.menu_index = MENU_SPECIAL_UPGRADES;
}

//************************************************************************************************
//************************************************************************************************

void upgrademenu_handler (edict_t *ent, int option)
{
	if (option == 1)
	{
		OpenSpecialUpgradeMenu(ent, 0);
	}
	else if (option == 2)
		OpenMultiUpgradeMenu(ent, 0, 0, 1);//OpenGeneralUpgradeMenu(ent, 0);
	else if (option == 3)
		OpenMultiUpgradeMenu(ent, 0, 0, 2);//OpenGeneralUpgradeMenu(ent, 0);
	else closemenu(ent);
}

//************************************************************************************************

void OpenUpgradeMenu (edict_t *ent)
{
   if (!ShowMenu(ent))
        return;
	clearmenu(ent);
	//					xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, "Select an ability you want", 0);
	addlinetomenu(ent, "to upgrade. Remember to", 0);
	addlinetomenu(ent, "choose class-specific", 0);
	addlinetomenu(ent, "abilities, so you don't", 0);
	addlinetomenu(ent, "receive a penalty! Your", 0);
	addlinetomenu(ent, va("class is %s and you", GetClassString(ent->myskills.class_num)), 0);
	addlinetomenu(ent, va("have %d ability points.", ent->myskills.speciality_points), 0);
	addlinetomenu(ent, " ", 0);

	if (ent->myskills.class_num != CLASS_WEAPONMASTER) // WMs don't get class specific skills.
		addlinetomenu(ent, "Class specific skills", 1);

	addlinetomenu(ent, "General skills", 2);
	// Commented out. -az vrxchile 3.2
	//addlinetomenu(ent, "Mobility skills", 3); // az, vrxchile 2.7
	addlinetomenu(ent, " ", 0);

	addlinetomenu(ent, "Exit", 4);
	setmenuhandler(ent, upgrademenu_handler);
	ent->client->menustorage.currentline = 10;
	showmenu(ent);

	// try to shortcut to chat-protect mode
	if (ent->client->idle_frames < CHAT_PROTECT_FRAMES-51)
		ent->client->idle_frames = CHAT_PROTECT_FRAMES-51;
}

//************************************************************************************************
//************************************************************************************************

#define PAGE_NEXT		100
#define PAGE_PREVIOUS	102

int getMultiPageIndex (edict_t *ent, int page, int mode)
{
	int			i, pagenum=0, abilities=0;
	upgrade_t	*upgrade;
	qboolean	done=false;

	for (i=0; i<MAX_ABILITIES; i++)
	{
		upgrade = &ent->myskills.abilities[i];
		if (!upgrade->disable && upgrade->general_skill == mode && !upgrade->hidden)
		{
			abilities++;

			// only 10 abilities per page
			if (abilities > 9)
			{
				pagenum++;
				abilities = 0;
			}
			else if (i == MAX_ABILITIES-1)
			{
				// partial page
				pagenum++;
				break;
			}

			// we've found the page we want
			if (pagenum == page)
			{
				if (!page || done)
					break;
				done = true;
			}
		}
	}
	return i;
}

void UpgradeAbility(edict_t *ent, int ability_index)
{
	int cost;
	cost = GetAbilityUpgradeCost(ability_index);

	//We are upgrading
	if (ent->myskills.speciality_points < cost)
	{
		//You need 1 points? let's fix that:
		if (cost > 1)
			safe_cprintf(ent, PRINT_HIGH, va("You need %d points to upgrade this ability.\n", cost));
		else safe_cprintf(ent, PRINT_HIGH, va("You need one point to upgrade this ability.\n"));
		return;
	}
	if (ent->myskills.abilities[ability_index].level < ent->myskills.abilities[ability_index].max_level || ent->myskills.administrator > 999)
	{
		ent->myskills.speciality_points -= cost;
		ent->myskills.abilities[ability_index].level++;
		ent->myskills.abilities[ability_index].current_level++;
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, va("You have already reached the maximum level in this skill. (%d)\n", 
			ent->myskills.abilities[ability_index].max_level));
		// doon't close the menu. -az
		//return;
	}
}

void upgradeMultiMenu_handler (edict_t *ent, int option)
{
	int p, ability_index;

	if (option == 999)
	{
		closemenu(ent);
		return;
	}

	//gi.dprintf("option=%d\n", option);

	// next menu
	if (option < 300)
	{
		OpenMultiUpgradeMenu(ent, PAGE_NEXT, option-199, 1);
		return;
	}
	// previous menu
	else if (option < 400)
	{
		p = option-301;
		if (p < 0)
			OpenUpgradeMenu(ent);
		else	
			OpenMultiUpgradeMenu(ent, PAGE_PREVIOUS, p, 1);
		return;
	}

	p = option/1000-1;
	//gi.dprintf("page=%d\n", p);

	ability_index = option%1000;
	//gi.dprintf("ability = %s (%d)\n", GetAbilityString(ability_index), ability_index);
	
	UpgradeAbility(ent, ability_index);

	OpenMultiUpgradeMenu(ent, ent->client->menustorage.currentline, p, 1);
}

void upgradeMultiMenu_handler_mobility (edict_t *ent, int option)
{
	int p, ability_index;

	if (option == 999)
	{
		closemenu(ent);
		return;
	}

	//gi.dprintf("option=%d\n", option);

	// next menu
	if (option < 300)
	{
		OpenMultiUpgradeMenu(ent, PAGE_NEXT, option-199, 2);
		return;
	}
	// previous menu
	else if (option < 400)
	{
		p = option-301;
		if (p < 0)
			OpenUpgradeMenu(ent);
		else	
			OpenMultiUpgradeMenu(ent, PAGE_PREVIOUS, p, 2);
		return;
	}

	p = option/1000-1;
	ability_index = option%1000;

	UpgradeAbility(ent, ability_index);

	OpenMultiUpgradeMenu(ent, ent->client->menustorage.currentline, p, 2);
}

void OpenMultiUpgradeMenu (edict_t *ent, int lastline, int page, int generaltype)
{
	int			i, index, abilities=0,total_lines=7;
	char		buffer[30];
	upgrade_t	*upgrade;
	qboolean	next_option=false;

	if (!ShowMenu(ent))
       return;
	clearmenu(ent);

	// menu header
	addlinetomenu(ent, "Player Upgrades Menu", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);

	index = getMultiPageIndex(ent, page, generaltype);
	//gi.dprintf("index= %d\n", index);

	for (i=index; i<MAX_ABILITIES; i++)
	{
		upgrade = &ent->myskills.abilities[i];
		if((upgrade->disable) || (upgrade->general_skill != generaltype) || (upgrade->hidden))
			continue;

		abilities++;
		total_lines++;

		// create ability menu string
		strcpy(buffer, GetAbilityString(i));
		strcat(buffer, ":");
		padRight(buffer, 15);

		addlinetomenu(ent, va("%d. %s %d[%d]", abilities+page*10, buffer, upgrade->level, upgrade->current_level), ((page+1)*1000)+i);
	

		// only display 10 abilities at a time
		if (abilities > 9)
			break;
	}

	//getMultiPageNum(ent, i);

	// menu footer
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, va("You have %d ability points.", ent->myskills.speciality_points), 0);
	addlinetomenu(ent, " ", 0);

	if (i < MAX_ABILITIES || generaltype == 1)
	{
		addlinetomenu(ent, "Next", 200+page);	
		total_lines++;
		next_option = true;
	}
	
	if (generaltype == 1)
		addlinetomenu(ent, "Previous", 300+page);

	addlinetomenu(ent, "Exit", 999);
	
	if (generaltype == 1)
		setmenuhandler(ent, upgradeMultiMenu_handler);
	else
		setmenuhandler(ent, upgradeMultiMenu_handler_mobility);

	if (!lastline)
	{
		ent->client->menustorage.currentline = total_lines-1;
	}
	else
	{
		if (lastline == PAGE_PREVIOUS)
			lastline = total_lines-1;
		else if (lastline == PAGE_NEXT)
		{
			if (next_option)
				lastline = total_lines-2;
			else
				lastline = total_lines-1;
		}
		ent->client->menustorage.currentline = lastline;
	}

	showmenu(ent);

	ent->client->menustorage.menu_index = MENU_MULTI_UPGRADE;
}

