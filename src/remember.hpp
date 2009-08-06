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

enum { ALL, PERSONAL, GROUP, PRAY };

} // namespace Remember

class CharRemember
{
public:
	CharRemember() : answer_id_(NOBODY), num_str_(15) {};
	void add_str(std::string text, int flag);
	std::string get_text(int flag) const;
	void set_answer_id(int id);
	int get_answer_id() const;
	void reset();
private:
	typedef std::list<std::string> RememberListType;
	RememberListType personal_; // �����
	RememberListType group_; // ������
	RememberListType pray_; // ��������� (������)
	long answer_id_; // id ���������� ��������� (��� ������)
	unsigned int num_str_; // ���-�� ��������� ����� (����� ���������)
	std::string last_tell_; // ��������� ��������� ������ (�� �����)
};

#endif // REMEMBER_HPP_INCLUDED
