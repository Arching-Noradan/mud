// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_SETS_HPP_INCLUDED
#define OBJ_SETS_HPP_INCLUDED

#include "conf.h"
#include <array>
#include <vector>

#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"

/// ������ ������� ������� �� �������� ����������� �������� �� ����, � �� ��
/// ���������� ��������, �������� �� ������ ����� + ��� ����� ����������� ���
namespace obj_sets
{

struct idx_node
{
	// ������ ����
	size_t set_idx;
	// �������� �� ���� �� ������� ����
	std::vector<OBJ_DATA *> obj_list;
	// ���-�� ��� �������������� ������ (��� ��������� ���������/�����������)
	int activated_cnt;
	// ���������� ��� ��� ��������� ���������
	bool show_msg;
};

/// ���������� ������ ��������� � ��������� �� �����
/// ��� ����� ������� ��������� � ���� � affect_total()
class WornSets
{
public:
	/// ������� ����� ������ ��������������
	void clear();
	/// ���������� ������ ����� idx_list_ ��� ������ ��������� �� ����
	void add(CHAR_DATA *ch, OBJ_DATA *obj);
	/// �������� ����������� (��� ����� �����)
	void check(CHAR_DATA *ch);

private:
	std::array<idx_node, NUM_WEARS> idx_list_;
};

void load();
void save();
void print_off_msg(CHAR_DATA *ch, OBJ_DATA *obj);
void print_identify(CHAR_DATA *ch, const OBJ_DATA *obj);
void init_xhelp();
std::string get_name(size_t idx);

} // namespace obj_sets

namespace obj_sets_olc
{

void parse_input(CHAR_DATA *ch, const char *arg);

} // namespace obj_sets_olc

ACMD(do_slist);
ACMD(do_sedit);

#endif // OBJ_SETS_HPP_INCLUDED
