/* ************************************************************************
*   File: mobmax.h                                      Part of Bylins    *
*  Usage: header file: constants and fn prototypes for ������� �� �����   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _MOBMAX_H_
#define _MOBMAX_H_

int clear_kill_vnum(CHAR_DATA * vict, int vnum);
void inc_kill_vnum(CHAR_DATA * ch, int vnum, int incvalue);
int get_kill_vnum(CHAR_DATA * ch, int vnum);
void save_mkill(CHAR_DATA * ch, FILE * saved);
void free_mkill(CHAR_DATA * ch);
void mob_lev_count(void);

#endif
