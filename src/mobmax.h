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
#include "diskio.h"

void new_load_mkill(CHAR_DATA * ch);
int clear_kill_vnum(CHAR_DATA * vict, int vnum);
void inc_kill_vnum(CHAR_DATA * ch, int vnum, int incvalue);
int get_kill_vnum(CHAR_DATA * ch, int vnum);
void new_save_mkill(CHAR_DATA * ch);
void save_mkill(CHAR_DATA * ch, FBFILE * saved);
void free_mkill(CHAR_DATA * ch);
void delete_mkill_file(char *name);
void mob_lev_count(void);
