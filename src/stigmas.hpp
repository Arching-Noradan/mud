#include <vector>
#include <string>
#include "char.hpp"
#include "handler.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

struct Stigma
{
		// id ����������
		unsigned int id;
		// ��� 
		std::string name;
		// ������� ��������� ����������
		void(*activation_stigma)(CHAR_DATA*);
		// ����� �������
		unsigned reload;
	};

struct StigmaWear
{
		Stigma stigma;
		// ����� �� �������
		unsigned int reload;
		// �������� ���
		std::string get_name() const;
};

