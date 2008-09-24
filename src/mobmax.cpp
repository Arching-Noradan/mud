// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <map>
#include <vector>
#include <boost/array.hpp>
#include "mobmax.hpp"
#include "char.hpp"
#include "utils.h"

extern mob_rnum top_of_mobt;
extern CHAR_DATA *mob_proto;
extern INDEX_DATA *mob_index;

// ������������ ������� �����
static const int MAX_MOB_LEVEL = 50;
// ����������� ���������� ����� ������ ������ � ����� �������
static const int MIN_MOB_IN_MOBKILL  = 2;
// ������������ ���������� ����� ������ ������ � ����� �������
static const int MAX_MOB_IN_MOBKILL = 100;
// �� ������� ��� ���� ����� ����� ������, ��� �� ���� � ����, ����� ������ ������������
static const int MOBKILL_KOEFF = 3;
// ���-�� ����� ������� ������
static boost::array<int, MAX_MOB_LEVEL + 1> num_levels = {0};
// ��� ������������ ������ � ������� (��� �������� ������ �����-�����)
static std::map<int/* ���� ���� */, int/* ����� ���� */> vnum_to_level;

/**
* ���� ������ ���-�� ����� ������� ������ � ��� ������������ ������ � �������.
*/
void MobMax::init()
{
	for (int i = 0; i <= top_of_mobt; ++i)
	{
		int level = GET_LEVEL(mob_proto + i);
		if (level > MAX_MOB_LEVEL)
			log("Warning! Mob >MAXIMUN lev!");
		else if (level < 0)
			log("Warning! Mob <0 lev!");
		else
		{
			++num_levels[level];
			vnum_to_level[mob_index[i].vnum] = level;
		}
	}

	for (int i = 0; i <= MAX_MOB_LEVEL; ++i)
	{
		log("Mob lev %d. Num of mobs %d", i, num_levels[i]);
		num_levels[i] = num_levels[i] / MOBKILL_KOEFF;
		if (num_levels[i] < MIN_MOB_IN_MOBKILL)
			num_levels[i] = MIN_MOB_IN_MOBKILL;
		if (num_levels[i] > MAX_MOB_IN_MOBKILL)
			num_levels[i] = MAX_MOB_IN_MOBKILL;
		log("Mob lev %d. Max in MobKill file %d", i, num_levels[i]);

	}
}

/**
* ���������� ����� ���������� vnum ���� ��� -1, ���� ������ ���.
*/
int MobMax::get_level_by_vnum(int vnum)
{
	std::map<int, int>::const_iterator it = vnum_to_level.find(vnum);
	if (it != vnum_to_level.end())
		return it->second;
	return -1;
}

/**
* ������ ������� �� ����� ������ level, ���� ����� ������� ������ ����� ����������� ���-��.
*/
void MobMax::refresh(int level)
{
	int count = 0;
	for (MobMaxType::iterator it = mobmax_.begin(); it != mobmax_.end();/* empty*/)
	{
		if (it->second.level == level)
		{
			if (count > num_levels[level])
				mobmax_.erase(it++);
			else
			{
				++count;
				++it;
			}
		}
		else
			++it;
	}
}

/**
* ���������� ������� �� ���� vnum, ������ level. count ��� ������ ���� ������� �����.
*/
void MobMax::add(CHAR_DATA *ch, int vnum, int count, int level)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0 || count < 1 || level < 0 || level > MAX_MOB_LEVEL) return;

	MobMaxType::iterator it = mobmax_.find(vnum);
	if (it != mobmax_.end())
		it->second.count += count;
	else
	{
		struct mobmax_data tmp_data;
		tmp_data.count = count;
		tmp_data.level = level;
		mobmax_.insert(std::make_pair(vnum, tmp_data));
	}
	refresh(level);
}

/**
* ������ add ��� ������ �������� ��� ����� �� ����� �������� ���������.
*/
void MobMax::load(CHAR_DATA *ch, int vnum, int count, int level)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0 || count < 1 || level < 0 || level > MAX_MOB_LEVEL) return;

	struct mobmax_data tmp_data;
	tmp_data.count = count;
	tmp_data.level = level;
	mobmax_.insert(std::make_pair(vnum, tmp_data));
}

/**
* �������� ������� �� ���������� ���� vnum.
*/
void MobMax::remove(int vnum)
{
	MobMaxType::iterator it = mobmax_.find(vnum);
	if (it != mobmax_.end())
		mobmax_.erase(it);
}

/**
* ���������� ���-�� ������ ����� ������� vnum.
*/
int MobMax::get_kill_count(int vnum) const
{
	MobMaxType::const_iterator it = mobmax_.find(vnum);
	if (it != mobmax_.end())
		return it->second.count;
	return 0;
}

/**
* ���������� � �����-����.
*/
void MobMax::save(FILE *saved) const
{
	fprintf(saved, "Mobs:\n");
	for (MobMaxType::const_iterator it = mobmax_.begin(); it != mobmax_.end(); ++it)
		fprintf(saved, "%d %d\n", it->first, it->second.count);
	fprintf(saved, "~\n");

}

/**
* ��� ������� ���� �������� ��� �������.
*/
void MobMax::clear()
{
	mobmax_.clear();
}
