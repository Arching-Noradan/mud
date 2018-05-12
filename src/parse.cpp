// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "parse.hpp"

#include "object.prototypes.hpp"
#include "logger.hpp"
#include "db.h"
#include "obj.hpp"
#include "utils.h"
#include "conf.h"

#include <unordered_map>
#include <array>

namespace TextId
{

///
/// �������� ������ ������������ �����=������/������=����� ��� ��������
/// ������-������� �������� � ��������� �� � �������.
/// ����� ��� ������������� ������ �� ��������� ������� �������� � ����.
///
class TextIdNode
{
public:
	void add(int num, std::string str);

	std::string to_str(int num) const;
	int to_num(const std::string &str) const;

private:
	std::unordered_map<int, std::string> num_to_str;
	std::unordered_map<std::string, int> str_to_num;
};

// ����� ������ �������������� ��������
std::array<TextIdNode, TEXT_ID_COUNT> text_id_list;

///
/// ���� ��������� �� ������� ��� �������.
///
void init_char_class()
{
	text_id_list.at(CHAR_CLASS).add(CLASS_CLERIC, "CLASS_CLERIC");
	text_id_list.at(CHAR_CLASS).add(CLASS_BATTLEMAGE, "CLASS_BATTLEMAGE");
	text_id_list.at(CHAR_CLASS).add(CLASS_THIEF, "CLASS_THIEF");
	text_id_list.at(CHAR_CLASS).add(CLASS_WARRIOR, "CLASS_WARRIOR");
	text_id_list.at(CHAR_CLASS).add(CLASS_ASSASINE, "CLASS_ASSASINE");
	text_id_list.at(CHAR_CLASS).add(CLASS_GUARD, "CLASS_GUARD");
	text_id_list.at(CHAR_CLASS).add(CLASS_CHARMMAGE, "CLASS_CHARMMAGE");
	text_id_list.at(CHAR_CLASS).add(CLASS_DEFENDERMAGE, "CLASS_DEFENDERMAGE");
	text_id_list.at(CHAR_CLASS).add(CLASS_NECROMANCER, "CLASS_NECROMANCER");
	text_id_list.at(CHAR_CLASS).add(CLASS_PALADINE, "CLASS_PALADINE");
	text_id_list.at(CHAR_CLASS).add(CLASS_RANGER, "CLASS_RANGER");
	text_id_list.at(CHAR_CLASS).add(CLASS_SMITH, "CLASS_SMITH");
	text_id_list.at(CHAR_CLASS).add(CLASS_MERCHANT, "CLASS_MERCHANT");
	text_id_list.at(CHAR_CLASS).add(CLASS_DRUID, "CLASS_DRUID");
}

///
/// ���� ��������� �� ���������� ��������� ��� ���������� � ����.
///
void init_obj_vals()
{
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_NUM), "POTION_SPELL1_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_LVL), "POTION_SPELL1_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_NUM), "POTION_SPELL2_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_LVL), "POTION_SPELL2_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_NUM), "POTION_SPELL3_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_LVL), "POTION_SPELL3_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_PROTO_VNUM), "POTION_PROTO_VNUM");
}

///
/// ����� ���� ������� ��������� ��, ��������� ��� ������ ����.
///
void init()
{
	/// CHAR_CLASS
	init_char_class();
	/// OBJ_VALS
	init_obj_vals();
	/// ...
}

///
/// ��������������� ���������� �� ��������� � �� �������� � ����.
/// \return �������� ��������� ��� -1, ���� ������ �� ���� �������
///
int to_num(IdType type, const std::string &str)
{
	if (type < TEXT_ID_COUNT)
	{
		return text_id_list.at(type).to_num(str);
	}
	return -1;
}

///
/// ��������������� �������� ��������� � �� ��������� ��.
/// \return ��������� �� ��������� ��� ������ ������, ���� ������ �� ���� �������
///
std::string to_str(IdType type, int num)
{
	if (type < TEXT_ID_COUNT)
	{
		return text_id_list.at(type).to_str(num);
	}
	return "";
}

///
/// ���������� ������������ ��������=���������/���������=��������
///
void TextIdNode::add(int num, std::string str)
{
	num_to_str.insert(std::make_pair(num, str));
	str_to_num.insert(std::make_pair(str, num));
}

///
/// ��������������� �������� -> ��������� ��
///
std::string TextIdNode::to_str(int num) const
{
	auto i = num_to_str.find(num);
	if (i != num_to_str.end())
	{
		return i->second;
	}
	return "";
}

///
/// ��������������� ��������� �� -> ��������
///
int TextIdNode::to_num(const std::string &str) const
{
	auto i = str_to_num.find(str);
	if (i != str_to_num.end())
	{
		return i->second;
	}
	return -1;
}

} // namespace TextId

namespace Parse
{

///
/// ������� ��������������� \param text � <int> � ���������� ����������
/// \return ����� ��� -1, � ������ �������
///
int cast_to_int(const char *text)
{
	int result = -1;

	try
	{
		result = std::stoi(text, nullptr, 10);
	}
	catch(...)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...lexical_cast<int> fail (value='%s')", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}

	return result;
}

///
/// ������� �� pugixml ��� ������ ��������� ���������
/// � ����������� � ���- � ������
/// � ������� ��� �������� ��� <param value="1234" />
/// \return -1 � ������ �������
int attr_int(const pugi::xml_node &node, const char *text)
{
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s read fail", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	return cast_to_int(attr.value());
}

// ���� �����, ��� � attr_int, ������, ���� �������� ���, ���������� -1
int attr_int_t(const pugi::xml_node &node, const char *text)
{
        pugi::xml_attribute attr = node.attribute(text);
        if (!attr)
        {
		return -1;
        }
        return cast_to_int(attr.value());
}


///
/// ����, ��� � attr_int, ������ ��� ������ child_value()
/// � ������� ��� �������� ��� <param>1234<param>
///
int child_value_int(const pugi::xml_node &node, const char *text)
{
	pugi::xml_node child_node = node.child(text);
	if (!child_node)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s read fail", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	return cast_to_int(child_node.child_value());
}

///
/// ������ attr_int, \return ������ �� ��������� ��� ������ ������
///
std::string attr_str(const pugi::xml_node &node, const char *text)
{
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s read fail", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	else
	{
		return attr.value();
	}
	return "";
}

///
/// ������ child_value_int, \return ������ �� ��������� ��� ������ ������
///
std::string child_value_str(const pugi::xml_node &node, const char *text)
{
	pugi::xml_node child_node = node.child(text);
	if (!child_node)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s read fail", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	else
	{
		return child_node.child_value();
	}
	return "";
}

template <class T>
pugi::xml_node get_child_template(const T &node, const char *name)
{
	pugi::xml_node tmp_node = node.child(name);
	if (!tmp_node)
	{
		char tmp[100];
		snprintf(tmp, sizeof(tmp), "...<%s> read fail", name);
		mudlog(tmp, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	return tmp_node;
}

pugi::xml_node get_child(const pugi::xml_document &node, const char *name)
{
	return get_child_template(node, name);
}

pugi::xml_node get_child(const pugi::xml_node &node, const char *name)
{
	return get_child_template(node, name);
}

///
/// �������� ���������� ����� ������� � ������������ ������ � ��� � ������
/// \return true - ���� ���� �������� ������� (����) � ������ ������
///
bool valid_obj_vnum(int vnum)
{
	if (real_object(vnum) < 0)
	{
		snprintf(buf, sizeof(buf), "...bad obj vnum (%d)", vnum);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return false;
	}

	return true;
}

} // namespace Parse

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
