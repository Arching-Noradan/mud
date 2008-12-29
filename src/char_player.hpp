// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include <boost/shared_ptr.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "quested.hpp"
#include "mobmax.hpp"

class Player
{
public:
	Player();
	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

	room_rnum get_from_room() const;
	void set_from_room(room_rnum was_in_room);

	void remort();

	void add_reserved_money(int money);
	void remove_reserved_money(int money);
	int get_reserved_money() const;

	void inc_reserved_count();
	void dec_reserved_count();
	int get_reserved_count() const;

	// ��� ��� ��� ������ ��������... =)
	friend void save_char(CHAR_DATA *ch);

	// ����� ����������� �������
	Quested quested;
	// ������� �� ��������� �����
	MobMax mobmax;
	// ����� ���� �����
	static boost::shared_ptr<Player> shared_mob;

private:
	// ���������� ����� � ����� �����-����� (�� ����� �����, �� ������ ������ ������ �� ���)
	// TODO: ������ ��� ����� ���������� ������ ����������� ������ �� ����� ��� ������ ���� � �.�. �����, ����������
	// get_ptable_by_name ��� find_name (������������ ���� ������) � ������ ������ �� ��/���, ���� ��� ����� ���-����
	int pfilepos_;
	// �������, � ������� ��� ��� �� ����, ��� ��� ��������� � �������� (linkdrop)
	room_rnum was_in_room_;
	// ��� ������
	std::string passwd_;
	// �������, ��� ��� ��� �� ������ char_from_room (was_in_room_ ��� ��� ������������ �� �� ������)
	// � ������ ������ ���� ����� ��� �������� ���� �� �� ��� ����� �� ����� ����� ��������, �� ����� � ��� ����� �����������
	room_rnum from_room_;
	// ����������������� �� ����� ���� (�������)
	int reserved_money_;
	// ���-�� ������ ��� ����������� � ��������� ��������
	int reserved_count_;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
