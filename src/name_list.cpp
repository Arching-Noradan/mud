// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <limits>
#include <map>
#include <string>
#include <set>
#include "name_list.hpp"
#include "utils.h"
#include "char.hpp"
#include "handler.h"
#include "interpreter.h"

namespace
{

/**
* ������ skip_spaces � a_isalnum.
*/
template<class T> void skip_delim(T string)
{
	for (; **string && !a_isalnum(**string); (*string)++) ;
}

/**
* ������ any_one_arg � a_isalnum.
* �� ���� ����� ���� ������� � ������ �������, ��� ������ ��� ���� �� ����������� �������� ���������.
*/
const char * any_one_name(const char *argument, char *first_arg)
{
	if (!argument)
	{
		log("SYSERR: any_one_name() passed a NULL pointer.");
		return 0;
	}
	skip_delim(&argument);

	while (*argument && a_isalnum(*argument))
	{
		*(first_arg++) = a_lcc(*argument);
		argument++;
	}

	*first_arg = '\0';

	return (argument);
}

/**
* ������ half_chop � a_isalnum.
*/
void get_one_name(char const *string, char *arg1, char *arg2)
{
	char const *temp;
	temp = any_one_name(string, arg1);
	skip_delim(&temp);
	strcpy(arg2, temp);
}

////////////////////////////////////////////////////////////////////////////////

// ������ ������� �� ������� ����� � ��������������� �� �����
typedef std::set<CHAR_DATA *> CharNodeListType;
typedef std::map<std::string /* ��� */, CharNodeListType> CharListType;
CharListType char_list;

// ������ �����, ������������� �� ���������� ������� ��� ��������� ������
typedef std::map<int /* char_num */, CHAR_DATA *> TempCharListType;

} // namespace

namespace CharacterList
{

/**
* ���������� ���� � ��� � ����������� �� ������� �� ��� �������.
*/
void add(CHAR_DATA *ch)
{
	if (!GET_NAME(ch)) return;

	ch->set_serial_num();

	char buffer[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	strcpy(buffer, GET_NAME(ch));

	while (buffer[0] != 0)
	{
		get_one_name(buffer, out, buffer);
		CharListType::iterator it = char_list.find(out);
		if (it != char_list.end())
		{
			it->second.insert(ch);
		}
		else
		{
			CharNodeListType tmp_node;
			tmp_node.insert(ch);
			char_list.insert(std::make_pair(out, tmp_node));
		}
	}
}

/**
* �������� ���� �� ���� ����� ����.
*/
void remove(CHAR_DATA *ch)
{
	for (CharListType::iterator it = char_list.begin(); it != char_list.end(); /* empty */)
	{
		CharNodeListType::iterator tmp_it = it->second.find(ch);
		if (tmp_it != it->second.end())
		{
			it->second.erase(tmp_it);
		}
		// ������, ���������� �������
		if (it->second.empty())
		{
			char_list.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

/**
* ���������� ���������� ������ �� ���� ������� �� ������� ����� � ����������� �� ����������� ������ ����.
*/
void fill_list_by_name(const char *name, TempCharListType &cont)
{
	CharListType::const_iterator it = char_list.lower_bound(name);
	while (it != char_list.end())
	{
		if (isname(name, it->first.c_str()))
		{
			for (CharNodeListType::iterator tmp_it = it->second.begin(); tmp_it != it->second.end(); ++tmp_it)
			{
				cont[(*tmp_it)->get_serial_num()] = *tmp_it;
			}
			++it;
		}
		else
		{
			return;
		}
	}
}

/**
* ����� �� ��������� ������ ������� ������������ � ������ ������� ������ ����.
*/
CHAR_DATA * check_node(const char *str, TempCharListType &cont)
{
	for (TempCharListType::reverse_iterator it = cont.rbegin(); it != cont.rend(); ++it)
	{
		if (isname(str, GET_NAME(it->second)))
		{
			return it->second;
		}
	}
	return 0;
}

/**
* ���������� ���������� (���������� ������� � character_list) ���� ��� 0 �� ��� ������.
*/
CHAR_DATA * get_by_name(const char *str)
{
	if (!str || !*str) return 0;

	char buffer[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	strcpy(buffer, str);

	get_one_name(buffer, out, buffer);
	if (!*out) return 0;

	TempCharListType temp_list;
	fill_list_by_name(out, temp_list);
	if (temp_list.empty())
	{
		return 0;
	}
	return check_node(str, temp_list);
}

} // namespace CharacterList

////////////////////////////////////////////////////////////////////////////////
// ���� ��� ������ �������� � ����� ��� ���������

namespace
{

typedef std::set<OBJ_DATA *> ObjNodeListType;
typedef std::map<std::string, ObjNodeListType> ObjListType;
ObjListType obj_list;

typedef std::map<int, OBJ_DATA *> TempObjListType;

} // namespace

namespace ObjectList
{

void add(OBJ_DATA *obj)
{
	if (!obj->name) return;

	obj->set_serial_num();

	char buffer[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	strcpy(buffer, obj->name);

	while (buffer[0] != 0)
	{
		get_one_name(buffer, out, buffer);
		ObjListType::iterator it = obj_list.find(out);
		if (it != obj_list.end())
		{
			it->second.insert(obj);
		}
		else
		{
			ObjNodeListType tmp_node;
			tmp_node.insert(obj);
			obj_list.insert(std::make_pair(out, tmp_node));
		}
	}
}

void remove(OBJ_DATA *obj)
{
	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); /* empty */)
	{
		ObjNodeListType::iterator tmp_it = it->second.find(obj);
		if (tmp_it != it->second.end())
		{
			it->second.erase(tmp_it);
		}
		// ������, ���������� �������
		if (it->second.empty())
		{
			obj_list.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void fill_list_by_name(const char *name, TempObjListType &cont)
{
	ObjListType::const_iterator it = obj_list.lower_bound(name);
	while (it != obj_list.end())
	{
		if (isname(name, it->first.c_str()))
		{
			for (ObjNodeListType::iterator tmp_it = it->second.begin(); tmp_it != it->second.end(); ++tmp_it)
			{
				cont[(*tmp_it)->get_serial_num()] = *tmp_it;
			}
			++it;
		}
		else
		{
			return;
		}
	}
}

OBJ_DATA * check_node(const char *str, TempObjListType &cont)
{
	for (TempObjListType::reverse_iterator it = cont.rbegin(); it != cont.rend(); ++it)
	{
		if (isname(str, it->second->name))
		{
			return it->second;
		}
	}
	return 0;
}

OBJ_DATA * get_by_name(const char *str)
{
	if (!str || !*str) return 0;

	char buffer[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	strcpy(buffer, str);

	get_one_name(buffer, out, buffer);
	if (!*out) return 0;

	TempObjListType temp_list;
	fill_list_by_name(out, temp_list);
	if (temp_list.empty())
	{
		return 0;
	}
	return check_node(str, temp_list);
}

/**
* ����� ���� ��� ����� �������, � ������ ������ ��� �� ����� ���������� �����,
* � ������ ����� ����� ������� � ������ �������.
*/
OBJ_DATA * locate_object(const char *str)
{
	if (!str || !*str) return 0;

	char buffer[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
	strcpy(buffer, str);

	get_one_name(buffer, out, buffer);
	if (!*out) return 0;

	ObjListType::const_iterator i = obj_list.lower_bound(out);
	if (i != obj_list.end())
	{
		for (ObjNodeListType::const_iterator k = i->second.begin(); k != i->second.end(); ++k)
		{
			if (isname(str, (*k)->name))
			{
				return *k;
			}
		}
	}
	return 0;
}

} // namespace ObjectList
