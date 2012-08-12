// Part of Bylins http://www.mud.ru

#include "fight.h"
#include "fight_local.hpp"
#include "utils.h"
#include "char.hpp"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "room.hpp"
#include "spells.h"
#include "dg_scripts.h"
#include "corpse.hpp"
#include "house.h"
#include "pk.h"
#include "stuff.hpp"
#include "sets_drop.hpp"
#include "top.h"
#include "constants.h"
#include "screen.h"
#include "magic.h"

// extern
void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR);
int level_exp(CHAR_DATA * ch, int chlevel);
int max_exp_gain_pc(CHAR_DATA * ch);
int max_exp_loss_pc(CHAR_DATA * ch);
void get_from_container(CHAR_DATA * ch, OBJ_DATA * cont, char *arg, int mode, int amount, bool autoloot);
ACMD(do_flee);

extern int material_value[];
extern int max_exp_gain_npc;

void process_mobmax(CHAR_DATA *ch, CHAR_DATA *killer)
{
	CHAR_DATA *master = 0;

	if (IS_NPC(killer)
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& killer->master)
	{
		master = killer->master;
	}
	else if (!IS_NPC(killer))
		master = killer;

	// �� ���� ������ master - PC
	if (master)
	{
		if (AFF_FLAGGED(master, AFF_GROUP))
		{
			int cnt = 0;

			// master - ���� ������, ��������� �� ������ ������
			if (master->master)
				master = master->master;

			if (IN_ROOM(master) == IN_ROOM(killer))
			{
				// ����� ������ � ����� �������, ��� � ������
				cnt = 1;
			}

			for (struct follow_type *f = master->followers; f; f = f->next)
			{
				if (AFF_FLAGGED(f->follower, AFF_GROUP)
					&& IN_ROOM(f->follower) == IN_ROOM(killer))
				{
					if (!number(0, cnt))
					{
						master = f->follower;
					}
					++cnt;
				}
			}
		}
		master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GET_LEVEL(ch));
	}
}

/* 29.11.09 ����������� �������� ����� (�) �������� */
//edited by WorM
void update_die_counts(CHAR_DATA *ch, CHAR_DATA *killer, int dec_exp)
{
	//��������� ������ ������ �������/����/������
	CHAR_DATA *rkiller = killer;

	if (rkiller
		&& IS_NPC(rkiller)
		&& (IS_CHARMICE(rkiller)
			|| IS_HORSE(rkiller)
			|| MOB_FLAGGED(killer, MOB_ANGEL)))
	{
		if(rkiller->master)
		{
			rkiller = rkiller->master;
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"die: %s killed by %s (without master)",
				GET_PAD(ch,0), GET_PAD(rkiller,0));
			mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
			rkiller = NULL;
		}
	}
	if (!IS_NPC(ch))
	{
		if (rkiller && rkiller != ch)
		{
			if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)) //��� �� �����
			{
				GET_RIP_ARENA(ch)= GET_RIP_ARENA(ch)+1;
				GET_WIN_ARENA(killer)= GET_WIN_ARENA(killer)+1;
				if (dec_exp)
				{
					GET_EXP_ARENA(ch)=GET_EXP_ARENA(ch)+dec_exp; //���� ��� � ��
				}
			}
			else if (IS_NPC(rkiller))
			{
				//��� �� ����
				GET_RIP_MOB(ch)= GET_RIP_MOB(ch)+1;
				GET_RIP_MOBTHIS(ch)= GET_RIP_MOBTHIS(ch)+1;
				if (dec_exp)
				{
					GET_EXP_MOB(ch)= GET_EXP_MOB(ch)+dec_exp;
					GET_EXP_MOBTHIS(ch)= GET_EXP_MOBTHIS(ch)+dec_exp;
				}
			}
			else if (!IS_NPC(rkiller))
			{
				//��� � ��
				GET_RIP_PK(ch)= GET_RIP_PK(ch)+1;
				GET_RIP_PKTHIS(ch)= GET_RIP_PKTHIS(ch)+1;
				if (dec_exp)
				{
					GET_EXP_PK(ch)= GET_EXP_PK(ch)+dec_exp;
					GET_EXP_PKTHIS(ch)= GET_EXP_PKTHIS(ch)+dec_exp;
				}
			}
		}
		else if ((!rkiller || (rkiller && rkiller == ch)) &&
			(ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) ||
			ROOM_FLAGGED(IN_ROOM(ch), ROOM_SLOWDEATH) ||
			ROOM_FLAGGED(IN_ROOM(ch), ROOM_ICEDEATH)))
		{
			//��� � ��
			GET_RIP_DT(ch)= GET_RIP_DT(ch)+1;
			GET_RIP_DTTHIS(ch)= GET_RIP_DTTHIS(ch)+1;
			if (dec_exp)
			{
				GET_EXP_DT(ch)= GET_EXP_DT(ch)+dec_exp;
				GET_EXP_DTTHIS(ch)= GET_EXP_DTTHIS(ch)+dec_exp;
			}
		}
		else// if (!rkiller || (rkiller && rkiller == ch))
		{
			//��� �� �������� �������������
			GET_RIP_OTHER(ch)= GET_RIP_OTHER(ch)+1;
			GET_RIP_OTHERTHIS(ch)= GET_RIP_OTHERTHIS(ch)+1;
			if (dec_exp)
			{
				GET_EXP_OTHER(ch)= GET_EXP_OTHER(ch)+dec_exp;
				GET_EXP_OTHERTHIS(ch)= GET_EXP_OTHERTHIS(ch)+dec_exp;
			}
		}
	}
}
//end by WorM
/*����� ������ (�) �������� */

void update_leadership(CHAR_DATA *ch, CHAR_DATA *killer)
{
	/* train LEADERSHIP */
	if (IS_NPC(ch)
		&& killer
		&& !IS_NPC(killer)
		&& AFF_FLAGGED(killer, AFF_GROUP)
		&& killer->master
		&& killer->master->get_skill(SKILL_LEADERSHIP) > 0
		&& IN_ROOM(killer) == IN_ROOM(killer->master))
	{
		improove_skill(killer->master, SKILL_LEADERSHIP, number(0, 1), ch);
	}

	/* decrease LEADERSHIP */
	if (!IS_NPC(ch)
		&& killer
		&& IS_NPC(killer)
		&& AFF_FLAGGED(ch, AFF_GROUP)
		&& ch->master
		&& IN_ROOM(ch) == IN_ROOM(ch->master)
		&& ch->master->get_inborn_skill(SKILL_LEADERSHIP) > 1)
	{
		ch->master->set_skill(SKILL_LEADERSHIP,
			ch->master->get_trained_skill(SKILL_LEADERSHIP) - 1);
	}
}

void die(CHAR_DATA *ch, CHAR_DATA *killer)
{
	int dec_exp = 0, e = GET_EXP(ch);

	if (!IS_NPC(ch) && (IN_ROOM(ch) == NOWHERE))
	{
		log("SYSERR: %s is dying in room NOWHERE.", GET_NAME(ch));
		return;
	}

	if (IS_NPC(ch)
		|| !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)
		|| RENTABLE(ch))
	{
		if (!(IS_NPC(ch)
			|| IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, GF_GODSLIKE)
			|| (killer && PRF_FLAGGED(killer, PRF_EXECUTOR))))//���� ���� �� �����
		{
			dec_exp = (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch))) / (3 + MIN(3, GET_REMORT(ch) / 5));
			gain_exp(ch, -dec_exp);
			dec_exp = e - GET_EXP(ch);
			sprintf(buf, "�� �������� %d %s �����.\r\n",
				dec_exp, desc_count(dec_exp, WHAT_POINT));
			send_to_char(buf, ch);
		}

		/* ��������� ������ �� ����� */
		/* ����� �������� ����������, ����� ������ ����������,         */
		/* ����� ����, �������� ������ � ������������ ������ � ������� */
		if (IS_NPC(ch) && killer)
		{
			process_mobmax(ch, killer);
		}

		if (killer)
		{
			update_leadership(ch, killer);
		}
	}

	update_die_counts(ch, killer, dec_exp);
	raw_kill(ch, killer);
}

/**
* ������ �������� � ���� ��� ������/����� � ��.
*/
void reset_affects(CHAR_DATA *ch)
{
	AFFECT_DATA *af, *naf;

	for (af = ch->affected; af; af = naf)
	{
		naf = af->next;
		if (!IS_SET(af->battleflag, AF_DEADKEEP))
			affect_remove(ch, af);
	}

	affect_total(ch);
}

void forget_all_spells(CHAR_DATA *ch)
{
	MemQ_flush(ch);
	for (int i = 0; i < MAX_SPELLS + 1; i++)
	{
		ch->real_abils.SplMem[i] = 0;
	}
}

/* ������� ������������ ��� "�����������" � "��������",
   ����� �� ���� ���� ��� ������ ��� � �����             */
int can_loot(CHAR_DATA * ch)
{
	if (ch != NULL)
	{
		if (!IS_NPC(ch)
			&& GET_MOB_HOLD(ch) == 0 // ���� ��� ������
			&& !AFF_FLAGGED(ch, AFF_STOPFIGHT) // ����������� ������
			&& !AFF_FLAGGED(ch, AFF_BLIND))	// ����
		{
			return TRUE;
		}
	}
	return FALSE;
}

void death_cry(CHAR_DATA * ch)
{
	int door;
	act("�������� ������ � ����� �� ������������� ����� $n1.",
		FALSE, ch, 0, 0, TO_ROOM | CHECK_DEAF | TO_ARENA_LISTEN);

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		if (CAN_GO(ch, door))
		{
			CHAR_DATA *people = world[world[IN_ROOM(ch)]->dir_option[door]->to_room]->people;
			if (people)
			{
				act("�������� ������ � ����� �� �����-�� ������������� �����.",
					FALSE, people, 0, 0, TO_CHAR | CHECK_DEAF);
				act("�������� ������ � ����� �� �����-�� ������������� �����.",
					FALSE, people, 0, 0, TO_ROOM | CHECK_DEAF);
			}
		}
	}
}

void arena_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	make_arena_corpse(ch, killer);
	//���� ���� ����� �� ��� ������ ������������ � ����
	if(killer && PRF_FLAGGED(killer, PRF_EXECUTOR))
	{
		killer->set_gold(ch->get_gold() + killer->get_gold());
		ch->set_gold(0);
	}
	change_fighting(ch, TRUE);
	GET_HIT(ch) = 1;
	GET_POS(ch) = POS_SITTING;
	char_from_room(ch);
	int to_room = real_room(GET_LOADROOM(ch));
	// ��� �������� ������� ������ ���� �� ������, ���� ��� � ����� �� ����
	if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
	{
		to_room = Clan::CloseRent(to_room);
	}
	if (to_room == NOWHERE)
	{
		SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
		HELL_DURATION(ch) = time(0) + 6;
		to_room = r_helled_start_room;
	}
	char_to_room(ch, to_room);
	look_at_room(ch, to_room);
	act("$n �� ������� ����$g � �����...", FALSE, ch, 0, 0, TO_ROOM);
}

void auto_loot(CHAR_DATA *ch, CHAR_DATA *killer, OBJ_DATA *corpse, int local_gold)
{
	char obj[256];

	if (IS_NPC(ch)
		&& !IS_NPC(killer)
		&& PRF_FLAGGED(killer, PRF_AUTOLOOT)
		&& (corpse != NULL)
		&& can_loot(killer))
	{
		sprintf(obj, "all");
		get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1, true);
	}
	else if (IS_NPC(ch)
		&& !IS_NPC(killer)
		&& local_gold
		&& PRF_FLAGGED(killer, PRF_AUTOMONEY)
		&& (corpse != NULL)
		&& can_loot(killer))
	{
		sprintf(obj, "all.coin");
		get_from_container(killer, corpse, obj, FIND_OBJ_INV, 1, false);
	}
	else if (IS_NPC(ch)
		&& IS_NPC(killer)
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& (corpse != NULL)
		&& killer->master
		&& killer->in_room == killer->master->in_room
		&& PRF_FLAGGED(killer->master, PRF_AUTOLOOT)
		&& can_loot(killer->master))
	{
		sprintf(obj, "all");
		get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1, true);
	}
	else if (IS_NPC(ch)
		&& IS_NPC(killer)
		&& local_gold
		&& (AFF_FLAGGED(killer, AFF_CHARM) || MOB_FLAGGED(killer, MOB_ANGEL))
		&& (corpse != NULL)
		&& killer->master
		&& killer->in_room == killer->master->in_room
		&& PRF_FLAGGED(killer->master, PRF_AUTOMONEY)
		&& can_loot(killer->master))
	{
		sprintf(obj, "all.coin");
		get_from_container(killer->master, corpse, obj, FIND_OBJ_INV, 1, false);
	}
}

void check_spell_capable(CHAR_DATA *ch, CHAR_DATA *killer)
{
	if(IS_NPC(ch)
		&& killer
		&& killer != ch
		&& MOB_FLAGGED(ch, MOB_CLONE)
		&& ch->master
		&& affected_by_spell(ch, SPELL_CAPABLE))
	{
		act("����, ���������� �� $n3, ������ ����������� � ����� ������������ � ����� �������.",
			FALSE, ch, 0, killer, TO_ROOM | TO_ARENA_LISTEN);
		int pos = GET_POS(ch);
		GET_POS(ch) = POS_STANDING;
		call_magic(ch, killer, NULL, world[IN_ROOM(ch)], ch->mob_specials.capable_spell,
			GET_LEVEL(ch), CAST_SPELL);
		GET_POS(ch) = pos;
	}
}

void clear_mobs_memory(CHAR_DATA *ch)
{
	for (CHAR_DATA *hitter = character_list; hitter; hitter = hitter->next)
	{
		if (IS_NPC(hitter) && MEMORY(hitter))
		{
			forget(hitter, ch);
		}
	}
}

void real_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	const long local_gold = ch->get_gold();
	OBJ_DATA *corpse = make_corpse(ch, killer);
	bloody::handle_corpse(corpse, ch, killer);

	// ������� ����� pk_revenge_action �� die, ����� �� ������ ��������
	// ����� ����� �� ������ ���� ��� ����
	if (IS_NPC(ch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) || RENTABLE(ch))
	{
		pk_revenge_action(killer, ch);
	}

	if (!IS_NPC(ch))
	{
		forget_all_spells(ch);
		clear_mobs_memory(ch);
		/* ���� ���� � ��� - �� ����� ����� �� ���� */
		RENTABLE(ch) = 0;
		AGRESSOR(ch) = 0;
		AGRO(ch) = 0;
	}
	else
	{
		if (killer && (!IS_NPC(killer) || IS_CHARMICE(killer)))
		{
			log("Killed: %d %d %ld", GET_LEVEL(ch), GET_MAX_HIT(ch), GET_EXP(ch));
			obj_load_on_death(corpse, ch);
		}
		if (MOB_FLAGGED(ch, MOB_CORPSE))
		{
			perform_drop_gold(ch, local_gold, SCMD_DROP, 0);
			ch->set_gold(0);
		}
		dl_load_obj(corpse, ch, NULL, DL_ORDINARY);
		dl_load_obj(corpse, ch, NULL, DL_PROGRESSION);
	}

	// ������ ���������� ������� "����������" � "����� ����" ���������� �� � damage,
	// � �����, ����� �������� ���������������� �����. ����� ����,
	// ���� ���� ������ � ������ � �������, �� ������� ���������� �������
	if ((ch != NULL) && (killer != NULL))
	{
		auto_loot(ch, killer, corpse, local_gold);
	}
}

void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer)
{
	check_spell_capable(ch, killer);

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	for (CHAR_DATA *hitter = combat_list; hitter; hitter = hitter->next_fighting)
	{
		if (hitter->get_fighting() == ch)
		{
			WAIT_STATE(hitter, 0);
		}
	}

	reset_affects(ch);

	if ((!killer || death_mtrigger(ch, killer)) && IN_ROOM(ch) != NOWHERE)
	{
		death_cry(ch);
	}

	if (IN_ROOM(ch) != NOWHERE)
	{
		if (!IS_NPC(ch)
			&& ((!RENTABLE(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
				|| (killer && PRF_FLAGGED(killer, PRF_EXECUTOR))))
		{
			//���� ����� �� ����� ��� �����
			arena_kill(ch, killer);
		}
		else
		{
			real_kill(ch, killer);
			extract_char(ch, TRUE);
		}
	}
}

int get_extend_exp(int exp, CHAR_DATA * ch, CHAR_DATA * victim)
{
	int base, diff;
	int koef;

	if (!IS_NPC(victim) || IS_NPC(ch))
		return (exp);

	for (koef = 100, base = 0, diff = ch->mobmax_get(GET_MOB_VNUM(victim));
			base < diff && koef > 5; base++, koef = koef * 95 / 100);

	exp = exp * MAX(5, koef) / 100;
	exp /= MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1);

	return (exp);
}

/* When ch kills victim */
void change_alignment(CHAR_DATA * ch, CHAR_DATA * victim)
{
	/*
	 * new alignment change algorithm: if you kill a monster with alignment A,
	 * you move 1/16th of the way to having alignment -A.  Simple and fast.
	 */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

/*++
   ������� ���������� �����
      ch - ���� ���� ���������
           ����� ���� ������� ��� NPC ������ �� �����, �� ��� �����
           �����-�� �������� ������ ����� �� ��������
--*/
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef)
{
	if (!EXTRA_FLAGGED(victim, EXTRA_GRP_KILL_COUNT)
		&& !IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& IS_NPC(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	{
		SetsDrop::add_mob_stat(victim, members);
		SET_BIT(EXTRA_FLAGS(victim, EXTRA_GRP_KILL_COUNT), EXTRA_GRP_KILL_COUNT);
	}

// �������, �� ��� NPC ��� ������� ���� ������ ��������
//  if (IS_NPC(ch) || !OK_GAIN_EXP(ch,victim))
	if (!OK_GAIN_EXP(ch, victim))
	{
		send_to_char("���� ������ ����� �� ������.\r\n", ch);
		return;
	}

	// 1. ���� ������� ������� �� ����
	int exp = GET_EXP(victim) / MAX(members, 1);

	if(victim->get_zone_group() > 1 && members < victim->get_zone_group())
	{
		// � ������ ����-���� ������ ���� ������ �� ��� ���-�� ������� � ������
		exp = GET_EXP(victim) / victim->get_zone_group();
	}

	// 2. ����������� ����������� (���������, �������� �������)
	//    �� ��� ������ ��� ���������� ������������ ��� � �� � ����� ���������,
	//    ���� � ����������� ������� ��� ��� �����
	exp = exp * koef / 100;

	// 3. ���������� ����� ��� PC � NPC
	if (IS_NPC(ch))
	{
		exp = MIN(max_exp_gain_npc, exp);
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	}
	else
	{
		exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
	}

	// 4. ��������� ��������
	exp = MAX(1, exp);

	int clan_exp = 0;
	if (CLAN(ch))
	{
		clan_exp = exp - CLAN(ch)->SetClanExp(ch, exp);
		exp -= clan_exp;
		if (clan_exp > 0)
		{
			sprintf(buf1, "�� ������ %d %s ����� ������ �����.\r\n",
				clan_exp, desc_count(clan_exp, WHAT_POINT));
			send_to_char(buf1, ch);
		}
	}

	if (exp > 1)
	{
		sprintf(buf2, "��� ���� ��������� �� %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
		send_to_char(buf2, ch);
	}
	else if (exp == 1)
		send_to_char("��� ���� ��������� ����� ���� �� ��������� ��������.\r\n", ch);

	gain_exp(ch, exp, clan_exp);
	change_alignment(ch, victim);
	TopPlayer::Refresh(ch);
}

/*++
   ������� ����������� ������ ������ ��� ������ ��� ��������� �����,
 ����� ���� �������� ������� ��������� ����� ��� ���� ������ ������
 �.�. ������ ������ ����� ���� ������ PC, �� ��� ������� ������� ���� ������ PC

   ch - ����������� ���� ������, �� ���� �������:
            1. ��� �� NPC
            2. �� ��������� � ������ ������ (��� ��� �����)

   ������ ��� PC-�������������� ��� ������� �� ����������

--*/
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int inroom_members, koef = 100, maxlevel, rmrt;
	CHAR_DATA *k;
	struct follow_type *f;
	int leader_inroom;

	maxlevel = GET_LEVEL(ch);

	if (!(k = ch->master))
		k = ch;

	// k - ���������� �� ������ ������
	leader_inroom = (AFF_FLAGGED(k, AFF_GROUP)
					 && (k->in_room == IN_ROOM(ch)));

	// ���������� ����������� � �������
	if (leader_inroom)
	{
		inroom_members = 1;
		maxlevel = GET_LEVEL(k);
	}
	else
		inroom_members = 0;

	// ��������� ������������ ������� � ������
	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
		{
			// �������� ������ ������ � ��� �� �������
			// ���� ������ => PC �������������
			++inroom_members;
			maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
		}

	// ���������, ���� �� ������ �����, ������� ������� ������, ���� �� �����
	rmrt = MIN(14, (int)GET_REMORT(k));
	if (maxlevel - GET_LEVEL(k) > grouping[(int)GET_CLASS(k)][rmrt] && leader_inroom)
	{
		koef -= 50 + (maxlevel - GET_LEVEL(k) - grouping[(int)GET_CLASS(k)][rmrt]) * 2;
	}
	else	// ���� � ������� ��� �� ���� �� �� ���, ������� �� ������
	{
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
			{
				rmrt = MIN(14, (int)GET_REMORT(f->follower));
				if (maxlevel - GET_LEVEL(f->follower) >
						grouping[(int)GET_CLASS(f->follower)][rmrt])
				{
					koef -= 50 + (maxlevel - GET_LEVEL(f->follower)
						- grouping[(int)GET_CLASS(f->follower)][rmrt]) * 2;
					break;
				}
			}
		}
	}

	koef = MAX(0, koef);

	// ��������� ������������, ���� � ������� ����� � ���� ��� ���� ���-��
	// �� ������ �� PC (������������� ���� ������ ��� �������� �� ���������)
	if (koef >= 100 && leader_inroom && (inroom_members > 1) && calc_leadership(k))
	{
		koef += 20;
	}

	// ������� �����

	if (leader_inroom)
		perform_group_gain(k, victim, inroom_members, koef);

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == IN_ROOM(ch))
			perform_group_gain(f->follower, victim, inroom_members, koef);
}

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
	if (ch != victim
		&& OK_GAIN_EXP(ch, victim)
		&& GET_EXP(victim) > 0
		&& !AFF_FLAGGED(victim, AFF_CHARM)
		&& !MOB_FLAGGED(victim, MOB_ANGEL)
		&& !IS_NPC(ch)
		&& !MOB_FLAGGED(victim, MOB_NO_BATTLE_EXP))
	{
		int max_exp = MIN(max_exp_gain_pc(ch), (GET_LEVEL(victim) * GET_MAX_HIT(victim) + 4) /
			(5 * MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1)));
		double coeff = MIN(dam, GET_HIT(victim)) / static_cast<double>(GET_MAX_HIT(victim));
		int battle_exp = MAX(1, static_cast<int>(max_exp * coeff));
//		int battle_exp = MAX(1, (GET_LEVEL(victim) * MIN(dam, GET_HIT(victim)) + 4) /
//						 (5 * MAX(1, GET_REMORT(ch) - MAX_EXP_COEFFICIENTS_USED - 1)));
		gain_exp(ch, battle_exp);
		ch->dps_add_exp(battle_exp, true);
	}
}

/* Alterate equipment
 *
 */
void alterate_object(OBJ_DATA * obj, int dam, int chance)
{
	if (!obj)
		return;
	dam = number(0, dam * (material_value[GET_OBJ_MATER(obj)] + 30) /
				 MAX(1, GET_OBJ_MAX(obj) *
					 (IS_OBJ_STAT(obj, ITEM_NODROP) ? 5 :
					  IS_OBJ_STAT(obj, ITEM_BLESS) ? 15 : 10) * (GET_OBJ_SKILL(obj) == SKILL_BOWS ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100))
	{
		if (dam > 1 && obj->worn_by && GET_EQ(obj->worn_by, WEAR_SHIELD) == obj)
		{
			dam /= 2;
		}
		if ((GET_OBJ_CUR(obj) -= dam) <= 0)
		{
			if (obj->worn_by)
				act("$o ��������$U, �� �������� �����������.", FALSE, obj->worn_by, obj, 0, TO_CHAR);
			else if (obj->carried_by)
				act("$o ��������$U, �� �������� �����������.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
			extract_obj(obj);
		}
	}
}

void alt_equip(CHAR_DATA * ch, int pos, int dam, int chance)
{
	// calculate chance if
	if (pos == NOWHERE)
	{
		pos = number(0, 100);
		if (pos < 3)
			pos = WEAR_FINGER_R + number(0, 1);
		else if (pos < 6)
			pos = WEAR_NECK_1 + number(0, 1);
		else if (pos < 20)
			pos = WEAR_BODY;
		else if (pos < 30)
			pos = WEAR_HEAD;
		else if (pos < 45)
			pos = WEAR_LEGS;
		else if (pos < 50)
			pos = WEAR_FEET;
		else if (pos < 58)
			pos = WEAR_HANDS;
		else if (pos < 66)
			pos = WEAR_ARMS;
		else if (pos < 76)
			pos = WEAR_SHIELD;
		else if (pos < 86)
			pos = WEAR_ABOUT;
		else if (pos < 90)
			pos = WEAR_WAIST;
		else if (pos < 94)
			pos = WEAR_WRIST_R + number(0, 1);
		else
			pos = WEAR_HOLD;
	}

	if (pos <= 0 || pos > WEAR_BOTHS || !GET_EQ(ch, pos) || dam < 0)
		return;
	alterate_object(GET_EQ(ch, pos), dam, chance);
}

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;

	for (; *str; str++)
	{
		if (*str == '#')
		{
			switch (*(++str))
			{
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		}
		else
			*(cp++) = *str;

		*cp = 0;
	}			/* For */

	return (buf);
}

bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line)
{
	if (!ch || ch->purged() || !victim || victim->purged())
	{
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim ? (victim->purged() ? "purged" : "true") : "false",
			fname, line);
		return false;
	}
	return true;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim  died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */

void char_dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, int attacktype, bool mayflee)
{
	if (IN_ROOM(ch) == NOWHERE)
		return;

	switch (GET_POS(victim))
	{
	case POS_MORTALLYW:
		act("$n ���������� �����$a � �����, ���� $m �� �������.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("�� ���������� ������ � ������, ���� ��� �� �������.\r\n", victim);
		break;
	case POS_INCAP:
		act("$n ��� �������� � �������� �������. �������� �� $m.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("�� ��� �������� � �������� ��������, ��������� ��� ������.\r\n", victim);
		break;
	case POS_STUNNED:
		act("$n ��� ��������, �� �������� $e ��� ������� (������� :).", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		send_to_char("�������� �������� ���. � ����� �� ��� ���� ����� ����.\r\n", victim);
		break;
	case POS_DEAD:
		if (IS_NPC(victim) && (MOB_FLAGGED(victim, MOB_CORPSE)))
		{
			act("$n ��������$g � ��������$u � ����.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("������ ��� ����� � ���� ���� �� �������� !\r\n", victim);
		}
		else
		{
			act("$n �����$a, $s ���� �������� ���������� � ������.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			send_to_char("�� ������!  ��� ����� ����...\r\n", victim);
		}
		break;
	default:		/* >= POSITION SLEEPING */
		if (dam > (GET_REAL_MAX_HIT(victim) / 4))
			send_to_char("��� ������������� ������ !\r\n", victim);

		if (dam > 0 && GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4))
		{
			sprintf(buf2,
					"%s �� �������, ����� ���� ���� �� ����������� ��� ������ ! %s\r\n",
					CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
			send_to_char(buf2, victim);
		}
		if (ch != victim &&
				IS_NPC(victim) &&
				GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4) &&
				MOB_FLAGGED(victim, MOB_WIMPY) && mayflee && GET_POS(victim) > POS_SITTING)
			do_flee(victim, NULL, 0, 0);

		if (ch != victim &&
				!IS_NPC(victim) &&
				HERE(victim) &&
				GET_WIMP_LEV(victim) &&
				GET_HIT(victim) < GET_WIMP_LEV(victim) && mayflee && GET_POS(victim) > POS_SITTING)
		{
			send_to_char("�� ������������ � ���������� ������� !\r\n", victim);
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}
}

void test_self_hitroll(CHAR_DATA *ch)
{
	HitType hit;
	hit.type = TYPE_UNDEFINED;
	hit.weapon = RIGHT_WEAPON;
	hit.init(ch, ch);
	hit.calc_base_hr(ch);
	log("t2: %d", hit.calc_thaco);

	HitType hit2;
	hit2.type = TYPE_UNDEFINED;
	hit2.weapon = LEFT_WEAPON;
	hit2.init(ch, ch	);
	hit2.calc_base_hr(ch);
	log("t3: %d", hit2.calc_thaco);
}

/**
 * ������� ��� ������� ����
 */
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype, bool mayflee, int dmg_type)
{
	DmgType dmg;
	dmg.dam = dam;
	dmg.w_type = attacktype;
	dmg.mayflee = mayflee;
	dmg.dmg_type = dmg_type;
	return dmg.damage(ch, victim);
}

/*
void solo_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int exp;

  if (IS_NPC(ch) || !OK_GAIN_EXP(ch, victim))
     {send_to_char("���� ������ ����� �� ������.\r\n",ch);
      return;
     }

  if (IS_NPC(ch))
     {exp  = MIN(max_exp_gain_npc, GET_EXP(victim));
      exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
     }
  else
     {exp = get_extend_exp(GET_EXP(victim), ch, victim);
      exp = MIN(max_exp_gain_pc(ch), exp);
     };

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp = MAX(1,exp);

  if (exp > 1)
     {sprintf(buf2, "��� ���� ��������� �� %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("��� ���� ��������� ����� ���� �� ��������� ��������.\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}
*/
