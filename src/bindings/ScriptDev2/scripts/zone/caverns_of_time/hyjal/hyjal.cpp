/* Copyright (C) 2006,2007 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* ScriptData
SDName: Npc_Hyjal
SD%Complete: 100
SDComment: 
SDCategory: Caverns of Time, Mount Hyjal
EndScriptData */

#include "precompiled.h"
#include "hyjalAI.h"

#define GOSSIP_ITEM_BEGIN_ALLY  "We are ready to defend the Alliance base."
#define GOSSIP_ITEM_ANETHERON   "The defenses are holding up; we can continue."
#define GOSSIP_ITEM_RETREAT     "We can't keep this up. Let's retreat!"

#define GOSSIP_ITEM_BEGIN_HORDE "We're here to help! The Alliance are overrun."
#define GOSSIP_ITEM_AZGALOR     "We're okay so far. Let's do this!"

#define HORDEBASE_X         5464.5522
#define HORDEBASE_Y         -2731.5644
#define HORDEBASE_Z         1485.7075

#define NIGHTELFBASE_X      5186.07
#define NIGHTELFBASE_Y      -3383.49
#define NIGHTELFBASE_Z      1638.28

CreatureAI* GetAI_npc_jaina_proudmoore(Creature *_Creature)
{
    hyjalAI* ai = new hyjalAI(_Creature);

    ai->Reset();
    ai->EnterEvadeMode();

    ai->Spell[0].SpellId = SPELL_BLIZZARD;
    ai->Spell[0].Cooldown = 15000 + rand()%20000;
    ai->Spell[0].TargetType = TARGETTYPE_RANDOM;
    
    ai->Spell[1].SpellId = SPELL_PYROBLAST;
    ai->Spell[1].Cooldown = 2000 + rand()%7000;
    ai->Spell[1].TargetType = TARGETTYPE_RANDOM;

    ai->Spell[2].SpellId = SPELL_SUMMON_ELEMENTALS;
    ai->Spell[2].Cooldown = 15000 + rand()%30000;
    ai->Spell[2].TargetType = TARGETTYPE_SELF;

    return ai;
}

bool GossipHello_npc_jaina_proudmoore(Player *player, Creature *_Creature)
{
    if((!((hyjalAI*)_Creature->AI())->EventBegun) && (!((hyjalAI*)_Creature->AI())->FirstBossDead))
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_BEGIN_ALLY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    else
    {
        if((!((hyjalAI*)_Creature->AI())->SecondBossDead))
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_ANETHERON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        else
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_RETREAT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
    }
    player->PlayerTalkClass->SendGossipMenu(907, _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_jaina_proudmoore(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    player->PlayerTalkClass->GetGossipMenu();
    switch(action)
    {
        case GOSSIP_ACTION_INFO_DEF + 1:
            ((hyjalAI*)_Creature->AI())->StartEvent(player);
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            ((hyjalAI*)_Creature->AI())->StartEvent(player);
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            ((hyjalAI*)_Creature->AI())->TeleportRaid(player, HORDEBASE_X, HORDEBASE_Y, HORDEBASE_Z);
            break;
    }

    return true;
}

CreatureAI* GetAI_npc_thrall(Creature *_Creature)
{
    hyjalAI* ai = new hyjalAI(_Creature);

    ai->Reset();
    ai->EnterEvadeMode();

    ai->Spell[0].SpellId = SPELL_CHAIN_LIGHTNING;
    ai->Spell[0].Cooldown = 2000 + rand()%5000;
    ai->Spell[0].TargetType = TARGETTYPE_VICTIM;

    ai->Spell[1].SpellId = SPELL_SUMMON_DIRE_WOLF;
    ai->Spell[1].Cooldown = 6000 + rand()%35000;
    ai->Spell[1].TargetType = TARGETTYPE_RANDOM;

    return ai;
}

bool GossipHello_npc_thrall(Player *player, Creature *_Creature)
{
    uint32 AnetheronEvent = 0;
    AnetheronEvent = ((hyjalAI*)_Creature->AI())->GetInstanceData(DATA_ANETHERONEVENT);
    if(AnetheronEvent >= DONE) // Only let them start the Horde phase if Anetheron is dead.
    {
        if((((hyjalAI*)_Creature->AI())->EventBegun) && (!((hyjalAI*)_Creature->AI())->FirstBossDead))
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_BEGIN_HORDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        else
        {
            if((!((hyjalAI*)_Creature->AI())->SecondBossDead))
                player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_AZGALOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            else
                player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_RETREAT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
    }

    player->PlayerTalkClass->SendGossipMenu(907, _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_thrall(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    player->PlayerTalkClass->GetGossipMenu();
    switch(action)
    {
        case GOSSIP_ACTION_INFO_DEF + 1:
            ((hyjalAI*)_Creature->AI())->StartEvent(player);
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            ((hyjalAI*)_Creature->AI())->StartEvent(player);
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            ((hyjalAI*)_Creature->AI())->TeleportRaid(player, NIGHTELFBASE_X, NIGHTELFBASE_Y, NIGHTELFBASE_Z);
            break;
    }

    return true;
}

bool GossipHello_npc_tyrande_whisperwind(Player* player, Creature* _Creature)
{
    player->ADD_GOSSIP_ITEM(1, "Aid us in defending Nordrassil", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
    player->PlayerTalkClass->SendGossipMenu(907, _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_tyrande_whisperwind(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    if(action == GOSSIP_ACTION_TRADE)
        player->SEND_VENDORLIST( _Creature->GetGUID() );

    return true;
}

void AddSC_hyjal()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_jaina_proudmoore";
    newscript->GetAI = GetAI_npc_jaina_proudmoore;
    newscript->pGossipHello = &GossipHello_npc_jaina_proudmoore;
    newscript->pGossipSelect = &GossipSelect_npc_jaina_proudmoore;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_thrall";
    newscript->GetAI = GetAI_npc_thrall;
    newscript->pGossipHello = &GossipHello_npc_thrall;
    newscript->pGossipSelect = &GossipSelect_npc_thrall;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_tyrande_whisperwind";
    newscript->pGossipHello = &GossipHello_npc_tyrande_whisperwind;
    newscript->pGossipSelect = &GossipSelect_npc_tyrande_whisperwind;
    newscript->RegisterSelf();
}
