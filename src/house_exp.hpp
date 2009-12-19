// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HOUSE_EXP_HPP_INCLUDED
#define HOUSE_EXP_HPP_INCLUDED

#include <list>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "char.hpp"

void update_clan_exp();
void save_clan_exp();
// ������ ���������� � ���������� ����� (� �������)
const int CLAN_EXP_UPDATE_PERIOD = 60;

class ClanExp
{
public:
	ClanExp() : buffer_exp_(0), total_exp_(0) {};
	long long get_exp() const;
	void add_chunk();
	void add_temp(int exp);
	void load(std::string abbrev);
	void save(std::string abbrev) const;
	void update_total_exp();
private:
	int buffer_exp_;
	long long total_exp_;
	typedef std::list<long long> ExpListType;
	ExpListType list_;
};

/**
* ������ ��������� �� � �������� ����� �� ������� �����.
*/
class ClanPkLog
{
public:
	ClanPkLog() : need_save(false) {};

	void load(std::string abbrev);
	void save(std::string abbrev);
	void print(CHAR_DATA *ch) const;
	static void check(CHAR_DATA *ch, CHAR_DATA *victim);

private:
	void add(const std::string &text);

	bool need_save;
	std::list<std::string> pk_log;
};

#endif // HOUSE_EXP_HPP_INCLUDED
