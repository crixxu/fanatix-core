/*
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
 
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "Vehicle.h"
#include "MapManager.h"
#include "Transports.h"
#include "BattleGround.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "World.h"
 
//#define __ANTI_DEBUG__
 
#ifdef __ANTI_DEBUG__
#include "Chat.h"
std::string FlagsToStr(const uint32 Flags)
{
    std::string Ret="";
    if(Flags==0)
    {
        Ret="None";
        return Ret;
    }
    
    if(Flags & MOVEMENTFLAG_FORWARD)
    { Ret+="FW "; }
    if(Flags & MOVEMENTFLAG_BACKWARD)
    { Ret+="BW "; }
    if(Flags & MOVEMENTFLAG_STRAFE_LEFT)
    { Ret+="STL "; }
    if(Flags & MOVEMENTFLAG_STRAFE_RIGHT)
    { Ret+="STR "; }
    if(Flags & MOVEMENTFLAG_LEFT)
    { Ret+="LF "; }
    if(Flags & MOVEMENTFLAG_RIGHT)
    { Ret+="RI "; }
    if(Flags & MOVEMENTFLAG_PITCH_UP)
    { Ret+="PTUP "; }
    if(Flags & MOVEMENTFLAG_PITCH_DOWN)
    { Ret+="PTDW "; }
    if(Flags & MOVEMENTFLAG_WALK_MODE)
    { Ret+="WALK "; }
    if(Flags & MOVEMENTFLAG_ONTRANSPORT)
    { Ret+="TRANS "; }
    if(Flags & MOVEMENTFLAG_LEVITATING)
    { Ret+="LEVI "; }
    if(Flags & MOVEMENTFLAG_FLY_UNK1)
    { Ret+="FLYUNK1 "; }
    if(Flags & MOVEMENTFLAG_JUMPING)
    { Ret+="JUMP "; }
    if(Flags & MOVEMENTFLAG_UNK4)
    { Ret+="UNK4 "; }
    if(Flags & MOVEMENTFLAG_FALLING)
    { Ret+="FALL "; }
    if(Flags & MOVEMENTFLAG_SWIMMING)
    { Ret+="SWIM "; }
    if(Flags & MOVEMENTFLAG_FLY_UP)
    { Ret+="FLYUP "; }
    if(Flags & MOVEMENTFLAG_CAN_FLY)
    { Ret+="CFLY "; }
    if(Flags & MOVEMENTFLAG_FLYING)
    { Ret+="FLY "; }
    if(Flags & MOVEMENTFLAG_FLYING2)
    { Ret+="FLY2 "; }
    if(Flags & MOVEMENTFLAG_WATERWALKING)
    { Ret+="WTWALK "; }
    if(Flags & MOVEMENTFLAG_SAFE_FALL)
    { Ret+="SAFE "; }
    if(Flags & MOVEMENTFLAG_UNK3)
    { Ret+="UNK3 "; }
    if(Flags & MOVEMENTFLAG_SPLINE)
    { Ret+="SPLINE "; }
    if(Flags & MOVEMENTFLAG_SPLINE2)
    { Ret+="SPLINE2 "; }
    
    return Ret;
}
#endif // __ANTI_DEBUG__
 
bool WorldSession::Anti__ReportCheat(const char* Reason,float Speed,const char* Op,float Val1,uint32 Val2,MovementInfo* MvInfo)
{
    if(!Reason)
    {
        sLog.outError("Anti__ReportCheat: Missing Reason parameter!");
        return false;
    }
    const char* Player=GetPlayer()->GetName();
    uint32 Acc=GetPlayer()->GetSession()->GetAccountId();
    uint32 Map=GetPlayer()->GetMapId();
    if(!Player)
    {
        sLog.outError("Anti__ReportCheat: Player with no name?!?");
        return false;
    }
 
    QueryResult *Res=CharacterDatabase.PQuery("SELECT speed,Val1 FROM cheaters WHERE player='%s' AND reason LIKE '%s' AND Map='%u' AND last_date >= NOW()-300",Player,Reason,Map);
    if(Res)
    {
        Field* Fields = Res->Fetch();
        
        std::stringstream Query;
        Query << "UPDATE cheaters SET count=count+1,last_date=NOW()";
        Query.precision(5);
        if(Speed>0.0f && Speed > Fields[0].GetFloat())
        {
            Query << ",speed='";
            Query << std::fixed << Speed;
            Query << "'";
        }
 
        if(Val1>0.0f && Val1 > Fields[1].GetFloat())
        {
            Query << ",Val1='";
            Query << std::fixed << Val1;
            Query << "'";
        }
        
        Query << " WHERE player='" << Player << "' AND reason='" << Reason << "' AND Map='" << Map << "' AND last_date >= NOW()-300 ORDER BY entry DESC LIMIT 1";
        
        CharacterDatabase.Execute(Query.str().c_str());
        delete Res;
    }
    else
    {
        if(!Op)
        { Op=""; }
        std::stringstream Pos;
        Pos << "OldPos: " << GetPlayer()->GetPositionX() << " " << GetPlayer()->GetPositionY() << " "
            << GetPlayer()->GetPositionZ();
        if(MvInfo)
        {
            Pos << "\nNew: " << MvInfo->x << " " << MvInfo->y << " " << MvInfo->z << "\n"
                << "Flags: " << MvInfo->flags << "\n"
                << "t_guid: " << MvInfo->t_guid << " falltime: " << MvInfo->fallTime;
        }
        CharacterDatabase.PExecute("INSERT INTO cheaters (player,acctid,reason,speed,count,first_date,last_date,`Op`,Val1,Val2,Map,Pos,Level) "
                                   "VALUES ('%s','%u','%s','%f','1',NOW(),NOW(),'%s','%f','%u','%u','%s','%u')",
                                   Player,Acc,Reason,Speed,Op,Val1,Val2,Map,
                                   Pos.str().c_str(),GetPlayer()->getLevel());
    }
 
    if(sWorld.GetMvAnticheatKill() && GetPlayer()->isAlive())
    {
        GetPlayer()->DealDamage(GetPlayer(), GetPlayer()->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
    }
    if(sWorld.GetMvAnticheatKick())
    {
        GetPlayer()->GetSession()->KickPlayer();
    }
    if(sWorld.GetMvAnticheatBan() & 1)
    {
        sWorld.BanAccount(BAN_CHARACTER,Player,sWorld.GetMvAnticheatBanTime(),"Cheat","Anticheat");
    }
    if(sWorld.GetMvAnticheatBan() & 2)
    {
        QueryResult *result = loginDatabase.PQuery("SELECT last_ip FROM account WHERE id=%u", Acc);
        if(result)
        {
 
            Field *fields = result->Fetch();
            std::string LastIP = fields[0].GetCppString();
            if(!LastIP.empty())
            {
                sWorld.BanAccount(BAN_IP,LastIP,sWorld.GetMvAnticheatBanTime(),"Cheat","Anticheat");
            }
            delete result;
        }
    }
    return true;
}
 
bool WorldSession::Anti__CheatOccurred(uint32 CurTime,const char* Reason,float Speed,const char* Op,
                                float Val1,uint32 Val2,MovementInfo* MvInfo)
{
    if(!Reason)
    {
        sLog.outError("Anti__CheatOccurred: Missing Reason parameter!");
        return false;
    }
    
    GetPlayer()->m_anti_lastalarmtime = CurTime;
    GetPlayer()->m_anti_alarmcount = GetPlayer()->m_anti_alarmcount + 1;
 
    if (GetPlayer()->m_anti_alarmcount > sWorld.GetMvAnticheatAlarmCount())
    {
        Anti__ReportCheat(Reason,Speed,Op,Val1,Val2,MvInfo);
        return true;
    }
    return false;
}
 
 
void WorldSession::HandleMoveWorldportAckOpcode( WorldPacket & /*recv_data*/ )
{
    sLog.outDebug( "WORLD: got MSG_MOVE_WORLDPORT_ACK." );
    HandleMoveWorldportAckOpcode();
}
 
void WorldSession::HandleMoveWorldportAckOpcode()
{
    // get the teleport destination
    WorldLocation &loc = GetPlayer()->GetTeleportDest();
 
    // possible errors in the coordinate validity check
    if(!MapManager::IsValidMapCoord(loc.mapid,loc.x,loc.y,loc.z,loc.o))
    {
        LogoutPlayer(false);
        return;
    }
 
    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.mapid);
    InstanceTemplate const* mInstance = objmgr.GetInstanceTemplate(loc.mapid);
 
    // reset instance validity, except if going to an instance inside an instance
    if(GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;
 
    GetPlayer()->SetSemaphoreTeleport(false);
 
    // relocate the player to the teleport destination
    GetPlayer()->SetMapId(loc.mapid);
    GetPlayer()->Relocate(loc.x, loc.y, loc.z, loc.o);
    GetPlayer()->m_anti_TeleTime=time(NULL);
 
    // since the MapId is set before the GetInstance call, the InstanceId must be set to 0
    // to let GetInstance() determine the proper InstanceId based on the player's binds
    GetPlayer()->SetInstanceId(0);
 
    // check this before Map::Add(player), because that will create the instance save!
    bool reset_notify = (GetPlayer()->GetBoundInstance(GetPlayer()->GetMapId(), GetPlayer()->GetDifficulty()) == NULL);
 
    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if(!GetPlayer()->GetMap()->Add(GetPlayer()))
    {
        sLog.outDebug("WORLD: teleport of player %s (%d) to location %d,%f,%f,%f,%f failed", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.mapid, loc.x, loc.y, loc.z, loc.o);
        // teleport the player home
        GetPlayer()->SetDontMove(false);
        if(!GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation()))
        {
            // the player must always be able to teleport home
            sLog.outError("WORLD: failed to teleport player %s (%d) to homebind location %d,%f,%f,%f,%f!", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
            assert(false);
        }
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if(_player->InBattleGround())
    {
        // cleanup seting if outdated
        if(!mEntry->IsBattleGroundOrArena())
        {
            _player->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE); // We're not in BG.
            // reset destination bg team
            _player->SetBGTeam(0);
        }
        // join to bg case
        else if(BattleGround *bg = _player->GetBattleGround())
        {
            if(_player->IsInvitedForBattleGroundInstance(_player->GetBattleGroundId()))
                bg->AddPlayer(_player);
        }
    }
 
    GetPlayer()->SendInitialPacketsAfterAddToMap();
 
    // flight fast teleport case
    if(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()==FLIGHT_MOTION_TYPE)
    {
        if(!_player->InBattleGround())
        {
            // short preparations to continue flight
            GetPlayer()->SetDontMove(false);
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Initialize(*GetPlayer());
            return;
        }
 
        // battleground state prepare, stop flight
        GetPlayer()->GetMotionMaster()->MovementExpired();
        GetPlayer()->m_taxi.ClearTaxiDestinations();
    }
 
    // resurrect character at enter into instance where his corpse exist after add to map
    Corpse *corpse = GetPlayer()->GetCorpse();
    if (corpse && corpse->GetType() != CORPSE_BONES && corpse->GetMapId() == GetPlayer()->GetMapId())
    {
        if( mEntry->IsDungeon() )
        {
            GetPlayer()->ResurrectPlayer(0.5f);
            GetPlayer()->SpawnCorpseBones();
            GetPlayer()->SaveToDB();
        }
    }
 
    if(mEntry->IsRaid() && mInstance)
    {
        if(reset_notify)
        {
            uint32 timeleft = sInstanceSaveManager.GetResetTimeFor(GetPlayer()->GetMapId()) - time(NULL);
            GetPlayer()->SendInstanceResetWarning(GetPlayer()->GetMapId(), timeleft); // greeting at the entrance of the resort raid instance
        }
    }
 
    // mount allow check
    if(!mEntry->IsMountAllowed())
        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
 
    // honorless target
    if(GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);
 
    // resummon pet
    if(GetPlayer()->m_temporaryUnsummonedPetNumber)
    {
        Pet* NewPet = new Pet;
        if(!NewPet->LoadPetFromDB(GetPlayer(), 0, GetPlayer()->m_temporaryUnsummonedPetNumber, true))
            delete NewPet;
 
        GetPlayer()->m_temporaryUnsummonedPetNumber = 0;
    }
 
    GetPlayer()->SetDontMove(false);
}
 
void WorldSession::HandleMovementOpcodes( WorldPacket & recv_data )
{
    uint32 opcode = recv_data.GetOpcode();
    sLog.outDebug("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);
 
    if(GetPlayer()->GetDontMove())
        return;
 
    /* extract packet */
    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);
    /*----------------*/
 
    if(recv_data.size() != recv_data.rpos())
    {
        sLog.outError("MovementHandler: player %s (guid %d, account %u) sent a packet (opcode %u) that is %u bytes larger than it should be. Kicked as cheater.", _player->GetName(), _player->GetGUIDLow(), _player->GetSession()->GetAccountId(), recv_data.GetOpcode(), recv_data.size() - recv_data.rpos());
        KickPlayer();
        return;
    }
 
    if (!MaNGOS::IsValidMapCoord(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o))
        return;
 
    /* handle special cases */
    if (movementInfo.flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if( movementInfo.t_x > 50 || movementInfo.t_y > 50 || movementInfo.t_z > 50 )
            return;
 
        if( !MaNGOS::IsValidMapCoord(movementInfo.x+movementInfo.t_x, movementInfo.y+movementInfo.t_y,
            movementInfo.z+movementInfo.t_z, movementInfo.o+movementInfo.t_o) )
            return;
 
        if ((GetPlayer()->m_anti_transportGUID == 0) && (movementInfo.t_guid !=0))
        {
            // if we boarded a transport, add us to it
            if (!GetPlayer()->m_transport)
            {
                // elevators also cause the client to send MOVEMENTFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
                for (MapManager::TransportSet::iterator iter = MapManager::Instance().m_Transports.begin(); iter != MapManager::Instance().m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetGUID() == movementInfo.t_guid)
                    {
                        GetPlayer()->m_transport = (*iter);
                        (*iter)->AddPassenger(GetPlayer());
                        break;
                    }
                }
            }
            GetPlayer()->m_anti_transportGUID = movementInfo.t_guid;
        }
        else
        {
            float trans_rad = movementInfo.t_x*movementInfo.t_x + movementInfo.t_y*movementInfo.t_y + movementInfo.t_z*movementInfo.t_z;
            if (trans_rad > 3600.0f) // transport radius = 60 yards //cheater with on_transport_flag
            {
                if (GetPlayer()->m_transport)
                {
                    GetPlayer()->m_transport->RemovePassenger(GetPlayer());
                    GetPlayer()->m_transport = NULL;
                }
                movementInfo.t_x = 0.0f;
                movementInfo.t_y = 0.0f;
                movementInfo.t_z = 0.0f;
                movementInfo.t_o = 0.0f;
                movementInfo.t_time = 0;
                movementInfo.t_seat = -1;
                GetPlayer()->m_anti_transportGUID = 0;
            }
        }
    }
    else if (GetPlayer()->m_anti_transportGUID != 0) {
        if (GetPlayer()->m_transport) // if we were on a transport, leave
        {
            GetPlayer()->m_transport->RemovePassenger(GetPlayer());
            GetPlayer()->m_transport = NULL;
        }
        movementInfo.t_x = 0.0f;
        movementInfo.t_y = 0.0f;
        movementInfo.t_z = 0.0f;
        movementInfo.t_o = 0.0f;
        movementInfo.t_time = 0;
        movementInfo.t_seat = -1;
        GetPlayer()->m_anti_transportGUID = 0;
    }
 
    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == MSG_MOVE_FALL_LAND && !GetPlayer()->isInFlight())
        GetPlayer()->HandleFall(movementInfo);

    if(((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != GetPlayer()->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        GetPlayer()->SetInWater( !GetPlayer()->IsInWater() || GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.x, movementInfo.y, movementInfo.z) );
    }
 
    // ---- anti-cheat features -->>>
    uint32 Anti_TeleTimeDiff=time(NULL) - GetPlayer()->m_anti_TeleTime;
    static const uint32 Anti_TeleTimeIgnoreDiff=sWorld.GetMvAnticheatIgnoreAfterTeleport();
    if ((GetPlayer()->m_anti_transportGUID == 0) && sWorld.GetMvAnticheatEnable() &&
        GetPlayer()->GetSession()->GetSecurity() <= sWorld.GetMvAnticheatGmLevel() &&
        GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()!=FLIGHT_MOTION_TYPE &&
        Anti_TeleTimeDiff>Anti_TeleTimeIgnoreDiff)
    {
        const uint32 CurTime=getMSTime();
        if(getMSTimeDiff(GetPlayer()->m_anti_lastalarmtime,CurTime) > sWorld.GetMvAnticheatAlarmPeriod())
        {
            GetPlayer()->m_anti_alarmcount = 0;
        }
        /* I really don't care about movement-type yet (todo)
UnitMoveType move_type;
 
if (movementInfo.flags & MOVEMENTFLAG_FLYING) move_type = MOVE_FLY;
else if (movementInfo.flags & MOVEMENTFLAG_SWIMMING) move_type = MOVE_SWIM;
else if (movementInfo.flags & MOVEMENTFLAG_WALK_MODE) move_type = MOVE_WALK;
else move_type = MOVE_RUN;*/
 
        float delta_x = GetPlayer()->GetPositionX() - movementInfo.x;
        float delta_y = GetPlayer()->GetPositionY() - movementInfo.y;
        float delta_z = GetPlayer()->GetPositionZ() - movementInfo.z;
        float delta = sqrt(delta_x * delta_x + delta_y * delta_y); // Len of movement-vector via Pythagoras (a^2+b^2=Len)
        float tg_z = 0.0f; //tangens
        float delta_t = getMSTimeDiff(GetPlayer()->m_anti_lastmovetime,CurTime);
        GetPlayer()->m_anti_lastmovetime = CurTime;
        GetPlayer()->m_anti_MovedLen += delta;
 
        if(delta_t > 15000.0f)
        { delta_t = 15000.0f; }
        // Tangens of walking angel
        /*if (!(movementInfo.flags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)))
{
//Mount hack detection currently disabled
tg_z = ((delta !=0.0f) && (delta_z > 0.0f)) ? (atan((delta_z*delta_z) / delta) * 180.0f / M_PI) : 0.0f;
}*/
 
        //antiOFF fall-damage, MOVEMENTFLAG_UNK4 seted by client if player try movement when falling and unset in this case the MOVEMENTFLAG_FALLING flag.
        if((GetPlayer()->m_anti_BeginFallZ == INVALID_HEIGHT) &&
           (movementInfo.flags & (MOVEMENTFLAG_FALLING | MOVEMENTFLAG_UNK4)) != 0)
        {
            GetPlayer()->m_anti_BeginFallZ=(float)(movementInfo.z);
        }
 
        if(GetPlayer()->m_anti_NextLenCheck <= CurTime)
        {
            // Check every 500ms is a lot more advisable then 1000ms, because normal movment packet arrives every 500ms
            uint32 OldNextLenCheck=GetPlayer()->m_anti_NextLenCheck;
            float delta_xyt=GetPlayer()->m_anti_MovedLen/(float)(getMSTimeDiff(OldNextLenCheck-500,CurTime));
            GetPlayer()->m_anti_NextLenCheck=CurTime+500;
            GetPlayer()->m_anti_MovedLen=0.0f;
            static const float MaxDeltaXYT=sWorld.GetMvAnticheatMaxXYT();
#ifdef __ANTI_DEBUG__
            SendAreaTriggerMessage("XYT: %f ; Flags: %s",delta_xyt,FlagsToStr(movementInfo.flags).c_str());
#endif //__ANTI_DEBUG__
            if(delta_xyt>MaxDeltaXYT && delta<=100.0f)
            {
                Anti__CheatOccurred(CurTime,"Speed hack",delta_xyt,LookupOpcodeName(opcode),
                                    (float)(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()),
                                    (float)(getMSTimeDiff(OldNextLenCheck-500,CurTime)),&movementInfo);
            }
        }
        if(delta>100.0f)
        {
            Anti__ReportCheat("Tele hack",delta,LookupOpcodeName(opcode));
        }
 
        // Check for waterwalking
        if(((movementInfo.flags & MOVEMENTFLAG_WATERWALKING) != 0) &&
           ((movementInfo.flags ^ MOVEMENTFLAG_WATERWALKING) != 0) && // Client sometimes set waterwalk where it shouldn't do that...
           ((movementInfo.flags & MOVEMENTFLAG_JUMPING) == 0) &&
           GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.x, movementInfo.y, movementInfo.z-6.0f) &&
           !(GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK) || GetPlayer()->HasAuraType(SPELL_AURA_GHOST)))
        {
            Anti__CheatOccurred(CurTime,"Water walking",0.0f,NULL,0.0f,(uint32)(movementInfo.flags));
        }
        
        // Check for walking upwards a mountain while not beeing able to do that
        /*if ((tg_z > 85.0f))
{
Anti__CheatOccurred(CurTime,"Mount hack",tg_z,NULL,delta,delta_z);
}*/
        
        float Anti__GroundZ = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),MAX_HEIGHT);
        float Anti__FloorZ = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ());
        static const float DIFF_OVERGROUND=10.0f;
        float Anti__MapZ = ((Anti__FloorZ <= (INVALID_HEIGHT+5.0f)) ? Anti__GroundZ : Anti__FloorZ)+DIFF_OVERGROUND;
 
        if(!GetPlayer()->CanFly() &&
           !GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.x, movementInfo.y, movementInfo.z-7.0f) &&
           Anti__MapZ < GetPlayer()->GetPositionZ() && Anti__MapZ>(INVALID_HEIGHT+DIFF_OVERGROUND+5.0f))
        {
            static const float DIFF_AIRJUMP=25.0f; // 25 is realy high, but to many false positives...
            // Air-Jump-Detection definitively needs a better way to be detected...
            if((movementInfo.flags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2)) != 0) // Fly Hack
            {
                Anti__CheatOccurred(CurTime,"Fly hack",
                                    ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_FLY))) +
                                    ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))*2),
                                    NULL,GetPlayer()->GetPositionZ()-Anti__MapZ);
            }
            else if((Anti__MapZ+DIFF_AIRJUMP < GetPlayer()->GetPositionZ() &&
       (movementInfo.flags & (MOVEMENTFLAG_FALLING | MOVEMENTFLAG_UNK4))==0) ||
          (Anti__MapZ < GetPlayer()->GetPositionZ() &&
           opcode==MSG_MOVE_JUMP))
            {
                Anti__CheatOccurred(CurTime,"Possible Air Jump Hack",
                                    0.0f,LookupOpcodeName(opcode),0.0f,movementInfo.flags,&movementInfo);
            }
        }
 
        /*if(Anti__FloorZ < -199900.0f && Anti__GroundZ >= -199900.0f &&
GetPlayer()->GetPositionZ()+5.0f < Anti__GroundZ)
{
Anti__CheatOccurred(CurTime,"Teleport2Plane hack",
GetPlayer()->GetPositionZ(),NULL,Anti__GroundZ);
}*/
    } // <<---- anti-cheat features
 
 
    /* process position-change */
    Unit *mover = _player->m_mover;
    recv_data.put<uint32>(6, getMSTime()); // fix time, offset flags(4) + unk(2)
    WorldPacket data(recv_data.GetOpcode(), (mover->GetPackGUID().size()+recv_data.size()));
    data.append(_player->m_mover->GetPackGUID()); // use mover guid
    data.append(recv_data.contents(), recv_data.size());
    GetPlayer()->SendMessageToSet(&data, false);
 
    if(!_player->GetCharmGUID()) // nothing is charmed
    {
        _player->SetPosition(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
        _player->m_movementInfo = movementInfo;
        _player->SetUnitMovementFlags(movementInfo.flags);
    }
    else
    {
        if(mover->GetTypeId() != TYPEID_PLAYER) // unit, creature, pet, vehicle...
        {
            if(Map *map = mover->GetMap())
                map->CreatureRelocation((Creature*)mover, movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
            mover->SetUnitMovementFlags(movementInfo.flags);
        }
        else // player
        {
            ((Player*)mover)->SetPosition(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
            ((Player*)mover)->m_movementInfo = movementInfo;
            ((Player*)mover)->SetUnitMovementFlags(movementInfo.flags);

            // FG: diplomacy related stuff (group distance calculation, etc...)
            // - we dont do anything in Player::SetPosition because we want to listen to 1 opcode only - performance reasons
            if(opcode == MSG_MOVE_HEARTBEAT)
            {
                ((Player*)mover)->UpdateDiplomacyDistance();
            }
        }
    }
 
    if (GetPlayer()->m_lastFallTime >= movementInfo.fallTime || GetPlayer()->m_lastFallZ <=movementInfo.z || recv_data.GetOpcode() == MSG_MOVE_FALL_LAND)
        GetPlayer()->SetFallInformation(movementInfo.fallTime, movementInfo.z);
 
    if(GetPlayer()->isMovingOrTurning())
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
 
    if(movementInfo.z < -500.0f)
    {
        if(GetPlayer()->InBattleGround()
            && GetPlayer()->GetBattleGround()
            && GetPlayer()->GetBattleGround()->HandlePlayerUnderMap(_player))
        {
            // do nothing, the handle already did if returned true
        }
        else
        {
            // NOTE: this is actually called many times while falling
            // even after the player has been teleported away
            // TODO: discard movement packets after the player is rooted
            if(GetPlayer()->isAlive())
            {
                GetPlayer()->EnvironmentalDamage(GetPlayer()->GetGUID(),DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
                // change the death state to CORPSE to prevent the death timer from
                // starting in the next player update
                GetPlayer()->KillPlayer();
                GetPlayer()->BuildPlayerRepop();
            }
 
            // cancel the death timer here if started
            GetPlayer()->RepopAtGraveyard();
        }
    }
}
 
void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(recv_data.GetOpcode()), recv_data.GetOpcode(), recv_data.GetOpcode());
 
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8+4);
 
    /* extract packet */
    uint64 guid;
    uint32 unk1;
    float newspeed;
 
    recv_data >> guid;
 
    // now can skip not our packet
    if(_player->GetGUID() != guid)
        return;
 
    // continue parse packet
 
    recv_data >> unk1; // counter or moveEvent
 
    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);
 
    // recheck
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);
 
    recv_data >> newspeed;
    /*----------------*/
 
    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;
 
    static char const* move_type_name[MAX_MOVE_TYPE] = { "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };
 
    uint16 opcode = recv_data.GetOpcode();
    switch(opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK: move_type = MOVE_WALK; force_move_type = MOVE_WALK; break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK: move_type = MOVE_RUN; force_move_type = MOVE_RUN; break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK: move_type = MOVE_RUN_BACK; force_move_type = MOVE_RUN_BACK; break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK: move_type = MOVE_SWIM; force_move_type = MOVE_SWIM; break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK: move_type = MOVE_SWIM_BACK; force_move_type = MOVE_SWIM_BACK; break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK: move_type = MOVE_TURN_RATE; force_move_type = MOVE_TURN_RATE; break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK: move_type = MOVE_FLIGHT; force_move_type = MOVE_FLIGHT; break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK: move_type = MOVE_FLIGHT_BACK; force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK: move_type = MOVE_PITCH_RATE; force_move_type = MOVE_PITCH_RATE; break;
        default:
            sLog.outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }
 
    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if(_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if(_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }
 
    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if(_player->GetSpeed(move_type) > newspeed) // must be greater - just correct
        {
            sLog.outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeed(move_type,_player->GetSpeedRate(move_type),true);
        }
        else // must be lesser - cheating
        {
            sLog.outBasic("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(),_player->GetSession()->GetAccountId(),_player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}
 
void WorldSession::HandleSetActiveMoverOpcode(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_SET_ACTIVE_MOVER");
    recv_data.hexlike();
 
    CHECK_PACKET_SIZE(recv_data, 8);
 
    uint64 guid;
    recv_data >> guid;
 
    if(_player->m_mover->GetGUID() != guid)
    {
        sLog.outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is " I64FMT " and should be " I64FMT, _player->m_mover->GetGUID(), guid);
        return;
    }
}
 
void WorldSession::HandleMoveNotActiveMover(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");
    recv_data.hexlike();
 
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8);
 
    uint64 old_mover_guid;
    recv_data >> old_mover_guid;
 
    if(_player->m_mover->GetGUID() == old_mover_guid)
    {
        sLog.outError("HandleMoveNotActiveMover: incorrect mover guid: mover is " I64FMT " and should be " I64FMT " instead of " I64FMT, _player->m_mover->GetGUID(), _player->GetGUID(), old_mover_guid);
        return;
    }
 
    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;
}
 
void WorldSession::HandleDismissControlledVehicle(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();
 
    uint64 vehicleGUID = _player->GetCharmGUID();
 
    if(!vehicleGUID) // something wrong here...
        return;
 
    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;
 
    // using charm guid, because we don't have vehicle guid...
    if(Vehicle *vehicle = ObjectAccessor::GetVehicle(vehicleGUID))
    {
        _player->ExitVehicle(vehicle);
        vehicle->Dismiss();
    }
}
 
void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvdata*/)
{
    //sLog.outDebug("WORLD: Recvd CMSG_MOUNTSPECIAL_ANIM");
 
    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << uint64(GetPlayer()->GetGUID());
 
    GetPlayer()->SendMessageToSet(&data, false);
}
 
void WorldSession::HandleMoveKnockBackAck( WorldPacket & /*recv_data*/ )
{
    // CHECK_PACKET_SIZE(recv_data,?);
    sLog.outDebug("CMSG_MOVE_KNOCK_BACK_ACK");
    // Currently not used but maybe use later for recheck final player position
    // (must be at call same as into "recv_data >> x >> y >> z >> orientation;"
 
    /*
uint32 flags, time;
float x, y, z, orientation;
uint64 guid;
uint32 sequence;
uint32 ukn1;
float xdirection,ydirection,hspeed,vspeed;
 
recv_data >> guid;
recv_data >> sequence;
recv_data >> flags >> time;
recv_data >> x >> y >> z >> orientation;
recv_data >> ukn1; //unknown
recv_data >> vspeed >> xdirection >> ydirection >> hspeed;
 
// skip not personal message;
if(GetPlayer()->GetGUID()!=guid)
return;
 
// check code
*/
}
 
void WorldSession::HandleMoveHoverAck( WorldPacket& /*recv_data*/ )
{
    sLog.outDebug("CMSG_MOVE_HOVER_ACK");
}
 
void WorldSession::HandleMoveWaterWalkAck(WorldPacket& /*recv_data*/)
{
    sLog.outDebug("CMSG_MOVE_WATER_WALK_ACK");
}
 
void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    CHECK_PACKET_SIZE(recv_data,8+1);
 
    if(!_player->isAlive() || _player->isInCombat() )
        return;
 
    uint64 summoner_guid;
    bool agree;
    recv_data >> summoner_guid;
    recv_data >> agree;
 
    _player->SummonIfPossible(agree);
}