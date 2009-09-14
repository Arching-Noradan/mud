// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DPS_HPP_INCLUDED
#define DPS_HPP_INCLUDED

#include <list>
#include <map>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace DpsSystem
{

// ������, ����� �� ������� ������� (����, ����� ��������, ���� �� ������, ������� ����� �� ������)
enum { PERS_DPS, PERS_CHARM_DPS, GROUP_DPS, GROUP_CHARM_DPS };

class DpsNode
{
public:
	DpsNode(long id = 0) : dmg_(0), over_dmg_(0), curr_time_(0), total_time_(0), id_(id) {};
	void start_timer();
	void stop_timer();
	void add_dmg(int dmg, int over_dmg);
	void set_name(const char *name);
	int get_stat() const;
	unsigned get_dmg() const;
	unsigned get_over_dmg() const;
	long get_id() const;
	const std::string & get_name() const;

private:
	unsigned dmg_; // ���������� �����
	unsigned over_dmg_; // ����� ������ �� ��� ������� ����
	time_t curr_time_; // �����, ����������� � ������� ���
	time_t total_time_; // ����� ����������� � ���� �����
	std::string name_; // ��� ��� ������ � ������ ��� ���������� � ����
	long id_; // ��� �������������
};

typedef std::list<DpsNode> CharmListType;

/**
* ������� �� DpsNode �� ������� �������� (��� ������).
*/
class PlayerDpsNode : public DpsNode
{
public:
	void start_charm_timer(int id, const char *name);
	void stop_charm_timer(int id);
	void add_charm_dmg(int id, int dmg, int over_dmg);
	std::string print_charm_stats() const;
	void print_group_charm_stats(CHAR_DATA *ch) const;

private:
	CharmListType::iterator find_charmice(long id);

	CharmListType charm_list_; // ������ �������� (MAX_DPS_CHARMICE)
};

typedef std::map<long /* id */, PlayerDpsNode> GroupListType;

/**
* ������� ��������, ������� � Player.
*/
class Dps
{
public:
	void start_timer(int type, CHAR_DATA *ch);
	void stop_timer(int type, CHAR_DATA *ch);
	void add_dmg(int type, CHAR_DATA *ch, int dmg, int over_dmg);
	void clear(int type);
	void print_stats(CHAR_DATA *ch);
	void print_group_stats(CHAR_DATA *ch);

private:
	void start_group_timer(int id, const char *name);
	void stop_group_timer(int id);
	void add_group_dmg(int id, int dmg, int over_dmg);

	void start_group_charm_timer(CHAR_DATA *ch);
	void stop_group_charm_timer(int master_id, int charm_id);
	void add_group_charm_dmg(int master_id, int charm_id, int dmg, int over_dmg);

	GroupListType group_dps_; // ��������� ����������
	PlayerDpsNode pers_dps_; // ������������ ���������� � ���� �������
};

} // namespace DpsSystem

#endif // DPS_HPP_INCLUDED
