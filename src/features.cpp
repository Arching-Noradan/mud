/*************************************************************************
*   File: features.cpp                                 Part of Bylins    *
*   Features code                                                        *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                	                                 *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "skills.h"
#include "spells.h"
#include "features.hpp"
#include "char.hpp"

extern const char *unused_spellname;

struct feat_info_type feat_info[MAX_FEATS];

void unused_feat(int feat);
void assign_feats(void);
int find_feat_num(char *name);
bool can_use_feat(CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
int find_feat_slot(CHAR_DATA *ch, int feat);
int feature_mod(int feat, int location);
void check_berserk(CHAR_DATA * ch);

ACMD(do_lightwalk);

/*
   ��������� ����� ��� �������� �������� �������� � ������ affected ��������� �����������
   ���� � ����-�� ���� �������, ����� ������ ������� ������������ ��� ���� �����, ����������
   ������ ������� � ���������, ������ ������� ���� � ���������� �����������
   ������ ����� �������� ������� ��������� � ���������� feat_info �� ����
*/
class aff_array
{
public:
	explicit aff_array() : _pos(0), i(MAX_FEAT_AFFECT) {}

	int pos(int pos = -1)
	{
		if (pos == -1)
		{
			return _pos;
		}
		else if (pos >= 0 && pos < MAX_FEAT_AFFECT)
		{
			_pos = pos;
			return _pos;
		}
		sprintf(buf, "SYSERR: invalid arg passed to features::aff_aray.pos!");
		mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
	}

	void insert(byte location, sbyte modifier)
	{
		affected[_pos].location = location;
		affected[_pos].modifier = modifier;
		_pos++;
		if (_pos >= MAX_FEAT_AFFECT)
			_pos = 0;
	}

	void clear()
	{
		_pos = 0;
		for (i = 0; i < MAX_FEAT_AFFECT; i++)
		{
			affected[i].location = APPLY_NONE;
			affected[i].modifier = 0;
		}
	}

	struct obj_affected_type
				affected[MAX_FEAT_AFFECT];
private:
	int _pos, i;
};

/* ����� ������ ����������� �� ����� */
int find_feat_num(char *name)
{
	int index, ok;
	char const *temp, *temp2;
	char first[256], first2[256];
	for (index = 1; index < MAX_FEATS; index++)
	{
		if (is_abbrev(name, feat_info[index].name))
			return (index);
		ok = TRUE;
		/* It won't be changed, but other uses of this function elsewhere may. */
		temp = any_one_arg(feat_info[index].name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return (index);
	}
	return (-1);
}

/* ������������� ����������� ��������� ���������� */
void feato(int feat, const char *name, int type, bool can_up_slot, aff_array app)
{
	int i, j;
	for (i = 0; i < NUM_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			feat_info[feat].min_remort[i][j] = 0;
			feat_info[feat].min_level[i][j] = 0;
		}
	feat_info[feat].name = name;
	feat_info[feat].type = type;
	feat_info[feat].up_slot = can_up_slot;
	for (i = 0; i < MAX_FEAT_AFFECT; i++)
	{
		feat_info[feat].affected[i].location = app.affected[i].location;
		feat_info[feat].affected[i].modifier = app.affected[i].modifier;
	}
}

/* ������������� ��� unused features */
void unused_feat(int feat)
{
	int i, j;

	for (i = 0; i < NUM_CLASSES; i++)
		for (j = 0; j < NUM_KIN; j++)
		{
			feat_info[feat].min_remort[i][j] = MAX_REMORT;
			feat_info[feat].min_level[i][j] = LVL_IMPL + 1;
			feat_info[feat].natural_classfeat[i][j] = FALSE;
			feat_info[feat].classknow[i][j] = FALSE;
		}

	feat_info[feat].name = unused_spellname;
	feat_info[feat].type = UNUSED_FTYPE;
	feat_info[feat].up_slot = FALSE;

	for (i = 0; i < MAX_FEAT_AFFECT; i++)
	{
		feat_info[feat].affected[i].location = APPLY_NONE;
		feat_info[feat].affected[i].modifier = 0;
	}
}

/* ������������� ������� �������� ������������ */
void assign_feats(void)
{
	int i;
	aff_array feat_app;
	for (i = 0; i < MAX_FEATS; i++)
	{
		unused_feat(i);
	}

//1
	feato(BERSERK_FEAT, "������������ ������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//2
	feato(PARRY_ARROW_FEAT, "������ ������", NORMAL_FTYPE, TRUE, feat_app);
//3
	feato(BLIND_FIGHT_FEAT, "������ ���", NORMAL_FTYPE, TRUE, feat_app);
//4
	feat_app.insert(APPLY_MR, 1);
	feat_app.insert(APPLY_AR, 1);
	feato(IMPREGNABLE_FEAT, "�������������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//5-*
	feato(APPROACHING_ATTACK_FEAT, "��������� �����", NORMAL_FTYPE, TRUE, feat_app);
//6-*
	feato(DODGE_FEAT, "�������", NORMAL_FTYPE, TRUE, feat_app);
//7-*
	feato(TWO_WEAPON_FIGHTING_FEAT, "��������� ���", NORMAL_FTYPE, TRUE, feat_app);
//8
	feato(LIGHT_WALK_FEAT, "������ �������", NORMAL_FTYPE, TRUE, feat_app);
//9-*
	feato(DEPOSIT_FINDING_FEAT, "�����������", NORMAL_FTYPE, TRUE, feat_app);
//10
	feato(SPELL_SUBSTITUTE_FEAT, "������� ����������", NORMAL_FTYPE, TRUE, feat_app);
//11
	feato(POWER_ATTACK_FEAT, "������ �����", NORMAL_FTYPE, TRUE, feat_app);
//12
	feat_app.insert(APPLY_RESIST_FIRE, 5);
	feat_app.insert(APPLY_RESIST_AIR, 5);
	feat_app.insert(APPLY_RESIST_WATER, 5);
	feat_app.insert(APPLY_RESIST_EARTH, 5);
	feato(WOODEN_SKIN_FEAT, "���������� ����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//13
	feat_app.insert(APPLY_RESIST_FIRE, 10);
	feat_app.insert(APPLY_RESIST_AIR, 10);
	feat_app.insert(APPLY_RESIST_WATER, 10);
	feat_app.insert(APPLY_RESIST_EARTH, 10);
	feat_app.insert(APPLY_ABSORBE, 5);
	feato(IRON_SKIN_FEAT, "�������� ����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//14
	feat_app.insert(FEAT_TIMER, 8);
	feato(CONNOISEUR_FEAT, "������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//15
	feato(EXORCIST_FEAT, "���������� ������", SKILL_MOD_FTYPE, TRUE, feat_app);
//16
	feato(HEALER_FEAT, "��������", NORMAL_FTYPE, TRUE, feat_app);
//17
	feat_app.insert(APPLY_SAVING_REFLEX, -4);
	feato(LIGHTING_REFLEX_FEAT, "���������� �������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//18
	feat_app.insert(FEAT_TIMER, 8);
	feato(DRUNKARD_FEAT, "�������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//19 -*
	feato(DIEHARD_FEAT, "�������� ������", NORMAL_FTYPE, TRUE, feat_app);
//20
	feat_app.insert(APPLY_MOVEREG, 40);
	feato(ENDURANCE_FEAT, "������������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//21
	feat_app.insert(APPLY_SAVING_WILL, -4);
	feat_app.insert(APPLY_SAVING_STABILITY, -4);
	feato(GREAT_FORTITUDE_FEAT, "���� ����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//22
	feat_app.insert(APPLY_HITREG, 35);
	feato(FAST_REGENERATION_FEAT, "������� ����������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//23
	feato(STEALTHY_FEAT, "������������", SKILL_MOD_FTYPE, TRUE, feat_app);
//24
	feat_app.insert(APPLY_CAST_SUCCESS, 25);
	feato(RELATED_TO_MAGIC_FEAT, "���������� �������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//25 -*
	feat_app.insert(APPLY_HITREG, 10);
	feat_app.insert(APPLY_SAVING_CRITICAL, -4);
	feato(SPLENDID_HEALTH_FEAT, "����������� ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//26
	feato(TRACKER_FEAT, "��������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//27
	feato(WEAPON_FINESSE_FEAT, "������ ����", NORMAL_FTYPE, TRUE, feat_app);
//28
	feato(COMBAT_CASTING_FEAT, "������ ����������", NORMAL_FTYPE, TRUE, feat_app);
//29
	feat_app.insert(SKILL_PUNCH, APPLY_NONE);
	feat_app.insert(PUNCH_FOCUS_FEAT, APPLY_NONE);
	feato(PUNCH_MASTER_FEAT, "������ ��������� ���", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//30
	feat_app.insert(SKILL_CLUBS, APPLY_NONE);
	feat_app.insert(CLUB_FOCUS_FEAT, APPLY_NONE);
	feato(CLUBS_MASTER_FEAT, "������ ������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//31
	feat_app.insert(SKILL_AXES, APPLY_NONE);
	feat_app.insert(AXES_FOCUS_FEAT, APPLY_NONE);
	feato(AXES_MASTER_FEAT, "������ ������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//32
	feat_app.insert(SKILL_LONGS, APPLY_NONE);
	feat_app.insert(LONGS_FOCUS_FEAT, APPLY_NONE);
	feato(LONGS_MASTER_FEAT, "������ ����", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//33
	feat_app.insert(SKILL_SHORTS, APPLY_NONE);
	feat_app.insert(SHORTS_FOCUS_FEAT, APPLY_NONE);
	feato(SHORTS_MASTER_FEAT, "������ ����", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//34
	feat_app.insert(SKILL_NONSTANDART, APPLY_NONE);
	feat_app.insert(NONSTANDART_FOCUS_FEAT, APPLY_NONE);
	feato(NONSTANDART_MASTER_FEAT, "������ ���������� ������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//35
	feat_app.insert(SKILL_BOTHHANDS, APPLY_NONE);
	feat_app.insert(BOTHHANDS_FOCUS_FEAT, APPLY_NONE);
	feato(BOTHHANDS_MASTER_FEAT, "������ ����������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//36
	feat_app.insert(SKILL_PICK, APPLY_NONE);
	feat_app.insert(PICK_FOCUS_FEAT, APPLY_NONE);
	feato(PICK_MASTER_FEAT, "������ �������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//37
	feat_app.insert(SKILL_SPADES, APPLY_NONE);
	feat_app.insert(SPADES_FOCUS_FEAT, APPLY_NONE);
	feato(SPADES_MASTER_FEAT, "������ �����", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//38
	feat_app.insert(SKILL_BOWS, APPLY_NONE);
	feat_app.insert(BOWS_FOCUS_FEAT, APPLY_NONE);
	feato(BOWS_MASTER_FEAT, "������-������", NORMAL_FTYPE, TRUE, feat_app);
	feat_app.clear();
//39
	feato(FOREST_PATHS_FEAT, "������ �����", NORMAL_FTYPE, TRUE, feat_app);
//40
	feato(MOUNTAIN_PATHS_FEAT, "������ �����", NORMAL_FTYPE, TRUE, feat_app);
//41
	feat_app.insert(APPLY_MORALE, 5);
	feato(LUCKY_FEAT, "�����������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//42
	feato(SPIRIT_WARRIOR_FEAT, "������ ���", NORMAL_FTYPE, TRUE, feat_app);
//43 -*
	feato(FIGHTING_TRICK_FEAT, "������ ������", NORMAL_FTYPE, TRUE, feat_app);
//44
	feat_app.insert(APPLY_MANAREG, 100);
	feato(EXCELLENT_MEMORY_FEAT, "������������ ������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//45
	feat_app.insert(APPLY_C3, 1);
	feat_app.insert(APPLY_CAST_SUCCESS, -5);
	feat_app.insert(APPLY_MANAREG, -5);
	feato(THIRD_RING_SPELL_FEAT, "������ ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//46
	feat_app.insert(APPLY_C4, 1);
	feat_app.insert(APPLY_MANAREG, -5);
	feato(FOURTH_RING_SPELL_FEAT, "��������� ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//47
	feat_app.insert(APPLY_C5, 1);
	feat_app.insert(APPLY_CAST_SUCCESS, -5);
	feat_app.insert(APPLY_MANAREG, -5);
	feato(FIFTH_RING_SPELL_FEAT, "����� ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//48
	feat_app.insert(APPLY_C6, 1);
	feat_app.insert(APPLY_MANAREG, -10);
	feato(SIXTH_RING_SPELL_FEAT, "������ ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//49
	feat_app.insert(APPLY_C7, 1);
	feat_app.insert(APPLY_MANAREG, -10);
	feato(SEVENTH_RING_SPELL_FEAT, "������� ��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//50 -*
	feato(LEGIBLE_WRITTING_FEAT, "������ ������", NORMAL_FTYPE, TRUE, feat_app);
//51
	feato(BREW_POTION_FEAT, "�������", NORMAL_FTYPE, TRUE, feat_app);
//52
	feato(JUGGLER_FEAT, "�������", NORMAL_FTYPE, TRUE, feat_app);
//53
	feato(NIMBLE_FINGERS_FEAT, "������", SKILL_MOD_FTYPE, TRUE, feat_app);
//54
	feato(GREAT_POWER_ATTACK_FEAT, "���������� ������ �����", NORMAL_FTYPE, TRUE, feat_app);
//55
	feat_app.insert(APPLY_RESIST_IMMUNITY, 15);
	feato(IMMUNITY_FEAT, "�������� � ���", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//56
	feat_app.insert(APPLY_AC, -40);
	feato(MOBILITY_FEAT, "�����������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//57
	feat_app.insert(APPLY_STR, 1);
	feato(NATURAL_STRENGTH_FEAT, "�����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//58
	feat_app.insert(APPLY_DEX, 1);
	feato(NATURAL_DEXTERY_FEAT, "����������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//59
	feat_app.insert(APPLY_INT, 1);
	feato(NATURAL_INTELLECT_FEAT, "��������� ��", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//60
	feat_app.insert(APPLY_WIS, 1);
	feato(NATURAL_WISDOM_FEAT, "������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//61
	feat_app.insert(APPLY_CON, 1);
	feato(NATURAL_CONSTITUTION_FEAT, "��������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//62
	feat_app.insert(APPLY_CHA, 1);
	feato(NATURAL_CHARISMA_FEAT, "��������� �������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//63
	feat_app.insert(APPLY_MANAREG, 25);
	feato(MNEMONIC_ENHANCER_FEAT, "�������� ������", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//64 -*
	feat_app.insert(SKILL_LEADERSHIP, 5);
	feato(MAGNETIC_PERSONALITY_FEAT, "������������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//65
	feat_app.insert(APPLY_DAMROLL, 2);
	feato(DAMROLL_BONUS_FEAT, "����� �� ����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//66
	feat_app.insert(APPLY_HITROLL, 1);
	feato(HITROLL_BONUS_FEAT, "������� ����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//67
	feat_app.insert(APPLY_CAST_SUCCESS, 5);
	feato(MAGICAL_INSTINCT_FEAT, "���������� �����", AFFECT_FTYPE, TRUE, feat_app);
	feat_app.clear();
//68
	feat_app.insert(SKILL_PUNCH, APPLY_NONE);
	feato(PUNCH_FOCUS_FEAT, "�������_������: ����� ����", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//69
	feat_app.insert(SKILL_CLUBS, APPLY_NONE);
	feato(CLUB_FOCUS_FEAT, "�������_������: ������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//70
	feat_app.insert(SKILL_AXES, APPLY_NONE);
	feato(AXES_FOCUS_FEAT, "�������_������: ������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//71
	feat_app.insert(SKILL_LONGS, APPLY_NONE);
	feato(LONGS_FOCUS_FEAT, "�������_������: ���", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//72
	feat_app.insert(SKILL_SHORTS, APPLY_NONE);
	feato(SHORTS_FOCUS_FEAT, "�������_������: ���", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//73
	feat_app.insert(SKILL_NONSTANDART, APPLY_NONE);
	feato(NONSTANDART_FOCUS_FEAT, "�������_������: ���������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//74
	feat_app.insert(SKILL_BOTHHANDS, APPLY_NONE);
	feato(BOTHHANDS_FOCUS_FEAT, "�������_������: ���������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//75
	feat_app.insert(SKILL_PICK, APPLY_NONE);
	feato(PICK_FOCUS_FEAT, "�������_������: ������", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//76
	feat_app.insert(SKILL_SPADES, APPLY_NONE);
	feato(SPADES_FOCUS_FEAT, "�������_������: �����", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//77
	feat_app.insert(SKILL_BOWS, APPLY_NONE);
	feato(BOWS_FOCUS_FEAT, "�������_������: ���", SKILL_MOD_FTYPE, TRUE, feat_app);
	feat_app.clear();
//78
	feato(AIMING_ATTACK_FEAT, "���������� �����", NORMAL_FTYPE, TRUE, feat_app);
//79
	feato(GREAT_AIMING_ATTACK_FEAT, "���������� ���������� �����", NORMAL_FTYPE, TRUE, feat_app);
//80
	feato(DOUBLESHOT_FEAT, "������� �������", NORMAL_FTYPE, TRUE, feat_app);
//81
	feato(PORTER_FEAT, "���������", NORMAL_FTYPE, TRUE, feat_app);
//82
	feato(RUNE_NEWBIE_FEAT, "����������� ���", NORMAL_FTYPE, TRUE, feat_app);
//83
	feato(RUNE_USER_FEAT, "������ ����", NORMAL_FTYPE, TRUE, feat_app);
//84
	feato(RUNE_MASTER_FEAT, "�������� ����", NORMAL_FTYPE, TRUE, feat_app);
//85
	feato(RUNE_ULTIMATE_FEAT, "���� �����", NORMAL_FTYPE, TRUE, feat_app);
//86
	feato(TO_FIT_ITEM_FEAT, "����������", NORMAL_FTYPE, TRUE, feat_app);
//87
	feato(TO_FIT_CLOTHCES_FEAT, "��������", NORMAL_FTYPE, TRUE, feat_app);
//88
	feato(STRENGTH_CONCETRATION_FEAT, "������������ ����", NORMAL_FTYPE, TRUE, feat_app);
//89
	feato(DARK_READING_FEAT, "������� ����", NORMAL_FTYPE, TRUE, feat_app);

	/*
	//
		feato(AIR_MAGIC_FOCUS_FEAT, "�������_�����: ������", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(FIRE_MAGIC_FOCUS_FEAT, "�������_�����: �����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(WATER_MAGIC_FOCUS_FEAT, "�������_�����: ����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(EARTH_MAGIC_FOCUS_FEAT, "�������_�����: �����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(LIGHT_MAGIC_FOCUS_FEAT, "�������_�����: ����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(DARK_MAGIC_FOCUS_FEAT, "�������_�����: ����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(MIND_MAGIC_FOCUS_FEAT, "�������_�����: �����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	//
		feato(LIFE_MAGIC_FOCUS_FEAT, "�������_�����: �����", SKILL_MOD_FTYPE, TRUE, feat_app);
		feat_app.clear();
	*/
}

/* ����� �� �������� ������������ �����������? �������� �� ������, ��������, ���������� ���������, �����������. */
bool can_use_feat(CHAR_DATA *ch, int feat)
{
	if (!HAVE_FEAT(ch, feat))
		return FALSE;
	if (GET_LEVEL(ch) < feat_info[feat].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
		return FALSE;
	if (GET_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
		return FALSE;

	switch (feat)
	{
	case WEAPON_FINESSE_FEAT:
		if (GET_REAL_DEX(ch) < STRENGTH_APPLY_INDEX(ch) || GET_REAL_DEX(ch) < 18)
			return FALSE;
		break;
	case PARRY_ARROW_FEAT:
		if (GET_REAL_DEX(ch) < 16)
			return FALSE;
		break;
	case POWER_ATTACK_FEAT:
		if (GET_REAL_STR(ch) < 20)
			return FALSE;
		break;
	case GREAT_POWER_ATTACK_FEAT:
		if (GET_REAL_STR(ch) < 22)
			return FALSE;
		break;
	case AIMING_ATTACK_FEAT:
		if (GET_REAL_DEX(ch) < 16)
			return FALSE;
		break;
	case GREAT_AIMING_ATTACK_FEAT:
		if (GET_REAL_DEX(ch) < 18)
			return FALSE;
		break;
	case DOUBLESHOT_FEAT:
		if (ch->get_skill(SKILL_BOWS) < 40)
			return FALSE;
		break;
	}

	return TRUE;
}

/* ����� �� �������� ������� ��� �����������? */
bool can_get_feat(CHAR_DATA *ch, int feat)
{
	int i, count = 0;

	if (feat <= 0 || feat >= MAX_FEATS)
	{
		sprintf(buf, "�������� ����� ����������� (%d) ������� � features::can_get_feat!", feat);
		mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		return FALSE;
	}
	/* ����������� �� ������, ������, �������. */
	if (!feat_info[feat].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
			|| GET_LEVEL(ch) < feat_info[feat].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
			|| GET_REMORT(ch) < feat_info[feat].min_remort[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
		return FALSE;

	/* ������� ��������� ������ */
	if (find_feat_slot(ch, feat) < 0)
		return FALSE;

	/* ����������� ���������� ��� �������� */
	switch (feat)
	{
	case PARRY_ARROW_FEAT:
		if (!ch->get_skill(SKILL_MULTYPARRY) && !ch->get_skill(SKILL_PARRY))
			return FALSE;
		break;
	case CONNOISEUR_FEAT:
		if (!ch->get_skill(SKILL_IDENTIFY))
			return FALSE;
		break;
	case EXORCIST_FEAT:
		if (!ch->get_skill(SKILL_TURN_UNDEAD))
			return FALSE;
		break;
	case HEALER_FEAT:
		if (!ch->get_skill(SKILL_AID))
			return FALSE;
		break;
	case STEALTHY_FEAT:
		if (!ch->get_skill(SKILL_HIDE) && !ch->get_skill(SKILL_SNEAK) && !ch->get_skill(SKILL_CAMOUFLAGE))
			return FALSE;
		break;
	case TRACKER_FEAT:
		if (!ch->get_skill(SKILL_TRACK) && !ch->get_skill(SKILL_SENSE))
			return FALSE;
		break;
	case PUNCH_MASTER_FEAT:
	case CLUBS_MASTER_FEAT:
	case AXES_MASTER_FEAT:
	case LONGS_MASTER_FEAT:
	case SHORTS_MASTER_FEAT:
	case NONSTANDART_MASTER_FEAT:
	case BOTHHANDS_MASTER_FEAT:
	case PICK_MASTER_FEAT:
	case SPADES_MASTER_FEAT:
	case BOWS_MASTER_FEAT:
		if (!HAVE_FEAT(ch, (ubyte) feat_info[feat].affected[1].location))
			return FALSE;
		for (i = PUNCH_MASTER_FEAT; i <= BOWS_MASTER_FEAT; i++)
			if (HAVE_FEAT(ch, i))
				count++;
		if (count >= 1)
			return FALSE;
		break;
	case SPIRIT_WARRIOR_FEAT:
		if (!HAVE_FEAT(ch, GREAT_FORTITUDE_FEAT))
			return FALSE;
		break;
	case NIMBLE_FINGERS_FEAT:
		if (!ch->get_skill(SKILL_STEAL) && !ch->get_skill(SKILL_PICK_LOCK))
			return FALSE;
		break;
	case GREAT_POWER_ATTACK_FEAT:
		if (!HAVE_FEAT(ch, POWER_ATTACK_FEAT))
			return FALSE;
		break;
	case PUNCH_FOCUS_FEAT:
	case CLUB_FOCUS_FEAT:
	case AXES_FOCUS_FEAT:
	case LONGS_FOCUS_FEAT:
	case SHORTS_FOCUS_FEAT:
	case NONSTANDART_FOCUS_FEAT:
	case BOTHHANDS_FOCUS_FEAT:
	case PICK_FOCUS_FEAT:
	case SPADES_FOCUS_FEAT:
	case BOWS_FOCUS_FEAT:
		if (!ch->get_skill((ubyte) feat_info[feat].affected[0].location))
			return FALSE;
		for (i = PUNCH_FOCUS_FEAT; i <= BOWS_FOCUS_FEAT; i++)
			if (HAVE_FEAT(ch, i))
				count++;
		if (count >= 2)
			return FALSE;
		break;
	case GREAT_AIMING_ATTACK_FEAT:
		if (!HAVE_FEAT(ch, AIMING_ATTACK_FEAT))
			return FALSE;
		break;
	case DOUBLESHOT_FEAT:
		if (!HAVE_FEAT(ch, BOWS_FOCUS_FEAT) || ch->get_skill(SKILL_BOWS) < 40)
			return FALSE;
		break;
	case RUNE_USER_FEAT:
		if (!HAVE_FEAT(ch, RUNE_NEWBIE_FEAT))
			return FALSE;
		break;
	case RUNE_MASTER_FEAT:
		if (!HAVE_FEAT(ch, RUNE_USER_FEAT))
			return FALSE;
		break;
	case RUNE_ULTIMATE_FEAT:
		if (!HAVE_FEAT(ch, RUNE_MASTER_FEAT))
			return FALSE;
		break;
	}

	return TRUE;
}

/*
	���� ��������� ���� ��� �����������.
	���������� ����� �� 0 �� MAX_ACC_FEAT-1 - ����� �����,
	0, ���� ����������� ���������� � -1 ���� ������ �� �������.
	�� ��������� ����������� ���������� ����������� �� ������ ������.
*/
int find_feat_slot(CHAR_DATA *ch, int feat)
{
	int i, slot, fslot;
	bitset<MAX_ACC_FEAT> sockets;

	if (feat_info[feat].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
		return (0);

	slot = FEAT_SLOT(ch, feat);
	for (i = 1; i < MAX_FEATS; i++)
	{
		if (!HAVE_FEAT(ch, i) || feat_info[i].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)])
			continue;

		fslot = FEAT_SLOT(ch, i);
		for (; fslot < MAX_ACC_FEAT; fslot++)
		{
			if (!sockets.test(fslot))
			{
				sockets.set(fslot);
				break;
			}
		}
	}

	if (abs(static_cast<int>(sockets.count())) >= NUM_LEV_FEAT(ch))
		return (-1);

	for (; slot < MAX_ACC_FEAT && slot < NUM_LEV_FEAT(ch);)
	{
		if (sockets.test(slot))
		{
			if (feat_info[feat].up_slot)
			{
				slot++;
				continue;
			}
			return (-1);
		}
		else
			return (slot);
	}

	return (-1);
}

/* ���������� �������� �������� ������������ �� ���� location ��������� affected */
int feature_mod(int feat, int location)
{
	int i;
	for (i = 0; i < MAX_FEAT_AFFECT; i++)
		if (feat_info[feat].affected[i].location == location)
			return (int) feat_info[feat].affected[i].modifier;
	return 0;
}


void check_berserk(CHAR_DATA * ch)
{
	AFFECT_DATA af;
	struct timed_type timed;
	int prob;

	if (affected_by_spell(ch, SPELL_BERSERK) &&
			(GET_HIT(ch) > GET_REAL_MAX_HIT(ch) / 2))
	{
		affect_from_char(ch, SPELL_BERSERK);
		send_to_char("������������ ����������� �������� ���.\r\n", ch);
	}
//!IS_NPC(ch) &&
	if (can_use_feat(ch, BERSERK_FEAT) && ch->get_fighting() &&
			!timed_by_feat(ch, BERSERK_FEAT) && !AFF_FLAGGED(ch, AFF_BERSERK) &&
			(GET_HIT(ch) < GET_REAL_MAX_HIT(ch) / 4))
	{

//		if (!IS_NPC(ch)) {
//Gorrah: ����� �� � ����� ������ ������ ��� ��, ��� ��� ������ ���� �� ������
		timed.skill = BERSERK_FEAT;
		timed.time = 4;
		timed_feat_to_char(ch, &timed);
//		}

		af.type = SPELL_BERSERK;
		af.duration = pc_duration(ch, 1, 60, 30, 0, 0);
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.battleflag = 0;

		prob = IS_NPC(ch) ? 400 : (500 - GET_LEVEL(ch) * 5 - GET_REMORT(ch) * 5);
		if (number(1, 1000) <=  prob)
		{
			af.bitvector = AFF_BERSERK;
			act("��� ������ ������������ ������!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n0 ����������� �����$g � ������$u �� ����������!", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			af.bitvector = 0;
			act("�� ������� ��������, ������� �������� ����������. ��� �����.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n0 ������� �������$g, ������� �������� ����������. �������...", FALSE, ch, 0, 0, TO_ROOM);
		}
		affect_join(ch, &af, TRUE, FALSE, TRUE, FALSE);
	}
}

/* ������ ������� */
ACMD(do_lightwalk)
{
	AFFECT_DATA af;
	struct timed_type timed;

	if (IS_NPC(ch) || !can_use_feat(ch, LIGHT_WALK_FEAT))
	{
		send_to_char("�� �� ������ �����.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		act("������������ ������ � ������ �������� ��� $N3...", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}

	if (affected_by_spell(ch, SPELL_LIGHT_WALK))
	{
		send_to_char("�� ��� ���������� ������ �����.\r\n", ch);
		return;
	}
	if (timed_by_feat(ch, LIGHT_WALK_FEAT))
	{
		send_to_char("�� ������� �������� ��� �����.\r\n", ch);
		return;
	}

	affect_from_char(ch, SPELL_LIGHT_WALK);

	timed.skill = LIGHT_WALK_FEAT;
	timed.time = 24;
	timed_feat_to_char(ch, &timed);

	send_to_char("������, �� ����������� ����, �� �������� ������ ������.\r\n", ch);
	af.type = SPELL_LIGHT_WALK;
	af.duration = pc_duration(ch, 2, GET_LEVEL(ch), 5, 2, 8);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (number(1, 1000) > number(1, GET_REAL_DEX(ch) * 20))
	{
		af.bitvector = 0;
		send_to_char("��� �� ������� ��������...\r\n", ch);
	}
	else
	{
		af.bitvector = AFF_LIGHT_WALK;
		send_to_char("���� ���� ����� ����� �������.\r\n", ch);
	}
	affect_to_char(ch, &af);
}

//�������� � �����������
ACMD(do_fit)
{
	OBJ_DATA *obj;
	CHAR_DATA *vict;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	/*��������� ���� ��� ��-�����*/
	if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	};

	//����� �� ����� ������������ ��� �����������?
	if ((subcmd == SCMD_DO_ADAPT) && !can_use_feat(ch, TO_FIT_ITEM_FEAT))
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	};
	if ((subcmd == SCMD_MAKE_OVER) && !can_use_feat(ch, TO_FIT_CLOTHCES_FEAT))
	{
		send_to_char("�� �� ������ �����.", ch);
		return;
	};

	//���� � ��� �������, ������� ����� ����������?
	argument = one_argument(argument, arg1);

	if (!*arg1)
	{
		send_to_char("��� �� ������ ����������?\r\n", ch);
		return;
	};

	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
	{
		sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg1);
		send_to_char(buf, ch);
		return;
	};

	//�� ���� ������������?
	argument = one_argument(argument, arg2);
	if (!(vict = get_char_vis(ch, arg2, FIND_CHAR_ROOM)))
	{
		send_to_char("��� ���� �� ������ ���������� ��� ����?\r\n ��� ������ �������� � ������!\r\n", ch);
		return;
	};

	//������� ��� ����� ���������
	if (GET_OBJ_OWNER(obj) != 0)
	{
		send_to_char("� ���� ���� ��� ���� ��������.\r\n", ch);
		return;

	};

	//������� ������ �� ����������, �������������� ��� �� ���� ���������
	//� �������� ��� ���� �������� ����� ��������, �� ����� ����� �������� ����
	//����� ���� ��� �� ������� �������� �� ��������
	if ((GET_OBJ_WEAR(obj) <= 1) || OBJ_FLAGGED(obj, ITEM_SETSTUFF))
	{
		send_to_char("���� ������� ���������� ����������.\r\n", ch);
		return;
	}

// �� ����������� �������� ��
//(GET_OBJ_MATER(obj) != MAT_CRYSTALL) ���������
// (GET_OBJ_MATER(obj) != MAT_FARFOR) ��������
//(GET_OBJ_MATER(obj) != MAT_ROCK) �����
// (GET_OBJ_MATER(obj) != MAT_PAPER) ������
//(GET_OBJ_MATER(obj) != MAT_DIAMOND) ������������ �����

	//�������� �� ��������?
	switch (subcmd)
	{
	case SCMD_DO_ADAPT:
		if ((GET_OBJ_MATER(obj) != MAT_NONE) &&
				(GET_OBJ_MATER(obj) != MAT_BULAT) &&
				(GET_OBJ_MATER(obj) != MAT_BRONZE) &&
				(GET_OBJ_MATER(obj) != MAT_IRON) &&
				(GET_OBJ_MATER(obj) != MAT_STEEL) &&
				(GET_OBJ_MATER(obj) != MAT_SWORDSSTEEL) &&
				(GET_OBJ_MATER(obj) != MAT_COLOR) &&
				(GET_OBJ_MATER(obj) != MAT_WOOD) &&
				(GET_OBJ_MATER(obj) != MAT_SUPERWOOD) &&
				(GET_OBJ_MATER(obj) != MAT_GLASS))
		{
			sprintf(buf, "� ��������� %s ������%s �� ������������� ���������.\r\n",
					GET_OBJ_PNAME(obj, 0), GET_OBJ_SUF_6(obj));
			send_to_char(buf, ch);
			return;
		}
		break;
	case SCMD_MAKE_OVER:
		if ((GET_OBJ_MATER(obj) != MAT_BONE) &&
				(GET_OBJ_MATER(obj) != MAT_MATERIA) &&
				(GET_OBJ_MATER(obj) != MAT_SKIN) &&
				(GET_OBJ_MATER(obj) != MAT_ORGANIC))
		{
			sprintf(buf, "� ��������� %s ������%s �� ������������� ���������.\r\n",
					GET_OBJ_PNAME(obj, 0), GET_OBJ_SUF_6(obj));
			send_to_char(buf, ch);
			return;
		}
		break;
	};
	GET_OBJ_OWNER(obj) = GET_UNIQUE(vict);
	sprintf(buf, "�� ����� ������� � ������, ����������� ������ �� ������ ���.\r\n");
	sprintf(buf + strlen(buf), "�� ������ ���� ������� � 10000 ��� �������.\r\n");
	sprintf(buf + strlen(buf), "� �����-������ ��������� %s ����� �� ����� %s.\r\n", GET_OBJ_PNAME(obj, 3), GET_PAD(vict, 1));

	send_to_char(buf, ch);

}

/*
��������� ���� ������� �������.

ACMD(do_blow)
{



}
*/
