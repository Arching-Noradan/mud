// deathtrap.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEATHTRAP_HPP_INCLUDED
#define DEATHTRAP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/**
* ������ ����-�� (������� ������������ ��� ���), ����� �� ������ ������ 2 ������� �� 64� ��������.
* ���������/�������� ��� ������, ������ � ��� � ������������ ��� ���
*/
namespace DeathTrap {

// ������������� ������ ��� �������� ����
void load();
// ���������� ����� ������� � ��������� �� �����������
void add(ROOM_DATA* room);
// �������� ������� �� ������ ����-��
void remove(ROOM_DATA* room);
// �������� ���������� ��, ��������� ������ 2 ������� � ��������
void activity();

} // namespace DeathTrap

#endif // DEATHTRAP_HPP_INCLUDED
