#include "common.h"
#include "utils.h"
#include "cdetour.h"
#include "module.h"
#include "detours.h"
#include "utils/simplecmds.h"

#include "movement/movement.h"

#include "tier0/memdbgon.h"
extern CEntitySystem* g_pEntitySystem;
CUtlVector<CDetourBase *> g_vecDetours;

DECLARE_DETOUR(Host_Say, Detour_Host_Say, &modules::server);
DECLARE_DETOUR(CBaseTrigger_StartTouch, Detour_CBaseTrigger_StartTouch, &modules::server);
DECLARE_DETOUR(CBaseTrigger_EndTouch, Detour_CBaseTrigger_EndTouch, &modules::server);
DECLARE_DETOUR(RecvServerBrowserPacket, Detour_RecvServerBrowserPacket, &modules::steamnetworkingsockets);


DECLARE_MOVEMENT_DETOUR(GetMaxSpeed);
DECLARE_MOVEMENT_DETOUR(ProcessMovement);
DECLARE_MOVEMENT_DETOUR(PlayerMoveNew);
DECLARE_MOVEMENT_DETOUR(CheckParameters);
DECLARE_MOVEMENT_DETOUR(CanMove);
DECLARE_MOVEMENT_DETOUR(FullWalkMove);
DECLARE_MOVEMENT_DETOUR(MoveInit);
DECLARE_MOVEMENT_DETOUR(CheckWater);
DECLARE_MOVEMENT_DETOUR(CheckVelocity);
DECLARE_MOVEMENT_DETOUR(Duck);
DECLARE_MOVEMENT_DETOUR(LadderMove);
DECLARE_MOVEMENT_DETOUR(CheckJumpButton);
DECLARE_MOVEMENT_DETOUR(OnJump);
DECLARE_MOVEMENT_DETOUR(AirAccelerate);
DECLARE_MOVEMENT_DETOUR(Friction);
DECLARE_MOVEMENT_DETOUR(WalkMove);
DECLARE_MOVEMENT_DETOUR(TryPlayerMove);
DECLARE_MOVEMENT_DETOUR(CategorizePosition);
DECLARE_MOVEMENT_DETOUR(FinishGravity);
DECLARE_MOVEMENT_DETOUR(CheckFalling);
DECLARE_MOVEMENT_DETOUR(PlayerMovePost);
DECLARE_MOVEMENT_DETOUR(PostThink);

void InitDetours()
{
	g_vecDetours.RemoveAll();
	INIT_DETOUR(Host_Say);
	INIT_DETOUR(CBaseTrigger_StartTouch);
	INIT_DETOUR(CBaseTrigger_EndTouch);
	INIT_DETOUR(RecvServerBrowserPacket);
}

void FlushAllDetours()
{
	FOR_EACH_VEC(g_vecDetours, i)
	{
		g_vecDetours[i]->FreeDetour();
	}
	
	g_vecDetours.RemoveAll();
}

void Detour_Host_Say(CCSPlayerController *pEntity, const CCommand *args, bool teamonly, uint32_t nCustomModRules, const char *pszCustomModPrepend)
{
	DEBUG_PRINT(Detour_Host_Say);
	META_RES mres = scmd::OnHost_Say(pEntity, *args);
	if (mres != MRES_SUPERCEDE)
	{
		Host_Say(pEntity, args, teamonly, nCustomModRules, pszCustomModPrepend);
	}
	DEBUG_PRINT_POST(Detour_Host_Say);
}

bool IsEntTriggerMultiple(CBaseEntity *ent)
{
	bool result = false;
	if (ent && ent->m_pEntity)
	{
		const char *classname = ent->m_pEntity->m_designerName.String();
		result = classname && stricmp(classname, "trigger_multiple") == 0;
	}
	return result;
}

bool IsTriggerStartZone(CBaseTrigger *trigger)
{
	bool result = false;
	if (trigger && trigger->m_pEntity)
	{
		const char *targetname = trigger->m_pEntity->m_name.String();
		result = targetname && stricmp(targetname, "timer_startzone") == 0;
	}
	return result;
}

bool IsTriggerEndZone(CBaseTrigger *trigger)
{
	bool result = false;
	if (trigger && trigger->m_pEntity)
	{
		const char *targetname = trigger->m_pEntity->m_name.String();
		result = targetname && stricmp(targetname, "timer_endzone") == 0;
	}
	return result;
}

void FASTCALL Detour_CBaseTrigger_StartTouch(CBaseTrigger *this_, CBaseEntity *pOther)
{
	DEBUG_PRINT(Detour_CBaseTrigger_StartTouch);
	CBaseTrigger_StartTouch(this_, pOther);
	
	if (utils::IsEntityPawn(pOther))
	{
		if (IsEntTriggerMultiple((CBaseEntity *)this_))
		{
			MovementPlayer *player = g_pPlayerManager->ToPlayer((CBasePlayerPawn *)pOther);
			if (IsTriggerStartZone(this_))
			{
				player->StartZoneStartTouch();
			}
			else if (IsTriggerEndZone(this_))
			{
				player->EndZoneStartTouch();
			}
		}
	}
	DEBUG_PRINT_POST(Detour_CBaseTrigger_StartTouch);
}

void FASTCALL Detour_CBaseTrigger_EndTouch(CBaseTrigger *this_, CBaseEntity *pOther)
{
	DEBUG_PRINT(Detour_CBaseTrigger_EndTouch);
	CBaseTrigger_EndTouch(this_, pOther);

	if (!pOther) return;
	if (utils::IsEntityPawn(pOther))
	{
		if (IsEntTriggerMultiple((CBaseEntity *)this_))
		{
			MovementPlayer *player = g_pPlayerManager->ToPlayer((CBasePlayerPawn *)pOther);
			if (IsTriggerStartZone(this_))
			{
				player->StartZoneEndTouch();
			}
		}
	}
	DEBUG_PRINT_POST(Detour_CBaseTrigger_EndTouch);
}

int FASTCALL Detour_RecvServerBrowserPacket(RecvPktInfo_t &info, void* pSock)
{
	DEBUG_PRINT(Detour_RecvServerBrowserPacket);
	int retValue = RecvServerBrowserPacket(info, pSock);
	// META_CONPRINTF("Detour_RecvServerBrowserPacket: Message received from %i.%i.%i.%i:%i, returning %i\nPayload: %s\n", 
	// 	info.m_adrFrom.m_IPv4Bytes.b1, info.m_adrFrom.m_IPv4Bytes.b2, info.m_adrFrom.m_IPv4Bytes.b3, info.m_adrFrom.m_IPv4Bytes.b4, 
	// 	info.m_adrFrom.m_usPort, retValue, (char*)info.m_pPkt);
	DEBUG_PRINT_POST(Detour_RecvServerBrowserPacket);
	return retValue;
}