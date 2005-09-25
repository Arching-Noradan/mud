/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     * 
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef __FEATURES_HPP__
#define __FEATURES_HPP__
#include <bitset>
using std::bitset;

#define THAC0_FEAT			0   //DO NOT USED
#define BERSERK_FEAT			1   //������������ ������
#define PARRY_ARROW_FEAT		2   //������ ������
#define BLIND_FIGHT_FEAT		3   //������ ����
#define CLEAVE_FEAT			4   //���������� �����
#define APPROACHING_ATTACK_FEAT		5   //��������� �����
#define DODGE_FEAT			6   //�������
#define TWO_WEAPON_FIGHTING_FEAT	7   //��������� ���
#define LIGHT_WALK_FEAT			8   //������ �������
#define DEPOSIT_FINDING_FEAT		9   //�����������
#define SPELL_SUBSTITUTE_FEAT		10   //������� ����������
#define POWER_ATTACK_FEAT		11  //������ �����
#define WOODEN_SKIN_FEAT		12  //���������� ����
#define IRON_SKIN_FEAT			13  //�������� ����
#define CONNOISEUR_FEAT			14  //������
#define EXORCIST_FEAT			15  //���������� ������
#define HEALER_FEAT			16  //��������
#define LIGHTING_REFLEX_FEAT		17  //���������� �������
#define DRUNKARD_FEAT			18  //�������
#define DIEHARD_FEAT			19  //������ �������
#define ENDURANCE_FEAT			20  //������������
#define GREAT_FORTITUDE_FEAT		21  //���� ����
#define SKILL_FOCUS_FEAT		22  //������� ������
#define STEALTHY_FEAT			23  //������������
#define WEAPON_FOCUS_FEAT		24  //����������� ������
#define SPLENDID_HEALTH_FEAT		25  //����������� ��������
#define TRACKER_FEAT			26  //��������
#define WEAPON_FINESSE_FEAT		27  //����������
#define COMBAT_CASTING_FEAT		28  //������ �����������
#define PUNCH_MASTER_FEAT		29  //�������� ����
#define CLUBS_MASTER_FEAT		30  //������ ������
#define AXES_MASTER_FEAT		31  //������ ������
#define LONGS_MASTER_FEAT		32  //������ ����
#define SHORTS_MASTER_FEAT		33  //������ �������
#define NOSTANDART_MASTER_FEAT		34  //��������� ������
#define BOTHHANDS_MASTER_FEAT		35  //������ ����������
#define PICK_MASTER_FEAT		36  //������ ����
#define SPADES_MASTER_FEAT		37  //������ �����
#define BOWS_MASTER_FEAT		38  //������ ��������
#define FOREST_PATHS_FEAT		39  //������ �����
#define MOUNTAIN_PATHS_FEAT		40  //������ �����
#define LUCKY_FEAT			41  //�����������
#define SPIRIT_WARRIOR_FEAT		42  //������ ���
#define FIGHTING_TRICK_FEAT		43  //������ ������
#define SPELL_FOCUS_FEAT		44  //������� ����������
#define THIRD_RING_SPELL_FEAT		45  //
#define FOURTH_RING_SPELL_FEAT		46  //
#define FIFTH_RING_SPELL_FEAT		47  //
#define SIXTH_RING_SPELL_FEAT		48  //
#define SEVENTH_RING_SPELL_FEAT		49  //
#define LEGIBLE_WRITTING_FEAT		50  //������ ������
#define BREW_POTION_FEAT		51  //����� �����

/* MAX_FEATS ������������ � structs.h */

#define UNUSED_FTYPE	-1
#define NORMAL_FTYPE	0
#define AFFECT_FTYPE	1
#define SKILL_MOD_FTYPE	2

#define MAX_FEAT_AFFECT	5 /* ���������� ��� "��������-��������" */
#define MAX_ACC_FEAT	6 /* ����������� ��������� ���������� ��-���������� ������������. */
#define LEV_ACC_FEAT	5 /* ��� � ������� ������� ���������� ����� ����������� */

/* ���� ��������� ��� ������������ (����� AFFECT_FEAT_TYPE, ��� ��� ������������ ����������� ���� APPLY) */
#define FEAT_TIMER 1

extern struct feat_info_type feat_info[MAX_FEATS];

SPECIAL(feat_teacher);
int find_feat_num(char *name);
void assign_feats(void);
bool can_use_feat(CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
int number_of_features(CHAR_DATA *ch);

#endif

struct feat_info_type {
        int min_remort[NUM_CLASSES][NUM_KIN];
        int min_level[NUM_CLASSES][NUM_KIN];
	bool classknow[NUM_CLASSES][NUM_KIN];
	bool natural_classfeat[NUM_CLASSES][NUM_KIN];
	struct obj_affected_type
			affected[MAX_FEAT_AFFECT];
        int type;
        const char *name;
};
