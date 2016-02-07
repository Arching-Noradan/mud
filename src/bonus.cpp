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

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

namespace Bonus
{
	// ����� ������, � ���������� ��������� -1
	int time_bonus = -1;
	// ��������� ������
	int mult_bonus = 2;
	// ���� ������
	// 0 - ���������
	// 1 - ����
	// 2 - �����
	int type_bonus = BONUS_EXP;;
	// ������� �������
	std::vector<std::string> bonus_log;


	ACMD(do_bonus_info)
	{
		show_log(ch);
	}

	ACMD(do_bonus)
	{
		argument = two_arguments(argument, buf, buf2);
		std::string out = "&W*** ����������� ";

		if (!isname(buf, "������� ������� ��������"))
		{
			send_to_char("��������� �������:\r\n����� <�������|�������|��������> [���������|����] [�����]\r\n", ch);
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
			out += "������� ����� ";
			mult_bonus = 2;
		}
		else if (is_abbrev(buf, "�������"))
		{
			out += "������� ����� ";
			mult_bonus = 3;
		}
		else
		{
			return;
		}
		if (is_abbrev(buf2, "���������"))
		{
			out += "���������� ����� ";
			type_bonus = 0;
		}
		else if (is_abbrev(buf2, "����"))
		{
			out += "����� ";
			type_bonus = 1;
		}
		else if (is_abbrev(buf2, "�����"))
		{
			out += "������������ �����";
			type_bonus = 2;
		}
		else
		{
			return;
		}
		out += "�� " + boost::lexical_cast<string>(time_bonus) + " �����. ***&n\r\n";
		bonus_log_add(out);
		send_to_all(out.c_str());
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
		if (time_bonus > 4)
			sprintf(buf, "&W�� ����� ������ �������� %d �����.&n\r\n", time_bonus);
		else if (time_bonus == 4)
			sprintf(buf, "&W�� ����� ������ �������� ������ ����.&n\r\n");
		else if (time_bonus == 3)
			sprintf(buf, "&W�� ����� ������ �������� ��� ����.&n\r\n");
		else if (time_bonus == 2)
			sprintf(buf, "&W�� ����� ������ �������� ��� ����.&n\r\n");
		else
			sprintf(buf, "&W�� ����� ������ ������� ��������� ���!&n\r\n");
		send_to_all(buf);
	}

	// �������� �� ��� ������
	bool is_bonus(int type)
	{
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
		fout << std::string(rustime(localtime(&nt))) + " " + name;
		fout.close();
	}

	// ��������� ��� ������ �� �����
	void bonus_log_load()
	{
		std::ifstream fin("../log/bonus.log");
		if (!fin.is_open())
			return;
		std::string temp_buf;
		while (std::getline(fin, temp_buf))
			bonus_log.push_back(temp_buf);
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
		std::string buf_str = "";
		for (size_t i = bonus_log.size(); i >= 0; i--)
		{
			buf_str += bonus_log[i];
		}
		page_string(ch->desc, buf_str);
	}

	// ���������� ��������� ������
	int get_mult_bonus()
	{
		return mult_bonus;
	}
	


}