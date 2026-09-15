#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "horse_rider.h"
#include "config.h"
#include "char_manager.h"

const int HORSE_HEALTH_DROP_INTERVAL = 3 * 24 * 60 * 60;
const int HORSE_STAMINA_CONSUME_INTERVAL = 6 * 60;
const int HORSE_STAMINA_REGEN_INTERVAL = 12 * 60;
//const int HORSE_HP_DROP_INTERVAL = 60;
//const int HORSE_STAMINA_CONSUME_INTERVAL = 3;
//const int HORSE_STAMINA_REGEN_INTERVAL = 6;

THorseStat c_aHorseStat[HORSE_MAX_LEVEL+1] =
/*
   int iMinLevel;
   int iNPCRace;
   int iMaxHealth;
   int iMaxStamina;
   int iST;
   int iDX;
   int iHT;
   int iIQ;
   int iDamMean;
   int iDamMin;
   int iDamMax;
   int iDef;
 */
{
	{  0,	0,	1,	1,	0,		0,	0,	0,	0,	0,	0,	0  },
	{ 25,	20101,	3,	4,		26,	35,	18,	0,	54,	43,	64,	32 }, // 1
	{ 25,	20101,	4,	4,		27,	36,	18,	0,	55,	44,	66,	33 },
	{ 25,	20101,	5,	5,		28,	38,	19,	0,	56,	44,	67,	33 },
	{ 25,	20101,	7,	5,		29,	39,	19,	0,	57,	45,	68,	34 },
	{ 25,	20101,	8,	6,		30,	40,	20,	0,	58,	46,	69,	34 },
	{ 25,	20101,	9,	6,		31,	41,	21,	0,	59,	47,	70,	35 },
	{ 25,	20101,	11,	7,		32,	42,	21,	0,	60,	48,	72,	36 },
	{ 25,	20101,	12,	7,		33,	44,	22,	0,	61,	48,	73,	36 },
	{ 25,	20101,	13,	8,		34,	45,	22,	0,	62,	49,	74,	37 },
	{ 25,	20101,	15,	10,		35,	46,	23,	0,	63,	50,	75,	37 },
	{ 35,	20104,	18,	30,		40,	53,	27,	0,	69,	55,	82,	41 }, // 11
	{ 35,	20104,	19,	35,		41,	54,	27,	0,	70,	56,	84,	42 },
	{ 35,	20104,	21,	40,		42,	56,	28,	0,	71,	56,	85,	42 },
	{ 35,	20104,	22,	50,		43,	57,	28,	0,	72,	57,	86,	43 },
	{ 35,	20104,	24,	55,		44,	58,	29,	0,	73,	58,	87,	43 },
	{ 35,	20104,	25,	60,		44,	59,	30,	0,	74,	59,	88,	44 },
	{ 35,	20104,	27,	65,		45,	60,	30,	0,	75,	60,	90,	45 },
	{ 35,	20104,	28,	70,		46,	62,	31,	0,	76,	60,	91,	45 },
	{ 35,	20104,	30,	80,		47,	63,	31,	0,	77,	61,	92,	46 },
	{ 35,	20104,	32,	100,	48,	64,	32,	0,	78,	62,	93,	46 },
	{ 50,	20107,	35,	120,	53,	71,	36,	0,	84,	67,	100,	50 }, // 21
	{ 50,	20107,	36,	125,	55,	74,	37,	0,	86,	68,	103,	51 },
	{ 50,	20107,	37,	130,	57,	76,	38,	0,	88,	70,	105,	52 },
	{ 50,	20107,	38,	135,	59,	78,	39,	0,	90,	72,	108,	54 },
	{ 50,	20107,	40,	140,	60,	80,	40,	0,	91,	72,	109,	54 },
	{ 50,	20107,	42,	145,	61,	81,	40,	0,	92,	73,	110,	55 },
	{ 50,	20107,	44,	150,	62,	83,	42,	0,	94,	75,	112,	56 },
	{ 50,	20107,	46,	160,	63,	84,	42,	0,	95,	76,	114,	57 },
	{ 50,	20107,	48,	170,	65,	87,	43,	0,	97,	77,	116,	58 },
	{ 50,	20107,	50,	200,	67,	89,	45,	0,	99,	79,	118,	59 }
};

CHorseRider::CHorseRider()
{
	Initialize();
}

CHorseRider::~CHorseRider()
{
	Destroy();
}

void CHorseRider::Initialize()
{
	m_eventStaminaRegen = NULL;
	m_eventStaminaConsume = NULL;
	m_bIsStopStaminaConsume = false;
	memset(&m_Horse, 0, sizeof(m_Horse));
}

void CHorseRider::Destroy()
{
	event_cancel(&m_eventStaminaRegen);
	event_cancel(&m_eventStaminaConsume);
}

void CHorseRider::EnterHorse()
{
	if (GetHorseLevel() <= 0)
		return;

	if (IsHorseRiding())
	{
		m_Horse.bRiding = !m_Horse.bRiding;
		StartRiding();
	}
	else
	{
		StartStaminaRegenEvent();
	}
	CheckHorseHealthDropTime(false);
}

short CHorseRider::GetHorseMaxHealth() const
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxHealth;
}

short CHorseRider::GetHorseMaxStamina() const
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxStamina;
}

BYTE CHorseRider::GetHorseHealthGrade()
{
	if (GetHorseHealth() == 0)
		return 0;
	else if (GetHorseHealth() * 10 <= GetHorseMaxHealth() * 3)
		return 1;
	else if (GetHorseHealth() * 10 <= GetHorseMaxHealth() * 7)
		return 2;
	else
		return 3;
}

void CHorseRider::FeedHorse()
{
	if (GetHorseLevel() > 0)
	{
		UpdateHorseHealth(+1);

		ResetHorseHealthDropTime();
	}
}

void CHorseRider::SetHorseData(const THorseInfo& crInfo)
{
	m_Horse = crInfo;
}

// Stamina
void CHorseRider::UpdateHorseDataByLogoff(DWORD dwLogoffTime)
{
	if (GetHorseLevel() <= 0)
		return;

	if (dwLogoffTime >= 12 * 60)
		UpdateHorseStamina(dwLogoffTime / 12 / 60, false);
}

void CHorseRider::UpdateHorseStamina(int iStamina, bool bSend)
{
	if (iStamina < 1 && m_bIsStopStaminaConsume)
	{
		if (test_server)
			sys_log(0, "stamina consume prevented");
		return;
	}

	int level = GetHorseLevel();

	m_Horse.sStamina = MINMAX(0, m_Horse.sStamina + iStamina, c_aHorseStat[level].iMaxStamina);

	if (GetHorseStamina() == 0 && IsHorseRiding())
	{
		StopRiding();
	}

	if (bSend)
		SendHorseInfo();
}

bool CHorseRider::StartRiding()
{
	if (m_Horse.bRiding)
		return false;

	if (GetHorseLevel() <= 0)
		return false;

	if (GetHorseStamina() <= 0)
		return false;

	m_Horse.bRiding = true;
	StartStaminaConsumeEvent();
	SendHorseInfo();
	return true;
}

bool CHorseRider::StopRiding()
{
	if (!m_Horse.bRiding)
		return false;

	m_Horse.bRiding = false;
	StartStaminaRegenEvent();
	return true;
}

EVENTINFO(horserider_info)
{
	CHorseRider* hr;

	horserider_info()
	: hr( 0 )
	{
	}
};

EVENTFUNC(horse_stamina_consume_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_consume_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	hr->UpdateHorseStamina(-1);
	hr->UpdateRideTime(HORSE_STAMINA_CONSUME_INTERVAL);

	int delta = PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL);

	if (hr->GetHorseStamina() == 0)
	{
		hr->m_eventStaminaConsume = NULL;
		delta = 0;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA - %p", get_pointer(event));
	return delta;
}

EVENTFUNC(horse_stamina_regen_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_regen_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	hr->UpdateHorseStamina(+1);
	int delta = PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL);
	if (hr->GetHorseStamina() == hr->GetHorseMaxStamina())
	{
		delta = 0;
		hr->m_eventStaminaRegen = NULL;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA + %p", get_pointer(event));

	return delta;
}

void CHorseRider::StartStaminaConsumeEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	sys_log(0,"HORSE STAMINA REGEN EVENT CANCEL %p", get_pointer(m_eventStaminaRegen));
	event_cancel(&m_eventStaminaRegen);

	if (m_eventStaminaConsume)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaConsume = event_create(horse_stamina_consume_event, info, PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL));
	sys_log(0,"HORSE STAMINA CONSUME EVENT CREATE %p", get_pointer(m_eventStaminaConsume));
}

void CHorseRider::StartStaminaRegenEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	sys_log(0,"HORSE STAMINA CONSUME EVENT CANCEL %p", get_pointer(m_eventStaminaConsume));
	event_cancel(&m_eventStaminaConsume);

	if (m_eventStaminaRegen)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaRegen = event_create(horse_stamina_regen_event, info, PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL));
	sys_log(0,"HORSE STAMINA REGEN EVENT CREATE %p", get_pointer(m_eventStaminaRegen));
}

// Health
void CHorseRider::ResetHorseHealthDropTime()
{
	m_Horse.dwHorseHealthDropTime = get_global_time() + HORSE_HEALTH_DROP_INTERVAL;
}

void CHorseRider::CheckHorseHealthDropTime(bool bSend)
{
	DWORD now = get_global_time();

	while (m_Horse.dwHorseHealthDropTime < now)
	{
		m_Horse.dwHorseHealthDropTime += HORSE_HEALTH_DROP_INTERVAL;
		UpdateHorseHealth(-1, bSend);
	}
}

void CHorseRider::UpdateHorseHealth(int iHealth, bool bSend)
{
	int level = GetHorseLevel();

	m_Horse.sHealth = MINMAX(0, m_Horse.sHealth + iHealth, c_aHorseStat[level].iMaxHealth);

	if (bSend)
		SendHorseInfo();
}

void CHorseRider::SetHorseLevel(int iLevel)
{
	m_Horse.bLevel = iLevel = MINMAX(0, iLevel, HORSE_MAX_LEVEL);

	m_Horse.sStamina = c_aHorseStat[iLevel].iMaxStamina;
	m_Horse.sHealth = c_aHorseStat[iLevel].iMaxHealth;
	m_Horse.dwHorseHealthDropTime = 0;

	ResetHorseHealthDropTime();

	SendHorseInfo();
}

BYTE CHorseRider::GetHorseGrade()
{
	BYTE grade = 0;

	if (GetHorseLevel())
		grade = (GetHorseLevel() - 1) / 10 + 1;

	return grade;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
