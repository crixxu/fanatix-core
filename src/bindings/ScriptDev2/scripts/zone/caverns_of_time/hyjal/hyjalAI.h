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

#include "def_hyjal.h"

// Trash Mobs summoned in waves
#define NECROMANCER         17899
#define ABOMINATION         17898
#define GHOUL               17895
#define BANSHEE             17905
#define CRYPT_FIEND         17897
#define GARGOYLE            17906
#define FROST_WYRM          17907
#define GIANT_INFERNAL      17908
#define FEL_STALKER         17916

// Bosses summoned after every 8 waves
#define RAGE_WINTERCHILL    17767
#define ANETHERON           17808
#define KAZROGAL            17888
#define AZGALOR             17842
#define ARCHIMONDE          17968

#define SPELL_TELEPORT_VISUAL   41232

#define WORLDSTATE_WAVES        2842
#define WORLDSTATE_ENEMY        2453
#define WORLDSTATE_ENEMYCOUNT   2454

/*** Spells for Jaina ***/
#define SPELL_BRILLIANCE_AURA     31260 // The database must handle this spell via creature_addon
#define SPELL_BLIZZARD            31266
#define SPELL_PYROBLAST           31263
#define SPELL_SUMMON_ELEMENTALS   31264

/** Thrall spells **/
#define SPELL_CHAIN_LIGHTNING     31330
#define SPELL_SUMMON_DIRE_WOLF    31331

struct Wave
{
    uint32 Mob[18]; // Stores Creature Entries to be summoned in Waves
    uint32 WaveTimer; // The timer before the next wave is summoned
    bool IsBoss; // Simply used to inform the wave summoner that the next wave contains a boss to halt all waves after that
};

static Wave AllianceWaves[]= // Waves that will be summoned in the Alliance Base
{
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, 0, 0, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 1
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 2
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 3
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 4
    {GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 5
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, 0, 0, 0, 0, 0, 120000, false}, // Wave 6
    {GHOUL, GHOUL, GHOUL, GHOUL, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 7
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 180000, false}, // Wave 8
    {RAGE_WINTERCHILL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, true}, // All 8 Waves are summoned, summon Rage Winterchill, next few waves are for Anetheron
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, 0, 0, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 1
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 2
    {GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 3
    {GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, BANSHEE, BANSHEE, BANSHEE, BANSHEE, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 4
    {CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, BANSHEE, BANSHEE, BANSHEE, BANSHEE, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 5
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 6
    {CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, BANSHEE, BANSHEE, BANSHEE, BANSHEE, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 7
    {GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, BANSHEE, BANSHEE, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 180000, false}, // Wave 8
    {ANETHERON, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, true} // All 8 Waves are summoned, summon Anatheron
};

static Wave HordeWaves[]= // Waves that are summoned in the Horde base
{
    {GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, BANSHEE, BANSHEE, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 1
    {GHOUL, GHOUL, GHOUL, GHOUL, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, 0, 0, 0, 0, 120000, false}, // Wave 2
    {GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 120000, false}, // Wave 3
    {CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, NECROMANCER, NECROMANCER            , 0, 0, 0, 0, 120000, false}, // Wave 4
    {GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 5
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, NECROMANCER, FROST_WYRM, 0, 0, 120000, false}, // Wave 6
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, NECROMANCER, FROST_WYRM, 0, 0, 120000, false}, // Wave 7
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, BANSHEE, BANSHEE, 180000, false}, // Wave 8
    {KAZROGAL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, true}, // All 8 Waves are summoned, summon Kaz'Rogal, next few waves are for Azgalor
    {ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 1
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, FROST_WYRM, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, GARGOYLE, 0, 0, 0, 0, 120000, false}, // Wave 2
    {GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GHOUL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, 0, 0, 0, 0, 120000, false}, // Wave 3
    {GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, 0, 0, 0, 0, 120000, false}, // Wave 4
    {FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, FEL_STALKER, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, 0, 0, 0, 0, 120000, false}, // Wave 5
    {NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, NECROMANCER, BANSHEE, BANSHEE, BANSHEE, BANSHEE, BANSHEE, BANSHEE, 0, 0, 0, 0, 120000, false}, // Wave 6
    {GHOUL, GHOUL, CRYPT_FIEND, CRYPT_FIEND, FEL_STALKER, FEL_STALKER, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, GIANT_INFERNAL, 0, 0, 0, 0, 0, 0, 120000, false}, // Wave 7
    {CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, CRYPT_FIEND, FEL_STALKER, FEL_STALKER, ABOMINATION, ABOMINATION, ABOMINATION, ABOMINATION, BANSHEE, BANSHEE, BANSHEE, BANSHEE, NECROMANCER, NECROMANCER, 0, 0, 180000, false}, // Wave 8
    {AZGALOR, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, true} // All 8 Waves are summoned, summon Azgalor
};

enum TargetType // Used in the spell cast system for the AI
{
    TARGETTYPE_SELF     = 0,
    TARGETTYPE_RANDOM   = 1,
    TARGETTYPE_VICTIM   = 2,
};

struct Yells
{
    uint32 id; // Used to determine the type of yell (attack, rally, etc)
    char* text; // The text to be yelled
    uint32 sound; // Sound that corresponds to the text
};

enum YellId
{
    ATTACKED     = 0, // Used when attacked and set in combat
    BEGIN        = 1, // Used when the event is begun
    INCOMING     = 2, // Used to warn the raid that another wave phase is coming
    RALLY        = 3, // Used to rally the raid and warn that the next wave has been summoned
    FAILURE      = 4, // Used when raid has failed (unsure where to place)
    SUCCESS      = 5, // Used when the raid has sucessfully defeated a wave phase
    DEATH        = 6, // Used on death
};

static Yells JainaQuotes[]=
{
    {ATTACKED, "I'm in jeopardy, help me if you can!", 11007},
    {ATTACKED, "They've broken through!", 11049},
    {INCOMING, "Stay alert! Another wave approaches.", 11008},
    {BEGIN, "Hold them back as long as possible", 11050},
    {RALLY, "Don't give up! We must prevail!", 11006},
    {RALLY, "We must hold strong!", 11051},
    {FAILURE, "We are lost. Fall back!", 11009},
    {SUCCESS, "We have won valuable time. Now we must pull back!", 11011},
    {DEATH, "I did... my best.", 11010},
};

static Yells ThrallQuotes[]=
{ 
    {ATTACKED, "I will lie down for no one!", 11031},
    {ATTACKED, "Bring the fight to me and pay with your lives!", 11061},
    {INCOMING, "Make ready for another wave! LOK-TAR OGAR!", 11032},
    {BEGIN, "Do not give an inch of ground!", 11060},
    {RALLY, "Hold them back! Do not falter!", 11030},
    {RALLY, "Victory or death!", 11059},
    {RALLY, "Do not give an inch of ground!", 11060},
    {FAILURE, "It is over. Withdraw! We have failed.", 11033},
    {SUCCESS, "We have played our part and done well. It is up to the others now.", 11035},
    {DEATH, "Uraaa...", 11034},
};

struct MANGOS_DLL_DECL hyjalAI : public ScriptedAI
{
    hyjalAI(Creature *c);

    void Reset(); // Generically used to reset our variables. Do *not* call in EnterEvadeMode as this may make problems if the raid is still in combat

    void EnterEvadeMode(); // Send creature back to spawn location and evade.

    void Aggro(Unit *who); // Used to reset cooldowns for our spells and to inform the raid that we're under attack

    void UpdateAI(const uint32 diff); // Called to summon waves, check for boss deaths and to cast our spells.

    void JustDied(Unit* killer); // Called on death, informs the raid that they have failed.

    void SetFaction(uint32 _faction) { Faction = _faction; } // Set the faction to either Alliance or Horde in Hyjal

    void TeleportRaid(Player *player, float X, float Y, float Z); // Used by gossip to teleport the entire raid to the next location

    void SummonCreature(uint32 entry, float Base[4][3]); // Summons a creature for that wave in that base
    
    void SummonNextWave(Wave wave[18], uint32 Count, float Base[4][3]); // Summons the next wave, calls SummonCreature

    void StartEvent(Player* player); // Begins the event by gossip click

    uint32 GetInstanceData(uint32 Event); // Gets instance data for this instance, used to check if raid has gotten past a certain point and can access the next phase

    void Talk(uint32 id); // Searches for the appropriate yell and sound and uses it to inform the raid of various things

    void UpdateWorldState(uint32 field, uint32 value); // NYI: Requires core support. Updates the world state counter at the top of the UI.
public:    
    ScriptedInstance* pInstance;

    uint64 PlayerGUID;
    uint64 BossGUID[2];

    uint32 NextWaveTimer;
    uint32 WaveCount;
    uint32 CheckBossTimer;
    uint32 Faction;
    uint32 EnemyCount;

    bool EventBegun;
    bool FirstBossDead;
    bool SecondBossDead;
    bool Summon;

    struct Spell
    {
        uint32 SpellId;
        uint32 Cooldown;
        uint32 TargetType;
    }Spell[3];

private:
    uint32 SpellTimer[3];
};
