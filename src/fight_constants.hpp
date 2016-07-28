#if !defined __FIGHT_CONSTANTS_HPP__
#define __FIGHT_CONSTANTS_HPP__

namespace FightSystem
{
	enum DmgType { UNDEF_DMG, PHYS_DMG, MAGE_DMG };

	enum
	{
		type_hit, type_skin, type_whip, type_slash, type_bite,
		type_bludgeon, type_crush, type_pound, type_claw, type_maul,
		type_thrash, type_pierce, type_blast, type_punch, type_stab,
		type_pick, type_sting
	};

	enum
	{
		// ����� �����
		IGNORE_SANCT,
		// ����� ������
		IGNORE_PRISM,
		// ����� �����
		IGNORE_ARMOR,
		// �������������� ����������� �����
		HALF_IGNORE_ARMOR,
		// ����� ����������
		IGNORE_ABSORBE,
		// ������ �������
		NO_FLEE,
		// ���� ����
		CRIT_HIT,
		// ����� �������� ������ �� ��������� ����
		IGNORE_FSHIELD,
		// ����� ���� �� ����������� ������� ��� ��������� �������
		MAGIC_REFLECT,
		// ������ ����� �������� ���
		VICTIM_FIRE_SHIELD,
		// ������ ����� ��������� ���
		VICTIM_AIR_SHIELD,
		// ������ ����� ������� ���
		VICTIM_ICE_SHIELD,
		// ���� ��������� �� ��������� ���� � ������� ������
		DRAW_BRIEF_FIRE_SHIELD,
		// ���� ��������� �� ���������� ���� � ������� ������
		DRAW_BRIEF_AIR_SHIELD,
		// ���� ��������� �� �������� ���� � ������� ������
		DRAW_BRIEF_ICE_SHIELD,
		// ���� ������� �� ���. �������
		DRAW_BRIEF_MAG_MIRROR,

		// ���-�� ������
		HIT_TYPE_FLAGS_NUM
	};
} // namespace FightSystem

#endif