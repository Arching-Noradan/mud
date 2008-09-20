// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_PLAYER_HPP_INCLUDED
#define CHAR_PLAYER_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

// ������ ��� ���������� ������ ���� � ��� ������� ������ ����� ���������:
// ����� ����������� ������ � PlayerI, ���� � ������ � Player, ������ � PlayerProxy

/**
* ��������� ��� ����� ������ � ��� �������.
*/
class PlayerI
{
public:
	virtual ~PlayerI() = 0;

private:
	virtual int get_pfilepos() const = 0;
	virtual void set_pfilepos(int pfilepos) = 0;

	virtual room_rnum get_was_in_room() const = 0;
	virtual void set_was_in_room(room_rnum was_in_room) = 0;

	virtual std::string const & get_passwd() const = 0;
	virtual void set_passwd(std::string const & passwd) = 0;
};

/**
* ����� ���������� ���� � ������ ������.
*/
class Player : public PlayerI
{
public:
	Player();

	int get_pfilepos() const;
	void set_pfilepos(int pfilepos);

	room_rnum get_was_in_room() const;
	void set_was_in_room(room_rnum was_in_room);

	std::string const & get_passwd() const;
	void set_passwd(std::string const & passwd);

private:
	// ���������� ����� � ����� �����-����� (�� ����� �����, �� ������ ������ ������ �� ���)
	int pfilepos_;
	// �������, � ������� ��� ��� �� ����, ��� ��� ��������� � �������� (linkdrop)
	room_rnum was_in_room_;
	// ��� ������
	std::string passwd_;
};

#endif // CHAR_PLAYER_HPP_INCLUDED
