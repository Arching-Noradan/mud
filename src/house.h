/* ****************************************************************************
* File: house.h                                                Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _HOUSE_H_
#define _HOUSE_H_

#include "conf.h"
#include <vector>
#include <map>
#include <bitset>
#include <string>
#include <boost/shared_ptr.hpp>

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

#define CLAN_PRIVILEGES_NUM 12
#define MAY_CLAN_INFO       0
#define MAY_CLAN_ADD        1
#define MAY_CLAN_REMOVE     2
#define MAY_CLAN_PRIVILEGES 3
#define MAY_CLAN_CHANNEL    4
#define MAY_CLAN_POLITICS   5
#define MAY_CLAN_NEWS       6
#define MAY_CLAN_PKLIST     7
#define MAY_CLAN_CHEST_PUT  8
#define MAY_CLAN_CHEST_TAKE 9
#define MAY_CLAN_BANK       10
#define MAY_CLAN_EXIT       11
// �� �������� ��� CLAN_PRIVILEGES_NUM

#define POLITICS_NEUTRAL  0
#define POLITICS_WAR      1
#define POLITICS_ALLIANCE 2

#define HCE_ATRIUM 0
#define	HCE_PORTAL 1

#define CLAN_MAIN_MENU      0
#define CLAN_PRIVILEGE_MENU 1
#define CLAN_SAVE_MENU      2
#define CLAN_ADDALL_MENU    3
#define CLAN_DELALL_MENU    4

// vnum ��������� �������
#define CLAN_CHEST 330
// ������ ������ �� ����� (�����)
#define CHEST_UPDATE_PERIOD 10
// ������ ���������� � ������ ������� ����� (�����)
#define CHEST_INVOICE_PERIOD 60
// ������ ���������� ������ ����� � ���� ������ � ������ ������� ���������� �� ���� (�����)
#define CLAN_TOP_REFRESH_PERIOD 360
// �������� ����� � ����
#define CLAN_TAX 1000
// ����� �� ������� �� ���������� �� ��������� � ����
#define CLAN_STOREHOUSE_TAX 1000
// ������� ��������� ����� ������ (������) ��� ���������
#define CLAN_STOREHOUSE_COEFF 50

#define MAX_CLANLEVEL 5
// ����� ���� � ����������� ����-�����
#define CLAN_STUFF_ZONE 18

#define CHEST_IDENT_PAY 110

class ClanMember {
	public:
	ClanMember() : rank_num(0), money(0), exp(0), exp_persent(0), clan_exp(0) {};
	std::string name;   // ��� ������
	int rank_num;       // ����� �����
	long long money;    // ������ ��������� �� ��������� � �������� �����
	long long exp;      // ��������� ���-�����
	int exp_persent;    // ������� ����� ����������� � ����
	long long clan_exp; // ��������� ����-�����
};

struct ClanPk {
	long author;            // ��� ������
	std::string victimName;	// ��� ������
	std::string authorName;	// ��� ������
	time_t time;            // ����� ������
	std::string text;       // �����������
};

struct ClanStuffName {
	int num;
	std::string name;
	std::string desc;
	std::string longdesc;
	std::vector<std::string> PNames;
};

class Clan;

typedef boost::shared_ptr<Clan> ClanPtr;
typedef std::vector<ClanPtr> ClanListType;
typedef boost::shared_ptr<ClanMember> ClanMemberPtr;
typedef std::map<long, ClanMemberPtr> ClanMemberList;
typedef boost::shared_ptr<ClanPk> ClanPkPtr;
typedef std::map<long, ClanPkPtr> ClanPkList;
typedef std::vector<std::bitset<CLAN_PRIVILEGES_NUM> > ClanPrivileges;
typedef std::map<int, int> ClanPolitics;
typedef std::vector<ClanStuffName> ClanStuffList;

struct ClanOLC {
	int mode;                  // ��� �������� ��������� ���
	ClanPtr clan;              // ����, ������� ������
	ClanPrivileges privileges; // ���� ������ ���������� �� ������ �� ���������� ��� ������
	int rank;                  // ������������� � ������ ������ ����
	std::bitset<CLAN_PRIVILEGES_NUM> all_ranks; // ����� ��� ��������/���������� ���� ������
};

struct ClanInvite {
	ClanPtr clan; // ������������ ����
	int rank;     // ����� �������������� �����
};


class Clan
{
	public:
	Clan();
	~Clan();

	static ClanListType ClanList; // ������ ������

	static void ClanLoad();
	static void ClanSave();
	static void ChestSave();
	static void HconShow(CHAR_DATA * ch);
	static void SetClanData(CHAR_DATA * ch);
	static void ChestUpdate();
	static bool MayEnter(CHAR_DATA * ch, room_rnum room, bool mode);
	static bool InEnemyZone(CHAR_DATA * ch);
	static bool PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static bool TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static void ChestInvoice();
	static bool BankManage(CHAR_DATA * ch, char *arg);
	static room_rnum CloseRent(room_rnum to_room);
	static ClanListType::const_iterator IsClanRoom(room_rnum room);
	static void CheckPkList(CHAR_DATA * ch);
	static void SyncTopExp();
	static int GetTotalCharScore(CHAR_DATA * ch);
	static int GetRankByUID(long);
	static bool ChestShow(OBJ_DATA * list, CHAR_DATA * ch);
	static void remove_from_clan(long unique);

	void Manage(DESCRIPTOR_DATA * d, const char * arg);
	void AddTopExp(CHAR_DATA * ch, int add_exp);

	const char * GetAbbrev() { return this->abbrev.c_str(); };
	int GetRent();
	int SetClanExp(CHAR_DATA *ch, int add);  //�� ����� - ����� � ���� - �� ������ ����� ������ ������. �� ������� ��� ��� ���� � ����
	int GetMemberExpPersent(CHAR_DATA *ch);
	int GetClanLevel() { return this->clan_level; };
	std::string GetClanTitle() { return this->title; };
	bool CheckPrivilege(int rank, int privilege) { return this->privileges[rank][privilege]; };

	friend ACMD(DoHouse);
	friend ACMD(DoClanChannel);
	friend ACMD(DoClanList);
	friend ACMD(DoShowPolitics);
	friend ACMD(DoHcontrol);
	friend ACMD(DoWhoClan);
	friend ACMD(DoClanPkList);
	friend ACMD(DoStoreHouse);
	friend ACMD(do_clanstuff);

	private:
	std::string abbrev; // ������������ �����, ���� �����
	std::string name;   // ������� ��� �����
	std::string title;  // ��� ����� ����� � ������ ������ ����� (����� ���.�����, ���� ��� �� ������������)
	std::string owner;  // ��� �������
	mob_vnum guard;     // �������� �����
	time_t builtOn;     // ���� ��������
	double bankBuffer;  // ������ ��� ����� ������� ������ �� ���������
	bool entranceMode;  // ���� � ����� ��� ����/������ ���� � ������
	std::vector <std::string> ranks; // ������ �������� ������
	std::vector <std::string> ranks_female; // ������ �������� ������ ��� �������� ����
	ClanPolitics politics;     // ��������� ��������
	ClanPkList pkList;  // ������
	ClanPkList frList;  // ������
	long bank;          // ��������� ����� �����
	long long exp; // ��������� ���-�����
	long long clan_exp; //��������� ����-�����
	long exp_buf;  // ������ ��� ��������� ���-����� � ������ ������� �������� � ���-����� (exp_info), ������������� ��� � 6 �����
	int clan_level; // ������� ������� �����
	int rent;       // ����� ����������� ������� � �����, ������ ��� �����
	int out_rent;   // ����� ���� ��� ����������, ����� �� ���������� � ����� ������
	int chest_room; // ������� � ��������, �� ������� ��������� �����. ����� �� ������ �������� ���� � ������
	ClanPrivileges privileges; // ������ ���������� ��� ������
	ClanMemberList members;    // ������ ������ ������� (���, ���, ����� �����)
	ClanStuffList clanstuff;   // ����-����
	bool storehouse;    // ����� ������� �� ��������� �� ���������� �����
	bool exp_info;      // ���������� ��� ��� ��������� �����
	bool test_clan;     // �������� ���� (������ ���)
	//no save
	int chest_objcount;
	int chest_discount;
	int chest_weight;
	// ������, ���� �������� ��� ����-������ �����, �� ���� ����� ��� � ������ ��������

	void ClanUpgrade();
	int CheckPolitics(int victim);
	void SetPolitics(int victim, int state);
	void ManagePolitics(CHAR_DATA * ch, std::string & buffer);
	void HouseInfo(CHAR_DATA * ch);
	void HouseAdd(CHAR_DATA * ch, std::string & buffer);
	void HouseRemove(CHAR_DATA * ch, std::string & buffer);
	void TaxManage(CHAR_DATA * ch, std::string & arg);
	void ClanAddMember(CHAR_DATA * ch, int rank);
	void HouseOwner(CHAR_DATA * ch, std::string & buffer);
	void HouseLeave(CHAR_DATA * ch);
	int GetClanScore();
	void HouseStat(CHAR_DATA * ch, std::string & buffer);
	void remove_member(ClanMemberList::iterator &it);

	// house ��� ���
	void MainMenu(DESCRIPTOR_DATA * d);
	void PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num);
	void AllMenu(DESCRIPTOR_DATA * d, unsigned flag);
	void GodToChannel(CHAR_DATA *ch, std::string text, int subcmd);
	void CharToChannel(CHAR_DATA *ch, std::string text, int subcmd);

	static void HcontrolBuild(CHAR_DATA * ch, std::string & buffer);
	static void HcontrolDestroy(CHAR_DATA * ch, std::string & buffer);
	static void hcon_outcast(CHAR_DATA *ch, std::string buffer);
	static void ChestLoad();
	int ChestTax();
	int ChestMaxObjects() {return (this->clan_level+1)*500+100;};
	int ChestMaxWeight() {return (this->clan_level+1)*5000+500;};

	// ��� ���������� ������ ������ ����� �� ������, ����� ��� ����� ���� ���� ���������
	class SortRank
	{
		public:
		bool operator() (const CHAR_DATA * ch1, const CHAR_DATA * ch2);
	};
};

void SetChestMode(CHAR_DATA *ch, std::string &buffer);
std::string GetChestMode(CHAR_DATA *ch);

#endif
