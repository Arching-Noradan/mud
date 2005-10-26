/* ************************************************************************
*   File: act.offensive.cpp                               Part of Bylins  *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "screen.h"
#include "spells.h"
#include "skills.h"
#include "pk.h"
#include "features.hpp"

/* extern variables */
extern DESCRIPTOR_DATA *descriptor_list;

/* extern functions */
void raw_kill(CHAR_DATA * ch, CHAR_DATA * killer);
//void   check_killer(CHAR_DATA * ch, CHAR_DATA * vict);
int compute_armor_class(CHAR_DATA * ch);
int awake_others(CHAR_DATA * ch);
void appear(CHAR_DATA * ch);
int legal_dir(CHAR_DATA * ch, int dir, int need_specials_check, int show_msg);
void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance);
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict);
/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_manadrain);
ACMD(do_coddle_out);

int have_mind(CHAR_DATA * ch)
{
	if (!AFF_FLAGGED(ch, AFF_CHARM) && !IS_HORSE(ch))
		return (TRUE);
	return (FALSE);
}

void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room)
{
	if (!WAITLESS(ch) && (!victim_in_room || (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))))
		WAIT_STATE(ch, waittime * PULSE_VIOLENCE);
};

int set_hit(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return (FALSE);
	}
	if (FIGHTING(ch) || GET_MOB_HOLD(ch)) {
		return (FALSE);
	}
	hit(ch, victim, TYPE_UNDEFINED, AFF_FLAGGED(ch, AFF_STOPRIGHT) ? 2 : 1);
	set_wait(ch, 2, TRUE);
	return (TRUE);
};

int onhorse(CHAR_DATA * ch)
{
	if (on_horse(ch)) {
		act("��� ������ $N.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return (TRUE);
	}
	return (FALSE);
};

// Add by Voropay 8/05/2004
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch, int skill)
{

	CHAR_DATA *vict;
	int percent = 0;
	int prob = 0;

	for (vict = world[IN_ROOM(victim)]->people; vict; vict = vict->next_in_room) {
		if (PROTECTING(vict) == victim &&
		    !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
		    !AFF_FLAGGED(vict, AFF_MAGICSTOPFIGHT) &&
		    GET_WAIT(vict) <= 0 && !GET_MOB_HOLD(vict) && GET_POS(vict) >= POS_FIGHTING) {
			percent = number(1, skill_info[SKILL_PROTECT].max_percent);
			prob = calculate_skill(vict, SKILL_PROTECT, skill_info[SKILL_PROTECT].max_percent, victim);
			prob = prob * 8 / 10;
			improove_skill(vict, SKILL_PROTECT, prob >= percent, ch);

			if (GET_GOD_FLAG(vict, GF_GODSCURSE))
				percent = 0;


			if ((FIGHTING(vict) != ch) && (ch != victim)) {
				// ����������� � ���������� ������������� ...
				pk_agro_action(vict, ch);
				stop_fighting(vict, FALSE);
				set_fighting(vict, ch);
				// ��� �� ������������.
				set_wait(vict, 2, TRUE);
				// ��������������� ���������.
				go_protect(vict, victim);
			}

			if (prob < percent) {
				act("�� �� ������ �������� $N3.", FALSE, vict, 0, victim, TO_CHAR);
				act("$N �� ����$Q �������� ���.", FALSE, victim, 0, vict, TO_CHAR);
				act("$n �� ����$q �������� $N3.", TRUE, vict, 0, victim, TO_NOTVICT);
			} else {
				act("�� ���������� �������� $N3, ������ ���� �� ����.", FALSE,
				    vict, 0, victim, TO_CHAR);
				act("$N ���������� �������$G ���, ������ ���� �� ����.", FALSE,
				    victim, 0, vict, TO_CHAR);
				act("$n ���������� �������$g $N3, ������ ���� �� ����.", TRUE,
				    vict, 0, victim, TO_NOTVICT);
				return vict;
				break;
			}


		}
	}
	return victim;
}

void parry_override(CHAR_DATA * ch)
{
	char *message = NULL;
	if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
		message = "�� ���������� ��������� �� ��� � ��������� � ���.";
		CLR_AF_BATTLE(ch, EAF_BLOCK);
	}
	if (GET_AF_BATTLE(ch, EAF_PARRY)) {
		message = "�� ���������� ���������� ����� � ��������� � ���.";
		CLR_AF_BATTLE(ch, EAF_PARRY);
	}
	if (GET_AF_BATTLE(ch, EAF_MULTYPARRY)) {
		message = "�� ������ � ������ � ��������� � ���.";
		CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
	}
	if (GET_AF_BATTLE(ch, EAF_PROTECT)) {
		message = "�� �������� ������ �������� ��� ��������� � �������� � ���.";
		CLR_AF_BATTLE(ch, EAF_PROTECT);
		PROTECTING(ch) = NULL;
	}

	if (message)
		act(message, FALSE, ch, 0, 0, TO_CHAR);
}

int used_attack(CHAR_DATA * ch)
{
	char *message = NULL;

	parry_override(ch);

	if (!GET_EXTRA_VICTIM(ch))
		return (FALSE);
	else
		switch (GET_EXTRA_SKILL(ch)) {
		case SKILL_BASH:
			message = "����������. �� ��������� ����� $N3.";
			break;
		case SKILL_KICK:
			message = "����������. �� ��������� ����� $N3.";
			break;
		case SKILL_CHOPOFF:
			message = "����������. �� ��������� ������� $N3.";
			break;
		case SKILL_DISARM:
			message = "����������. �� ��������� ����������� $N3.";
			break;
		case SKILL_THROW:
			message = "����������. �� ��������� ������� ������ � $N3.";
			break;
		default:
			return (FALSE);
		}
	if (message)
		act(message, FALSE, ch, 0, GET_EXTRA_VICTIM(ch), TO_CHAR);
	return (TRUE);
}

ACMD(do_assist)
{
	CHAR_DATA *helpee, *opponent;

	if (FIGHTING(ch)) {
		send_to_char("����������. �� ���������� ����.\r\n", ch);
		return;
	}
	one_argument(argument, arg);

	if (!*arg) {
		for (helpee = world[ch->in_room]->people; helpee; helpee = helpee->next_in_room)
			if (FIGHTING(helpee) && FIGHTING(helpee) != ch && ((ch->master && ch->master == helpee->master)
									   || ch->master == helpee
									   || helpee->master == ch))
				break;
		if (!helpee) {
			send_to_char("���� �� ������ ������ ?\r\n", ch);
			return;
		}
	} else if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char(NOPERSON, ch);
		return;
	} else if (helpee == ch) {
		send_to_char("��� ����� ������ ������ ���� !\r\n", ch);
		return;
	}

	/*
	 * Hit the same enemy the person you're helping is.
	 */
	if (FIGHTING(helpee))
		opponent = FIGHTING(helpee);
	else
		for (opponent = world[ch->in_room]->people;
		     opponent && (FIGHTING(opponent) != helpee); opponent = opponent->next_in_room);

	if (!opponent)
		act("�� ����� �� ��������� � $N4!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (!CAN_SEE(ch, opponent))
		act("�� �� ������ ���������� $N1 !", FALSE, ch, 0, helpee, TO_CHAR);
	else if (opponent == ch)
		act("��� $E ��������� � ���� !", FALSE, ch, 0, helpee, TO_CHAR);
	else if (!may_kill_here(ch, opponent))
		return;
	else if (check_pkill(ch, opponent, NULL))
		act("����������� ������� '���������' ��� ��������� �� $N1.", FALSE, ch, 0, opponent, TO_CHAR);
	else if (set_hit(ch, opponent)) {
		act("�� �������������� � �����, ������� $N2!", FALSE, ch, 0, helpee, TO_CHAR);
		act("$N �����$G ������ ��� � ����� !", 0, helpee, 0, ch, TO_CHAR);
		act("$n �������$g � ��� �� ������� $N1.", FALSE, ch, 0, helpee, TO_NOTVICT);
	}
}


ACMD(do_hit)
{
	CHAR_DATA *vict;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("���� ����-�� ����� ?\r\n", ch);
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_to_char("�� �� ������ ����.\r\n", ch);
	else if (vict == ch) {
		send_to_char("�� ������� ����... ���� ������ ��!.\r\n", ch);
		act("$n ������$g ����, � ������ �������$g '�������, ������ ����...'",
		    FALSE, ch, 0, vict, TO_ROOM | CHECK_DEAF);
		act("$n ������$g ����", FALSE, ch, 0, vict, TO_ROOM | CHECK_NODEAF);
	} else if (!may_kill_here(ch, vict))
		return;
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
		act("$N ������� ����� ��� ���, ����� ���� $S.", FALSE, ch, 0, vict, TO_CHAR);
	else {
		if (subcmd != SCMD_MURDER && check_pkill(ch, vict, arg))
			return;
		if (FIGHTING(ch)) {
			if (vict == FIGHTING(ch)) {
				act("�� ��� ���������� � $N4.", FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
			if (ch != FIGHTING(vict)) {
				act("$N �� ��������� � ����, �� �������� $S.", FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
			stop_fighting(ch, FALSE);
			set_fighting(ch, vict);
			set_wait(ch, 2, TRUE);
		} else if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
			set_hit(ch, vict);
		} else
			send_to_char("��� ���� �� �� ��� !\r\n", ch);
	}
}



ACMD(do_kill)
{
	CHAR_DATA *vict;

	if (!IS_IMPL(ch)) {
		do_hit(ch, argument, cmd, subcmd);
		return;
	}
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("���� �� ����� ������ ������-�� ?\r\n", ch);
	} else {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			send_to_char("� ��� ��� ����� :P.\r\n", ch);
		else if (ch == vict)
			send_to_char("�� ��������... :(\r\n", ch);
		else if (IS_IMPL(vict) && !GET_COMMSTATE(ch))
			send_to_char("� ���� �� ��� �������� �������� ? �����, �������, ����� !\r\n", ch);
		else {
			act("�� �������� $N3 � ����! ��������! �����!", FALSE, ch, 0, vict, TO_CHAR);
			act("$N �������$g ��� � ���� ����� ����������� ������!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n ������ ���������$g �������� $N3!", FALSE, ch, 0, vict, TO_NOTVICT);
			raw_kill(vict, ch);
		}
	}
}

/************************ BACKSTAB VICTIM */
void go_backstab(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;


	if (onhorse(ch))
		return;

	pk_agro_action(ch, vict);

	if (((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) || (FIGHTING(vict) && GET_CLASS(ch) != CLASS_THIEF))
	    && !IS_GOD(ch)) {
		act("�� ��������, ��� $N �������$y ��� �������� !", FALSE, vict, 0, ch, TO_CHAR);
		act("$n �������$g ���� ������� �������� $s !", FALSE, vict, 0, ch, TO_VICT);
		act("$n �������$g ������� $N1 �������� $s !", FALSE, vict, 0, ch, TO_NOTVICT);
		set_hit(vict, ch);
		return;
	}

	percent = number(1, skill_info[SKILL_BACKSTAB].max_percent);
	prob = train_skill(ch, SKILL_BACKSTAB, skill_info[SKILL_BACKSTAB].max_percent, vict);

	if (FIGHTING(vict))
		prob = prob * (GET_REAL_DEX(ch) + 50) / 100;

	if (AFF_FLAGGED(ch, AFF_HIDE))
		prob += 5;	// Add by Alez - Improove in hide stab probability 

 	if (GET_MOB_HOLD(vict))
		prob = prob * 5 / 4;
	if (GET_GOD_FLAG(vict, GF_GODSCURSE))
		prob = percent;
	if (GET_GOD_FLAG(vict, GF_GODSLIKE) || GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;

	if (percent > prob)
		damage(ch, vict, 0, SKILL_BACKSTAB + TYPE_HIT, TRUE);
	else
		hit(ch, vict, SKILL_BACKSTAB, 1);
	set_wait(ch, 2, TRUE);
}


ACMD(do_backstab)
{
	CHAR_DATA *vict;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char("���� �� ��� ������ ����������, ��� ������ �������� ?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("��, �����������, ������������ !\r\n", ch);
		return;
	}

	if (!GET_EQ(ch, WEAR_WIELD)) {
		send_to_char("��������� ������� ������ � ������ ����.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
		send_to_char("�������� ����� ������ ������ ������� !\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT)
	    || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (FIGHTING(vict) && GET_CLASS(ch) != CLASS_THIEF) {
		send_to_char("���� ���� ������� ������ �������� - �� ������ ���������� !\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	go_backstab(ch, vict);
}


/******************* CHARM ORDERS PROCEDURES */
ACMD(do_order)
{
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	bool found = FALSE;
	room_rnum org_room;
	CHAR_DATA *vict;
	struct follow_type *k, *k_next;

	half_chop(argument, name, message);
	if (GET_GOD_FLAG(ch, GF_GODSCURSE)) {
		send_to_char("�� �������� ������ � ����� �� ��������� ���!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_SIELENCE)) {
		send_to_char("�� �� � ��������� ����������� ������.\r\n", ch);
		return;
	}
	if (!*name || !*message)
		send_to_char("��������� ��� � ���� ?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) &&
		 !is_abbrev(name, "followers") && !is_abbrev(name, "���") && !is_abbrev(name, "����"))
		send_to_char("�� �� ������ ������ ���������.\r\n", ch);
	else if (ch == vict)
		send_to_char("�� ������ ������� ������������ ������ - ������ � ��������� !\r\n", ch);
	else {
		if (vict && !IS_NPC(vict) && !IS_GOD(ch)) {
			send_to_char(ch, "������� ����������� ����� ������ ����!.\r\n");
			return;
		}
		if (AFF_FLAGGED(ch, AFF_CHARM)) {
			send_to_char("� ����� ��������� �� �� ������ ���� �������� �������.\r\n", ch);
			return;
		}
		if (vict && !is_abbrev(name, "���") && !is_abbrev(name, "����")
		    && !is_abbrev(name, "followers")) {
			sprintf(buf, "$N ��������$g ��� '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR | CHECK_DEAF);
			act("$n �����$g ������ $N2.", FALSE, ch, 0, vict, TO_ROOM | CHECK_DEAF);

			if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM)
			    || AFF_FLAGGED(vict, AFF_DEAFNESS))
				act("$n ����������� ������� �� ��������.", FALSE, vict, 0, 0, TO_ROOM);
			else {
				send_to_char(OK, ch);
				if (GET_WAIT_STATE(vict) <= 0)
					command_interpreter(vict, message);
				else if (FIGHTING(vict)) {
					if (vict->last_comm != NULL)
						free(vict->last_comm);
					vict->last_comm = str_dup(message);
				}

			}
		} else {	/* This is order "followers" */
			org_room = ch->in_room;
			act("$n �����$g ������.", FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF);

			for (k = ch->followers; k; k = k_next) {
				k_next = k->next;
				if (org_room == k->follower->in_room)
					if (AFF_FLAGGED(k->follower, AFF_CHARM)
					    && !AFF_FLAGGED(k->follower, AFF_DEAFNESS)) {
						found = TRUE;
						if (GET_WAIT_STATE(k->follower) <= 0)
							command_interpreter(k->follower, message);
						else if (FIGHTING(k->follower)) {
							if (k->follower->last_comm != NULL)
								free(k->follower->last_comm);
							k->follower->last_comm = str_dup(message);
						}
					}
			}
			if (found)
				send_to_char(OK, ch);
			else
				send_to_char("�� ��������� ������ �������!\r\n", ch);
		}
	}
}

/********************** FLEE PROCEDURE */
void go_flee(CHAR_DATA * ch)
{
	int i, attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);
	CHAR_DATA *was_fighting;

	if (on_horse(ch) && GET_POS(get_horse(ch)) >= POS_FIGHTING && !GET_MOB_HOLD(get_horse(ch))) {
		if (!WAITLESS(ch))
			WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		while (scandirs != (1 << NUM_OF_DIRS) - 1) {
			attempt = number(0, NUM_OF_DIRS - 1);
			if (IS_SET(scandirs, (1 << attempt)))
				continue;
			SET_BIT(scandirs, (1 << attempt));
			if (!legal_dir(ch, attempt, TRUE, FALSE) ||
			    ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH))
				continue;
			/*����� ��������, ����� �� ������� ��
			   ������ � ������� � � ������� � ������ !������ */
			if (ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_TUNNEL) ||
			    (ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_NOHORSE)))
				continue;
			was_fighting = FIGHTING(ch);
			if (do_simple_move(ch, attempt | 0x80, TRUE, 0)) {
				act("����$W $N �����$Q ��� �� ���.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
				if (was_fighting && !IS_NPC(ch)) {
					loss = MAX(1, GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting));
					loss *= GET_LEVEL(was_fighting);
					if (!IS_THIEF(ch) && !IS_MERCHANT(ch) && !ROOM_FLAGGED(was_in, ROOM_ARENA))
						gain_exp(ch, -loss);
				}
				return;
			}
		}
		send_to_char("������ �������� ����. �� �� ������ ������� !\r\n", ch);
		return;
	}

	if (GET_MOB_HOLD(ch))
		return;
	if (AFF_FLAGGED(ch, AFF_NOFLEE)) {
		send_to_char("��������� ����� ������ ��� �������.\r\n", ch);
		return;
	}
	if (GET_WAIT(ch) > 0)
		return;
	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("�� �� ������ ������� �� ����� ���������.\r\n", ch);
		return;
	}
	if (!WAITLESS(ch))
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	for (i = 0; i < 6; i++) {
		attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
		if (legal_dir(ch, attempt, TRUE, FALSE) && !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
			act("$n �����������$g � �����$u ������� !", TRUE, ch, 0, 0, TO_ROOM);
			was_fighting = FIGHTING(ch);
			if ((do_simple_move(ch, attempt | 0x80, TRUE, 0))) {
				send_to_char("�� ������ ������� � ���� �����.\r\n", ch);
				if (was_fighting && !IS_NPC(ch)) {
					loss = MAX(1, GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting));
					loss *= GET_LEVEL(was_fighting);
					if (!IS_THIEF(ch) &&
					    !IS_MERCHANT(ch) && !IS_ASSASINE(ch) && !ROOM_FLAGGED(was_in, ROOM_ARENA))
						gain_exp(ch, -loss);
				}
			} else {
				act("$n �����������$g � �������$u �������, �� �� ����$q!", FALSE, ch, 0, 0, TO_ROOM);
				send_to_char("������ �������� ����. �� �� ������ ������� !\r\n", ch);
			}
			return;
		}
	}
	send_to_char("������ �������� ����. �� �� ������ ������� !\r\n", ch);
}


void go_dir_flee(CHAR_DATA * ch, int direction)
{
	int attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);
	CHAR_DATA *was_fighting;

	if (GET_MOB_HOLD(ch))
		return;
	if (AFF_FLAGGED(ch, AFF_NOFLEE)) {
		send_to_char("��������� ����� ������ ��� �������.\r\n", ch);
		return;
	}
	if (GET_WAIT(ch) > 0)
		return;
	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("�� �� ������� ������� �� ����� ���������.\r\n", ch);
		return;
	}

	if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

	while (scandirs != (1 << NUM_OF_DIRS) - 1) {
		attempt = direction >= 0 ? direction : number(0, NUM_OF_DIRS - 1);
		direction = -1;
		if (IS_SET(scandirs, (1 << attempt)))
			continue;
		SET_BIT(scandirs, (1 << attempt));
		if (!legal_dir(ch, attempt, TRUE, FALSE) || ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH))
			continue;
		/*����� ��������, ����� �� ������� �� ������ � �������
		   � � ������� � ������ !������ */
		if (ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_TUNNEL) ||
		    (ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_NOHORSE)))
			if (on_horse(ch))
				continue;
		act("$n �����������$g � �������$u �������.", FALSE, ch, 0, 0, TO_ROOM);
		was_fighting = FIGHTING(ch);
		if (do_simple_move(ch, attempt | 0x80, TRUE, 0)) {
			send_to_char("�� ������ ������� � ���� �����.\r\n", ch);
			if (was_fighting && !IS_NPC(ch)) {
				loss = GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
				loss *= GET_LEVEL(was_fighting);
				if (!IS_THIEF(ch) && !IS_MERCHANT(ch) && !ROOM_FLAGGED(was_in, ROOM_ARENA))
					gain_exp(ch, -loss);
			}
			return;
		} else
			send_to_char("������ �������� ���� ! �� �� ������ c������.\r\n", ch);
	}
	send_to_char("������ �������� ���� ! �� �� ������ c������.\r\n", ch);
}


const char *FleeDirs[] = { "�����",
	"������",
	"��",
	"�����",
	"�����",
	"����",
	"\n"
};

ACMD(do_flee)
{
	int direction = -1;
	if (!FIGHTING(ch)) {
		send_to_char("�� �� ���� �� � ��� �� ���������� !\r\n", ch);
		return;
	}
	if (IS_THIEF(ch) || IS_MERCHANT(ch) || IS_IMMORTAL(ch)
	    || GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		one_argument(argument, arg);
		if ((direction = search_block(arg, dirs, FALSE)) >= 0 ||
		    (direction = search_block(arg, FleeDirs, FALSE)) >= 0) {
			go_dir_flee(ch, direction);
			return;
		}
	}
	go_flee(ch);
}

/************************** BASH PROCEDURES */
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent = 0, prob;

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_STOPLEFT)
	    || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if (!(IS_NPC(ch) ||	// ���
	      GET_EQ(ch, WEAR_SHIELD) ||	// ���� ���
	      IS_IMMORTAL(ch) ||	// �����������
	      GET_MOB_HOLD(vict) ||	// ���� ���������
	      GET_GOD_FLAG(vict, GF_GODSCURSE)	// ���� ��������
	    )) {
		send_to_char("�� �� ������ ������� ����� ��� ����.\r\n", ch);
		return;
	};

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("��� ����� ������ �� ����.\r\n", ch);
		return;
	}

	vict = try_protect(vict, ch, SKILL_BASH);

	percent = number(1, skill_info[SKILL_BASH].max_percent);
	prob = train_skill(ch, SKILL_BASH, skill_info[SKILL_BASH].max_percent, vict);
	if (PRF_FLAGGED(ch, PRF_AWAKE))
		prob /= 2;
	if (GET_GOD_FLAG(ch, GF_GODSLIKE) || GET_MOB_HOLD(vict))
		prob = percent;
	if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		prob = percent;
	if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;
	if (MOB_FLAGGED(vict, MOB_NOBASH))
		prob = 0;


	if (percent > prob) {
		damage(ch, vict, 0, SKILL_BASH + TYPE_HIT, TRUE);
		GET_POS(ch) = POS_SITTING;
		prob = 3;
	} else {
		/*
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */
		int dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) +
		    MAX(0, GET_SKILL(ch, SKILL_BASH) / 10 - 5) + GET_LEVEL(ch) / 5;
//      log("[BASH params] = actor = %s, actorlevel = %d, actordex = %d
//           target=  %s, targetlevel = %d, targetdex = %d ,skill = %d,
//           dice = %d, dam = %d", GET_NAME(ch), GET_LEVEL(ch), GET_REAL_DEX(ch),
//         GET_NAME(vict), GET_LEVEL(vict), GET_REAL_DEX(vict),
//         percent, prob, dam);         
//������ ������������ ����
		if (GET_AF_BATTLE(vict, EAF_BLOCK) &&
		    !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
		    !AFF_FLAGGED(vict, AFF_MAGICSTOPFIGHT) &&
		    !AFF_FLAGGED(vict, AFF_STOPLEFT) && GET_WAIT(vict) <= 0 && GET_MOB_HOLD(vict) == 0) {
			if (!(GET_EQ(vict, WEAR_SHIELD) ||
			      IS_NPC(vict) || IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE)))
				send_to_char("� ��� ����� �������� ����� ����������.\r\n", vict);
			else {
				int range, prob2;
				range = number(1, skill_info[SKILL_BLOCK].max_percent);
				prob2 = train_skill(vict, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
				if (prob2 < range) {
					act("�� �� ������ ����������� ������� $N1 ����� ���.",
					    FALSE, vict, 0, ch, TO_CHAR);
					act("$N �� ����$Q ����������� ���� ������� ����� $S.",
					    FALSE, ch, 0, vict, TO_CHAR);
					act("$n �� ����$q ����������� ������� $N1 ����� $s.",
					    TRUE, vict, 0, ch, TO_NOTVICT);
				} else {
					act("�� ����������� ������� $N1 ����� ��� � ���.", FALSE, vict, 0, ch, TO_CHAR);
					act("�� ������ ����� $N1, �� ��$G ����������$G ���� �������.",
					    FALSE, ch, 0, vict, TO_CHAR);
					act("$n ����������$g ������� $N1 ����� $s.", TRUE, vict, 0, ch, TO_NOTVICT);
					alt_equip(vict, WEAR_SHIELD, 30, 10);
					return;
				}
			}
		}
//������ ������������ ����

		prob = 0;
		dam = damage(ch, vict, dam, SKILL_BASH + TYPE_HIT, FALSE);
		if (dam > 0 || (dam == 0 && AFF_FLAGGED(vict, AFF_SHIELD))) {	/* -1 = dead, 0 = miss */
			prob = 3;
			if (IN_ROOM(ch) == IN_ROOM(vict)) {
				GET_POS(vict) = POS_SITTING;
				if (on_horse(vict)) {
					act("�� ����� � $N1.", FALSE, vict, 0, get_horse(vict), TO_CHAR);
					REMOVE_BIT(AFF_FLAGS(vict, AFF_HORSE), AFF_HORSE);
				}
				if (IS_HORSE(vict) && on_horse(vict->master))
					horse_drop(vict);
			}
			set_wait(vict, prob, FALSE);
			prob = 2;
		}
	}
	set_wait(ch, prob, TRUE);
}


ACMD(do_bash)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("���� �� �� ��� ������ ������� ����� ?\r\n", ch);
			return;
		}
	}

	if (vict == ch) {
		send_to_char("��� ����������� ���� ������ ��� ������... �� ������������� ���� �����.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !FIGHTING(ch))
		go_bash(ch, vict);
	else if (!used_attack(ch)) {
		act("������. �� ����������� ����� $N3.", FALSE, ch, 0, vict, TO_CHAR);
		SET_EXTRA(ch, SKILL_BASH, vict);
	}
}

/******************** RESCUE PROCEDURES */
void go_rescue(CHAR_DATA * ch, CHAR_DATA * vict, CHAR_DATA * tmp_ch)
{
	int percent, prob;

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	percent = number(1, skill_info[SKILL_RESCUE].max_percent);
	prob = calculate_skill(ch, SKILL_RESCUE, skill_info[SKILL_RESCUE].max_percent, tmp_ch);
	improove_skill(ch, SKILL_RESCUE, prob >= percent, tmp_ch);

	if (GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		prob = 0;

	if (percent != skill_info[SKILL_RESCUE].max_percent && percent > prob) {
		act("�� ���������� �������� ������ $N3", FALSE, ch, 0, vict, TO_CHAR);
		set_wait(ch, 1, FALSE);
		return;
	}

	act("����� �����, �� ���������� ������ $N3 !", FALSE, ch, 0, vict, TO_CHAR);
	act("�� ���� ������� $N4. �� ���������� ���� �����!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n ���������� ����$q $N3!", TRUE, ch, 0, vict, TO_NOTVICT);

	if (FIGHTING(vict) == tmp_ch)
		stop_fighting(vict, FALSE);
//  if (FIGHTING(tmp_ch))
//     stop_fighting(tmp_ch,FALSE);
//  if (FIGHTING(ch))
//     stop_fighting(ch,FALSE);

	pk_agro_action(ch, tmp_ch);

//  set_fighting(ch, tmp_ch);
//  set_fighting(tmp_ch, ch);
	if (FIGHTING(ch))
		FIGHTING(ch) = tmp_ch;
	else
		set_fighting(ch, tmp_ch);
	if (FIGHTING(tmp_ch))
		FIGHTING(tmp_ch) = ch;
	else
		set_fighting(tmp_ch, ch);
	set_wait(ch, 1, FALSE);
	set_wait(vict, 2, FALSE);
}

ACMD(do_rescue)
{
	CHAR_DATA *vict, *tmp_ch;

	if (!GET_SKILL(ch, SKILL_RESCUE)) {
		send_to_char("�� �� �� ������ ���.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char("���� �� ������ ������ ?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("���� �������� �� ������ �������� ������ �����.\r\n", ch);
		return;
	}
	if (FIGHTING(ch) == vict) {
		send_to_char("�� ��������� ������ ���������� ��� ?\r\n" "��� �� � ��� �� ������ ���� � ���� ?\r\n", ch);
		return;
	}

	for (tmp_ch = world[ch->in_room]->people; tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

	if (!tmp_ch) {
		act("�� ����� �� ��������� � $N4!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
//  if (!IS_NPC(FIGHTING(vict)) && !IS_NPC(ch) && IS_NPC(vict))
//     {send_to_char("�� ��������� ������ ������ ����������.\r\n", ch);
//      return;
//     }

	if (IS_NPC(vict) && tmp_ch && (!IS_NPC(tmp_ch) || (AFF_FLAGGED(tmp_ch, AFF_CHARM)
							   && tmp_ch->master && !IS_NPC(tmp_ch->master))) &&
	    (!IS_NPC(ch)
	     || (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master)))) {
		send_to_char("�� ��������� ������ ������ ����������.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, tmp_ch))
		return;

	go_rescue(ch, vict, tmp_ch);
}

/*******************  KICK PROCEDURES */
void go_kick(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	vict = try_protect(vict, ch, SKILL_KICK);

	/* 101% is a complete failure */
	percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, skill_info[SKILL_KICK].max_percent);
	prob = train_skill(ch, SKILL_KICK, skill_info[SKILL_KICK].max_percent, vict);
	if (GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_MOB_HOLD(vict) > 0)
		prob = percent;
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) || on_horse(vict))
		prob = 0;

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_KICK + TYPE_HIT, TRUE);
		prob = 2;
	} else {
		int dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) + GET_LEVEL(ch) / 6;
//      int dam = str_app[GET_REAL_STR(ch)].todam + (IS_NPC(ch) ? 0 : GET_REAL_DR(ch)) + GET_LEVEL(ch)/6;
		// ����������� �� ���� ����� �� ������ ��� PC (�������� �����������):
		//  0 -  50%
		//  5 -  75%
		// 10 - 100%
		// 20 - 150%
		// 30 - 200%
//      if ( !IS_NPC(ch) ){
		if (!IS_NPC(ch) || (IS_NPC(ch) && GET_EQ(ch, WEAR_FEET))) {
			int modi = MAX(0, (GET_SKILL(ch, SKILL_KICK) + 4) / 5);
			dam += number(0, modi * 2);
			modi = 5 * (10 + (GET_EQ(ch, WEAR_FEET) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_FEET)) : 0));
			dam = modi * dam / 100;
		}
//      log("[KICK damage] Name==%s dam==%d",GET_NAME(ch),dam);
		if (AFF_FLAGGED(ch, EAF_AWAKE))
			dam >>= 2;	// � 4 ���� ������
//+++++
// ��� ����� ���������, ������ ����� �������� ������������ ���� �������
// ����� �� fight.c
		// Include a damage multiplier if victim isn't ready to fight:
		// Position sitting  1.5 x normal
		// Position resting  2.0 x normal
		// Position sleeping 2.5 x normal
		// Position stunned  3.0 x normal
		// Position incap    3.5 x normal
		// Position mortally 4.0 x normal
		//
		// Note, this is a hack because it depends on the particular
		// values of the POSITION_XXX constants.
		//
		if (GET_POS(ch) < POS_FIGHTING)
			dam -= (dam * (POS_FIGHTING - GET_POS(ch)) / 4);

		if (GET_POS(vict) == POS_SITTING &&
		    (AFF_FLAGGED(vict, AFF_AIRSHIELD) ||
		     AFF_FLAGGED(vict, AFF_FIRESHIELD) || AFF_FLAGGED(vict, AFF_ICESHIELD))) {
			// ������ ����� � ����, ����������� �� ��������
		} else if (GET_POS(vict) < POS_FIGHTING)
			dam += (dam * (POS_FIGHTING - GET_POS(vict)) / 3);

		if (GET_MOB_HOLD(vict))
			dam += (dam >> 1);

		// Cut damage in half if victim has sanct, to a minimum 1
		if (AFF_FLAGGED(vict, AFF_PRISMATICAURA))
			dam *= 2;
		if (AFF_FLAGGED(vict, AFF_SANCTUARY) && dam >= 2)
			dam /= 2;

//---
		damage(ch, vict, dam, SKILL_KICK + TYPE_HIT, TRUE);
		prob = 2;
	}
	set_wait(ch, prob, TRUE);
}

ACMD(do_kick)
{
	CHAR_DATA *vict = NULL;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	one_argument(argument, arg);
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("��� ��� ��� ������ �������� ��� ������ ������ ?\r\n", ch);
			return;
		}
	}
	if (vict == ch) {
		send_to_char("�� ������ ����� ���� ! ����������, �� ����� � ���...\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !FIGHTING(ch))
		go_kick(ch, vict);
	else if (!used_attack(ch)) {
		act("������. �� ����������� ����� $N3.", FALSE, ch, 0, vict, TO_CHAR);
		SET_EXTRA(ch, SKILL_KICK, vict);
	}
}

/******************** BLOCK PROCEDURES */
void go_block(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_STOPLEFT)) {
		send_to_char("���� ���� ������������.\r\n", ch);
		return;
	}
	SET_AF_BATTLE(ch, EAF_BLOCK);
	send_to_char("������, �� ���������� �������� ����� ��������� �����.\r\n", ch);
}

ACMD(do_block)
{
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BLOCK)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}
	if (!FIGHTING(ch)) {
		send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
		return;
	};
	if (!(IS_NPC(ch) ||	// ���
	      GET_EQ(ch, WEAR_SHIELD) ||	// ���� ���
	      IS_IMMORTAL(ch) ||	// �����������
	      GET_GOD_FLAG(ch, GF_GODSLIKE)	// ��������
	    )) {
		send_to_char("�� �� ������ ������� ��� ��� ����.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
		send_to_char("�� ��� ������������� ����� !\r\n", ch);
		return;
	}
	go_block(ch);
}

/***************** MULTYPARRY PROCEDURES */
void go_multyparry(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
	    AFF_FLAGGED(ch, AFF_STOPLEFT) || AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, EAF_MULTYPARRY);
	send_to_char("�� ���������� ������������ ������� ������.\r\n", ch);
}

ACMD(do_multyparry)
{
	OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD), *offhand = GET_EQ(ch, WEAR_HOLD);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MULTYPARRY)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}
	if (!FIGHTING(ch)) {
		send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
		return;
	}
	if (!(IS_NPC(ch) ||	// ���
	      (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON && offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON) ||	// ��� ������
	      IS_IMMORTAL(ch) ||	// �����������
	      GET_GOD_FLAG(ch, GF_GODSLIKE)	// ��������
	    )) {
		send_to_char("�� �� ������ �������� ����� ����������.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
		send_to_char("���������� ! �� ���������� �������� ����������.\r\n", ch);
		return;
	}
	go_multyparry(ch);
}




/***************** PARRY PROCEDURES */
void go_parry(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
	    AFF_FLAGGED(ch, AFF_STOPLEFT) || AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, EAF_PARRY);
	send_to_char("�� ���������� ��������� ��������� �����.\r\n", ch);
}

ACMD(do_parry)
{
	OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD), *offhand = GET_EQ(ch, WEAR_HOLD);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PARRY)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}
	if (!FIGHTING(ch)) {
		send_to_char("�� �� �� � ��� �� ���������� ?\r\n", ch);
		return;
	}
	if (!(IS_NPC(ch) ||	// ���
	      (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON && offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON) ||	// ��� ������
	      IS_IMMORTAL(ch) ||	// �����������
	      GET_GOD_FLAG(ch, GF_GODSLIKE)	// ��������
	    )) {
		send_to_char("�� �� ������ ��������� ����� ����������.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
		send_to_char("���������� ! �� ���������� �������� ����������.\r\n", ch);
		return;
	}
	go_parry(ch);
}

/*************** PROTECT PROCEDURES */
void go_protect(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	PROTECTING(ch) = vict;
	act("�� ����������� �������� $N1 �� ��������� �����.", FALSE, ch, 0, vict, TO_CHAR);
	SET_AF_BATTLE(ch, EAF_PROTECT);
}

ACMD(do_protect)
{
	CHAR_DATA *vict, *tch;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PROTECT)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char("� ��� ��� ������ ��� ������ ������ ?\r\n", ch);
		return;
	};

	for (tch = world[IN_ROOM(ch)]->people; tch; tch = tch->next_in_room)
		if (FIGHTING(tch) == vict)
			break;

	if (!tch) {
		act("�� $N3 ����� �� �������.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (vict == ch) {
		send_to_char("���������� ���������� ����� ��� ���������� �����.\r\n", ch);
		return;
	}

	if (FIGHTING(ch) == vict) {
		send_to_char("�� ���� ��������, ��� ��������.\r\n", ch);
		return;
	}

	if (IS_NPC(vict) && tch && (!IS_NPC(tch) || (AFF_FLAGGED(tch, AFF_CHARM)
						     && tch->master && !IS_NPC(tch->master))) && (!IS_NPC(ch)
												  ||
												  (AFF_FLAGGED
												   (ch, AFF_CHARM)
												   && ch->master
												   && !IS_NPC(ch->
													      master))))
	{
		send_to_char("�� ��������� �������� ������ ����������.\r\n", ch);
		return;
	}

	for (tch = world[IN_ROOM(ch)]->people; tch; tch = tch->next_in_room) {
		if (FIGHTING(tch) == vict && !may_kill_here(ch, tch))
			return;
	}
	go_protect(ch, vict);
}

/************** TOUCH PROCEDURES */
void go_touch(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}
	act("�� ����������� ����������� ��������� ����� $N1.", FALSE, ch, 0, vict, TO_CHAR);
	SET_AF_BATTLE(ch, EAF_TOUCH);
	TOUCHING(ch) = vict;
}

ACMD(do_touch)
{
	OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch,
										     WEAR_BOTHS);
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TOUCH)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}
	if (!(IS_IMMORTAL(ch) ||	// �����������
	      IS_NPC(ch) ||	// ���
	      GET_GOD_FLAG(ch, GF_GODSLIKE) ||	// ��������
	      !primary		// ��� ������
	    )) {
		send_to_char("� ��� ������ ����.\r\n", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next_in_room)
			if (FIGHTING(vict) == ch)
				break;
		if (!vict) {
			if (!FIGHTING(ch)) {
				send_to_char("�� �� �� � ��� �� ����������.\r\n", ch);
				return;
			} else
				vict = FIGHTING(ch);
		}
	}

	if (ch == vict) {
		send_to_char(GET_NAME(ch), ch);
		send_to_char(", �� ������ �� �������, �������� ����������� �����.\r\n", ch);
		return;
	}
	if (FIGHTING(vict) != ch && FIGHTING(ch) != vict) {
		act("�� �� �� ���������� � $N4.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_AF_BATTLE(ch, EAF_MIGHTHIT)) {
		send_to_char("����������. �� ������������� � ������������ �����.\r\n", ch);
		return;
	}

	if (check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_touch(ch, vict);
}

/************** DEVIATE PROCEDURES */
void go_deviate(CHAR_DATA * ch)
{
	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}
	if (onhorse(ch))
		return;
	SET_AF_BATTLE(ch, EAF_DEVIATE);
	send_to_char("������, �� ����������� ���������� �� ��������� ����� !\r\n", ch);
}

ACMD(do_deviate)
{
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DEVIATE)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(FIGHTING(ch))) {
		send_to_char("�� �� ���� �� � ��� �� ���������� !\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if (GET_AF_BATTLE(ch, EAF_DEVIATE)) {
		send_to_char("�� � ��� ���������, ��� ������.\r\n", ch);
		return;
	};
	go_deviate(ch);
}

/************** DISARM PROCEDURES */
void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob, pos = 0;
	OBJ_DATA *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
	    GET_EQ(vict, WEAR_BOTHS), *helded = GET_EQ(vict, WEAR_HOLD);

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}
// shapirus: ������ ���������� ����� ���, ����� �����
	if (!((wielded && GET_OBJ_TYPE(wielded) != ITEM_LIGHT) || (helded && GET_OBJ_TYPE(helded) != ITEM_LIGHT)))
		return;
	if (number(1, 100) > 30)
		pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
	else
		pos = helded ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);

	if (!pos || !GET_EQ(vict, pos))
		return;

	percent = number(1, skill_info[SKILL_DISARM].max_percent);
	prob = train_skill(ch, SKILL_DISARM, skill_info[SKILL_DISARM].max_percent, vict);
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, GF_GODSCURSE)
	    || GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSLIKE))
		prob = 0;


	if (percent > prob || OBJ_FLAGGED(GET_EQ(vict, pos), ITEM_NODISARM)) {
		sprintf(buf, "%s�� �� ������ ����������� %s...%s\r\n",
			CCWHT(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		// act("�� �� ������ ����������� $N1 !",FALSE,ch,0,vict,TO_CHAR);
		prob = 3;
	} else {
		wielded = unequip_char(vict, pos);
		sprintf(buf, "%s�� ����� ������ %s �� ��� %s...%s\r\n",
			CCIBLU(ch, C_NRM), wielded->PNames[3], GET_PAD(vict, 1), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		// act("�� ����� ������ $o3 �� ��� $N1.",FALSE,ch,wielded,vict,TO_CHAR);
		act("$n ����� �����$g $o3 �� ����� ���.", FALSE, ch, wielded, vict, TO_VICT);
		act("$n ����� �����$g $o3 �� ��� $N1.", TRUE, ch, wielded, vict, TO_NOTVICT);
		set_wait(vict, IS_NPC(vict) ? 1 : 2, FALSE);
		prob = 2;
		if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
			obj_to_char(wielded, vict);
		else {
			obj_to_room(wielded, IN_ROOM(vict));
			obj_decay(wielded);
		}
	}

	pk_agro_action(ch, vict);

	appear(ch);
	if (IS_NPC(vict) && CAN_SEE(vict, ch) && have_mind(vict) && GET_WAIT(ch) <= 0) {
		set_hit(vict, ch);
	}
	set_wait(ch, prob, FALSE);
}

ACMD(do_disarm)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DISARM)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("���� ������������� ?\r\n", ch);
			return;
		}
	};

	if (ch == vict) {
		send_to_char(GET_NAME(ch), ch);
		send_to_char(", ���������� ������� \"����� <��������.������>\".\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

// shapirus: ������ ���������� ����� ���, ����� �����
	if (!((GET_EQ(vict, WEAR_WIELD)
	       && GET_OBJ_TYPE(GET_EQ(vict, WEAR_WIELD)) != ITEM_LIGHT)
	      || (GET_EQ(vict, WEAR_HOLD)
		  && GET_OBJ_TYPE(GET_EQ(vict, WEAR_HOLD)) != ITEM_LIGHT)
	      || (GET_EQ(vict, WEAR_BOTHS)
		  && GET_OBJ_TYPE(GET_EQ(vict, WEAR_BOTHS)) != ITEM_LIGHT))) {
		send_to_char("�� �� ������ ����������� ���������� ��������.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !FIGHTING(ch))
		go_disarm(ch, vict);
	else if (!used_attack(ch)) {
		act("������. �� ����������� ���������� $N3.", FALSE, ch, 0, vict, TO_CHAR);
		SET_EXTRA(ch, SKILL_DISARM, vict);
	}
}

/************************** CHOPOFF PROCEDURES */
void go_chopoff(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}
	if (onhorse(ch))
		return;

	/* ������� �����: coded by ����� */
	if ((GET_POS(vict) < POS_FIGHTING)) {
		if (number(1, 100) < GET_SKILL(ch, SKILL_CHOPOFF)) {
			send_to_char("�� ������������� �������� ��������, �� ������� ������������.\r\n", ch);
			set_wait(ch, 1, FALSE);
			return;
		}
	}

	percent = number(1, skill_info[SKILL_CHOPOFF].max_percent);
	prob = train_skill(ch, SKILL_CHOPOFF, skill_info[SKILL_CHOPOFF].max_percent, vict);

	if (GET_GOD_FLAG(ch, GF_GODSLIKE) || GET_MOB_HOLD(vict) > 0 || GET_GOD_FLAG(vict, GF_GODSCURSE))
		prob = percent;

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
	    GET_GOD_FLAG(vict, GF_GODSLIKE) ||
	    on_horse(vict) || GET_POS(vict) < POS_FIGHTING || MOB_FLAGGED(vict, MOB_NOTRIP) || IS_IMMORTAL(vict))
		prob = 0;

	if (percent > prob) {
		sprintf(buf, "%s�� ���������� ������� %s, �� ����� ����...%s\r\n",
			CCWHT(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		// act("�� ���������� ������� $N3, �� ����� ����...",FALSE,ch,0,vict,TO_CHAR);
		act("$n �������$u ������� ���, �� ����$g ���$g.", FALSE, ch, 0, vict, TO_VICT);
		act("$n �������$u ������� $N3, �� ����$g ���$g.", TRUE, ch, 0, vict, TO_NOTVICT);

		GET_POS(ch) = POS_SITTING;
		prob = 3;
	} else {
		sprintf(buf, "%s�� ������� ��������, ����� ������ %s �� �����.%s\r\n",
			CCIBLU(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		// act("�� ����� �������� $N3, ������ $S �� �����.",FALSE,ch,0,vict,TO_CHAR);
		act("$n ����� ������$q ���, ������ �� ����.", FALSE, ch, 0, vict, TO_VICT);
		act("$n ����� ������$q $N3, ������ $S �� �����.", TRUE, ch, 0, vict, TO_NOTVICT);
		set_wait(vict, 3, FALSE);
		if (IN_ROOM(ch) == IN_ROOM(vict))
			GET_POS(vict) = POS_SITTING;
		if (IS_HORSE(vict) && on_horse(vict->master))
			horse_drop(vict);
		prob = 1;
	}

	pk_agro_action(ch, vict);

	appear(ch);
	if (IS_NPC(vict) && CAN_SEE(vict, ch) && have_mind(vict) && GET_WAIT_STATE(vict) <= 0)
		set_hit(vict, ch);

	set_wait(ch, prob, FALSE);
}


ACMD(do_chopoff)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_CHOPOFF)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (onhorse(ch))
		return;

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("���� �� ����������� ������� ?\r\n", ch);
			return;
		}
	}

	if (vict == ch) {
		send_to_char("�� ������ ��������������� �������� <�����>.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !FIGHTING(ch))
		go_chopoff(ch, vict);
	else if (!used_attack(ch)) {
		act("������. �� ����������� ������� $N3.", FALSE, ch, 0, vict, TO_CHAR);
		SET_EXTRA(ch, SKILL_CHOPOFF, vict);
	}
}

/************************** STUPOR PROCEDURES */
void go_stupor(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (!FIGHTING(ch)) {
		SET_AF_BATTLE(ch, EAF_STUPOR);
		hit(ch, victim, TYPE_NOPARRY, 1);
		set_wait(ch, 2, TRUE);
	} /* else if ((FIGHTING(victim) != ch) && (FIGHTING(ch) != victim))
		act("$N �� ��������� � ����, �� �������� $S.", FALSE, ch, 0, victim, TO_CHAR); */
	else {
		act("�� ����������� �������� $N3.", FALSE, ch, 0, victim, TO_CHAR);
		if (FIGHTING(ch) != victim) {
			stop_fighting(ch, FALSE);
			set_fighting(ch, victim);
			set_wait(ch, 2, TRUE);
		}
		SET_AF_BATTLE(ch, EAF_STUPOR);
	}
}

ACMD(do_stupor)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STUPOR)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("���� �� ������ �������� ?\r\n", ch);
			return;
		}
	}

	if (vict == ch) {
		send_to_char("�� ������ �������, �������� ���� ����������� �����.\r\n", ch);
		return;
	}
//  if (GET_AF_BATTLE(ch, EAF_PARRY))
//     {send_to_char("����������. �� ������������� �� ���������� ����.\r\n", ch);
//      return;
//     }
	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	vict = try_protect(vict, ch, SKILL_STUPOR);

	go_stupor(ch, vict);
}

/************************** MIGHTHIT PROCEDURES */
void go_mighthit(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (!FIGHTING(ch)) {
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
		hit(ch, victim, TYPE_NOPARRY, 1);
		set_wait(ch, 2, TRUE);
	} else if ((FIGHTING(victim) != ch) && (FIGHTING(ch) != victim))
		act("$N �� ��������� � ����, �� �������� $S.", FALSE, ch, 0, victim, TO_CHAR);
	else {
		act("�� ����������� ������� ����������� ���� �� $N2.", FALSE, ch, 0, victim, TO_CHAR);
		if (FIGHTING(ch) != victim) {
			stop_fighting(ch, FALSE);
			set_fighting(ch, victim);
			set_wait(ch, 2, TRUE);
		}
		SET_AF_BATTLE(ch, EAF_MIGHTHIT);
	}
}

ACMD(do_mighthit)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MIGHTHIT)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("���� �� ������ ������ ������� ?\r\n", ch);
			return;
		}
	}

	if (vict == ch) {
		send_to_char("�� ������ ������� ����. �� �� � �� �����.\r\n", ch);
		return;
	}

	if (GET_AF_BATTLE(ch, EAF_TOUCH)) {
		send_to_char("����������. �� ������������� �� ������� ����������.\r\n", ch);
		return;
	}
	if (!(IS_NPC(ch) || IS_IMMORTAL(ch)) &&
	    (GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_WIELD) ||
	     GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_SHIELD) || GET_EQ(ch, WEAR_LIGHT))) {
		send_to_char("���� ���������� ������ ��� ������� ����.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	vict = try_protect(vict, ch, SKILL_MIGHTHIT);

	go_mighthit(ch, vict);
}

const char *cstyles[] = { "normal",
	"�������",
	"punctual",
	"������",
	"awake",
	"����������",
	"powerattack",
	"�����������",
	"grandpowerattack",
	"���������������������",
	"aimattack",
	"���������������",
	"grandaimattack",
	"�������������������������",
	"\n"
};

ACMD(do_style)
{
	int tp;

	one_argument(argument, arg);



	if (!*arg) {
		sprintf(buf, "�� ���������� %s ������.\r\n",
			IS_SET(PRF_FLAGS(ch, PRF_PUNCTUAL),
			       PRF_PUNCTUAL) ? "������" : IS_SET(PRF_FLAGS(ch,
									   PRF_AWAKE),
								 PRF_AWAKE) ? "����������" : "�������");
		send_to_char(buf, ch);
		return;
	}
	if (((tp = search_block(arg, cstyles, FALSE)) == -1)) {
		send_to_char("������: ����� { �������� ����� }\r\n", ch);
		return;
	}
	tp >>= 1;
	if ((tp == 1 && !GET_SKILL(ch, SKILL_PUNCTUAL)) || (tp == 2 && !GET_SKILL(ch, SKILL_AWAKE))) {
		send_to_char("��� ���������� ����� ����� ���.\r\n", ch);
		return;
	}
	if ((tp == 3 && !can_use_feat(ch, POWER_ATTACK_FEAT)) || (tp == 4 && !can_use_feat(ch, GREAT_POWER_ATTACK_FEAT))) {
		send_to_char("�� �� ������ ������������ ��� �����.\r\n", ch);
		return;
	}
	if ((tp == 5 && !can_use_feat(ch, AIMING_ATTACK_FEAT)) || (tp == 6 && !can_use_feat(ch, GREAT_AIMING_ATTACK_FEAT))) {
		send_to_char("�� �� ������ ������������ ��� �����.\r\n", ch);
		return;
	}
	switch (tp) {
	case 0:
	case 1:
	case 2:
		REMOVE_BIT(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_AWAKE), PRF_AWAKE);

		SET_BIT(PRF_FLAGS(ch, PRF_PUNCTUAL), PRF_PUNCTUAL * (tp == 1));
		SET_BIT(PRF_FLAGS(ch, PRF_AWAKE), PRF_AWAKE * (tp == 2));

		if (FIGHTING(ch) && !(AFF_FLAGGED(ch, AFF_COURAGE) ||
				      AFF_FLAGGED(ch, AFF_DRUNKED) || AFF_FLAGGED(ch, AFF_ABSTINENT))) {
			CLR_AF_BATTLE(ch, EAF_PUNCTUAL);
			CLR_AF_BATTLE(ch, EAF_AWAKE);
			if (tp == 1)
				SET_AF_BATTLE(ch, EAF_PUNCTUAL);
			else if (tp == 2)
				SET_AF_BATTLE(ch, EAF_AWAKE);
		}

		sprintf(buf, "�� ������� %s%s%s ����� ���.\r\n",
			CCRED(ch, C_SPR), tp == 0 ? "�������" : tp == 1 ? "������" : "����������", CCNRM(ch, C_OFF));
	break;
	case 3:
		REMOVE_BIT(PRF_FLAGS(ch, PRF_AIMINGATTACK), PRF_AIMINGATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATAIMINGATTACK), PRF_GREATAIMINGATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
		if (PRF_FLAGGED(ch, PRF_POWERATTACK)) {
			REMOVE_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
			sprintf(buf, "%s�� ���������� ������������ ������ �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		} else {
			SET_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
			sprintf(buf, "%s�� ������ ������������ ������ �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		}
	break;
	case 4:
		REMOVE_BIT(PRF_FLAGS(ch, PRF_AIMINGATTACK), PRF_AIMINGATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATAIMINGATTACK), PRF_GREATAIMINGATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
		if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK)) {
			REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
			sprintf(buf, "%s�� ���������� ������������ ���������� ������ �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		} else {
			SET_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
			sprintf(buf, "%s�� ������ ������������ ���������� ������ �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		}
	break;
	case 5:
		REMOVE_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATAIMINGATTACK), PRF_GREATAIMINGATTACK);
		if (PRF_FLAGGED(ch, PRF_AIMINGATTACK)) {
			REMOVE_BIT(PRF_FLAGS(ch, PRF_AIMINGATTACK), PRF_AIMINGATTACK);
			sprintf(buf, "%s�� ���������� ������������ ���������� �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		} else {
			SET_BIT(PRF_FLAGS(ch, PRF_AIMINGATTACK), PRF_AIMINGATTACK);
			sprintf(buf, "%s�� ������ ������������ ���������� �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		}
	break;
	case 6:
		REMOVE_BIT(PRF_FLAGS(ch, PRF_POWERATTACK), PRF_POWERATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATPOWERATTACK), PRF_GREATPOWERATTACK);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_AIMINGATTACK), PRF_AIMINGATTACK);
		if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK)) {
			REMOVE_BIT(PRF_FLAGS(ch, PRF_GREATAIMINGATTACK), PRF_GREATAIMINGATTACK);
			sprintf(buf, "%s�� ���������� ������������ ���������� ���������� �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		} else {
			SET_BIT(PRF_FLAGS(ch, PRF_GREATAIMINGATTACK), PRF_GREATAIMINGATTACK);
			sprintf(buf, "%s�� ������ ������������ ���������� ���������� �����.%s\r\n",
								CCIGRN(ch, C_SPR), CCNRM(ch, C_OFF));
		}
	break;
	}
	send_to_char(buf, ch);
	if (!WAITLESS(ch))
		WAIT_STATE(ch, PULSE_VIOLENCE);
}

/****************** STOPFIGHT ************************************/
ACMD(do_stopfight)
{
	CHAR_DATA *tmp_ch;

	if (!FIGHTING(ch) || IS_NPC(ch)) {
		send_to_char("�� �� �� �� � ��� �� ����������.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("�� ����� ��������� ��������� ����������.\r\n", ch);
		return;
	}

	for (tmp_ch = world[IN_ROOM(ch)]->people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
		if (FIGHTING(tmp_ch) == ch)
			break;

	if (tmp_ch) {
		send_to_char("����������, �� ���������� �� ���� �����.\r\n", ch);
		return;
	} else {
		stop_fighting(ch, TRUE);
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
			WAIT_STATE(ch, PULSE_VIOLENCE);
		send_to_char("�� ��������� �� �����.\r\n", ch);
		act("$n �����$g �� �����.", FALSE, ch, 0, 0, TO_ROOM);
	}
}

/************** THROW PROCEDURES */
void go_throw(CHAR_DATA * ch, CHAR_DATA * vict)
{
	int percent, prob;
	OBJ_DATA *wielded = GET_EQ(ch, WEAR_WIELD);

	if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_MAGICSTOPFIGHT)) {
		send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
		return;
	}

	if (!(wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)) {
		send_to_char("��� �� ������ ������� ?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !OBJ_FLAGGED(wielded, ITEM_THROWING)) {
		act("$o �� ������������$A ��� �������.", FALSE, ch, wielded, 0, TO_CHAR);
		return;
	}
	percent = number(1, skill_info[SKILL_THROW].max_percent);
	prob = train_skill(ch, SKILL_THROW, skill_info[SKILL_THROW].max_percent, vict);
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, GF_GODSCURSE)
	    || GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSLIKE))
		prob = 0;

	// log("Start throw");
	if (percent > prob)
		damage(ch, vict, 0, SKILL_THROW + TYPE_HIT, TRUE);
	else
		hit(ch, vict, SKILL_THROW, 1);
	// log("[THROW] Start extract weapon...");
	if (GET_EQ(ch, WEAR_WIELD)) {
		wielded = unequip_char(ch, WEAR_WIELD);
		if (IN_ROOM(vict) != NOWHERE)
			obj_to_char(wielded, vict);
		else
			obj_to_room(wielded, IN_ROOM(ch));
		obj_decay(wielded);
	}
	// log("[THROW] Miss stop extract weapon...");
	set_wait(ch, 3, TRUE);
	// log("Stop throw");
}

ACMD(do_throw)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_THROW)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
			vict = FIGHTING(ch);
		} else {
			send_to_char("� ���� ����� ?\r\n", ch);
			return;
		}
	};

	if (ch == vict) {
		send_to_char("�� ������, � �� ������ ������ !\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict))
		return;
	if (check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !FIGHTING(ch))
		go_throw(ch, vict);
	else if (!used_attack(ch)) {
		act("������. �� ����������� ������� ������ � $N3.", FALSE, ch, 0, vict, TO_CHAR);
		SET_EXTRA(ch, SKILL_THROW, vict);
	}
}

ACMD(do_manadrain)
{
	CHAR_DATA *vict;
	struct timed_type timed;
	int drained_mana, prob, percent, skill;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MANADRAIN)) {
		send_to_char("�� �� ������ ���.\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && timed_by_skill(ch, SKILL_MANADRAIN)) {
		send_to_char("��� ����� �� ���������.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
			vict = FIGHTING(ch);
		} else {
			send_to_char("� � ��� �� ������� ��������� �����?\r\n", ch);
			return;
		}
	};

	if (ch == vict) {
		send_to_char("�� ������� ���� �� ����� ���.\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("������� ������ ����� ��� ��������� ����� ����������� ������������.\r\n", ch);
		return;
	}

	if (!IS_NPC(vict)) {
		send_to_char("�� ����� ��������? ������ �� ��� ���!\r\n", ch);
		return;
	}

	if (affected_by_spell(vict, SPELL_SHIELD) || MOB_FLAGGED(vict, MOB_PROTECT)) {
		send_to_char("���� ������ ���� ������.\r\n", ch);
		return;
	}

	skill = GET_SKILL(ch, SKILL_MANADRAIN);

	percent = number(1, skill_info[SKILL_MANADRAIN].max_percent);
	/* ����������� ������ - 90% - 5% �� ������ ������� ������ ������ ������ ���� 
	   20% ���. */
	prob = MAX(20, 90 - 5 * MAX(0, GET_LEVEL(vict) - GET_LEVEL(ch)));
	improove_skill(ch, SKILL_MANADRAIN, percent > prob, vict);

	if (percent > prob) {
		damage(ch, vict, 0, SKILL_MANADRAIN + TYPE_HIT, TRUE);
	} else {
		/* % ��������������� ���� - %������ - 10% �� ������ ������� ������ ������
		   ������ ���� - 10% ���. */
		skill = MAX(10, skill - 10 * MAX(0, GET_LEVEL(ch) - GET_LEVEL(vict)));
		drained_mana = (GET_MAX_MANA(ch) - GET_MANA_STORED(ch)) * skill / 100;
		damage(ch, vict, 10, SKILL_MANADRAIN + TYPE_HIT, TRUE);
		GET_MANA_STORED(ch) = MIN(GET_MAX_MANA(ch), GET_MANA_STORED(ch) + drained_mana);
	}

	if (!IS_IMMORTAL(ch)) {
		timed.skill = SKILL_MANADRAIN;
		timed.time = 6;
		timed_to_char(ch, &timed);
	}

}

extern char cast_argument[MAX_INPUT_LENGTH];
ACMD(do_townportal)
{

	struct char_portal_type *tmp, *dlt = NULL;
	char arg2[MAX_INPUT_LENGTH];
	int vnum = 0;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TOWNPORTAL)) {
		send_to_char("������ ������� ������ ���������� ����.\r\n", ch);
		return;
	}

	two_arguments(argument, arg, arg2);
	if (!str_cmp(arg, "������")) {
		vnum = find_portal_by_word(arg2);
		for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next) {
			if (tmp->vnum == vnum) {
				if (dlt) {
					dlt->next = tmp->next;
				} else {
					GET_PORTALS(ch) = tmp->next;
				}
				free(tmp);
				sprintf(buf, "�� ��������� ������, ��� �������� ������ '&R%s&n'.\r\n", arg2);
				send_to_char(buf, ch);
				break;
			}
			dlt = tmp;
		}
		return;
	}

	*cast_argument = '\0';
	if (argument)
		strcat(cast_argument, arg);
	spell_townportal(GET_LEVEL(ch), ch, NULL, NULL);

}

/* Added by Gorrah */
ACMD(do_turn_undead)
{
	int percent, dam = 0;
        int i, sum, ch_list_size, max_level;
	struct timed_type timed;
        CHAR_DATA **ch_list;
        CHAR_DATA *ch_vict;

	if (IS_NPC(ch))		/* Cannot use on mobs. */
		return;

	if (!GET_SKILL(ch, SKILL_TURN_UNDEAD)) {
		send_to_char("��� ��� �� �� �����.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_TURN_UNDEAD)) {
		send_to_char("��� ������ �� �� ����� �������� ������, ����� ���������.\r\n", ch);
		return;
	}

	timed.skill = SKILL_TURN_UNDEAD;
	timed.time = IS_PALADINE(ch) ? 6 : 8;
	if (can_use_feat(ch, EXORCIST_FEAT))
		timed.time -= 2;
	timed_to_char(ch, &timed); 

	send_to_char (ch, "�� ����� ���� � ���������� ����� � �������� ������� ����� ���� �����.\r\n");
	act("$n ����$g ���� � ���������� ����� � �������� ������� ����� ���� �����.\r\n", FALSE, ch, 0, 0, TO_ROOM);

//���������� ������ ���� ������ � ������� � ���������� ������������� � ��-������
        for (ch_list_size = 0, ch_vict = world[ch->in_room]->people;
             ch_vict; ++ch_list_size, ch_vict = ch_vict->next_in_room);
        CREATE(ch_list, CHAR_DATA *, ch_list_size);
        for (i = 0, ch_vict = world[ch->in_room]->people; ch_vict; ch_vict = ch_vict->next_in_room) {
                if (IS_IMMORTAL(ch_vict))
                        continue;
                if (!HERE(ch_vict))
                        continue;
                if (same_group(ch, ch_vict))
                        continue;
                if (!IS_UNDEAD(ch_vict))
                        continue;
	        if (!may_kill_here(ch, ch_vict))
        	        return;
                ch_list[i++] = ch_vict;
        }
        ch_list_size = i;

	if (i > 0)
		percent = GET_SKILL(ch, SKILL_TURN_UNDEAD);
	else {
	        free(ch_list);
		return;
	}

//���������� ������������ ������� ���������� ������
	if (number(1, skill_info[SKILL_TURN_UNDEAD].max_percent) <= percent)
		max_level = GET_LEVEL(ch) + number(1, percent) / 10 + 5;
	else
		max_level = GET_LEVEL(ch) - number(1, 5);
	sum = dice(3, 8) + GET_LEVEL(ch) + percent / 2;

//���������.
//���� ������� ������ �������������, ��� ����������� - ���� �� ����� �����
//���� ��������� - �� �����+�����, ���� �� ������ ������ ���� - ������ �����.
        for (i = 0; i < ch_list_size; ++i) {
		if (sum <= 0)
			break;
                ch_vict = ch_list[i];
                if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch_vict) == NOWHERE)
                        continue;
		if ((GET_LEVEL(ch_vict) > max_level) ||
			(dice(1, GET_SAVE(ch_vict, SAVING_STABILITY) - con_app[GET_REAL_CON(ch_vict)].affect_saving) > 
				dice(1, GET_REAL_WIS(ch)))) {
			train_skill(ch, SKILL_TURN_UNDEAD, skill_info[SKILL_TURN_UNDEAD].max_percent, ch_vict);				
			damage(ch, ch_vict, 0, SKILL_TURN_UNDEAD + TYPE_HIT, TRUE);
			//send_to_char (ch, "���� ������ ��������� ����������.\r\n");
			continue;
		} 
		sum -= GET_LEVEL(ch_vict);
		if (GET_LEVEL(ch) - 8 >= GET_LEVEL(ch_vict)) {
			dam = MAX (1, GET_HIT (ch_vict) + 11);
		} else {
			if (IS_CLERIC(ch))
				dam = dice(8, 3 * GET_REAL_WIS(ch)) + GET_LEVEL(ch);
			else
				dam = dice(8, 4 * GET_REAL_WIS(ch) + GET_REAL_INT(ch)) + GET_LEVEL(ch);
		}
		train_skill(ch, SKILL_TURN_UNDEAD, skill_info[SKILL_TURN_UNDEAD].max_percent, ch_vict);
		damage(ch, ch_vict, dam, SKILL_TURN_UNDEAD + TYPE_HIT, TRUE);
	        if (!MOB_FLAGGED(ch_vict, MOB_NOFEAR) &&
				!general_savingthrow(ch_vict, SAVING_WILL, GET_REAL_WIS(ch) + GET_REAL_INT(ch), 0))
	                go_flee(ch_vict);
        }

        free(ch_list);
}
/* End of changes */

