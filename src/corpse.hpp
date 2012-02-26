// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer);
OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer = NULL);

namespace FullSetDrop
{

// ������ ���������� ������� ����� (������)
const int SAVE_PERIOD = 27;
// ��� ���������� ������� �� �����������
const bool SOLO_TYPE = false;
const bool GROUP_TYPE = true;

// ���� ������� ��� ������ ����
void init();
// ���������� ���� � ������ � ����������� �� ���-�� ������� �������
void add_kill(CHAR_DATA *mob, int members);
// ���������� �������
void save_list(bool list_type);
// ����� ������� ������ �� show stats
void show_stats(CHAR_DATA *ch);
// ������ ������ ����� � ������������� ������ ������
// ��� ������� ������� ���� � ����-������
void reload();
// �������� ����� ������
int check_mob(int mob_rnum);
// ����� ���� ������ ������
void renumber_obj_rnum(int rnum);
// ���������� ���� � ������� �������
void init_xhelp();
void init_xhelp_full();
// �������� ���� ����� �� ��������� �����
void add_mob_stat(CHAR_DATA *mob, int members);
// ���� ������ �������� ����� �����
void init_mob_stat();
// ���� �������� ����� �����
void save_mob_stat();

} // namespace FullSetDrop

namespace GlobalDrop
{

void init();
void save();
void renumber_obj_rnum(int rnum);
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *ch);

// ������ ���������� ���������� ����� � ������ (� �������)
const int SAVE_PERIOD = 10;
// ��� ��������� ����� ��������� ������
const int BOOK_UPGRD_VNUM = 1920;
// ��� ��������� ��������
const int WARR1_ENCHANT_VNUM = 1921;
const int WARR2_ENCHANT_VNUM = 1922;
const int WARR3_ENCHANT_VNUM = 1923;

} // namespace GlobalDrop

#endif // CORPSE_HPP_INCLUDED
