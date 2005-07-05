	/* ****************************************************************************
* File: house.cpp                                              Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include <fstream>
#include <sstream>
#include <boost/format.hpp>
#include "sys/stat.h"
#include <cmath>
#include <iomanip>

#include "house.h"
#include "comm.h"
#include "handler.h"
#include "pk.h"
#include "screen.h"
#include "boards.h"
#include "skills.h"

extern void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int file_to_string_alloc(const char *name, char **buf);
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern const char *apply_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *weapon_class[];
extern const char *weapon_affects[];
extern const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
extern int ext_search_block(char *arg, const char **list, int exact);


inline bool Clan::SortRank::operator() (const CHAR_DATA * ch1, const CHAR_DATA * ch2) {
    return GET_CLAN_RANKNUM(ch1) < GET_CLAN_RANKNUM(ch2);
}

ClanListType Clan::ClanList;

// ������������ ������� �� ������
const char *Clan::GetAbbrev(CHAR_DATA * ch)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (GET_CLAN_RENT(ch) == (*clan)->rent)
			return (*clan)->abbrev.c_str();
	return 0;
}

// ����� to_room � ����� ����-������, ���������� ����� ���� �������
room_rnum Clan::CloseRent(room_rnum to_room)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[to_room]->zone == world[real_room((*clan)->rent)]->zone)
			return (to_room = real_room((*clan)->out_rent));
	return to_room;
}

// ��������� ��������� �� ��� � ���� ������ �����
bool Clan::InEnemyZone(CHAR_DATA * ch)
{
	int zone = world[IN_ROOM(ch)]->zone;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (zone == world[real_room((*clan)->rent)]->zone) {
			if (GET_CLAN_RENT(ch) == (*clan)->rent
			    || (*clan)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
				return 0;
			else
				return 1;
		}
	return 0;
}

Clan::Clan()
	: rent(0), bank(0), exp(0), guard(0), builtOn(time(0)), bankBuffer(0), entranceMode(0), out_rent(0), storehouse(0)
{

}

Clan::~Clan()
{

}

// ����/������ ������� � ������ ������
void Clan::ClanLoad()
{
	// �� ������ �������
	Clan::ClanList.clear();

	// ���� �� ������� ������
	std::ifstream file(LIB_HOUSE "index");
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer;
	std::list<std::string> clanIndex;
	while (file >> buffer)
		clanIndex.push_back(buffer);
	file.close();

	// ���������� ������ �����
	for (std::list < std::string >::const_iterator it = clanIndex.begin(); it != clanIndex.end(); ++it) {
		ClanPtr tempClan(new Clan);

		std::ifstream file((LIB_HOUSE + *it).c_str());
		if (!file.is_open()) {
			log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + *it).c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		while (file >> buffer) {
			if (buffer == "Abbrev:")
				file >> tempClan->abbrev;
			else if (buffer == "Name:") {
				std::getline(file, buffer);
				SkipSpaces(buffer);
				tempClan->name = buffer;
			} else if (buffer == "Title:") {
				std::getline(file, buffer);
				SkipSpaces(buffer);
				tempClan->title = buffer;
			} else if (buffer == "Rent:") {
				int rent = 0;
				file >> rent;
				// ���� ����� � �� ����
				if (!real_room(rent)) {
					log("Room %d is no longer exist (%s).", rent, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->rent = rent;
			} else if (buffer == "OutRent:") {
				int out_rent = 0;
				file >> out_rent;
				// ���� ����� � �� ����
				if (!real_room(out_rent)) {
					log("Room %d is no longer exist (%s).", out_rent, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->out_rent = out_rent;
			} else if (buffer == "Guard:") {
				int guard = 0;

				file >> guard;
				// ��� � ���������
				if (real_mobile(guard) < 0) {
					log("Guard %d is no longer exist (%s).", guard, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->guard = guard;
			} else if (buffer == "BuiltOn:")
				file >> tempClan->builtOn;
			else if (buffer == "Ranks:") {
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				unsigned long priv = 0;

				while (stream >> buffer >> priv) {
					tempClan->ranks.push_back(buffer);
					tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (priv));
				}
				// ��� �������� ������� ��������� ��� ����������
				for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
					tempClan->privileges[0].set(i);
			} else if (buffer == "Politics:") {
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				room_vnum room = 0;
				int state = 0;

				while (stream >> room >> state)
					tempClan->politics[room] = state;
			} else if (buffer == "EntranceMode:")
				file >> tempClan->entranceMode;
			else if (buffer == "Storehouse:")
				file >> tempClan->storehouse;
			else if (buffer == "Exp:")
				file >> tempClan->exp;
			else if (buffer == "Bank:") {
				file >> tempClan->bank;
				if (tempClan->bank == 0) {
					log("Clan has 0 in bank, remove from list (%s).", (LIB_HOUSE + *it).c_str());
					break;
				}
			}
			else if (buffer == "Owner:") {
				long unique = 0;
				long long money = 0;
				unsigned long long exp = 0;
				int tax = 0;

				file >> unique >> money >> exp >> tax;
				std::string name = GetNameByUnique(unique);

				// ������� ���� ��� ����� �� ����
				if (!name.empty()) {
					ClanMemberPtr tempMember(new ClanMember);
					name[0] = UPPER(name[0]);
					tempMember->name = name;
					tempMember->rankNum = 0;
					tempMember->money = money;
					tempMember->exp = exp;
					tempMember->tax = tax;
					tempClan->members[unique] = tempMember;
					tempClan->owner = name;
				} else {
					log("Owner %ld is no longer exist (%s).", unique, (LIB_HOUSE + *it).c_str());
					break;
				}
			} else if (buffer == "Members:") {
				// ���������, ��������� ��� ��������, ����� ��������� �� ���� ��� (����� ��������)
				long unique = 0;
				unsigned rank = 0;
				long long money = 0;
				unsigned long long exp = 0;
				int tax = 0;

				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				while (stream >> unique >> rank >> money >> exp >> tax) {
					// �� ������, ���� ������ ����� ������
					if (!tempClan->ranks.empty() && rank > tempClan->ranks.size() - 1)
						rank = tempClan->ranks.size() - 1;
					std::string name = GetNameByUnique(unique);

					// ��������� ��������� ������ ������������
					if (!name.empty()) {
						ClanMemberPtr tempMember(new ClanMember);
						name[0] = UPPER(name[0]);
    					tempMember->name = name;
						tempMember->rankNum = rank;
						tempMember->money = money;
						tempMember->exp = exp;
						tempMember->tax = tax;
						tempClan->members[unique] = tempMember;
					}
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

		// ���������� ���/���
		std::ifstream pkFile((LIB_HOUSE + *it + ".pkl").c_str());
		if (pkFile.is_open()) {
			while (pkFile) {
				int victim = 0, author = 0;
				time_t tempTime = time(0);
				bool flag = 0;

				pkFile >> victim >> author >> tempTime >> flag;
				std::getline(pkFile, buffer, '~');
				SkipSpaces(buffer);
				const char *authorName = GetNameByUnique(author);
				const char *victimName = GetNameByUnique(victim, 1);

				// ���� ������ ��� ��� - ������� ��� ��� ��������
				if (!authorName)
					author = 0;
				// ���� ������ ��� ���, ��� ������ �������� � ���� - �� ������
				if (victimName) {
					ClanPkPtr tempRecord(new ClanPk);

					tempRecord->author = author;
					tempRecord->authorName = authorName ? authorName : "���-��";
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
		Clan::ClanList.push_back(tempClan);
	}
	Clan::ChestLoad();
	Clan::ChestUpdate();
	Clan::ChestSave();
	Clan::ClanSave();
	Board::ClanInit();
	// �� ������ ������� ������ ��� ����������� ��������� ������� ������
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character)
			Clan::SetClanData(d->character);
	}
}

// ����� ���� ���������� � ������
void Clan::ClanInfo(CHAR_DATA * ch)
{
	std::ostringstream buffer;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		buffer << "Abbrev: " << (*clan)->abbrev << "\r\n"
		    << "Name: " << (*clan)->name << "\r\n"
		    << "Title: " << (*clan)->title << "\r\n"
		    << "Owner: " << (*clan)->owner << "\r\n"
		    << "Rent: " << (*clan)->rent << "\r\n"
			<< "OutRent: " << (*clan)->out_rent << "\r\n"
			<< "Guard: " << (*clan)->guard << "\r\n";

		char timeBuf[17];
		strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&(*clan)->builtOn));

		buffer << "BuiltOn: " << timeBuf << "\r\n"
		    << "EntranceMode: " << (*clan)->entranceMode << "\r\n"
			<< "Storehouse: " << (*clan)->storehouse << "\r\n"
		    << "Bank: " << (*clan)->bank << "\r\n" << "Ranks: ";
		for (std::vector < std::string >::const_iterator it =
		     (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it)
			buffer << (*it) << " ";
		buffer << "\r\n" << "Members:" << "\r\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it) {
			buffer << "Uid: " << (*it).first << " "
			    << "Name: " << (*it).second->name << " "
			    << "RankNum: " << (*it).second->rankNum << " "
			    << "Money: " << (*it).second->money << " "
			    << "Exp: " << (*it).second->exp << " "
			    << "Tax: " << (*it).second->tax << " "
			    << "RankName: " << (*clan)->ranks[(*it).second->rankNum] << "\r\n";
		}
		buffer << "Politics:\r\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			buffer << (*it).first << ": " << (*it).second << "\r\n";
	}
	char *text = str_dup(buffer.str());
	page_string(ch->desc, text, TRUE);
	free(text);
//	send_to_char(buffer.str(), ch);
}

// ���������� ������ � �����
void Clan::ClanSave()
{
	std::ofstream index(LIB_HOUSE "index");
	if (!index.is_open()) {
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		// ������ ����� ��� ����� ������ ��� ������������ (���������� � ������ �������)
		std::string buffer = (*clan)->abbrev;
		CreateFileName(buffer);
		index << buffer << "\n";

		std::ofstream file((LIB_HOUSE + buffer).c_str());
		if (!file.is_open()) {
			log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + buffer).c_str(),
			    __FILE__, __func__, __LINE__);
			return;
		}

		file << "Abbrev: " << (*clan)->abbrev << "\n"
		    << "Name: " << (*clan)->name << "\n"
		    << "Title: " << (*clan)->title << "\n"
		    << "Rent: " << (*clan)->rent << "\n"
		    << "OutRent: " << (*clan)->out_rent << "\n"
		    << "Guard: " << (*clan)->guard << "\n"
		    << "BuiltOn: " << (*clan)->builtOn << "\n"
		    << "EntranceMode: " << (*clan)->entranceMode << "\n"
			<< "Storehouse: " << (*clan)->storehouse << "\n"
		    << "Exp: " << (*clan)->exp << "\n"
		    << "Bank: " << (*clan)->bank << "\n" << "Politics:\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			file << " " << (*it).first << " " << (*it).second << "\n";
		file << "~\n" << "Ranks:\n";
		int i = 0;

		for (std::vector < std::string >::const_iterator it =
		     (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it, ++i)
			file << " " << (*it) << " " << (*clan)->privileges[i].to_ulong() << "\n";
		file << "~\n" << "Owner: ";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if ((*it).second->rankNum == 0) {
				file << (*it).first << " " << (*it).second->money << " " << (*it).second->exp << " " << (*it).second->tax << "\n";
				break;
			}
		file << "Members:\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if ((*it).second->rankNum != 0)
				file << " " << (*it).first << " " << (*it).second->rankNum << " " << (*it).second->money << " " << (*it).second->exp << " " << (*it).second->tax << "\n";
		file << "~\n";
		file.close();

		std::ofstream pkFile((LIB_HOUSE + buffer + ".pkl").c_str());
		if (!pkFile.is_open()) {
			log("Error open file: %s! (%s %s %d)",
			    (LIB_HOUSE + buffer + ".pkl").c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it)
			pkFile << (*it).first << " " << (*it).second->author << " " << (*it).
			    second->time << " " << "0\n" << (*it).second->text << "\n~\n";
		for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it)
			pkFile << (*it).first << " " << (*it).second->author << " " << (*it).
			    second->time << " " << "1\n" << (*it).second->text << "\n~\n";
		pkFile.close();
	}
	index.close();
}

// ����������� ��������� ������ ���� ���� �� ��������
// ������ � ����� ��� �������� �� ���������� �������� GET_CLAN_RENT()
void Clan::SetClanData(CHAR_DATA * ch)
{
	GET_CLAN_RENT(ch) = 0;
	ClanListType::const_iterator clan;
	ClanMemberList::const_iterator it;
	// ���� ����-�� ��������, �� ������� ����� ��������� �� ���� � ������ ������
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		it = (*clan)->members.find(GET_UNIQUE(ch));
		if (it != (*clan)->members.end()) {
			GET_CLAN_RENT(ch) = (*clan)->rent;
			break;
		}
	}
	// ������ �� ��������
	if (!GET_CLAN_RENT(ch)) {
		if (GET_CLAN_STATUS(ch))
			free(GET_CLAN_STATUS(ch));
		GET_CLAN_STATUS(ch) = 0;
		GET_CLAN_RANKNUM(ch) = 0;
		return;
	}
	// ����-�� ���� ��������
	std::string buffer = (*clan)->ranks[(*it).second->rankNum] + " " + (*clan)->title;
	GET_CLAN_STATUS(ch) = str_dup(buffer.c_str());
	GET_CLAN_RANKNUM(ch) = (*it).second->rankNum;
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
	if (clan == Clan::ClanList.end() || IS_GRGOD(ch)
	    || !ROOM_FLAGGED(room, ROOM_HOUSE) || (*clan)->entranceMode)
		return 1;
	if (!GET_CLAN_RENT(ch))
		return 0;

	bool isMember = 0;

	if (GET_CLAN_RENT(ch) == (*clan)->rent || (*clan)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
		isMember = 1;

	switch (mode) {
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
		if (RENTABLE(ch)) {
			if (mode == HCE_ATRIUM)
				send_to_char
				    ("������ ������� ����� � ���� ������, � ����� ����� ������� ������.\r\n", ch);
			return 0;
		}
		return 1;
	}
	return 0;
}

// ���������� �������� �� ����, ���� �������� ����-���� ��������
ClanListType::const_iterator Clan::GetClan(CHAR_DATA * ch)
{
	ClanListType::const_iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (GET_CLAN_RENT(ch) == (*clan)->rent)
			return clan;
	return clan;
}

// � ����������� �� ����������� ������� ����� ����� ������ ������ ������
const char *HOUSE_FORMAT[] = {
	"  ����� ����������\r\n",
	"  ����� ������� ��� ������\r\n",
	"  ����� ������� ���\r\n",
	"  ����� ����������\r\n",
	"  ������� (�����)\r\n",
	"  �������� <��� �������> <�����������|�����|������>\r\n",
	"  ��������� <������|��������>\r\n",
	"  ������|������ <��������|�������>\r\n",
	"  ������ ���� � ���������\r\n",
	"  ����� ���� �� ���������\r\n",
	"  ����� (�������)\r\n",
	"  ����� �������� (����� �� �������)\r\n"
};

// ��������� �������� ���������� (������� house)
ACMD(DoHouse)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	// ���� ����� ����������, �� ���� ������� � ������������
	if (!GET_CLAN_RENT(ch)) {
		if (CompareParam(buffer2, "��������") && ch->desc->clan_invite)
			ch->desc->clan_invite->clan->ClanAddMember(ch, ch->desc->clan_invite->rank, 0, 0, 0);
		else
			send_to_char("���� ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		// ����� � ����� - ���� �� ���� ��������, �� �� ��� ���-�� ����� ������ ���������� �����
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}

	if (CompareParam(buffer2, "����������")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_INFO])
		(*clan)->HouseInfo(ch);
	else if (CompareParam(buffer2, "�������")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_ADD])
		(*clan)->HouseAdd(ch, buffer);
	else if (CompareParam(buffer2, "�������")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_REMOVE])
		(*clan)->HouseRemove(ch, buffer);
	else if (CompareParam(buffer2, "����������")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PRIVILEGES]) {
		boost::shared_ptr<struct ClanOLC> temp_clan_olc(new ClanOLC);
		temp_clan_olc->clan = (*clan);
		temp_clan_olc->privileges = (*clan)->privileges;
		ch->desc->clan_olc = temp_clan_olc;
		STATE(ch->desc) = CON_CLANEDIT;
		(*clan)->MainMenu(ch->desc);
	} else if (CompareParam(buffer2, "�������") && !GET_CLAN_RANKNUM(ch))
		(*clan)->HouseOwner(ch, buffer);
	else if (CompareParam(buffer2, "����������"))
		(*clan)->HouseStat(ch, buffer);
	else if (CompareParam(buffer2, "��������", 1)
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_LEAVE])
		(*clan)->HouseLeave(ch);
	else {
		// ��������� ������ ��������� ������ �� ������ ���������
		buffer = "��������� ��� ���������� �������:\r\n";
		for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i)
			if ((*clan)->privileges[GET_CLAN_RANKNUM(ch)][i])
				buffer += HOUSE_FORMAT[i];
		// ������� �� ���� ����� ��� ������� � ������� �������
		if (!GET_CLAN_RANKNUM(ch))
			buffer += "  ����� �������\r\n";
		if ((*clan)->storehouse)
			buffer += "  ��������� <�������>\r\n";
		buffer += "  ����� ���������� (���|��������)\r\n";
		send_to_char(buffer, ch);
	}
}

// house ����������
void Clan::HouseInfo(CHAR_DATA * ch)
{
	// �����, ���������� ������ ������������� �� ���� ���������, ������� ���������� �� ������
	std::vector<ClanMemberPtr> tempList;
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
		tempList.push_back((*it).second);
	std::sort(tempList.begin(), tempList.end(), Clan::SortRank());
	std::ostringstream buffer;
	buffer << "� ����� ���������: ";
	for (std::vector<ClanMemberPtr>::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
		buffer << (*it)->name << " ";
	buffer << "\r\n����������:\r\n";
	int num = 0;

	for (std::vector<std::string>::const_iterator it = ranks.begin(); it != ranks.end(); ++it, ++num) {
		buffer << "  " << (*it) << ":";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (this->privileges[num][i])
				switch (i) {
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
				case MAY_CLAN_LEAVE:
					buffer << " ��������";
					break;
				}
		buffer << "\r\n";
	}
	// ���� � ����� � ���������
	OBJ_DATA *temp, *chest;
	int chest_num = real_object(CLAN_CHEST), count = 0, cost = 0;
	if (chest_num >= 0) {
		for (chest = world[real_room(this->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				// ���������� ����
				for (temp = chest->contains; temp; temp = temp->next_content) {
					cost += GET_OBJ_RENTEQ(temp);
					++count;
				}
				break;
			}
		}
	}
	cost = cost * CLAN_STOREHOUSE_COEFF / 100;
	buffer << "� ��������� ����� ������� " << count << " ��������� (" << cost << " " << desc_count(cost, WHAT_MONEYa) << " � ����).\r\n"
		<< "��������� �����: " << this->bank << " " << desc_count(this->bank, WHAT_MONEYa) << ".\r\n"
		<< "����� ���������� " << CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX) << " " << desc_count(CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX), WHAT_MONEYa) << " � ����.\r\n";
	cost += CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX);
	if (!cost)
		buffer << "����� ����� ������ �� ���������� ���������� ����.\r\n";
	else
		buffer << "����� ����� ������ �������� �� " << this->bank / cost << " " << desc_count(this->bank/cost, WHAT_DAY) << ".\r\n";
	send_to_char(buffer.str(), ch);
}

// house �������
// �������� ����� ������ �� ����������� ���� ������
void Clan::HouseAdd(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	if (buffer2.empty()) {
		send_to_char("������� ��� ���������.\r\n", ch);
		return;
	}
	long unique = 0;

	if (!(unique = GetUniqueByName(buffer2))) {
		send_to_char("����������� ��������.\r\n", ch);
		return;
	}
	std::string name = buffer2;
	name[0] = UPPER(name[0]);
	if (unique == GET_UNIQUE(ch)) {
		send_to_char("��� ���� �������, ������ ���� ����� �������������?\r\n", ch);
		return;
	}
	DESCRIPTOR_DATA *d;

	if (!(d = DescByUID(unique))) {
		send_to_char("����� ��������� ��� � ����!\r\n", ch);
		return;
	}
	if (GET_CLAN_RENT(d->character) && GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character)) {
		send_to_char("�� �� ������ ��������� ����� ������ �������.\r\n", ch);
		return;
	}
	if (GET_CLAN_RENT(ch) == GET_CLAN_RENT(d->character)
	    && GET_CLAN_RANKNUM(ch) >= GET_CLAN_RANKNUM(d->character)) {
		send_to_char("�� ������ ������ ������ ������ � ����������� ������ �������.\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);

	// ����� ������ ������� � 0 ������ �� ����� �������� � �� ���� ��� ��������� ��� 10 ������
	int rank = GET_CLAN_RANKNUM(ch);
	if(!rank)
		++rank;

	if (buffer2.empty()) {
		buffer = "������� ������ ���������.\r\n��������� ���������: ";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);
		return;
	}
	int temp_rank = rank; // ������ rank ���� �� ����������� � ������ ��������� ������
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
		if (CompareParam(buffer2, *it)) {
			if (GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch)) {
				// ����� ��� �������� � ���� �������, ������ ������ ������
				ClanAddMember(d->character, temp_rank, this->members[GET_UNIQUE(ch)]->money, this->members[GET_UNIQUE(ch)]->exp, this->members[GET_UNIQUE(ch)]->tax);
				return;
			} else {
				// �� �������� - ������� ��� ����������� � �����, ���� �� ����������
				if(ignores(d->character, ch, IGNORE_CLAN)) {
					send_to_char(ch, "%s ���������� ���� �����������.\r\n", GET_NAME(d->character));
					return;
				}
				boost::shared_ptr<struct ClanInvite> temp_invite (new ClanInvite);
				ClanListType::const_iterator clan = Clan::GetClan(ch);
				if (clan == Clan::ClanList.end()) {
					log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
					return;
				}
				temp_invite->clan = (*clan);
				temp_invite->rank = temp_rank;
				d->clan_invite = temp_invite;
				buffer = CCWHT(d->character, C_NRM);
				buffer += "$N ���������$G � ���� �������, ������ - " + *it + ".";
				buffer += CCNRM(d->character, C_NRM);
				// ��������� ����������
				act(buffer.c_str(), FALSE, ch, 0, d->character, TO_CHAR);
				buffer = CCWHT(d->character, C_NRM);
				buffer += "�� �������� ����������� � ������� '" + this->name + "', ������ - " + *it + ".\r\n"
					+ "����� ������� ����������� �������� '����� ��������'.\r\n";
				buffer += CCNRM(d->character, C_NRM);
				send_to_char(buffer, d->character);
				return;
			}
		}
	buffer = "�������� ������, ��������� ���������:\r\n";
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
		buffer += "'" + *it + "' ";
	buffer += "\r\n";
	send_to_char(buffer, ch);
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
	else if ((*it).second->rankNum <= GET_CLAN_RANKNUM(ch))
		send_to_char("�� ������ ��������� �� ������� ������ ��������� �� ������� ���� ������.\r\n", ch);
	else {
		this->members.erase(it);
		DESCRIPTOR_DATA *d, *k;
		if ((k = DescByUID(unique))) {
			Clan::SetClanData(k->character);
			send_to_char(k->character, "��� ��������� �� ������� '%s'.\r\n", this->name.c_str());
		}
		for (d = descriptor_list; d; d = d->next) {
			if (d->character && GET_CLAN_RENT(d->character) == this->rent)
				send_to_char(d->character, "%s ������%s �� ����� ����� ����������.\r\n", GET_NAME(k->character), GET_CH_SUF_1(k->character));
		}
	}
}

// ��������� ����-������ � ������ ���������, ��� ������, ��� � ����
// �������� ���� �� ����� �������� ������ ��������, � �� � ��������� ���������
// ��� ������ ��������� ����� �������� ������ ������
ACMD(DoClanChannel)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;

	if (IS_GRGOD(ch) && !GET_CLAN_RENT(ch)) {
		// ������� ���������� ��� ������� �����-�� �������
		std::string buffer2;
		GetOneParam(buffer, buffer2);
		SkipSpaces(buffer);
		ClanListType::const_iterator clan = Clan::GetClan(ch);
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam((*clan)->abbrev, buffer2))
				break;
		if (clan == Clan::ClanList.end()) {
			send_to_char("������� � ����� ������������� �� �������.\r\n", ch);
			return;
		}
		if (buffer.empty()) {
			send_to_char("��� �� ������ �� ��������?\r\n", ch);
			return;
		}
		switch (subcmd) {
			DESCRIPTOR_DATA *d;

			// ������� ��� ������� �����-�� �������
		case SCMD_CHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				    && (*clan)->rent == GET_CLAN_RENT(d->character) && ch != d->character)
					send_to_char(d->character, "%s ����� �������: %s'%s'%s\r\n",
						     GET_NAME(ch), CCIRED(d->character, C_NRM),
						     buffer.c_str(), CCNRM(d->character, C_NRM));
			send_to_char(ch, "�� ������� %s: %s'%s'.%s\r\n",
				     (*clan)->abbrev.c_str(), CCIRED(ch, C_NRM), buffer.c_str(), CCNRM(ch, C_NRM));
			break;
			// �� �� � ����� ��������� ���� �������, ���� ��� ������ ����
		case SCMD_ACHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && GET_CLAN_RENT(d->character))
					if ((*clan)->CheckPolitics(GET_CLAN_RENT(d->character)) ==
					    POLITICS_ALLIANCE || GET_CLAN_RENT(d->character) == (*clan)->rent) {
						// �������� �� ������ � ����� ������, ����� ��� �� ������
						if (GET_CLAN_RENT(d->character) != (*clan)->rent) {
							ClanListType::const_iterator vict = Clan::GetClan(d->character);
							if (vict == Clan::ClanList.end()) {
								log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
								return;
							}
							if ((*vict)->CheckPolitics((*clan)->rent) == POLITICS_ALLIANCE)
								send_to_char(d->character,
									     "%s ����� ���������: %s'%s'%s\r\n",
									     GET_NAME(ch), CCIGRN(d->character, C_NRM),
									     buffer.c_str(), CCNRM(d->character,
												   C_NRM));
						}
						// ��������������� ����� �������� ������
						else
							send_to_char(d->character, "%s ����� ���������: %s'%s'%s\r\n",
								     GET_NAME(ch), CCIGRN(d->character, C_NRM),
								     buffer.c_str(), CCNRM(d->character, C_NRM));
					}
			send_to_char(ch, "�� ��������� %s: %s'%s'.%s\r\n",
				     (*clan)->abbrev.c_str(), CCIGRN(ch, C_NRM), buffer.c_str(), CCNRM(ch, C_NRM));
			break;
		}
	} else {
		if (!GET_CLAN_RENT(ch)) {
			send_to_char("�� �� ������������ �� � ����� �������.\r\n", ch);
			return;
		}
		ClanListType::const_iterator clan = Clan::GetClan(ch);
		if (clan == Clan::ClanList.end()) {
			log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__,
			    __LINE__);
			return;
		}
		// ����������� �� ����-����� �� ������ �� ����� ������, ���� ��� ���
		if (!IS_IMMORTAL(ch)
		    && !(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHANNEL]
		    || PLR_FLAGGED(ch, PLR_DUMB)) {
			send_to_char("�� �� ������ ������������ ������� �������.\r\n", ch);
			return;
		}
		SkipSpaces(buffer);
		if (buffer.empty()) {
			send_to_char("��� �� ������ ��������?\r\n", ch);
			return;
		}
		DESCRIPTOR_DATA *d;

		switch (subcmd) {
			// ����� �������
		case SCMD_CHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				    && GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch) && !ignores(d->character, ch, IGNORE_CLAN))
					send_to_char(d->character, "%s �������: %s'%s'.%s\r\n",
						     GET_NAME(ch), CCIRED(d->character, C_NRM),
						     buffer.c_str(), CCNRM(d->character, C_NRM));
			send_to_char(ch, "�� �������: %s'%s'.%s\r\n", CCIRED(ch, C_NRM),
				     buffer.c_str(), CCNRM(ch, C_NRM));
			break;
			// ���������
		case SCMD_ACHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && !PRF_FLAGGED (d->character, PRF_ALLIANCE)
				    && GET_CLAN_RENT(d->character) && !ignores(d->character, ch, IGNORE_ALLIANCE))
					if ((*clan)->CheckPolitics(GET_CLAN_RENT(d->character)) ==
					    POLITICS_ALLIANCE || GET_CLAN_RENT(ch) == GET_CLAN_RENT(d->character)) {
						// �������� �� ������ � ����� ������, ��� �� ������� ���� ����� �� ���
						if (GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character)) {
							ClanListType::const_iterator vict = Clan::GetClan(d->character);
							if (vict == Clan::ClanList.end()) {
								log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
								return;
							}
							if ((*vict)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
								send_to_char(d->character, "%s ���������: %s'%s'.%s\r\n",
									     GET_NAME(ch), CCIGRN(d->character, C_NRM),
									     buffer.c_str(), CCNRM(d->character, C_NRM));
						}
						// ������ ����� �������� ������
						else
							send_to_char(d->character, "%s ���������: %s'%s'.%s\r\n",
								     GET_NAME(ch), CCIGRN(d->character, C_NRM),
								     buffer.c_str(), CCNRM(d->character, C_NRM));
					}
			send_to_char(ch, "�� ���������: %s'%s'.%s\r\n", CCIGRN(ch, C_NRM),
				     buffer.c_str(), CCNRM(ch, C_NRM));
			break;
		}
	}
}

// ������ ������������������ ������ � �� ���������� �������� (�����������)
ACMD(DoClanList)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	SkipSpaces(buffer);
	ClanListType::const_iterator clan;

	if (buffer.empty()) {
		// ���������� ������ �� �����
		std::multimap<unsigned long long, ClanPtr> sort_clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			sort_clan.insert(std::make_pair((*clan)->members.empty() ? (*clan)->exp : (*clan)->exp/(*clan)->members.size(), (*clan)));

		std::ostringstream out;
		out << "� ���� ���������������� ��������� �������:\r\n"
				<< "     #                   ����� ��������                       ������� �����\r\n\r\n";
		int count = 1;
		// ����� ��, ����� � ����, �� �� ���������� � ���� ������������ ��������� ���������
		for (std::multimap<unsigned long long, ClanPtr>::reverse_iterator it = sort_clan.rbegin(); it != sort_clan.rend(); ++it, ++count) {
			out << std::setw(6) << count << "  " << std::setw(6) << (*it).second->abbrev << " " << std::setw(15)
				<< (*it).second->owner << " ";
			out.setf(std::ios_base::left, std::ios_base::adjustfield);
			out << std::setw(30) << (*it).second->name << " " << (*it).second->exp << "\r\n";
			out.setf(std::ios_base::right, std::ios_base::adjustfield);
		}
		send_to_char(out.str(), ch);
		return;
	}

	bool all = 1;
	std::vector<CHAR_DATA *> tempList;

	if (!CompareParam(buffer, "���")) {
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam(buffer, (*clan)->abbrev))
				break;
		if (clan == Clan::ClanList.end()) {
			send_to_char("����� ������� �� ����������������\r\n", ch);
			return;
		}
		all = 0;
	}
	// �������� ������ ������ ������� ��� ���� ������ (�� ����� all)
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character)
		    && CAN_SEE_CHAR(ch, d->character) && !IS_IMMORTAL(d->character))
			if (all || GET_CLAN_RENT(d->character) == (*clan)->rent)
				tempList.push_back(d->character);
	// �� ���� ���������� �� ������
	std::sort(tempList.begin(), tempList.end(), Clan::SortRank());

	std::ostringstream buffer2;
	buffer2 << "� ���� ���������������� ��������� �������:\r\n" << "     #                  ����� ��������\r\n\r\n";
	boost::format clanFormat(" %5d  %6s %15s %s\r\n");
	boost::format memberFormat(" %-10s%s%s%s %s%s%s\r\n");
	// ���� ������ ���������� ������� - ������� ��
	if (!all) {
		buffer2 << clanFormat % 0 % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
		for (std::vector < CHAR_DATA * >::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
			buffer2 << memberFormat % (*clan)->ranks[GET_CLAN_RANKNUM(*it)] %
			    CCPK(*it, C_NRM, *it) % noclan_title(*it) % CCNRM(*it, C_NRM) % CCIRED(*it, C_NRM)
			    % (PLR_FLAGGED(*it, PLR_KILLER) ? "(�������)" : "") % CCNRM(*it, C_NRM);
	}
	// ������ ������� ��� ������� � ���� ������ (��� ��������� '���' � ������ ����� ������ �������)
	else {
		int count = 1;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan, ++count) {
			buffer2 << clanFormat % count % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
			for (std::vector < CHAR_DATA * >::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
				if (GET_CLAN_RENT(*it) == (*clan)->rent)
					buffer2 << memberFormat % (*clan)->ranks[GET_CLAN_RANKNUM(*it)] %
					    CCPK(*it, C_NRM, *it) % noclan_title(*it) % CCNRM(*it, C_NRM) %
					    CCIRED(*it, C_NRM) % (PLR_FLAGGED(*it, PLR_KILLER) ? "(�������)" : "") % CCNRM(*it, C_NRM);
		}
	}
	send_to_char(buffer2.str(), ch);
}

// ���������� ��������� �������� clan �� ��������� � victim
// ��� ���������� ��������������� �����������
int Clan::CheckPolitics(room_vnum victim)
{
	ClanPolitics::const_iterator it = politics.find(victim);
	if (it != politics.end())
		return (*it).second;
	return POLITICS_NEUTRAL;
}

// ���������� ����� ��������(state) �� ��������� � victim
// ����������� �������� ������ �������� ������, ���� ��� ����
void Clan::SetPolitics(room_vnum victim, int state)
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
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("���� ?\r\n", ch);
		return;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer = argument;
	SkipSpaces(buffer);
	if (!buffer.empty()
	    && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_POLITICS]) {
		(*clan)->ManagePolitics(ch, buffer);
		return;
	}
	boost::format strFormat("  %-3s             %s%-11s%s                 %s%-11s%s\r\n");
	int p1 = 0, p2 = 0;

	std::ostringstream buffer2;
	buffer2 << "��������� ����� ������� � ������� ���������:\r\n" <<
	    "��������     ��������� ����� �������     ��������� � ����� �������\r\n";

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim) {
		if (clanVictim == clan)
			continue;
		p1 = (*clan)->CheckPolitics((*clanVictim)->rent);
		p2 = (*clanVictim)->CheckPolitics((*clan)->rent);
		buffer2 << strFormat % (*clanVictim)->abbrev % (p1 == POLITICS_WAR ? CCIRED(ch, C_NRM)
								 : (p1 ==
								    POLITICS_ALLIANCE ?
								    CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		    % politicsnames[p1] % CCNRM(ch, C_NRM) % (p2 == POLITICS_WAR ? CCIRED(ch, C_NRM)
							      : (p2 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM)
								 : CCNRM(ch,
									 C_NRM))) %
		    politicsnames[p2] % CCNRM(ch, C_NRM);
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
	if (vict == Clan::ClanList.end()) {
		send_to_char("��� ����� �������.\r\n", ch);
		return;
	}
	if ((*vict)->rent == GET_CLAN_RENT(ch)) {
		send_to_char("������ �������� �� ��������� � ����� �������? ��� �� ����...\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
		send_to_char("������� ��������: �����������|�����|������.\r\n", ch);
	else if (CompareParam(buffer2, "�����������")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_NEUTRAL) {
			send_to_char(ch,
				     "���� ������� ��� ��������� � ��������� ������������ � �������� %s.\r\n",
				     (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_NEUTRAL);
		send_to_char(ch, "�� ��������� ����������� � �������� %s.\r\n", (*vict)->abbrev.c_str());
	} else if (CompareParam(buffer2, "�����")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_WAR) {
			send_to_char(ch, "���� ������� ��� ����� � �������� %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_WAR);
		// �������� ������ �������
		for (d = descriptor_list; d; d = d->next)
			if (d->character && d->character != ch && STATE(d) == CON_PLAYING
			    && GET_CLAN_RENT(d->character) == (*vict)->rent)
				send_to_char(d->character,
					     "%s������� %s �������� ����� ������� �����!!!%s\r\n",
					     CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
		send_to_char(ch, "�� �������� ����� ������� %s.\r\n", (*vict)->abbrev.c_str());
	} else if (CompareParam(buffer2, "������")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_ALLIANCE) {
			send_to_char(ch, "���� ������� ��� � ������� � �������� %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_ALLIANCE);
		// ��� �����
		for (d = descriptor_list; d; d = d->next)
			if (d->character && d->character != ch && STATE(d) == CON_PLAYING
			    && GET_CLAN_RENT(d->character) == (*vict)->rent)
				send_to_char(d->character,
					     "%s������� %s ��������� � ����� �������� ������!!!%s\r\n",
					     CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
		send_to_char(ch, "�� ��������� ������ � �������� %s.\r\n", (*vict)->abbrev.c_str());
	}
}

const char *HCONTROL_FORMAT =
    "������: hcontrol build <rent vnum> <outrent vnum> <guard vnum> <leader name> <abbreviation> <clan title> <clan name>\r\n"
	"        hcontrol show\r\n"
	"        hcontrol save\r\n"
    "        hcontrol destroy <house vnum>\r\n";

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
	else if (CompareParam(buffer2, "show"))
		Clan::ClanInfo(ch);
	else if (CompareParam(buffer2, "save"))
		Clan::ClanSave();
	else
		send_to_char(HCONTROL_FORMAT, ch);
}

// ��������� ������� (hcontrol build)
void Clan::HcontrolBuild(CHAR_DATA * ch, std::string & buffer)
{
	// ������ ��� ���������, ����� ����� ��������� �� ���� ���� �����
	std::string buffer2;
	// �����
	GetOneParam(buffer, buffer2);
	room_vnum rent = atoi(buffer2.c_str());
	// ����� ��� �����
	GetOneParam(buffer, buffer2);
	room_vnum out_rent = atoi(buffer2.c_str());
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
	SkipSpaces(buffer);
	std::string name = buffer;

	// ��� ��������� ������� ��� ����� ����
	if (name.empty()) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if (!real_room(rent)) {
		send_to_char(ch, "������� %d �� ����������.\r\n", rent);
		return;
	}
	if (!real_room(out_rent)) {
		send_to_char(ch, "������� %d �� ����������.\r\n", out_rent);
		return;
	}
	if (real_mobile(guard) < 0) {
		send_to_char(ch, "���� %d �� ����������.\r\n", guard);
		return;
	}
	long unique = 0;
	if (!(unique = GetUniqueByName(owner))) {
		send_to_char(ch, "��������� %s �� ����������.\r\n", owner.c_str());
		return;
	}
	// � ��� - �� ����� �� �������� ������ ������
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		if ((*clan)->rent == rent) {
			send_to_char(ch, "������� %d ��� ������ ������ ��������.\r\n", rent);
			return;
		}
		if ((*clan)->guard == guard) {
			send_to_char(ch, "�������� %d ��� ����� ������ ��������.\r\n", rent);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end()) {
			send_to_char(ch, "%s ��� �������� � ������� %s.\r\n", owner.c_str(), (*clan)->abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->abbrev, abbrev, 1)) {
			send_to_char(ch, "������������ '%s' ��� ������ ������ ��������.\r\n", abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->name, name, 1)) {
			send_to_char(ch, "��� '%s' ��� ������ ������ ��������.\r\n", name.c_str());
			return;
		}
		if (CompareParam((*clan)->title, title, 1)) {
			send_to_char(ch, "�������� � ����� '%s' ��� ������ ������ ��������.\r\n", title.c_str());
			return;
		}
	}

	// ���������� ����
	ClanPtr tempClan(new Clan);
	tempClan->rent = rent;
	tempClan->out_rent = out_rent;
	tempClan->guard = guard;
	// ����� �������
	owner[0] = UPPER(owner[0]);
	tempClan->owner = owner;
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = owner;
	tempMember->rankNum = 0;
	tempMember->money = 0;
	tempMember->exp = 0;
	tempMember->tax = 0;
	tempClan->members[unique] = tempMember;
	// ��������
	tempClan->abbrev = abbrev;
	tempClan->name = name;
	tempClan->title = title;
	// �����
	const char *ranks[] = { "�������", "������", "��������", "�����", "�����", "�������", "���", "���", "�����", "�����" };
	std::vector<std::string> tempRanks(ranks, ranks + 10);
	tempClan->ranks = tempRanks;
	// ����������
	for (std::vector<std::string>::const_iterator it =
	     tempClan->ranks.begin(); it != tempClan->ranks.end(); ++it)
		tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
	// ������� ��������� ��� ����������
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
		tempClan->privileges[0].set(i);

	Clan::ClanList.push_back(tempClan);
	Clan::ClanSave();
	Board::ClanInit();

	// ���������� ���������� ������� � ����
	DESCRIPTOR_DATA *d;
	if ((d = DescByUID(unique))) {
		Clan::SetClanData(d->character);
		send_to_char(d->character, "�� ����� �������� ������ �����. ����� ����������!\r\n");
	}
	send_to_char(ch, "������� '%s' �������!\r\n", abbrev.c_str());
}

// �������� ������� (hcontrol destroy)
void Clan::HcontrolDestroy(CHAR_DATA * ch, std::string & buffer)
{
	SkipSpaces(buffer);
	room_vnum rent = atoi(buffer.c_str());

	ClanListType::iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if ((*clan)->rent == rent)
			break;
	if (clan == Clan::ClanList.end()) {
		send_to_char(ch, "������� � ������� %d �� ����������.\r\n", rent);
		return;
	}

	// ���� ������� ������ - �������� ��� �����
	DESCRIPTOR_DATA *d;
	ClanMemberList members = (*clan)->members;
	Clan::ClanList.erase(clan);
	Clan::ClanSave();
	Board::ClanInit();
	// ���������� � ������ ���� �������
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it) {
		if ((d = DescByUID((*it).first))) {
			Clan::SetClanData(d->character);
			send_to_char(d->character, "���� ������� ���������. ������ �����!\r\n");
		}
	}
	send_to_char("������� ���������.\r\n", ch);
}

// ���������� (������ �����������, ����������� ������)
ACMD(DoWhoClan)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("���� ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}

	std::ostringstream buffer;
	buffer << boost::format(" ���� �������: %s%s%s.\r\n") % CCIRED(ch, C_NRM) % (*clan)->abbrev % CCNRM(ch, C_NRM);
	buffer << boost::format(" %s������ � ���� ���� ���������:%s\r\n\r\n") % CCWHT(ch, C_NRM) % CCNRM(ch, C_NRM);
	DESCRIPTOR_DATA *d;
	int num = 0;

	for (d = descriptor_list, num = 0; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch)) {
			buffer << "    " << race_or_title(d->character) << "\r\n";
			++num;
		}
	buffer << "\r\n �����: " << num << ".\r\n";
	send_to_char(buffer.str(), ch);
}

const char *CLAN_PKLIST_FORMAT[] = {
	"������: ������|������\r\n"
	"        ������|������ ���\r\n",
	"        ������|������ �������� ��� �������\r\n"
	"        ������|������ ������� ���|���\r\n"
};

// ���/���
ACMD(DoClanPkList)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("���� ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	std::ostringstream info;
	char timeBuf[11];
	DESCRIPTOR_DATA *d;

	boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");
	// ������� ������ ���, ��� ������
	if (buffer2.empty()) {
		ClanPkList::const_iterator it;
		info << CCWHT(ch,
			      C_NRM) << "������������ ������ ����������� � ���� ���������:\r\n\r\n" << CCNRM(ch, C_NRM);
		// ������, ��������, ����� ������ �� ������ ����������, ��� �� ������ ��/�� �����
		for (d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character) != GET_CLAN_RENT(ch)) {
				// ���
				if (!subcmd)
					it = (*clan)->pkList.find(GET_UNIQUE(d->character));
				// ���
				else
					it = (*clan)->frList.find(GET_UNIQUE(d->character));
				if (it != (*clan)->pkList.end() && it != (*clan)->frList.end()) {
					strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y",
						 localtime(&((*it).second->time)));
					info << frmt % timeBuf %
					    ((*it).second->author ? (*it).second->
					     authorName : "���� �� � ���� ���") % (*it).second->victimName %
					    (*it).second->text;
				}
			}
		// ��� � ����� - �� ������� ���� ��� � ����� ����� � ������ �������� ������
		char *text = str_dup(info.str());
		page_string(ch->desc, text, TRUE);
		free(text);

	} else if (CompareParam(buffer2, "���") || CompareParam(buffer2, "all")) {
		// ������� ���� ������
		info << CCWHT(ch, C_NRM) << "������ ������������ ���������:\r\n\r\n" << CCNRM(ch, C_NRM);
		// ���
		if (!subcmd)
			for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it) {
				strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&((*it).second->time)));
				info << frmt % timeBuf % ((*it).second->author ? (*it).second->authorName : "���� �� � ���� ���")
					% (*it).second->victimName % (*it).second->text;
			}
		// ���
		else
			for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it) {
				strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&((*it).second->time)));
				info << frmt % timeBuf % ((*it).second->author ? (*it).second->authorName : "���� �� � ���� ���")
					% (*it).second->victimName % (*it).second->text;
			}

		char *text = str_dup(info.str());
		page_string(ch->desc, text, TRUE);
		free(text);

	} else if ((CompareParam(buffer2, "��������") || CompareParam(buffer2, "add"))
		   && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
		// ��������� ������
		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			send_to_char("���� �������� ��?\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (!unique) {
			send_to_char("������������ ��� �������� �� ������.\r\n", ch);
			return;
		}
		if (unique < 0) {
			send_to_char("�� ���� ���, ����� �������� ���� �� ����....\r\n", ch);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end()) {
			send_to_char("������� �� ����� �������� ������ ������ ������?\r\n", ch);
			return;
		}
		SkipSpaces(buffer);
		if (buffer.empty()) {
			send_to_char("����������� �����������������, �� ��� �� ��� ���.\r\n", ch);
			return;
		}

		// ���������� ����� ����� ������/�����
		ClanPkPtr tempRecord(new ClanPk);
		tempRecord->author = GET_UNIQUE(ch);
		tempRecord->authorName = GET_NAME(ch);
		buffer2[0] = UPPER(buffer2[0]);
		tempRecord->victimName = buffer2;
		tempRecord->time = time(0);
		tempRecord->text = buffer;
		if (!subcmd)
			(*clan)->pkList[unique] = tempRecord;
		else
			(*clan)->frList[unique] = tempRecord;
		DESCRIPTOR_DATA *d;
		if ((d = DescByUID(unique))) {
			if (!subcmd)
				send_to_char(d->character, "%s������� '%s' �������� ��� � ������ ����� ������!%s\r\n", CCIRED(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
			else
				send_to_char(d->character, "%s������� '%s' �������� ��� � ������ ����� ������!%s\r\n", CCGRN(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
		}
		send_to_char("�������, ��������.\r\n", ch);

	} else if ((CompareParam(buffer2, "�������") || CompareParam(buffer2, "remove"))
		   && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
		// �������� ������
		GetOneParam(buffer, buffer2);
		if (CompareParam(buffer2, "���", 1)) {
			// ���
			if (!subcmd)
				(*clan)->pkList.erase((*clan)->pkList.begin(), (*clan)->pkList.end());
			// ���
			else
				(*clan)->frList.erase((*clan)->frList.begin(), (*clan)->frList.end());
			send_to_char("������ ������.\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (unique <= 0) {
			send_to_char("������������ ��� �������� �� ������.\r\n", ch);
			return;
		}
		ClanPkList::iterator it;
		// ���
		if (!subcmd)
		{
			it = (*clan)->pkList.find(unique);
			if (it != (*clan)->pkList.end())
				(*clan)->pkList.erase(it);
		// ���
		} else
		{
			it = (*clan)->frList.find(unique);
			if (it != (*clan)->frList.end())
				(*clan)->frList.erase(it);
		}

		if (it != (*clan)->pkList.end() && it != (*clan)->frList.end()) {
			send_to_char("������ �������.\r\n", ch);
			DESCRIPTOR_DATA *d;
			if ((d = DescByUID(unique))) {
				if (!subcmd)
					send_to_char(d->character, "%s������� '%s' ������� ��� �� ������ ����� ������!%s\r\n", CCGRN(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
				else
					send_to_char(d->character, "%s������� '%s' ������� ��� �� ������ ����� ������!%s\r\n", CCIRED(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
			}
    	} else
			send_to_char("������ �� �������.\r\n", ch);

	} else {
		if ((*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
			buffer = CLAN_PKLIST_FORMAT[0];
			buffer += CLAN_PKLIST_FORMAT[1];
			send_to_char(buffer, ch);
		} else
			send_to_char(CLAN_PKLIST_FORMAT[0], ch);
	}
}

// ������� � ������ (����� ��� ����������)
SPECIAL(Clan::ClanChest)
{
	OBJ_DATA *chest = (OBJ_DATA *) me;

	if (!ch->desc)
		return 0;
	if (CMD_IS("��������") || CMD_IS("���������") || CMD_IS("look") || CMD_IS("examine")) {
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		SkipSpaces(buffer);
		// ��� ������ ������ ��� ������ '�� � ������' � �������� ��������� � ��� �����
		if (buffer2.empty()
		    || (buffer.empty() && !isname(buffer2.c_str(), chest->name))
		    || (!buffer.empty() && !isname(buffer.c_str(), chest->name)))
			return 0;
		// �������� �������� ���������� ���, � �� ���������
		if (real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
			send_to_char("�� �� ��� ��� �������, �����, ��� �� �����.\r\n", ch);
			return 1;
		}
		send_to_char("��������� ����� �������:\r\n", ch);
		list_obj_to_char(chest->contains, ch, 1, 3);
		return 1;
	}
	return 0;
}

// ������ � ������ (��� ������� ����������)
// ���� ������� - ������, �� ��������� ���� � ����-�����, ���������� ������ ������
bool Clan::PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch) || real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHEST_PUT]) {
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}

	if (IS_OBJ_STAT(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ZONEDECAY) || GET_OBJ_TYPE(obj) == ITEM_KEY)
		act("��������� ���� �������� �������� $o3 � $O3.", FALSE, ch, obj, chest, TO_CHAR);
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
		act("� $o5 ���-�� �����.", FALSE, ch, obj, 0, TO_CHAR);

	else if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
		long gold = GET_OBJ_VAL(obj, 0);
		// ����� � �����: � ������ ������������  - ������ ������� �����, ��������� ���������� ����
		// �� ������ ������ ��� ���������� � ������ �������� 2147483647 ���
		// �� ���� ��� ������ � ���� � �������� � ���� �� ����� ��������� ������
		if (((*clan)->bank + gold) < 0) {
			long over = LONG_MAX - (*clan)->bank;
			(*clan)->bank += over;
			(*clan)->members[GET_UNIQUE(ch)]->money += over;
			gold -= over;
			GET_GOLD(ch) += gold;
			obj_from_char(obj);
			extract_obj(obj);
			send_to_char(ch, "�� ������� ������� � ����� ������� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			return 1;
		}
		(*clan)->bank += gold;
		(*clan)->members[GET_UNIQUE(ch)]->money += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "�� ������� � ����� ������� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));

	} else {
		obj_from_char(obj);
		obj_to_obj(obj, chest);
		act("$n �������$g $o3 � $O3.", TRUE, ch, obj, chest, TO_ROOM);
		act("�� �������� $o3 � $O3.", FALSE, ch, obj, chest, TO_CHAR);
	}
	return 1;
}

// ����� �� ����-������� (��� ������� ����������)
bool Clan::TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch) || real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return 0;
	}
	if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHEST_TAKE]) {
		send_to_char("�� ������ ����� ������!\r\n", ch);
		return 0;
	}

	obj_from_obj(obj);
	obj_to_char(obj, ch);
	if (obj->carried_by == ch) {
		act("�� ����� $o3 �� $O1.", FALSE, ch, obj, chest, TO_CHAR);
		act("$n ����$g $o3 �� $O1.", TRUE, ch, obj, chest, TO_ROOM);
	}
	return 1;
}

// ��������� ��� ������� � �����
// �������� write_one_object (��� ������� ��� ���������, ��� ������� ���� ������� ����� � �����
// ���������� � ���� ������ ��� ��������� ���������� �� �������)
void Clan::ChestSave()
{
	OBJ_DATA *chest, *temp;
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		std::string buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		buffer += ".obj";
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num) {
				std::string buffer2;
				for (temp = chest->contains; temp; temp = temp->next_content) {
					char databuf[MAX_STRING_LENGTH];
					char *data = databuf;

					write_one_object(&data, temp, 0);
					buffer2 += databuf;
				}

				std::ofstream file((LIB_HOUSE + buffer).c_str());
				if (!file.is_open()) {
					log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + buffer).c_str(),
					    __FILE__, __func__, __LINE__);
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
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;

	// �� ������ ������� - ������ ����� ���� ��� ��� ���� � ��������
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				for (temp = chest->contains; temp; temp = obj_next) {
					obj_next = temp->next_content;
					obj_from_obj(temp);
					extract_obj(temp);
				}
			}
		}
	}

	int error = 0, fsize = 0;
	FILE *fl;
	char *data, *databuf;
	std::string buffer;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		buffer += ".obj";

		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num)
				break;
		if (!chest) {
			log("<Clan> Where chest for '%s' ?! (%s %s %d)", (*clan)->abbrev.c_str(),
			    __FILE__, __func__, __LINE__);
			return;
		}
		if (!(fl = fopen((LIB_HOUSE + buffer).c_str(), "r+b")))
			continue;
		fseek(fl, 0L, SEEK_END);
		fsize = ftell(fl);
		if (!fsize) {
			fclose(fl);
			log("<Clan> Empty obj file '%s'. (%s %s %d)",
			    (LIB_HOUSE + buffer).c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		CREATE(databuf, char, fsize + 1);

		fseek(fl, 0L, SEEK_SET);
		if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf) {
			fclose(fl);
			log("<Clan> Error reading obj file '%s'. (%s %s %d)",
			    (LIB_HOUSE + buffer).c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		fclose(fl);

		data = databuf;
		*(data + fsize) = '\0';

		for (fsize = 0; *data && *data != '$'; fsize++) {
			if (!(obj = read_one_object_new(&data, &error))) {
				if (error)
					log("<Clan> Objects reading fail for %s error %d.",
					    (LIB_HOUSE + buffer).c_str(), error);
				continue;
			}
			obj_to_obj(obj, chest);
		}
		free(databuf);
	}
}

// ������� ���� � �������� � ����� �� ����� �� �����
void Clan::ChestUpdate()
{
	OBJ_DATA *temp, *chest, *obj_next;
	double cost, i;
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;
	bool find = 0;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		cost = 0;
		// ���� ������
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num) {
				// ���������� � ��� ����
				for (temp = chest->contains; temp; temp = temp->next_content)
					cost += GET_OBJ_RENTEQ(temp);
				find = 1;
				break;
			}
		// ������ � �������� �� ����� (����� ����� �� �����������) ���������� ����-� 1/3
		cost = (((cost / (60  * 24)) * CLAN_STOREHOUSE_COEFF) / 100) * CHEST_UPDATE_PERIOD;
		// ����� ������ �����
		cost += ((double)(CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_COEFF)) / (60 * 24)) * CHEST_UPDATE_PERIOD;
		(*clan)->bankBuffer += cost;
		modf((*clan)->bankBuffer, &i);
		if (i) {
			(*clan)->bank -= (long) i;
			(*clan)->bankBuffer -= i;
		}
		// ��� ������� ����� ��� ������ � ������� ������
		if ((*clan)->bank < 0 && find) {
			(*clan)->bank = 0;
			for (temp = chest->contains; temp; temp = obj_next) {
				obj_next = temp->next_content;
				obj_from_obj(temp);
				extract_obj(temp);
			}
		}
	}
}

// ����� �������... ������� ���� ����� � ���������� '�����' � ������
// ��������/���������� ����� ���, ������� �� ����������, ����� �� ����������� ��������
bool Clan::BankManage(CHAR_DATA * ch, char *arg)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch))
		return 0;

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "������") || CompareParam(buffer2, "balance")) {
		send_to_char(ch, "�� ����� ����� ������� ����� %ld.\r\n", (*clan)->bank, desc_count((*clan)->bank, WHAT_MONEYu));
		return 1;

	} else if (CompareParam(buffer2, "�������") || CompareParam(buffer2, "deposit")) {
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0) {
			send_to_char("������� �� ������ �������?\r\n", ch);
			return 1;
		}
		if (GET_GOLD(ch) < gold) {
			send_to_char("� ����� ����� �� ������ ������ �������!\r\n", ch);
			return 1;
		}

		// �� ������ ������������ �����
		if (((*clan)->bank + gold) < 0) {
			long over = LONG_MAX - (*clan)->bank;
			(*clan)->bank += over;
			(*clan)->members[GET_UNIQUE(ch)]->money += over;
			GET_GOLD(ch) -= over;
			send_to_char(ch, "�� ������� ������� � ����� ������� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		GET_GOLD(ch) -= gold;
		(*clan)->bank += gold;
		(*clan)->members[GET_UNIQUE(ch)]->money += gold;
		send_to_char(ch, "�� ������� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	} else if (CompareParam(buffer2, "��������") || CompareParam(buffer2, "withdraw")) {
		if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_BANK]) {
			send_to_char("� ���������, � ��� ��� ����������� ���������� �������� �������.\r\n", ch);
			return 1;
		}
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0) {
			send_to_char("�������� ���������� �����, ������� �� ������ ��������?\r\n", ch);
			return 1;
		}
		if ((*clan)->bank < gold) {
			send_to_char("� ���������, ���� ������� �� ��� ������.\r\n", ch);
			return 1;
		}

		// �� ������ ������������ ���������
		if ((GET_GOLD(ch) + gold) < 0) {
			long over = LONG_MAX - GET_GOLD(ch);
			GET_GOLD(ch) += over;
			(*clan)->bank -= over;
			(*clan)->members[GET_UNIQUE(ch)]->money -= over;
			send_to_char(ch, "�� ������� ����� ������ %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		(*clan)->bank -= gold;
		(*clan)->members[GET_UNIQUE(ch)]->money -= gold;
		GET_GOLD(ch) += gold;
		send_to_char(ch, "�� ����� %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n ��������$g ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	} else
		send_to_char(ch, "������ �������: ����� �������|��������|������ �����.\r\n");
	return 1;
}

void Clan::Manage(DESCRIPTOR_DATA * d, const char *arg)
{
	unsigned num = atoi(arg);

	switch (d->clan_olc->mode) {
	case CLAN_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			// ���� �������, ��� �� ����� � ��� � ����� ������� ���-�� ������
			if (d->clan_olc->clan->privileges.size() != d->clan_olc->privileges.size()) {
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
			if (!*arg || !num) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			// ��� ������ ������� ��� (���� ���������� � 1 + ���� ���� ����)
			unsigned min_rank = GET_CLAN_RANKNUM(d->character);
			if ((num + min_rank) >= d->clan_olc->clan->ranks.size()) {
				unsigned i = (num + min_rank) - d->clan_olc->clan->ranks.size();
				if (i == 0)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 1)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 2 && !GET_CLAN_RANKNUM(d->character)) {
					if (d->clan_olc->clan->storehouse)
						d->clan_olc->clan->storehouse = 0;
					else
						d->clan_olc->clan->storehouse = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else {
					send_to_char("�������� �����!\r\n", d->character);
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				return;
			}
			if (num < 0 || (num + min_rank) >= d->clan_olc->clan->ranks.size()) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			d->clan_olc->rank = num + min_rank;
			d->clan_olc->clan->PrivilegeMenu(d, num + min_rank);
			return;
		}
		break;
	case CLAN_PRIVILEGE_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			// ����� � ����� ����
			d->clan_olc->rank = 0;
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			// �� ������ ������ ��������� ���������� � ���� � ��������� ������ �� ���
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
		switch (*arg) {
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
		switch (*arg) {
		case 'q':
		case 'Q':
			// ����� � ����� ���� � ���������� ���� ������
			for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i) {
				if (d->clan_olc->all_ranks[i]) {
					unsigned j = GET_CLAN_RANKNUM(d->character) + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j) {
						d->clan_olc->privileges[j][i] = 1;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
		switch (*arg) {
		case 'q':
		case 'Q':
			// ����� � ����� ���� � ���������� ���� ������
			for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i) {
				if (d->clan_olc->all_ranks[i]) {
					unsigned j = GET_CLAN_RANKNUM(d->character) + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j) {
						d->clan_olc->privileges[j][i] = 0;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("�������� �����!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
	int rank = GET_CLAN_RANKNUM(d->character) + 1;
	int num = 0;
	for (std::vector<std::string>::const_iterator it = d->clan_olc->clan->ranks.begin() + rank; it != d->clan_olc->clan->ranks.end(); ++it)
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM) << ") " << (*it) << "\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "�������� ����\r\n"
		<< CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "������ � ����\r\n";
	if (!GET_CLAN_RANKNUM(d->character)) {
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
			<< ") " << "������� �� ��������� �� ���������� ���������\r\n"
			<< "    + �������� ��������� ��� ����� � �������� ������ (1000 ��� � ����) ";
		if (this->storehouse)
			buffer << "(���������)\r\n";
		else
			buffer << "(��������)\r\n";
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
		<< ") �����\r\n" << "��� �����:";

	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_MAIN_MENU;
}

void Clan::PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num)
{
	if (num > d->clan_olc->clan->ranks.size() || num < 0) {
		log("Different size clan->ranks and clan->privileges! (%s %s %d)", __FILE__, __func__, __LINE__);
		if (d->character) {
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
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
		case MAY_CLAN_INFO:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_INFO]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_INFO])
					buffer << "[x] ���������� � ������� (����� ����������)\r\n";
				else
					buffer << "[ ] ���������� � ������� (����� ����������)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_ADD]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_ADD])
					buffer << "[x] �������� � ������� (����� �������)\r\n";
				else
					buffer << "[ ] �������� � ������� (����� �������)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_REMOVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_REMOVE])
					buffer << "[x] �������� �� ������� (����� �������)\r\n";
				else
					buffer << "[ ] �������� �� ������� (����� �������)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PRIVILEGES]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PRIVILEGES])
					buffer << "[x] �������������� ���������� (����� ����������)\r\n";
				else
					buffer << "[ ] �������������� ���������� (����� ����������)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHANNEL]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHANNEL])
					buffer << "[x] ����-����� (�������)\r\n";
				else
					buffer << "[ ] ����-����� (�������)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_POLITICS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_POLITICS])
					buffer << "[x] ��������� �������� ������� (��������)\r\n";
				else
					buffer << "[ ] ��������� �������� ������� (��������)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_NEWS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_NEWS])
					buffer << "[x] ���������� �������� ������� (���������)\r\n";
				else
					buffer << "[ ] ���������� �������� ������� (���������)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PKLIST]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PKLIST])
					buffer << "[x] ���������� � ��-���� (������)\r\n";
				else
					buffer << "[ ] ���������� � ��-���� (������)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_PUT]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_PUT])
					buffer << "[x] ������ ���� � ���������\r\n";
				else
					buffer << "[ ] ������ ���� � ���������\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_TAKE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_TAKE])
					buffer << "[x] ����� ���� �� ���������\r\n";
				else
					buffer << "[ ] ����� ���� �� ���������\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_BANK]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_BANK])
					buffer << "[x] ����� �� ����� �������\r\n";
				else
					buffer << "[ ] ����� �� ����� �������\r\n";
			}
			break;
		case MAY_CLAN_LEAVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_LEAVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_LEAVE])
					buffer << "[x] ��������� ����� �� �������\r\n";
				else
					buffer << "[ ] ��������� ����� �� �������\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
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
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
		case MAY_CLAN_INFO:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_INFO]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_INFO])
					buffer << "[x] ���������� � ������� (����� ����������)\r\n";
				else
					buffer << "[ ] ���������� � ������� (����� ����������)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_ADD]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_ADD])
					buffer << "[x] �������� � ������� (����� �������)\r\n";
				else
					buffer << "[ ] �������� � ������� (����� �������)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_REMOVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_REMOVE])
					buffer << "[x] �������� �� ������� (����� �������)\r\n";
				else
					buffer << "[ ] �������� �� ������� (����� �������)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PRIVILEGES]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PRIVILEGES])
					buffer << "[x] �������������� ���������� (����� ����������)\r\n";
				else
					buffer << "[ ] �������������� ���������� (����� ����������)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHANNEL]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHANNEL])
					buffer << "[x] ����-����� (�������)\r\n";
				else
					buffer << "[ ] ����-����� (�������)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_POLITICS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_POLITICS])
					buffer << "[x] ��������� �������� ������� (��������)\r\n";
				else
					buffer << "[ ] ��������� �������� ������� (��������)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_NEWS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_NEWS])
					buffer << "[x] ���������� �������� ������� (���������)\r\n";
				else
					buffer << "[ ] ���������� �������� ������� (���������)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PKLIST]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PKLIST])
					buffer << "[x] ���������� � ��-���� (������)\r\n";
				else
					buffer << "[ ] ���������� � ��-���� (������)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_PUT]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_PUT])
					buffer << "[x] ������ ���� � ���������\r\n";
				else
					buffer << "[ ] ������ ���� � ���������\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_TAKE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_TAKE])
					buffer << "[x] ����� ���� �� ���������\r\n";
				else
					buffer << "[ ] ����� ���� �� ���������\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_BANK]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_BANK])
					buffer << "[x] ����� �� ����� �������\r\n";
				else
					buffer << "[ ] ����� �� ����� �������\r\n";
			}
			break;
		case MAY_CLAN_LEAVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_LEAVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_LEAVE])
					buffer << "[x] ��������� ����� �� �������\r\n";
				else
					buffer << "[ ] ��������� ����� �� �������\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
		<< ") ���������\r\n" << "��� �����:";
	send_to_char(buffer.str(), d->character);
	if (flag == 0)
		d->clan_olc->mode = CLAN_ADDALL_MENU;
	else
		d->clan_olc->mode = CLAN_DELALL_MENU;
}

// ����� ���� (������ �����) ���� ���� ��� ������
void Clan::ClanAddMember(CHAR_DATA * ch, int rank, long long money, unsigned long long exp, int tax)
{
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = GET_NAME(ch);
	tempMember->rankNum = rank;
	tempMember->money = money;
	tempMember->exp = exp;
	tempMember->tax = tax;
	this->members[GET_UNIQUE(ch)] = tempMember;
	Clan::SetClanData(ch);
	DESCRIPTOR_DATA *d;
	std::string buffer;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
		    && this->rent == GET_CLAN_RENT(d->character) && ch != d->character) {
				send_to_char(d->character, "%s%s ��������%s � ����� �������, ������ - '%s'.%s\r\n",
					CCWHT(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_6(ch), (this->ranks[rank]).c_str(), CCNRM(d->character, C_NRM));
		}
	}
	send_to_char(ch, "%s��� ��������� � ������� '%s', ������ - '%s'.%s\r\n", CCWHT(ch, C_NRM), this->name.c_str(), (this->ranks[rank]).c_str(), CCNRM(ch, C_NRM));
	Clan::ClanSave();
	return;
}

void Clan::HouseOwner(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	DESCRIPTOR_DATA *d;
	long unique = 0;

	if (buffer2.empty())
		send_to_char("������� ��� ���������.\r\n", ch);
	else if (!(unique = GetUniqueByName(buffer2)))
		send_to_char("����������� ��������.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("������� ���� �� ������ ����? �� �������.\r\n", ch);
	else if (!(d = DescByUID(unique)))
		send_to_char("����� ��������� ��� � ����!\r\n", ch);
	else if (GET_CLAN_RENT(d->character) && GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character))
		send_to_char("�� �� ������ �������� ���� ����� ����� ������ �������.\r\n", ch);
	else {
		buffer2[0] = UPPER(buffer2[0]);
		// ������� �����
		ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
		this->members.erase(it);
		Clan::SetClanData(ch);
		// ������ ������ ������� (���� �� ��� � ����� - ��������� ����)
		if (GET_CLAN_RENT(d->character)) {
			this->ClanAddMember(d->character, 0, this->members[GET_UNIQUE(d->character)]->money, this->members[GET_UNIQUE(d->character)]->exp, this->members[GET_UNIQUE(d->character)]->tax);
		} else
			this->ClanAddMember(d->character, 0, 0, 0, 0);
		this->owner = buffer2;
		send_to_char(ch, "�����������, �� �������� ���� ���������� %s!\r\n", GET_PAD(d->character, 2));
		Clan::ClanSave();
	}
}

void Clan::HouseLeave(CHAR_DATA * ch)
{
	if(!GET_CLAN_RANKNUM(ch)) {
		send_to_char("���� �� ������ ���������� ���� �������, �� ����������� � �����.\r\n"
					"� ���� ��� ������ ����� ��������, �� ��������� ���������� � ����� ���� ������...\r\n", ch);
		return;
	}

	ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
	if (it != this->members.end()) {
		this->members.erase(it);
		Clan::SetClanData(ch);
		send_to_char(ch, "�� �������� ������� '%s'.\r\n", this->name.c_str());
		DESCRIPTOR_DATA *d;
		for (d = descriptor_list; d; d = d->next) {
			if (d->character && GET_CLAN_RENT(d->character) == this->rent)
				send_to_char(d->character, "%s �������%s ���� �������.", GET_NAME(ch), GET_CH_SUF_1(ch));
		}
	}
}

void Clan::CheckPkList(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return;
	ClanPkList::iterator it;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)	{
		// ���
		for (ClanPkList::const_iterator pklist = (*clan)->pkList.begin(); pklist != (*clan)->pkList.end(); ++pklist) {
			if ((it = (*clan)->pkList.find(GET_UNIQUE(ch))) != (*clan)->pkList.end())
				send_to_char(ch, "���������� � ������ ������ ������� '%s'.\r\n", (*clan)->name.c_str());
		}
		// ���
		for (ClanPkList::const_iterator frlist = (*clan)->frList.begin(); frlist != (*clan)->frList.end(); ++frlist) {
			if ((it = (*clan)->frList.find(GET_UNIQUE(ch))) != (*clan)->frList.end())
				send_to_char(ch, "���������� � ������ ������ ������� '%s'.\r\n", (*clan)->name.c_str());
		}
	}
}

// ���������� ��� �����... ���������� num � ��� ���� � flags
bool CompareBits(FLAG_DATA flags, const char *names[], int affect)
{
	int i;
	for (i = 0; i < 4; i++) {
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
		while (fail) {
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1) {
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

// ������ ��� ����-���� �� ����� + �����
ACMD(DoStorehouse)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("���� ?\r\n", ch);
		return;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (!(*clan)->storehouse) {
		send_to_char("���� ?\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	if (!*argument) {
		OBJ_DATA *chest;
		int chest_num = real_object(CLAN_CHEST);
		if (chest_num < 0)
			return;
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				send_to_char("��������� ����� �������:\r\n", ch);
				list_obj_to_char(chest->contains, ch, 1, 3);
				return;
			}
		}
	}

	// ��� ������� ���������� ��� ��� ���, ������ ��������� ��� � ����������
	boost::shared_ptr<ChestFilter> filter(new ChestFilter);
	filter->type = -1;
	filter->state= -1;
	filter->wear = -1;
	filter->weap_class = -1;
	filter->affect = -1;

	int num = 0, find = 0;
	char tmpbuf[MAX_INPUT_LENGTH];
	while (*argument) {
		switch (*argument) {
		case '�':
			argument = one_argument(++argument, tmpbuf);
			filter->name = tmpbuf;
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "����") || is_abbrev(tmpbuf, "light"))
				filter->type = ITEM_LIGHT;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "scroll"))
				filter->type = ITEM_SCROLL;
			else if (is_abbrev(tmpbuf, "�������") || is_abbrev(tmpbuf, "wand"))
				filter->type = ITEM_WAND;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "staff"))
				filter->type = ITEM_STAFF;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "weapon"))
				filter->type = ITEM_WEAPON;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "armor"))
				filter->type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "�������") || is_abbrev(tmpbuf, "potion"))
				filter->type = ITEM_POTION;
			else if (is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "������") || is_abbrev(tmpbuf, "other"))
				filter->type = ITEM_OTHER;
			else if (is_abbrev(tmpbuf, "���������") || is_abbrev(tmpbuf, "container"))
				filter->type = ITEM_CONTAINER;
			else if (is_abbrev(tmpbuf, "�����") || is_abbrev(tmpbuf, "book"))
				filter->type = ITEM_BOOK;
			else if (is_abbrev(tmpbuf, "����") || is_abbrev(tmpbuf, "rune"))
				filter->type = ITEM_INGRADIENT;
			else if (is_abbrev(tmpbuf, "����������") || is_abbrev(tmpbuf, "ingradient"))
				filter->type = ITEM_MING;
			else {
				send_to_char("�������� ��� ��������.\r\n", ch);
				return;
			}
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "������"))
				filter->state = 1;
			else if (is_abbrev(tmpbuf, "�����_����������"))
				filter->state = 21;
			else if (is_abbrev(tmpbuf, "���������"))
				filter->state = 41;
			else if (is_abbrev(tmpbuf, "������"))
				filter->state = 61;
			else if (is_abbrev(tmpbuf, "��������"))
				filter->state = 81;
			else {
				send_to_char("�������� ��������� ��������.\r\n", ch);
				return;
			}

			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "�����"))
				filter->wear = 1;
			else if (is_abbrev(tmpbuf, "���") || is_abbrev(tmpbuf, "�����"))
				filter->wear = 2;
			else if (is_abbrev(tmpbuf, "����"))
				filter->wear = 3;
			else if (is_abbrev(tmpbuf, "������"))
				filter->wear = 4;
			else if (is_abbrev(tmpbuf, "����"))
				filter->wear = 5;
			else if (is_abbrev(tmpbuf, "������"))
				filter->wear = 6;
			else if (is_abbrev(tmpbuf, "�����"))
				filter->wear = 7;
			else if (is_abbrev(tmpbuf, "����"))
				filter->wear = 8;
			else if (is_abbrev(tmpbuf, "���"))
				filter->wear = 9;
			else if (is_abbrev(tmpbuf, "�����"))
				filter->wear = 10;
			else if (is_abbrev(tmpbuf, "����"))
				filter->wear = 11;
			else if (is_abbrev(tmpbuf, "��������"))
				filter->wear = 12;
			else if (is_abbrev(tmpbuf, "�����"))
				filter->wear = 13;
			else if (is_abbrev(tmpbuf, "������"))
				filter->wear = 14;
			else if (is_abbrev(tmpbuf, "���"))
				filter->wear = 15;
			else {
				send_to_char("�������� ����� �������� ��������.\r\n", ch);
				return;
			}
			break;
		case '�':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "����"))
				filter->weap_class = 0;
			else if (is_abbrev(tmpbuf, "��������"))
				filter->weap_class = 1;
			else if (is_abbrev(tmpbuf, "�������"))
				filter->weap_class = 2;
			else if (is_abbrev(tmpbuf, "������"))
				filter->weap_class = 3;
			else if (is_abbrev(tmpbuf, "������"))
				filter->weap_class = 4;
			else if (is_abbrev(tmpbuf, "����"))
				filter->weap_class = 5;
			else if (is_abbrev(tmpbuf, "����������"))
				filter->weap_class = 6;
			else if (is_abbrev(tmpbuf, "�����������"))
				filter->weap_class = 7;
			else if (is_abbrev(tmpbuf, "�����"))
				filter->weap_class = 8;
			else {
				send_to_char("�������� ����� ������.\r\n", ch);
				return;
			}
			break;
		case '�':
			num = 0;
			argument = one_argument(++argument, tmpbuf);
			if (strlen(tmpbuf) < 2) {
				send_to_char("������� ���� �� ��� ������ ������� �������.\r\n", ch);
				return;
			}
			for (int flag = 0; flag < 4; ++flag) {
				for (; *weapon_affects[num] != '\n'; ++num) {
					if (is_abbrev(tmpbuf, weapon_affects[num])) {
						filter->affect = num;
						find = 1;
						break;
					}
				}
				++num;
			}
			if (!find) {
				num = 0;
				for (; *apply_types[num] != '\n'; ++num) {
					if (is_abbrev(tmpbuf, apply_types[num])) {
						filter->affect = num;
						find = 2;
						break;
					}
				}
			}
			if (filter->affect < 0) {
				send_to_char("�������� ������ ��������.\r\n", ch);
				return;
			}
			break;
		default:
			++argument;
		}
	}
	std::string buffer = "������� �� ��������� ����������: ";
	if (!filter->name.empty())
		buffer += filter->name + " ";
	if (filter->type >= 0) {
		buffer += item_types[filter->type];
		buffer += " ";
	}
	if (filter->state >= 0) {
		if (filter->state < 20)
			buffer += "������ ";
		else if (filter->state < 40)
			buffer += "����� ���������� ";
		else if (filter->state < 60)
			buffer += "��������� ";
		else if (filter->state < 80)
			buffer += "������ ";
		else
			buffer += "�������� ";
	}
	if (filter->wear >= 0) {
		buffer += wear_bits[filter->wear];
		buffer += " ";
	}
	if (filter->weap_class >= 0) {
		buffer += weapon_class[filter->weap_class];
		buffer += " ";
	}
	if (filter->affect >= 0) {
		if (find == 1)
			buffer += weapon_affects[filter->affect];
		else if (find == 2)
			buffer += apply_types[filter->affect];
		else // ������?
			return;
		buffer += " ";
	}
	buffer += "\r\n";
	send_to_char(buffer, ch);

	int chest_num = real_object(CLAN_CHEST);
	OBJ_DATA *temp_obj, *chest;
	for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
		if (chest->item_number == chest_num) {
			for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content) {
				// ������� ���
				if (!filter->name.empty() && CompareParam(filter->name, temp_obj->name)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// ���
				if (filter->type >= 0 && filter->type == GET_OBJ_TYPE(temp_obj)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// ������
				if (filter->state >= 0) {
					if (!GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(temp_obj))) {
						send_to_char("������� ������ ���������, �������� �����!", ch);
						return;
					}
					int tm = (GET_OBJ_TIMER(temp_obj) * 100 / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(temp_obj)));
					if ((tm + 1) > filter->state) {
						show_obj_to_char(temp_obj, ch, 1, 2, 1);
						continue;
					}
				}
				// ���� ����� �����
				if (filter->wear >= 0 && CAN_WEAR(temp_obj, filter->wear)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// ����� ������
				if (filter->weap_class >= 0 && filter->weap_class == GET_OBJ_SKILL(temp_obj)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// ������
				if (filter->affect >= 0) {
					if (find == 1) {
						if (CompareBits(temp_obj->obj_flags.affects, weapon_affects, filter->affect))
							show_obj_to_char(temp_obj, ch, 1, 2, 1);
					} else if (find == 2) {
						for (int i = 0; i < MAX_OBJ_AFFECT; ++i) {
							if(temp_obj->affected[i].location == filter->affect) {
								show_obj_to_char(temp_obj, ch, 1, 2, 1);
								break;
							}
						}
					}
				}
			}
			break;
		}
	}
}

void Clan::HouseStat(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	bool all = 0;
	std::ostringstream out;

	if (CompareParam(buffer2, "��������") || CompareParam(buffer2, "�������")) {
		if (GET_CLAN_RANKNUM(ch)) {
			send_to_char("� ��� ��� ���� ������� ����������.\r\n", ch);
			return;
		}
		for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it) {
			(*it).second->money = 0;
			(*it).second->exp = 0;
		}
		send_to_char("���������� ����� ������� �������.\r\n", ch);
		return;
	} else if (CompareParam(buffer2, "���")) {
		all = 1;
		out << "���������� ����� �������:\r\n";
	} else
		out << "���������� ����� ������� (����������� ������):\r\n";

	out << "���             ������� �����  ���������� ���  ������� ����� � �����\r\n";
	for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it) {
		if (!all) {
			DESCRIPTOR_DATA *d;
			if (!(d = DescByUID((*it).first)))
				continue;
		}
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << std::setw(15) << (*it).second->name << " " << std::setw(14) << (*it).second->exp << " ";
		out << std::setw(15) << (*it).second->money;
		out << " " << (*it).second->tax << "%\r\n";
	}
	send_to_char(out.str(), ch);
}

int Clan::GetClanTax(CHAR_DATA * ch)
{
	if(IS_NPC(ch) || !GET_CLAN_RENT(ch))
		return 0;
	
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan != Clan::ClanList.end())
		return (*clan)->members[GET_UNIQUE(ch)]->tax;
	return 0;
}

// �� money ��������� � ���������� tax, ������������ ������ �������� ������
int Clan::SetTax(CHAR_DATA * ch, int * money)
{
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan != Clan::ClanList.end()) {
		if(!(*clan)->members[GET_UNIQUE(ch)]->tax)
			return -1;
		int tax = *money * (*clan)->members[GET_UNIQUE(ch)]->tax / 100;
		(*clan)->bank += tax;
		(*clan)->members[GET_UNIQUE(ch)]->money += tax;
		*money -= tax;
		return tax;
	}
	return -1;
}
