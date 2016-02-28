// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su

#include "bonus.h"

#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "modify.h"
#include "char.hpp"
#include "char_player.hpp"

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>

namespace Bonus
{
	const size_t MAXIMUM_BONUS_RECORDS = 10;

	// ����� ������, � ���������� ��������� -1
	int time_bonus = -1;

	// ��������� ������
	int mult_bonus = 2;

	// ���� ������
	// 0 - ���������
	// 1 - ����
	// 2 - �����
	EBonusType type_bonus = BONUS_EXP;

	// ������� �������
	typedef std::list<std::string> bonus_log_t;
	bonus_log_t bonus_log;

	ACMD(do_bonus_info)
	{
		show_log(ch);
	}

	ACMD(do_bonus)
	{
		argument = two_arguments(argument, buf, buf2);
		std::string out = "*** ����������� ";

		if (!isname(buf, "������� ������� ��������"))
		{
			send_to_char("��������� �������:\r\n����� <�������|�������|��������> [���������|����|����] [�����]\r\n", ch);
			return;
		}
		if (is_abbrev(buf, "��������"))
		{
			sprintf(buf, "����� ��� �������.\r\n");
			send_to_all(buf);
			time_bonus = -1;
			return;
		}
		if (!*buf || !*buf2 || !a_isascii(*buf2))
		{
			send_to_char("��������� �������:\r\n����� <�������|�������|��������> [���������|����|����] [�����]\r\n", ch);
			return;
		}
		if (!isname(buf2, "��������� ���� ����"))
		{
			send_to_char("��� ������ ����� ���� &W���������&n, &W����&n ��� &W����&n.\r\n", ch);
			return;
		}
		if (*argument) time_bonus = atol(argument);

		if ((time_bonus < 1) || (time_bonus > 60))
		{
			send_to_char("��������� ��������� ��������: �� 1 �� 60 ������� �����.\r\n", ch);
			return;
		}
		if (is_abbrev(buf, "�������"))
		{
			out += "������� �����";
			mult_bonus = 2;
		}
		else if (is_abbrev(buf, "�������"))
		{
			out += "������� �����";
			mult_bonus = 3;
		}
		else
		{
			return;
		}
		if (is_abbrev(buf2, "���������"))
		{
			out += " ���������� �����";
			type_bonus =  BONUS_WEAPON_EXP;
		}
		else if (is_abbrev(buf2, "����"))
		{
			out += " �����";
			type_bonus = BONUS_EXP;
		}
		else if (is_abbrev(buf2, "����"))
		{
			out += " ������������ �����";
			type_bonus = BONUS_DAMAGE;
		}
		else
		{
			return;
		}
		out += " �� " + boost::lexical_cast<string>(time_bonus) + " �����. ***";
		bonus_log_add(out);
		send_to_all(("&W" + out + "&n\r\n").c_str());
	}

	// ���������� � ������ ������� �������� �� ����� ������
	std::string bonus_end()
	{
		std::stringstream ss;
		if (time_bonus > 4)
		{
			ss << "�� ����� ������ �������� " << time_bonus <<" �����.";
		}
		else if (time_bonus == 4)
		{
			ss << "�� ����� ������ �������� ������ ����.";
		}
		else if (time_bonus == 3)
		{
			ss << "�� ����� ������ �������� ��� ����.";
		}
		else if (time_bonus == 2)
		{
			ss << "�� ����� ������ �������� ��� ����.";
		}
		else if (time_bonus == 1)
		{
			ss << "�� ����� ������ ������� ��������� ���!";
		}
		else
		{
			ss << "������ ���.";
		}
		return ss.str();
	}

	// ���������� � ����� ��� ������
	std::string str_type_bonus()
	{
		switch (type_bonus)
		{
		case BONUS_DAMAGE:
			return "������ ���� �����: ���������� ����.";

		case BONUS_EXP:
			return "������ ���� �����: ���������� ���� �� �������� ����.";

		case BONUS_WEAPON_EXP:
			return "������ ���� �����: ���������� ���� �� ����� ������.";

		default:
			return "";
		}
	}


	// ������ ������
	void timer_bonus()
	{
		if (time_bonus <= -1)
		{
			return;
		}
		time_bonus--;
		if (time_bonus < 1)
		{
			send_to_all("&W����� ����������...&n\r\n");
			time_bonus = -1;
			return;
		}
		std::string bonus_str = "&W" + bonus_end() + "&n\r\n";
		send_to_all(bonus_str.c_str());
	}

	// �������� �� ��� ������
	bool is_bonus(int type)
	{
		if (type == 0)
		{
			return time_bonus <= -1 ? false : true;
		}
		if (time_bonus <= -1) return false;
		if (type == type_bonus) return true;
		return false;
	}

	// ��������� ������ � ���
	void bonus_log_add(std::string name)
	{
		time_t nt = time(NULL);
		bonus_log.push_back(std::string(rustime(localtime(&nt))) + " " + name);
		std::ofstream fout("../log/bonus.log", std::ios_base::app);
		fout << std::string(rustime(localtime(&nt))) + " " + name << std::endl;
		fout.close();
	}

	// ��������� ��� ������ �� �����
	void bonus_log_load()
	{
		std::ifstream fin("../log/bonus.log");
		if (!fin.is_open())
		{
			return;
		}
		std::string temp_buf;
		bonus_log.clear();
		while (std::getline(fin, temp_buf))
		{
			bonus_log.push_back(temp_buf);
		}
		fin.close();
	}

	// ������� ���� ��� � �������� �������
	void show_log(CHAR_DATA *ch)
	{
		if (bonus_log.size() == 0)
		{
			send_to_char(ch, "��� ������!\r\n");
			return;
		}

		size_t counter = 0;
		std::stringstream buf_str;
		std::list<bonus_log_t::const_reverse_iterator> to_output;
		for (bonus_log_t::const_reverse_iterator i = bonus_log.rbegin(); i != bonus_log.rend() && MAXIMUM_BONUS_RECORDS > counter; ++i, ++counter)
		{
			to_output.push_front(i);
		}
		counter = 0;
		for (const auto i : to_output)
		{
			buf_str << "&G" << ++counter << ". &W" << *i << "&n\r\n";
		}
		page_string(ch->desc, buf_str.str());
	}

	// ���������� ��������� ������
	int get_mult_bonus()
	{
		return mult_bonus;
	}
	


}