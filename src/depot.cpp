// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#include <map>
#include <list>
#include <sstream>
#include <cmath>
#include <bitset>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include "depot.hpp"
#include "db.h"
#include "handler.h"
#include "utils.h"
#include "comm.h"
#include "auction.h"
#include "exchange.h"
#include "interpreter.h"
#include "screen.h"

extern SPECIAL(bank);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);

namespace Depot {

enum {PURGE_CHEST = 0, PERS_CHEST, SHARE_CHEST};
const int DEPOT_NUM = 3; // ���-�� �������� (��� �������) + 1 ��� ��������� ����� ������ ��� �������� �����
const int PERS_CHEST_VNUM = 331;
const int SHARE_CHEST_VNUM = 332;
// ����� ��������������� ��� ������������� � renumber_obj_rnum
int PERS_CHEST_RNUM = -1;
int SHARE_CHEST_RNUM = -1;

class OfflineNode
{
	public:
	OfflineNode() : vnum(0), timer(0), rent_cost(0), uid(0) {};
	int vnum; // ����
	int timer; // ������
	int rent_cost; // ���� ����� � ����
	int uid; // ���������� ���
};

// ��� ���������� ���������, �� ������������ ������ ������ ������������
// �������� �� �������, � ����������� ����� � ���� ������ ��������� ������, �������
// ������� ��� ����� ������ �������� �� ��� ������� � �� ������ ������ ���.
class AllowedChar
{
	public:
	AllowedChar(const std::string &name_, bool mutual_) : name(name_), mutual(mutual_) {};
	std::string name; // ��� ���� �� �����������
	bool mutual; // true - ���� �������� ������, false - ���� ��������� �������������
};

typedef std::list<OBJ_DATA *> ObjListType; // ���, ������
typedef std::vector<OfflineNode> TimerListType; // ������ �����, ������������ � ��������
typedef std::map<long, AllowedChar> AllowedCharType;  // ���, ���� ����

class CharNode
{
	public:
	CharNode() : ch(0), money(0), money_spend(0), buffer_cost(0), cost_per_day(0), state(0) {};
	// ������
	ObjListType pers_online; // ������ � ��������� ������
	ObjListType share_online; // ������ � �������� ������
	CHAR_DATA *ch; // ��� (������ �����)
    // �������
	TimerListType offline_list; // ������ � ����� ���������� �������
	long money; // ����� ����+���� �������
	long money_spend; // ������� ���� ��������� �� ����� ���� � �����
	// ����� ����
	float buffer_cost; // ������ ��� ������� ������ �����
	int cost_per_day; // ����� ��������� ����� ����� � ���� (�������� ������������ ��������� ������), ����� �� ����� ������� � �����������
	AllowedCharType allowed_chars; // ������ �����, ������� ��������� ��������� (��� ��������� �������������)
	std::vector<long> waiting_allowed_chars; // ������ ����� �����, ��� ��������� ����� ���������� ��� ����.�������
	std::string name; // ��� ����, ����� �� ������ ���� ����� ���� ��� ���������
	// ��������� ��� ���������� ������ (������ ������ ���� �� enum ����� ��������)
	// 0 - ����� �������� ��� ������ ��� ����� ���� � ����, 1 - ��������� ������ ������������ ������, 2 - ��������� ������ ����� ������
	std::bitset<DEPOT_NUM> state;

	ObjListType & get_depot_type(int type);
	void take_item(CHAR_DATA *ch, char *arg, int howmany, int type);
	void remove_item(CHAR_DATA *vict, ObjListType::iterator &obj_it, ObjListType &cont, int type);
	void update_online_item(ObjListType &cont, int type);
	void removal_period_cost();
	void load_online_objs(int file_type, bool reload = 0);
	void add_item(OBJ_DATA *obj, int type);
	void load_item(OBJ_DATA*obj, int type);
	void online_to_offline(ObjListType &cont, int file_type);
	bool any_other_share();
	bool obj_from_obj_list(char *name, int type, CHAR_DATA *vict);
	void reset();
};

typedef std::map<long, CharNode> DepotListType; // ��� ����, ����
DepotListType depot_list; // ������ ������ ��������

/**
* ������������� ������ ��������, ���� �� � ������.
* ���� ����� � ������� ����������� �� ���������.
*/
void init_depot()
{
	PERS_CHEST_RNUM = real_object(PERS_CHEST_VNUM);
	SHARE_CHEST_RNUM = real_object(SHARE_CHEST_VNUM);

	for (CHAR_DATA *ch = character_list; ch; ch = ch->next)
	{
		if (ch->nr > 0 && ch->nr <= top_of_mobt && mob_index[ch->nr].func == bank)
		{
			OBJ_DATA *pers_chest = read_object(PERS_CHEST_VNUM, VIRTUAL);
			OBJ_DATA *share_chest = read_object(SHARE_CHEST_VNUM, VIRTUAL);
			if (!pers_chest || !share_chest)
				return;
			obj_to_room(share_chest, ch->in_room);
			obj_to_room(pers_chest, ch->in_room);
		}
	}

	const char *depot_file = LIB_DEPOT"depot.db";
	std::ifstream file(depot_file);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while (file >> buffer)
	{
		if (buffer != "<Node>")
		{
			log("���������: ������ ������ <Node>");
			break;
		}
		CharNode tmp_node;
		long uid;
		// ����� ����
		if (!(file >> uid >> tmp_node.money >> tmp_node.money_spend >> tmp_node.buffer_cost))
		{
			log("���������: ������ ������ uid(%ld), money(%ld), money_spend(%ld), buffer_cost(%f).",
				uid, tmp_node.money, tmp_node.money_spend, tmp_node.buffer_cost);
			break;
		}
		// ������ ���������
		file >> buffer;
		if (buffer != "<Objects>")
		{
			log("���������: ������ ������ <Objects>.");
			break;
		}
		while (file >> buffer)
		{
			if (buffer == "</Objects>") break;

			OfflineNode tmp_obj;
			try
			{
				tmp_obj.vnum = boost::lexical_cast<int>(buffer);
			}
			catch(boost::bad_lexical_cast &)
			{
				log("���������: ������ ������ vnum (%s)", buffer.c_str());
				break;
			}
			if (!(file >> tmp_obj.timer >> tmp_obj.rent_cost >> tmp_obj.uid))
			{
				log("���������: ������ ������ timer(%d) rent_cost(%d) uid(%d) (obj vnum: %d).",
					tmp_obj.timer, tmp_obj.rent_cost, tmp_obj.uid, tmp_obj.vnum);
				break;
			}
			// ��������� ������������� ��������� ��������
			int rnum = real_object(tmp_obj.vnum);
			if (rnum >= 0)
			{
				obj_index[rnum].number++;
				tmp_node.cost_per_day += tmp_obj.rent_cost;
				tmp_node.offline_list.push_back(tmp_obj);
			}
		}
		if (buffer != "</Objects>")
		{
			log("���������: ������ ������ </Objects>.");
			break;
		}
		// ������ ���������� �����
		file >> buffer;
		if (buffer != "<Allowed>")
		{
			log("���������: ������ ������ <Allowed>.");
			break;
		}
		while (file >> buffer)
		{
			if (buffer == "</Allowed>") break;

			bool mutual;
			long allowed_uid;
			try
			{
				allowed_uid = boost::lexical_cast<long>(buffer);
			}
			catch(boost::bad_lexical_cast &)
			{
				log("���������: ������ ������ allowed_uid (%s)", buffer.c_str());
				break;
			}
			if (!(file >> mutual))
			{
				log("���������: ������ ������ mutual (allowed_uid: %ld).", allowed_uid);
				break;
			}
			// ������ ������ ����������� ���������
			std::string allowed_name = GetNameByUnique(allowed_uid);
			if (allowed_name.empty())
			{
				log("���������: UID %ld - ����������� ��������� �� ����������.", allowed_uid);
				continue;
			}
			name_convert(allowed_name);
			// ��� ��, ����� ���
			AllowedChar tmp_allowed(allowed_name, mutual);
			tmp_node.allowed_chars.insert(std::make_pair(allowed_uid, tmp_allowed));
		}
		if (buffer != "</Allowed>")
		{
			log("���������: ������ ������ </Allowed>.");
			break;
		}

		// �������� ���������� ������������
		file >> buffer;
		if (buffer != "</Node>")
		{
			log("���������: ������ ������ </Node>.");
			break;
		}

		// ��������� ���� �� ��� ����� ��� ������
		tmp_node.name = GetNameByUnique(uid);
		if (tmp_node.name.empty())
		{
			log("���������: UID %ld - ��������� �� ����������.", uid);
			continue;
		}
		name_convert(tmp_node.name);

		depot_list[uid] = tmp_node;
	}

}

/**
* ��������, �������� �� obj ������������ ��� ����� ����������.
* \return 0 - �� ��������, > 0 - ��� ��������� (�� enum).
*/
int is_depot(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (obj->item_number == PERS_CHEST_RNUM)
		return PERS_CHEST;
	if (obj->item_number == SHARE_CHEST_RNUM)
		return SHARE_CHEST;
	return 0;
}

/**
* � ������ ������������ ����� �� ����� - ������ ������� �����, ��������� ���������� ���� �� ����.
*/
void put_gold_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) != ITEM_MONEY) return;

	long gold = GET_OBJ_VAL(obj, 0);
	if ((GET_BANK_GOLD(ch) + gold) < 0)
	{
		long over = std::numeric_limits<long>::max() - GET_BANK_GOLD(ch);
		GET_BANK_GOLD(ch) += over;
		gold -= over;
		GET_GOLD(ch) += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "�� ������� ������� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
	}
	else
	{
		GET_BANK_GOLD(ch) += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "�� ������� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
	}
}

/**
* �������� ����������� �������� ������ � ���������.
* FIXME � ������� ��������.
*/
bool can_put_chest(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_OBJ_STAT(obj, ITEM_NODROP)
		|| OBJ_FLAGGED(obj, ITEM_ZONEDECAY)
		|| GET_OBJ_TYPE(obj) == ITEM_KEY
		|| IS_OBJ_STAT(obj, ITEM_NORENT)
		|| GET_OBJ_RENT(obj) < 0
		|| GET_OBJ_RNUM(obj) <= NOTHING)
	{
		send_to_char(ch, "��������� ���� �������� �������� %s � ���������.\r\n", OBJ_PAD(obj, 3));
		return 0;
	}
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
	{
		send_to_char(ch, "� %s ���-�� �����.\r\n", OBJ_PAD(obj, 5));
		return 0;
	}
	else if (!GET_BANK_GOLD(ch) && !GET_GOLD(ch))
	{
		send_to_char(ch, "� ��� ���� ������ ��� �����, ��� �� ����������� �������������� �� �������� �����?\r\n", OBJ_PAD(obj, 5));
		return 0;
	}
	return 1;
}

/**
* ������ ��� ���������� put_depot - ���������� ������ � ������ ������������� ��� ������ ���������.
* ����� ��������� ������ ���� ������ � ����� ���������. � ������������� ��� ���������� ��� �������� � �������.
* \param type - ����� � ����� ��� ��������� (�� enum).
*/
void CharNode::add_item(OBJ_DATA *obj, int type)
{
	if (type == PERS_CHEST)
		pers_online.push_front(obj);
	else
	{
		share_online.push_front(obj);
		cost_per_day += GET_OBJ_RENTEQ(obj);
	}
	state.set(type);
}

/**
* ������ add_item, ������ ��� � ��� ����� ������ ����������, ���� ���� ���� � ������������ ���������� ������.
* ������������ ��� ����� ������ ������ ���� ��� ����� � ����, ����� ��������� ������ ���� �����.
*/
void CharNode::load_item(OBJ_DATA*obj, int type)
{
	if (type == PERS_CHEST)
	{
		pers_online.push_front(obj);
		cost_per_day -= GET_OBJ_RENTEQ(obj);
	}
	else
		share_online.push_front(obj);
	state.set(type);
}

/**
* ����� ��������� ��������� ��� �������� ������, ����� �� ��������.
* \param ch - ����������� � ����������� �� ������� ���� ������, ����� ����� �������.
*/
DepotListType::iterator create_depot(long uid, CHAR_DATA *ch = 0)
{
	DepotListType::iterator it = depot_list.find(uid);
	if (it == depot_list.end())
	{
		CharNode tmp_node;
		if (ch)
		{
			tmp_node.ch = ch;
			tmp_node.name = GET_NAME(ch);
		}
		depot_list[uid] = tmp_node;
		it = depot_list.find(uid);
	}
	return it;
}

/**
* ������ ������ � ��������� (����� �������� �����), ������ ��������� �� ���� � �����.
*/
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj, int type)
{
	if (IS_NPC(ch)) return 0;
	if (IS_IMMORTAL(ch))
	{
		send_to_char("� ��� ��������� ����������...\r\n" , ch);
		return 1;
	}
	if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
	{
		put_gold_chest(ch, obj);
		return 1;
	}
	if (!can_put_chest(ch, obj)) return 0;

	DepotListType::iterator it = create_depot(GET_UNIQUE(ch), ch);
	it->second.add_item(obj, type);

	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "������������";
	else
		chest_name = "�����";
	sprintf(buf, "�� �������� $o3 � %s ���������.", chest_name.c_str());
	sprintf(buf1, "$n �������$g $o3 � %s ���������.", chest_name.c_str());
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	act(buf1, TRUE, ch, obj, 0, TO_ROOM);

	obj_from_char(obj);
	check_auction(NULL, obj);
	OBJ_DATA *temp;
	REMOVE_FROM_LIST(obj, object_list, next);

	return 1;
}

void print_obj(std::stringstream &out, OBJ_DATA *obj, int count)
{
	out << obj->short_description;
	if (count > 1)
		out << " [" << count << "]";
	out << " [" << GET_OBJ_RENTEQ(obj) << " " << desc_count(GET_OBJ_RENTEQ(obj), WHAT_MONEYa) << "]\r\n";
}

/**
* ��� ���������� show_depot - ����� ������ ��������� ���������.
* ���������� �� ������� � ����������� ���������� ���������.
*/
std::string print_obj_list(CHAR_DATA * ch, ObjListType &cont, const std::string &chest_name)
{
	int rent_per_day = 0;
	std::stringstream out;

	cont.sort(
		boost::bind(std::less<char *>(),
			boost::bind(&obj_data::name, _1),
			boost::bind(&obj_data::name, _2)));

	ObjListType::const_iterator prev_obj_it = cont.end();
	int count = 0;
	bool found = 0;
	for(ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		if (prev_obj_it == cont.end())
		{
			prev_obj_it = obj_it;
			count = 1;
		}
		else if (!equal_obj(*obj_it, *prev_obj_it))
		{
			print_obj(out, *prev_obj_it, count);
			prev_obj_it = obj_it;
			count = 1;
		}
		else
			count++;
		rent_per_day += GET_OBJ_RENTEQ(*obj_it);
		found = true;
	}
	if (prev_obj_it != cont.end() && count)
		print_obj(out, *prev_obj_it, count);
	if (!found)
		out << "� ������ ������ ��������� ��������� �����.\r\n";

	std::stringstream head;
	head << CCWHT(ch, C_NRM) << chest_name << " (����� �����: " << cont.size()
		<< ", ����� � ����: " << rent_per_day << " " << desc_count(rent_per_day, WHAT_MONEYa) << CCNRM(ch, C_NRM) << "\r\n";
	return (head.str() + out.str());
}

/**
* ������� ��������� ��������� ������ ��������� ����������.
* � ������ ������ ��������� ���� �� ���� ������������ ����������.
*/
void show_depot(CHAR_DATA * ch, OBJ_DATA * obj, int type)
{
	if (IS_NPC(ch)) return;
	if (IS_IMMORTAL(ch))
	{
		send_to_char("� ��� ��������� ����������...\r\n" , ch);
		return;
	}

	DepotListType::iterator it = create_depot(GET_UNIQUE(ch), ch);

	std::string out;
	if (type == PERS_CHEST)
		out = print_obj_list(ch, it->second.pers_online, "���� ������������ ���������");
	else
	{
		out = CCWHT(ch, C_NRM);
		out += "��� ������ � ����� ���������� �������� ������� '���������'.\r\n";
		out += CCNRM(ch, C_NRM);
		out += print_obj_list(ch, it->second.share_online, "���� ����� ���������");
		out += "\r\n";
		// ���� ����� ��������� ������ ���������� � ������� �� ����
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin(); al_it != it->second.allowed_chars.end(); ++al_it)
		{
			if (al_it->second.mutual)
			{
				DepotListType::iterator vict_it = depot_list.find(al_it->first);
				if (vict_it != depot_list.end())
				{
					out += print_obj_list(ch, vict_it->second.share_online, al_it->second.name);
					// ���� ��������� ��� �� ������������ - ���� ��� � �������, � �� ����� ������, ��� �� ��������
					if ((std::find(it->second.waiting_allowed_chars.begin(), it->second.waiting_allowed_chars.end(), al_it->first)
					!= it->second.waiting_allowed_chars.end()) && vict_it->second.share_online.empty())
						out += "��������� ����� �������� � ������� ���� �����.\r\n";
					else
						out += "\r\n";
				}
			}
		}
	}
	page_string(ch->desc, out.c_str(), 1);
}

/**
* ����� ������ � ���������� (�� ������� �������), ������� �� ��� ��.
* ����� ���������� ��������� ���� �� ������ �� ����������.
*/
bool CharNode::obj_from_obj_list(char *name, int type, CHAR_DATA *vict)
{
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	strcpy(tmp, name);

	ObjListType &cont = get_depot_type(type);

	int j = 0, number;
	if (!(number = get_number(&tmp)))
		return false;

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end() && (j <= number); ++obj_it)
	{
		if (isname(tmp, (*obj_it)->name) && ++j == number)
		{
			remove_item(vict, obj_it, cont, type);
			return true;
		}
	}

	if (type == SHARE_CHEST)
	{
		for (AllowedCharType::const_iterator al_it = allowed_chars.begin(); al_it != allowed_chars.end(); ++al_it)
		{
			if (al_it->second.mutual)
			{
				DepotListType::iterator vict_it = depot_list.find(al_it->first);
				if (vict_it != depot_list.end())
				{
					for (ObjListType::iterator vict_obj_it = vict_it->second.share_online.begin(); vict_obj_it != vict_it->second.share_online.end() && (j <= number); ++vict_obj_it)
					{
						if (isname(tmp, (*vict_obj_it)->name) && ++j == number)
						{
							remove_item(vict, vict_obj_it, vict_it->second.share_online, type);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/**
* ����� ������ �� ���������.
* \param vict - ��� ������� �� �� ���� ��������� �� ������, ���� ����� �� ������.
*/
void CharNode::remove_item(CHAR_DATA *vict, ObjListType::iterator &obj_it, ObjListType &cont, int type)
{
	(*obj_it)->next = object_list;
	object_list = *obj_it;
	obj_to_char(*obj_it, vict);

	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "�������������";
	else
		chest_name = "������";
	sprintf(buf, "�� ����� $o3 �� %s ���������.", chest_name.c_str());
	sprintf(buf1, "$n ����$g $o3 �� %s ���������.", chest_name.c_str());
	act(buf, FALSE, vict, *obj_it, 0, TO_CHAR);
	act(buf1, TRUE, vict, *obj_it, 0, TO_ROOM);

	if (type != PERS_CHEST)
		cost_per_day -= GET_OBJ_RENTEQ(*obj_it);
	cont.erase(obj_it++);
	state.set(type);
}

/**
* ����� �� �������� ������ � ��������� ���������� ������ ���� � �������.
*/
ObjListType & CharNode::get_depot_type(int type)
{
	if (type == PERS_CHEST)
		return pers_online;
	else
		return share_online;
}

/**
* ����� ������ � ��������� � �� ������, � ��� ����� � �� ����� ��������.
*/
void CharNode::take_item(CHAR_DATA *vict, char *arg, int howmany, int type)
{
	ObjListType &cont = get_depot_type(type);

	int obj_dotmode = find_all_dots(arg);
	if (obj_dotmode == FIND_INDIV)
	{
		bool result = obj_from_obj_list(arg, type, vict);
		if (!result)
		{
			send_to_char(ch, "�� �� ������ '%s' � ���������.\r\n", arg);
			return;
		}
		while (result && --howmany)
			result = obj_from_obj_list(arg, type, vict);
	}
	else
	{
		if (obj_dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("����� ��� \"���\" ?\r\n", ch);
			return;
		}
		bool found = 0;
		for (ObjListType::iterator obj_list_it = cont.begin(); obj_list_it != cont.end(); )
		{
			if (obj_dotmode == FIND_ALL || isname(arg, (*obj_list_it)->name))
			{
				found = 1;
				remove_item(vict, obj_list_it, cont, type);
			}
			else
				++obj_list_it;
		}
		// � ������ ������ ��������� ���� �� ���� ����������� (FIXME ���� ����-���� � obj_from_obj_list � ������ �����������)
		if (type == SHARE_CHEST)
		{
			for (AllowedCharType::const_iterator al_it = allowed_chars.begin(); al_it != allowed_chars.end(); ++al_it)
			{
				if (al_it->second.mutual)
				{
					DepotListType::iterator vict_it = depot_list.find(al_it->first);
					if (vict_it != depot_list.end())
					{
						for (ObjListType::iterator vict_obj_it = vict_it->second.share_online.begin(); vict_obj_it != vict_it->second.share_online.end(); )
						{
							if (obj_dotmode == FIND_ALL || isname(arg, (*vict_obj_it)->name))
							{
								found = 1;
								remove_item(vict, vict_obj_it, vict_it->second.share_online, type);
							}
							else
								++vict_obj_it;
						}
					}
				}
			}
		}

		if (!found)
		{
			send_to_char(ch, "�� �� ������ ������ �������� �� '%s' � ���������.\r\n", arg);
			return;
		}
	}
}

/**
* ������ ����-�� �� ��������.
*/
void take_depot(CHAR_DATA *vict, char *arg, int howmany, int type)
{
	if (IS_NPC(vict)) return;
	if (IS_IMMORTAL(vict))
	{
		send_to_char("� ��� ��������� ����������...\r\n" , vict);
		return;
	}

	DepotListType::iterator it = depot_list.find(GET_UNIQUE(vict));
	if (it == depot_list.end())
	{
		send_to_char("� ������ ������ ���� ��������� ��������� �����.\r\n", vict);
		return;
	}

	if (type == PERS_CHEST)
		it->second.take_item(vict, arg, howmany, PERS_CHEST);
	else
		it->second.take_item(vict, arg, howmany, SHARE_CHEST);
}

/**
* ������ �������� � ������ ������� � ����������� � �����, ���� ��� ������.
*/
void CharNode::update_online_item(ObjListType &cont, int type)
{
	std::string chest_name;
	if (type == PERS_CHEST)
		chest_name = "������������";
	else
		chest_name = "�����";

	for (ObjListType::iterator obj_it = cont.begin(); obj_it != cont.end(); )
	{
		GET_OBJ_TIMER(*obj_it)--;
		if (GET_OBJ_TIMER(*obj_it) <= 0)
		{
			if (ch)
				send_to_char(ch, "[%s ���������]: %s'%s ��������%s � ����'%s\r\n",
					chest_name.c_str(), CCIRED(ch, C_NRM), (*obj_it)->short_description, GET_OBJ_SUF_2((*obj_it)), CCNRM(ch, C_NRM));
			if (type != PERS_CHEST)
				cost_per_day -= GET_OBJ_RENTEQ(*obj_it);
			extract_obj(*obj_it);
			cont.erase(obj_it++);
			state.set(type);
		}
		else
			++obj_it;
	}
}

/**
* �������� ���� ������� ����� � ����� ����� � �������.
*/
void CharNode::reset()
{
	for (ObjListType::iterator obj_it = pers_online.begin(); obj_it != pers_online.end(); ++obj_it)
		extract_obj(*obj_it);
	for (ObjListType::iterator obj_it = share_online.begin(); obj_it != share_online.end(); ++obj_it)
		extract_obj(*obj_it);
	pers_online.clear();
	share_online.clear();
	offline_list.clear();
	cost_per_day = 0;
	buffer_cost = 0;
	money = 0;
	money_spend = 0;
	state.reset();
}

/**
* ������ ����� �� ������ � ������� � �������� � ��������� ����� ��� �������� �����.
*/
void CharNode::removal_period_cost()
{
	float i;
	buffer_cost += (static_cast<float>(cost_per_day) / SECS_PER_MUD_DAY);
	modff(buffer_cost, &i);
	if (i)
	{
		long diff = static_cast<long> (i);
		if (ch)
			GET_BANK_GOLD(ch) -= diff;
		else
		{
			money -= diff;
			money_spend += diff;
		}
		buffer_cost -= i;
	}

	// ������/�������
	if (ch)
	{
		if (GET_BANK_GOLD(ch) < 0)
		{
			GET_GOLD(ch) += GET_BANK_GOLD(ch);
			GET_BANK_GOLD(ch) = 0;
			if (GET_GOLD(ch) < 0)
			{
				GET_GOLD(ch) = 0;
				reset();
				state.set(0);
			}
		}
	}
	else
	{
		if (money < 0)
		{
			reset();
			state.set(0);
		}
	}
}

/**
* ������ �������� �� ���� �������, �������� ��� ������� �������� (� �����������), �������� ������ ��� ������ ���������.
*/
void update_timers()
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); )
	{
		// ���������� ������
		if (!it->second.pers_online.empty())
			it->second.update_online_item(it->second.pers_online, PERS_CHEST);
		if (!it->second.share_online.empty())
			it->second.update_online_item(it->second.share_online, SHARE_CHEST);
		// ����������� ������
		for (TimerListType::iterator obj_it = it->second.offline_list.begin(); obj_it != it->second.offline_list.end(); )
		{
			--(obj_it->timer);
			if (obj_it->timer < 0)
			{
				// ������ ������ � ����
				int rnum = real_object(obj_it->vnum);
				if (rnum >= 0)
				obj_index[rnum].number--;

				it->second.cost_per_day -= obj_it->rent_cost;
				it->second.offline_list.erase(obj_it++);
			}
			else
				++obj_it;
		}
		// ������ ����� � ���� �����, ���� ����� ��� �� �������
		if (it->second.cost_per_day)
			it->second.removal_period_cost();
		// ������ ��������� ����� �������� �������, �.�. ��� ��� �������� � ������ ������ ������ �� ����
		if (it->second.offline_list.empty()
		&& it->second.allowed_chars.empty()
		&& it->second.state.none()
		&& it->second.pers_online.empty()
		&& it->second.share_online.empty())
			depot_list.erase(it++);
		else
			++it;
	}
}

const char *HELP_FORMAT =
	"������: ��������� - ������� �� ������� � ������ ���� � ��������� ������� �������������.\r\n"
	"        ��������� �������� <���> - �������� ��������� � ������ �� ����������� ��������.\r\n"
	"        ��������� ������� <���> - ������� ��������� �� ����� ������.\r\n\r\n"
	"������� �� ������� ��������:\r\n"
	"��� ������������� ������� ���������� ���������� � ����� (�����).\r\n"
	"������������ ��������� - ���� ������ ���������, ������� ���������� ���� ����� ������.\r\n"
	"������� ��� �������� ����� - ���������, ������� ����� ������������ ������������ ��������� ����������,\r\n"
	"������ ������� ������� �������� '���������'. ����������� ������ ��� �������� �������� � ������.\r\n"
	"������ �� �����:\r\n"
	"- �� �������� � ����� ������������ ��������� ����� ��������� ������ ����� �� �� � ����.\r\n"
	"- �� �������� � �������� ����� ��������� ��������� (������ �� ���� ����� ���������).\r\n"
	"- ����� ��������� ������� ������ ������ ��������� ����� � ������ ����� ��� � ����� �� �����, ���� ���� ������.\r\n"
	"- ��� ������ ����� ��� �������� � ����� ���������� ������������ (� ����� ��������� �������� ������ ����� �����).\r\n"
	"���������� �������� � ������ ����� ���������� ������ �� ����� ������ ������� (�� ������ ����� ���������).\r\n";

/**
* ������� '���������' ��� ���������� � �����.
*/
SPECIAL(Special)
{
	if (IS_NPC(ch) || !CMD_IS("���������")) return false;
	if (IS_IMMORTAL(ch))
	{
		send_to_char("� ��� ��������� ����������...\r\n" , ch);
		return true;
	}

	DepotListType::iterator ch_it = create_depot(GET_UNIQUE(ch), ch);

	std::string buffer = argument, command;
	GetOneParam(buffer, command);

	if (CompareParam(command, "��������"))
	{
		std::string name;
		GetOneParam(buffer, name);
		long uid = GetUniqueByName(name);
		if (uid < 0)
			send_to_char("������ ��������� �� ����������.\r\n", ch);
		else if (uid == GET_UNIQUE(ch))
			send_to_char("�������� �������� �����...\r\n", ch);
		else
		{
			AllowedCharType::const_iterator tmp_vict = ch_it->second.allowed_chars.find(uid);
			if (tmp_vict != ch_it->second.allowed_chars.end())
			{
				send_to_char("�� ��� � ��� �������� � ��� ������������� ������.\r\n", ch);
				return true;
			}

			// ��������� ��� ����������. ������ ������� ������� ������������ ���� �� ������� �������� ������
			// ���� ��� ���� - �� ������� ��� ���� �������� ������
			bool mutual = false;
			DepotListType::iterator vict_it = depot_list.find(uid);
			if (vict_it != depot_list.end())
			{
				AllowedCharType::iterator al_it = vict_it->second.allowed_chars.find(GET_UNIQUE(ch));
				if (al_it != vict_it->second.allowed_chars.end())
					al_it->second.mutual = mutual = true;
			}
			// � ����� ��� ����� ��� � ���� � ���������, ���� ��� ��� �������� ������
			name_convert(name);
			AllowedChar tmp_char(name, mutual);
			ch_it->second.allowed_chars.insert(std::make_pair(uid, tmp_char));

			send_to_char(ch, "�������� �������� � ������������� ������.\r\n");
			if (mutual)
			{
				ch_it->second.waiting_allowed_chars.push_back(uid);
				send_to_char(ch, "������� �������� ������, ��������� ����� ���������� � ������� ���� �����.\r\n");
			}
			else
				send_to_char(ch, "�������� ������ �� �������, ��������� ������������� ���������� ���������.\r\n");
		}
		return true;
	}
	else if (CompareParam(command, "�������"))
	{
		std::string name;
		GetOneParam(buffer, name);
		name_convert(name);
		long uid = -1;
		// ������� ��������� �� ������ �������������� ������
		for (AllowedCharType::iterator al_it = ch_it->second.allowed_chars.begin(); al_it != ch_it->second.allowed_chars.end(); ++al_it)
		{
			if (al_it->second.name == name)
			{
				uid = al_it->first;
				ch_it->second.allowed_chars.erase(al_it);
				break;
			}
		}
		// ������� � ���� ���� �������� ������ �� ���, ���� ������� ����
		if (uid != -1)
		{
			DepotListType::iterator vict_it = depot_list.find(uid);
			if (vict_it != depot_list.end())
			{
				AllowedCharType::iterator al_it = vict_it->second.allowed_chars.find(GET_UNIQUE(ch));
				if (al_it != vict_it->second.allowed_chars.end())
					al_it->second.mutual = false;
				// ���� �� ��� ��������� ������ ��� ����������� ����� ������ - ��������� ��� � �������
				if (!vict_it->second.any_other_share())
					vict_it->second.online_to_offline(vict_it->second.share_online, SHARE_DEPOT_FILE);
			}
			send_to_char(ch, "�������� ������ �� ������ �������������� ������.\r\n");
		}
		else
			send_to_char(ch, "��������� �������� �� ������.\r\n");
	}
	else
	{
		std::string out = "������ ���������� �� ����������� ��������:\r\n";
		for (AllowedCharType::const_iterator al_it = ch_it->second.allowed_chars.begin(); al_it != ch_it->second.allowed_chars.end(); ++al_it)
		{
			out += al_it->second.name + " ";
			if (al_it->second.mutual)
			{
				if (std::find(ch_it->second.waiting_allowed_chars.begin(), ch_it->second.waiting_allowed_chars.end(), al_it->first)
				!= ch_it->second.waiting_allowed_chars.end())
					out += "(�����������, ����������� ���������� � ������� ���� �����)\r\n";
				else
					out += "(��������� ����������)\r\n";
			}
			else
				out += "(��������� �������������)\r\n";
		}
		if (ch_it->second.allowed_chars.empty())
			out += "� ������ ������ ��� �������\r\n";
		out += "\r\n";
		out += HELP_FORMAT;
		send_to_char(ch, out.c_str());
	}
	return true;
}

/**
* ������ ����� �� ������ (������ ������ ������).
*/
void write_obj_file(const std::string &name, int file_type, const ObjListType &cont)
{
	char filename[MAX_STRING_LENGTH];
	if (!get_filename(name.c_str(), filename, file_type))
	{
		log("���������: �� ������� ������������� ��� ����� (name: %s, filename: %s) (%s %s %d).", name.c_str(), filename, __FILE__, __func__, __LINE__);
		return;
	}
	// ��� ������ ������ ������ ������� ����, ����� �� ������� �������� � ����
	if (cont.empty())
	{
		remove(filename);
		return;
	}

	std::ofstream file(filename);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", filename, __FILE__, __func__, __LINE__);
		return;
	}
	file << "* Items file\n";
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		char databuf[MAX_STRING_LENGTH];
		char *data = databuf;
		write_one_object(&data, *obj_it, 0);
		file << databuf;
	}
	file << "\n$\n$\n";
	file.close();
}

/**
* ���������� ���������� ������� � �����.
*/
void save_online_objs()
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		// ��� ����� ���� � ������� � ������ ������������ ������ ���������
		if (it->second.state[PERS_CHEST])
		{
			write_obj_file(it->second.name, PERS_DEPOT_FILE, it->second.pers_online);
			it->second.state.reset(PERS_CHEST);
		}
		if (it->second.state[SHARE_CHEST])
		{
			write_obj_file(it->second.name, SHARE_DEPOT_FILE, it->second.share_online);
			it->second.state.reset(SHARE_CHEST);
		}
	}
}

/**
* ��� �������� � save_timedata(), ������ ������ � ����������� ������.
*/
void write_objlist_timedata(const ObjListType &cont, std::ofstream &file)
{
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
		file << GET_OBJ_VNUM(*obj_it) << " " << GET_OBJ_TIMER(*obj_it) << " " << GET_OBJ_RENTEQ(*obj_it) << " " << GET_OBJ_UID(*obj_it) << "\n";
}

/**
* ���������� ���� ������� � ����� ���� � ����-����� ������.
*/
void save_timedata()
{
	const char *depot_file = LIB_DEPOT"depot.db";
	std::ofstream file(depot_file);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", depot_file, __FILE__, __func__, __LINE__);
		return;
	}

	for (DepotListType::const_iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		file << "<Node>\n" << it->first << " ";
		// ��� ������ - ����� ����� � ��� �����, ����� ����� �� ����������� ����� ����
		if (it->second.ch)
			file << GET_BANK_GOLD(it->second.ch) + GET_GOLD(it->second.ch) << " 0 ";
		else
			file << it->second.money << " " << it->second.money_spend << " ";
		file << it->second.buffer_cost << "\n";

		// ������ ����� ������ �� ������ - �� � �����, ����� �� ���������
		file << "<Objects>\n";
		write_objlist_timedata(it->second.pers_online, file);
		write_objlist_timedata(it->second.share_online, file);
		for (TimerListType::const_iterator obj_it = it->second.offline_list.begin(); obj_it != it->second.offline_list.end(); ++obj_it)
			file << obj_it->vnum << " " << obj_it->timer << " " << obj_it->rent_cost << " " << obj_it->uid << "\n";
		file << "</Objects>\n";

		file << "<Allowed>\n";
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin(); al_it != it->second.allowed_chars.end(); ++al_it)
			file << al_it->first << " " << al_it->second.mutual << "\n";
		file << "</Allowed>\n";
		file << "</Node>\n";
	}
}

/**
* ���� ������ ������ � ������ ������.
* FIXME ����-���� ������ ��������� � ���� ����.
* \param reload - ��� ������� ������ ����, �� ��������� ���� (���������� ��������� �� � ������� ������� ��� ����� ��� ���).
*/
void CharNode::load_online_objs(int file_type, bool reload)
{
	if (offline_list.empty()) return;

	char filename[MAX_STRING_LENGTH];
	get_filename(name.c_str(), filename, file_type);

	FILE *fl = fopen(filename, "r+b");
	if (!fl) return;

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize)
	{
		fclose(fl);
		log("���������: ������ ���� ��������� (%s).", filename);
		return;
	}
	char *databuf = new char [fsize + 1];

	fseek(fl, 0L, SEEK_SET);
	if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf)
	{
		fclose(fl);
		log("���������: ������ ������ ����� ��������� (%s).", filename);
		return;
	}
	fclose(fl);

	char *data = databuf;
	*(data + fsize) = '\0';
	int error = 0;
	OBJ_DATA *obj;

	for (fsize = 0; *data && *data != '$'; fsize++)
	{
		if (!(obj = read_one_object_new(&data, &error)))
		{
			if (error)
				log("���������: ������ ������ �������� (%s, error: %d).", filename, error);
			continue;
		}
		if (!reload)
		{
			// ������ �������� �� ������� �������� � ��������� ������������ ������
			TimerListType::iterator obj_it = std::find_if(offline_list.begin(), offline_list.end(),
				boost::bind(std::equal_to<long>(),
					boost::bind(&OfflineNode::uid, _1), GET_OBJ_UID(obj)));
			if (obj_it != offline_list.end() && obj_it->vnum == GET_OBJ_VNUM(obj))
			{
				GET_OBJ_TIMER(obj) = obj_it->timer;
				// ������� �� ������ ���������� ����, �.�. ������ �������� ����� ������ - ��� ���� ��� ��������� ������ ������ ���������
				offline_list.erase(obj_it);
			}
			else
			{
				extract_obj(obj);
				continue;
			}
		}
		// ��� ������� �� ������ �� �������, � ������ ���, ��� ����
		if (file_type == PERS_DEPOT_FILE)
			load_item(obj, PERS_CHEST);
		else
			load_item(obj, SHARE_CHEST);
		// ������� ��� �� ����������� �����, � ������� �� ��������� ��� �� ������ ������ �� �����
		OBJ_DATA *temp;
		REMOVE_FROM_LIST(obj, object_list, next);
	}
	delete [] databuf;
}

/**
* ���� ���� � ���� - ������ �� ���������, ���� ������, ���������� � ���-�� ������ �����,
* ������� ������� � ������, ��������� ����� ��������.
*/
void enter_char(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		// ���������� � ���� �������� ����� - �������� ���
		if (it->second.state[PURGE_CHEST])
		{
			it->second.reset();
			// ��� ������ ������� - ������ �����
			write_obj_file(it->second.name, PERS_DEPOT_FILE, it->second.pers_online);
			write_obj_file(it->second.name, SHARE_DEPOT_FILE, it->second.share_online);
			send_to_char(ch, "%s���������: � ��� �� ������� ����� �� ������ ����� � ��� ��� ���� ���� �������!%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			GET_BANK_GOLD(ch) = 0;
			GET_GOLD(ch) = 0;
		}
		// ������� �����, ���� ���-�� ���� ��������� �� �����
		if (it->second.money_spend > 0)
		{
			GET_BANK_GOLD(ch) -= it->second.money_spend;
			if (GET_BANK_GOLD(ch) < 0)
			{
				GET_GOLD(ch) += GET_BANK_GOLD(ch);
				GET_BANK_GOLD(ch) = 0;
				if (GET_GOLD(ch) < 0)
					GET_GOLD(ch) = 0;
			}

			send_to_char(ch, "%s���������: �� ����� ������ ���������� �������� %d %s.%s\r\n\r\n",
				CCWHT(ch, C_NRM), it->second.money_spend, desc_count(it->second.money_spend, WHAT_MONEYa), CCNRM(ch, C_NRM));

		}
		// ������ ��� ��������� � ���������� �� �� ����, ����� �������� ��������� �� ������� �������
		it->second.load_online_objs(PERS_DEPOT_FILE);
		it->second.load_online_objs(SHARE_DEPOT_FILE);
		it->second.state.set(PERS_CHEST);
		it->second.state.set(SHARE_CHEST);
		// ��������� ����������� ����� � ������������ ch ��� ������ ����� ������
		it->second.money = 0;
		it->second.money_spend = 0;
		it->second.offline_list.clear();
		it->second.ch = ch;
		// ������ �� ��������� ����������� �������� (��� �� �� ����.��������)
		for (AllowedCharType::const_iterator al_it = it->second.allowed_chars.begin(); al_it != it->second.allowed_chars.end(); ++al_it)
			if (al_it->second.mutual)
				it->second.waiting_allowed_chars.push_back(al_it->first);
		// ���� ��������� ��� ���� ���������� - ��� ������, ������ ������ �� ����������
		// ������� ��� ����� ��������� �� ���� �� ������� �� ��������� ������ �� ���������
	}
}

/**
* ������� ������ ������ � �������.
*/
void CharNode::online_to_offline(ObjListType &cont, int file_type)
{
	write_obj_file(name, file_type, cont);
	for (ObjListType::const_iterator obj_it = cont.begin(); obj_it != cont.end(); ++obj_it)
	{
		OfflineNode tmp_obj;
		tmp_obj.vnum = GET_OBJ_VNUM(*obj_it);
		tmp_obj.timer = GET_OBJ_TIMER(*obj_it);
		tmp_obj.rent_cost = GET_OBJ_RENTEQ(*obj_it);
		tmp_obj.uid = GET_OBJ_UID(*obj_it);
		offline_list.push_back(tmp_obj);
		extract_obj(*obj_it);
		// �� ����.� ���� �� ������ �� ������
		int rnum = real_object(tmp_obj.vnum);
		if (rnum >= 0)
			obj_index[rnum].number++;
	}
	cont.clear();
}

/**
* ��������, ��������� �� ����-���� ��� ���������.
*/
bool CharNode::any_other_share()
{
	for (AllowedCharType::const_iterator al_it = allowed_chars.begin(); al_it != allowed_chars.end(); ++al_it)
	{
		if (al_it->second.mutual)
		{
			DESCRIPTOR_DATA *d = DescByUID(al_it->first);
			if (d)
				return true;
		}
	}
	return false;
}

/**
* ����� ���� �� ���� - ������� �������� � �������.
*/
void exit_char(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		// ������������ ��������� � ����� ������ ������ � �������
		if (!it->second.pers_online.empty())
			it->second.online_to_offline(it->second.pers_online, PERS_DEPOT_FILE);
		// ����� ��������� ������ ������ ���� � ������� �� �������� ������ �� ���������� ����� � �������� �������
		if (!it->second.share_online.empty() && !it->second.any_other_share())
			it->second.online_to_offline(it->second.share_online, SHARE_DEPOT_FILE);
		it->second.ch = 0;
		it->second.money = GET_BANK_GOLD(ch) + GET_GOLD(ch);
		it->second.money_spend = 0;
	}
}

/**
* ����� �� ���� � ����, �� � ������ ������������� ��������� ������, ��� ������ ����� ������.
* ��� ������� ������� �� ����, ��� ���������� ������������� ����� � ������� ������.
*/
int get_cost_per_day(CHAR_DATA *ch)
{
	DepotListType::iterator it = depot_list.find(GET_UNIQUE(ch));
	if (it != depot_list.end())
	{
		int cost_per_day = 0;
		if (!it->second.pers_online.empty())
			for (ObjListType::const_iterator obj_it = it->second.pers_online.begin(); obj_it != it->second.pers_online.end(); ++obj_it)
				cost_per_day += GET_OBJ_RENTEQ(*obj_it);
		cost_per_day += it->second.cost_per_day;
		return cost_per_day;
	}
	return 0;
}

/**
* ��������� ����� ��������, ��������� �����������.
*/
void load_share_depots()
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		if (!it->second.waiting_allowed_chars.empty())
		{
			for(std::vector<long>::iterator w_it = it->second.waiting_allowed_chars.begin(); w_it != it->second.waiting_allowed_chars.end(); ++w_it)
			{
				// ������� ���� ����, ��� ��������� ��� ����� � ������ ������ ���� ��� �� ���� ��� ���������
				DepotListType::iterator vict_it = depot_list.find(*w_it);
				if (vict_it != depot_list.end() && vict_it->second.share_online.empty())
					vict_it->second.load_online_objs(SHARE_DEPOT_FILE);
			}
			it->second.waiting_allowed_chars.clear();
		}
	}
}

/**
* ��������� ������� ���� - ����� �� �������� � ����� ���������� ����� �����.
*/
void rename_char(long uid)
{
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		AllowedCharType::iterator al_it = it->second.allowed_chars.find(uid);
		if (al_it != it->second.allowed_chars.end())
		{
			std::string name = GetNameByUnique(uid);
			if (!name.empty())
				al_it->second.name = name;
			return;
		}
	}
}

/**
* ������ ������ ���� �� ������ ������.
*/
void reload_char(long uid)
{
	DepotListType::iterator it = create_depot(uid);
	// ������� �������� ���, ��� ����
	it->second.reset();
	// ����� ������ ����
	it->second.load_online_objs(PERS_DEPOT_FILE, 1);
	it->second.load_online_objs(SHARE_DEPOT_FILE, 1);
}

/**
* ����� ���� � show stats.
*/
void show_stats(CHAR_DATA *ch)
{
	std::stringstream out;
	int pers_count = 0, share_count = 0, offline_count = 0;
	for (DepotListType::iterator it = depot_list.begin(); it != depot_list.end(); ++it)
	{
		pers_count += it->second.pers_online.size();
		share_count += it->second.share_online.size();
		offline_count += it->second.offline_list.size();
	}

	out << "  ����� ��������: " << depot_list.size() << "\r\n"
		<< "  ��������� � ������������ ����������: " << pers_count << "\r\n"
		<< "  ��������� � ����� ����������: " << share_count << "\r\n"
		<< "  ��������� � ��������: " << offline_count << "\r\n";
	send_to_char(out.str().c_str(), ch);
}

} // namespace Depot
