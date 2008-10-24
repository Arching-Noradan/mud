// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo (original object uid and dupe check coded by Karachyn)
// Part of Bylins http://www.mud.ru

#include <map>
#include "obj_dupe.hpp"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "char.hpp"

namespace
{

int global_uid = 0;

typedef std::multimap<int /* ��� ������ */, OBJ_DATA *> DupeListType;
DupeListType dupe_list;

int count_dupes(OBJ_DATA *obj)
{
	int count = 0;

	DupeListType::iterator it = dupe_list.find(GET_OBJ_UID(obj));
	if (it != dupe_list.end())
	{
		do
		{
			if (GET_OBJ_VNUM(it->second) == GET_OBJ_VNUM(obj) && GET_OBJ_TIMER(it->second) > 0)
			{
				++count;
			}
			++it;
		}
		while (it != dupe_list.end() && it->first == GET_OBJ_UID(obj));
	}
	return count;
}

} // namespace

namespace ObjDupe
{

void load_global_uid()
{
	FILE *guid;
	char buffer[256];

	if (!(guid = fopen(LIB_MISC "globaluid", "r")))
	{
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
	return;
}

void save_global_uid()
{
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w")))
	{
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
	return;
}

// TODO: � �������� ����� �������� ���� � ��� ����� � � ������ ������ � ��� ��� ��� ����� ���� ��� ����.������
void add(OBJ_DATA *obj)
{
	dupe_list.insert(std::make_pair(GET_OBJ_UID(obj), obj));
}

void remove(OBJ_DATA *obj)
{
	DupeListType::iterator it = dupe_list.find(GET_OBJ_UID(obj));
	if (it != dupe_list.end())
	{
		do
		{
			if (it->second == obj)
			{
				dupe_list.erase(it);
				return;
			}
			++it;
		}
		while (it != dupe_list.end() && it->first == GET_OBJ_UID(obj));
	}
}

void check(CHAR_DATA *ch, OBJ_DATA *object)
{
	// �������� ������������ ���������
	if (object && // ������ ����������
			GET_OBJ_UID(object) != 0 && // ���� UID
			GET_OBJ_TIMER(object) > 0) // ���������
	{
		// ������ ����� ��� ��������. ���� � ���� ����� ��.
		int inworld = count_dupes(object);
		if (inworld > 1) // � ������� ���� ��� ������� ���� �����
		{
			sprintf(buf, "Copy detected and prepared to extract! Object %s (UID=%d, VNUM=%d), holder %s. In world %d.",
					object->PNames[0], GET_OBJ_UID(object), GET_OBJ_VNUM(object), GET_NAME(ch), inworld);
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
			// �������� ��������
			act("$o0 �������$G � �� ������� �������� ������������ ���� 'DUPE'.", FALSE, ch, object, 0, TO_CHAR);
			GET_OBJ_TIMER(object) = 0; // ���� ��������, ���������� �� ����
			SET_BIT(GET_OBJ_EXTRA(object, ITEM_NOSELL), ITEM_NOSELL); // ��� �����
			SET_BIT(GET_OBJ_EXTRA(object, ITEM_NORENT), ITEM_NORENT);
		}
	} // ��������� UID
	else if (GET_OBJ_VNUM(object) > 0 && // ������ �� �����������
			 GET_OBJ_UID(object) == 0)   // � ������� ����� ��� ����
	{
		++global_uid; // ����������� ���������� ������� �����
		global_uid = global_uid <= 0 ? 1 : global_uid; // ���� ��������� ������������ ����
		GET_OBJ_UID(object) = global_uid; // ��������� ���
		ObjDupe::add(object);
		log("%s obj_to_char %s #%d|%u", GET_NAME(ch), object->PNames[0], GET_OBJ_VNUM(object), object->uid);
	}
}

} // namespace ObjDupe
