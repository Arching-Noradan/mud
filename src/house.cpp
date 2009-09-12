/* ****************************************************************************
* File: house.cpp                                              Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include "conf.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sys/stat.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "house.h"
#include "comm.h"
#include "handler.h"
#include "pk.h"
#include "screen.h"
#include "boards.h"
#include "skills.h"
#include "spells.h"
#include "privilege.hpp"
#include "char.hpp"
#include "char_player.hpp"

extern void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int file_to_string_alloc(const char *name, char **buf);
extern struct player_index_element *player_table;
// TODO: ������ ���� � ����, ��� ��������� ������� �� ������, ��� ������� �������� �� ��� ������ �����, ��� ��� � ��������
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
extern int top_of_p_table;
extern const char *apply_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *weapon_class[];
extern const char *weapon_affects[];
extern const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
extern void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch);
extern void mort_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch, int fullness);

// vnum ��������� �������
static const int CLAN_CHEST_VNUM = 330;
static int CLAN_CHEST_RNUM = -1;

long long clan_level_exp [MAX_CLANLEVEL+1] =
{
	0LL,
	100000000LL, /* 1 level, should be achieved fast, 100M imho is possible*/
	1000000000LL, /* 1bilion. OMG. */
	5000000000LL, /* 5bilions. */
	15000000000LL, /* 15billions. */
	1000000000000LL /* BIG NUMBER. */
};

inline bool Clan::SortRank::operator()(const CHAR_DATA * ch1, const CHAR_DATA * ch2)
{
	return CLAN_MEMBER(ch1)->rank_num < CLAN_MEMBER(ch2)->rank_num;
}

ClanListType Clan::ClanList;

// ����� to_room � ����� ����-������, ���������� �� �����, ���� �������
room_rnum Clan::CloseRent(room_rnum to_room)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[to_room]->zone == world[real_room((*clan)->rent)]->zone)
			return real_room((*clan)->out_rent);
	return to_room;
}

// ��������� ��������� �� ��� � ���� ������ �����
bool Clan::InEnemyZone(CHAR_DATA * ch)
{
	int zone = world[IN_ROOM(ch)]->zone;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (zone == world[real_room((*clan)->rent)]->zone)
		{
			if (CLAN(ch) && (CLAN(ch) == *clan || (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE))
				return 0;
			else
				return 1;
		}
	return 0;
}

Clan::Clan()
		: guard(0), builtOn(time(0)), bankBuffer(0), entranceMode(0), bank(2000), exp(0), clan_exp(0),
		exp_buf(0), clan_level(0), rent(0), out_rent(0), chest_room(0), storehouse(1), exp_info(1),
		test_clan(0), chest_objcount(0), chest_discount(0), chest_weight(0)
{

}

Clan::~Clan()
{

}

// ����/������ ������� � ������ ������
void Clan::ClanLoad()
{
	init_chest_rnum();
	// �� ������ �������
	Clan::ClanList.clear();

	// ���� �� ������� ������
	std::ifstream file(LIB_HOUSE "index");
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer;
	std::list<std::string> clanIndex;
	while (file >> buffer)
		clanIndex.push_back(buffer);
	file.close();
	// ���������� ������ �����
	for (std::list < std::string >::const_iterator it = clanIndex.begin(); it != clanIndex.end(); ++it)
	{
		ClanPtr tempClan(new Clan);

		std::string filename = LIB_HOUSE + *it + "/" + *it;
		std::ifstream file(filename.c_str());
		if (!file.is_open())
		{
			log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		while (file >> buffer)
		{
			if (buffer == "Abbrev:")
			{
				if (!(file >> tempClan->abbrev))
				{
					log("Error open 'Abbrev:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Name:")
			{
				std::getline(file, buffer);
				boost::trim(buffer);
				tempClan->name = buffer;
			}
			else if (buffer == "Title:")
			{
				std::getline(file, buffer);
				boost::trim(buffer);
				tempClan->title = buffer;
			}
			else if (buffer == "Rent:")
			{
				int rent = 0;
				if (!(file >> rent))
				{
					log("Error open 'Rent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// ���� ����� � �� ����
				if (!real_room(rent))
				{
					log("Room %d is no longer exist (%s).", rent, filename.c_str());
					break;
				}
				tempClan->rent = rent;
			}
			else if (buffer == "OutRent:")
			{
				int out_rent = 0;
				if (!(file >> out_rent))
				{
					log("Error open 'OutRent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// ���� ����� � �� ����
				if (!real_room(out_rent))
				{
					log("Room %d is no longer exist (%s).", out_rent, filename.c_str());
					break;
				}
				tempClan->out_rent = out_rent;
			}
			else if (buffer == "ChestRoom:")
			{
				int chest_room = 0;
				if (!(file >> chest_room))
				{
					log("Error open 'OutRent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// ���� ����� � �� ����
				if (!real_room(chest_room))
				{
					log("Room %d is no longer exist (%s).", chest_room, filename.c_str());
					break;
				}
				tempClan->chest_room = chest_room;
			}
			else if (buffer == "Guard:")
			{
				int guard = 0;
				if (!(file >> guard))
				{
					log("Error open 'Guard:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// ��� � ���������
				if (real_mobile(guard) < 0)
				{
					log("Guard %d is no longer exist (%s).", guard, filename.c_str());
					break;
				}
				tempClan->guard = guard;
			}
			else if (buffer == "BuiltOn:")
			{
				if (!(file >> tempClan->builtOn))
				{
					log("Error open 'BuiltOn:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Ranks:")
			{
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				unsigned long priv = 0;

				std::string buffer2;
				// ��� �������� ������� ��������� ��� ����������, ������ ������ � ������
				if (!(stream >> buffer >> buffer2 >> priv))
				{
					log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				tempClan->ranks.push_back(buffer);
				tempClan->ranks_female.push_back(buffer2);
				tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
				for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
					tempClan->privileges[0].set(i);

				while (stream >> buffer)
				{
					if (!(stream >> buffer2 >> priv))
					{
						log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
						break;
					}
					tempClan->ranks.push_back(buffer);
					tempClan->ranks_female.push_back(buffer2);
					// �� ������ ���������� ����������
					if (priv > tempClan->privileges[0].to_ulong())
						priv = tempClan->privileges[0].to_ulong();
					tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (priv));
				}
			}
			else if (buffer == "Politics:")
			{
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				int room = 0;
				int state = 0;

				while (stream >> room >> state)
					tempClan->politics[room] = state;
			}
			else if (buffer == "EntranceMode:")
			{
				if (!(file >> tempClan->entranceMode))
				{
					log("Error open 'EntranceMode:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Storehouse:")
			{
				if (!(file >> tempClan->storehouse))
				{
					log("Error open 'Storehouse:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "ExpInfo:")
			{
				if (!(file >> tempClan->exp_info))
				{
					log("Error open 'ExpInfo:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "TestClan:")
			{
				if (!(file >> tempClan->test_clan))
				{
					log("Error open 'TestClan:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Exp:")
			{
				if (!(file >> tempClan->exp))
				{
					log("Error open 'Exp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "ExpBuf:")
			{
				if (!(file >> tempClan->exp_buf))
				{
					log("Error open 'ExpBuf:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Bank:")
			{
				file >> tempClan->bank;
				if (tempClan->bank <= 0)
				{
					log("Clan has 0 in bank, remove from list (%s).", filename.c_str());
					break;
				}
			}
			else if (buffer == "StoredExp:")
			{
				if (!(file >> tempClan->clan_exp))
					log("Error open 'StoredExp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			}
			else if (buffer == "ClanLevel:")
			{
				if (!(file >> tempClan->clan_level))
					log("Error open 'ClanLevel:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			}
			else if (buffer == "Owner:")
			{
				long unique = 0;
				long long money = 0;
				long long exp = 0;
				int exp_persent = 0;
				long long clan_exp = 0;

				if (!(file >> unique >> money >> exp >> exp_persent >> clan_exp))
				{
					log("Error open 'Owner:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// ������� ���� ��� ����� �� ����
				ClanMemberPtr tempMember(new ClanMember);
				tempMember->name = GetNameByUnique(unique);
				if (tempMember->name.empty())
				{
					log("Owner %ld is no longer exist (%s).", unique, filename.c_str());
					break;
				}
				tempMember->name[0] = UPPER(tempMember->name[0]);
				tempMember->rank_num = 0;
				tempMember->money = money;
				tempMember->exp = exp;
				tempMember->exp_persent = exp_persent;
				tempMember->clan_exp = clan_exp;
				tempClan->members[unique] = tempMember;
				tempClan->owner = tempMember->name;

			}
			else if (buffer == "Members:")
			{
				// ���������, ��������� ��� ��������, ����� ��������� �� ���� ��� (����� ��������)
				long unique = 0;
				unsigned rank = 0;
				long long money = 0;
				long long exp = 0;
				int exp_persent = 0;
				long long clan_exp = 0;

				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				while (stream >> unique)
				{
					if (!(stream >> rank >> money >> exp >> exp_persent >> clan_exp))
					{
						log("Error read %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
						break;
					}
					// �� ������, ���� ������ ����� ������
					if (!tempClan->ranks.empty() && rank > tempClan->ranks.size() - 1)
						rank = tempClan->ranks.size() - 1;

					// ��������� ��������� ������ ������������
					ClanMemberPtr tempMember(new ClanMember);
					tempMember->name = GetNameByUnique(unique);
					if (tempMember->name.empty())
					{
						log("Member %ld is no longer exist (%s).", unique, filename.c_str());
						continue;
					}
					tempMember->name[0] = UPPER(tempMember->name[0]);
					tempMember->rank_num = rank;
					tempMember->money = money;
					tempMember->exp = exp;
					tempMember->exp_persent = exp_persent;
					tempMember->clan_exp = clan_exp;
					tempClan->members[unique] = tempMember;
				}
			}
		}
		file.close();

		// ��� ����� ��������� ������� ��������� ��� ����� �����
		// �.�. �������� ��� �������� � ��������� � ����� - ���-�� ����� �� ���������������������
		if (tempClan->abbrev.empty() || tempClan->name.empty()
				|| tempClan->title.empty() || tempClan->owner.empty()
				|| tempClan->rent == 0 || tempClan->guard == 0 || tempClan->out_rent == 0
				|| tempClan->ranks.empty() || tempClan->privileges.empty())
			continue;
		// ������ �� ������� �� �����
		if (!tempClan->chest_room)
			tempClan->chest_room = tempClan->rent;

		// ����� �� ���������� ������/�������� �����
		if (tempClan->exp_buf)
		{
			tempClan->exp += tempClan->exp_buf;
			if (tempClan->exp < 0)
				tempClan->exp = 0;
			tempClan->exp_buf = 0;
		}

		// ���������� ���/���
		std::ifstream pkFile((filename + ".pkl").c_str());
		if (pkFile.is_open())
		{
			int author = 0;
			while (pkFile >> author)
			{
				int victim = 0;
				time_t tempTime = time(0);
				bool flag = 0;

				if (!(pkFile >> victim >> tempTime >> flag))
				{
					log("Error read %s! (%s %s %d)", (filename + ".pkl").c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				std::getline(pkFile, buffer, '~');
				boost::trim(buffer);
				std::string authorName = GetNameByUnique(author);
				std::string victimName = GetNameByUnique(victim, 1);
				name_convert(authorName);
				name_convert(victimName);
				// ���� ������ ��� ��� - ������� ��� ��� ��������
				if (authorName.empty())
					author = 0;
				// ���� ������ ��� ���, ��� ������ �������� � ���� - �� ������
				if (!victimName.empty())
				{
					ClanPkPtr tempRecord(new ClanPk);
					tempRecord->author = author;
					tempRecord->authorName = authorName.empty() ? "���� �� � ���� ���" : authorName;
					tempRecord->victimName = victimName;
					tempRecord->time = tempTime;
					tempRecord->text = buffer;
					if (!flag)
						tempClan->pkList[victim] = tempRecord;
					else
						tempClan->frList[victim] = tempRecord;
				}
			}
			pkFile.close();
		}
		//���������� ���������
		std::ifstream stuffFile((filename + ".stuff").c_str());
		if (stuffFile.is_open())
		{
			std::string line;
			int i;
			while (stuffFile >> i)
			{
				ClanStuffName temp;
				temp.num = i;

				std::getline(stuffFile, temp.name, '~');
				boost::trim(temp.name);

				for (int j = 0;j < 6; j++)
				{
					std::getline(stuffFile, buffer, '~');
					boost::trim(buffer);
					temp.PNames.push_back(buffer);
				}

				std::getline(stuffFile, temp.desc, '~');
				boost::trim(temp.desc);
				std::getline(stuffFile, temp.longdesc, '~');
				boost::trim(temp.longdesc);

				tempClan->clanstuff.push_back(temp);
			}
		}
		// ����-�����
		tempClan->last_exp.load(tempClan->get_abbrev());

		Clan::ClanList.push_back(tempClan);
	}

	Clan::ChestLoad();
	Clan::ChestUpdate();
	Clan::ChestSave();
	Clan::ClanSave();
	Board::ClanInit();

	// �� ������ ������� ������ ��� ����������� ��������� ������� ������
	// �������� ��������� � ������ �����, ����� � ��� ���-���� ��������, �������� ��������� �������
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
		if (d->character)
			Clan::SetClanData(d->character);
}

// ����� ���� ���������� � ������
void Clan::HconShow(CHAR_DATA * ch)
{
	std::ostringstream buffer;
	buffer << "Abbrev|  Rent|OutRent| Chest|  Guard|CreateDate|      StoredExp|      Bank|Items|DayTax|Lvl|Test\r\n";
	boost::format show("%6d|%6d|%7d|%6d|%7d|%10s|%15d|%10d|%5d|%6d|%3s|%4s\r\n");
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		char timeBuf[17];
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&(*clan)->builtOn));

		int cost = (*clan)->ChestTax();
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);

		buffer << show % (*clan)->abbrev % (*clan)->rent % (*clan)->out_rent % (*clan)->chest_room % (*clan)->guard % timeBuf
		% (*clan)->clan_exp % (*clan)->bank % (*clan)->chest_objcount % cost % (*clan)->clan_level % ((*clan)->test_clan ? "y" : "n");
	}
	send_to_char(ch, buffer.str().c_str());
}

// ���������� ������ � �����
void Clan::ClanSave()
{
	std::ofstream index(LIB_HOUSE "index");
	if (!index.is_open())
	{
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		// ������ ����� ��� ����� ������ ��� ������������ (���������� � ������ �������)
		std::string buffer = (*clan)->abbrev;
		CreateFileName(buffer);
		index << buffer << "\n";

		std::string filepath = LIB_HOUSE + buffer + "/" + buffer;
		std::string path = LIB_HOUSE + buffer;
		struct stat result;
		if (stat(filepath.c_str(), &result))
		{
			if (mkdir(path.c_str(), 0700))
			{
				log("Can't create dir: %s! (%s %s %d)", path.c_str(), __FILE__, __func__, __LINE__);
				return;
			}
		}
		std::ofstream file(filepath.c_str());
		if (!file.is_open())
		{
			log("Error open file: %s! (%s %s %d)", filepath.c_str(), __FILE__, __func__, __LINE__);
			return;
		}

		file << "Abbrev: " << (*clan)->abbrev << "\n"
		<< "Name: " << (*clan)->name << "\n"
		<< "Title: " << (*clan)->title << "\n"
		<< "Rent: " << (*clan)->rent << "\n"
		<< "OutRent: " << (*clan)->out_rent << "\n"
		<< "ChestRoom: " << (*clan)->chest_room << "\n"
		<< "Guard: " << (*clan)->guard << "\n"
		<< "BuiltOn: " << (*clan)->builtOn << "\n"
		<< "EntranceMode: " << (*clan)->entranceMode << "\n"
		<< "Storehouse: " << (*clan)->storehouse << "\n"
		<< "ExpInfo: " << (*clan)->exp_info << "\n"
		<< "TestClan: " << (*clan)->test_clan << "\n"
		<< "Exp: " << (*clan)->exp << "\n"
		<< "ExpBuf: " << (*clan)->exp_buf << "\n"
		<< "StoredExp: " << (*clan)->clan_exp << "\n"
		<< "ClanLevel: " << (*clan)->clan_level << "\n"
		<< "Bank: " << (*clan)->bank << "\n" << "Politics:\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			file << " " << it->first << " " << it->second << "\n";
		file << "~\n" << "Ranks:\n";
		int i = 0;

		for (std::vector < std::string >::const_iterator it = (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it, ++i)
			file << " " << (*it) << " " << (*clan)->ranks_female[i] << " " << (*clan)->privileges[i].to_ulong() << "\n";
		file << "~\n" << "Owner: ";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if (it->second->rank_num == 0)
			{
				file << it->first << " " << it->second->money << " " << it->second->exp << " " << it->second->exp_persent << " " << it->second->clan_exp << "\n";
				break;
			}
		file << "Members:\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if (it->second->rank_num != 0)
				file << " " << it->first << " " << it->second->rank_num << " " << it->second->money  << " " << it->second->exp << " " << it->second->exp_persent << " " << it->second->clan_exp << "\n";
		file << "~\n";
		file.close();

		std::ofstream pkFile((filepath + ".pkl").c_str());
		if (!pkFile.is_open())
		{
			log("Error open file: %s! (%s %s %d)", (filepath + ".pkl").c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
			second->time << " " << "0\n" << it->second->text << "\n~\n";
		for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
			second->time << " " << "1\n" << it->second->text << "\n~\n";
		pkFile.close();
	}
	index.close();
}

// ����������� ��������� ������ ���� ���� �� ��������
// ������ � ����� ��� �������� �� ���������� �������� CLAN()
void Clan::SetClanData(CHAR_DATA * ch)
{
	CLAN(ch).reset();
	CLAN_MEMBER(ch).reset();
	// ���� ������ ���� ������� � ���, �� �������� ��� ����������
	if (STATE(ch->desc) == CON_CLANEDIT)
	{
		ch->desc->clan_olc.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("�������������� �������� ��-�� ���������� ����� ������ � �������.\r\n", ch);
	}

	// ���� ����-�� ��������, �� ������� ����� ��������� �� ���� � ������ ������
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::const_iterator member = (*clan)->members.find(GET_UNIQUE(ch));
		if (member != (*clan)->members.end())
		{
			CLAN(ch) = *clan;
			CLAN_MEMBER(ch) = member->second;
			break;
		}
	}
	// ������ �� ��������
	if (!CLAN(ch))
	{
		free(GET_CLAN_STATUS(ch));
		GET_CLAN_STATUS(ch) = 0;
		return;
	}
	// ����-�� ���� ��������
	std::string buffer;
	if (IS_MALE(ch))
		buffer = CLAN(ch)->ranks[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title;
	else
		buffer = CLAN(ch)->ranks_female[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title;
	GET_CLAN_STATUS(ch) = str_dup(buffer.c_str());

	// ����� ��� ������ �� ���� ����������� ����� �� ���� ����� ����
	ch->desc->clan_invite.reset();
}

// �������� ������� �� �������������� ������-���� �����
ClanListType::const_iterator Clan::IsClanRoom(room_rnum room)
{
	ClanListType::const_iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[room]->zone == world[real_room((*clan)->rent)]->zone)
			return clan;
	return clan;
}

// ����� �� �������� ����� � �����
bool Clan::MayEnter(CHAR_DATA * ch, room_rnum room, bool mode)
{
	ClanListType::const_iterator clan = IsClanRoom(room);
	if (clan == Clan::ClanList.end()
			|| IS_GRGOD(ch)
			|| !ROOM_FLAGGED(room, ROOM_HOUSE)
			|| (*clan)->entranceMode
			|| Privilege::check_flag(ch, Privilege::KRODER))
	{
		return 1;
	}
	if (!CLAN(ch))
		return 0;

	bool isMember = 0;

	if (CLAN(ch) == *clan || (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
		isMember = 1;

	switch (mode)
	{
		// ���� ����� ����� - ������������ ��������
	case HCE_ATRIUM:
		CHAR_DATA * mobs;
		for (mobs = world[IN_ROOM(ch)]->people; mobs; mobs = mobs->next_in_room)
			if ((*clan)->guard == GET_MOB_VNUM(mobs))
				break;
		// ��������� ��� - ��������� ������
		if (!mobs)
			return 1;
		// break �� ����

		// ������������
	case HCE_PORTAL:
		if (!isMember)
			return 0;
		// � ��������� ������ ���� �����
		if (RENTABLE(ch))
		{
			if (mode == HCE_ATRIUM)
				send_to_char("������ ������� ����� � ���� ������, � ����� ����� ������� ������.\r\n", ch);
			return 0;
		}
		return 1;
	}
	return 0;
}

// � ����������� �� ����������� ������� ����� ����� ������ ������ ������
const char *HOUSE_FORMAT[] =
{
	"  ���� ����������\r\n",
	"  ���� ������� ��� ������\r\n",
	"  ���� ������� ���\r\n",
	"  ���� ����������\r\n",
	"  ������� (�����)\r\n",
	"  �������� <��� �������> <�����������|�����|������>\r\n",
	"  ��������� <������|��������>\r\n",
	"  ������|������ <��������|�������>\r\n",
	"  ������ ���� � ���������\r\n",
	"  ����� ���� �� ���������\r\n",
	"  ����� (�������)\r\n",
	"  ���� �������� (����� �� �������)\r\n",
};

// ��������� �������� ���������� (������� house)
ACMD(DoHouse)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	// ���� ����� ����������, �� ���� ������� � ������������
	if (!CLAN(ch))
	{
		if (CompareParam(buffer2, "��������") && ch->desc->clan_invite)
			ch->desc->clan_invite->clan->ClanAddMember(ch, ch->desc->clan_invite->rank);
		else if (CompareParam(buffer2, "��������") && ch->desc->clan_invite)
		{
			ch->desc->clan_invite.reset();
			send_to_char("����������� ������� ���������.\r\n", ch);
			return;
		}
		else
			send_to_char("������ ������� �������� ������ ������ ����������������� ������ ������,\r\n"
						 "� ���� ��� ���������� �������� � ����, �� ��� � ������ '���� ��������' ��� '���� ��������'.\r\n"
						 "����� �� ������ ������� ��� ����� ����������� �� ��� ���, ���� �� �� ����������� � ����.\r\n", ch);
		return;
	}

	if (CompareParam(buffer2, "����������") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_INFO])
		CLAN(ch)->HouseInfo(ch);
	else if (CompareParam(buffer2, "�������") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_ADD])
		CLAN(ch)->HouseAdd(ch, buffer);
	else if (CompareParam(buffer2, "�������") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_REMOVE])
		CLAN(ch)->HouseRemove(ch, buffer);
	else if (CompareParam(buffer2, "����������") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PRIVILEGES])
	{
		boost::shared_ptr<struct ClanOLC> temp_clan_olc(new ClanOLC);
		temp_clan_olc->clan = CLAN(ch);
		temp_clan_olc->privileges = CLAN(ch)->privileges;
		ch->desc->clan_olc = temp_clan_olc;
		STATE(ch->desc) = CON_CLANEDIT;
		CLAN(ch)->MainMenu(ch->desc);
	}
	else if (CompareParam(buffer2, "�������") && !CLAN_MEMBER(ch)->rank_num)
		CLAN(ch)->HouseOwner(ch, buffer);
	else if (CompareParam(buffer2, "����������"))
		CLAN(ch)->HouseStat(ch, buffer);
	else if (CompareParam(buffer2, "��������", 1) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_EXIT])
		CLAN(ch)->HouseLeave(ch);
	else if (CompareParam(buffer2, "�����"))
		CLAN(ch)->TaxManage(ch, buffer);
	else
	{
		// ��������� ������ ��������� ������ �� ������ ���������
		buffer = "��������� ��� ���������� �������:\r\n";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][i])
				buffer += HOUSE_FORMAT[i];
		// ������� �� ���� ����� ��� ������� � ������� �������
		if (!CLAN_MEMBER(ch)->rank_num)
			buffer += "  ���� ������� (���)\r\n";
		buffer += "  ���� ����� (������� ����������� �����)\r\n";
		if (CLAN(ch)->storehouse)
			buffer += "  ��������� <�������>\r\n";
		buffer += "  ���� ���������� <!����/!������������/!�����/!����������/!���>";
		if (!CLAN_MEMBER(ch)->rank_num)
			buffer += " <���|���|��������|�������� ������>\r\n";
		else
			buffer += " <���|���>\r\n";
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS])
			buffer += "  �������� (������ ��������)\r\n";
		send_to_char(buffer, ch);
	}
}

// house ����������
void Clan::HouseInfo(CHAR_DATA * ch)
{
	// �����, ���������� ������ ������������� �� ���� ���������, ������� ���������� �� ������
	std::vector<ClanMemberPtr> temp_list;
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
		temp_list.push_back(it->second);

	std::sort(temp_list.begin(), temp_list.end(),
			  boost::bind(std::less<long long>(),
						  boost::bind(&ClanMember::rank_num, _1),
						  boost::bind(&ClanMember::rank_num, _2)));

	std::ostringstream buffer;
	buffer << "� ����� ���������: ";
	for (std::vector<ClanMemberPtr>::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
		buffer << (*it)->name << " ";
	buffer << "\r\n����������:\r\n";
	int num = 0;

	for (std::vector<std::string>::const_iterator it = ranks.begin(); it != ranks.end(); ++it, ++num)
	{
		buffer << "  " << (*it) << ":";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (this->privileges[num][i])
				switch (i)
				{
				case MAY_CLAN_INFO:
					buffer << " ����";
					break;
				case MAY_CLAN_ADD:
					buffer << " �������";
					break;
				case MAY_CLAN_REMOVE:
					buffer << " �������";
					break;
				case MAY_CLAN_PRIVILEGES:
					buffer << " ����������";
					break;
				case MAY_CLAN_CHANNEL:
					buffer << " ��";
					break;
				case MAY_CLAN_POLITICS:
					buffer << " ��������";
					break;
				case MAY_CLAN_NEWS:
					buffer << " ���";
					break;
				case MAY_CLAN_PKLIST:
					buffer << " ���";
					break;
				case MAY_CLAN_CHEST_PUT:
					buffer << " ����.�����";
					break;
				case MAY_CLAN_CHEST_TAKE:
					buffer << " ����.�����";
					break;
				case MAY_CLAN_BANK:
					buffer << " �����";
					break;
				case MAY_CLAN_EXIT:
					buffer << " �����";
					break;
				}
		buffer << "\r\n";
	}
	//���� � ����� ����� ������ �����, �������� ����� � �������
	buffer << "��� ����� ������ " << this->clan_exp << " ����� ����� � ����� ������� " << this->clan_level << "\r\n";
	buffer << "������� ������ �����: " << this->exp << " ��� ����� ����� :), �� ������ ��� �� ����.\r\n";
	buffer << "� ��������� ����� ����� �������� �� " << this->ChestMaxObjects() << " " << desc_count(this->ChestMaxObjects(), WHAT_OBJECT) << " � ����� ����� �� ����� ��� " << this->ChestMaxWeight() << ".\r\n";

	// ���� � ����� � ���������
	int cost = ChestTax();
	buffer << "� ��������� ����� ������� " << this->chest_objcount << " " << desc_count(this->chest_objcount, WHAT_OBJECT)  << " ����� ����� � " << this->chest_weight << " (" << cost << " " << desc_count(cost, WHAT_MONEYa) << " � ����).\r\n"
	<< "��������� �����: " << this->bank << " " << desc_count(this->bank, WHAT_MONEYa) << ".\r\n"
	<< "����� ���������� " << CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX) << " " << desc_count(CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX), WHAT_MONEYa) << " � ����.\r\n";
	cost += CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX);
	if (!cost)
		buffer << "����� ����� ������ �� ���������� ���������� ����.\r\n";
	else
		buffer << "����� ����� ������ �������� �� " << this->bank / cost << " " << desc_count(this->bank / cost, WHAT_DAY) << ".\r\n";
	send_to_char(buffer.str(), ch);
}

// ���� �������, �������� ����� ������ �� ����������� ���� ������
void Clan::HouseAdd(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
	{
		send_to_char("������� ��� ���������.\r\n", ch);
		return;
	}

	long unique = GetUniqueByName(buffer2);
	if (!unique)
	{
		send_to_char("����������� ��������.\r\n", ch);
		return;
	}
	std::string name = buffer2;
	name[0] = UPPER(name[0]);
	if (unique == GET_UNIQUE(ch))
	{
		send_to_char("��� ���� �������, ������ ���� ����� �������������?\r\n", ch);
		return;
	}

	// ��������� ������ � ������ �������
	// ���� ���� ��� ��������� �������
	ClanMemberList::iterator it_member = this->members.find(unique);
	if (it_member != this->members.end())
	{
		if (it_member->second->rank_num <= CLAN_MEMBER(ch)->rank_num)
		{
			send_to_char("�� ������ ������ ������ ������ � ����������� ������ �������.\r\n", ch);
			return;
		}

		int rank = CLAN_MEMBER(ch)->rank_num;
		if (!rank) ++rank;

		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			buffer = "������� ������ ���������.\r\n��������� ���������: ";
			for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
				buffer += "'" + *it + "' ";
			buffer += "\r\n";
			send_to_char(buffer, ch);
			return;
		}

		int temp_rank = rank;
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
			if (CompareParam(buffer2, *it))
			{
				CHAR_DATA *editedChar = NULL;
				DESCRIPTOR_DATA *d = DescByUID(unique);
				this->members[unique]->rank_num = temp_rank;
				if (d)
				{
					editedChar = d->character;
					Clan::SetClanData(d->character);
					send_to_char(d->character, "%s���� ������ ��������, ������ �� %s.%s\r\n",
								 CCWHT(d->character, C_NRM),
								 (*it).c_str(),
								 CCNRM(d->character, C_NRM));
				}

				// ���������� ����������� � ��������� ������
				for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
				{
					if (d->character && CLAN(d->character) && CLAN(d->character)->GetRent() == this->GetRent() && editedChar != d->character)
						send_to_char(d->character, "%s%s ������ %s.%s\r\n",
									 CCWHT(d->character, C_NRM),
									 it_member->second->name.c_str(), (*it).c_str(),
									 CCNRM(d->character, C_NRM));
				}
				return;
			}

		buffer = "�������� ������, ��������� ���������:\r\n";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);

		return;
	}

	DESCRIPTOR_DATA *d = DescByUID(unique);
	if (!d || !CAN_SEE(ch, d->character))
	{
		send_to_char("����� ��������� ��� � ����!\r\n", ch);
		return;
	}
	if (PRF_FLAGGED(d->character, PRF_CODERINFO))
	{
		send_to_char("�� �� ������ ��������� ����� ������.\r\n", ch);
		return;
	}
	if (CLAN(d->character) && CLAN(ch) != CLAN(d->character))
	{
		send_to_char("�� �� ������ ��������� ����� ������ �������.\r\n", ch);
		return;
	}
	if (d->clan_invite)
	{
		if (d->clan_invite->clan == CLAN(ch))
		{
			send_to_char("�� ��� ���������� ����� ������, ����� �������.\r\n", ch);
			return;
		}
		else
		{
			send_to_char("�� ��� ��������� � ������ �������, ��������� ��� ������ � ���������� �����.\r\n", ch);
			return;
		}
	}
	if (GET_KIN(d->character) != GET_KIN(ch))
	{
		send_to_char("�� �� ������ ��������� � ����� ����� � ��������������.\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);

	// ����� ������ ������� � 0 ������ �� ����� �������� � �� ���� ��� ��������� ��� 10 ������
	int rank = CLAN_MEMBER(ch)->rank_num;
	if (!rank)
		++rank;

	if (buffer2.empty())
	{
		buffer = "������� ������ ���������.\r\n��������� ���������: ";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);
		return;
	}
	int temp_rank = rank; // ������ rank ���� �� ����������� � ������ ��������� ������
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
		if (CompareParam(buffer2, *it))
		{
			// �� �������� - ������� ��� ����������� � �����, ���� �� ����������
			boost::shared_ptr<struct ClanInvite> temp_invite(new ClanInvite);
			temp_invite->clan = CLAN(ch);
			temp_invite->rank = temp_rank;
			d->clan_invite = temp_invite;
			buffer = CCWHT(d->character, C_NRM);
			buffer += "$N ���������$G � ���� �������, ������ - " + *it + ".";
			buffer += CCNRM(d->character, C_NRM);
			// ��������� ����������
			act(buffer.c_str(), FALSE, ch, 0, d->character, TO_CHAR);
			buffer = CCWHT(d->character, C_NRM);
			buffer += "�� �������� ����������� � ������� '" + this->name + "', ������ - " + *it + ".\r\n"
					  + "����� ������� ����������� �������� '���� ��������', ��� ������ '���� ��������'.\r\n"
					  + "����� �� ������ ������� ��� ����� ����������� �� ��� ���, ���� �� �� ����������� � ����.\r\n";

			buffer += CCNRM(d->character, C_NRM);
			send_to_char(buffer, d->character);
			return;
		}
	buffer = "�������� ������, ��������� ���������:\r\n";
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
		buffer += "'" + *it + "' ";
	buffer += "\r\n";
	send_to_char(buffer, ch);
}

/**
* ����������� ��������� �� ����� � ����������� ���� ���������������� ������,
* ����������� �� ������� ����� � ���������� ����� ��� �������������.
*/
void Clan::remove_member(ClanMemberList::iterator &it)
{
	std::string name = it->second->name;
	long unique = it->first;
	this->members.erase(it);

	DESCRIPTOR_DATA *k = DescByUID(unique);
	if (k && k->character)
	{
		Clan::SetClanData(k->character);
		send_to_char(k->character, "��� ��������� �� ������� '%s'!\r\n", this->name.c_str());

		ClanListType::const_iterator clan = Clan::IsClanRoom(IN_ROOM(k->character));
		if (clan != Clan::ClanList.end())
		{
			char_from_room(k->character);
			act("$n ���$g ��������$a �� ������� �����!", TRUE, k->character, 0, 0, TO_ROOM);
			send_to_char("�� ���� ��������� �� ������� �����!\r\n", k->character);
			char_to_room(k->character, real_room((*clan)->out_rent));
			look_at_room(k->character, real_room((*clan)->out_rent));
			act("$n ������$u � �����, ���������� �����-�� ������������!", TRUE, k->character, 0, 0, TO_ROOM);
		}
	}
	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character && CLAN(d->character) && CLAN(d->character)->GetRent() == this->GetRent())
			send_to_char(d->character, "%s ����� �� �������� ������ ����� �������.\r\n", name.c_str());
	}
}

// house ������� (������ ������� ���� ������)
void Clan::HouseRemove(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	ClanMemberList::iterator it = this->members.find(unique);

	if (buffer2.empty())
		send_to_char("������� ��� ���������.\r\n", ch);
	else if (!unique)
		send_to_char("����������� ��������.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("�������� �������� �������...\r\n", ch);
	if (it == this->members.end())
		send_to_char("�� � ��� �� �������� � ����� �������.\r\n", ch);
	else if (it->second->rank_num <= CLAN_MEMBER(ch)->rank_num)
		send_to_char("�� ������ ��������� �� ������� ������ ��������� �� ������� ���� ������.\r\n", ch);
	else
		remove_member(it);
}

void Clan::HouseLeave(CHAR_DATA * ch)
{
	if (!CLAN_MEMBER(ch)->rank_num)
	{
		send_to_char("���� �� ������ ���������� ���� �������, �� ����������� � �����.\r\n"
					 "� ���� ��� ������ ����� ��������, �� ��������� ���������� � ����� ���� ������...\r\n", ch);
		return;
	}

	ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
	if (it != this->members.end())
		remove_member(it);
}

/**
* hcontrol outcast ��� - ����������� ������ ��������� �� �������, ����� �������.
*/
void Clan::hcon_outcast(CHAR_DATA *ch, std::string buffer)
{
	std::string name;
	GetOneParam(buffer, name);
	long unique = GetUniqueByName(name);

	if (!unique)
	{
		send_to_char("����������� ��������.\r\n", ch);
		return;
	}

	for (ClanListType::iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			if (!it->second->rank_num)
			{
				send_to_char(ch, "�� �� ������ ��������� �������, ��� �������� ������� ���������� hcontrol destroy.\r\n");
				return;
			}
			(*clan)->remove_member(it);
			send_to_char(ch, "%s �������� �� ������� '%s'.\r\n", name.c_str(), (*clan)->name.c_str());
			return;
		}
	}

	send_to_char("�� � ��� �� ������� �� � ����� �������.\r\n", ch);
}

// ���, �����, ����/������
void Clan::GodToChannel(CHAR_DATA *ch, std::string text, int subcmd)
{
	boost::trim(text);
	// �� ���� ������ � ��, ��-����� ��� ��������� ��� �� � ����, ��� ������� ���� 5, ��� ����� �������� ��������? �)
	if (text.empty())
	{
		send_to_char("��� �� ������ �� ��������?\r\n", ch);
		return;
	}
	switch (subcmd)
	{
		// ������� ��� ������� �����-�� �������
	case SCMD_CHANNEL:
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& ch != d->character
					&& STATE(d) == CON_PLAYING
					&& CLAN(d->character).get() == this
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS))
			{
				send_to_char(d->character, "%s ����� �������: %s'%s'%s\r\n",
							 GET_NAME(ch), CCIRED(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		send_to_char(ch, "�� ������� %s: %s'%s'.%s\r\n", this->abbrev.c_str(), CCIRED(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		break;
		// �� �� � ����� ��������� ���� �������, ���� ��� ������ ����
	case SCMD_ACHANNEL:
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& STATE(d) == CON_PLAYING
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& d->character != ch)
			{
				if (this->CheckPolitics(CLAN(d->character)->GetRent()) == POLITICS_ALLIANCE
						|| CLAN(d->character).get() == this)
				{
					// �������� �� ������ � ����� ������, ����� ��� �� ������
					if (CLAN(d->character).get() != this)
					{
						if (CLAN(d->character)->CheckPolitics(this->rent) == POLITICS_ALLIANCE)
							send_to_char(d->character, "%s ����� ���������: %s'%s'%s\r\n",
										 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
					}
					// ��������������� ����� �������� ������
					else
						send_to_char(d->character, "%s ����� ���������: %s'%s'%s\r\n",
									 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
				}
			}
		}
		send_to_char(ch, "�� ��������� %s: %s'%s'.%s\r\n",
					 this->abbrev.c_str(), CCIGRN(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		break;
	}
}

// ��� (��� �������� ���), �����, ����/������
void Clan::CharToChannel(CHAR_DATA *ch, std::string text, int subcmd)
{
	boost::trim(text);
	if (text.empty())
	{
		send_to_char("��� �� ������ ��������?\r\n", ch);
		return;
	}

	switch (subcmd)
	{
	// ����� �������
	case SCMD_CHANNEL:
	{
		// ���������
		snprintf(buf, MAX_STRING_LENGTH, "%s �������: &R'%s'.&n\r\n", GET_NAME(ch), text.c_str());
		CLAN(ch)->add_remember(buf, Remember::CLAN);

		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && d->character != ch
					&& STATE(d) == CON_PLAYING
					&& CLAN(d->character) == CLAN(ch)
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& !ignores(d->character, ch, IGNORE_CLAN))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%s �������: %s'%s'.%s\r\n",
						GET_NAME(ch), CCIRED(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
				d->character->remember_add(buf, Remember::ALL);
				send_to_char(buf, d->character);
			}
		}
		snprintf(buf, MAX_STRING_LENGTH, "�� �������: %s'%s'.%s\r\n", CCIRED(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		ch->remember_add(buf, Remember::ALL);
		send_to_char(buf, ch);
		break;
	}
	// ���������
	case SCMD_ACHANNEL:
	{
		// ���������
		snprintf(buf, MAX_STRING_LENGTH, "%s ���������: &G'%s'.&n\r\n", GET_NAME(ch), text.c_str());
		for (ClanListType::iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		{
			if ((CLAN(ch)->CheckPolitics((*clan)->GetRent())== POLITICS_ALLIANCE
					&& (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
				|| CLAN(ch) == (*clan))
			{
				(*clan)->add_remember(buf, Remember::ALLY);
			}
		}

		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& STATE(d) == CON_PLAYING
					&& d->character != ch
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& !ignores(d->character, ch, IGNORE_ALLIANCE))
			{
				if (CLAN(ch)->CheckPolitics(CLAN(d->character)->GetRent()) == POLITICS_ALLIANCE
						|| CLAN(ch) == CLAN(d->character))
				{
					// �������� �� ������ � ����� ������, ��� �� ������� ���� ����� �� ���
					if ((CLAN(d->character)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
						|| CLAN(ch) == CLAN(d->character))
					{
							snprintf(buf, MAX_STRING_LENGTH, "%s ���������: %s'%s'.%s\r\n",
										 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
							d->character->remember_add(buf, Remember::ALL);
							send_to_char(buf, d->character);
					}
				}
			}
		}
		snprintf(buf, MAX_STRING_LENGTH, "�� ���������: %s'%s'.%s\r\n", CCIGRN(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		ch->remember_add(buf, Remember::ALL);
		send_to_char(buf, ch);
		break;
	}
	} // switch
}


// ��������� ����-������ � ������ ���������, ��� ������, ��� � ����
// �������� ���� ���� 34 �� ����� �������� ������ ��������, � �� � ��������� ���������
// ��� ������ ��������� ����� �������� ������ ������
ACMD(DoClanChannel)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;

	// ������� ���������� ��� 34 �������� ��� ������� �����-�� �������
	if (IS_IMPL(ch) || (IS_GRGOD(ch) && !CLAN(ch)))
	{
		std::string buffer2;
		GetOneParam(buffer, buffer2);

		ClanListType::const_iterator clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam((*clan)->abbrev, buffer2, 1))
				break;
		if (clan == Clan::ClanList.end())
		{
			if (!CLAN(ch)) // ���������� 34 ������ �������������
				send_to_char("������� � ����� ������������� �� �������.\r\n", ch);
			else   // �������� 34 ��������� �� �����, ���� � ��� ����-�����
			{
				buffer = argument; // ���� �����
				CLAN(ch)->CharToChannel(ch, buffer, subcmd);
			}
			return;
		}
		(*clan)->GodToChannel(ch, buffer, subcmd);
		// ��������� ������� ������ � ���� �������
	}
	else
	{
		if (!CLAN(ch))
		{
			send_to_char("�� �� ������������ �� � ����� �������.\r\n", ch);
			return;
		}
		// ����������� �� ����-����� �� ������ �� ����� ������, ���� ��� ���
		if (!IS_IMMORTAL(ch) && (!(CLAN(ch))->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHANNEL] || PLR_FLAGGED(ch, PLR_DUMB)))
		{
			send_to_char("�� �� ������ ������������ ������� �������.\r\n", ch);
			return;
		}
		CLAN(ch)->CharToChannel(ch, buffer, subcmd);
	}
}

// ������ ������������������ ������ � �� ���������� �������� (�����������)
ACMD(DoClanList)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	ClanListType::const_iterator clan;

	if (buffer.empty())
	{
		// ���������� ������ �� �����
		std::multimap<long long, ClanPtr> sort_clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		{
			if (!(*clan)->test_clan)
				sort_clan.insert(std::make_pair((*clan)->last_exp.get_exp(), *clan));
			else
				sort_clan.insert(std::make_pair(0, *clan));
		}

		std::ostringstream out;
		boost::format clanTopFormat(" %5d  %6s   %-30s %14s%14s %9d\r\n");
		out << "� ���� ���������������� ��������� �������:\r\n"
		<< "     #           ��������                          ����� �����    �� 30 ����   �������\r\n\r\n";
		int count = 1;
		for (std::multimap<long long, ClanPtr>::reverse_iterator it = sort_clan.rbegin(); it != sort_clan.rend(); ++it, ++count)
			out << clanTopFormat % count % it->second->abbrev % it->second->name % ExpFormat(it->second->exp) % ExpFormat(it->second->last_exp.get_exp()) % it->second->members.size();
		send_to_char(out.str(), ch);
		return;
	}

	bool all = 0;
	std::vector<CHAR_DATA *> temp_list;

	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (CompareParam(buffer, (*clan)->abbrev))
			break;
	if (clan == Clan::ClanList.end())
	{
		if (CompareParam(buffer, "���"))
			all = 1;
		else
		{
			send_to_char("����� ������� �� ����������������\r\n", ch);
			return;
		}
	}
	// �������� ������ ������ ������� ��� ���� ������ (�� ����� all)
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character
				&& STATE(d) == CON_PLAYING
				&& CLAN(d->character)
				&& CAN_SEE_CHAR(ch, d->character)
				&& !IS_IMMORTAL(d->character)
				&& !PRF_FLAGGED(d->character, PRF_CODERINFO)
				&& (all || CLAN(d->character) == *clan))
		{
			temp_list.push_back(d->character);
		}
	}

	// �� ���� ���������� �� ������
	std::sort(temp_list.begin(), temp_list.end(), Clan::SortRank());

	std::ostringstream buffer2;
	buffer2 << "� ���� ���������������� ��������� �������:\r\n" << "     #                  ����� ��������\r\n\r\n";
	boost::format clanFormat(" %5d  %6s %15s %s\r\n");
	boost::format memberFormat(" %-10s%s%s%s %s%s%s\r\n");
	// ���� ������ ���������� ������� - ������� ��
	if (!all)
	{
		buffer2 << clanFormat % 1 % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
		for (std::vector < CHAR_DATA * >::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
			buffer2 << memberFormat % (*clan)->ranks[CLAN_MEMBER(*it)->rank_num]
			% CCPK(ch, C_NRM, *it) % noclan_title(*it)
			% CCNRM(ch, C_NRM) % CCIRED(ch, C_NRM)
			% (PLR_FLAGGED(*it, PLR_KILLER) ? "(�������)" : "")
			% CCNRM(ch, C_NRM);
	}
	// ������ ������� ��� ������� � ���� ������ (��� ��������� '���' � ������ ����� ������ �������)
	else
	{
		int count = 1;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan, ++count)
		{
			buffer2 << clanFormat % count % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
			for (std::vector < CHAR_DATA * >::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
				if (CLAN(*it) == *clan)
					buffer2 << memberFormat % (*clan)->ranks[CLAN_MEMBER(*it)->rank_num]
					% CCPK(ch, C_NRM, *it) % noclan_title(*it)
					% CCNRM(ch, C_NRM) % CCIRED(ch, C_NRM)
					% (PLR_FLAGGED(*it, PLR_KILLER) ? "(�������)" : "")
					% CCNRM(ch, C_NRM);
		}
	}
	buffer2 << "\r\n����� ������� - " << temp_list.size() << "\r\n";
	send_to_char(buffer2.str(), ch);
}

// ���������� ��������� �������� clan �� ��������� � victim
// ��� ���������� ��������������� �����������
int Clan::CheckPolitics(int victim)
{
	ClanPolitics::const_iterator it = politics.find(victim);
	if (it != politics.end())
		return it->second;
	return POLITICS_NEUTRAL;
}

// ���������� ����� ��������(state) �� ��������� � victim
// ����������� �������� ������ �������� ������, ���� ��� ����
void Clan::SetPolitics(int victim, int state)
{
	ClanPolitics::iterator it = politics.find(victim);
	if (it == politics.end() && state == POLITICS_NEUTRAL)
		return;
	if (state == POLITICS_NEUTRAL)
		politics.erase(it);
	else
		politics[victim] = state;
}

const char *politicsnames[] = { "�����������", "�����", "������" };

// ������� ���������� �� ���������� ������ ����� �����
ACMD(DoShowPolitics)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("���� ?\r\n", ch);
		return;
	}

	std::string buffer = argument;
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	if (!buffer.empty() && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS])
	{
		CLAN(ch)->ManagePolitics(ch, buffer);
		return;
	}
	boost::format strFormat("  %-3s             %s%-11s%s                 %s%-11s%s\r\n");
	int p1 = 0, p2 = 0;

	std::ostringstream buffer2;
	buffer2 << "��������� ����� ������� � ������� ���������:\r\n" <<
	"��������     ��������� ����� �������     ��������� � ����� �������\r\n";

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim)
	{
		if (*clanVictim == CLAN(ch))
			continue;
		p1 = CLAN(ch)->CheckPolitics((*clanVictim)->rent);
		p2 = (*clanVictim)->CheckPolitics(CLAN(ch)->rent);
		buffer2 << strFormat % (*clanVictim)->abbrev
		% (p1 == POLITICS_WAR ? CCIRED(ch, C_NRM) : (p1 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		% politicsnames[p1] % CCNRM(ch, C_NRM)
		% (p2 == POLITICS_WAR ? CCIRED(ch, C_NRM) : (p2 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		% politicsnames[p2] % CCNRM(ch, C_NRM);
	}
	send_to_char(buffer2.str(), ch);
}

// ���������� �������� � ������������ ������ ������� � ����� ��� �������
void Clan::ManagePolitics(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	DESCRIPTOR_DATA *d;

	ClanListType::const_iterator vict;

	for (vict = Clan::ClanList.begin(); vict != Clan::ClanList.end(); ++vict)
		if (CompareParam(buffer2, (*vict)->abbrev))
			break;
	if (vict == Clan::ClanList.end())
	{
		send_to_char("��� ����� �������.\r\n", ch);
		return;
	}
	if (*vict == CLAN(ch))
	{
		send_to_char("������ �������� �� ��������� � ����� �������? ��� �� ����...\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
		send_to_char("������� ��������: �����������|�����|������.\r\n", ch);
	else if (CompareParam(buffer2, "�����������"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_NEUTRAL)
		{
			send_to_char(ch, "���� ������� ��� ��������� � ��������� ������������ � �������� %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_NEUTRAL);
		set_wait(ch, 1, FALSE);
		// ���������� ��� �������
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%s������� %s ��������� � ����� �������� �����������!%s\r\n", CCWHT(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%s���� ������� ��������� � �������� %s �����������!%s\r\n", CCWHT(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE)) // � �� ��� ����� �� ������� �����
			send_to_char(ch, "%s���� ������� ��������� � �������� %s �����������!%s\r\n", CCWHT(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));

	}
	else if (CompareParam(buffer2, "�����"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_WAR)
		{
			send_to_char(ch, "���� ������� ��� ����� � �������� %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_WAR);
		set_wait(ch, 1, FALSE);
		// ��� �����
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%s������� %s �������� ����� ������� �����!%s\r\n", CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%s���� ������� �������� ������� %s �����!%s\r\n", CCIRED(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE))
			send_to_char(ch, "%s���� ������� �������� ������� %s �����!%s\r\n", CCIRED(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));

	}
	else if (CompareParam(buffer2, "������"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_ALLIANCE)
		{
			send_to_char(ch, "���� ������� ��� � ������� � �������� %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		set_wait(ch, 1, FALSE);
		SetPolitics((*vict)->rent, POLITICS_ALLIANCE);
		// ��� �����
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%s������� %s ��������� � ����� �������� ������!%s\r\n", CCGRN(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%s���� ������� ��������� ������ � �������� %s!%s\r\n", CCGRN(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE))
			send_to_char(ch, "%s���� ������� ��������� ������ � �������� %s!%s\r\n", CCGRN(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));
	}
}

const char *HCONTROL_FORMAT =
	"������: hcontrol build <rent vnum> <outrent vnum> <guard vnum> <leader name> <abbreviation> <clan title> <clan name>\r\n"
	"        hcontrol show\r\n"
	"        hcontrol destroy <house vnum>\r\n"
	"        hcontrol outcast <name>\r\n"
	"        hcontrol save\r\n";

// ������������ hcontrol
ACMD(DoHcontrol)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "build") && !buffer.empty())
		Clan::HcontrolBuild(ch, buffer);
	else if (CompareParam(buffer2, "destroy") && !buffer.empty())
		Clan::HcontrolDestroy(ch, buffer);
	else if (CompareParam(buffer2, "outcast") && !buffer.empty())
		Clan::hcon_outcast(ch, buffer);
	else if (CompareParam(buffer2, "show"))
		Clan::HconShow(ch);
	else if (CompareParam(buffer2, "save"))
	{
		Clan::ClanSave();
		Clan::ChestUpdate();
		Clan::ChestSave();
	}
	else
		send_to_char(HCONTROL_FORMAT, ch);
}

// �������� ������� (hcontrol build)
void Clan::HcontrolBuild(CHAR_DATA * ch, std::string & buffer)
{
	// ������ ��� ���������, ����� ����� ��������� �� ���� ���� �����
	std::string buffer2;
	// �����
	GetOneParam(buffer, buffer2);
	int rent = atoi(buffer2.c_str());
	// ����� ��� �����
	GetOneParam(buffer, buffer2);
	int out_rent = atoi(buffer2.c_str());
	// ��������
	GetOneParam(buffer, buffer2);
	mob_vnum guard = atoi(buffer2.c_str());
	// �������
	std::string owner;
	GetOneParam(buffer, owner);
	// ������������
	std::string abbrev;
	GetOneParam(buffer, abbrev);
	// �������� � �����
	std::string title;
	GetOneParam(buffer, title);
	// �������� �����
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	std::string name = buffer;

	// ��� ��������� ������� ��� ����� ����
	if (name.empty())
	{
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if (!real_room(rent))
	{
		send_to_char(ch, "������� %d �� ����������.\r\n", rent);
		return;
	}
	if (!real_room(out_rent))
	{
		send_to_char(ch, "������� %d �� ����������.\r\n", out_rent);
		return;
	}
	if (real_mobile(guard) < 0)
	{
		send_to_char(ch, "���� %d �� ����������.\r\n", guard);
		return;
	}
	long unique = 0;
	if (!(unique = GetUniqueByName(owner)))
	{
		send_to_char(ch, "��������� %s �� ����������.\r\n", owner.c_str());
		return;
	}
	// � ��� - �� ����� �� �������� ������ ������
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		if ((*clan)->rent == rent)
		{
			send_to_char(ch, "������� %d ��� ������ ������ ��������.\r\n", rent);
			return;
		}
		if ((*clan)->guard == guard)
		{
			send_to_char(ch, "�������� %d ��� ����� ������ ��������.\r\n", rent);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			send_to_char(ch, "%s ��� �������� � ������� %s.\r\n", owner.c_str(), (*clan)->abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->abbrev, abbrev, 1))
		{
			send_to_char(ch, "������������ '%s' ��� ������ ������ ��������.\r\n", abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->name, name, 1))
		{
			send_to_char(ch, "��� '%s' ��� ������ ������ ��������.\r\n", name.c_str());
			return;
		}
		if (CompareParam((*clan)->title, title, 1))
		{
			send_to_char(ch, "�������� � ����� '%s' ��� ������ ������ ��������.\r\n", title.c_str());
			return;
		}
	}

	// ���������� ����
	ClanPtr tempClan(new Clan);
	tempClan->rent = rent;
	tempClan->out_rent = out_rent;
	tempClan->chest_room = rent;
	tempClan->guard = guard;
	// ����� �������
	owner[0] = UPPER(owner[0]);
	tempClan->owner = owner;
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = owner;
	tempClan->members[unique] = tempMember;
	// ��������
	tempClan->abbrev = abbrev;
	tempClan->name = name;
	tempClan->title = title;
	// �����
	const char *ranks[] = { "�������", "������", "��������", "�����", "�����", "�������", "���", "���", "�����", "�����" };
	// ������� ��� ���� ���� �����, � �� ������ �����...
	const char *ranks_female[] = { "�������", "������", "��������", "�����", "�����", "�������", "���", "���", "�����", "�����" };
	std::vector<std::string> temp_ranks(ranks, ranks + 10);
	std::vector<std::string> temp_ranks_female(ranks_female, ranks_female + 10);
	tempClan->ranks = temp_ranks;
	tempClan->ranks_female = temp_ranks_female;
	// ����������
	for (std::vector<std::string>::const_iterator it =
				tempClan->ranks.begin(); it != tempClan->ranks.end(); ++it)
		tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
	// ������� ��������� ��� ����������
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
		tempClan->privileges[0].set(i);
	// �������� ����� ���������
	OBJ_DATA * chest = read_object(CLAN_CHEST_VNUM, VIRTUAL);
	if (chest)
		obj_to_room(chest, real_room(tempClan->chest_room));

	Clan::ClanList.push_back(tempClan);
	Clan::ClanSave();
	Board::ClanInit();

	// ���������� ���������� ������� � ����
	DESCRIPTOR_DATA *d;
	if ((d = DescByUID(unique)))
	{
		Clan::SetClanData(d->character);
		send_to_char(d->character, "�� ����� �������� ������ �����. ����� ����������!\r\n");
	}
	send_to_char(ch, "������� '%s' �������!\r\n", abbrev.c_str());
}

// �������� ������� (hcontrol destroy)
void Clan::HcontrolDestroy(CHAR_DATA * ch, std::string & buffer)
{
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	int rent = atoi(buffer.c_str());

	ClanListType::iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if ((*clan)->rent == rent)
			break;
	if (clan == Clan::ClanList.end())
	{
		send_to_char(ch, "������� � ������� %d �� ����������.\r\n", rent);
		return;
	}

	// ���� ������� ������ - �������� ��� �����
	DESCRIPTOR_DATA *d;
	ClanMemberList members = (*clan)->members;
	Clan::ClanList.erase(clan);
	Clan::ClanSave();
	Board::ClanInit();
	// TODO: �� ���� ����� ������ � ��� ���������� �������, �� �� ����, ��� ��� ������
	// ���������� � ������ ���� �������
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		if ((d = DescByUID(it->first)))
		{
			Clan::SetClanData(d->character);
			send_to_char(d->character, "���� ������� ���������. ������ �����!\r\n");
		}
	}
	send_to_char("������� ���������.\r\n", ch);
}

// ���������� (������ �����������, ����������� ������)
ACMD(DoWhoClan)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("���� ?\r\n", ch);
		return;
	}

	std::ostringstream buffer;
	buffer << boost::format(" ���� �������: %s%s%s.\r\n") % CCIRED(ch, C_NRM) % CLAN(ch)->abbrev % CCNRM(ch, C_NRM);
	buffer << boost::format(" %s������ � ���� ���� ���������:%s\r\n\r\n") % CCWHT(ch, C_NRM) % CCNRM(ch, C_NRM);
	DESCRIPTOR_DATA *d;
	int num = 0;

	for (d = descriptor_list, num = 0; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && CLAN(d->character) == CLAN(ch))
		{
			buffer << "    " << race_or_title(d->character) << "\r\n";
			++num;
		}
	buffer << "\r\n �����: " << num << ".\r\n";
	send_to_char(buffer.str(), ch);
}

const char *CLAN_PKLIST_FORMAT[] =
{
	"������: ������|������ (���)\r\n"
	"        ������|������ ��� (���)\r\n",
	"        ������|������ �������� ��� �������\r\n"
	"        ������|������ ������� ���|���\r\n"
};

/**
* ��� �������� ���/��� - �� ������������ ���� � ��������� ���������, ����� ����������� � ��,
* �.�. ��� ����� ���-�� � ���� ��������, ��� ��������� ��������� ��������� ��� ������.
*/
bool check_online_state(long uid)
{
	for (CHAR_DATA *tch = character_list; tch; tch = tch->next)
	{
		if (IS_NPC(tch) || GET_UNIQUE(tch) != uid || (!tch->desc && !RENTABLE(tch))) continue;

		return true;
	}
	return false;
}

/**
* ���������� ���/��� � ������ ������ '���������'.
*/
void print_pkl(CHAR_DATA *ch, std::ostringstream &stream, ClanPkList::const_iterator &it)
{
	static char timeBuf[11];
	static boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");

	if (PRF_FLAGGED(ch, PRF_PKFORMAT_MODE))
		stream << it->second->victimName << " : " << it->second->text << "\n";
	else
	{
		strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&(it->second->time)));
		stream << frmt % timeBuf % it->second->authorName % it->second->victimName % it->second->text;
	}
}

// ���/���
ACMD(DoClanPkList)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("���� ?\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	std::ostringstream info;

	boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");
	if (buffer2.empty())
	{
		// ������� ������ ���, ��� ������
		send_to_char(ch, "%s������������ ������ ����������� � ���� ���������:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		ClanPkList::const_iterator it;
		// ������ ����� ������� ����� � ��, ����������� � �� - �������� ������ �� ��������-�����
		for (CHAR_DATA *tch = character_list; tch; tch = tch->next)
		{
			if (IS_NPC(tch) || (!tch->desc && !RENTABLE(tch)))
				continue;
			// ���
			if (!subcmd)
			{
				it = CLAN(ch)->pkList.find(GET_UNIQUE(tch));
				if (it != CLAN(ch)->pkList.end())
					print_pkl(ch, info, it);
			}
			// ���
			else
			{
				it = CLAN(ch)->frList.find(GET_UNIQUE(tch));
				if (it != CLAN(ch)->frList.end())
					print_pkl(ch, info, it);
			}
		}
		if (info.str().empty())
			info << "������ � ��������� ������ �����������.\r\n";
		page_string(ch->desc, info.str(), 1);

	}
	else if (CompareParam(buffer2, "���") || CompareParam(buffer2, "all"))
	{
		// ������� ���� ������
		send_to_char(ch, "%s������ ������������ ���������:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		// ���
		if (!subcmd)
			for (ClanPkList::const_iterator it = CLAN(ch)->pkList.begin(); it != CLAN(ch)->pkList.end(); ++it)
				print_pkl(ch, info, it);
		// ���
		else
			for (ClanPkList::const_iterator it = CLAN(ch)->frList.begin(); it != CLAN(ch)->frList.end(); ++it)
				print_pkl(ch, info, it);

		if (info.str().empty())
			info << "������ � ��������� ������ �����������.\r\n";

		page_string(ch->desc, info.str(), 1);

	}
	else if ((CompareParam(buffer2, "��������") || CompareParam(buffer2, "add")) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
	{
		// ��������� ������
		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			send_to_char("���� �������� ��?\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (!unique)
		{
			send_to_char("������������ ��� �������� �� ������.\r\n", ch);
			return;
		}
		if (unique < 0)
		{
			send_to_char("�� ���� ���, ����� �������� ���� �� ����....\r\n", ch);
			return;
		}
		ClanMemberList::const_iterator it = CLAN(ch)->members.find(unique);
		if (it != CLAN(ch)->members.end())
		{
			send_to_char("������� �� ����� �������� ������ ������ ������?\r\n", ch);
			return;
		}
		boost::trim_if(buffer, boost::is_any_of(" \'"));
		if (buffer.empty())
		{
			send_to_char("����������� �����������������, �� ��� �� ��� ���.\r\n", ch);
			return;
		}

		// ��� ���� ���������
		ClanPkList::iterator it2;
		if (!subcmd)
			it2 = CLAN(ch)->pkList.find(unique);
		else
			it2 = CLAN(ch)->frList.find(unique);
		if ((!subcmd && it2 != CLAN(ch)->pkList.end()) || (subcmd && it2 != CLAN(ch)->frList.end()))
		{
			// ��� ��� ������� �� �����������, ��� ��������
			ClanMemberList::const_iterator rank_it = CLAN(ch)->members.find(it2->second->author);
			if (rank_it != CLAN(ch)->members.end() && rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
			{
				if (!subcmd)
					send_to_char("���� ������ ��� ��������� � ������ ������ ������� �� ������.\r\n", ch);
				else
					send_to_char("�������� ��� �������� � ������ ������ ������� �� ������.\r\n", ch);
				return;
			}
		}

		// ���������� ����� ����� ������/�����
		ClanPkPtr tempRecord(new ClanPk);
		tempRecord->author = GET_UNIQUE(ch);
		tempRecord->authorName = GET_NAME(ch);
		tempRecord->victimName = buffer2;
		name_convert(tempRecord->victimName);
		tempRecord->time = time(0);
		tempRecord->text = buffer;
		if (!subcmd)
			CLAN(ch)->pkList[unique] = tempRecord;
		else
			CLAN(ch)->frList[unique] = tempRecord;
		DESCRIPTOR_DATA *d;
		if ((d = DescByUID(unique)) && PRF_FLAGGED(d->character, PRF_PKL_MODE))
		{
			if (!subcmd)
				send_to_char(d->character, "%s������� '%s' �������� ��� � ������ ����� ������!%s\r\n", CCIRED(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
			else
				send_to_char(d->character, "%s������� '%s' �������� ��� � ������ ����� ������!%s\r\n", CCGRN(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
			set_wait(ch, 1, FALSE);
		}
		send_to_char("�������, ��������.\r\n", ch);

	}
	else if ((CompareParam(buffer2, "�������") || CompareParam(buffer2, "delete")) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
	{
		// �������� ������
		GetOneParam(buffer, buffer2);
		if (CompareParam(buffer2, "���", 1) || CompareParam(buffer2, "all", 1))
		{
			if (CLAN_MEMBER(ch)->rank_num)
			{
				send_to_char("������ ������� ������ �������� ������ �������.\r\n", ch);
				return;
			}
			// ���
			if (!subcmd)
				CLAN(ch)->pkList.erase(CLAN(ch)->pkList.begin(), CLAN(ch)->pkList.end());
			// ���
			else
				CLAN(ch)->frList.erase(CLAN(ch)->frList.begin(), CLAN(ch)->frList.end());
			send_to_char("������ ������.\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (unique <= 0)
		{
			send_to_char("������������ ��� �������� �� ������.\r\n", ch);
			return;
		}
		ClanPkList::iterator it;
		// ���, ��������� ��� ��� ������ ��������, ��� �� ���� ��� subcmd ���������
		if (!subcmd)
		{
			it = CLAN(ch)->pkList.find(unique);
			if (it != CLAN(ch)->pkList.end())
			{
				ClanMemberList::const_iterator pk_rank_it = CLAN(ch)->members.find(it->second->author);
				if (pk_rank_it != CLAN(ch)->members.end() && pk_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
				{
					send_to_char("���� ������ ���� ��������� ������� �� ������.\r\n", ch);
					return;
				}
				CLAN(ch)->pkList.erase(it);
			}
			// ���
		}
		else
		{
			it = CLAN(ch)->frList.find(unique);
			if (it != CLAN(ch)->frList.end())
			{
				ClanMemberList::const_iterator fr_rank_it = CLAN(ch)->members.find(it->second->author);
				if (fr_rank_it != CLAN(ch)->members.end() && fr_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
				{
					send_to_char("�������� ��� �������� ������� �� ������.\r\n", ch);
					return;
				}
				CLAN(ch)->frList.erase(it);
			}
		}

		if (it != CLAN(ch)->pkList.end() && it != CLAN(ch)->frList.end())
		{
			send_to_char("������ �������.\r\n", ch);
			DESCRIPTOR_DATA *d;
			if ((d = DescByUID(unique)) && PRF_FLAGGED(d->character, PRF_PKL_MODE))
			{
				if (!subcmd)
					send_to_char(d->character, "%s������� '%s' ������� ��� �� ������ ����� ������!%s\r\n", CCGRN(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
				else
					send_to_char(d->character, "%s������� '%s' ������� ��� �� ������ ����� ������!%s\r\n", CCIRED(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
				set_wait(ch, 1, FALSE);
			}
		}
		else
			send_to_char("������ �� �������.\r\n", ch);

	}
	else
	{
		boost::trim(buffer);
		bool online = 1;

		if (CompareParam(buffer, "all") || CompareParam(buffer, "���"))
			online = 0;

		if (online)
			send_to_char(ch, "%s������������ ������ ����������� � ���� ���������:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		else
			send_to_char(ch, "%s������ ������������ ���������:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));

		std::ostringstream out;

		if (!subcmd)
		{
			for (ClanPkList::const_iterator it = CLAN(ch)->pkList.begin(); it != CLAN(ch)->pkList.end(); ++it)
			{
				if (CompareParam(buffer2, it->second->victimName, 1))
				{
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		}
		else
		{
			for (ClanPkList::const_iterator it = CLAN(ch)->frList.begin(); it != CLAN(ch)->frList.end(); ++it)
			{
				if (CompareParam(buffer2, it->second->victimName, 1))
				{
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		}
		if (!out.str().empty())
			page_string(ch->desc, out.str(), 1);
		else
		{
			send_to_char("�� ������ ������� ������ �� �������.\r\n", ch);
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer = CLAN_PKLIST_FORMAT[0];
				buffer += CLAN_PKLIST_FORMAT[1];
				send_to_char(buffer, ch);
			}
			else
				send_to_char(CLAN_PKLIST_FORMAT[0], ch);
		}
	}
}

// ������ � ������ (��� ������� ����������)
// ���� ������� - ������, �� ��������� ���� � ����-�����, ���������� ������ ������
bool Clan::PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !CLAN(ch)
			|| real_room(CLAN(ch)->chest_room) != IN_ROOM(ch)
			|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_PUT])
	{
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
	{
		long gold = GET_OBJ_VAL(obj, 0);
		// ����� � �����: � ������ ������������  - ������ ������� �����, ��������� ���������� ����
		// �� ������ ������ ��� ���������� � ������ �������� 2147483647 ���
		// �� ���� ��� ������ � ���� � �������� � ���� �� ����� ��������� ������
		if ((CLAN(ch)->bank + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money += over;
			gold -= over;
			add_gold(ch, gold);
			obj_from_char(obj);
			extract_obj(obj);
			send_to_char(ch, "�� ������� ������� � ����� ������� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			return 1;
		}
		CLAN(ch)->bank += gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "�� ������� � ����� ������� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));

	}
	else if (IS_OBJ_STAT(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ZONEDECAY) || GET_OBJ_TYPE(obj) == ITEM_KEY || IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 || GET_OBJ_RNUM(obj) <= NOTHING)
		act("��������� ���� �������� �������� $o3 � $O3.", FALSE, ch, obj, chest, TO_CHAR);
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
		act("� $o5 ���-�� �����.", FALSE, ch, obj, 0, TO_CHAR);
	else
	{
		if ((GET_OBJ_WEIGHT(chest) + GET_OBJ_WEIGHT(obj)) > CLAN(ch)->ChestMaxWeight() ||
				CLAN(ch)->chest_objcount == CLAN(ch)->ChestMaxObjects())
		{
			act("�� ���������� ��������� $o3 � $O3, �� �� ������ - ��� ������ ��� �����.", FALSE, ch, obj, chest, TO_CHAR);
			return 0;
		}
		obj_from_char(obj);
		obj_to_obj(obj, chest);

		// ����� ���������
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == CLAN(ch) && PRF_FLAGGED(d->character, PRF_TAKE_MODE))
				send_to_char(d->character, "[���������]: %s'%s ����%s %s.'%s\r\n", CCIRED(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_1(ch), obj->PNames[3], CCNRM(d->character, C_NRM));

		if (!PRF_FLAGGED(ch, PRF_DECAY_MODE))
			act("�� �������� $o3 � $O3.", FALSE, ch, obj, chest, TO_CHAR);
		CLAN(ch)->chest_objcount++;
	}
	return 1;
}

// ����� �� ����-������� (��� ������� ����������)
bool Clan::TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !CLAN(ch)
			|| real_room(CLAN(ch)->chest_room) != IN_ROOM(ch)
			|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_TAKE])
	{
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}

	obj_from_obj(obj);
	obj_to_char(obj, ch);
	if (obj->carried_by == ch)
	{
		// ����� ���������
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == CLAN(ch) && PRF_FLAGGED(d->character, PRF_TAKE_MODE))
				send_to_char(d->character, "[���������]: %s'%s ������%s %s.'%s\r\n", CCIRED(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_1(ch), obj->PNames[3], CCNRM(d->character, C_NRM));

		if (!PRF_FLAGGED(ch, PRF_TAKE_MODE))
			act("�� ����� $o3 �� $O1.", FALSE, ch, obj, chest, TO_CHAR);
		CLAN(ch)->chest_objcount--;
	}
	return 1;
}

// ��������� ��� ������� � �����
// �������� write_one_object (��� ������� ��� ���������, ��� ������� ���� ������� ����� � �����
// ���������� � ���� ������ ��� ��������� ���������� �� �������)
void Clan::ChestSave()
{
	OBJ_DATA *chest, *temp;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		std::string buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
			if (Clan::is_clan_chest(chest))
			{
				std::string buffer2;
				for (temp = chest->contains; temp; temp = temp->next_content)
				{
					char databuf[MAX_STRING_LENGTH];
					char *data = databuf;

					write_one_object(&data, temp, 0);
					buffer2 += databuf;
				}

				std::ofstream file(filename.c_str());
				if (!file.is_open())
				{
					log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					return;
				}
				file << "* Items file\n" << buffer2 << "\n$\n$\n";
				file.close();
				break;
			}
	}
}

// ������ ������ �������� ��������
// �������� read_one_object_new ��� ������ ������ ������� � �����
void Clan::ChestLoad()
{
	OBJ_DATA *obj, *chest, *temp, *obj_next;

	// TODO: ��� ������� ������� ��� ����� ��������� ��� ���� ������ ��� ������ ��� ����/�������� � ������� ��� chest
	// �������� � �� ����������, �� ������ ������� ������ � ��������� ������� � ���������� (����� � ���� �� ������� ������)

	// �� ������ ������� - ������ ����� ���� ��� ��� ���� � ��������
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp = chest->contains; temp; temp = obj_next)
				{
					obj_next = temp->next_content;
					obj_from_obj(temp);
					extract_obj(temp);
				}
				extract_obj(chest);
			}
		}
	}

	int error = 0, fsize = 0;
	FILE *fl;
	char *data, *databuf;
	std::string buffer;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";

		//������ ������. � ����� ��� ������� �� �����.
		chest = read_object(CLAN_CHEST_VNUM, VIRTUAL);
		if (chest)
			obj_to_room(chest, real_room((*clan)->chest_room));

		if (!chest)
		{
			log("<Clan> Chest load error '%s'! (%s %s %d)", (*clan)->abbrev.c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		if (!(fl = fopen(filename.c_str(), "r+b")))
			continue;
		fseek(fl, 0L, SEEK_END);
		fsize = ftell(fl);
		if (!fsize)
		{
			fclose(fl);
			log("<Clan> Empty obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		databuf = new char [fsize + 1];

		fseek(fl, 0L, SEEK_SET);
		if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf)
		{
			fclose(fl);
			log("<Clan> Error reading obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		fclose(fl);

		data = databuf;
		*(data + fsize) = '\0';

		for (fsize = 0; *data && *data != '$'; fsize++)
		{
			if (!(obj = read_one_object_new(&data, &error)))
			{
				if (error)
					log("<Clan> Items reading fail for %s error %d.", filename.c_str(), error);
				continue;
			}
			obj_to_obj(obj, chest);
		}
		delete [] databuf;
	}
}

// ������� ���� � �������� � ����� �� ����� �� �����
void Clan::ChestUpdate()
{
	double i, cost;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		cost = (*clan)->ChestTax();
		// ������ � �������� �� ����� (����� ����� �� �����������) ����� ������ �����
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);
		cost = (cost * CHEST_UPDATE_PERIOD) / (60 * 24);

		(*clan)->bankBuffer += cost;
		modf((*clan)->bankBuffer, &i);
		if (i)
		{
			(*clan)->bank -= (long) i;
			(*clan)->bankBuffer -= i;
		}
		// ��� ������� ����� ��� ������ � ������� ������
		// TODO: � ��� �������� ����� ������, ��� ���� ������ ��� �������, ���� �������
		if ((*clan)->bank < 0)
		{
			(*clan)->bank = 0;
			OBJ_DATA *temp, *chest, *obj_next;
			for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
			{
				if (Clan::is_clan_chest(chest))
				{
					for (temp = chest->contains; temp; temp = obj_next)
					{
						obj_next = temp->next_content;
						obj_from_obj(temp);
						extract_obj(temp);
					}
					break;
				}
			}
		}
	}
}

//��������� ������ �� ���� ����
void Clan::TaxManage(CHAR_DATA * ch, std::string & arg)
{
	if (IS_NPC(ch) || !CLAN(ch))
		return;
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		send_to_char("����� ������.\r\n", ch);
		return;
	}

	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);
	long value = atol(buffer2.c_str());

	if (value < 0 || value > 100)
	{
		send_to_char("����� ����� �� ������ ����������? ������� �� 0 �� 100.\r\n", ch);
		return;
	}
	else
	{
		if (value == 0)
			send_to_char("������� - �� �����! �� � ������ �� ��.\r\n", ch);
		else if (value < 100)
			send_to_char(ch, "������ %d%% ������ ����� ���� �� ��������������� �����.\r\n"
						 "����������, ��� ������ ���� �� ����������� � ���� ������ � ���� ������ ������ �� ����. �)\r\n", value);
		else
			send_to_char(ch, "�� �������������� ������� ���� ���������� ���� ����� �������.\r\n"
						 "����������, ��� ������ ���� �� ����������� � ���� ������ � ���� ������ ������ �� ����. �)\r\n");

		CLAN_MEMBER(ch)->exp_persent = value;
	}
}

// ����� �������... ������� ���� ����� � ���������� '�����' � ������
// ��������/���������� ����� ���, ������� �� ����������, ����� �� ����������� ��������
bool Clan::BankManage(CHAR_DATA * ch, char *arg)
{
	if (IS_NPC(ch) || !CLAN(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
		return 0;

	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "������") || CompareParam(buffer2, "balance"))
	{
		send_to_char(ch, "�� ����� ����� ������� ����� %ld %s.\r\n", CLAN(ch)->bank, desc_count(CLAN(ch)->bank, WHAT_MONEYu));
		return 1;

	}
	else if (CompareParam(buffer2, "�������") || CompareParam(buffer2, "deposit"))
	{
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0)
		{
			send_to_char("������� �� ������ �������?\r\n", ch);
			return 1;
		}
		if (get_gold(ch) < gold)
		{
			send_to_char("� ����� ����� �� ������ ������ �������!\r\n", ch);
			return 1;
		}

		// �� ������ ������������ �����
		if ((CLAN(ch)->bank + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money += over;
			add_gold(ch, -over);
			send_to_char(ch, "�� ������� ������� � ����� ������� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		add_gold(ch, -gold);
		CLAN(ch)->bank += gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money += gold;
		send_to_char(ch, "�� ������� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	}
	else if (CompareParam(buffer2, "��������") || CompareParam(buffer2, "withdraw"))
	{
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_BANK])
		{
			send_to_char("� ���������, � ��� ��� ����������� ���������� �������� �������.\r\n", ch);
			return 1;
		}
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0)
		{
			send_to_char("�������� ���������� �����, ������� �� ������ ��������?\r\n", ch);
			return 1;
		}
		if (CLAN(ch)->bank < gold)
		{
			send_to_char("� ���������, ���� ������� �� ��� ������.\r\n", ch);
			return 1;
		}

		// �� ������ ������������ ���������
		if ((get_gold(ch) + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - get_gold(ch);
			add_gold(ch, over);
			CLAN(ch)->bank -= over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money -= over;
			send_to_char(ch, "�� ������� ����� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		CLAN(ch)->bank -= gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money -= gold;
		add_gold(ch, gold);
		send_to_char(ch, "�� ����� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	}
	else
		send_to_char(ch, "������ �������: ����� �������|��������|������ �����.\r\n");
	return 1;
}

void Clan::Manage(DESCRIPTOR_DATA * d, const char *arg)
{
	unsigned num = atoi(arg);

	switch (d->clan_olc->mode)
	{
	case CLAN_MAIN_MENU:
		switch (*arg)
		{
		case '�':
		case '�':
		case 'q':
		case 'Q':
			// ���� �������, ��� �� ����� � ��� � ����� ������� ���-�� ������
			if (d->clan_olc->clan->privileges.size() != d->clan_olc->privileges.size())
			{
				send_to_char("�� ����� �������������� ���������� � ����� ������� ���� �������� ���������� ������.\r\n"
							 "�� ��������� �������������� ������� � ���� ��� ���.\r\n"
							 "�������������� ��������.\r\n", d->character);
				d->clan_olc.reset();
				STATE(d) = CON_PLAYING;
				return;
			}
			send_to_char("�� ������� ��������� ���������? Y(�)/N(�) : ", d->character);
			d->clan_olc->mode = CLAN_SAVE_MENU;
			return;
		default:
			if (!*arg || !num)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			// ��� ������ ������� ��� (���� ���������� � 1 + ���� ���� ����)
			unsigned choise = num + CLAN_MEMBER(d->character)->rank_num;
			if (choise >= d->clan_olc->clan->ranks.size())
			{
				unsigned i = choise - d->clan_olc->clan->ranks.size();
				if (i == 0)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 1)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 2 && !CLAN_MEMBER(d->character)->rank_num)
				{
					if (d->clan_olc->clan->storehouse)
						d->clan_olc->clan->storehouse = 0;
					else
						d->clan_olc->clan->storehouse = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else if (i == 3 && !CLAN_MEMBER(d->character)->rank_num)
				{
					if (d->clan_olc->clan->exp_info)
						d->clan_olc->clan->exp_info = 0;
					else
						d->clan_olc->clan->exp_info = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else
				{
					send_to_char("�������� �����!\r\n", d->character);
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				return;
			}

			if (choise >= d->clan_olc->clan->ranks.size())
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			d->clan_olc->rank = choise;
			d->clan_olc->clan->PrivilegeMenu(d, choise);
			return;
		}
		break;
	case CLAN_PRIVILEGE_MENU:
		switch (*arg)
		{
		case '�':
		case '�':
		case 'q':
		case 'Q':
			// ����� � ����� ����
			d->clan_olc->rank = 0;
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			// �� ������ ������ ��������� ���������� � ���� � ��������� ������ �� ���
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->privileges[d->clan_olc->rank][parse_num])
				d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 0;
			else
				d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 1;
			d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
			return;
		}
		break;
	case CLAN_SAVE_MENU:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case '�':
		case '�':
			d->clan_olc->clan->privileges.clear();
			d->clan_olc->clan->privileges = d->clan_olc->privileges;
			d->clan_olc.reset();
			Clan::ClanSave();
			STATE(d) = CON_PLAYING;
			send_to_char("��������� ���������.\r\n", d->character);
			return;
		case 'n':
		case 'N':
		case '�':
		case '�':
			d->clan_olc.reset();
			STATE(d) = CON_PLAYING;
			send_to_char("�������������� ��������.\r\n", d->character);
			return;
		default:
			send_to_char("�������� �����!\r\n�� ������� ��������� ���������? Y(�)/N(�) : ", d->character);
			d->clan_olc->mode = CLAN_SAVE_MENU;
			return;
		}
		break;
	case CLAN_ADDALL_MENU:
		switch (*arg)
		{
		case '�':
		case '�':
		case 'q':
		case 'Q':
			// ����� � ����� ���� � ���������� ���� ������
			for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			{
				if (d->clan_olc->all_ranks[i])
				{
					unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j)
					{
						d->clan_olc->privileges[j][i] = 1;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->all_ranks[parse_num])
				d->clan_olc->all_ranks[parse_num] = 0;
			else
				d->clan_olc->all_ranks[parse_num] = 1;
			d->clan_olc->clan->AllMenu(d, 0);
			return;
		}
		break;
	case CLAN_DELALL_MENU:
		switch (*arg)
		{
		case '�':
		case '�':
		case 'q':
		case 'Q':
			// ����� � ����� ���� � ���������� ���� ������
			for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			{
				if (d->clan_olc->all_ranks[i])
				{
					unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j)
					{
						d->clan_olc->privileges[j][i] = 0;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->all_ranks[parse_num])
				d->clan_olc->all_ranks[parse_num] = 0;
			else
				d->clan_olc->all_ranks[parse_num] = 1;
			d->clan_olc->clan->AllMenu(d, 1);
			return;
		}
		break;
	default:
		log("No case, wtf!? mode: %d (%s %s %d)", d->clan_olc->mode, __FILE__, __func__, __LINE__);
	}
}

void Clan::MainMenu(DESCRIPTOR_DATA * d)
{
	std::ostringstream buffer;
	buffer << "������ ���������� � �������� ����������.\r\n"
	<< "�������� ������������� ������ ��� ��������:\r\n";
	// ������ ���� ������
	int rank = CLAN_MEMBER(d->character)->rank_num + 1;
	int num = 0;
	for (std::vector<std::string>::const_iterator it = d->clan_olc->clan->ranks.begin() + rank; it != d->clan_olc->clan->ranks.end(); ++it)
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM) << ") " << (*it) << "\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
	<< ") " << "�������� ����\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
	<< ") " << "������ � ����\r\n";
	if (!CLAN_MEMBER(d->character)->rank_num)
	{
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "������� �� ��������� �� ���������� ���������\r\n"
		<< "    + �������� ��������� ��� ����� � �������� ������ (1000 ��� � ����) ";
		if (this->storehouse)
			buffer << "(���������)\r\n";
		else
			buffer << "(��������)\r\n";
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "����������� ��������� ���� ������� � ���� (��� ���������� ���������� ��� � 6 �����) ";
		if (this->exp_info)
			buffer << "(���������)\r\n";
		else
			buffer << "(��������)\r\n";
	}
	buffer << CCGRN(d->character, C_NRM) << " �(Q)" << CCNRM(d->character, C_NRM)
	<< ") �����\r\n" << "��� �����:";

	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_MAIN_MENU;
}

void Clan::PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num)
{
	if (num > d->clan_olc->clan->ranks.size())
	{
		log("Different size clan->ranks and clan->privileges! (%s %s %d)", __FILE__, __func__, __LINE__);
		if (d->character)
		{
			d->clan_olc.reset();
			STATE(d) = CON_PLAYING;
			send_to_char(d->character, "��������� ���-�� ��������, �������� �����!");
		}
		return;
	}
	std::ostringstream buffer;
	buffer << "������ ���������� ��� ������ '" << CCIRED(d->character, C_NRM)
	<< d->clan_olc->clan->ranks[num] << CCNRM(d->character, C_NRM) << "':\r\n";

	int count = 0;
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
	{
		switch (i)
		{
		case MAY_CLAN_INFO:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_INFO])
					buffer << "[x] ���������� � ������� (���� ����������)\r\n";
				else
					buffer << "[ ] ���������� � ������� (���� ����������)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_ADD])
					buffer << "[x] �������� � ������� (���� �������)\r\n";
				else
					buffer << "[ ] �������� � ������� (���� �������)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_REMOVE])
					buffer << "[x] �������� �� ������� (���� �������)\r\n";
				else
					buffer << "[ ] �������� �� ������� (���� �������)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PRIVILEGES])
					buffer << "[x] �������������� ���������� (���� ����������)\r\n";
				else
					buffer << "[ ] �������������� ���������� (���� ����������)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHANNEL])
					buffer << "[x] ����-����� (�������)\r\n";
				else
					buffer << "[ ] ����-����� (�������)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_POLITICS])
					buffer << "[x] ��������� �������� ������� (��������)\r\n";
				else
					buffer << "[ ] ��������� �������� ������� (��������)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_NEWS])
					buffer << "[x] ���������� �������� ������� (���������)\r\n";
				else
					buffer << "[ ] ���������� �������� ������� (���������)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PKLIST])
					buffer << "[x] ���������� � ��-���� (������)\r\n";
				else
					buffer << "[ ] ���������� � ��-���� (������)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_PUT])
					buffer << "[x] ������ ���� � ���������\r\n";
				else
					buffer << "[ ] ������ ���� � ���������\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_TAKE])
					buffer << "[x] ����� ���� �� ���������\r\n";
				else
					buffer << "[ ] ����� ���� �� ���������\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_BANK])
					buffer << "[x] ����� �� ����� �������\r\n";
				else
					buffer << "[ ] ����� �� ����� �������\r\n";
			}
			break;
		case MAY_CLAN_EXIT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_EXIT])
					buffer << "[x] ��������� ����� �� �������\r\n";
				else
					buffer << "[ ] ��������� ����� �� �������\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " �(Q)" << CCNRM(d->character, C_NRM)
	<< ") �����\r\n" << "��� �����:";
	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_PRIVILEGE_MENU;
}

// ���� ����������/�������� ���������� � ���� ������, ����� �� ��������� ��� � ���� �������
// flag 0 - ����������, 1 - ��������
void Clan::AllMenu(DESCRIPTOR_DATA * d, unsigned flag)
{
	std::ostringstream buffer;
	if (flag == 0)
		buffer << "�������� ����������, ������� �� ������ " << CCIRED(d->character, C_NRM)
		<< "�������� ����" << CCNRM(d->character, C_NRM) << " �������:\r\n";
	else
		buffer << "�������� ����������, ������� �� ������ " << CCIRED(d->character, C_NRM)
		<< "������ � ����" << CCNRM(d->character, C_NRM) << " ������:\r\n";

	int count = 0;
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
	{
		switch (i)
		{
		case MAY_CLAN_INFO:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_INFO])
					buffer << "[x] ���������� � ������� (���� ����������)\r\n";
				else
					buffer << "[ ] ���������� � ������� (���� ����������)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_ADD])
					buffer << "[x] �������� � ������� (���� �������)\r\n";
				else
					buffer << "[ ] �������� � ������� (���� �������)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_REMOVE])
					buffer << "[x] �������� �� ������� (���� �������)\r\n";
				else
					buffer << "[ ] �������� �� ������� (���� �������)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PRIVILEGES])
					buffer << "[x] �������������� ���������� (���� ����������)\r\n";
				else
					buffer << "[ ] �������������� ���������� (���� ����������)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHANNEL])
					buffer << "[x] ����-����� (�������)\r\n";
				else
					buffer << "[ ] ����-����� (�������)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_POLITICS])
					buffer << "[x] ��������� �������� ������� (��������)\r\n";
				else
					buffer << "[ ] ��������� �������� ������� (��������)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_NEWS])
					buffer << "[x] ���������� �������� ������� (���������)\r\n";
				else
					buffer << "[ ] ���������� �������� ������� (���������)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PKLIST])
					buffer << "[x] ���������� � ��-���� (������)\r\n";
				else
					buffer << "[ ] ���������� � ��-���� (������)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_PUT])
					buffer << "[x] ������ ���� � ���������\r\n";
				else
					buffer << "[ ] ������ ���� � ���������\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_TAKE])
					buffer << "[x] ����� ���� �� ���������\r\n";
				else
					buffer << "[ ] ����� ���� �� ���������\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_BANK])
					buffer << "[x] ����� �� ����� �������\r\n";
				else
					buffer << "[ ] ����� �� ����� �������\r\n";
			}
			break;
		case MAY_CLAN_EXIT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_EXIT])
					buffer << "[x] ��������� ����� �� �������\r\n";
				else
					buffer << "[ ] ��������� ����� �� �������\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " �(Q)" << CCNRM(d->character, C_NRM)
	<< ") ���������\r\n" << "��� �����:";
	send_to_char(buffer.str(), d->character);
	if (flag == 0)
		d->clan_olc->mode = CLAN_ADDALL_MENU;
	else
		d->clan_olc->mode = CLAN_DELALL_MENU;
}

// ����� ����
void Clan::ClanAddMember(CHAR_DATA * ch, int rank)
{
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = GET_NAME(ch);
	tempMember->rank_num = rank;

	this->members[GET_UNIQUE(ch)] = tempMember;
	Clan::SetClanData(ch);
	DESCRIPTOR_DATA *d;
	std::string buffer;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character && CLAN(d->character) && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				&& this->GetRent() == CLAN(d->character)->GetRent() && ch != d->character)
		{
			send_to_char(d->character, "%s%s ��������%s � ����� �������, ������ - '%s'.%s\r\n",
						 CCWHT(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_6(ch), (this->ranks[rank]).c_str(), CCNRM(d->character, C_NRM));
		}
	}
	send_to_char(ch, "%s��� ��������� � ������� '%s', ������ - '%s'.%s\r\n", CCWHT(ch, C_NRM), this->name.c_str(), (this->ranks[rank]).c_str(), CCNRM(ch, C_NRM));
	Clan::ClanSave();
	return;
}

// �������� ����������
void Clan::HouseOwner(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	DESCRIPTOR_DATA *d = DescByUID(unique);

	if (buffer2.empty())
		send_to_char("������� ��� ���������.\r\n", ch);
	else if (!unique)
		send_to_char("����������� ��������.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("������� ���� �� ������ ����? �� �������.\r\n", ch);
	else if (!d || !CAN_SEE(ch, d->character))
		send_to_char("����� ��������� ��� � ����!\r\n", ch);
	else if (CLAN(d->character) && CLAN(ch) != CLAN(d->character))
		send_to_char("�� �� ������ �������� ���� ����� ����� ������ �������.\r\n", ch);
	else
	{
		buffer2[0] = UPPER(buffer2[0]);
		// ������� ���� ������ ����
		this->members[GET_UNIQUE(ch)]->rank_num = 1;
		Clan::SetClanData(ch);
		// ������ ������ ������� (���� �� ��� � ����� - ������ ������ ����)
		if (CLAN(d->character))
		{
			this->members[GET_UNIQUE(d->character)]->rank_num = 0;
			Clan::SetClanData(d->character);
		}
		else
			this->ClanAddMember(d->character, 0);
		this->owner = buffer2;
		send_to_char(ch, "�����������, �� �������� ���� ���������� %s!\r\n", GET_PAD(d->character, 2));
	}
}

void Clan::CheckPkList(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return;
	ClanPkList::iterator it;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		// ���
		if ((it = (*clan)->pkList.find(GET_UNIQUE(ch))) != (*clan)->pkList.end())
			send_to_char(ch, "���������� � ������ ������ ������� '%s', ����������: %s.\r\n", (*clan)->name.c_str(), it->second->authorName.c_str());
		// ���
		if ((it = (*clan)->frList.find(GET_UNIQUE(ch))) != (*clan)->frList.end())
			send_to_char(ch, "���������� � ������ ������ ������� '%s', ����������: %s.\r\n", (*clan)->name.c_str(), it->second->authorName.c_str());
	}
}

// ���������� ��� �����... ���������� num � ��� ���� � flags
bool CompareBits(FLAG_DATA flags, const char *names[], int affect)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		int nr = 0, fail = 0;
		bitvector_t bitvector = flags.flags[i] | (i << 30);
		if ((unsigned long) bitvector < (unsigned long) INT_ONE);
		else if ((unsigned long) bitvector < (unsigned long) INT_TWO)
			fail = 1;
		else if ((unsigned long) bitvector < (unsigned long) INT_THREE)
			fail = 2;
		else
			fail = 3;
		bitvector &= 0x3FFFFFFF;
		while (fail)
		{
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1)
		{
			if (IS_SET(bitvector, 1))
				if (*names[nr] != '\n')
					if (nr == affect)
						return 1;
			if (*names[nr] != '\n')
				nr++;
		}
	}
	return 0;
}

// ��� ������� ����������� ��������� �� ��������
struct ChestFilter
{
	std::string name;      // ���
	int type;              // ���
	int state;             // ���������
	int wear;              // ���� ���������
	int wear_message;      // ��� �������� ���� �����
	int weap_class;        // ����� ������
	int weap_message;      // ��� �������� ������
	vector<int> affect;    // ������� weap
	vector<int> affect2;   // ������� apply
};

// ������ ��� ����-���� �� ����� + �����
ACMD(DoStoreHouse)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("���� ?\r\n", ch);
		return;
	}
	if (!CLAN(ch)->storehouse)
	{
		send_to_char("��� ������� ����� ����� � �������� ��� �����������! :(\r\n", ch);
		return;
	}

	OBJ_DATA *temp_obj, *chest;

	skip_spaces(&argument);
	if (!*argument)
	{
		for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				Clan::ChestShow(chest, ch);
				return;
			}
		}
	}

	char *stufina = one_argument(argument, arg);
	skip_spaces(&stufina);


	if (is_abbrev(arg, "��������������") || is_abbrev(arg, "identify") || is_abbrev(arg, "��������"))
	{
		if ((get_bank_gold(ch) < CHEST_IDENT_PAY) && (GET_LEVEL(ch) < LVL_IMPL))
		{
			send_to_char("� ��� ������������ ����� � ����� ��� ������ ������������.\r\n", ch);
			return;
		}

		for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content)
				{
					if (isname(stufina, temp_obj->name))
					{
						sprintf(buf1, "�������������� ��������: %s\r\n", stufina);
						send_to_char(buf1, ch);
						GET_LEVEL(ch) < LVL_IMPL ? mort_show_obj_values(temp_obj, ch, 200) : imm_show_obj_values(temp_obj, ch);
						add_bank_gold(ch, -(CHEST_IDENT_PAY));
						sprintf(buf1, "%s�� ���������� � �������� � ������ ����������� ����� ����� %d %s%s\r\n",  CCIGRN(ch, C_NRM), CHEST_IDENT_PAY, desc_count(CHEST_IDENT_PAY, WHAT_MONEYu), CCNRM(ch, C_NRM));
						send_to_char(buf1, ch);
						return;
					}
				}
			}
		}

		sprintf(buf1, "������ �������� �� %s � ��������� ���������! ������ ������������.\r\n", stufina);
		send_to_char(buf1, ch);
		return;

	}

	// ��� ������� ���������� ��� ��� ���, ������ ��������� ��� � ����������
	ChestFilter filter;
	filter.type = -1;
	filter.state = -1;
	filter.wear = -1;
	filter.wear_message = -1;
	filter.weap_class = -1;
	filter.weap_message = -1;

	int num = 0;
	bool find = 0;
	char tmpbuf[MAX_INPUT_LENGTH];
	while (*argument)
	{
		switch (*argument)
		{
		case '�':
			argument = one_argument(++argument, tmpbuf);
			filter.name = tmpbuf;
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "����") || is_abbrev(tmpbuf, "light"))
				filter.type = ITEM_LIGHT;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "scroll"))
				filter.type = ITEM_SCROLL;
			else if (is_abbrev(tmpbuf, "�������") || is_abbrev(tmpbuf, "wand"))
				filter.type = ITEM_WAND;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "staff"))
				filter.type = ITEM_STAFF;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "weapon"))
				filter.type = ITEM_WEAPON;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "armor"))
				filter.type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "�������") || is_abbrev(tmpbuf, "potion"))
				filter.type = ITEM_POTION;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "other"))
				filter.type = ITEM_OTHER;
			else if (is_abbrev(tmpbuf, "���������") || is_abbrev(tmpbuf, "container"))
				filter.type = ITEM_CONTAINER;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "book"))
				filter.type = ITEM_BOOK;
			else if (is_abbrev(tmpbuf, "����") || is_abbrev(tmpbuf, "rune"))
				filter.type = ITEM_INGRADIENT;
			else if (is_abbrev(tmpbuf, "����������") || is_abbrev(tmpbuf, "ingradient"))
				filter.type = ITEM_MING;
			else
			{
				send_to_char("�������� ��� ��������.\r\n", ch);
				return;
			}
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "������"))
				filter.state = 0;
			else if (is_abbrev(tmpbuf, "����� ����������"))
				filter.state = 20;
			else if (is_abbrev(tmpbuf, "���������"))
				filter.state = 40;
			else if (is_abbrev(tmpbuf, "������"))
				filter.state = 60;
			else if (is_abbrev(tmpbuf, "��������"))
				filter.state = 80;
			else
			{
				send_to_char("�������� ��������� ��������.\r\n", ch);
				return;
			}

			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "�����"))
			{
				filter.wear = ITEM_WEAR_FINGER;
				filter.wear_message = 1;
			}
			else if (is_abbrev(tmpbuf, "���") || is_abbrev(tmpbuf, "�����"))
			{
				filter.wear = ITEM_WEAR_NECK;
				filter.wear_message = 2;
			}
			else if (is_abbrev(tmpbuf, "����"))
			{
				filter.wear = ITEM_WEAR_BODY;
				filter.wear_message = 3;
			}
			else if (is_abbrev(tmpbuf, "������"))
			{
				filter.wear = ITEM_WEAR_HEAD;
				filter.wear_message = 4;
			}
			else if (is_abbrev(tmpbuf, "����"))
			{
				filter.wear = ITEM_WEAR_LEGS;
				filter.wear_message = 5;
			}
			else if (is_abbrev(tmpbuf, "������"))
			{
				filter.wear = ITEM_WEAR_FEET;
				filter.wear_message = 6;
			}
			else if (is_abbrev(tmpbuf, "�����"))
			{
				filter.wear = ITEM_WEAR_HANDS;
				filter.wear_message = 7;
			}
			else if (is_abbrev(tmpbuf, "����"))
			{
				filter.wear = ITEM_WEAR_ARMS;
				filter.wear_message = 8;
			}
			else if (is_abbrev(tmpbuf, "���"))
			{
				filter.wear = ITEM_WEAR_SHIELD;
				filter.wear_message = 9;
			}
			else if (is_abbrev(tmpbuf, "�����"))
			{
				filter.wear = ITEM_WEAR_ABOUT;
				filter.wear_message = 10;
			}
			else if (is_abbrev(tmpbuf, "����"))
			{
				filter.wear = ITEM_WEAR_WAIST;
				filter.wear_message = 11;
			}
			else if (is_abbrev(tmpbuf, "��������"))
			{
				filter.wear = ITEM_WEAR_WRIST;
				filter.wear_message = 12;
			}
			else if (is_abbrev(tmpbuf, "������"))
			{
				filter.wear = ITEM_WEAR_WIELD;
				filter.wear_message = 13;
			}
			else if (is_abbrev(tmpbuf, "�����"))
			{
				filter.wear = ITEM_WEAR_HOLD;
				filter.wear_message = 14;
			}
			else if (is_abbrev(tmpbuf, "���"))
			{
				filter.wear = ITEM_WEAR_BOTHS;
				filter.wear_message = 15;
			}
			else
			{
				send_to_char("�������� ����� �������� ��������.\r\n", ch);
				return;
			}
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "����"))
			{
				filter.weap_class = SKILL_BOWS;
				filter.weap_message = 0;
			}
			else if (is_abbrev(tmpbuf, "��������"))
			{
				filter.weap_class = SKILL_SHORTS;
				filter.weap_message = 1;
			}
			else if (is_abbrev(tmpbuf, "�������"))
			{
				filter.weap_class = SKILL_LONGS;
				filter.weap_message = 2;
			}
			else if (is_abbrev(tmpbuf, "������"))
			{
				filter.weap_class = SKILL_AXES;
				filter.weap_message = 3;
			}
			else if (is_abbrev(tmpbuf, "������"))
			{
				filter.weap_class = SKILL_CLUBS;
				filter.weap_message = 4;
			}
			else if (is_abbrev(tmpbuf, "����"))
			{
				filter.weap_class = SKILL_NONSTANDART;
				filter.weap_message = 5;
			}
			else if (is_abbrev(tmpbuf, "����������"))
			{
				filter.weap_class = SKILL_BOTHHANDS;
				filter.weap_message = 6;
			}
			else if (is_abbrev(tmpbuf, "�����������"))
			{
				filter.weap_class = SKILL_PICK;
				filter.weap_message = 7;
			}
			else if (is_abbrev(tmpbuf, "�����"))
			{
				filter.weap_class = SKILL_SPADES;
				filter.weap_message = 8;
			}
			else
			{
				send_to_char("�������� ����� ������.\r\n", ch);
				return;
			}
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (filter.affect.size() + filter.affect2.size() >= 2)
				break;
			find = 0;
			num = 0;
			for (int flag = 0; flag < 4; ++flag)
			{
				for (/* ��� ������ �� ���� */; *weapon_affects[num] != '\n'; ++num)
				{
					if (is_abbrev(tmpbuf, weapon_affects[num]))
					{
						filter.affect.push_back(num);
						find = 1;
						break;
					}
				}
				if (find)
					break;
				++num;
			}
			if (!find)
			{
				for (num = 0; *apply_types[num] != '\n'; ++num)
				{
					if (is_abbrev(tmpbuf, apply_types[num]))
					{
						filter.affect2.push_back(num);
						find = 1;
						break;
					}
				}
			}
			if (!find)
			{
				send_to_char("�������� ������ ��������.\r\n", ch);
				return;
			}
			break;
		default:
			++argument;
		}
	}
	std::string buffer = "������� �� ��������� ����������: ";
	if (!filter.name.empty())
		buffer += filter.name + " ";
	if (filter.type >= 0)
	{
		buffer += item_types[filter.type];
		buffer += " ";
	}
	if (filter.state >= 0)
	{
		if (!filter.state)
			buffer += "������ ";
		else if (filter.state == 20)
			buffer += "����� ���������� ";
		else if (filter.state == 40)
			buffer += "��������� ";
		else if (filter.state == 60)
			buffer += "������ ";
		else if (filter.state == 80)
			buffer += "�������� ";
	}
	if (filter.wear >= 0)
	{
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (filter.weap_class >= 0)
	{
		buffer += weapon_class[filter.weap_message];
		buffer += " ";
	}
	if (!filter.affect.empty())
	{
		for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
		{
			buffer += weapon_affects[*it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty())
	{
		for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end(); ++it)
		{
			buffer += apply_types[*it];
			buffer += " ";
		}
	}
	buffer += "\r\n";
	send_to_char(buffer, ch);
	set_wait(ch, 1, FALSE);

	std::ostringstream out;
	for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
	{
		if (Clan::is_clan_chest(chest))
		{
			for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content)
			{
				std::ostringstream modif;
				// ������� ���
				if (!filter.name.empty() && !CompareParam(filter.name, temp_obj->name))
					continue;
				// ���
				if (filter.type >= 0 && filter.type != GET_OBJ_TYPE(temp_obj))
					continue;
				// ������
				if (filter.state >= 0)
				{
					if (!GET_OBJ_TIMER(obj_proto[GET_OBJ_RNUM(temp_obj)]))
					{
						send_to_char("������� ������ ���������, �������� �����!", ch);
						return;
					}
					int tm = (GET_OBJ_TIMER(temp_obj) * 100 / GET_OBJ_TIMER(obj_proto[GET_OBJ_RNUM(temp_obj)]));
					if ((tm + 1) < filter.state || (tm + 1) > (filter.state + 20))
						continue;
				}
				// ���� ����� �����
				if (filter.wear >= 0 && !CAN_WEAR(temp_obj, filter.wear))
					continue;
				// ����� ������
				if (filter.weap_class >= 0 && filter.weap_class != GET_OBJ_SKILL(temp_obj))
					continue;
				// �������
				find = 1;
				if (!filter.affect.empty())
				{
					for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
					{
						if (!CompareBits(temp_obj->obj_flags.affects, weapon_affects, *it))
						{
							find = 0;
							break;
						}
					}
					// ������ ����� �� �������, ���������� ������ ���
					if (!find)
						continue;
				}
				if (!filter.affect2.empty())
				{
					for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it)
					{
						find = 0;
						for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
						{
							if (temp_obj->affected[i].location == *it)
							{
								if (temp_obj->affected[i].modifier > 0)
									modif << "+";
								else
									modif << "-";
								find = 1;
								break;
							}
						}
					}
				}
				if (find)
				{
					if (!modif.str().empty())
						out << modif.str() << " ";
					out << show_obj_to_char(temp_obj, ch, 1, 3, 1);
				}
			}
			break;
		}
	}
	if (!out.str().empty())
		page_string(ch->desc, out.str(), TRUE);
	else
		send_to_char("������ �� �������.\r\n", ch);
}

void Clan::HouseStat(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	bool all = 0, name = 0;
	std::ostringstream out;
	// �������� ����������
	enum ESortParam
	{
		SORT_STAT_BY_EXP,
		SORT_STAT_BY_CLANEXP,
		SORT_STAT_BY_MONEY,
		SORT_STAT_BY_EXPPERCENT,
		SORT_STAT_BY_LOGON,
		SORT_STAT_BY_NAME
	};
	ESortParam sortParameter;
	long long lSortParam;

	// �.�. � ���8-� ������� ����� �� ���������
	const char *pSortAlph = "������������������������֣������";
	// ������ ����� �����
	char pcFirstChar[2];

	// ��� ��������� �������� � ������� ������ ���������� �� ����� "!"
	// ������ �������:
	// ���� ���� [!����/!������������/!�����/!����������] [���/���]
	sortParameter = SORT_STAT_BY_EXP;
	if (buffer2.length() > 1)
	{
		if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "�����"))) // ����� �������
			sortParameter = SORT_STAT_BY_CLANEXP;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "������������")))
			sortParameter = SORT_STAT_BY_MONEY;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "�����")))
			sortParameter = SORT_STAT_BY_EXPPERCENT;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "����������")))
			sortParameter = SORT_STAT_BY_LOGON;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "���")))
			sortParameter = SORT_STAT_BY_NAME;

		// ����� ��������� ��������
		if (buffer2[0] == '!') GetOneParam(buffer, buffer2);
	}

	out << CCWHT(ch, C_NRM);
	if (CompareParam(buffer2, "��������") || CompareParam(buffer2, "�������"))
	{
		if (CLAN_MEMBER(ch)->rank_num)
		{
			send_to_char("� ��� ��� ���� ������� ����������.\r\n", ch);
			return;
		}
		// ����� ��������� ���� ������ ��� �������� ����� � �������� ������
		GetOneParam(buffer, buffer2);
		bool money = 0;
		if (CompareParam(buffer2, "������") || CompareParam(buffer2, "money"))
			money = 1;

		for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it)
		{
			it->second->money = 0;
			if (money)
				continue;
			it->second->exp = 0;
			it->second->clan_exp = 0;
		}

		if (money)
			send_to_char("���������� ������� ����� ������� �������.\r\n", ch);
		else
			send_to_char("���������� ����� ������� ��������� �������.\r\n", ch);
		return;
	}
	else if (CompareParam(buffer2, "���"))
	{
		all = 1;
		out << "���������� ����� ������� ";
	}
	else if (!buffer2.empty())
	{
		name = 1;
		out << "���������� ����� ������� (����� �� ����� '" << buffer2 << "') ";
	}
	else
		out << "���������� ����� ������� (����������� ������) ";

	// ����� ������ ����������
	out << "(���������� ��: ";
	switch (sortParameter)
	{
	case SORT_STAT_BY_EXP:
		out << "����������� �����";
		break;
	case SORT_STAT_BY_CLANEXP:
		out << "����� �������";
		break;
	case SORT_STAT_BY_MONEY:
		out << "������������ �����";
		break;
	case SORT_STAT_BY_EXPPERCENT:
		out << "����������� ����� �������";
		break;
	case SORT_STAT_BY_LOGON:
		out << "���������� ������ � ����";
		break;
	case SORT_STAT_BY_NAME:
		out << "������ ����� �����";
		break;

		// ����� ���� �� ������
	default:
		out << "���� ��� ������";
	}
	out << "):\r\n";

	out << CCNRM(ch, C_NRM) << "���             ����������� �����  ����� �������  ���������� ���  ����� ��������� ����\r\n";

	// multimap ��� ����� ���� ����������
	std::multimap<long long, std::pair<std::string, ClanMemberPtr> > temp_list;

	for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it)
	{
		if (!all && !name)
		{
			DESCRIPTOR_DATA *d;
			if (!(d = DescByUID(it->first)))
				continue;
		}
		else if (name)
		{
			if (!CompareParam(buffer2, it->second->name))
				continue;
		}
		char timeBuf[17];
		time_t tmp_time = get_lastlogon_by_uniquie(it->first);
		if (tmp_time <= 0) tmp_time = time(0);
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&tmp_time));

		// ���������� ��...
		switch (sortParameter)
		{
		case SORT_STAT_BY_EXP:
			lSortParam = it->second->exp;
			break;
		case SORT_STAT_BY_CLANEXP:
			lSortParam = it->second->clan_exp;
			break;
		case SORT_STAT_BY_MONEY:
			lSortParam = it->second->money;
			break;
		case SORT_STAT_BY_EXPPERCENT:
			lSortParam = it->second->exp_persent;
			break;
		case SORT_STAT_BY_LOGON:
			lSortParam = get_lastlogon_by_uniquie(it->first);
			break;
		case SORT_STAT_BY_NAME:
		{
			pcFirstChar[0] = LOWER(it->second->name[0]);
			pcFirstChar[1] = '\0';
			char const *pTmp = strpbrk(pSortAlph, pcFirstChar);
			if (pTmp) lSortParam = pTmp - pSortAlph; // ������ ������ ����� � �������
			else lSortParam = pcFirstChar[0]; // ��� �� ������� ����� ��� � ��
			break;
		}

		// �� ������ ������
		default:
			lSortParam = it->second->exp;
		}

		temp_list.insert(std::make_pair(lSortParam,
										std::make_pair(std::string(timeBuf), it->second)));
	}

	for (std::multimap<long long, std::pair<std::string, ClanMemberPtr> >::reverse_iterator it = temp_list.rbegin(); it != temp_list.rend(); ++it)
	{
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << std::setw(15) << it->second.second->name << " "
		<< std::setw(18) << it->second.second->exp << " " // ������ it->first
		<< std::setw(14) << it->second.second->clan_exp << " "
		<< std::setw(15) << it->second.second->money << " "
		<< std::setw(5)  << it->second.second->exp_persent << " "
		<< std::setw(14) << it->second.first << "\r\n";
	}
	page_string(ch->desc, out.str(), 1);
}

// ����������� ��� ������� ����� � ����� �������
void Clan::ChestInvoice()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		int cost = (*clan)->ChestTax();
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);
		if (!cost)
			continue; // ��� ���� �� �����
		else
		{
			// �������
			if ((*clan)->bank < cost)
				for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
					if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == *clan)
						send_to_char(d->character, "[���������]: %s'���������, ��� ������� � ����� ������� ������ �����, ��� �� �����!'%s\r\n", CCIRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
		}
	}
}

int Clan::ChestTax()
{
	OBJ_DATA *temp, *chest;
	int cost = 0;
	int count = 0;
	for (chest = world[real_room(this->chest_room)]->contents; chest; chest = chest->next_content)
	{
		if (Clan::is_clan_chest(chest))
		{
			// ���������� ����
			for (temp = chest->contains; temp; temp = temp->next_content)
			{
				cost += GET_OBJ_RENTEQ(temp);
				++count;
			}
			this->chest_weight = GET_OBJ_WEIGHT(chest);
			break;
		}
	}
	this->chest_objcount = count;
	this->chest_discount = MAX(25, 50 - 5 * (this->clan_level));
	return cost * this->chest_discount / 100;
}

/**
* ������ �������� ������ ������ ������������� ������ ����������� �� ������ ����-�������.
* �������� ����� ��-��� ������ ����������.
* \todo ������� �� ������. �� � ������ ��� ������� ����� ����.
* \param obj - ���������
* \param ch - ���������
* \return 0 - ��� �� ����-������, 1 - ��� ��� ��
*/
bool Clan::ChestShow(OBJ_DATA * obj, CHAR_DATA * ch)
{
	if (!ch->desc || !Clan::is_clan_chest(obj))
		return 0;

	if (CLAN(ch) && real_room(CLAN(ch)->chest_room) == IN_ROOM(obj))
	{
		send_to_char("��������� ����� �������:\r\n", ch);
		int cost = CLAN(ch)->ChestTax();
		send_to_char(ch, "����� �����: %d   ����� � ����: %d %s\r\n\r\n", CLAN(ch)->chest_objcount, cost, desc_count(cost, WHAT_MONEYa));
		list_obj_to_char(obj->contains, ch, 1, 3);
	}
	else
		send_to_char("�� �� ��� ��� �������, �����, ��� �� �����.\r\n", ch); // �������� �������� ���������� ���, � �� ���������
	return 1;
}

// +/- ����-�����
int Clan::SetClanExp(CHAR_DATA *ch, int add)
{
	// ��� �� ������
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return add;

	int toclan, tochar;
	if (add > 0)
	{
		toclan = add * CLAN_MEMBER(ch)->exp_persent / 100;
		tochar = add - toclan;
	}
	else
	{
		toclan = add / 5;
		tochar = 0;
	}

	CLAN_MEMBER(ch)->clan_exp += toclan; // �������� �� ���� ������

	this->clan_exp += toclan;
	if (this->clan_exp < 0)
		this->clan_exp = 0;

	if (this->clan_exp > clan_level_exp[this->clan_level+1] && this->clan_level < MAX_CLANLEVEL)
	{
		this->clan_level++;
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character)->GetRent() == this->rent)
				send_to_char(d->character, "%s��� ����� ������ ������, %d ������! �����������!%s\r\n", CCIRED(d->character, C_NRM), this->clan_level, CCNRM(d->character, C_NRM));
	}
	else if (this->clan_exp < clan_level_exp[this->clan_level] && this->clan_level > 0)
	{
		this->clan_level--;
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character)->GetRent() == this->rent)
				send_to_char(d->character, "%s��� ����� ������� �������! ������ �� %d ������! �����������!%s\r\n", CCIRED(d->character, C_NRM), this->clan_level, CCNRM(d->character, C_NRM));
	}
	return tochar;
}

// ���������� ����� ��� ���� ������ � ������� � �������
void Clan::AddTopExp(CHAR_DATA * ch, int add_exp)
{
	// ��� �� ������
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return;

	CLAN_MEMBER(ch)->exp += add_exp;
	if (CLAN_MEMBER(ch)->exp < 0)
		CLAN_MEMBER(ch)->exp = 0;

	// � ������ ��� ����� � ���
	if (this->exp_info)
	{
		this->exp += add_exp;
		if (this->exp < 0)
			this->exp = 0;
	}
	else
		this->exp_buf += add_exp; // ��� �������� �� ����
}

// ������������� �������� ����� ��� ��� � ��������, ���� ���� ����� ������� ������ � ���-�����
void Clan::SyncTopExp()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (!(*clan)->exp_info)
		{
			(*clan)->exp += (*clan)->exp_buf;
			if ((*clan)->exp < 0)
				(*clan)->exp = 0;
			(*clan)->exp_buf = 0;
		}
}

// ��������� ������ ���������� �� ���������� � ���������
void SetChestMode(CHAR_DATA *ch, std::string &buffer)
{
	if (IS_NPC(ch))
		return;
	if (!CLAN(ch))
	{
		send_to_char("��� ������ ������������ ��������.\r\n", ch);
		return;
	}

	boost::trim_if(buffer, boost::is_any_of(" \'"));
	if (CompareParam(buffer, "���"))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("�������.\r\n", ch);
	}
	else if (CompareParam(buffer, "����������"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("�������.\r\n", ch);
	}
	else if (CompareParam(buffer, "���������"))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		SET_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("�������.\r\n", ch);
	}
	else if (CompareParam(buffer, "������"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		SET_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("�������.\r\n", ch);
	}
	else
	{
		send_to_char("�������� ����� ���������� �� ���������� � ��������� �������.\r\n"
					 "������ �������: ����� ��������� <���|����������|���������|������>\r\n"
					 " ��� - ���������� ������ ���������\r\n"
					 " ���������� - �������� ������ ��������� � ���������� �����\r\n"
					 " ��������� - �������� ������ ��������� � ������/���������� �����\r\n"
					 " ������ - �������� ��� ���� ���������\r\n", ch);
		return;
	}
}

// ��� �� �������� � ������, � ������ ������ �����
std::string GetChestMode(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_DECAY_MODE))
	{
		if (PRF_FLAGGED(ch, PRF_TAKE_MODE))
			return "������";
		else
			return "����������";
	}
	else if (PRF_FLAGGED(ch, PRF_TAKE_MODE))
		return "���������";
	else
		return "����";
}

ACMD(do_clanstuff)
{
	OBJ_DATA *obj;
	int vnum, rnum;
	int cnt = 0, gold_total = 0;

	if (!CLAN(ch))
	{
		send_to_char("������� �������� � �����-������ ����.\r\n", ch);
		return;
	}

	if (GET_ROOM_VNUM(IN_ROOM(ch)) != CLAN(ch)->GetRent())
	{
		send_to_char("�������� �������� ���������� �� ������ ������ � ������ ������ �����.\r\n", ch);
		return;
	}

	std::string title = CLAN(ch)->GetClanTitle();

	half_chop(argument, arg, buf);

	ClanStuffList::iterator it = CLAN(ch)->clanstuff.begin();
	for (;it != CLAN(ch)->clanstuff.end();it++)
	{
		vnum = CLAN_STUFF_ZONE * 100 + it->num;
		obj = read_object(vnum, VIRTUAL);
		if (!obj)
			continue;
		rnum = GET_OBJ_RNUM(obj);

		if (rnum == NOTHING)
			continue;
		if (*arg && !strstr(obj_proto[rnum]->short_description, arg))
			continue;

		sprintf(buf, "%s %s clan%d!", it->name.c_str(), title.c_str(), CLAN(ch)->GetRent());
		obj->name              = str_dup(buf);
		obj->short_description = str_dup((it->PNames[0] + " " + title).c_str());

		for (int i = 0;i < 6; i++)
			obj->PNames[i] = str_dup((it->PNames[i] + " " + title).c_str());

		obj->description = str_dup((it->desc).c_str());

		if (it->longdesc.length() > 0)
		{
			EXTRA_DESCR_DATA * new_descr;
			CREATE(new_descr, EXTRA_DESCR_DATA, 1);
			new_descr->keyword = str_dup(obj->short_description);
			new_descr->description = str_dup(it->longdesc.c_str());
			new_descr->next = NULL;
			obj->ex_description = new_descr;
		}

		if (!cnt)
		{
			act("$n ������$g ������ ������� �� ����������� �����������\r\n", FALSE, ch, 0, 0, TO_ROOM);
			act("�� ������� ������ ������� �� ����������� �����������\r\n", FALSE, ch, 0, 0, TO_CHAR);
		}

		int gold = GET_OBJ_COST(obj);

		if (get_gold(ch) >= gold)
		{
			add_gold(ch, -gold);
			gold_total += gold;
		}
		else
		{
			send_to_char(ch, "��������� �������!\r\n");
			break;
		}

		obj_to_char(obj, ch);
		cnt++;

		sprintf(buf, "$n ����$g %s �� �������", obj->PNames[0]);
		sprintf(buf2, "�� ����� %s �� �������", obj->PNames[0]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
		act(buf2, FALSE, ch, 0, 0, TO_CHAR);
	}

	if (cnt)
	{
		sprintf(buf2, "\r\n���������� �������� ��� � %d %s.", gold_total, desc_count(gold_total, WHAT_MONEYu));
		act("\r\n$n ������$g ������ �������", FALSE, ch, 0, 0, TO_ROOM);
		act(buf2, FALSE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		act("\r\n$n �����$u � ������� �� ����������� �����������, �� ������ �� ���$y", FALSE, ch, 0, 0, TO_ROOM);
		act("\r\n�� �������� � ������� �� ����������� �����������, �� �� ����� ������ �����������", FALSE, ch, 0, 0, TO_CHAR);
	}
	set_wait(ch, 1, FALSE);
}

// ������ ������� ������?
int Clan::GetRent()
{
	return this->rent;
}

int Clan::GetOutRent()
{
	return this->out_rent;
}

/**
* �������� ���� �� �����, ���� ������� �� ����� ���� ����, � ���� �� ���� ������
*/
void Clan::remove_from_clan(long unique)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			(*clan)->members.erase(it);
			return;
		}
	}
}

int Clan::GetMemberExpPersent(CHAR_DATA *ch)
{
	return CLAN(ch) ? CLAN_MEMBER(ch)->exp_persent : 0;
}

/**
*
*/
void Clan::init_chest_rnum()
{
	CLAN_CHEST_RNUM = real_object(CLAN_CHEST_VNUM);
}

bool Clan::is_clan_chest(OBJ_DATA *obj)
{
	if (CLAN_CHEST_RNUM < 0 || obj->item_number != CLAN_CHEST_RNUM)
		return false;
	return true;
}

/**
* ���������� ����������� � �����/������ ���� �����.
* \param enter 1 - ���� ����, 0 - �����.
*/
void Clan::clan_invoice(CHAR_DATA *ch, bool enter)
{
	if (IS_NPC(ch) || !CLAN(ch))
		return;

	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character && d->character != ch
				&& STATE(d) == CON_PLAYING
				&& CLAN(d->character) == CLAN(ch)
				&& PRF_FLAGGED(d->character, PRF_WORKMATE_MODE))
		{
			if (enter)
			{
				send_to_char(d->character, "%s��������%s %s ���%s � ���.%s\r\n",
							 CCINRM(d->character, C_NRM), IS_MALE(ch) ? "�" : "��", GET_NAME(ch),
							 GET_CH_SUF_5(ch), CCNRM(d->character, C_NRM));
			}
			else
			{
				send_to_char(d->character, "%s��������%s %s �������%s ���.%s\r\n",
							 CCINRM(d->character, C_NRM), IS_MALE(ch) ? "�" : "��", GET_NAME(ch),
							 GET_CH_SUF_1(ch), CCNRM(d->character, C_NRM));
			}
		}
	}
}

int Clan::print_spell_locate_object(CHAR_DATA *ch, int count, std::string name)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		OBJ_DATA *temp, *chest;
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp = chest->contains; temp; temp = temp->next_content)
				{
					if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
						continue;
					if (!isname(name.c_str(), temp->name))
						continue;

					snprintf(buf, MAX_STRING_LENGTH, "%s �����%s�� � ��������� ������� '%s'.\r\n",
							temp->short_description, GET_OBJ_POLY_1(ch, temp), (*clan)->GetAbbrev());
					CAP(buf);
					send_to_char(buf, ch);

					if (--count <= 0)
						return count;
				}
				break;
			}
		}
	}
	return count;
}

int Clan::GetClanWars(CHAR_DATA *ch)
{
	int result=0, p1=0;
	if (IS_NPC(ch) || !CLAN(ch))
		return 0;

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim)
	{
		if (*clanVictim == CLAN(ch))
			continue;
		p1 = CLAN(ch)->CheckPolitics((*clanVictim)->rent);
		if (p1 == POLITICS_WAR) result++;
	}

	return result;
}

void Clan::add_remember(std::string text, int flag)
{
	std::string buffer = Remember::time_format();
	buffer += text;

	switch (flag)
	{
	case Remember::CLAN:
		remember_.push_back(buffer);
		if (remember_.size() > Remember::MAX_REMEMBER_NUM)
		{
			remember_.erase(remember_.begin());
		}
		break;
	case Remember::ALLY:
		remember_ally_.push_back(buffer);
		if (remember_ally_.size() > Remember::MAX_REMEMBER_NUM)
		{
			remember_ally_.erase(remember_ally_.begin());
		}
		break;
	default:
		log("SYSERROR: �� �� ������ ���� ���� �������, flag: %d, func: %s",
				flag, __func__);
		break;
	}
}

std::string Clan::get_remember(unsigned int num, int flag) const
{
	std::string buffer;

	switch (flag)
	{
	case Remember::CLAN:
	{
		Remember::RememberListType::const_iterator it = remember_.begin();
		if (remember_.size() > num)
		{
			std::advance(it, remember_.size() - num);
		}
		for (; it != remember_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case Remember::ALLY:
	{
		Remember::RememberListType::const_iterator it = remember_ally_.begin();
		if (remember_ally_.size() > num)
		{
			std::advance(it, remember_ally_.size() - num);
		}
		for (; it != remember_ally_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	default:
		log("SYSERROR: �� �� ������ ���� ���� �������, flag: %d, func: %s",
				flag, __func__);
		break;
	}
	if (buffer.empty())
	{
		buffer = "��� ������ ���������.\r\n";
	}
	return buffer;
}
