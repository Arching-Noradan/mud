// title.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include <boost/algorithm/string.hpp>
#include <sstream>
#include "title.hpp"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "pk.h"

namespace TitleSystem {

// ��� ������ �������, ������ ������������� �������� ��� ��������� ������
struct waiting_title
{
	std::string title;     // �����
	std::string pre_title; // ���������
	long unique;           // ��� ������ (��� ����� �������)
};

const unsigned int MAX_TITLE_LENGTH = 80; // ����.����� ������ ������ (�����+���������)
const int SET_TITLE_COST = 1000;          // ���� �� ������� ��������� ������
const char* TITLE_FILE = LIB_PLRSTUFF"titles.lst"; // ���� ����������/��������� ������ ��������� �������
const char* MORTAL_DO_TITLE_FORMAT =
	"����� - ������� � ������� � ���������� �� ������, ������������ �� ������������ ��� ������� ������ �������������\r\n"
	"����� ���������� <�����> - ��������������� ��������� ������ ������, ������� �������������\r\n"
	"      ������ � �����: '����� ���������� �������� �����' -> ����, �������� �����\r\n"
	"����� ���������� <����� ����������!����� ������> - ����, ��� � � �������\r\n"
	"      ������ � �����: '����� ���������� ���������!�������� �����' -> ��������� ����, �������� �����\r\n"
	"����� �������� - ������������� � �������� ������ �����\r\n"
	"����� �������� - ������ �������� ����� ������ �� ������ ������������ �����\r\n"
	"����� ������� - ������� ���� ������� ����� � ���������\r\n";
const char* GOD_DO_TITLE_FORMAT =
	"����� - ����� ������� ������ ��� ���� ������� � ������ �� ����������\r\n"
	"����� ��������/��������� ��� - ��������� ��� ����� � ������ ������ �� ������\r\n"
	"����� ���������� <����� ������> - ��������� ������ ������\r\n"
	"����� ���������� <����� ����������!����� ������> - ��������� ������ ���������� � ������\r\n"
	"����� ������� - ������� ���� ������� ����� � ���������\r\n";
const char* TITLE_SEND_FORMAT = "�� ������ ��������� �������� ��, �������� �����, ��� �������� �������� '����� ��������'\r\n";

typedef boost::shared_ptr<waiting_title> WaitingTitlePtr;
typedef std::map<std::string, WaitingTitlePtr> TitleListType;
TitleListType title_list;
TitleListType temp_title_list;

bool check_title(const std::string& text, CHAR_DATA* ch);
bool check_pre_title(std::string text, CHAR_DATA* ch);
bool check_alphabet(const std::string& text, CHAR_DATA* ch, const std::string& allowed);
bool is_new_petition(CHAR_DATA* ch);
void manage_title_list(std::string& name, bool action, CHAR_DATA* ch);
void set_player_title(CHAR_DATA* ch, const std::string& pre_title, const std::string& title, const char* god);
const char* print_help_string(CHAR_DATA* ch);
std::string print_agree_string(CHAR_DATA* ch, bool new_petittion);
std::string print_title_string(CHAR_DATA* ch, const std::string& pre_title, const std::string& title);
std::string print_title_string(const std::string& name, const std::string& pre_title, const std::string& title);

} // namespace TitleSystem

/**
* ������� �����, title. ACMD(do_title), ��� ������� � ��� ����� ��� ����� ��������.
*/
void TitleSystem::do_title(CHAR_DATA *ch, char *argument, int cmd, int subcmd)
{
	if (IS_NPC(ch)) return;

	std::string buffer(argument), buffer2;
	GetOneParam(buffer, buffer2);

	if (buffer2.empty()) {
		if (IS_IMMORTAL(ch) && !title_list.empty())
			show_title_list(ch);
		else {
			std::stringstream out;
			TitleListType::iterator it = title_list.find(GET_NAME(ch));
			if (it != title_list.end()) {
				out << "������ ������ ��������� � ������������ � �����:\r\n"
					<< print_title_string(ch, it->second->pre_title, it->second->title)
					<< TITLE_SEND_FORMAT << "\r\n";
			}
			it = temp_title_list.find(GET_NAME(ch));
			if (it != temp_title_list.end()) {
				out << "������ ����� ���� ������ ������������� ��� �������� ������ �����:\r\n"
					<< print_title_string(ch, it->second->pre_title, it->second->title)
					<< print_agree_string(ch, is_new_petition(ch)) << "\r\n";
			}
			out << print_help_string(ch);
			send_to_char(out.str(), ch);
		}
	} else if (IS_IMMORTAL(ch) && CompareParam(buffer2, "��������")) {
		manage_title_list(buffer, 1, ch);
	} else if (IS_IMMORTAL(ch) && CompareParam(buffer2, "���������")) {
		manage_title_list(buffer, 0, ch);
	} else if (CompareParam(buffer2, "����������")) {
		boost::trim(buffer);
		if (buffer.size() > MAX_TITLE_LENGTH) {
			send_to_char(ch, "����� ������ ���� �� ������� %d ��������.\r\n", MAX_TITLE_LENGTH);
			return;
		}

		std::string pre_title, title;
		std::string::size_type beg_idx = buffer.find("!");
		if (beg_idx != std::string::npos) {
			pre_title = buffer.substr(0, beg_idx);
			beg_idx = buffer.find_first_not_of("!", beg_idx);
			if (beg_idx != std::string::npos)
				title = buffer.substr(beg_idx);
			boost::trim(title);
			boost::trim(pre_title);
			if (!pre_title.empty() && !check_pre_title(pre_title, ch)) return;
			if (!title.empty() && !check_title(title, ch)) return;
		} else {
			if (!check_title(buffer, ch)) return;
			title = buffer;
		}
		if (IS_IMMORTAL(ch)) {
			set_player_title(ch, pre_title, title, GET_NAME(ch));
			send_to_char("����� ����������\r\n", ch);
			return;
		}
		if (title.empty() && pre_title.empty()) {
			send_to_char("�� ������ ������ �������� ����� ��� ���������.\r\n", ch);
			return;
		}

		bool new_petition = is_new_petition(ch);
		WaitingTitlePtr temp(new waiting_title);
		temp->title = title;
		temp->pre_title = pre_title;
		temp->unique = GET_UNIQUE(ch);
		temp_title_list[GET_NAME(ch)] = temp;

		std::stringstream out;
		out << "��� ����� ����� ��������� ��������� �������:\r\n" << CCPK(ch, C_NRM, ch);
		out << print_title_string(ch, pre_title, title) << print_agree_string(ch, new_petition);
		send_to_char(out.str(), ch);
		return;
	} else if (!IS_IMMORTAL(ch) && CompareParam(buffer2, "��������")) {
		TitleListType::iterator it = temp_title_list.find(GET_NAME(ch));
		if (it != temp_title_list.end()) {
			if (GET_BANK_GOLD(ch) < SET_TITLE_COST) {
				send_to_char("�� ����� ����� �� ������� ����� ��� ������ ���� ������.\r\n", ch);
				return;
			}
			GET_BANK_GOLD(ch) -= SET_TITLE_COST;
			title_list[it->first] = it->second;
			temp_title_list.erase(it);
			send_to_char("���� ������ ���������� ����� � ����� ����������� � ��������� �����.\r\n", ch);
			send_to_char(TITLE_SEND_FORMAT, ch);
		} else
			send_to_char("� ������ ������ ��� ������, �� ������� ��������� ���� ��������.\r\n", ch);
	} else if (!IS_IMMORTAL(ch) && CompareParam(buffer2, "��������")) {
		TitleListType::iterator it = temp_title_list.find(GET_NAME(ch));
		if (it != temp_title_list.end()) {
			temp_title_list.erase(it);
			send_to_char("���� ������ �� ����� ��������.\r\n", ch);
		} else
			send_to_char("� ������ ������ ��� ������ ��������.\r\n", ch);
	} else if (CompareParam(buffer2, "�������", 1)) {
		if (GET_TITLE(ch)) {
			free(GET_TITLE(ch));
			GET_TITLE(ch) = 0;
		}
		send_to_char("���� ����� � ��������� �������.\r\n", ch);
	} else
		send_to_char(print_help_string(ch), ch);
}

/**
* �������� �� ���������� ������ � �������
* ��� ������� 25+ ����� ��� ������� �������, ����� �� ��������
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_title(const std::string& text, CHAR_DATA* ch)
{
	if (!check_alphabet(text, ch, " ,.-?")) return 0;

	if (GET_LEVEL(ch) < 25 && !GET_REMORT(ch) && !IS_IMMORTAL(ch)) {
		send_to_char(ch, "��� ����� �� ����� �� ������ ���������� 25�� ������ ��� ����� ��������������.\r\n");
		return 0;
	}

	return 1;
}

/**
* �������� �� ���������� ������ � �����������.
* ��� ������� ���������� ������� �������� ��� ������� ����� (����� - ��� 4+ ����) � ����������,
* �� 3 ���� - ������� ��� �������. �� ����� 3 ���� � 3 ���������, ����� �� ��������.
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_pre_title(std::string text, CHAR_DATA* ch)
{
	if (!check_alphabet(text, ch, " .-?")) return 0;

	if (IS_IMMORTAL(ch)) return 1;

	if (!GET_REMORT(ch)) {
		send_to_char(ch, "�� ������ ����� �� ������� ���� ���� �������������� ��� ����������.\r\n");
		return 0;
	}

	int word = 0, prep = 0;
	typedef boost::split_iterator<std::string::iterator> split_it;
	for(split_it it = boost::make_split_iterator(text, boost::first_finder(" ", boost::is_iequal())); it != split_it(); ++it) {
		if (boost::copy_range<std::string>(*it).size() > 3)
			++word;
		else
			++prep;
	}
	if (word > 3 || prep > 3) {
		send_to_char(ch, "������� ����� ���� � ����������.\r\n");
		return 0;
	}
	if (word > GET_REMORT(ch)) {
		send_to_char(ch, "� ��� ������������ �������������� ��� �������� ���� � ����������.\r\n");
		return 0;
	}

	return 1;
}

/**
* �������� �� ���������� ������ � ����� �������������� � ������� �������� � �������� �� ������ �� �����
* \param allowed - ������ � ����������� ��������� ������ �������� �������� (��� ������ � ���������� ������)
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_alphabet(const std::string& text, CHAR_DATA* ch, const std::string& allowed)
{
	int i = 0;
	std::string::size_type idx;
	for (std::string::const_iterator it = text.begin(); it != text.end(); ++it, ++i) {
		unsigned char c = static_cast<char>(*it);
		idx = allowed.find(*it);
		if (c < 192 && idx == std::string::npos) {
			send_to_char(ch, "������������ ������ '%c' � ������� %d.\r\n", *it, ++i);
			return 0;
		}
	}
	return 1;
}

/**
* �������� ������ �� ������ ������� � ����������/��������.
* ���������� ������ �� ���������, ���� �� ������. ������ � ����, ���� �������.
* \param action - 0 ������ ������, 1 ���������
*/
void TitleSystem::manage_title_list(std::string& name, bool action, CHAR_DATA* ch)
{
	std::string buffer;
	GetOneParam(name, buffer);
	name_convert(buffer);
	TitleListType::iterator it = title_list.find(buffer);
	if (it != title_list.end()) {
		if (action) {
			DESCRIPTOR_DATA* d = DescByUID(it->second->unique, 0);
			if (d) {
				set_player_title(d->character, it->second->pre_title, it->second->title, GET_NAME(ch));
				send_to_char(d->character, "��� ����� ��� ������� ������.\r\n");
			} else {
				CHAR_DATA* victim;
				CREATE(victim, CHAR_DATA, 1);
				clear_char(victim);
				if (load_char(it->first.c_str(), victim) < 0) {
					send_to_char("�������� ��� ������ ��� �������� �����-�� �����.\r\n", ch);
					free(victim);
					title_list.erase(it);
					return;
				}
				set_player_title(victim, it->second->pre_title, it->second->title, GET_NAME(ch));
				save_char(victim, GET_LOADROOM(victim));
				free_char(victim);
			}
			send_to_char("����� �������.\r\n", ch);
		} else
			send_to_char("����� ��������.\r\n", ch);
		title_list.erase(it);
	} else
		send_to_char("� ������ ��� ��������� � ����� ������.\r\n", ch);
}

/**
* ����� ����� �������, ������ ���������
*/
void TitleSystem::show_title_list(CHAR_DATA* ch)
{
	if (title_list.empty()) return 0;

	std::stringstream out;
	out << "\r\n������ ��������� ���� ��������� ������ (����� ��������/��������� <�����>):\r\n" << CCWHT(ch, C_NRM);
	for (TitleListType::const_iterator it = title_list.begin(); it != title_list.end(); ++it)
		out << print_title_string(it->first, it->second->pre_title, it->second->title);
	out << CCNRM(ch, C_NRM);
	send_to_char(out.str(), ch);
	return 1;
}

/**
* ���������� ������ ������ ���� ��� ��������� (��� �������� ���������)
*/
std::string TitleSystem::print_title_string(const std::string& name, const std::string& pre_title, const std::string& title)
{
	std::stringstream out;
	if (!pre_title.empty())
		out << pre_title << " ";
	out << name;
	if (!title.empty())
		out << ", "<< title;
	out << "\r\n";
	return out.str();
}

/**
* ���������� ������ ������, ��� ��� ����� ����� � ���� � ������ ����� ��
*/
std::string TitleSystem::print_title_string(CHAR_DATA* ch, const std::string& pre_title, const std::string& title)
{
	std::stringstream out;
	out << CCPK(ch, C_NRM, ch);
	if (!pre_title.empty())
		out << pre_title << " ";
	out << GET_NAME(ch);
	if (!title.empty())
		out << ", "<< title;
	out << CCNRM(ch, C_NRM) << "\r\n";
	return out.str();
}

/**
* ��������� ���� ������ (GET_TITLE(ch)) ������
* \param ch - ���� ������
* \param god - ��� ����������� ����� ����
*/
void TitleSystem::set_player_title(CHAR_DATA* ch, const std::string& pre_title, const std::string& title, const char* god)
{
	std::stringstream out;
	out << title;
	if (!pre_title.empty())
		out << ";" << pre_title;
	out << "/������� �����: " << god << "/";
	if (GET_TITLE(ch))
		free(GET_TITLE(ch));
	GET_TITLE(ch) = str_dup(out.str().c_str());
}

/**
* ��� ���������� ������ ��������� ���� � ������
*/
const char* TitleSystem::print_help_string(CHAR_DATA* ch)
{
	if (IS_IMMORTAL(ch))
		return GOD_DO_TITLE_FORMAT;

	return MORTAL_DO_TITLE_FORMAT;
}

/**
* ���������� ������ ������� �� ���������
*/
void TitleSystem::save_title_list()
{
	std::ofstream file(TITLE_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", TITLE_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	for (TitleListType::const_iterator it = title_list.begin(); it != title_list.end(); ++it)
		file << it->first << " " <<  it->second->unique << "\n" << it->second->pre_title << "\n" << it->second->title << "\n";
	file.close();
}

/**
* �������� ������ ������� �� ���������, ������ ������������ ������ ����� reload title
*/
void TitleSystem::load_title_list()
{
	title_list.clear();

	std::ifstream file(TITLE_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", TITLE_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	std::string name, pre_title, title;
	long unique;
	while (file >> name >> unique) {
		ReadEndString(file);
		std::getline(file, pre_title);
		std::getline(file, title);
		WaitingTitlePtr temp(new waiting_title);
		temp->title = title;
		temp->pre_title = pre_title;
		temp->unique = unique;
		title_list[name] = temp;
	}
	file.close();
}

/**
* ��� ������� ������� ����� ��� ��� ����� �� ������ (�� �����, ���� ��� ���� �������������� ������)
* \return 0 �� ������� ������ � ��������� ��� ���� ������, 1 ����� ������
*/
bool TitleSystem::is_new_petition(CHAR_DATA* ch)
{
	TitleListType::iterator it = temp_title_list.find(GET_NAME(ch));
	if (it != temp_title_list.end())
		return 0;
	return 1;
}

/**
* ��� ����������� ���������� ��� ��� � ������ ���� �� ������
* \param new_petition - 0 ���������� ������ ������, 1 ����� ������
*/
std::string TitleSystem::print_agree_string(CHAR_DATA* ch, bool new_petition)
{
	std::stringstream out;
	out << "��� ������������� ������ �������� '����� ��������'";
	if (new_petition)
		out << ", ��������� ������ " << SET_TITLE_COST << " ���.";
	out << "\r\n";
	return out.str();
}
