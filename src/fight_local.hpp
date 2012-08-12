// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_LOCAL_HPP_
#define _FIGHT_LOCAL_HPP_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "AffectHandler.hpp"

#define RIGHT_WEAPON 1
#define LEFT_WEAPON  2

//Polud �������, ���������� ����������� ��������, ���� ��� ����
template <class S> void handle_affects( S& params ) //��� params ������������ ��� ������ �������
{
	AFFECT_DATA* aff;
	for (aff=params.ch->affected; aff; aff = aff->next)
	{
		if (aff->handler)
			aff->handler->Handle(params); //� ����������� �� ���� params ��������� ������ Handler
	}
}

struct HitType
{
	HitType() : weapon(0), wielded(0), weapon_pos(WEAR_WIELD), weap_skill(0),
		weap_skill_is(0), type(0), w_type(0), victim_ac(0), calc_thaco(0),
		dam(0), noparryhit(0), was_critic(0), dam_critic(0)
	{
		diceroll = number(100, 2099) / 100;
	};

	/** hit */
	void init(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_base_hr(CHAR_DATA *ch);
	void calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_ac(CHAR_DATA *victim);
	void add_weapon_damage(CHAR_DATA *ch);
	void add_hand_damage(CHAR_DATA *ch);
	void check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_crit_chance(CHAR_DATA *ch);
	void check_weap_feats(CHAR_DATA *ch);

	/** extdamage */
	int extdamage(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim);

	/** init() */
	// 1 - ����� ������ ��� ����� ������ (RIGHT_WEAPON),
	// 2 - ����� ����� ����� (LEFT_WEAPON)
	int weapon;
	// �����, ������� � ������ ������ ������������ ����
	OBJ_DATA *wielded;
	// ����� ������� (NUM_WEARS) �����
	int weapon_pos;
	// ����� �����, ������ �� ����� ��� ����� ���
	int weap_skill;
	// ���-�� ����� ����� skill � ����
	int weap_skill_is;
	// ��������� ������ �� ������ ������� ���������
	int diceroll;
	// ����� ����� �����, ��������� �� ������ hit()
	// ����������� �� TYPE_UNDEFINED � TYPE_NOPARRY, � extdamage �� ����
	int type;
	// type + TYPE_HIT ��� ������ ��������� �� ����� � ��� ���� ����� ����
	int w_type;

	/** ������������� �� ���� ��� */
	// �� ������ ��� ������� ���������
	int victim_ac;
	// ������� ���������� ��� ������� ���������
	int calc_thaco;
	// ����� ����������
	int dam;
	// ����� �� �������� �����, ������� ����� ��������� � dam
	int noparryhit;
	// was_critic = TRUE, dam_critic = 0 - ����������� ����
	// was_critic = TRUE, dam_critic > 0 - ���� ������ ������
	int was_critic;
	int dam_critic;
};

struct DmgType
{
	DmgType() : w_type(0), dam(0), was_critic(0), dam_critic(0),
		fs_damage(0), mayflee(true), dmg_type(PHYS_DMG) {};

	int damage(CHAR_DATA *ch, CHAR_DATA *victim);
	bool magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	bool armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim);
	void compute_critical(CHAR_DATA *ch, CHAR_DATA *victim);

	// type + TYPE_HIT ��� ������ ��������� �� ����� � ��� ���� ����� ����
	int w_type;
	// ����� ����������
	int dam;
	// was_critic = TRUE, dam_critic = 0 - ����������� ����
	// was_critic = TRUE, dam_critic > 0 - ���� ������ ������
	int was_critic;
	int dam_critic;
	// �������� ����� �� ��������� ����
	int fs_damage;
	// ����� �� �������
	bool mayflee;
	// ��� ����� (���/���/�������)
	int dmg_type;
};

/** fight.cpp */

int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void set_battle_pos(CHAR_DATA * ch);

/** fight_hit.cpp */

int calc_leadership(CHAR_DATA * ch);
void exthit(CHAR_DATA * ch, int type, int weapon);

/** fight_stuff.cpp */

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line);

#endif // _FIGHT_LOCAL_HPP_
