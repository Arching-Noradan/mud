// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su

#include "bonus.h"

#include "bonus.command.parser.hpp"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "modify.h"
#include "char.hpp"
#include "char_player.hpp"
#include "utils.h"

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

	void do_bonus_info(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
	{
		show_log(ch);
	}

	void setup_bonus(const int duration, const int multilpier, EBonusType type)
	{
		time_bonus = duration;
		mult_bonus = multilpier;
		type_bonus = type;
	}

	void bonus_log_add(const std::string& name)
	{
		time_t nt = time(nullptr);
		std::stringstream ss;
		ss << rustime(localtime(&nt)) << " " << name;

		bonus_log.push_back(ss.str());

		std::ofstream fout("../log/bonus.log", std::ios_base::app);
		fout << ss.str() << std::endl;
		fout.close();
	}

	void do_bonus(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
	{
		ArgumentsParser bonus(argument, type_bonus, time_bonus);

		bonus.parse();

		const auto result = bonus.result();
		switch (result)
		{
		case ArgumentsParser::ER_ERROR:
			send_to_char(bonus.error_message().c_str(), ch);
			break;

		case ArgumentsParser::ER_START:
			switch (bonus.type())
			{
			case BONUS_DAMAGE:
				send_to_char("����� ������ \"����\" � ��������� ����� ��������.", ch);
				break;

			default:
				{
					const std::string& message = bonus.broadcast_message();
					send_to_all(message.c_str());
					std::stringstream ss;
					ss << "&W" << message << "&n\r\n";
					bonus_log_add(ss.str());
					setup_bonus(bonus.time(), bonus.multiplier(), bonus.type());
				}
			}
			break;

		case ArgumentsParser::ER_STOP:
			send_to_all(bonus.broadcast_message().c_str());
			time_bonus = -1;
			break;
		}
	}

	// �������� ������� do_bonus
	void dg_do_bonus(char *cmd)
	{
		CHAR_DATA *ch = new CHAR_DATA();
		do_bonus(ch, cmd, 0, 0);
		extract_char(ch, 0);
	}

	// ���������� � ������ ������� �������� �� ����� ������
	std::string bonus_end()
	{
		std::stringstream ss;
		if (time_bonus > 4)
		{
			ss << "�� ����� ������ �������� " << time_bonus << " �����.";
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

		if (time_bonus <= -1)
		{
			return false;
		}

		if (type == type_bonus)
		{
			return true;
		}

		return false;
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
