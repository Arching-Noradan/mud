#ifndef __BOARDS_MESSAGE_HPP__
#define __BOARDS_MESSAGE_HPP__

#include <string>
#include <memory>
#include <deque>

// ��������� ���������
struct Message
{
	using shared_ptr = std::shared_ptr<Message>;

	Message() : num(0), unique(0), level(0), rank(0), date(0) {};

	int num;             // ����� �� �����
	std::string author;  // ��� ������
	long unique;         // ��� ������
	int level;           // ������� ������ (��� ����, ����� �������� � ������������ �����)
	int rank;            // ��� ��� � ������
	time_t date;         // ���� �����
	std::string subject; // ����
	std::string text;    // ����� ���������
};

using MessageListType = std::deque<Message::shared_ptr>;

#endif // __BOARDS_MESSAGE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
