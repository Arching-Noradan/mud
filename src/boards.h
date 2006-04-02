/* ****************************************************************************
* File: boards.h                                               Part of Bylins *
* Usage: Header file for bulletin boards                                      *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

#define MAX_MESSAGE_LENGTH 4096 // ������������ ������ ���������
#define MAX_BOARD_MESSAGES 200  // ������������ ���-�� ��������� �� ����� �����
#define MIN_WRITE_LEVEL    6    // ���.����� ��� ����� �� ����� ������
// ���� �����
#define GENERAL_BOARD    0  // �����
#define IDEA_BOARD       1  // ����
#define ERROR_BOARD      2  // ������
#define NEWS_BOARD       3  // �������
#define GODNEWS_BOARD    4  // ������� (������ ��� �����)
#define GODGENERAL_BOARD 5  // ����� (������ ��� �����)
#define GODBUILD_BOARD   6  // ������� (������ ��� �����)
#define GODCODE_BOARD    7  // ������ (������ ��� �����)
#define GODPUNISH_BOARD  8  // ��������� (������ ��� �����)
#define PERS_BOARD       9  // ������������ (������ ��� �����)
#define CLAN_BOARD       10 // ��������
#define CLANNEWS_BOARD   11 // �������� �������
// ��������, �� ������� ������ �������� ��� ������� ������� ������ � �������
#define GODGENERAL_BOARD_OBJ 250
#define GENERAL_BOARD_OBJ    251
#define GODCODE_BOARD_OBJ    253
#define GODPUNISH_BOARD_OBJ  257
#define GODBUILD_BOARD_OBJ   259

typedef boost::shared_ptr<Board> BoardPtr;
typedef std::vector<BoardPtr> BoardListType;
typedef boost::shared_ptr<struct Message> MessagePtr;
typedef std::vector<MessagePtr> MessageListType;

// ��������� ���������
struct Message {
	int num;             // ����� �� �����
	std::string author;  // ��� ������
	long unique;         // ��� ������
	int level;           // ������� ������ (��� ����, ����� �������� � ������������ �����)
	int rank;            // ��� ��� � ������
	time_t date;         // ���� �����
	std::string subject; // ����
	std::string text;    // ����� ���������
};

// �����... ������������ �����
class Board {
public:
	static BoardListType BoardList; // ������ �����
	MessageListType messages; // ������ ���������

	Board();
	int Access(CHAR_DATA * ch);
	int GetType() { return this->type; };
	int GetClanRent() { return this->clanRent; };
	std::string & GetName() { return this->name; };
	void SetLastRead(long unique) { this->lastWrite = unique; };
	void Save();
	static void BoardInit();
	static void ClanInit();
	static void GodInit();
	static SPECIAL(Board::Special);
	static void LoginInfo(CHAR_DATA * ch);

	friend ACMD(DoBoard);
	friend ACMD(DoBoardList);

	private:
	int type;                 // ��� �����
	std::string name;         // ��� �����
	std::string desc;         // �������� �����
	long lastWrite;           // ��� ���������� ��������� (�� ������)
	int clanRent;             // ����� ����� ����� (��� �������� �����)
	int persUnique;           // ��� (��� ������������ �����)
	std::string persName;     // ��� (��� ������������ �����)
	std::string file;         // ��� ����� ��� �����/�����

	void Load();
	void SetLastReadDate(CHAR_DATA * ch, long date);
	long LastReadDate(CHAR_DATA * ch);
	static void ShowMessage(CHAR_DATA * ch, MessagePtr message);
};

#endif
