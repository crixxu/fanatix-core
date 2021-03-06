/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Garr
SD%Complete: 90
SDComment: ready for testing
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

// Garr spells
#define SPELL_ANTIMAGICPULSE        19492
#define SPELL_MAGMASHACKLES         19496
#define SPELL_ENRAGE                19516   //Stacking enrage (stacks to 10 times)

//Add spells
#define SPELL_ERUPTION              19497
#define SPELL_IMMOLATE              20294

struct MANGOS_DLL_DECL boss_garrAI : public ScriptedAI
{
    boss_garrAI(Creature *c) : ScriptedAI(c)
	{
		pInstance = ((ScriptedInstance*)c->GetInstanceData());
		Reset();
	}
	ScriptedInstance *pInstance;

    uint32 AntiMagicPulse_Timer;
    uint32 MagmaShackles_Timer;

    bool Enraged[7];

    void Reset()
    {
        AntiMagicPulse_Timer = 25000;      //These times are probably wrong
        MagmaShackles_Timer = 15000;

		for(int i=0;i<7;i++)
		{
			Enraged[i]=false;
		}
    }

    void Aggro(Unit *who)
    {
    }

	void JustDied(Unit* Killer)
    {
		if(pInstance)
			pInstance->SetData(DATA_GARR_DEAD,0);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;
		
		//Cast Enrage for dead Adds
        if(pInstance)
		{
			for(int i=0;i<=pInstance->GetData(DATA_FIRESWORNISDEAD);i++)
			{
				if(Enraged[i] == false)
				{
					DoCast(m_creature,SPELL_ENRAGE);
					Enraged[i] = true;
				}
			}
		}

        //AntiMagicPulse_Timer
        if (AntiMagicPulse_Timer < diff)
        {
            //Cast
            DoCast(m_creature,SPELL_ANTIMAGICPULSE);

            //14-18 seconds until we should cast this agian
            AntiMagicPulse_Timer = 10000 + rand()%5000;
        }else AntiMagicPulse_Timer -= diff;

        //MagmaShackles_Timer
        if (MagmaShackles_Timer < diff)
        {
            //Cast
            DoCast(m_creature,SPELL_MAGMASHACKLES);

            //8-12 seconds until we should cast this agian
            MagmaShackles_Timer = 8000 + rand()%4000;
        }else MagmaShackles_Timer -= diff;

        DoMeleeAttackIfReady();
    }
}; 


struct MANGOS_DLL_DECL mob_fireswornAI : public ScriptedAI
{
    mob_fireswornAI(Creature *c) : ScriptedAI(c)
	{
		pInstance = ((ScriptedInstance*)c->GetInstanceData());
		Reset();
	}
	ScriptedInstance *pInstance;

    uint32 Immolate_Timer;

	bool Erruption;

    void Reset()
    {
        Immolate_Timer = 4000;      //These times are probably wrong

		Erruption = true;

        //m_creature->RemoveAllAuras();
        //m_creature->DeleteThreatList();
        //m_creature->CombatStop();
        //DoGoHome();
    }

    void Aggro(Unit *who)
    {
    }

    void MoveInLineOfSight(Unit *who)
    {
    }

	void JustDied(Unit* Killer)
    {
        if(pInstance)
			pInstance->SetData(DATA_FIRESWORN_DEATH,0);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        //Immolate_Timer
        if (Immolate_Timer < diff)
        {
            //Cast
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM,0))
                DoCast(target,SPELL_IMMOLATE);

            //5-10 seconds until we should cast this agian
            Immolate_Timer = 5000 + rand()%5000;
        }else Immolate_Timer -= diff;

        //Cast Erruption and let them die
        if (m_creature->GetHealth() <= m_creature->GetMaxHealth() * 0.10 && Erruption)
        {
			if(rand()<50)
			{
				//Cast
				DoCast(m_creature->getVictim(),SPELL_ERUPTION);
				m_creature->setDeathState(JUST_DIED);
				m_creature->RemoveCorpse();
			}else Erruption = false;
        }

        DoMeleeAttackIfReady();
    }
}; 
CreatureAI* GetAI_boss_garr(Creature *_Creature)
{
    return new boss_garrAI (_Creature);
}
CreatureAI* GetAI_mob_firesworn(Creature *_Creature)
{
    return new mob_fireswornAI (_Creature);
}



void AddSC_boss_garr()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_garr";
    newscript->GetAI = &GetAI_boss_garr;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_firesworn";
    newscript->GetAI = &GetAI_mob_firesworn;
    newscript->RegisterSelf();

}
