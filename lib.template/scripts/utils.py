# -*- coding: koi8-r -*-

import sys
import traceback
import mud, constants

def log_sys(msg):
    mud.log(msg + "\n" + traceback.format_exc(), channel=constants.SYSLOG)
	
def reg_cmd(cmd_text, command,  position_min, level_min, unhide_percent):
	mud.register_global_command(cmd_text, cmd_text.encode("koi8-r"), command, position_min, level_min, unhide_percent)
	
def load_obj(vnum):
	"""������� ������ � ��������� ������"""
	return mud.get_obj_proto(mud.get_obj_rnum(vnum))