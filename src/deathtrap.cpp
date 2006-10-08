// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include <list>
#include <algorithm>
#include "deathtrap.hpp"
#include "constants.h"
#include "db.h"
#include "spells.h"
#include "utils.h"
#include "handler.h"

namespace DeathTrap {

// ������ ������� ����-�� � ����
std::list<ROOM_DATA*> room_list;

} // namespace DeathTrap

/**
* ������������� ������ ��� �������� ���� ��� �������������� ������ � ���
*/
void DeathTrap::load()
{
	// �� ������ �������, ������� ������ ����
	room_list.clear();

	for (int i = FIRST_ROOM; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
			room_list.push_back(world[i]);
}

/**
* ���������� ����� ������� � ��������� �� �����������
* \param room - �������, ���. ���������
*/
void DeathTrap::add(ROOM_DATA* room)
{
	std::list<ROOM_DATA*>::const_iterator it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* �������� ������� �� ������ ����-��
* \param room - �������, ���. �������
*/
void DeathTrap::remove(ROOM_DATA* room)
{
	room_list.remove(room);
}

/**
* �������� ���������� ��, ��������� ������ 2 ������� � ��������.
*/
void DeathTrap::activity()
{
	CHAR_DATA *ch, *next;

	for(std::list<ROOM_DATA*>::const_iterator it = room_list.begin(); it != room_list.end(); ++it)
		for (ch = (*it)->people; ch; ch = next) {
			next = ch->next_in_room;
			if (!IS_NPC(ch) && (damage(ch, ch, MAX(1, GET_REAL_MAX_HIT(ch) >> 2), TYPE_ROOMDEATH, FALSE) < 0))
				log("Player %s died in slow DT (room %d)", GET_NAME(ch), (*it)->number);
		}
}
