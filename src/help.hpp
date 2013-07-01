// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include <string>
#include <vector>
#include "conf.h"
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

#endif // HELP_HPP_INCLUDED
