// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef REMEMBER_HPP_INCLUDED
#define REMEMBER_HPP_INCLUDED

#include <list>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace Remember
{

enum { ALL, PERSONAL, CLAN, ALLY, GOSSIP, OFFTOP, PRAY };
// ���-�� ������������ ����� � ������ ������
const unsigned int MAX_REMEMBER_NUM = 100;
// ���-�� ��������� ������ �� ���������
const unsigned int DEF_REMEMBER_NUM = 15;
typedef std::list<std::string> RememberListType;

std::string time_format();
std::string format_gossip(CHAR_DATA *ch, CHAR_DATA *vict, int cmd, const char *argument);

} // namespace Remember

class CharRemember
{
public:
	CharRemember() : num_str_(Remember::DEF_REMEMBER_NUM) {};
	void add_str(std::string text, int flag);
	std::string get_text(int flag) const;
	void reset();
	bool set_num_str(unsigned int num);
	unsigned int get_num_str() const;

private:
	unsigned int num_str_; // ���-�� ��������� ����� (����� ���������)
	Remember::RememberListType all_; // ��� ������������ ������ + ��������� (TODO: ����� � ������ � �����?), ������� �����������
	Remember::RememberListType personal_; // �����
};

#endif // REMEMBER_HPP_INCLUDED
