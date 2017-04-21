// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "shop_ext.hpp"

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "char.hpp"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "constants.h"
#include "dg_scripts.h"
#include "room.hpp"
#include "glory.hpp"
#include "glory_const.hpp"
#include "named_stuff.hpp"
#include "screen.h"
#include "house.h"
#include "modify.h"
#include "liquid.hpp"
#include "logger.hpp"
#include "utils.h"
#include "parse.hpp"
#include "pugixml.hpp"
#include "pk.h"
#include "ext_money.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <vector>
#include <string>
#include <unordered_map>

/*
������ ������� (sample configuration) (plrstuff/shop/test.xml):
<?xml version="1.0"?>
<shop_item_sets>
	<shop_item_set id="GEMS">
	        <item vnum="909" price="100" />
        	<item vnum="910" price="100" />
	        <item vnum="911" price="100" />
        	<item vnum="912" price="100" />
	        <item vnum="913" price="100" />
        	<item vnum="914" price="100" />
	        <item vnum="915" price="100" />
	        <item vnum="916" price="100" />
        	<item vnum="917" price="100" />
	</shop_item_set>
</shop_item_sets>

<shop_list>
    <shop currency="����" id="GEMS_SHOP" profit="40" waste_time_min="0">
	<mob mob_vnum="4010" />
	<mob mob_vnum="4015" />
	<item_set>GEMS</item_set>
        <item vnum="4001" price="500" />
    </shop>
    <shop currency="����" id="BANDAGE_SHOP" profit="60" waste_time_min="15">
	<mob mob_vnum="4018"/>
	<mob mob_vnum="4019"/>
        <item vnum="1913" price="500" />
       	<item vnum="1914" price="1000" />
        <item vnum="1915" price="2000" />
       	<item vnum="1916" price="4000" />
        <item vnum="1917" price="8000" />
       	<item vnum="1918" price="16000" />
        <item vnum="1919" price="25" />
    </shop>
</shop_list>
*/

extern void do_echo(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
extern int do_social(CHAR_DATA * ch, char *argument);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_align(CHAR_DATA * ch, const OBJ_DATA * obj);
extern char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);
extern char *diag_timer_to_char(const OBJ_DATA * obj);

namespace ShopExt
{
long get_sell_price(OBJ_DATA * obj);

const int IDENTIFY_COST = 110;
int spent_today = 0;
const char *MSG_NO_STEAL_HERE = "$n, �����$w �������, ���� ��������!";
EWearFlag wear = EWearFlag::ITEM_WEAR_UNDEFINED;
int type = -10;

struct item_desc_node
{
	std::string name;
	std::string description;
	std::string short_description;
	boost::array<std::string, 6> PNames;
	ESex sex;
	CObjectPrototype::triggers_list_t trigs;
};

typedef std::map<int/*vnum ��������*/, item_desc_node> ObjDescType;
std::map<std::string/*id �������*/, ObjDescType> item_descriptions;

bool check_money(CHAR_DATA *ch, long price, const std::string& currency)
{
	if (currency == "�����")
	{
		const auto total_glory = Glory::get_glory(GET_UNIQUE(ch)) + GloryConst::get_glory(GET_UNIQUE(ch));
		return total_glory >= price;
	}

	if (currency == "����")
	{
		return ch->get_gold() >= price;
	}

	if (currency == "���")
	{
		return ch->get_ice_currency() >= price;
	}

	return false;
}

bool init_type(const char *str)
{
	if (is_abbrev(str, "����")
		|| is_abbrev(str, "light"))
	{
		type = OBJ_DATA::ITEM_LIGHT;
	}
	else if (is_abbrev(str, "������")
		|| is_abbrev(str, "scroll"))
	{
		type = OBJ_DATA::ITEM_SCROLL;
	}
	else if (is_abbrev(str, "�������")
		|| is_abbrev(str, "wand"))
	{
		type = OBJ_DATA::ITEM_WAND;
	}
	else if (is_abbrev(str, "�����")
		|| is_abbrev(str, "staff"))
	{
		type = OBJ_DATA::ITEM_STAFF;
	}
	else if (is_abbrev(str, "������")
		|| is_abbrev(str, "weapon"))
	{
		type = OBJ_DATA::ITEM_WEAPON;
	}
	else if (is_abbrev(str, "�����")
		|| is_abbrev(str, "armor"))
	{
		type = OBJ_DATA::ITEM_ARMOR;
	}
	else if (is_abbrev(str, "��������")
		|| is_abbrev(str, "material"))
	{
		type = OBJ_DATA::ITEM_MATERIAL;
	}
	else if (is_abbrev(str, "�������")
		|| is_abbrev(str, "potion"))
	{
		type = OBJ_DATA::ITEM_POTION;
	}
	else if (is_abbrev(str, "������")
		|| is_abbrev(str, "������")
		|| is_abbrev(str, "other"))
	{
		type = OBJ_DATA::ITEM_OTHER;
	}
	else if (is_abbrev(str, "���������")
		|| is_abbrev(str, "container"))
	{
		type = OBJ_DATA::ITEM_CONTAINER;
	}
	else if (is_abbrev(str, "�������")
		|| is_abbrev(str, "tank"))
	{
		type = OBJ_DATA::ITEM_DRINKCON;
	}
	else if (is_abbrev(str, "�����")
		|| is_abbrev(str, "book"))
	{
		type = OBJ_DATA::ITEM_BOOK;
	}
	else if (is_abbrev(str, "����")
		|| is_abbrev(str, "rune"))
	{
		type = OBJ_DATA::ITEM_INGREDIENT;
	}
	else if (is_abbrev(str, "����������")
		|| is_abbrev(str, "ingradient"))
	{
		type = OBJ_DATA::ITEM_MING;
	}
	else if (is_abbrev(str, "������")
		|| is_abbrev(str, "������"))
	{
		type = OBJ_DATA::ITEM_ARMOR_LIGHT;
	}
	else if (is_abbrev(str, "�������")
		|| is_abbrev(str, "�������"))
	{
		type = OBJ_DATA::ITEM_ARMOR_MEDIAN;
	}
	else if (is_abbrev(str, "�������")
		|| is_abbrev(str, "�������"))
	{
		type = OBJ_DATA::ITEM_ARMOR_HEAVY;
	}
	else
	{
		return false;
	}

	return true;
}

bool init_wear(const char *str)
{
	if (is_abbrev(str, "�����"))
	{
		wear = EWearFlag::ITEM_WEAR_FINGER;
	}
	else if (is_abbrev(str, "���") || is_abbrev(str, "�����"))
	{
		wear = EWearFlag::ITEM_WEAR_NECK;
	}
	else if (is_abbrev(str, "����"))
	{
		wear = EWearFlag::ITEM_WEAR_BODY;
	}
	else if (is_abbrev(str, "������"))
	{
		wear = EWearFlag::ITEM_WEAR_HEAD;
	}
	else if (is_abbrev(str, "����"))
	{
		wear = EWearFlag::ITEM_WEAR_LEGS;
	}
	else if (is_abbrev(str, "������"))
	{
		wear = EWearFlag::ITEM_WEAR_FEET;
	}
	else if (is_abbrev(str, "�����"))
	{
		wear = EWearFlag::ITEM_WEAR_HANDS;
	}
	else if (is_abbrev(str, "����"))
	{
		wear = EWearFlag::ITEM_WEAR_ARMS;
	}
	else if (is_abbrev(str, "���"))
	{
		wear = EWearFlag::ITEM_WEAR_SHIELD;
	}
	else if (is_abbrev(str, "�����"))
	{
		wear = EWearFlag::ITEM_WEAR_ABOUT;
	}
	else if (is_abbrev(str, "����"))
	{
		wear = EWearFlag::ITEM_WEAR_WAIST;
	}
	else if (is_abbrev(str, "��������"))
	{
		wear = EWearFlag::ITEM_WEAR_WRIST;
	}
	else if (is_abbrev(str, "������"))
	{
		wear = EWearFlag::ITEM_WEAR_WIELD;
	}
	else if (is_abbrev(str, "�����"))
	{
		wear = EWearFlag::ITEM_WEAR_HOLD;
	}
	else if (is_abbrev(str, "���"))
	{
		wear = EWearFlag::ITEM_WEAR_BOTHS;
	}
	else
	{
		return false;
	}

	return true;
}

class GoodsStorage
{
public:
	using shared_ptr = std::shared_ptr<GoodsStorage>;

	GoodsStorage(): m_object_uid_change_observer(new ObjectUIDChangeObserver(*this)) {}
	~GoodsStorage() { clear(); }

	const auto begin() const { return m_activities.begin(); }
	const auto end() const { return m_activities.end(); }

	void add(OBJ_DATA* object);
	void remove(OBJ_DATA* object);
	OBJ_DATA* get_by_uid(const int uid) const;
	void clear();

private:
	using activities_t = std::unordered_map<OBJ_DATA* /* object pointer */, int /* to last activity */>;
	using objects_by_uid_t = std::unordered_map<int /* UID */, OBJ_DATA* /* to object ponter */>;

	friend class ObjectUIDChangeObserver;

	class ObjectUIDChangeObserver: public UIDChangeObserver
	{
	public:
		ObjectUIDChangeObserver(GoodsStorage& storage) : m_parent(storage) {}

		virtual void notify(OBJ_DATA& object, const int old_uid) override;

	private:
		GoodsStorage& m_parent;
	};

	activities_t m_activities;
	objects_by_uid_t m_objects_by_uid;

	ObjectUIDChangeObserver::shared_ptr m_object_uid_change_observer;
};

void GoodsStorage::ObjectUIDChangeObserver::notify(OBJ_DATA& object, const int old_uid)
{
	const auto i = m_parent.m_objects_by_uid.find(old_uid);
	if (i == m_parent.m_objects_by_uid.end())
	{
		log("LOGIC ERROR: Got notification about changing UID %d of the object that is not registered. "
			"Won't do anything.",
			old_uid);
		return;
	}

	if (i->second != &object)
	{
		log("LOGIC ERROR: Got notification about changing UID %d of the object at address %p. But object with such "
			"UID is registered at address %p. Won't do anything.",
			old_uid, &object, i->second);
		return;
	}

	m_parent.m_objects_by_uid.erase(i);
	m_parent.m_objects_by_uid.emplace(object.get_uid(), &object);
}

void GoodsStorage::add(OBJ_DATA* object)
{
	const auto activity_i = m_activities.find(object);
	if (activity_i != m_activities.end())
	{
		log("LOGIC ERROR: Try to add object at ptr %p twice. Won't do anything. Object VNUM: %d",
			object, object->get_vnum());
		return;
	}

	const auto uid_i = m_objects_by_uid.find(object->get_uid());
	if (uid_i != m_objects_by_uid.end())
	{
		log("LOGIC ERROR: Try to add object at ptr %p with UID %d but such UID already presented for the object at address %p. "
			"Won't do anything. VNUM of the addee object: %d; VNUM of the presented object: %d.",
			object, object->get_uid(), uid_i->second, object->get_vnum(), uid_i->second->get_vnum());
		return;
	}

	m_activities.emplace(object, time(NULL));
	m_objects_by_uid.emplace(object->get_uid(), object);
	object->subscribe_for_uid_change(m_object_uid_change_observer);
}

void GoodsStorage::remove(OBJ_DATA* object)
{
	std::stringstream error;

	object->unsubscribe_for_uid_change(m_object_uid_change_observer);

	const auto uid = object->get_uid();
	const auto object_by_uid_i = m_objects_by_uid.find(uid);
	if (object_by_uid_i != m_objects_by_uid.end())
	{
		m_objects_by_uid.erase(object_by_uid_i);
	}
	else
	{
		error << "Try to remove object with UID " << uid << " but such UID is not registered.";
	}

	const auto activity_i = m_activities.find(object);
	if (activity_i != m_activities.end())
	{
		m_activities.erase(activity_i);
	}
	else
	{
		if (0 < error.tellp())
		{
			error << " ";
		}
		error << "Try to remove object at address " << object<< " but object at this address is not registered.";
	}

	if (0 < error.tellp())
	{
		log("LOGIC ERROR: %s", error.str().c_str());
	}
}

OBJ_DATA* GoodsStorage::get_by_uid(const int uid) const
{
	const auto i = m_objects_by_uid.find(uid);
	if (i != m_objects_by_uid.end())
	{
		return i->second;
	}

	return nullptr;
}

void GoodsStorage::clear()
{
	m_activities.clear();
	for (const auto& uid_pair : m_objects_by_uid)
	{
		uid_pair.second->unsubscribe_for_uid_change(m_object_uid_change_observer);
	}
	m_objects_by_uid.clear();
}

class ItemNode
{
public:
	using shared_ptr = std::shared_ptr<ItemNode>;
	using uid_t = unsigned;
	using temporary_ids_t = std::unordered_set<uid_t>;
	using item_descriptions_t = std::unordered_map<int/*vnum ��������*/, item_desc_node>;

	const static uid_t NO_UID = ~0u;

	ItemNode(const obj_vnum vnum, const long price) : m_vnum(vnum), m_price(price) {}
	ItemNode(const obj_vnum vnum, const long price, const uid_t uid) : m_vnum(vnum), m_price(price) { add_uid(uid); }

	const std::string& get_item_name(int keeper_vnum, int pad = 0) const;

	void add_desc(const mob_vnum vnum, const item_desc_node& desc) { m_descs[vnum] = desc; }
	void replace_descs(OBJ_DATA *obj, const int vnum) const;
	bool has_desc(const obj_vnum vnum) const { return m_descs.find(vnum) != m_descs.end(); }

	bool empty() const { return m_item_uids.empty(); }
	const auto size() const { return m_item_uids.size(); }

	auto uid() const { return 0 < m_item_uids.size() ? *m_item_uids.begin() : NO_UID; }
	void add_uid(const uid_t uid) { m_item_uids.emplace(uid); }
	void remove_uid(const uid_t uid) { m_item_uids.erase(uid); }

	const auto vnum() const { return m_vnum; }

	const auto get_price() const { return m_price; }
	void set_price(const long _) { m_price = _; }

private:
	int m_vnum;						// VNUM of the item's prototype
	long m_price;					// Price of the item

	temporary_ids_t m_item_uids;	// List of uids of this item in the shop
	item_descriptions_t m_descs;
};

const std::string& ItemNode::get_item_name(int keeper_vnum, int pad /*= 0*/) const
{
	const auto desc_i = m_descs.find(keeper_vnum);
	if (desc_i != m_descs.end())
	{
		return desc_i->second.PNames[pad];
	}
	else
	{
		const auto rnum = obj_proto.rnum(m_vnum);
		const static std::string wrong_vnum = "<unknown VNUM>";
		if (-1 == rnum)
		{
			return wrong_vnum;
		}
		return GET_OBJ_PNAME(obj_proto[rnum], pad);
	}
}

void ItemNode::replace_descs(OBJ_DATA *obj, const int vnum) const
{
	const auto desc_i = m_descs.find(vnum);
	if (!obj
		|| desc_i == m_descs.end())
	{
		return;
	}

	const auto& desc = desc_i->second;

	obj->set_description(desc.description.c_str());
	obj->set_aliases(desc.name.c_str());
	obj->set_short_description(desc.short_description.c_str());
	obj->set_PName(0, desc.PNames[0].c_str());
	obj->set_PName(1, desc.PNames[1].c_str());
	obj->set_PName(2, desc.PNames[2].c_str());
	obj->set_PName(3, desc.PNames[3].c_str());
	obj->set_PName(4, desc.PNames[4].c_str());
	obj->set_PName(5, desc.PNames[5].c_str());
	obj->set_sex(desc.sex);

	if (!desc.trigs.empty())
	{
		obj->attach_triggers(desc.trigs);
	}

	obj->set_ex_description(nullptr); //���� � ������� ������ ������� �������������� - ������� �����

	if ((GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
		&& (GET_OBJ_VAL(obj, 1) > 0)) //���� �������� � �������� ��������...
	{
		name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2)); //...������� ������� ���������� �������
	}
}

class ItemsList
{
public:
	using items_list_t = std::vector<ItemNode::shared_ptr>;

	ItemsList() {}

	void add(const obj_vnum vnum, const long price);
	void add(const obj_vnum vnum, const long price, const ItemNode::uid_t uid);
	void remove(const size_t index) { m_items_list.erase(m_items_list.begin() + index); }

	const auto& node(const size_t index) const;
	const auto size() const { return m_items_list.size(); }
	bool empty() const { return m_items_list.empty(); }

private:
	items_list_t m_items_list;
};

void ItemsList::add(const obj_vnum vnum, const long price, const ItemNode::uid_t uid)
{
	const auto item = std::make_shared<ItemNode>(vnum, price, uid);
	m_items_list.push_back(std::move(item));
}

void ItemsList::add(const obj_vnum vnum, const long price)
{
	const auto item = std::make_shared<ItemNode>(vnum, price);
	m_items_list.push_back(std::move(item));
}

const auto& ItemsList::node(const size_t index) const
{
	if (index < m_items_list.size())
	{
		return m_items_list[index];
	}

	// if we are here then this is an error
	log("LOGIC ERROR: Requested shop item with too high index %zd but shop has only %zd items", index, size());
	static decltype(m_items_list)::value_type null_ptr;
	return null_ptr;
}

class shop_node : public DictionaryItem
{
public:
	using shared_ptr = std::shared_ptr<shop_node>;
	using mob_vnums_t = std::list<mob_vnum>;

	shop_node(): waste_time_min(0), can_buy(true) {};

	std::string currency;
	std::string id;
	int profit;
	int waste_time_min;
	bool can_buy;

	void add_item(const obj_vnum vnum, const long price) { m_items_list.add(vnum, price); }
	void add_item(const obj_vnum vnum, const long price, const ItemNode::uid_t uid) { m_items_list.add(vnum, price, uid); }

	const auto& items_list() const { return m_items_list; }

	void add_mob_vnum(const mob_vnum vnum) { m_mob_vnums.push_back(vnum); }

	const auto& mob_vnums() const { return m_mob_vnums; }

	void process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument);
	void print_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum);	// it should be const
	void filter_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum);
	void process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);
	void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);	// it should be const
	void clear_store();
	bool empty() const { return m_items_list.empty(); }

private:
	void put_to_storage(OBJ_DATA* object) { m_storage.add(object); }

	void remove_from_storage(OBJ_DATA *obj);
	OBJ_DATA* get_from_shelve(const size_t index);
	unsigned get_item_num(std::string &item_name, int keeper_vnum);	// it should be const
	int can_sell_count(const int item_index) const;
	void put_item_to_shop(OBJ_DATA* obj);
	void do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd);

	ItemsList m_items_list;
	mob_vnums_t m_mob_vnums;

	GoodsStorage m_storage;
};

void shop_node::process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument)
{
	std::string buffer2(argument), buffer1;
	GetOneParam(buffer2, buffer1);
	boost::trim(buffer2);

	if (buffer1.empty())
	{
		tell_to_char(keeper, ch, "��� �� ������ ������?");
		return;
	}

	int item_num = 0, item_count = 1;

	if (buffer2.empty())
	{
		if (is_number(buffer1.c_str()))
		{
			// buy 5
			try
			{
				item_num = boost::lexical_cast<unsigned>(buffer1);
			}
			catch (const boost::bad_lexical_cast&)
			{
				item_num = 0;
			}
		}
		else
		{
			// buy sword
			item_num = get_item_num(buffer1, GET_MOB_VNUM(keeper));
		}
	}
	else if (is_number(buffer1.c_str()))
	{
		if (is_number(buffer2.c_str()))
		{
			// buy 5 10
			try
			{
				item_num = boost::lexical_cast<unsigned>(buffer2);
			}
			catch (const boost::bad_lexical_cast&)
			{
				item_num = 0;
			}
		}
		else
		{
			// buy 5 sword
			item_num = get_item_num(buffer2, GET_MOB_VNUM(keeper));
		}
		try
		{
			item_count = boost::lexical_cast<unsigned>(buffer1);
		}
		catch (const boost::bad_lexical_cast&)
		{
			item_count = 1000;
		}
	}
	else
	{
		tell_to_char(keeper, ch, "��� �� ������ ������?");
		return;
	}

	if (!item_count
		|| !item_num
		|| static_cast<size_t>(item_num) > m_items_list.size())
	{
		tell_to_char(keeper, ch, "������ �����, � ���� ��� ����� ����.");
		return;
	}

	if (item_count >= 1000)
	{
		tell_to_char(keeper, ch, "� ����� �� �������?");
		return;
	}

	const auto item_index = item_num - 1;

	CObjectPrototype* tmp_obj = nullptr;
	bool obj_from_proto = true;
	const auto item = m_items_list.node(item_index);
	if (!item->empty())
	{
		tmp_obj = get_from_shelve(item_index);

		if (!tmp_obj)
		{
			log("SYSERROR : �� ������� ��������� ������� (%s:%d)", __FILE__, __LINE__);
			send_to_char("�������� �����.\r\n", ch);
			return;
		}

		obj_from_proto = false;
	}

	auto proto = (tmp_obj ? tmp_obj : get_object_prototype(item->vnum()).get());
	if (!proto)
	{
		log("SYSERROR : �� ������� ��������� �������� (%s:%d)", __FILE__, __LINE__);
		send_to_char("�������� �����.\r\n", ch);
		return;
	}

	const long price = item->get_price();
	if (!check_money(ch, price, currency))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"� ��� ��� ������� %s!", ExtMoney::name_currency_plural(currency).c_str());
		tell_to_char(keeper, ch, buf);

		char local_buf[MAX_INPUT_LENGTH];
		switch (number(0, 3))
		{
		case 0:
			snprintf(local_buf, MAX_INPUT_LENGTH, "������ %s!", GET_NAME(ch));
			do_social(keeper, local_buf);
			break;

		case 1:
			snprintf(local_buf, MAX_INPUT_LENGTH,
				"���������$g �������� ������ %s",
				IS_MALE(keeper) ? "�����" : "��������");
			do_echo(keeper, local_buf, 0, SCMD_EMOTE);
			break;
		}
		return;
	}

	if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
		|| ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto)) > CAN_CARRY_W(ch)))
	{
		const auto& name = obj_from_proto
			? item->get_item_name(GET_MOB_VNUM(keeper), 3).c_str()
			: tmp_obj->get_short_description().c_str();
		snprintf(buf, MAX_STRING_LENGTH,
			"%s, � �������, ���� ���� ������ �� �����,\r\n"
			"�� %s ��� ���� ������ ��������.\r\n",
			GET_NAME(ch), name);
		send_to_char(buf, ch);
		return;
	}

	int bought = 0;
	int total_money = 0;
	int sell_count = can_sell_count(item_index);

	OBJ_DATA *obj = 0;
	while (bought < item_count
		&& check_money(ch, price, currency)
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
		&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch)
		&& (bought < sell_count || sell_count == -1))
	{

		if (!item->empty())
		{
			obj = get_from_shelve(item_index);
			item->remove_uid(obj->get_uid());
			if (item->empty())
			{
				m_items_list.remove(item_index);
			}
			remove_from_storage(obj);
		}
		else
		{
			obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
			item->replace_descs(obj, GET_MOB_VNUM(keeper));
		}

		if (obj)
		{
			if (GET_OBJ_ZONE(obj) == NOWHERE)
			{
				obj->set_zone(world[ch->in_room]->zone);
			}

			obj_to_char(obj, ch);
			if (currency == "�����")
			{
				// ����� �� ����� �� ������
				if (OBJ_DATA::ITEM_BOOK == GET_OBJ_TYPE(obj))
				{
					obj->set_extra_flag(EExtraFlag::ITEM_NO_FAIL);
				}

				// ������ � ����������� �����
				GloryConst::add_total_spent(price);

				int removed = Glory::remove_glory(GET_UNIQUE(ch), price);
				if (removed > 0)
				{
					GloryConst::transfer_log("%s bought %s for %d temp glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0).c_str(), removed);
				}

				if (removed != price)
				{
					GloryConst::remove_glory(GET_UNIQUE(ch), price - removed);
					GloryConst::transfer_log("%s bought %s for %d const glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0).c_str(), price - removed);
				}
			}
			else if (currency == "���")
			{
				// ����� �� ���, ��� � �� �����, �� ������
				if (OBJ_DATA::ITEM_BOOK == GET_OBJ_TYPE(obj))
				{
					obj->set_extra_flag(EExtraFlag::ITEM_NO_FAIL);
				}
				ch->sub_ice_currency(price);

			}
			else
			{
				ch->remove_gold(price);
				spent_today += price;
			}
			++bought;

			total_money += price;
		}
		else
		{
			log("SYSERROR : �� ������� ��������� ������� obj_vnum=%d (%s:%d)",
				GET_OBJ_VNUM(proto), __FILE__, __LINE__);
			send_to_char("�������� �����.\r\n", ch);
			return;
		}
	}

	if (bought < item_count)
	{
		if (!check_money(ch, price, currency))
		{
			snprintf(buf, MAX_STRING_LENGTH, "�������%s, �� ������ �������� ������ %d.",
				IS_MALE(ch) ? "�" : "��", bought);
		}
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		{
			snprintf(buf, MAX_STRING_LENGTH, "�� ������� ������ ������ %d.", bought);
		}
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) > CAN_CARRY_W(ch))
		{
			snprintf(buf, MAX_STRING_LENGTH, "�� ������� ������� ������ %d.", bought);
		}
		else if (bought > 0)
		{
			snprintf(buf, MAX_STRING_LENGTH, "� ������ ���� ������ %d.", bought);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "� �� ������ ���� ������.");
		}

		tell_to_char(keeper, ch, buf);
	}

	const auto suffix = desc_count(total_money, currency == "����" ? WHAT_MONEYu : (currency == "���" ? WHAT_ICEu : WHAT_GLORYu));
	snprintf(buf, MAX_STRING_LENGTH, "��� ����� ������ %d %s.", total_money, suffix);
	tell_to_char(keeper, ch, buf);

	if (obj)
	{
		send_to_char(ch, "������ �� ����� %s %s.\r\n",
			IS_MALE(ch) ? "���������� �����������" : "���������� ���������������",
			obj->item_count_message(bought, 1).c_str());
	}
}

void shop_node::print_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum)
{
	send_to_char(ch,
		" ##    ��������   �������                                      ���� (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		currency.c_str());
	int num = 1;
	std::stringstream out;
	std::string print_value;
	std::string name_value;

	for (size_t k = 0; k < m_items_list.size();)
	{
		int count = can_sell_count(num - 1);

		print_value.clear();
		name_value.clear();

		const auto& item = m_items_list.node(k);

		//Polud � ��������� � ����� �������� ���������� � ������ �� �������� �� ���������, � ���, ��������, ���������� ��������
		// ����� �� ���� � ������� ������ "���� @n1"
		if (item->empty())
		{
			print_value = item->get_item_name(keeper_vnum);
			const auto rnum = obj_proto.rnum(item->vnum());
			if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				print_value += " � " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}
		}
		else
		{
			OBJ_DATA* tmp_obj = get_from_shelve(k);
			if (tmp_obj)
			{
				print_value = tmp_obj->get_short_description();
				name_value = tmp_obj->get_aliases();
				item->set_price(GET_OBJ_COST(tmp_obj));
			}
			else
			{
				m_items_list.remove(k);	// remove from shop object that we cannot instantiate
				continue;
			}
		}

		std::string numToShow = (count == -1 || count > 100 ? "�������" : boost::lexical_cast<std::string>(count));

		// ���� ������ ������� ��� �� �� �������� ��� ������� � ��� � ������
		if (arg.empty()
			|| isname(arg, print_value)
			|| (!name_value.empty()
				&& isname(arg, name_value)))
		{
			std::string format_str = "%4d)  %10s  %-"
				+ std::to_string(std::count(print_value.begin(), print_value.end(), '&') * 2 + 45) + "s %8d\r\n";
			out << boost::str(boost::format(format_str) % num++ % numToShow % print_value % item->get_price());
		}
		else
		{
			num++;
		}

		++k;
	}

	page_string(ch->desc, out.str());
}

void shop_node::filter_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum)
{
	int num = 1;
	wear = EWearFlag::ITEM_WEAR_UNDEFINED;
	type = -10;

	std::string print_value = "";
	std::string name_value = "";

	const char *filtr_value = "";
	const char *first_simvol = "";

	if (!arg.empty())
	{
		first_simvol = arg.c_str();
		filtr_value = arg.substr(1, arg.size() - 1).c_str();
	}

	switch (first_simvol[0])
	{
	case '�':
		if (!init_type(filtr_value))
		{
			send_to_char("�������� ��� ��������.\r\n", ch);
			return;
		}
		break;

	case '�':
		if (!init_wear(filtr_value))
		{
			send_to_char("�������� ����� �������� ��������.\r\n", ch);
			return;
		}
		break;

	default:
		send_to_char("�������� ������. \r\n", ch);
		return;;
		break;
	};

	send_to_char(ch,
		" ##    ��������   �������(������)                              ���� (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		currency.c_str());

	std::stringstream out;
	for (auto k = 0; k < m_items_list.size();)
	{
		int count = can_sell_count(num - 1);
		bool show_name = true;

		print_value = "";
		name_value = "";

		const auto& item = m_items_list.node(k);

		//Polud � ��������� � ����� �������� ���������� � ������ �� �������� �� ���������, � ���, ��������, ���������� ��������
		// ����� �� ���� � ������� ������ "���� @n1"
		if (item->empty())
		{
			print_value = item->get_item_name(keeper_vnum);
			const auto rnum = obj_proto.rnum(item->vnum());
			if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				print_value += " � " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}

			if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED && obj_proto[rnum]->has_wear_flag(wear))
				|| (type > 0 && type == GET_OBJ_TYPE(obj_proto[rnum]))))
			{
				show_name = false;
			}
		}
		else
		{
			OBJ_DATA* tmp_obj = get_from_shelve(k);
			if (tmp_obj)
			{
				if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED && CAN_WEAR(tmp_obj, wear))
					|| (type > 0 && type == GET_OBJ_TYPE(tmp_obj))))
				{
					show_name = false;
				}

				print_value = tmp_obj->get_short_description();
				name_value = tmp_obj->get_aliases();
				item->set_price(GET_OBJ_COST(tmp_obj));
			}
			else
			{
				m_items_list.remove(k);	// remove from shop object that we cannot instantiate

				continue;
			}
		}

		std::string numToShow = count == -1
			? "�������"
			: boost::lexical_cast<std::string>(count);

		if (show_name)
		{
			out << (boost::str(boost::format("%4d)  %10s  %-47s %8d\r\n") % num++ % numToShow % print_value % item->get_price()));
		}
		else
		{
			num++;
		}

		++k;
	}

	page_string(ch->desc, out.str());
}

void shop_node::process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd)
{
	std::string buffer(argument), buffer1;
	GetOneParam(buffer, buffer1);
	boost::trim(buffer);

	if (!can_buy
		&& (cmd == "�������"
			|| cmd == "�������"))
	{
		tell_to_char(keeper, ch, "������, � ���� ���� ����������...");

		return;
	}

	if (!*argument)
	{
		tell_to_char(keeper, ch, (cmd + " ���?").c_str());
		return;
	}

	if (buffer1.empty())
	{
		return;
	}

	if (is_number(buffer1.c_str()))
	{
		int n = 0;
		try
		{
			n = boost::lexical_cast<int>(buffer1);
		}
		catch (const boost::bad_lexical_cast&)
		{
		}

		OBJ_DATA* obj = get_obj_in_list_vis(ch, buffer, ch->carrying);

		if (!obj)
		{
			send_to_char("� ��� ��� " + buffer + "!\r\n", ch);
			return;
		}

		while (obj && n > 0)
		{
			const auto obj_next = get_obj_in_list_vis(ch, buffer, obj->get_next_content());
			do_shop_cmd(ch, keeper, obj, cmd);
			obj = obj_next;
			n--;
		}
	}
	else
	{
		skip_spaces(&argument);
		int i, dotmode = find_all_dots(argument);
		std::string buffer2(argument);
		switch (dotmode)
		{
		case FIND_INDIV:
			{
				const auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);

				if (!obj)
				{
					if (cmd == "������" && is_abbrev(argument, "����������"))
					{
						for (i = 0; i < NUM_WEARS; i++)
						{
							if (ch->equipment[i])
							{
								do_shop_cmd(ch, keeper, ch->equipment[i], cmd);
							}
						}
						return;
					}
					send_to_char("� ��� ��� " + buffer2 + "!\r\n", ch);
					return;
				}

				do_shop_cmd(ch, keeper, obj, cmd);
			}

			break;

		case FIND_ALL:
			{
				OBJ_DATA* obj_next = nullptr;
				for (auto obj = ch->carrying; obj; obj = obj_next)
				{
					obj_next = obj->get_next_content();
					do_shop_cmd(ch, keeper, obj, cmd);
				}
			}

			break;

		case FIND_ALLDOT:
			{
				auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);
				if (!obj)
				{
					send_to_char("� ��� ��� " + buffer2 + "!\r\n", ch);
					return;
				}

				while (obj)
				{
					const auto obj_next = get_obj_in_list_vis(ch, buffer2, obj->get_next_content());
					do_shop_cmd(ch, keeper, obj, cmd);
					obj = obj_next;
				}
			}
			break;

		default:
			break;
		};
	}
}

void shop_node::process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd)
{
	std::string buffer(argument);
	boost::trim(buffer);

	if (buffer.empty())
	{
		tell_to_char(keeper, ch, "�������������� ���� �� ������ ������?");
		return;
	}

	unsigned item_num = 0;
	if (is_number(buffer.c_str()))
	{
		// �������������� 5
		try
		{
			item_num = boost::lexical_cast<unsigned>(buffer);
		}
		catch (const boost::bad_lexical_cast&)
		{
		}
	}
	else
	{
		// �������������� ���
		item_num = get_item_num(buffer, GET_MOB_VNUM(keeper));
	}

	if (!item_num
		|| item_num > items_list().size())
	{
		tell_to_char(keeper, ch, "������ �����, � ���� ��� ����� ����.");
		return;
	}

	const auto item_index = item_num - 1;

	const OBJ_DATA *ident_obj = nullptr;
	OBJ_DATA* tmp_obj = nullptr;
	const auto& item = m_items_list.node(item_index);
	if (item->empty())
	{
		const auto vnum = GET_MOB_VNUM(keeper);
		if (item->has_desc(vnum))
		{
			tmp_obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
			item->replace_descs(tmp_obj, vnum);
			ident_obj = tmp_obj;
		}
		else
		{
			const auto rnum = obj_proto.rnum(item->vnum());
			const auto object = world_objects.create_raw_from_prototype_by_rnum(rnum);
			ident_obj = tmp_obj = object.get();
		}
	}
	else
	{
		ident_obj = get_from_shelve(item_index);
	}

	if (!ident_obj)
	{
		log("SYSERROR : �� ������� �������� ������ (%s:%d)", __FILE__, __LINE__);
		send_to_char("�������� �����.\r\n", ch);
		return;
	}

	if (cmd == "�����������")
	{
		std::stringstream tell;
		tell << "������� " << ident_obj->get_short_description() << ": ";
		tell << item_types[GET_OBJ_TYPE(ident_obj)] << "\r\n";
		tell << diag_weapon_to_char(ident_obj, TRUE);
		tell << diag_timer_to_char(ident_obj);

		if (can_use_feat(ch, SKILLED_TRADER_FEAT)
			|| PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		{
			sprintf(buf, "�������� : ");
			sprinttype(ident_obj->get_material(), material_name, buf + strlen(buf));
			sprintf(buf + strlen(buf), ".\r\n");
			tell << buf;
		}

		tell_to_char(keeper, ch, tell.str().c_str());
		if (invalid_anti_class(ch, ident_obj)
			|| invalid_unique(ch, ident_obj)
			|| NamedStuff::check_named(ch, ident_obj, 0))
		{
			tell.str("�� ����� �� ���� �� ������������� �� ��� ����, �� ������� ��� �����.");
			tell_to_char(keeper, ch, tell.str().c_str());
		}
	}

	if (cmd == "��������������")
	{
		if (ch->get_gold() < IDENTIFY_COST)
		{
			tell_to_char(keeper, ch, "� ��� ��� ������� �����!");
			char local_buf[MAX_INPUT_LENGTH];
			switch (number(0, 3))
			{
			case 0:
				snprintf(local_buf, MAX_INPUT_LENGTH, "������ %s!", GET_NAME(ch));
				do_social(keeper, local_buf);
				break;

			case 1:
				snprintf(local_buf, MAX_INPUT_LENGTH,
					"���������$g �������� ������ %s",
					IS_MALE(keeper) ? "�����" : "��������");
				do_echo(keeper, local_buf, 0, SCMD_EMOTE);
				break;
			}
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"��� ������ ����� ������ %d %s.", IDENTIFY_COST,
				desc_count(IDENTIFY_COST, WHAT_MONEYu));
			tell_to_char(keeper, ch, buf);

			send_to_char(ch, "�������������� ��������: %s\r\n", GET_OBJ_PNAME(ident_obj, 0).c_str());
			mort_show_obj_values(ident_obj, ch, 200);
			ch->remove_gold(IDENTIFY_COST);
		}
	}

	if (tmp_obj)
	{
		extract_obj(tmp_obj);
	}
}

void shop_node::clear_store()
{
	for (const auto& stored_item : m_storage)
	{
		extract_obj(stored_item.first);
	}

	m_storage.clear();
}

void shop_node::remove_from_storage(OBJ_DATA *object)
{
	m_storage.remove(object);
}

OBJ_DATA* shop_node::get_from_shelve(const size_t index)
{
	const auto node = m_items_list.node(index);
	const auto uid = node->uid();
	if (ItemNode::NO_UID == uid)
	{
		return nullptr;
	}

	return m_storage.get_by_uid(uid);
}

unsigned shop_node::get_item_num(std::string &item_name, int keeper_vnum)
{
	int num = 1;
	if (!item_name.empty() && a_isdigit(item_name[0]))
	{
		std::string::size_type dot_idx = item_name.find_first_of(".");
		if (dot_idx != std::string::npos)
		{
			std::string first_param = item_name.substr(0, dot_idx);
			item_name.erase(0, ++dot_idx);
			if (is_number(first_param.c_str()))
			{
				try
				{
					num = boost::lexical_cast<int>(first_param);
				}
				catch (const boost::bad_lexical_cast&)
				{
					return 0;
				}
			}
		}
	}

	int count = 0;
	std::string name_value = "";
	std::string print_value;
	for (unsigned i = 0; i < items_list().size(); ++i)
	{
		print_value = "";
		const auto& item = m_items_list.node(i);
		if (item->empty())
		{
			name_value = item->get_item_name(keeper_vnum);
			const auto rnum = obj_proto.rnum(item->vnum());
			if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				name_value += " " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}
		}
		else
		{
			OBJ_DATA * tmp_obj = get_from_shelve(i);
			if (!tmp_obj)
			{
				continue;
			}
			name_value = tmp_obj->get_aliases();
			print_value = tmp_obj->get_short_description();
		}

		if (isname(item_name, name_value) || isname(item_name, print_value))
		{
			++count;
			if (count == num)
			{
				return ++i;
			}
		}
	}

	return 0;
}

int shop_node::can_sell_count(const int item_index) const
{
	const auto& item = m_items_list.node(item_index);
	if (!item->empty())
	{
		return static_cast<int>(item->size());
	}
	else
	{
		const auto rnum = obj_proto.rnum(item->vnum());
		int numToSell = obj_proto[rnum]->get_max_in_world();
		if (numToSell == 0)
		{
			return numToSell;
		}

		if (numToSell != -1)
		{
			numToSell -= MIN(numToSell, obj_proto.actual_count(rnum));	//������� �� ������ ������, �� � �� ��� � �����
		}

		return numToSell;
	}
}

void shop_node::put_item_to_shop(OBJ_DATA* obj)
{
	for (auto index = 0; index < m_items_list.size(); ++index)
	{
		const auto& item = m_items_list.node(index);
		if (item->vnum() == obj->get_vnum())
		{
			if (item->empty())
			{
				extract_obj(obj);
				return;
			}
			else
			{
				OBJ_DATA* tmp_obj = get_from_shelve(index);
				if (!tmp_obj)
				{
					continue;
				}

				if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING //� � ��� ���� ���� ����
					|| obj->get_short_description() == tmp_obj->get_short_description())
				{
					item->add_uid(obj->get_uid());
					put_to_storage(obj);

					return;
				}
			}
		}
	}

	add_item(GET_OBJ_RNUM(obj), GET_OBJ_COST(obj), obj->get_uid());

	put_to_storage(obj);
}

void shop_node::do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd)
{
	if (!obj)
	{
		return;
	}

	int rnum = GET_OBJ_RNUM(obj);
	if (rnum < 0
		|| obj->get_extra_flag(EExtraFlag::ITEM_ARMORED)
		|| obj->get_extra_flag(EExtraFlag::ITEM_SHARPEN)
		|| obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
	{
		tell_to_char(keeper, ch, std::string("� �� ��������� ����� ���� � ���� �����.").c_str());
		return;
	}

	if (GET_OBJ_VAL(obj, 2) == 0
		&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND
			|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF))
	{
		tell_to_char(keeper, ch, "� �� ������� �������������� ����!");
		return;
	}

	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER
		&& cmd != "������")
	{
		if (obj->get_contains())
		{
			tell_to_char(keeper, ch, "�� ���� ���������� ��� ���� � �����.");
			return;
		}
	}

	long buy_price = GET_OBJ_COST(obj);
	long buy_price_old = get_sell_price(obj);

	int repair = GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj);
	int repair_price = MAX(1, GET_OBJ_COST(obj) * MAX(0, repair) / MAX(1, GET_OBJ_MAX(obj)));

	// ���� �� �����, �� ��������� ������� ��������, ���� �����, �� ��������� ����, ��� ������� ������ ��� ������
	if (!can_use_feat(ch, SKILLED_TRADER_FEAT))
	{
		buy_price = MMAX(1, (buy_price * profit) / 100); //����� ������� ��������
	}
	else
	{
		buy_price = get_sell_price(obj);
	}

	// ���� ���� �������, ����, ��� ��������� ��������
	if (buy_price > buy_price_old)
	{
		buy_price = buy_price_old;
	}

	std::string price_to_show = boost::lexical_cast<std::string>(buy_price) + " " + std::string(desc_count(buy_price, WHAT_MONEYu));

	if (cmd == "�������")
	{
		if (bloody::is_bloody(obj))
		{
			tell_to_char(keeper, ch, "��� �� ����� ����� �������!");
			return;
		}

		if (obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
			|| obj->get_extra_flag(EExtraFlag::ITEM_NAMED)
			|| obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY)
			|| obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY))
		{
			tell_to_char(keeper, ch, "����� � �� �������.");
			return;
		}
		else
		{
			tell_to_char(keeper, ch, ("�, �������, ����� " + std::string(GET_OBJ_PNAME(obj, 3)) + " �� " + price_to_show + ".").c_str());
		}
	}

	if (cmd == "�������")
	{
		if (obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
			|| obj->get_extra_flag(EExtraFlag::ITEM_NAMED)
			|| obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY)
			|| (buy_price <= 1)
			|| obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
			|| bloody::is_bloody(obj))
		{
			if (bloody::is_bloody(obj))
			{
				tell_to_char(keeper, ch, "���� ��� ������, � ���� �� ����� �����!");
			}
			else
			{
				tell_to_char(keeper, ch, "����� � �� �������.");
			}

			return;
		}
		else
		{
			obj_from_char(obj);
			tell_to_char(keeper, ch, ("������ �� " + std::string(GET_OBJ_PNAME(obj, 3)) + " " + price_to_show + ".").c_str());
			ch->add_gold(buy_price);
			put_item_to_shop(obj);
		}
	}
	if (cmd == "������")
	{
		if (bloody::is_bloody(obj))
		{
			tell_to_char(keeper, ch, "� �� ���� ������ ������������� ����!");
			return;
		}

		if (repair <= 0)
		{
			tell_to_char(keeper, ch, (std::string(GET_OBJ_PNAME(obj, 3)) + " �� ����� ������.").c_str());
			return;
		}

		switch (obj->get_material())
		{
		case OBJ_DATA::MAT_BULAT:
		case OBJ_DATA::MAT_CRYSTALL:
		case OBJ_DATA::MAT_DIAMOND:
		case OBJ_DATA::MAT_SWORDSSTEEL:
			repair_price *= 2;
			break;

		case OBJ_DATA::MAT_SUPERWOOD:
		case OBJ_DATA::MAT_COLOR:
		case OBJ_DATA::MAT_GLASS:
		case OBJ_DATA::MAT_BRONZE:
		case OBJ_DATA::MAT_FARFOR:
		case OBJ_DATA::MAT_BONE:
		case OBJ_DATA::MAT_ORGANIC:
			repair_price += MAX(1, repair_price / 2);
			break;

		case OBJ_DATA::MAT_IRON:
		case OBJ_DATA::MAT_STEEL:
		case OBJ_DATA::MAT_SKIN:
		case OBJ_DATA::MAT_MATERIA:
			//repair_price = repair_price;
			break;

		default:
			//repair_price = repair_price;
			break;
		}

		if (repair_price <= 0
			|| obj->get_extra_flag(EExtraFlag::ITEM_DECAY)
			|| obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
			|| obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
		{
			tell_to_char(keeper, ch, ("� �� ���� ������� ���� ����������� ����� �� " + GET_OBJ_PNAME(obj, 3) + ".").c_str());
			return;
		}

		tell_to_char(keeper, ch, ("������� " + std::string(GET_OBJ_PNAME(obj, 1)) + " ��������� � "
			+ boost::lexical_cast<std::string>(repair_price) + " " + desc_count(repair_price, WHAT_MONEYu)).c_str());

		if (!IS_GOD(ch) && repair_price > ch->get_gold())
		{
			act("� ��� �� � ���� ���-��� �� � ���.", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}

		if (!IS_GOD(ch))
		{
			ch->remove_gold(repair_price);
		}

		act("$n ���������� �������$g $o3.", FALSE, keeper, obj, 0, TO_ROOM);

		obj->set_current_durability(GET_OBJ_MAX(obj));
	}
}

using ShopListType = std::vector<shop_node::shared_ptr>;

struct item_set_node
{
	long item_vnum;
	int item_price;
};

struct item_set
{
	item_set() {};

	std::string _id;
	std::vector<item_set_node> item_list;
};

typedef boost::shared_ptr<item_set> ItemSetPtr;
typedef std::vector<ItemSetPtr> ItemSetListType;
ShopListType shop_list;

void log_shop_load()
{
	for (const auto& shop : shop_list)
	{
		for (const auto& mob_vnum : shop->mob_vnums())
		{
			log("ShopExt: mob=%d", mob_vnum);
		}

		log("ShopExt: currency=%s", shop->currency.c_str());

		const auto& items = shop->items_list();
		for (auto index = 0; index < items.size(); ++index)
		{
			const auto& item = items.node(index);
			log("ItemList: vnum=%d, price=%ld", item->vnum(), item->get_price());
		}
	}
}

void load_item_desc()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/item_desc.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node node_list = doc.child("templates");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...templates list read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	item_descriptions.clear();
	pugi::xml_node child;
	for (pugi::xml_node item_template = node_list.child("template"); item_template; item_template = item_template.next_sibling("template"))
	{
		std::string templateId = item_template.attribute("id").value();
		for (pugi::xml_node item = item_template.child("item"); item; item = item.next_sibling("item"))
		{

			int item_vnum = Parse::attr_int(item, "vnum");
			if (item_vnum <= 0)
			{
					snprintf(buf, MAX_STRING_LENGTH,
						"...bad item description attributes (item_vnum=%d)", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
			}
			item_desc_node desc_node;
			child = item.child("name");
			desc_node.name = child.child_value();
			child = item.child("description");
			desc_node.description = child.child_value();
			child = item.child("short_description");
			desc_node.short_description = child.child_value();
			child = item.child("PNames0");
			desc_node.PNames[0] = child.child_value();
			child = item.child("PNames1");
			desc_node.PNames[1] = child.child_value();
			child = item.child("PNames2");
			desc_node.PNames[2] = child.child_value();
			child = item.child("PNames3");
			desc_node.PNames[3] = child.child_value();
			child = item.child("PNames4");
			desc_node.PNames[4] = child.child_value();
			child = item.child("PNames5");
			desc_node.PNames[5] = child.child_value();
			child = item.child("sex");
			desc_node.sex = static_cast<ESex>(atoi(child.child_value()));

			// ������ ������ ���������
			pugi::xml_node trig_list = item.child("triggers");
			CObjectPrototype::triggers_list_t trig_vnums;
			for (pugi::xml_node trig = trig_list.child("trig"); trig; trig = trig.next_sibling("trig"))
			{
				int trig_vnum;
				std::string tmp_value = trig.child_value();
				boost::trim(tmp_value);
				try
				{
					trig_vnum = boost::lexical_cast<unsigned>(tmp_value);
				}
				catch (boost::bad_lexical_cast &)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...error while casting to num (item_vnum=%d, casting value=%s)", item_vnum, tmp_value.c_str());
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (trig_vnum <= 0)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...error while parsing triggers (item_vnum=%d, parsed value=%s)", item_vnum, tmp_value.c_str());
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				trig_vnums.push_back(trig_vnum);
			}
			desc_node.trigs = trig_vnums;
			item_descriptions[templateId][item_vnum] = desc_node;
		}
	}
}

void load(bool reload)
{
	if (reload)
	{
		for (const auto& shop : shop_list)
		{
			shop->clear_store();

			for (const auto& mob_vnum : shop->mob_vnums())
			{
				int mob_rnum = real_mobile(mob_vnum);
				if (mob_rnum >= 0)
				{
					mob_index[mob_rnum].func = NULL;
				}
			}
		}

		shop_list.clear();
	}
	load_item_desc();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/shops.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("shop_list");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...shop_list read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	//������ ��������� - "���������" ��� �������� ��������� � ���������. ����� ������ �� ����� �����
	ItemSetListType itemSetList;
	pugi::xml_node itemSets = doc.child("shop_item_sets");
	for (pugi::xml_node itemSet = itemSets.child("shop_item_set"); itemSet; itemSet = itemSet.next_sibling("shop_item_set"))
	{
		std::string itemSetId = itemSet.attribute("id").value();
		ItemSetPtr tmp_set(new item_set);
		tmp_set->_id = itemSetId;

		for (pugi::xml_node item = itemSet.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = Parse::attr_int(item, "vnum");
			int price = Parse::attr_int(item, "price");
			if (item_vnum < 0 || price < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop item set attributes (item_set=%s, item_vnum=%d, price=%d)", itemSetId.c_str(), item_vnum, price);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
			struct item_set_node tmp_node;
			tmp_node.item_price = price;
			tmp_node.item_vnum = item_vnum;
			tmp_set->item_list.push_back(tmp_node);
		}
		itemSetList.push_back(tmp_set);
	}

	for (pugi::xml_node node = node_list.child("shop"); node; node = node.next_sibling("shop"))
	{
		std::string shop_id = node.attribute("id").value();
		std::string currency = node.attribute("currency").value();
		int profit = node.attribute("profit").as_int();
		std::string can_buy_value = node.attribute("can_buy").value();
		bool shop_can_buy = can_buy_value != "false";
		int store_time_min = (node.attribute("waste_time_min").value() ? node.attribute("waste_time_min").as_int() : 180);

		// ���� ��� �������
		const auto tmp_shop = std::make_shared<shop_node>();
		tmp_shop->id = shop_id;
		tmp_shop->currency = currency;
		tmp_shop->profit = profit;
		tmp_shop->can_buy = shop_can_buy;
		tmp_shop->waste_time_min = store_time_min;
		//��������� ������
		tmp_shop->SetDictionaryName(shop_id);//� ���� � ��������� ��������
		tmp_shop->SetDictionaryTID(shop_id);

		std::map<int, std::string> mob_to_template;

		for (pugi::xml_node mob = node.child("mob"); mob; mob=mob.next_sibling("mob"))
		{
			int mob_vnum = Parse::attr_int(mob, "mob_vnum");
			std::string templateId = mob.attribute("template").value();
			if (mob_vnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop attributes (mob_vnum=%d shop id=%s)", mob_vnum, shop_id.c_str());
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			if (!templateId.empty())
			{
				mob_to_template[mob_vnum] = templateId;
			}

			tmp_shop->add_mob_vnum(mob_vnum);
			// ��������� � ����� ���� �������
			// ���� ���� ������ ����� �� ���������� - ��� ����� �������� ������ �� ���������� ��������
			int mob_rnum = real_mobile(mob_vnum);
			if (mob_rnum >= 0)
			{
				if (mob_index[mob_rnum].func
					&& mob_index[mob_rnum].func != shop_ext)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"...shopkeeper already with special (mob_vnum=%d)", mob_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				}
				else
				{
					mob_index[mob_rnum].func = shop_ext;
				}
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect mob_vnum=%d", mob_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			}
		}

		// � ������ ��� ���������
		for (pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = Parse::attr_int(item, "vnum");
			int price = Parse::attr_int(item, "price");
			std::string items_template = item.attribute("template").value();
			if (item_vnum < 0
				|| price < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop attributes (item_vnum=%d, price=%d)", item_vnum, price);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			// ��������� ������
			int item_rnum = real_object(item_vnum);
			if (item_rnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d", item_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			// ���� �� � ������
			const auto item_price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price; //���� �� ������� ���� - ����� ���� �� ���������
			tmp_shop->add_item(item_vnum, item_price);
		}

		//� ��� ������� ������
		for (pugi::xml_node itemSet = node.child("item_set"); itemSet; itemSet = itemSet.next_sibling("item_set"))
		{
			std::string itemSetId = itemSet.child_value();
			for (ItemSetListType::const_iterator it = itemSetList.begin(); it != itemSetList.end();++it)
			{
				if ((*it)->_id == itemSetId)
				{
					for (unsigned i = 0; i < (*it)->item_list.size(); i++)
					{
						// ��������� ������
						int item_rnum = real_object((*it)->item_list[i].item_vnum);
						if (item_rnum < 0)
						{
							snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d in item_set=%s", (int)(*it)->item_list[i].item_vnum, (*it)->_id.c_str());
							mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
							return;
						}
						// ���� �� � ������
						const auto item_vnum = (*it)->item_list[i].item_vnum;
						const int price = (*it)->item_list[i].item_price;
						const auto item_price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price;
						tmp_shop->add_item(item_vnum, item_price);
					}
				}
			}
		}

		if (tmp_shop->empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...item list empty (shop_id=%s)", shop_id.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}

		const auto& items = tmp_shop->items_list();
		for (auto index = 0; index < items.size(); ++index)
		{
			const auto& item = items.node(index);
			for (auto id = item_descriptions.begin(); id != item_descriptions.end(); ++id)
			{
				if (id->second.find(item->vnum()) != id->second.end())
				{
					item_desc_node tmp_desc(id->second[item->vnum()]);
					for (const auto& mob_vnum : tmp_shop->mob_vnums())
					{
						if (mob_to_template.find(mob_vnum) != mob_to_template.end()
							&& mob_to_template[mob_vnum] == id->first)
						{
							item->add_desc(mob_vnum, tmp_desc);
						}
					}
				}
			}
		}

		shop_list.push_back(tmp_shop);
    }

    log_shop_load();
}

long get_sell_price(OBJ_DATA * obj)
{
	long cost = GET_OBJ_COST(obj);
	long cost_obj = GET_OBJ_COST(obj);
	int timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
	if (timer < obj->get_timer())
	    obj->set_timer(timer);
	cost = timer <= 0 ? 1 : (long)cost * ((float)obj->get_timer() / (float)timer); //����� ������

	// ���� ���� �������, ����, ��� ��������� ��������
	if (cost > cost_obj)
	{
		cost = cost_obj;
	}
	return MMAX(1, cost);
}

int get_spent_today()
{
	return spent_today;
}

} // namespace ShopExt

using namespace ShopExt;

int shop_ext(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	if (!ch->desc
		|| IS_NPC(ch))
	{
		return 0;
	}

	if (!(CMD_IS("������")
		|| CMD_IS("list")
		|| CMD_IS("������")
		|| CMD_IS("buy")
		|| CMD_IS("�������")
		|| CMD_IS("sell")
		|| CMD_IS("�������")
		|| CMD_IS("value")
		|| CMD_IS("������")
		|| CMD_IS("repair")
		|| CMD_IS("�������")
		|| CMD_IS("steal")
		|| CMD_IS("�����������")
		|| CMD_IS("filter")
		|| CMD_IS("�����������")
		|| CMD_IS("examine")
		|| CMD_IS("��������������")
		|| CMD_IS("identify")))
	{
		return 0;
	}

	char argm[MAX_INPUT_LENGTH];
	CHAR_DATA * const keeper = reinterpret_cast<CHAR_DATA *>(me);
	shop_node::shared_ptr shop;
	for (const auto& s : shop_list)
	{
		const auto found = std::find(s->mob_vnums().begin(), s->mob_vnums().end(), GET_MOB_VNUM(keeper)) != s->mob_vnums().end();
		if (found)
		{
			shop = s;
			break;
		}
	}

	if (!shop)
	{
		log("SYSERROR : ������� �� ������ mob_vnum=%d (%s:%d)", GET_MOB_VNUM(keeper), __FILE__, __LINE__);
		send_to_char("�������� �����.\r\n", ch);

		return 1;
	}


	if (CMD_IS("steal")
		|| CMD_IS("�������"))
	{
		sprintf(argm, "$N ��������$g '%s'", MSG_NO_STEAL_HERE);
		sprintf(buf, "������ %s", GET_NAME(ch));
		do_social(keeper, buf);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return 1;
	}

	if (CMD_IS("������")
		|| CMD_IS("list"))
	{
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		shop->print_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("�����������")
		|| CMD_IS("filter"))
	{
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		shop->filter_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("������")
		|| CMD_IS("buy"))
	{
		shop->process_buy(ch, keeper, argument);
		return 1;
	}
	if (CMD_IS("��������������") || CMD_IS("identify"))
	{
		shop->process_ident(ch, keeper, argument, "��������������");
		return 1;
	}


	if (CMD_IS("value") || CMD_IS("�������"))
	{
		shop->process_cmd(ch, keeper, argument, "�������");
		return 1;
	}
	if (CMD_IS("�������") || CMD_IS("sell"))
	{
		shop->process_cmd(ch, keeper, argument, "�������");
		return 1;
	}
	if (CMD_IS("������") || CMD_IS("repair"))
	{
		shop->process_cmd(ch, keeper, argument, "������");
		return 1;
	}
	if (CMD_IS("�����������") || CMD_IS("examine"))
	{
		shop->process_ident(ch, keeper, argument, "�����������");
		return 1;
	}

	return 0;
}

// * ���� ������������� ��������� � ������ �����.
void town_shop_keepers()
{
	// ������ ��� ����������� ���, ����� �� ������� ���� � ����� �������� � ����
	std::set<int> zone_list;

	for (CHAR_DATA *ch = character_list; ch; ch = ch->get_next())
	{
		if (IS_RENTKEEPER(ch)
			&& ch->in_room > 0
			&& !Clan::GetClanByRoom(ch->in_room)
			&& !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
			&& GET_ROOM_VNUM(ch->in_room) % 100 != 99
			&& zone_list.find(world[ch->in_room]->zone) == zone_list.end())
		{
			int rnum_start, rnum_end;
			if (get_zone_rooms(world[ch->in_room]->zone, &rnum_start, &rnum_end))
			{
				CHAR_DATA *mob = read_mobile(1901, VIRTUAL);
				if (mob)
				{
					char_to_room(mob, number(rnum_start, rnum_end));
				}
			}
			zone_list.insert(world[ch->in_room]->zone);
		}
	}
}

void do_shops_list(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	DictionaryPtr dic = DictionaryPtr(new Dictionary(SHOP));
	size_t n = dic->Size();
	std::string out;
	for (size_t i = 0; i < n; i++)
	{
		out = out + boost::lexical_cast<std::string>(i + 1) + " " + dic->GetNameByNID(i) + " " + dic->GetTIDByNID(i) + "\r\n";
	}
	send_to_char(out, ch);
}

void fill_shop_dictionary(DictionaryType &dic)
{
	ShopExt::ShopListType::const_iterator it = ShopExt::shop_list.begin();
	for (;it != ShopExt::shop_list.end();++it)
		dic.push_back((*it)->GetDictionaryItem());
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
