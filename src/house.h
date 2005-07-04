/* ****************************************************************************
* File: house.h                                                Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _HOUSE_H_
#define _HOUSE_H_

#include <vector>
#include <map>
#include <bitset>
#include <string>
#include <boost/shared_ptr.hpp>

#include "conf.h"
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
#define MAY_CLAN_LEAVE      11
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
// �������� ����� � ����
#define CLAN_TAX 1000
// ����� �� ������� �� ���������� �� ��������� � ����
#define CLAN_STOREHOUSE_TAX 1000
// ������� ��������� ����� ������ (������) ��� ���������
#define CLAN_STOREHOUSE_COEFF 50

struct ClanMember {
	std::string name; // ��� ������
	int rankNum;      // ����� �����
	long long money;  // ������ ��������� �� ��������� � �������� �����
	unsigned long long exp; // ��������� ����� ���������
	int tax;          // �������, ����������� � �����
};

struct ClanPk {
	long author;            // ��� ������
	std::string victimName;	// ��� ������
	std::string authorName;	// ��� ������
	time_t time;            // ����� ������
	std::string text;       // �����������
};

// ��� ������� ����������� ��������� �� ��������
struct ChestFilter {
	std::string name;       // ���
	int type;               // ���
	int state;              // ���������
	int wear;               // ���� ���������
	int weap_class;         // ����� ������
	int affect;             // ������
};

typedef boost::shared_ptr<class Clan> ClanPtr;
typedef std::vector<ClanPtr> ClanListType;
typedef boost::shared_ptr<ClanMember> ClanMemberPtr;
typedef std::map<long, ClanMemberPtr> ClanMemberList;
typedef boost::shared_ptr<ClanPk> ClanPkPtr;
typedef std::map<long, ClanPkPtr> ClanPkList;
typedef std::vector<std::bitset<CLAN_PRIVILEGES_NUM> > ClanPrivileges;
typedef std::map<room_vnum, int> ClanPolitics;
typedef std::vector< OBJ_DATA *> ClanObjList;

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
	std::string abbrev;           // ������������ �����, ���� �����
	room_vnum rent;               // ����� ����������� ������� � �����, ������ ��� �����
	ClanPrivileges privileges;    // ������ ���������� ��� ������
	long bank;                    // ��������� ����� �����
	unsigned long long exp;       // ��������� ����� �����
	ClanMemberList _members;      // ������ ������ ������� (���, ���, ����� �����)

	static void ClanLoad();
	static void ClanSave();
	static void ChestSave();
	static void ClanInfo(CHAR_DATA * ch);
	static void SetClanData(CHAR_DATA * ch);
	static void ChestUpdate();
	static bool MayEnter(CHAR_DATA * ch, room_rnum room, bool mode);
	static bool InEnemyZone(CHAR_DATA * ch);
	static bool PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static bool TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static bool BankManage(CHAR_DATA * ch, char *arg);
	static ClanListType::const_iterator GetClan(CHAR_DATA * ch);
	static room_rnum CloseRent(room_rnum to_room);
	static const char *GetAbbrev(CHAR_DATA * ch);
	static SPECIAL(ClanChest);
	static ClanListType::const_iterator IsClanRoom(room_rnum room);
	static void CheckPkList(CHAR_DATA * ch);
	static int GetClanTax(CHAR_DATA * ch);
	static int SetTax(CHAR_DATA * ch, int * money);
	void Manage(DESCRIPTOR_DATA * d, const char * arg);

	friend ACMD(DoHouse);
	friend ACMD(DoClanChannel);
	friend ACMD(DoClanList);
	friend ACMD(DoShowPolitics);
	friend ACMD(DoHcontrol);
	friend ACMD(DoWhoClan);
	friend ACMD(DoClanPkList);
	friend ACMD(DoStorehouse);

	private:
	std::string _name;   // ������� ��� �����
	std::string _title;  // ��� ����� ����� � ������ ������ ����� (����� ���.�����, ���� ��� �� ������������)
	std::string _owner;  // ��� �������
	mob_vnum _guard;     // �������� �����
	time_t _builtOn;     // ���� ��������
	double _bankBuffer;  // ������ ��� ����� ������� ������ �� ���������
	bool _entranceMode;  // ���� � ����� ��� ����/������ ���� � ������
	std::vector <std::string> _ranks; // ������ �������� ������
	ClanPolitics _politics;     // ��������� ��������
	ClanPkList _pkList;  // ������
	ClanPkList _frList;  // ������
	ClanObjList _chest;  // ������ ��������� � �������
	bool _storehouse;    // ����� ������� �� ��������� �� ���������� �����
	// ������, ���� �������� ��� ����-������ �����, �� ���� ����� ��� � ������ ��������

	int CheckPolitics(room_vnum victim);
	void SetPolitics(room_vnum victim, int state);
	void ManagePolitics(CHAR_DATA * ch, std::string & buffer);
	void HouseInfo(CHAR_DATA * ch);
	void HouseAdd(CHAR_DATA * ch, std::string & buffer);
	void HouseRemove(CHAR_DATA * ch, std::string & buffer);
	void ClanAddMember(CHAR_DATA * ch, int rank, long long money, unsigned long long exp, int tax);
	void HouseOwner(CHAR_DATA * ch, std::string & buffer);
	void HouseLeave(CHAR_DATA * ch);
	void Clan::HouseStat(CHAR_DATA * ch, std::string & buffer);
	// house ��� ���
	void MainMenu(DESCRIPTOR_DATA * d);
	void PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num);
	void AllMenu(DESCRIPTOR_DATA * d, unsigned flag);

	static void HcontrolBuild(CHAR_DATA * ch, std::string & buffer);
	static void HcontrolDestroy(CHAR_DATA * ch, std::string & buffer);
	static void ChestLoad();

	// ��� ���������� ������ ������ ����� �� ������
	class SortRank
	{
		public:
		bool operator() (const ClanMemberPtr ch1, const ClanMemberPtr ch2) {
			return ch1->rankNum < ch2->rankNum;
		}
		bool operator() (const CHAR_DATA * ch1, const CHAR_DATA * ch2) {
			return GET_CLAN_RANKNUM(ch1) < GET_CLAN_RANKNUM(ch2);
		}
	};
};

#endif
