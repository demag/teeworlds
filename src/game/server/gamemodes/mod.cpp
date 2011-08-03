/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>

#include "mod.h"

#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include <game/server/entities/character.h>

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "DDWar";
	m_NextBroadcastTick = 0;
	m_NextAnnounceTick = 0;
	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();

	if (*g_Config.m_SvAnnouncement && m_NextAnnounceTick <= Server()->Tick())
	{
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, g_Config.m_SvAnnouncement);
		m_NextAnnounceTick = Server()->Tick() + g_Config.m_SvAnnouncementInterval * 60 * Server()->TickSpeed();
	}

	if (*g_Config.m_SvBroadcast && m_NextBroadcastTick <= Server()->Tick())
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (GameServer()->IsClientReady(i))
				GameServer()->SendBroadcast(g_Config.m_SvBroadcast, i);
		}
		m_NextBroadcastTick = Server()->Tick() + (Server()->TickSpeed()<<1);
	}

	static int LastBounceTick[MAX_CLIENTS] = {0};
	static int RegainHammerTick[MAX_CLIENTS] = {0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if (!pChr || LastBounceTick[i] + g_Config.m_SvBounceDelay > Server()->Tick())
			continue;
		if (GameServer()->Collision()->GetCollisionAt(pChr->m_Pos.x, pChr->m_Pos.y) == TILE_BOUNCE)
		{
			GameServer()->CreateExplosion(pChr->m_Pos, i, WEAPON_GRENADE, true);
			GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_EXPLODE);
			pChr->TakeDamage(vec2(g_Config.m_SvBounceXforce/10.0f, g_Config.m_SvBounceYforce/10.0f), 0, i, WEAPON_GRENADE);
			LastBounceTick[i] = Server()->Tick();
		}

		if (pChr->GetHammerScore() > g_Config.m_SvHammerThreshold)
		{
			pChr->TakeWeapon(WEAPON_HAMMER);
			pChr->SetHammerScore(0);
			RegainHammerTick[i] = Server()->Tick() + g_Config.m_SvHammerPunishtime * Server()->TickSpeed();
			char aBuf[128];
			str_format(aBuf, sizeof aBuf, "WARNING: Your hammer has been taken away for %d seconds, because you are overusing it.", g_Config.m_SvHammerPunishtime);
			GameServer()->SendChatTarget(i, aBuf);
		}
		if (RegainHammerTick[i] > 0 && RegainHammerTick[i] <= Server()->Tick())
		{
			RegainHammerTick[i] = 0;
			pChr->GiveWeapon(WEAPON_HAMMER, -1);
			pChr->SetWeapon(WEAPON_HAMMER); // XXX
			GameServer()->SendChatTarget(i, "Here you got your hammer back, this time do not overuse it!");
		}
	}
}
