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

#define MAX_MESSAGE_LENGTH 4096 // ���ᨬ���� ࠧ��� ᮮ�饭��
#define MAX_BOARD_MESSAGES 200  // ���ᨬ��쭮� ���-�� ᮮ�饭�� �� ����� ��᪥
#define MIN_WRITE_LEVEL    6    // ���.����� ��� ���� �� ���� ��᪠�
// ⨯� ��᮪
#define GENERAL_BOARD    0  // ����
#define IDEA_BOARD       1  // ����
#define ERROR_BOARD      2  // �訡��
#define NEWS_BOARD       3  // ������
#define GODNEWS_BOARD    4  // ������ (⮫쪮 ��� �����)
#define GODGENERAL_BOARD 5  // ���� (⮫쪮 ��� �����)
#define GODBUILD_BOARD   6  // ������� (⮫쪮 ��� �����)
#define GODCODE_BOARD    7  // ������ (⮫쪮 ��� �����)
#define GODPUNISH_BOARD  8  // ��������� (⮫쪮 ��� �����)
#define PERS_BOARD       9  // ���ᮭ��쭠� (⮫쪮 ��� �����)
#define CLAN_BOARD       10 // ��������
#define CLANNEWS_BOARD   11 // ������� ������
// �६���, �� ����� ��蠥� ᯥ訠�� ��� ��ண� ᯮᮡ� ࠡ��� � ��᪠��
#define GODGENERAL_BOARD_OBJ 250
#define GENERAL_BOARD_OBJ    251
#define GODCODE_BOARD_OBJ    253
#define GODPUNISH_BOARD_OBJ  257
#define GODBUILD_BOARD_OBJ   259

typedef boost::shared_ptr<class Board> BoardPtr;
typedef std::vector<BoardPtr> BoardListType;
typedef boost::shared_ptr<struct Message> MessagePtr;
typedef std::vector<MessagePtr> MessageListType;

// �⤥�쭮� ᮮ�饭��
struct Message {
	int num;             // ����� �� ��᪥
	std::string author;  // ��� ����
	long unique;         // 㨤 ����
	int level;           // �஢��� ���� (��� ���, �஬� �������� � ���ᮭ����� ��᮪)
	long date;           // ��� ����
	std::string subject; // ⥬�
	std::string text;    // ⥪�� ᮮ�饭��
};

// ��᪨... ��᫥������� ��䨪
class Board
{
	public:
	Board();
	int type;                 // ⨯ ��᪨
	std::string name;         // ��� ��᪨
	std::string desc;         // ���ᠭ�� ��᪨
	long lastWrite;           // 㨤 ��᫥����� ��ᠢ襣� (�� ॡ��)
	int clanRent;             // ����� ७�� ����� (��� �������� ��᮪)
	int persUnique;           // 㨤 (��� ���ᮭ��쭮� ��᪨)
	std::string persName;     // ��� (��� ���ᮭ��쭮� ��᪨)
	MessageListType messages; // ᯨ᮪ ᮮ�饭��
	std::string file;         // ��� 䠩�� ��� ᥩ��/�����

	int Access(CHAR_DATA * ch);
	void SetLastReadDate(CHAR_DATA * ch, long date);
	long LastReadDate(CHAR_DATA * ch);
	void Load();
	void Save();

	static BoardListType BoardList; // ᯨ᮪ ��᮪

	static void BoardInit();
	static void Board::ClanInit();
	static void Board::GodInit();
	static void ShowMessage(CHAR_DATA * ch, MessagePtr message);
	static SPECIAL(Board::Special);

	friend ACMD(DoBoard);
	friend ACMD(DoBoardList);

	private:

};

#endif
