// Part of Bylins http://www.mud.ru

#ifndef SETS_DROP_HPP_INCLUDED
#define SETS_DROP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace SetsDrop
{

// ������ ���������� ������� ����� (������)
const int SAVE_PERIOD = 27;
// ���� ������� ��� ������ ����
void init();
// ����� ������� ������ �� show stats
void show_stats(CHAR_DATA *ch);
// ������ ������ ����� � ������������� ������ ������
// ��� ������� ���������� �� ��������� �����
void reload(int zone_vnum = 0);
// ���������� ������� ����� �� �������
void reload_by_timer();
// �������� ����� ������
int check_mob(int mob_rnum);
// ����� ���� ������ ������
void renumber_obj_rnum(const int rnum, const int mob_rnum = -1);
// ���������� ���� � ������� �������
void init_xhelp();
void init_xhelp_full();
// ���� ����� �� ��������� �����
void add_mob_stat(CHAR_DATA *mob, int members);
// ���� ����� �����
void save_mob_stat();
// ������ ���������� ���� �� ���������� ����
void show_zone_stat(CHAR_DATA *ch, int zone_vnum);
// ������ ������� ������ ������� ����� ����� ��������� �������
void print_timer_str(CHAR_DATA *ch);

} // namespace SetsDrop

#endif // SETS_DROP_HPP_INCLUDED
