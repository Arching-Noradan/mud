// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include "conf.h"
#include <string>
#include <map>

#include "sysdep.h"
#include "interpreter.h"

ACMD(do_help);

namespace HelpSystem
{

extern bool need_update;
enum Flags { STATIC, DYNAMIC, TOTAL_NUM };

// ���������� � ������� ������ � ������ ������ ����� ����
void add(const std::string key_str, const std::string entry_str, int min_level, HelpSystem::Flags add_flag);
// ���������� �����, ���� � DYNAMIC ������ � ���������� sets_drop_page
void add_sets(const std::string key_str, const std::string entry_str);
// ����/������ ����������� ������� �������
void reload(HelpSystem::Flags sort_flag);
// ����/������ ���� �������
void reload_all();
// �������� ��� � ������ ����� �� �������� ������������ ������� (����-�����, �����-����)
void check_update_dynamic();

} // namespace HelpSystem

namespace PrintActivators
{

// ��������� ������ ��� ����� �����
struct clss_activ_node
{
	clss_activ_node() { total_affects = clear_flags; };
	// �������
	FLAG_DATA total_affects;
	// ��������
	std::vector<obj_affected_type> affected;
	// �����
	std::map<int, int> skills;
};

std::string print_skills(const std::map<int, int> &skills, bool activ);
void sum_affected(std::vector<obj_affected_type> &l,
	const std::array<obj_affected_type, MAX_OBJ_AFFECT> &r);
void sum_skills(std::map<int, int> &target, const std::map<int, int> &add);

} // namespace PrintActivators

#endif // HELP_HPP_INCLUDED
