/*************************************************************************
*   File: item.creation.cpp                            Part of Bylins    *
*   Arena and toto code                                                  *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */
// �����������:
//1. ����������� ���������� ����� (��������,�������) - IMMORTALS
//2. ������� ������ �� ��������� ����� - MORTALS
//3. ����� ����� - ���������� ��������� �� ����� � �����.
//4. �������� ����� 
//5. ���� ������� ����� ��� ���. (��� �����)

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "screen.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "arena.hpp"

Arena tour_arena;

const char *arstate[] = {
	"��������",
	"�������",
	"��������"
};

const char *arenamng_cmd[] = {
	"��������", "add",
	"�������", "del",
	"�������", "open",
	"�������", "close",
	"�������", "win",
	"��������", "lose",
	"�����������", "lock",
	"��������������", "unlock",
	"\n"
};

/* ���������� ��������� ����� � ������ */
ACMD(do_arena)
{
	// ������ ������� �� ��������� ����� � ������ 
	// ���������� ������������ �����
	// ������� � �������� �������.
	// ����� �������� ����. 
}

/* ���������� ����������� ����� */
ACMD(do_mngarena)
{
	// ���������� : 
	// �������� - ��������� ��������� ����� (���� ����������� ������ �� ������)
	// ������� - ������� ��������� �����
	// ������� - ���������� ������ ���� ��� ����� ���������
	// �������� - ���������� �������� ��� ������ ����������� �����������. ???

	// ������� - ��������� ����� - ������ ��������� (������ ������������)
	// ������� - ��������� ����� - ������ �����������
	ARENA_DATA *tad;

	int mode, num;


	if (!*argument) {
		tour_arena.show(ch);
		return;
	}

	argument = one_argument(argument, arg);

	if ((mode = search_block(arg, arenamng_cmd, FALSE)) < 0) {
		send_to_char(ch, "������� ���������� ������ : \r\n"
			     "��������, �������, �������, �������, ��������,\r\n"
			     "�������, �����������, ��������������.\r\n");
		return;
	}

	mode >>= 1;
	switch (mode) {
	case ARENA_CMD_ADD:
		// ��������� ��������� ... ������ ��� ��������� 
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "������������ ����� �� ������ ������.\r\n");
		else {
			skip_spaces(&argument);
			if (!*argument) {
				send_to_char("������: mngarena �������� ���.���������\r\n", ch);
				return;
			}
			send_to_char(ch, "��������� ������ ���������.\r\n");
			// ������� ������
			CREATE(tad, ARENA_DATA, 1);
			tad->name = str_dup(argument);
			tad->bet = 0;
			tad->state = AS_WAIT;
			tour_arena.add(ch, tad);
		}
		break;

	case ARENA_CMD_DEL:
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "������������ ����� �� ������ ������.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "������: mngarena ������� �����.���������\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "��������� ��� ����� ������� �� ����������.\r\n");
			else {
				send_to_char(ch, "������� ��������� ��� ������� %d.\r\n", num);
				tour_arena.del(ch, tad);
			}
		}
		break;

	case ARENA_CMD_OPEN:
		if (tour_arena.is_open())
			send_to_char(ch, "����� ��� �������.\r\n");
		else {
			send_to_char(ch, "��������� �����.\r\n");
			tour_arena.open(ch);
		}
		break;

	case ARENA_CMD_CLOSE:
		if (!tour_arena.is_open())
			send_to_char(ch, "����� ��� �������.\r\n");
		else {
			send_to_char(ch, "��������� �����.\r\n");
			tour_arena.close(ch);
		}
		break;

	case ARENA_CMD_WIN:
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "������������ ����� ������.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "������: mngarena ������� �����.���������\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "��������� ��� ����� ������� �� ����������.\r\n");
			else {
				send_to_char(ch, "��������� (%s) ���������� ������.\r\n", tad->name);
				tour_arena.win(ch, tad);
			}
		}
		break;

	case ARENA_CMD_LOSE:
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "������������ ����� ������.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "������: mngarena ������� �����.���������\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "��������� ��� ����� ������� �� ����������.\r\n");
			else {
				send_to_char(ch, "��������� (%s) ���������� ���������.\r\n", tad->name);
				tour_arena.lose(ch, tad);
			}
		}
		break;
	case ARENA_CMD_LOCK:
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (tour_arena.is_locked())
			send_to_char(ch, "����� ��� �������������.\r\n");
		else {
			send_to_char(ch, "����� ������ ������������.\r\n");
			tour_arena.lock(ch);
		}
		break;
	case ARENA_CMD_UNLOCK:
		if (!tour_arena.is_open())
			send_to_char(ch, "���������� ������� ������� �����.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "����� ��� ��������������.\r\n");
		else {
			send_to_char(ch, "����� ������ ��������.\r\n");
			tour_arena.unlock(ch);
		}

		break;
	}
	return;
}

/* ��������� � ����� ����� */
void message_arena(char *message, CHAR_DATA * ch)
{
	// ����� ��������� � ����� �����
	DESCRIPTOR_DATA *i;

	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING &&
		    (!ch || i != ch->desc) &&
		    i->character &&
		    !PRF_FLAGGED(i->character, PRF_NOARENA) &&
		    !PLR_FLAGGED(i->character, PLR_WRITING) &&
		    !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && GET_POS(i->character) > POS_SLEEPING) {
			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(CCIGRN(i->character, C_NRM), i->character);
			act(message, FALSE, i->character, 0, 0, TO_CHAR | TO_SLEEP);

			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(CCNRM(i->character, C_NRM), i->character);
		}
	}
}

Arena::Arena()
{
	opened = false;
	locked = true;
}

Arena::~Arena()
{
	clear();
}

// ������� �����
void
 Arena::clear()
{
	// ������� ������
	ArenaList::iterator p;

	p = arena_slots.begin();

	while (p != arena_slots.end()) {
		if ((*p)->name != NULL)
			free((*p)->name);
		delete(*p);
		p++;
	}
	arena_slots.erase(arena_slots.begin(), arena_slots.end());

	return;
}

// ����������� �����
int Arena::show(CHAR_DATA * ch)
{
	//
	ArenaList::iterator p;
	int i = 0;

	if (!opened)
		send_to_char(ch, "����� �������.\r\n");
	else {
		send_to_char(ch, "����� �������.\r\n");
		if (locked)
			send_to_char(ch, "����� ������ ��������.\r\n");
		else
			send_to_char(ch, "����� ������ ��������.\r\n");

		if (arena_slots.size() == 0)
			send_to_char(ch, "��� ���������� �������.\r\n");
		else {
			send_to_char(ch, "###  ��������              C�����   ���������\r\n");
			send_to_char(ch, "---------------------------------------------\r\n");
			p = arena_slots.begin();
			while (p != arena_slots.end()) {
				i++;
				send_to_char(ch, "%2d.  %-20s %6d   %-10s\r\n", i, (*p)->name,
					     (*p)->bet, arstate[(*p)->state]);
				p++;
			}
		}

	}
	return (TRUE);
}

// ���������� � ������         
int Arena::add(CHAR_DATA * ch, ARENA_DATA * new_arr)
{
	arena_slots.push_back(new_arr);
	sprintf(buf, "����� : �������� ����� �������� ������� - %s.", new_arr->name);
	message_arena(buf, ch);
	return (TRUE);
}

// �������� � �����         
int Arena::del(CHAR_DATA * ch, ARENA_DATA * rem_arr)
{
	if (rem_arr == NULL)
		return (FALSE);

	sprintf(buf, "����� : ������ �������� ������� - %s.", rem_arr->name);
	message_arena(buf, ch);

	arena_slots.remove(rem_arr);

	free(rem_arr->name);
	free(rem_arr);

	return (TRUE);
}

int Arena::win(CHAR_DATA * ch, ARENA_DATA * arenaman)
{
	// �������� ������� ... ??? 
	sprintf(buf, "����� : �������� (%s) �������.", arenaman->name);
	message_arena(buf, ch);


	return (TRUE);
}

int Arena::lose(CHAR_DATA * ch, ARENA_DATA * arenaman)
{
	// �������� �������� ...
	// ������ �� ���� �������������� ����� ��-������������ ��������.
	sprintf(buf, "����� : �������� (%s) ��������.", arenaman->name);
	message_arena(buf, ch);

	// ����������� �� ���� ������� � ���� � ��.


	return (TRUE);
}

int Arena::open(CHAR_DATA * ch)
{
	opened = true;
	locked = true;

	// C������� � �������� �����.
	return (TRUE);
}

ARENA_DATA *Arena::get_slot(int slot_num)
{
	ArenaList::iterator p;

	int i = 0;

	if (slot_num <= 0)
		return (NULL);

	p = arena_slots.begin();
	while (p != arena_slots.end()) {
		if (slot_num - 1 == i) {
			return (*p);
		}
		i++;
		p++;
	}
	return (NULL);
}

int Arena::lock(CHAR_DATA * ch)
{
	locked = true;
	// �������� � ������������ �����.
	sprintf(buf, "����� : ����� ������ ��������.");
	message_arena(buf, ch);

	return (TRUE);
}

int Arena::unlock(CHAR_DATA * ch)
{
	locked = false;
	// �������� � ������������ �����.
	sprintf(buf, "����� : ����� ����� ������.");
	message_arena(buf, ch);

	return (TRUE);
}

int Arena::close(CHAR_DATA * ch)
{
	opened = false;
	locked = true;

	// �������� � �������� �����.
	return (TRUE);
}

int Arena::is_open()
{
	if (opened)
		return (TRUE);
	else
		return (FALSE);
}

int Arena::is_locked()
{
	if (locked)
		return (TRUE);
	else
		return (FALSE);
}

int Arena::make_bet(CHAR_DATA * ch, int slot, int bet)
{
	// ������ ������ �� ���������� ������... 
	ARENA_DATA *tad;
	// ������� � ��������� ����� 1-�� ������ ���������� ����.

	if ((tad = tour_arena.get_slot(slot)) == NULL)
		return (FALSE);

	// ������� ������ � �����.
	GET_BANK_GOLD(ch) -= bet;

	if ((GET_BANK_GOLD(ch) -= bet) < 0) {
		GET_GOLD(ch) += GET_BANK_GOLD(ch);
		GET_BANK_GOLD(ch) = 0;
	}
	// ������������� ��������� ������
	GET_BET(ch) = bet;
	GET_BET_SLOT(ch) = slot;

	tad->bet += bet;

	return (TRUE);
}

SPECIAL(betmaker)
{
	int slot, value;

	if (CMD_IS("������")) {
		// ������ ������ �� ���������� ���������.
		if (tour_arena.is_open() && !tour_arena.is_locked()) {
			if (sscanf(argument, "%d %d", &slot, &value) != 2) {
				send_to_char("������: ����� ������ �����.��������� ��������.������\r\n", ch);
				return (TRUE);
			}
			if (!tour_arena.get_slot(slot)) {
				send_to_char("�������� �� ���������������.\r\n", ch);
				return (TRUE);
			}

			if (GET_BET(ch) || GET_BET_SLOT(ch)) {
				send_to_char(ch, "�� �� �� ��� ������� ������.\r\n");
				return (TRUE);
			}

			if (value > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
				send_to_char("� ��� ��� ����� �����.\r\n", ch);
				return (TRUE);
			}

			if (tour_arena.make_bet(ch, slot, value)) {
				send_to_char(ch, "������ �������.\r\n");
				return (TRUE);
			} else {
				send_to_char(ch, "������ �� �������.\r\n");
				return (TRUE);
			}
		} else {
			// �������� ����� ������� 
			send_to_char(ch, "������ ������ �� �����������.\r\n");
		}
		return (TRUE);
	} else if (CMD_IS("��������")) {
		// �������� �������

		return (TRUE);
	} else
		return (FALSE);
}
