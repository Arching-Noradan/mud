// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_SETS_STUFF_HPP_INCLUDED
#define OBJ_SETS_STUFF_HPP_INCLUDED

#include "conf.h"
#include <array>
#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "char_player.hpp"

namespace obj_sets
{

/// ��������� ���������/����������� ��� ���� � �������
/// ����� ���� ����������� �� ��� ����, �� ���� ���, �� ���� �������
struct msg_node
{
	// ��������� ���� ��� ���������/�����������
	std::string char_on_msg;
	std::string char_off_msg;
	// ���� ���������, �� � �������
	std::string room_on_msg;
	std::string room_off_msg;

	// ��� ��������� � sedit
	bool operator!=(const msg_node &r) const
	{
		if (char_on_msg != r.char_on_msg
			|| char_off_msg != r.char_off_msg
			|| room_on_msg != r.room_on_msg
			|| room_off_msg != r.room_off_msg)
		{
			return true;
		}
		return false;
	}
	bool operator==(const msg_node &r) const
	{
		return !(*this != r);
	}
};

/// ������� ��������� � ��������� � ������� ��������
struct activ_node
{
	activ_node() : skill(0, 0)
	{
		affects = clear_flags;
	};

	// ������� (obj_flags.affects)
	FLAG_DATA affects;
	// APPLY_XXX ������� (affected[MAX_OBJ_AFFECT])
	std::array<obj_affected_type, MAX_OBJ_AFFECT> apply;
	// ��������� ������
	skill_node skill;

	// ��� ��������� � sedit
	bool operator!=(const activ_node &r) const
	{
		if (affects != r.affects
			|| apply != r.apply
			|| skill != r.skill)
		{
			return true;
		}
		return false;
	}
	bool operator==(const activ_node &r) const
	{
		return !(*this != r);
	}
	bool empty() const
	{
		if (!affects.empty() || skill.num > 0)
		{
			return false;
		}
		for (auto i = apply.begin(); i != apply.end(); ++i)
		{
			if (i->location > 0)
			{
				return false;
			}
		}
		return true;
	}
};

/// ���������� ��������� �����
struct set_node
{
	set_node() : enabled(true), uid(uid_cnt++) {};

	// ������ ����: ���/����, ��� ����� �� ������� � ����� ��� ���� ��������
	// ���������� ����, � ������� ����� ���� ������������� ���������� ����
	bool enabled;
	// ��� ���� - ��������� �����, ������� ����
	std::string name;
	// �����, ����������� ��� ���� � ��������� ������� ��� ������ �����
	std::string alias;
	// ����� ��� ����, ������� ������ � slist
	std::string comment;
	// ����� ���������, ���������/����������� (�����������)
	std::map<int, msg_node> obj_list;
	// first - ���-�� ��������� ��� ���������
	std::map<unsigned, activ_node> activ_list;
	// ��������� ���������/����������� �� ���� ��� (�����������)
	msg_node messages;
	// ��� ���� ��� ����������/���������� � ���
	int uid;

	// ������� �����
	static int uid_cnt;

	// ��� ��������� � sedit
	bool operator!=(const set_node &r) const
	{
		if (enabled != r.enabled
			|| name != r.name
			|| alias != r.alias
			|| comment != r.comment
			|| obj_list != r.obj_list
			|| activ_list != r.activ_list
			|| messages != r.messages
			|| uid != r.uid)
		{
			return true;
		}
		return false;
	}
	bool operator==(const set_node &r) const
	{
		return !(*this != r);
	}
};

extern std::vector<boost::shared_ptr<set_node>> sets_list;
extern msg_node global_msg;
extern const unsigned MIN_ACTIVE_SIZE;
extern const unsigned MAX_ACTIVE_SIZE;
extern const unsigned MAX_OBJ_LIST;

size_t setidx_by_objvnum(int vnum);
size_t setidx_by_uid(int uid);
std::string line_split_str(const std::string &str, const std::string &sep,
	size_t len, size_t base_offset = 0);
void verify_set(set_node &set);
void init_obj_index();
bool is_duplicate(int set_uid, int vnum);

} // namespace obj_sets

#endif // OBJ_SETS_STUFF_HPP_INCLUDED
