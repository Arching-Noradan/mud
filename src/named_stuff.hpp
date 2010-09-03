// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 WorM
// Part of Bylins http://www.mud.ru

#ifndef NAMEDSTUFF_HPP_INCLUDED
#define NAMEDSTUFF_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"
#define SCMD_NAMED_LIST 	1
#define SCMD_NAMED_EDIT 	2
#define NAMED_MAIN		0
#define NAMED_EDIT		1
#define NAMED_EDIT_VNUM		2
#define NAMED_EDIT_OWNER	3
#define NAMED_EDIT_CLAN		4
#define NAMED_EDIT_ALLI		5

namespace NamedStuff
{

ACMD(do_named);
void nedit_menu(CHAR_DATA *ch);
bool parse_nedit_menu(CHAR_DATA *ch, char *arg);

struct stuff_node
{
	int uid;		// uid ���������
	std::string mail;	// e-mail ���������� ������� ����� ���������
	int can_clan;		// ����� �� ���� ���������
	int can_alli;		// ����� �� ������ ���������
};

// ����� ������ �������� �����
typedef boost::shared_ptr<stuff_node> StuffNodePtr;
typedef std::map<long /* vnum �������� */, StuffNodePtr> StuffListType;

extern StuffListType stuff_list;

void save();
void load();
//�������� �������� �� ������� ������� ����, simple ��� �������� ����� � ������
//������������ �������� �� �������� � check_anti_classes false-�������� true-����������
bool check_named(CHAR_DATA * ch, OBJ_DATA * obj, const bool simple=false);
//��������� ��������� �������� ����� � ������
void receive_items(CHAR_DATA * ch, CHAR_DATA * mailman);

} // namespace NamedStuff

#endif // NAMEDSTUFF_HPP_INCLUDED
