/*
* part of bylins
* 2018 (c) bodrich
*/

#include <string>
#include <array>
#include <vector>
#include <bitset>
#include <ctime>
#include <memory>  
#include <unordered_map>
#include "structs.h"

struct DQuest
{
	int id;
	int count;
	time_t time;
};

struct login_index
{
	// ���������� �������
	int count;
	// ���� ���������� ������ � ������� ip
	time_t last_login;
};

class Account
{
private:
	std::string email;
	std::vector<std::string> characters;
	std::vector<DQuest> dquests;
	std::string karma;
	// ������ ����� �� ���� (������ ����)
	std::vector<int> players_list;
	// ������ (� ������ ��� ���) ��������
	std::string hash_password;
	// ���� ���������� ����� � �������
	time_t last_login;
	// ������� �������, ���� - ����, � ��������� ���������� ���, � ������� ��� ���������� ����� � ������� ����-������ + ����, ����� ��������� ��� �������� � ������� ���������
	std::unordered_map<std::string, login_index> history_logins;
public:
	Account(std::string name);
	void save_to_file();
	void read_from_file();
	std::string get_email();
	bool quest_is_available(int id);
	void complete_quest(int id);
	static std::shared_ptr<Account> get_account(std::string email);
	void show_list_players(DESCRIPTOR_DATA *d);
	void add_player(int uid);
	void remove_player(int uid);
	time_t get_last_login();
	void set_last_login();
	void set_password(std::string password);
	bool compare_password(std::string password);
	void show_history_logins(DESCRIPTOR_DATA *d);
	void add_login(std::string ip_addr);
};

