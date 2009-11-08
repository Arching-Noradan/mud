// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include <list>
#include "char.hpp"
#include "utils.h"
#include "db.h"
#include "pk.h"
#include "im.h"
#include "handler.h"
#include "interpreter.h"
#include "boards.h"
#include "privilege.hpp"
#include "skills.h"
#include "constants.h"
#include "char_player.hpp"
#include "spells.h"
#include "comm.h"
#include "room.hpp"

std::string PlayerI::empty_const_str;

namespace
{

// ������ �����/����� ����� ����� ��� ������������ �������� ��������
typedef std::vector<CHAR_DATA *> PurgedListType;
PurgedListType purged_list;

/**
* �� ����������� - ������� �� ��� ������ character.
*/
void check_purged(const CHAR_DATA *ch, const char *fnc)
{
	if (ch->purged())
	{
		log("SYSERR: Using purged character (%s).", fnc);
	}
}

int normolize_skill(int percent)
{
	const static int KMinSkillPercent = 0;
	const static int KMaxSkillPercent = 200;

	if (percent < KMinSkillPercent)
		percent = KMinSkillPercent;
	else if (percent > KMaxSkillPercent)
		percent = KMaxSkillPercent;

	return percent;
}

// ������ ��� �������� ������� �� ����������� ��� ����� ����
// ��� ��������� � ������ ��� �������� � ��� �� ������ ���������
std::list<CHAR_DATA *> fighting_list;

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace CharacterSystem
{

/**
* �������� �������� ���������� ����� ����� �����.
*/
void release_purged_list()
{
	if (purged_list.empty())
	{
		return;
	}
	struct timeval start, stop, result;
	gettimeofday(&start, 0);

	unsigned size = purged_list.size();
	for (PurgedListType::iterator i = purged_list.begin(); i != purged_list.end(); ++i)
	{
		delete *i;
	}
	purged_list.clear();

	gettimeofday(&stop, 0);
	timediff(&result, &stop, &start);

	snprintf(buf, MAX_STRING_LENGTH, "Purged: %u (%ld sec. %ld us)",
			size, result.tv_sec, result.tv_usec);
	mudlog(buf, NRM, 35, SYSLOG, TRUE);
}

} // namespace CharacterSystem

////////////////////////////////////////////////////////////////////////////////

Character::Character()
{
	this->zero_init();
}

Character::~Character()
{
	this->purge(true);
}

/**
* ��������� ���� ����� Character (������ ������������),
* �������� � ��������� �������, ����� ������� �� purge().
*/
void Character::zero_init()
{
	protecting_ = 0;
	touching_ = 0;
	fighting_ = 0;
	in_fighting_list_ = 0;
	serial_num_ = 0;
	purged_ = 0;
	// char_data
	nr = NOBODY;
	in_room = 0;
	wait = 0;
	punctual_wait = 0;
	last_comm = 0;
	player_specials = 0;
	affected = 0;
	timed = 0;
	timed_feat = 0;
	carrying = 0;
	desc = 0;
	id = 0;
	proto_script = 0;
	script = 0;
	memory = 0;
	next_in_room = 0;
	next = 0;
	next_fighting = 0;
	followers = 0;
	master = 0;
	CasterLevel = 0;
	DamageLevel = 0;
	pk_list = 0;
	helpers = 0;
	track_dirs = 0;
	CheckAggressive = 0;
	ExtractTimer = 0;
	Initiative = 0;
	BattleCounter = 0;
	Poisoner = 0;
	ing_list = 0;
	dl_list = 0;

	memset(&extra_attack_, 0, sizeof(extra_attack_type));
	memset(&cast_attack_, 0, sizeof(cast_attack_type));
	memset(&player_data, 0, sizeof(char_player_data));
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (int i = 0; i < NUM_WEARS; i++)
	{
		equipment[i] = 0;
	}

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Temporary, 0, sizeof(FLAG_DATA));
	memset(&BattleAffects, 0, sizeof(FLAG_DATA));
	char_specials.position = POS_STANDING;
	mob_specials.default_pos = POS_STANDING;
}

/**
* ������������ ���������� � Character ������, �������� �� �����������,
* �.�. ���� ������������� ������� �������� �� delete.
* \param destructor - true ����� ��������� �� ����������� � ��������/���������
* � purged_list �� �����, �� ������� = false.
*/
void Character::purge(bool destructor)
{
	if (GET_NAME(this))
		log("[FREE CHAR] (%s)", GET_NAME(this));

	int i, j, id = -1;
	struct alias_data *a;
	struct helper_data_type *temp;

	if (!IS_NPC(this) && GET_NAME(this))
	{
		id = get_ptable_by_name(GET_NAME(this));
		if (id >= 0)
		{
			player_table[id].level = (GET_REMORT(this) ? 30 : GET_LEVEL(this));
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1))
	{	/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_NAME(this))
			free(GET_NAME(this));

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j))
				free(GET_PAD(this, j));

		if (this->player_data.title)
			free(this->player_data.title);

		if (this->player_data.short_descr)
			free(this->player_data.short_descr);

		if (this->player_data.long_descr)
			free(this->player_data.long_descr);

		if (this->player_data.description)
			free(this->player_data.description);

		if (IS_NPC(this) && this->mob_specials.Questor)
			free(this->mob_specials.Questor);

		pk_free_list(this);

		while (this->helpers)
			REMOVE_FROM_LIST(this->helpers, this->helpers, next_helper);
	}
	else if ((i = GET_MOB_RNUM(this)) >= 0)
	{	/* otherwise, free strings only if the string is not pointing at proto */
		if (this->player_data.name && this->player_data.name != mob_proto[i].player_data.name)
			free(this->player_data.name);

		for (j = 0; j < NUM_PADS; j++)
			if (GET_PAD(this, j)
					&& (this->player_data.PNames[j] != mob_proto[i].player_data.PNames[j]))
				free(this->player_data.PNames[j]);

		if (this->player_data.title && this->player_data.title != mob_proto[i].player_data.title)
			free(this->player_data.title);

		if (this->player_data.short_descr && this->player_data.short_descr != mob_proto[i].player_data.short_descr)
			free(this->player_data.short_descr);

		if (this->player_data.long_descr && this->player_data.long_descr != mob_proto[i].player_data.long_descr)
			free(this->player_data.long_descr);

		if (this->player_data.description && this->player_data.description != mob_proto[i].player_data.description)
			free(this->player_data.description);

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	supress_godsapply = TRUE;
	while (this->affected)
		affect_remove(this, this->affected);
	supress_godsapply = FALSE;

	while (this->timed)
		timed_from_char(this, this->timed);

	if (this->desc)
		this->desc->character = NULL;

	if (this->player_specials != NULL && this->player_specials != &dummy_mob)
	{
		while ((a = GET_ALIASES(this)) != NULL)
		{
			GET_ALIASES(this) = (GET_ALIASES(this))->next;
			free_alias(a);
		}
		if (this->player_specials->poofin)
			free(this->player_specials->poofin);
		if (this->player_specials->poofout)
			free(this->player_specials->poofout);
		/* ������� */
		while (GET_RSKILL(this) != NULL)
		{
			im_rskill *r;
			r = GET_RSKILL(this)->link;
			free(GET_RSKILL(this));
			GET_RSKILL(this) = r;
		}
		/* ������� */
		while (GET_PORTALS(this) != NULL)
		{
			struct char_portal_type *prt_next;
			prt_next = GET_PORTALS(this)->next;
			free(GET_PORTALS(this));
			GET_PORTALS(this) = prt_next;
		}
// Cleanup punish reasons
		if (MUTE_REASON(this))
			free(MUTE_REASON(this));
		if (DUMB_REASON(this))
			free(DUMB_REASON(this));
		if (HELL_REASON(this))
			free(HELL_REASON(this));
		if (FREEZE_REASON(this))
			free(FREEZE_REASON(this));
		if (NAME_REASON(this))
			free(NAME_REASON(this));
// End reasons cleanup

		if (KARMA(this))
			free(KARMA(this));

		free(GET_LOGS(this));
// shapirus: ��������� �� ����������� �������� memory leak,
// ��������� ��������������� ������� ������...
		if (EXCHANGE_FILTER(this))
			free(EXCHANGE_FILTER(this));
		EXCHANGE_FILTER(this) = NULL;	// �� ������ ������
// ...� ������ � ����� ���� *���� :)
		while (IGNORE_LIST(this))
		{
			struct ignore_data *ign_next;
			ign_next = IGNORE_LIST(this)->next;
			free(IGNORE_LIST(this));
			IGNORE_LIST(this) = ign_next;
		}
		IGNORE_LIST(this) = NULL;

		if (GET_CLAN_STATUS(this))
			free(GET_CLAN_STATUS(this));

		// ������ ���� �������
		while (LOGON_LIST(this))
		{
			struct logon_data *log_next;
			log_next = LOGON_LIST(this)->next;
			free(this->player_specials->logons->ip);
			delete LOGON_LIST(this);
			LOGON_LIST(this) = log_next;
		}
		LOGON_LIST(this) = NULL;

		if (GET_BOARD(this))
			delete GET_BOARD(this);
		GET_BOARD(this) = 0;

		free(this->player_specials);
		this->player_specials = NULL;	// ����� ������� ACCESS VIOLATION !!!
		if (IS_NPC(this))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(this), GET_MOB_VNUM(this));
	}

	if (!destructor)
	{
		// �������� ���
		this->zero_init();
		// ����������� ������������ �� ������������ ����
		purged_ = true;
		char_specials.position = POS_DEAD;
		// ���������� � ������ ��������� ������ ����������
		purged_list.push_back(this);
	}
}

/**
* ����� � ������ ���� ������ � ������� �� ������/���.
*/
int Character::get_skill(int skill_num)
{
	int skill = get_trained_skill(skill_num) + get_equipped_skill(skill_num);
	if (AFF_FLAGGED(this, AFF_SKILLS_REDUCE))
	{
		skill -= skill * GET_POISON(this) / 100;
	}
	return normolize_skill(skill);
}

/**
* ����� �� ������.
*/
int Character::get_equipped_skill(int skill_num)
{
	int skill = 0;
	for (int i = 0; i < NUM_WEARS; ++i)
		if (equipment[i])
			skill += equipment[i]->get_skill(skill_num);

	return skill;
}

/**
* ������ ������������� ����� ����.
*/
int Character::get_trained_skill(int skill_num)
{
	if (Privilege::check_skills(this))
	{
		CharSkillsType::iterator it = skills.find(skill_num);
		if (it != skills.end())
		{
			return normolize_skill(it->second);
		}
	}
	return 0;
}

/**
* ������� ����� �� �� �����, � ��� ��������� ��� ���������� ������ ��� ������.
*/
void Character::set_skill(int skill_num, int percent)
{
	if (skill_num < 0 || skill_num > MAX_SKILL_NUM)
	{
		log("SYSERROR: ���������� ����� ������ %d � set_skill.", skill_num);
		return;
	}

	CharSkillsType::iterator it = skills.find(skill_num);
	if (it != skills.end())
	{
		if (percent)
			it->second = percent;
		else
			skills.erase(it);
	}
	else if (percent)
		skills[skill_num] = percent;
}

/**
*
*/
void Character::clear_skills()
{
	skills.clear();
}

/**
*
*/
int Character::get_skills_count()
{
	return skills.size();
}

int Character::get_obj_slot(int slot_num)
{
	if (slot_num > 0 && slot_num < MAX_ADD_SLOTS)
	{
		return add_abils.obj_slot[slot_num];
	}
	return 0;
}

void Character::add_obj_slot(int slot_num, int count)
{
	if (slot_num > 0 && slot_num < MAX_ADD_SLOTS)
	{
		add_abils.obj_slot[slot_num] += count;
	}
}

void Character::set_touching(CHAR_DATA *vict)
{
	touching_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_touching() const
{
	return touching_;
}

void Character::set_protecting(CHAR_DATA *vict)
{
	protecting_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_protecting() const
{
	return protecting_;
}

void Character::set_fighting(CHAR_DATA *vict)
{
	fighting_ = vict;
	check_fighting_list();
}

CHAR_DATA * Character::get_fighting() const
{
	return fighting_;
}

void Character::set_extra_attack(int skill, CHAR_DATA *vict)
{
	extra_attack_.used_skill = skill;
	extra_attack_.victim = vict;
	check_fighting_list();
}

int Character::get_extra_skill() const
{
	return extra_attack_.used_skill;
}

CHAR_DATA * Character::get_extra_victim() const
{
	return extra_attack_.victim;
}

void Character::set_cast(int spellnum, int spell_subst, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom)
{
	cast_attack_.spellnum = spellnum;
	cast_attack_.spell_subst = spell_subst;
	cast_attack_.tch = tch;
	cast_attack_.tobj = tobj;
	cast_attack_.troom = troom;
	check_fighting_list();
}

int Character::get_cast_spell() const
{
	return cast_attack_.spellnum;
}

int Character::get_cast_subst() const
{
	return cast_attack_.spell_subst;
}

CHAR_DATA * Character::get_cast_char() const
{
	return cast_attack_.tch;
}

OBJ_DATA * Character::get_cast_obj() const
{
	return cast_attack_.tobj;
}

void Character::check_fighting_list()
{
	/*
	if (!in_fighting_list_)
	{
		in_fighting_list_ = true;
		fighting_list.push_back(this);
	}
	*/
}

void Character::clear_fighing_list()
{
	if (in_fighting_list_)
	{
		in_fighting_list_ = false;
		std::list<CHAR_DATA *>::iterator it =  std::find(fighting_list.begin(), fighting_list.end(), this);
		if (it != fighting_list.end())
		{
			fighting_list.erase(it);
		}
	}
}

/**
* ������ ����� ��� ����� �� �������� � ��� ������ �������������� �� ��������.
*/
void change_fighting(CHAR_DATA * ch, int need_stop)
{
	/*
	for (std::list<CHAR_DATA *>::const_iterator it = fighting_list.begin(); it != fighting_list.end(); ++it)
	{
		CHAR_DATA *k = *it;
		if (k->get_touching() == ch)
		{
			k->set_touching(0);
			CLR_AF_BATTLE(k, EAF_PROTECT); // �����������
		}
		if (k->get_protecting() == ch)
		{
			k->set_protecting(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_extra_victim() == ch)
		{
			k->set_extra_attack(0, 0);
		}
		if (k->get_cast_char() == ch)
		{
			k->set_cast(0, 0, 0, 0, 0);
		}
		if (k->get_fighting() == ch && IN_ROOM(k) != NOWHERE)
		{
			log("[Change fighting] Change victim");
			CHAR_DATA *j;
			for (j = world[IN_ROOM(ch)]->people; j; j = j->next_in_room)
				if (j->get_fighting() == k)
				{
					act("�� ����������� �������� �� $N3.", FALSE, k, 0, j, TO_CHAR);
					act("$n ����������$u �� ��� !", FALSE, k, 0, j, TO_VICT);
					k->set_fighting(j);
					break;
				}
			if (!j && need_stop)
				stop_fighting(k, FALSE);
		}
	}
	*/

	CHAR_DATA *k, *j, *temp;
	for (k = character_list; k; k = temp)
	{
		temp = k->next;
		if (k->get_protecting() == ch)
		{
			k->set_protecting(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_touching() == ch)
		{
			k->set_touching(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}
		if (k->get_extra_victim() == ch)
		{
			k->set_extra_attack(0, 0);
		}
		if (k->get_cast_char() == ch)
		{
			k->set_cast(0, 0, 0, 0, 0);
		}
		if (k->get_fighting() == ch && IN_ROOM(k) != NOWHERE)
		{
			log("[Change fighting] Change victim");
			for (j = world[IN_ROOM(ch)]->people; j; j = j->next_in_room)
			{
				if (j->get_fighting() == k)
				{
					act("�� ����������� �������� �� $N3.", FALSE, k, 0, j, TO_CHAR);
					act("$n ����������$u �� ��� !", FALSE, k, 0, j, TO_VICT);
					k->set_fighting(j);
					break;
				}
			}
			if (!j && need_stop)
			{
				stop_fighting(k, FALSE);
			}
		}
	}
}

int fighting_list_size()
{
	return fighting_list.size();
}

int Character::get_serial_num()
{
	return serial_num_;
}

void Character::set_serial_num(int num)
{
	serial_num_ = num;
}

bool Character::purged() const
{
	return purged_;
}
