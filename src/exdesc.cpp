// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "handler.h"
#include "utils.h"

namespace ExtraDescSystem
{

void ExtraDesc::add(const char *key, const char *text)
{
	if (key && text)
	{
		extradesc_node tmp_node;
		tmp_node.key = key;
		tmp_node.text = text;
		list_.push_back(tmp_node);
	}
}

/**
* ��������� ����� �������� ��� ��� ������� ������� ���������� ���� text
* � ����� ��� ������.
*/
void ExtraDesc::add_from_make(const char *key, const char *text)
{
	if (list_.empty())
	{
		add(key, text);
	}
	else
	{
		list_[0].text += text;
	}
}

std::string ExtraDesc::find(const char *key) const
{
	if (!key || list_.empty())
	{
		return "";
	}
	for (ExtraDescList::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		if (isname(key, i->key))
		{
			return i->text;
		}
	}
	return "";
}

std::string ExtraDesc::print_keys() const
{
	std::string out;
	for (ExtraDescList::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		out += " " + i->key;
	}
	return out;
}

bool ExtraDesc::empty() const
{
	return list_.empty();
}

void ExtraDesc::clear()
{
	list_.clear();
}

/**
* �������� �� ������� �����-�������� � ���������� ������ � �������.
*/
bool ExtraDesc::have_desc(const std::string &key, const std::string &text) const
{
	for (ExtraDescList::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		if (!cpp_str_cmp(key, i->key) && !cpp_str_cmp(text, i->text))
		{
			return true;
		}
	}
	return false;
}

/**
* ���������� ��� ���������� �������� � ���� � ��������� �� ������������ � ���������.
* ��� ������� ��������� ��� ���������� �������� � ��������� - ������� � ���� (������).
*/
const char * ExtraDesc::print_to_char_file(const OBJ_DATA *proto) const
{
	std::string out;
	for (ExtraDescList::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		if (!proto || !proto->extra_desc->have_desc(i->key, i->text))
		{
			out += "Edes: " + i->key + "~\n" + i->text + "~\n";
		}
	}
	return out.c_str();
}

/**
* ���������� ��� ���������� � room � obj ����� ����.
*/
const char * ExtraDesc::print_to_zone_file() const
{
	std::string out;
	for (ExtraDescList::const_iterator i = list_.begin(); i != list_.end(); ++i)
	{
		out += "E\n" + i->key + "~\n" + i->text + "~\n";
	}
	return out.c_str();
}

/**
* ���������� ���� � ���������� ������� num �� ������� (��� ���).
*/
struct extradesc_node ExtraDesc::olc_get_node(unsigned num) const
{
	if (list_.size() > num)
	{
		return list_[num];
	}
	struct extradesc_node tmp;
	return tmp;
}

/**
* �������� ���� � ���������� ������� num � ������� (��� ���).
*/
void ExtraDesc::olc_set_node(unsigned num, struct extradesc_node node)
{
	if (list_.size() > num)
	{
		list_[num] = node;
	}
}

/**
* �������� �� ������� ���������� �������� ����� ����������� ������ num (��� ���).
*/
bool ExtraDesc::olc_get_next(unsigned num) const
{
	return list_.size() > num + 1 ? true : false;
}

} // namespace ExtraDescSystem
