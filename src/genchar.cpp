/* ************************************************************************
*   File: genchar.cpp                                   Part of Bylins    *
*  Usage: functions for character generation                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "genchar.h"
#include "spells.h"

char *genchar_help =
    "\r\n ������ �� ������ ������� ���� ��������������. � ����������� �� ����, ���\r\n"
    "�� �� ������������, �� ������ ����� �������� ����� ������ ���������.\r\n"
    "� ������ '����� ��������' �� ������ ����� �������, ������� ����� ������������\r\n"
    "�� ��������������� ������ ���������. �������� ��������������, �� ���������\r\n"
    "���� ���� ���, ���������� - ��������.\r\n"
    "����� �� ���������, ����� � ��� � ���� ������ ���������� ����� 0, �� �������\r\n"
    "��������� ��������� � ����� � ����.\r\n"
    "��������! ��������� ����� ������� �������� ����� ����� ����� �������������\r\n"
    "�� 2 ������, ��� ��������� '�������'.\r\n"
    "\r\n" "���� �� ������� ��� �� ������� �������� ��������������� �������, ��\r\n" "������� '�' � ����� 'ENTER'\r\n";

#define G_STR 0
#define G_DEX 1
#define G_INT 2
#define G_WIS 3
#define G_CON 4
#define G_CHA 5

int max_stats[][6] =
/*  Str Dex Int Wis Con Cha */
{ {14, 13, 24, 25, 15, 10},	/* ������ */
{14, 12, 25, 23, 13, 16},	/* ������ */
{19, 25, 12, 12, 17, 16},	/* ��� */
{25, 11, 15, 15, 25, 10},	/* �������� */
{22, 24, 14, 14, 20, 12},	/* ������� */
{23, 17, 14, 14, 22, 12},	/* ��������� */
{14, 12, 25, 23, 13, 16},	/* �������� */
{14, 12, 25, 23, 13, 16},	/* ��������� */
{15, 13, 25, 23, 14, 13},	/* ������������ */
{22, 13, 16, 19, 18, 17},	/* ������ */
{25, 21, 16, 16, 20, 16},	/* ������� */
{25, 17, 13, 15, 17, 16},	/* ������ */
{21, 17, 13, 13, 21, 16},	/* ����� */
{18, 12, 24, 18, 17, 12}	/* ����� */
};

int min_stats[][6] =
/*  Str Dex Int Wis Con Cha */
{ {11, 10, 19, 20, 12, 10},	/* ������ */
{10, 9, 20, 18, 10, 13},	/* ������ */
{16, 22, 9, 9, 14, 13},		/* ��� */
{21, 11, 11, 12, 22, 10},	/* �������� */
{17, 19, 11, 11, 15, 12},	/* ������� */
{20, 14, 10, 10, 17, 12},	/* ��������� */
{10, 9, 20, 18, 10, 13},	/* �������� */
{10, 9, 20, 18, 10, 13},	/* ��������� */
{10, 9, 20, 20, 11, 10},	/* ������������ */
{19, 10, 12, 15, 14, 13},	/* ������ */
{19, 15, 11, 11, 17, 11},	/* ������� */
{20, 14, 11, 12, 14, 12},	/* ������ */
{18, 14, 10, 10, 18, 13},	/* ����� */
{15, 10, 19, 15, 13, 12}	/* ����� */
};

#define MIN_STR(ch) min_stats[(int) GET_CLASS(ch)][G_STR]
#define MIN_DEX(ch) min_stats[(int) GET_CLASS(ch)][G_DEX]
#define MIN_INT(ch) min_stats[(int) GET_CLASS(ch)][G_INT]
#define MIN_WIS(ch) min_stats[(int) GET_CLASS(ch)][G_WIS]
#define MIN_CON(ch) min_stats[(int) GET_CLASS(ch)][G_CON]
#define MIN_CHA(ch) min_stats[(int) GET_CLASS(ch)][G_CHA]

#define MAX_STR(ch) max_stats[(int) GET_CLASS(ch)][G_STR]
#define MAX_DEX(ch) max_stats[(int) GET_CLASS(ch)][G_DEX]
#define MAX_INT(ch) max_stats[(int) GET_CLASS(ch)][G_INT]
#define MAX_WIS(ch) max_stats[(int) GET_CLASS(ch)][G_WIS]
#define MAX_CON(ch) max_stats[(int) GET_CLASS(ch)][G_CON]
#define MAX_CHA(ch) max_stats[(int) GET_CLASS(ch)][G_CHA]

void genchar_disp_menu(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];

	sprintf(buf,
		"\r\n              -      +\r\n"
		"  ����     : (�) %2d (�)        [%2d - %2d]\r\n"
		"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
		"  ��       : (�) %2d (�)        [%2d - %2d]\r\n"
		"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
		"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
		"  �������  : (�) %2d (�)        [%2d - %2d]\r\n"
		"\r\n"
		"  ����� ��������: %3d \r\n"
		"\r\n"
		"  �) ������\r\n"
		"\r\n"
		"  �) ������������� �������������.\r\n",
		GET_STR(ch), MIN_STR(ch), MAX_STR(ch),
		GET_DEX(ch), MIN_DEX(ch), MAX_DEX(ch),
		GET_INT(ch), MIN_INT(ch), MAX_INT(ch),
		GET_WIS(ch), MIN_WIS(ch), MAX_WIS(ch),
		GET_CON(ch), MIN_CON(ch), MAX_CON(ch),
		GET_CHA(ch), MIN_CHA(ch), MAX_CHA(ch), SUM_ALL_STATS - SUM_STATS(ch));
	send_to_char(buf, ch);
	if (SUM_ALL_STATS == SUM_STATS(ch))
		send_to_char("  �) ��������� ���������\r\n", ch);
	send_to_char(" ��� �����: ", ch);

}

int genchar_parse(CHAR_DATA * ch, char *arg)
{
	switch (*arg) {
	case '�':
	case '�':
		GET_STR(ch) = MAX(GET_STR(ch) - 1, MIN_STR(ch));
		break;
	case '�':
	case '�':
		GET_DEX(ch) = MAX(GET_DEX(ch) - 1, MIN_DEX(ch));
		break;
	case '�':
	case '�':
		GET_INT(ch) = MAX(GET_INT(ch) - 1, MIN_INT(ch));
		break;
	case '�':
	case '�':
		GET_WIS(ch) = MAX(GET_WIS(ch) - 1, MIN_WIS(ch));
		break;
	case '�':
	case '�':
		GET_CON(ch) = MAX(GET_CON(ch) - 1, MIN_CON(ch));
		break;
	case '�':
	case '�':
		GET_CHA(ch) = MAX(GET_CHA(ch) - 1, MIN_CHA(ch));
		break;
	case '�':
	case '�':
		GET_STR(ch) = MIN(GET_STR(ch) + 1, MAX_STR(ch));
		break;
	case '�':
	case '�':
		GET_DEX(ch) = MIN(GET_DEX(ch) + 1, MAX_DEX(ch));
		break;
	case '�':
	case '�':
		GET_INT(ch) = MIN(GET_INT(ch) + 1, MAX_INT(ch));
		break;
	case '�':
	case '�':
		GET_WIS(ch) = MIN(GET_WIS(ch) + 1, MAX_WIS(ch));
		break;
	case '�':
	case '�':
		GET_CON(ch) = MIN(GET_CON(ch) + 1, MAX_CON(ch));
		break;
	case '�':
	case '�':
		GET_CHA(ch) = MIN(GET_CHA(ch) + 1, MAX_CHA(ch));
		break;
	case '�':
	case '�':
		send_to_char(genchar_help, ch);
		break;
	case '�':
	case '�':
		if (SUM_STATS(ch) != SUM_ALL_STATS)
			break;
		return GENCHAR_EXIT;
	case 'Y':
	case 'y':
	case '�':
	case '�':
		roll_real_abils(ch);
		return GENCHAR_EXIT;
		break;
	default:
		break;
	}
	return GENCHAR_CONTINUE;
}

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
// hand - ��� ���������� ����/������� �����, �� � ������ �� ���� ��� ����� ��������� �� ������������
// ���/���� ������ ����/�����, �.�. ��� ������ ������ ���� ������ ��������� ����� ��������� ��� �� ��������������� ����
void roll_real_abils(CHAR_DATA * ch, bool hand)
{
	int i;

	switch (ch->player.chclass) {
	case CLASS_CLERIC:
		ch->real_abils.cha = 10;
		do {
			ch->real_abils.con = 12 + number(0, 3);
			ch->real_abils.wis = 18 + number(0, 5);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 57/48 roll 13/9
		while (ch->real_abils.con + ch->real_abils.wis + ch->real_abils.intel != 57);
		do {
			ch->real_abils.str = 11 + number(0, 3);
			ch->real_abils.dex = 10 + number(0, 3);
		}		// 92/88 roll 6/4
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.wis += 2;
		ch->real_abils.intel += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
		do {
			ch->real_abils.str = 10 + number(0, 4);
			ch->real_abils.wis = 17 + number(0, 5);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 55/45 roll 14/10
		while (ch->real_abils.str + ch->real_abils.wis + ch->real_abils.intel != 55);
		do {
			ch->real_abils.con = 10 + number(0, 3);
			ch->real_abils.dex = 9 + number(0, 3);
			ch->real_abils.cha = 13 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.wis += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_NECROMANCER:
		do {
			ch->real_abils.cha = 10 + number(0, 2);
			ch->real_abils.wis = 20 + number(0, 3);
			ch->real_abils.intel = 18 + number(0, 5);
		}		// 55/45 roll 14/10
		while (ch->real_abils.cha + ch->real_abils.wis + ch->real_abils.intel != 55);
		do {
			ch->real_abils.str = 9 + number(0, 6);
			ch->real_abils.dex = 9 + number(0, 4);
			ch->real_abils.con = 11 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.con += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_THIEF:
		do {
			ch->real_abils.str = 16 + number(0, 3);
			ch->real_abils.con = 14 + number(0, 3);
			ch->real_abils.dex = 20 + number(0, 3);
		}		// 57/50 roll 9/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 57);
		do {
			ch->real_abils.wis = 9 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 3);
			ch->real_abils.intel = 9 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.dex += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_WARRIOR:
		ch->real_abils.cha = 10;
		do {
			ch->real_abils.str = 20 + number(0, 4);
			if (hand)
				ch->real_abils.dex = 11;
			else
				ch->real_abils.dex = 8 + number(0, 3);
			ch->real_abils.con = 20 + number(0, 3);
		}		// 55/48 roll 10/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 11 + number(0, 4);
			if (hand)
				ch->real_abils.wis = 12 + number(0, 3);
			else
				ch->real_abils.wis = 11 + number(0, 4);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(165, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		break;

	case CLASS_ASSASINE:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.str = 16 + number(0, 5);
			ch->real_abils.dex = 18 + number(0, 5);
			ch->real_abils.con = 14 + number(0, 5);
		}		// 55/48 roll 15/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 11 + number(0, 3);
			ch->real_abils.wis = 11 + number(0, 3);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(150, 200);
		break;

	case CLASS_GUARD:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.str = 19 + number(0, 3);
			ch->real_abils.dex = 13 + number(0, 3);
			ch->real_abils.con = 16 + number(0, 5);
		}		// 55/48 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.intel = 10 + number(0, 4);
			ch->real_abils.wis = 10 + number(0, 4);
		}		// 92/87 roll 8/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 170) : number(160, 200);
		break;

	case CLASS_PALADINE:
		do {
			ch->real_abils.str = 18 + number(0, 3);
			ch->real_abils.wis = 14 + number(0, 4);
			ch->real_abils.con = 14 + number(0, 4);
		}		// 53/46 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.wis != 53);
		do {
			ch->real_abils.intel = 12 + number(0, 4);
			ch->real_abils.dex = 10 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 4);
		}		// 92/87 roll 11/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.cha += 1;
		ch->real_abils.wis += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 175) : number(140, 190);
		break;

	case CLASS_RANGER:

		do {
			ch->real_abils.str = 18 + number(0, 6);
			ch->real_abils.dex = 14 + number(0, 6);
			ch->real_abils.con = 16 + number(0, 3);
		}		// 53/46 roll 12/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 53);
		do {
			ch->real_abils.intel = 11 + number(0, 5);
			ch->real_abils.wis = 11 + number(0, 5);
			ch->real_abils.cha = 11 + number(0, 5);
		}		// 92/85 roll 10/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.con += 1;
		ch->real_abils.dex += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_SMITH:
		do {
			ch->real_abils.str = 18 + number(0, 5);
			ch->real_abils.dex = 14 + number(0, 3);
			ch->real_abils.con = 14 + number(0, 3);
		}		// 53/46 roll 11/7
		while (ch->real_abils.str + ch->real_abils.dex + ch->real_abils.con != 53);
		do {
			ch->real_abils.intel = 10 + number(0, 3);
			ch->real_abils.wis = 11 + number(0, 4);
			ch->real_abils.cha = 11 + number(0, 4);
		}		// 92/85 roll 11/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(170, 200);
		break;

	case CLASS_MERCHANT:
		do {
			ch->real_abils.str = 18 + number(0, 3);
			ch->real_abils.con = 16 + number(0, 3);
			ch->real_abils.dex = 14 + number(0, 3);
		}		// 55/48 roll 9/7
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex != 55);
		do {
			ch->real_abils.wis = 10 + number(0, 3);
			ch->real_abils.cha = 12 + number(0, 3);
			ch->real_abils.intel = 10 + number(0, 3);
		}		// 92/87 roll 9/5
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.con += 2;
		ch->real_abils.cha += 1;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_DRUID:
		ch->real_abils.cha = 12;
		do {
			ch->real_abils.con = 13 + number(0, 4);
			ch->real_abils.wis = 15 + number(0, 3);
			ch->real_abils.intel = 17 + number(0, 5);
		}		// 53/45 roll 12/8
		while (ch->real_abils.con + ch->real_abils.wis + ch->real_abils.intel != 53);
		do {
			ch->real_abils.str = 14 + number(0, 3);
			ch->real_abils.dex = 10 + number(0, 2);
		}		// 92/89 roll 5/3
		while (ch->real_abils.str + ch->real_abils.con + ch->real_abils.dex +
		       ch->real_abils.wis + ch->real_abils.intel + ch->real_abils.cha != 92);
		/* ADD SPECIFIC STATS */
		ch->real_abils.str += 1;
		ch->real_abils.intel += 2;
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(ch, i) = SPELL_RUNES;
		break;

	default:
		log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)", GET_NAME(ch));
	};
}
