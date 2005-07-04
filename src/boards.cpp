/* ****************************************************************************
* File: boards.cpp                                             Part of Bylins *
* Usage: Handling of multiple bulletin boards                                 *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include <fstream>
#include <sstream>
#include <boost/format.hpp>

#include "boards.h"
#include "house.h"
#include "screen.h"
#include "comm.h"

extern GodListType GodList;
extern int isname(const char *str, const char *namelist);

BoardListType Board::BoardList;

Board::Board() : type(0), lastWrite(0), clanRent(0), persUnique(0)
{
	// ��� ����� ������ � �� ����
}

// �������� ���� �����, ����� �������� � ������������
void Board::BoardInit()
{
	// ����� �����
	BoardPtr GeneralBoard(new Board);
	GeneralBoard->type = GENERAL_BOARD;
	GeneralBoard->name = "����";
	GeneralBoard->desc = "�������� ������";
	GeneralBoard->file = ETC_BOARD"general.board";
	GeneralBoard->Load();
	Board::BoardList.push_back(GeneralBoard);

	// ����
	BoardPtr IdeaBoard(new Board);
	IdeaBoard->type = IDEA_BOARD;
	IdeaBoard->name = "����";
	IdeaBoard->desc = "���� � �� ����������";
	IdeaBoard->file = ETC_BOARD"idea.board";
	IdeaBoard->Load();
	Board::BoardList.push_back(IdeaBoard);

	// ������
	BoardPtr ErrorBoard(new Board);
	ErrorBoard->type = ERROR_BOARD;
	ErrorBoard->name = "������";
	ErrorBoard->desc = "��������� �� ������� � ����";
	ErrorBoard->file = ETC_BOARD"error.board";
	ErrorBoard->Load();
	Board::BoardList.push_back(ErrorBoard);

	// �������
	BoardPtr NewsBoard(new Board);
	NewsBoard->type = NEWS_BOARD;
	NewsBoard->name = "�������";
	NewsBoard->desc = "������ � ������� �����";
	NewsBoard->file = ETC_BOARD"news.board";
	NewsBoard->Load();
	Board::BoardList.push_back(NewsBoard);

	// ������� �����
	BoardPtr GodNewsBoard(new Board);
	GodNewsBoard->type = GODNEWS_BOARD;
	GodNewsBoard->name = "GodNews";
	GodNewsBoard->desc = "������������ �������";
	GodNewsBoard->file = ETC_BOARD"god-news.board";
	GodNewsBoard->Load();
	Board::BoardList.push_back(GodNewsBoard);

	// ����� ��� �����
	BoardPtr GodGeneralBoard(new Board);
	GodGeneralBoard->type = GODGENERAL_BOARD;
	GodGeneralBoard->name = "��������";
	GodGeneralBoard->desc = "������������ �������� �������";
	GodGeneralBoard->file = ETC_BOARD"god-general.board";
	GodGeneralBoard->Load();
	Board::BoardList.push_back(GodGeneralBoard);

	// ��� ��������
	BoardPtr GodBuildBoard(new Board);
	GodBuildBoard->type = GODBUILD_BOARD;
	GodBuildBoard->name = "������";
	GodBuildBoard->desc = "������ ��� �������� ���";
	GodBuildBoard->file = ETC_BOARD"god-build.board";
	GodBuildBoard->Load();
	Board::BoardList.push_back(GodBuildBoard);

	// ��� �������
	BoardPtr GodCodeBoard(new Board);
	GodCodeBoard->type = GODCODE_BOARD;
	GodCodeBoard->name = "�����";
	GodCodeBoard->desc = "������ ��� �������� ����";
	GodCodeBoard->file = ETC_BOARD"god-code.board";
	GodCodeBoard->Load();
	Board::BoardList.push_back(GodCodeBoard);

	// ��� ������������ ���������
	BoardPtr GodPunishBoard(new Board);
	GodPunishBoard->type = GODPUNISH_BOARD;
	GodPunishBoard->name = "���������";
	GodPunishBoard->desc = "����������� � ����������";
	GodPunishBoard->file = ETC_BOARD"god-punish.board";
	GodPunishBoard->Load();
	Board::BoardList.push_back(GodPunishBoard);
}

// ����/������ �������� �����
void Board::ClanInit()
{
	// ������ ��� �������
	for (BoardListType::iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		if ((*board)->type == CLAN_BOARD || (*board)->type == CLANNEWS_BOARD) {
			Board::BoardList.erase(board);
			board = Board::BoardList.begin();
		}
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		std::string name = (*clan)->abbrev;
		CreateFileName(name);
		// ������ �������� �����
		BoardPtr board(new Board);
		board->type = CLAN_BOARD;
		board->name = "������";
		board->desc = "�������� ������ ������� " + (*clan)->abbrev;
		board->clanRent = (*clan)->rent;
		board->file = LIB_HOUSE + name + ".board";
		board->Load();
		Board::BoardList.push_back(board);
		// ������ �������� �������
		BoardPtr newsboard(new Board);
		newsboard->type = CLANNEWS_BOARD;
		newsboard->name = "���������";
		newsboard->desc = "������� ������� " + (*clan)->abbrev;
		newsboard->clanRent = (*clan)->rent;
		newsboard->file = LIB_HOUSE + name + "-news.board";
		newsboard->Load();
		Board::BoardList.push_back(newsboard);
	}
}

// ����/������ ������������ �����
void Board::GodInit()
{
	// ������ ��� �������
	for (BoardListType::iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		if ((*board)->type == PERS_BOARD) {
			Board::BoardList.erase(board);
			board = Board::BoardList.begin();
		}
	}

	for (GodListType::const_iterator god = GodList.begin(); god != GodList.end(); ++god) {
		// ������ �������
		BoardPtr board(new Board);
		board->type = PERS_BOARD;
		board->name = "�������";
		board->desc = "��� ������ ��� �������";
		board->persUnique = (*god).first;
		board->persName = (*god).second;
		// ������� ��� � ������ �� �����������
		std::string name = (*god).second;
		CreateFileName(name);
		board->file = ETC_BOARD + name + ".board";
		board->Load();
		Board::BoardList.push_back(board);
	}
}

// ���������� �����, ���� ��� ����������
void Board::Load()
{
	std::ifstream file((this->file).c_str());
	if (!file.is_open()) {
		this->Save();
		return;
	}

	std::string buffer;
	while (file >> buffer) {
		if (buffer == "Message:") {
			MessagePtr message(new Message);
			file >> message->num;
			file >> message->author;
			file >> message->unique;
			file >> message->level;
			file >> message->date;
			ReadEndString(file);
			std::getline(file, message->subject, '~');
			ReadEndString(file);
			std::getline(file, message->text, '~');
			// �� �������� ������� �� �� ��� ���������
			if (message->author.empty() || !message->unique || !message->level || !message->date)
				continue;
			this->messages.push_back(message);
		} else if (buffer == "Type:")
			file >> this->type;
		else if (buffer == "Clan:")
			file >> this->clanRent;
		else if (buffer == "PersUID:")
			file >> this->persUnique;
		else if (buffer == "PersName:")
			file >> this->persName;
	}
	file.close();

	this->Save();
}

// ��������� �����
void Board::Save()
{
	std::ofstream file((this->file).c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", (this->file).c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	file << "Type: " << this->type << " "
		<< "Clan: " << this->clanRent << " "
		<< "PersUID: " << this->persUnique << " "
		<< "PersName: " << (this->persName.empty() ? "none" : this->persName) << "\n";
	for (MessageListType::const_iterator message = this->messages.begin(); message != this->messages.end(); ++message) {
		file << "Message: " << (*message)->num << "\n"
			<< (*message)->author << " "
			<< (*message)->unique << " "
			<< (*message)->level << " "
			<< (*message)->date << "\n"
			<< (*message)->subject << "~\n"
			<< (*message)->text << "~\n";
	}
	file.close();
}

// ������� ��������� ���������
void Board::ShowMessage(CHAR_DATA * ch, MessagePtr message)
{
	std::ostringstream buffer;
	char timeBuf[17];
	strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&message->date));

	buffer << "[" << message->num + 1 << "] "
		<< timeBuf << " "
		<< "(" << message->author << ") :: "
		<< message->subject << "\r\n\r\n"
		<< message->text << "\r\n";

	char *text = str_dup(buffer.str());
	page_string(ch->desc, text, 1);
	free(text);
}

// �������� ���� ���������� ������ ���������� ����� � ���������
long Board::LastReadDate(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return 0;

	switch (this->type) {
	case GENERAL_BOARD:
		return GENERAL_BOARD_DATE(ch);
	case IDEA_BOARD:
		return IDEA_BOARD_DATE(ch);
	case ERROR_BOARD:
		return ERROR_BOARD_DATE(ch);
	case NEWS_BOARD:
		return NEWS_BOARD_DATE(ch);
	case GODNEWS_BOARD:
		return GODNEWS_BOARD_DATE(ch);
	case GODGENERAL_BOARD:
		return GODGENERAL_BOARD_DATE(ch);
	case GODBUILD_BOARD:
		return GODBUILD_BOARD_DATE(ch);
	case GODCODE_BOARD:
		return GODCODE_BOARD_DATE(ch);
	case GODPUNISH_BOARD:
		return GODPUNISH_BOARD_DATE(ch);
	case PERS_BOARD:
		return PERS_BOARD_DATE(ch);
	case CLAN_BOARD:
		return CLAN_BOARD_DATE(ch);
	case CLANNEWS_BOARD:
		return CLANNEWS_BOARD_DATE(ch);
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	return 0;
}

// ����������� ��������� ���� ������ ���������� �����
void Board::SetLastReadDate(CHAR_DATA * ch, long date)
{
	if (IS_NPC(ch))
		return;

	switch (this->type) {
	case GENERAL_BOARD:
		if (GENERAL_BOARD_DATE(ch) < date)
			GENERAL_BOARD_DATE(ch) = date;
		return;
	case IDEA_BOARD:
		if (IDEA_BOARD_DATE(ch) < date)
			IDEA_BOARD_DATE(ch) = date;
		return;
	case ERROR_BOARD:
		if (ERROR_BOARD_DATE(ch) < date)
			ERROR_BOARD_DATE(ch) = date;
		return;
	case NEWS_BOARD:
		if (NEWS_BOARD_DATE(ch) < date)
			NEWS_BOARD_DATE(ch) = date;
		return;
	case GODNEWS_BOARD:
		if (GODNEWS_BOARD_DATE(ch) < date)
			GODNEWS_BOARD_DATE(ch) = date;
		return;
	case GODGENERAL_BOARD:
		if (GODGENERAL_BOARD_DATE(ch) < date)
			GODGENERAL_BOARD_DATE(ch) = date;
		return;
	case GODBUILD_BOARD:
		if (GODBUILD_BOARD_DATE(ch) < date)
			GODBUILD_BOARD_DATE(ch) = date;
		return;
	case GODCODE_BOARD:
		if (GODCODE_BOARD_DATE(ch) < date)
			GODCODE_BOARD_DATE(ch) = date;
		return;
	case GODPUNISH_BOARD:
		if (GODPUNISH_BOARD_DATE(ch) < date)
			GODPUNISH_BOARD_DATE(ch) = date;
		return;
	case PERS_BOARD:
		if (PERS_BOARD_DATE(ch) < date)
			PERS_BOARD_DATE(ch) = date;
		return;
	case CLAN_BOARD:
		if (CLAN_BOARD_DATE(ch) < date)
			CLAN_BOARD_DATE(ch) = date;
		return;
	case CLANNEWS_BOARD:
		if (CLANNEWS_BOARD_DATE(ch) < date)
			CLANNEWS_BOARD_DATE(ch) = date;
		return;
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}

}

ACMD(DoBoard)
{
	if (!ch->desc)
		return;
	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("�� ���������!\r\n", ch);
		return;
	}

	BoardListType::const_iterator board;
	// �������� �����
	if (subcmd == CLAN_BOARD || subcmd == CLANNEWS_BOARD) {
		for (board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board)
			if ((*board)->type == subcmd && (*board)->clanRent == GET_CLAN_RENT(ch))
				break;
		if (board == Board::BoardList.end()) {
			send_to_char("���� ?\r\n", ch);
			return;
		}
	} else if (subcmd == PERS_BOARD && IS_GOD(ch)) {
	// ������������ �����
		for (board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board)
			if ((*board)->type == subcmd && (*board)->persUnique == GET_UNIQUE(ch) && CompareParam((*board)->persName, GET_NAME(ch), 1))
				break;
		if (board == Board::BoardList.end()) {
			send_to_char("���� ?\r\n", ch);
			return;
		}
	} else {
	// �� � � ��������� ������� ����� ���� �� ���� � ��������������� �����, �� ������ ��� ��������
		for (board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board)
			if ((*board)->type == subcmd && (*board)->Access(ch))
				break;
		if (board == Board::BoardList.end()) {
			send_to_char("���� ?\r\n", ch);
			return;
		}
	}

	std::string buffer, buffer2 = argument;
	long date = (*board)->LastReadDate(ch);
	GetOneParam(buffer2, buffer);
	SkipSpaces(buffer2);
	// ������ �������� � buffer, ������ � buffer2

	if (CompareParam(buffer, "������") || CompareParam(buffer, "list")) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("� ��� ��� ����������� ������ ���� ������.\r\n", ch);
			return;
		}
		std::ostringstream body;
		body << "��� �����, �� ������� ������ ������ �������� �������� ���� IMHO.\r\n" 
			<< "������: ������/�������� <����� ���������>, ������ <���� ���������>.\r\n";
		if ((*board)->messages.empty()) {
			body << "����� ���� �� ���������, ����� �����.\r\n";
			send_to_char(body.str(), ch);
			return;
		} else
			body << "����� ���������: " << (*board)->messages.size() << "\r\n";
		char timeBuf[17];
		for (MessageListType::reverse_iterator message = (*board)->messages.rbegin(); message != (*board)->messages.rend(); ++message) {
			if ((*message)->date > date) {
				// ��� ��������� ����� ������ ������-�����
				body << CCWHT(ch, C_NRM);
			}
			strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&(*message)->date));
			body << "[" << (*message)->num + 1 << "] "<< timeBuf << " "
				<< "(" << (*message)->author << ") :: "
				<< (*message)->subject << "\r\n" <<  CCNRM(ch, C_NRM);
		}
		char *text = str_dup(body.str());
		page_string(ch->desc, text, 1);
		free(text);
		return;
	}

	// ��� ������ ������� ��� '������' ��� ����� - ���������� ������ ������������� ���������, ���� ����� ����
	if (buffer.empty() || ((CompareParam(buffer, "������") || CompareParam(buffer, "read")) && buffer2.empty())) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("� ��� ��� ����������� ������ ���� ������.\r\n", ch);
			return;
		}
		for (MessageListType::const_iterator message = (*board)->messages.begin(); message != (*board)->messages.end(); ++message) {
			if ((*message)->date > date) {
				Board::ShowMessage(ch, *message);
				(*board)->SetLastReadDate(ch, (*message)->date);
				return;
			}
		}
		send_to_char("� ��� ��� ������������� ���������.\r\n", ch);
		return;
	}

	unsigned num = 0;
	// ���� ����� ������� ����� ����� ��� '������ �����' - �������� �������� ��� �������, ��� ��������� - ����� �� ��������� '������ �����...' ��� ������
	if (is_number(buffer.c_str()) || ((CompareParam(buffer, "������") || CompareParam(buffer, "read")) && is_number(buffer2.c_str()))) {
		if ((*board)->Access(ch) == 2) {
			send_to_char("� ��� ��� ����������� ������ ���� ������.\r\n", ch);
			return;
		}
		if (CompareParam(buffer, "������"))
			num = atoi(buffer2.c_str());
		else
			num = atoi(buffer.c_str());
		if (num <= 0 || num > (*board)->messages.size()) {
			send_to_char("��� ��������� ����� ��� ������ ����������.\r\n", ch);
			return;
		}
		num = (*board)->messages.size() - num;
		Board::ShowMessage(ch, (*board)->messages[num]);
		(*board)->SetLastReadDate(ch, (*board)->messages[num]->date);
		return;
	}

	if (CompareParam(buffer, "������") || CompareParam(buffer, "write")) {
		if ((*board)->Access(ch) < 2) {
			send_to_char("� ��� ��� ����������� ������ � ���� ������.\r\n", ch);
			return;
		}
		if (!IS_GOD(ch) && (*board)->lastWrite == GET_UNIQUE(ch)) {
			send_to_char("�� �� ���� ������ ��� ������ ����.\r\n", ch);
			return;
		}
		// ������� �������
		MessagePtr tempMessage(new Message);
		tempMessage->author = GET_NAME(ch);
		tempMessage->unique = GET_UNIQUE(ch);
		// ��� ����� ����� �������� � ������������ ����� ����� ������ (��� ��������� ������� ���-��)
		if (subcmd != CLAN_BOARD && subcmd != CLANNEWS_BOARD && subcmd != PERS_BOARD)
			tempMessage->level = GET_LEVEL(ch);
		// ������� ��� ��������������� ����
		if (buffer2.length() > 40) {
			buffer2.erase(40, std::string::npos);
			send_to_char(ch, "���� ��������� ��������� �� '%s'.\r\n", buffer2.c_str());
		}
		// ����, �� � �� ����� �������, � � ��� ��� �� ������� �)
		if (subcmd == ERROR_BOARD) {
			char room[MAX_INPUT_LENGTH];
			sprintf(room, " Room: [%d]", GET_ROOM_VNUM(IN_ROOM(ch)));
			buffer2 += room;
		}
		tempMessage->subject = buffer2;
		// ���� ����� � ����� ����� ��� �� ����� ����������
		ch->desc->message = tempMessage;
		ch->desc->board = *board;

		char **text;
		CREATE(text, char *, 1);
		send_to_char(ch, "������ ������ ���������.  (/s �������� /h ������)\r\n", buffer2.c_str());
		STATE(ch->desc) = CON_WRITEBOARD;
		string_write(ch->desc, text, MAX_MESSAGE_LENGTH, 0, NULL);
	}

	if (CompareParam(buffer, "��������") || CompareParam(buffer, "remove")) {
		if(!is_number(buffer2.c_str())) {
			send_to_char("������� ���������� ����� ���������.\r\n", ch);
			return;
		}
		num = atoi(buffer2.c_str());
		if (num <= 0 || num > (*board)->messages.size()) {
			send_to_char("��� ��������� ����� ��� ������ ����������.\r\n", ch);
			return;
		}
		num = (*board)->messages.size() - num;
		(*board)->SetLastReadDate(ch, (*board)->messages[num]->date);
		// ��� �� ����� �������� ����� ������� (�� ������/����� ����� ��� �������), ��� ������ ����
		if ((*board)->Access(ch) < 4) {
			if ((*board)->messages[num]->unique != GET_UNIQUE(ch)) {
				send_to_char("� ��� ��� ����������� ������� ��� ���������.\r\n", ch);
				return;
			}
		} else if ((*board)->type != CLAN_BOARD && (*board)->type != CLANNEWS_BOARD
			&& (*board)->type != PERS_BOARD && GET_LEVEL(ch) < (*board)->messages[num]->level) {
		// ��� ������� ����� ������� ������ (��� �������� �����)
		// ��� �������� ������ ������ ������ � �������, � ������������ ������ ���
				send_to_char("� ��� ��� ����������� ������� ��� ���������.\r\n", ch);
				return;
		}
		// ���������� ������� � ����������� ����� �������
		(*board)->messages.erase((*board)->messages.begin() + num);
		int count = 0;
		for (MessageListType::reverse_iterator it = (*board)->messages.rbegin(); it != (*board)->messages.rend(); ++it)
			(*it)->num = count++;
		if ((*board)->lastWrite == GET_UNIQUE(ch))
			(*board)->lastWrite = 0;
		(*board)->Save();
		send_to_char("��������� �������.\r\n", ch);
		return;
	}
}

ACMD(DoBoardList)
{
	if (IS_NPC(ch))
		return;

	std::ostringstream buffer;
	buffer << " ���         ���  �����|�����                        ��������  ������\r\n"
		   << " ===  ==========  ===========  ==============================  ======\r\n";
	boost::format boardFormat(" %2d)  %10s  [%3d|%3d]    %30s  %6s\r\n");

	int num = 1;
	std::string access;
	for (BoardListType::const_iterator board = Board::BoardList.begin(); board != Board::BoardList.end(); ++board) {
		int unread = 0;
		int cansee = 1;
		switch ((*board)->Access(ch)) {
		case 0:
			cansee = 0;
			break;
		case 1:
			access = "������";
			break;
		case 2:
			access = "������";
			break;
		case 3:
			access = "������";
			break;
		case 4:
			access = "������";
			break;
		}
		if (!cansee)
			continue;
		for (MessageListType::const_iterator message = (*board)->messages.begin(); message != (*board)->messages.end(); ++message) {
			if ((*message)->date > (*board)->LastReadDate(ch))
				++unread;
		}
		buffer << boardFormat % num % (*board)->name % unread % (*board)->messages.size() % (*board)->desc % access;
		++num;
	}
	send_to_char(buffer.str(), ch);
}

// 0 - ��� �������, 1 - ������, 2 - ������, 3 - ������/������, 4 - ������(� ��������� �����)
int Board::Access(CHAR_DATA * ch)
{
	switch (this->type) {
	case GENERAL_BOARD:
	case IDEA_BOARD:
	// ��� ������, ����� � ���.������, 32 ������
		if (IS_GOD(ch))
			return 4;
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && !GET_REMORT(ch))
			return 1;
		else
			return 3;
	case ERROR_BOARD:
	// ��� ����� � ���.������, 34 ������
		if (IS_IMPL(ch))
			return 4;
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && !GET_REMORT(ch))
			return 0;
		else
			return 2;
	case NEWS_BOARD:
	// ��� ������, 34 ������
		if (IS_IMPL(ch))
			return 4;
		else
			return 1;
	case GODNEWS_BOARD:
	// 32 ������, 34 ������
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 1;
		else
			return 0;
	case GODGENERAL_BOARD:
	// 32 ������/�����, 34 ������
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 3;
		else
			return 0;
	case GODBUILD_BOARD:
	case GODCODE_BOARD:
	// 33 ������/�����, 34 ������
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GRGOD(ch))
			return 3;
		else
			return 0;
	case GODPUNISH_BOARD:
	// 32 ������/�����, 34 ������
		if (IS_IMPL(ch))
			return 4;
		else if (IS_GOD(ch))
			return 3;
		else
			return 0;
	case PERS_BOARD:
		if (IS_GOD(ch) && this->persUnique == GET_UNIQUE(ch) && CompareParam(this->persName, GET_NAME(ch), 1))
			return 4;
		else
			return 0;
	case CLAN_BOARD:
	// �� ����-�������� ���������� ���, ��� ������ ����� ��� ������
		if (GET_CLAN_RENT(ch) == this->clanRent) {
			// �������
			if (!GET_CLAN_RANKNUM(ch))
				return 4;
			else
				return 3;
		} else
			return 0;
	case CLANNEWS_BOARD:
	// ���������� �� �����, �������� ����� ������ ���, ������ ����� �� ����������, ������� ����� ������� �����
		if (GET_CLAN_RENT(ch) == this->clanRent) {
			if (!GET_CLAN_RANKNUM(ch))
				return 4;
			ClanListType::const_iterator clan = Clan::GetClan(ch);
			if ((*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_NEWS])
				return 3;
			return 1;
		} else
			return 0;
	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	return 0;
}

// ����� �� ������������ ����� �������� ������ �� ������ ����� � ����� ����������
SPECIAL(Board::Special)
{
	OBJ_DATA *board = (OBJ_DATA *) me;
	if (!ch->desc)
		return 0;
	// ������ ���������
	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	SkipSpaces(buffer);
	if (CMD_IS("��������") || CMD_IS("���������") || CMD_IS("look") || CMD_IS("examine")
			|| ((CMD_IS("������") || CMD_IS("read")) && isname(buffer2.c_str(), board->name)) ) {
		// ��� ������ ������ ��� ������ '�� �� �����' � �������� ��������� � ��� �����
		if (buffer2.empty()
		    || (buffer.empty() && !isname(buffer2.c_str(), board->name))
		    || (!buffer.empty() && !isname(buffer.c_str(), board->name)))
			return 0;

		// ����� �����
		if (board->item_number == real_object(GENERAL_BOARD_OBJ)) {
			char *temp_arg = str_dup("������");
			DoBoard(ch, temp_arg, 0, GENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		// ����� ���-�����
		if (board->item_number == real_object(GODGENERAL_BOARD_OBJ)) {
			char *temp_arg = str_dup("������");
			DoBoard(ch, temp_arg, 0, GODGENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		// ����� ���������
		if (board->item_number == real_object(GODPUNISH_BOARD_OBJ)) {
			char *temp_arg = str_dup("������");
			DoBoard(ch, temp_arg, 0, GODPUNISH_BOARD);
			free(temp_arg);
			return 1;
		}
		// ����� ��������
		if (board->item_number == real_object(GODBUILD_BOARD_OBJ)) {
			char *temp_arg = str_dup("������");
			DoBoard(ch, temp_arg, 0, GODBUILD_BOARD);
			free(temp_arg);
			return 1;
		}
		// ����� �������
		if (board->item_number == real_object(GODCODE_BOARD_OBJ)) {
			char *temp_arg = str_dup("������");
			DoBoard(ch, temp_arg, 0, GODCODE_BOARD);
			free(temp_arg);
			return 1;
		}
		return 0;
	}
	// ����� ��������� �������� ��������� �������� ���������
	if (CMD_IS("������") || CMD_IS("read") || CMD_IS("������") || CMD_IS("write")
		|| CMD_IS("��������") || CMD_IS("remove")) {
		// ����� �����
		char *temp_arg = str_dup(cmd_info[cmd].command);
		temp_arg = str_add(temp_arg, argument);
		if (board->item_number == real_object(GENERAL_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODGENERAL_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODGENERAL_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODPUNISH_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODPUNISH_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODBUILD_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODBUILD_BOARD);
			free(temp_arg);
			return 1;
		}
		if (board->item_number == real_object(GODCODE_BOARD_OBJ)) {
			DoBoard(ch, temp_arg, 0, GODCODE_BOARD);
			free(temp_arg);
			return 1;
		}
		free(temp_arg);
	}
	return 0;
}
