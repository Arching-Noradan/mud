# -*- coding: koi8-r -*-
import mud

class Command:
	# ���� �������
	command_text = "����"
	# �������, ������� � ������� ������� ����� ���������. constants.POS_XXX
	position_min = 0
	# ���. ������� ������ ��� ���������� �������
	level_min = 0
	# ����������� ������������
	unhide_percent = 100
	@staticmethod
	def command(ch, cmd, args):
		""" ������ �������� - ���, ������ �������, ������ ���������"""
		ch.send("���������: {0}, �������: {1}, ��� ����: {2}".format(args, cmd, ch.get_pad(0)))