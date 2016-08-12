/*
* ArcEmu MMORPG Server
* Copyright (C) 2005-2007 Ascent Team <http://www.ascentemu.com/>
* Copyright (C) 2008-2012 <http://www.ArcEmu.org/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "StdAfx.h"
#define SWIMMING_TOLERANCE_LEVEL -0.08f
#define MOVEMENT_PACKET_TIME_DELAY 500

#ifdef WIN32

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define DELTA_EPOCH_IN_USEC  11644473600000000ULL
uint32 TimeStamp()
{
	//return timeGetTime();

	FILETIME ft;
	uint64 t;
	GetSystemTimeAsFileTime(&ft);

	t = (uint64)ft.dwHighDateTime << 32;
	t |= ft.dwLowDateTime;
	t /= 10;
	t -= DELTA_EPOCH_IN_USEC;

	return uint32(((t / 1000000L) * 1000) + ((t % 1000000L) / 1000));
}

uint32 mTimeStamp()
{
	return timeGetTime();
}

#else

uint32 TimeStamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

uint32 mTimeStamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

#endif

void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket & recv_data)
{
	GetPlayer()->SetPlayerStatus(NONE);
	if (_player->IsInWorld())
	{
		// get outta here
		return;
	}
	LOG_DEBUG("WORLD: got MSG_MOVE_WORLDPORT_ACK.");

	if (_player->m_CurrentTransporter && _player->GetMapId() != _player->m_CurrentTransporter->GetMapId())
	{
		/* wow, our pc must really suck. */
		Transporter* pTrans = _player->m_CurrentTransporter;

		float c_tposx = pTrans->GetPositionX() + _player->transporter_info.x;
		float c_tposy = pTrans->GetPositionY() + _player->transporter_info.y;
		float c_tposz = pTrans->GetPositionZ() + _player->transporter_info.z;


		_player->SetMapId(pTrans->GetMapId());
		_player->SetPosition(c_tposx, c_tposy, c_tposz, _player->GetOrientation());

		WorldPacket dataw(SMSG_NEW_WORLD, 20);

		dataw << pTrans->GetMapId();
		dataw << c_tposx;
		dataw << c_tposy;
		dataw << c_tposz;
		dataw << _player->GetOrientation();

		SendPacket(&dataw);
	}
	else
	{
		_player->m_TeleportState = 2;
		_player->AddToWorld();
	}
}

void WorldSession::HandleMoveTeleportAckOpcode(WorldPacket & recv_data)
{
    ObjectGuid guid;
    uint32 flags, time;
    recv_data >> time >> flags;

    guid[0] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();
    guid[5] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[0]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[3]);

	if (guid == _player->GetGUID())
	{
		if (sWorld.antihack_teleport && !(HasGMPermissions() && sWorld.no_antihack_on_gm) && _player->GetPlayerStatus() != TRANSFER_PENDING)
		{
			/* we're obviously cheating */
			sCheatLog.writefromsession(this, "Used teleport hack, disconnecting.");
			Disconnect();
			return;
		}

		if (sWorld.antihack_teleport && !(HasGMPermissions() && sWorld.no_antihack_on_gm) && _player->m_position.Distance2DSq(_player->m_sentTeleportPosition) > 625.0f)	/* 25.0f*25.0f */
		{
			/* cheating.... :( */
			sCheatLog.writefromsession(this, "Used teleport hack {2}, disconnecting.");
			Disconnect();
			return;
		}

		LOG_DEBUG("WORLD: got CMSG_MOVE_TELEPORT_ACK.");
		GetPlayer()->SetPlayerStatus(NONE);
		if (GetPlayer()->m_rooted <= 0)
			GetPlayer()->SetMovement(MOVE_UNROOT, 5);
		_player->SpeedCheatReset();

		std::list<Pet*> summons = _player->GetSummons();
		for (std::list<Pet*>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
		{
			// move pet too
			(*itr)->SetPosition((GetPlayer()->GetPositionX() + 2), (GetPlayer()->GetPositionY() + 2), GetPlayer()->GetPositionZ(), M_PI_FLOAT);
		}
		if (_player->m_sentTeleportPosition.x != 999999.0f)
		{
			//_player->m_position = _player->m_sentTeleportPosition;
            _player->m_movementInfo.pos.m_positionX = _player->m_sentTeleportPosition.x;
            _player->m_movementInfo.pos.m_positionY = _player->m_sentTeleportPosition.y;
            _player->m_movementInfo.pos.m_positionZ = _player->m_sentTeleportPosition.z;
            _player->m_movementInfo.pos.SetOrientation(_player->m_sentTeleportPosition.o);
			_player->m_sentTeleportPosition.ChangeCoords(999999.0f, 999999.0f, 999999.0f);
		}
	}
}

void _HandleBreathing(MovementInfo & movement_info, Player* _player, WorldSession* pSession)
{
	// no water breathing is required
	if (!sWorld.BreathingEnabled || _player->FlyCheat || _player->m_bUnlimitedBreath || !_player->isAlive() || _player->GodModeCheat)
	{
		// player is flagged as in water
		if (_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING)
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_SWIMMING;

		// player is flagged as under water
		if (_player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER)
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(TIMER_BREATH) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(-1) << uint32(0);

			pSession->SendPacket(&data);
		}

		// player is above water level
		if (pSession->m_bIsWLevelSet)
		{
			if ((movement_info.z + _player->m_noseLevel) > pSession->m_wLevel)
			{
				_player->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_LEAVE_WATER);

				// unset swim session water level
				pSession->m_bIsWLevelSet = false;
			}
		}

		return;
	}

	//player is swimming and not flagged as in the water
	if (movement_info.flags & MOVEMENTFLAG_SWIMMING && !(_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING))
	{
		_player->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_ENTER_WATER);

		// get water level only if it was not set before
		if (!pSession->m_bIsWLevelSet)
		{
			// water level is somewhere below the nose of the character when entering water
			pSession->m_wLevel = movement_info.z + _player->m_noseLevel * 0.95f;
			pSession->m_bIsWLevelSet = true;
		}

		_player->m_UnderwaterState |= UNDERWATERSTATE_SWIMMING;
	}

	// player is not swimming and is not stationary and is flagged as in the water
	if (!(movement_info.flags & MOVEMENTFLAG_SWIMMING) && (movement_info.flags != MOVEMENTFLAG_NONE) && (_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING))
	{
		// player is above water level
		if ((movement_info.z + _player->m_noseLevel) > pSession->m_wLevel)
		{
			_player->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_LEAVE_WATER);

			// unset swim session water level
			pSession->m_bIsWLevelSet = false;

			_player->m_UnderwaterState &= ~UNDERWATERSTATE_SWIMMING;
		}
	}

	// player is flagged as in the water and is not flagged as under the water
	if (_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING && !(_player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER))
	{
		//the player is in the water and has gone under water, requires breath bar.
		if ((movement_info.z + _player->m_noseLevel) < pSession->m_wLevel)
		{
			_player->m_UnderwaterState |= UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(TIMER_BREATH) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(-1) << uint32(0);

			pSession->SendPacket(&data);
		}
	}

	// player is flagged as in the water and is flagged as under the water
	if (_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING && _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER)
	{
		//the player is in the water but their face is above water, no breath bar needed.
		if ((movement_info.z + _player->m_noseLevel) > pSession->m_wLevel)
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(TIMER_BREATH) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(10) << uint32(0);
			pSession->SendPacket(&data);
		}
	}

	// player is flagged as not in the water and is flagged as under the water
	if (!(_player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING) && _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER)
	{
		//the player is out of the water, no breath bar needed.
		if ((movement_info.z + _player->m_noseLevel) > pSession->m_wLevel)
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(TIMER_BREATH) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(10) << uint32(0);
			pSession->SendPacket(&data);
		}
	}

}

struct MovementFlagName
{
	uint32 flag;
	const char* name;
};

static MovementFlagName MoveFlagsToNames[] =
{
	{ MOVEMENTFLAG_NONE, "MOVEMENTFLAG_NONE" },
	{ MOVEMENTFLAG_FORWARD, "MOVEMENTFLAG_FORWARD" },
	{ MOVEMENTFLAG_BACKWARD, "MOVEMENTFLAG_BACKWARD" },
	{ MOVEMENTFLAG_STRAFE_LEFT, "MOVEMENTFLAG_STRAFE_LEFT" },
	{ MOVEMENTFLAG_STRAFE_RIGHT, "MOVEMENTFLAG_STRAFE_RIGHT" },
	{ MOVEMENTFLAG_LEFT, "MOVEMENTFLAG_LEFT" },
	{ MOVEMENTFLAG_RIGHT, "MOVEMENTFLAG_RIGHT" },
	{ MOVEMENTFLAG_PITCH_DOWN, "MOVEMENTFLAG_PITCH_DOWN" },
	{ MOVEMENTFLAG_PITCH_UP, "MOVEMENTFLAG_PITCH_UP" },
	{ MOVEMENTFLAG_WALKING, "MOVEMENTFLAG_WALKING" },
	{ MOVEMENTFLAG_DISABLE_GRAVITY, "MOVEMENTFLAG_DISABLE_GRAVITY" },
	{ MOVEMENTFLAG_DISABLE_COLLISION, "MOVEMENTFLAG_DISABLE_COLLISION" },
	{ MOVEMENTFLAG_ROOT, "MOVEMENTFLAG_ROOT" },
	//{ MOVEFLAG_REDIRECTED, "MOVEFLAG_REDIRECTED" },
	{ MOVEMENTFLAG_FALLING, "MOVEMENTFLAG_FALLING" },
	//{ MOVEFLAG_FALLING_FAR, "MOVEFLAG_FALLING_FAR" },
	//{ MOVEFLAG_FREE_FALLING, "MOVEFLAG_FREE_FALLING" },
	//{ MOVEFLAG_TB_PENDING_STOP, "MOVEFLAG_TB_PENDING_STOP" },
	//{ MOVEFLAG_TB_PENDING_UNSTRAFE, "MOVEFLAG_TB_PENDING_UNSTRAFE" },
	//{ MOVEFLAG_TB_PENDING_FALL, "MOVEFLAG_TB_PENDING_FALL" }, // where is this in 15595?
	//{ MOVEFLAG_TB_PENDING_FORWARD, "MOVEFLAG_TB_PENDING_FORWARD" },
	//{ MOVEFLAG_TB_PENDING_BACKWARD, "MOVEFLAG_TB_PENDING_BACKWARD" },
	{ MOVEMENTFLAG_SWIMMING, "MOVEMENTFLAG_SWIMMING" },
	//{ MOVEFLAG_FLYING_PITCH_UP, "MOVEFLAG_FLYING_PITCH_UP" },
	//{ MOVEFLAG_CAN_FLY, "MOVEFLAG_CAN_FLY" },
	//{ MOVEFLAG_AIR_SUSPENSION, "MOVEFLAG_AIR_SUSPENSION" },
	{ MOVEMENTFLAG_FLYING, "MOVEMENTFLAG_FLYING" },
	//{ MOVEMENTFLAG_SPLINE_ELEVATION, "MOVEMENTFLAG_SPLINE_ELEVATION" },
	//{ MOVEFLAG_SPLINE_ENABLED, "MOVEFLAG_SPLINE_ENABLED" }, // gone in 15595
	//{ MOVEFLAG_WATER_WALK, "MOVEFLAG_WATER_WALK" },
	//{ MOVEFLAG_FEATHER_FALL, "MOVEFLAG_FEATHER_FALL" },
	{ MOVEMENTFLAG_HOVER, "MOVEMENTFLAG_HOVER" },
	//{ MOVEFLAG_LOCAL, "MOVEFLAG_LOCAL" },
};

static const uint32 nmovementflags = sizeof(MoveFlagsToNames) / sizeof(MovementFlagName);

void WorldSession::HandleMovementOpcodes(WorldPacket & recv_data)
{
	uint16 opcode = recv_data.GetOpcode();

	Unit* mover = _player->GetMapMgr()->GetUnit(m_MoverWoWGuid.GetOldGuid());// _player->m_mover;

	ASSERT(mover != NULL);                      // there must always be a mover
	
	Player* plrMover = TO_PLAYER(mover); 

	// ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
	//if (plrMover && plrMover->IsBeingTeleported())
	//{
	//	recv_data.rfinish();                     // prevent warnings spam
	//	return;
	//}

	/* extract packet */
	MovementInfo3 movementInfo;
	GetPlayer()->ReadMovementInfo(recv_data, &movementInfo);

	// prevent tampered movement data
	if (movementInfo.guid != mover->GetGUID())
	{
		sLog.outError("network", "HandleMovementOpcodes: guid error");
		return;
	}
	//if (!movementInfo.pos.IsPositionValid())
	//{
	//	TC_LOG_ERROR("network", "HandleMovementOpcodes: Invalid Position");
	//	return;
	//}

	/* handle special cases 
	if (movementInfo.t_guid)
	{
		// transports size limited
		// (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
		if (movementInfo.transport.pos.GetPositionX() > 50 || movementInfo.transport.pos.GetPositionY() > 50 || movementInfo.transport.pos.GetPositionZ() > 50)
		{
			recv_data.rfinish();                 // prevent warnings spam
			return;
		}

		if (!Trinity::IsValidMapCoord(movementInfo.pos.GetPositionX() + movementInfo.transport.pos.GetPositionX(), movementInfo.pos.GetPositionY() + movementInfo.transport.pos.GetPositionY(),
			movementInfo.pos.GetPositionZ() + movementInfo.transport.pos.GetPositionZ(), movementInfo.pos.GetOrientation() + movementInfo.transport.pos.GetOrientation()))
		{
			recv_data.rfinish();                 // prevent warnings spam
			return;
		}

		// if we boarded a transport, add us to it
		if (plrMover)
		{
			if (!plrMover->GetTransport())
			{
				if (Transport* transport = plrMover->GetMap()->GetTransport(movementInfo.transport.guid))
				{
					plrMover->m_transport = transport;
					transport->AddPassenger(plrMover);
				}
			}
			else if (plrMover->GetTransport()->GetGUID() != movementInfo.transport.guid)
			{
				bool foundNewTransport = false;
				plrMover->m_transport->RemovePassenger(plrMover);
				if (Transport* transport = plrMover->GetMap()->GetTransport(movementInfo.transport.guid))
				{
					foundNewTransport = true;
					plrMover->m_transport = transport;
					transport->AddPassenger(plrMover);
				}

				if (!foundNewTransport)
				{
					plrMover->m_transport = NULL;
					movementInfo.ResetTransport();
				}
			}
		}

		if (!mover->GetTransport() && !mover->GetVehicle())
		{
			GameObject* go = mover->GetMap()->GetGameObject(movementInfo.transport.guid);
			if (!go || go->GetGoType() != GAMEOBJECT_TYPE_TRANSPORT)
				movementInfo.transport.guid = 0;
		}
	}
	else if (plrMover && plrMover->GetTransport())                // if we were on a transport, leave
	{
		plrMover->m_transport->RemovePassenger(plrMover);
		plrMover->m_transport = NULL;
		movementInfo.ResetTransport();
	}*/

	if (_player->blinked)
	{
		_player->blinked = false;
		_player->m_fallDisabledUntil = UNIXTIME + 5;
		_player->SpeedCheatDelay(2000);   //some say they managed to trigger system with knockback. Maybe they moved in air ?
	}
	else
	{
		if (opcode == MSG_MOVE_FALL_LAND)
		{
			// player has finished falling
			//if z_axisposition contains no data then set to current position
			if (!mover->z_axisposition)
				mover->z_axisposition = movement_info.z;

			// calculate distance fallen
			uint32 falldistance = float2int32(mover->z_axisposition - movement_info.z);
			if (mover->z_axisposition <= movement_info.z)
				falldistance = 1;
			/*Safe Fall*/
			if ((int)falldistance > mover->m_safeFall)
				falldistance -= mover->m_safeFall;
			else
				falldistance = 1;

			//checks that player has fallen more than 12 units, otherwise no damage will be dealt
			//falltime check is also needed here, otherwise sudden changes in Z axis position, such as using !recall, may result in death
			if (mover->isAlive() && !mover->bInvincible && (falldistance > 12) && !mover->m_noFallDamage &&
				((mover->GetGUID() != _player->GetGUID()) || (!_player->GodModeCheat && (UNIXTIME >= _player->m_fallDisabledUntil))))
			{
				// 1.7% damage for each unit fallen on Z axis over 13
				uint32 health_loss = static_cast< uint32 >(mover->GetHealth() * (falldistance - 12) * 0.017f);

				if (health_loss >= mover->GetHealth())
					health_loss = mover->GetHealth();
#ifdef ENABLE_ACHIEVEMENTS
				else if ((falldistance >= 65) && (mover->GetGUID() == _player->GetGUID()))
				{
					// Rather than Updating achievement progress every time fall damage is taken, all criteria currently have 65 yard requirement...
					// Achievement 964: Fall 65 yards without dying.
					// Achievement 1260: Fall 65 yards without dying while completely smashed during the Brewfest Holiday.
					_player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, falldistance, Player::GetDrunkenstateByValue(_player->GetDrunkValue()), 0);
				}
#endif

				mover->SendEnvironmentalDamageLog(mover->GetGUID(), DAMAGE_FALL, health_loss);
				mover->DealDamage(mover, health_loss, 0, 0, 0);

				//_player->RemoveStealth(); // cebernic : why again? lost stealth by AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN already.
			}
			mover->z_axisposition = 0.0f;
		}
		else
			//whilst player is not falling, continuously update Z axis position.
			//once player lands this will be used to determine how far he fell.
			if (!(movement_info.flags & MOVEMENTFLAG_FALLING))
				mover->z_axisposition = movement_info.z;
	}

	//if (plrMover && ((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != plrMover->IsInWater())
	//{
		// now client not include swimming flag in case jumping under water
	//	plrMover->SetInWater(!plrMover->IsInWater() || plrMover->GetBaseMap()->IsUnderWater(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ()));
	//}

	uint32 mstime = getMSTime();
	/*----------------------*/
	if (m_clientTimeDelay == 0)
		m_clientTimeDelay = mstime - movementInfo.time;

	/* process position-change */
	movementInfo.time = movementInfo.time + m_clientTimeDelay + MOVEMENT_PACKET_TIME_DELAY;

	movementInfo.guid = mover->GetGUID(); // didn't we read it from the packet anyways?
	mover->m_movementInfo = movementInfo; // we have to get rid of one of these sometime

	/*----------------------*/
	/* process position-change */
	// this is almost never true (not sure why it is sometimes, but it is), normally use mover->IsVehicle()
	//if (mover->GetVehicle())
	//{
	//	mover->SetOrientation(movementInfo.pos.GetOrientation());
	//	return;
	//}

	if (opcode == MSG_MOVE_SET_FACING)
	{
		mover->SetOrientation(movementInfo.pos.GetOrientation());
	}

	mover->SetPosition2(movementInfo.pos);// .x, movementInfo.y, movementInfo.z, movementInfo.o);	

	WorldPacket data(SMSG_PLAYER_MOVE, recv_data.size());
	mover->WriteMovementInfo(data);
	mover->SendMessageToSet(&data, false);

	/*if (plrMover)                                            // nothing is charmed, or player charmed
	{
		plrMover->UpdateFallInformationIfNeed(movementInfo, opcode);

		AreaTableEntry const* zone = GetAreaEntryByAreaID(plrMover->GetAreaId());
		float depth = zone ? zone->MaxDepth : -500.0f;
		if (movementInfo.z < depth)
		{
			if (!(plrMover->GetBattleground() && plrMover->GetBattleground()->HandlePlayerUnderMap(_player)))
			{
				// NOTE: this is actually called many times while falling
				// even after the player has been teleported away
				/// @todo discard movement packets after the player is rooted
				if (plrMover->isAlive())
				{
					plrMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
					// player can be alive if GM/etc
					// change the death state to CORPSE to prevent the death timer from
					// starting in the next player update
					if (!plrMover->isAlive())
						plrMover->KillPlayer();
				}
			}
		}
	}*/
}

void WorldSession::HandleMoveTimeSkippedOpcode(WorldPacket & recv_data)
{
    ObjectGuid guid;
    uint32 time;
    recv_data >> time;

    guid[5] = recv_data.ReadBit();
    guid[0] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[0]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[4]);
}

void WorldSession::HandleMoveNotActiveMoverOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

		WoWGuid guid;
	recv_data >> guid;

	if (guid == m_MoverWoWGuid)
		return;

	movement_info.init(recv_data);

	if ((guid != uint64(0)) && (guid == _player->GetCharmedUnitGUID()))
		m_MoverWoWGuid = guid;
	else
		m_MoverWoWGuid.Init(_player->GetGUID());

	// set up to the movement packet
	movement_packet[0] = m_MoverWoWGuid.GetNewGuidMask();
	memcpy(&movement_packet[1], m_MoverWoWGuid.GetNewGuid(), m_MoverWoWGuid.GetNewGuidLen());
}


void WorldSession::HandleSetActiveMoverOpcode(WorldPacket & recv_data)
{	
	ObjectGuid guid;

	recv_data.ReadBit();

	guid[3] = recv_data.ReadBit();
	guid[0] = recv_data.ReadBit();
	guid[2] = recv_data.ReadBit();
	guid[1] = recv_data.ReadBit();
	guid[5] = recv_data.ReadBit();
	guid[4] = recv_data.ReadBit();
	guid[7] = recv_data.ReadBit();
	guid[6] = recv_data.ReadBit();

	recv_data.ReadByteSeq(guid[3]);
	recv_data.ReadByteSeq(guid[4]);
	recv_data.ReadByteSeq(guid[5]);
	recv_data.ReadByteSeq(guid[2]);
	recv_data.ReadByteSeq(guid[7]);
	recv_data.ReadByteSeq(guid[0]);
	recv_data.ReadByteSeq(guid[1]);
	recv_data.ReadByteSeq(guid[6]);

	sLog.outError("Our HandleSetActiveMoverOpcode guid : %u", guid);
	if (guid != m_MoverWoWGuid.GetOldGuid())
	{
		sLog.outError("");
		// make sure the guid is valid and we aren't cheating
		//if (!(_player->m_CurrentCharm == guid) &&
		//	!(_player->GetGUID() == guid))
		//{
		//	if (_player->GetCurrentVehicle()->GetOwner()->GetGUID() != guid)
		//		return;
		//}

		// generate wowguid
		if (guid != 0)
			m_MoverWoWGuid.Init(guid);
		else
			m_MoverWoWGuid.Init(_player->GetGUID());

		// set up to the movement packet
		movement_packet[0] = m_MoverWoWGuid.GetNewGuidMask();
		memcpy(&movement_packet[1], m_MoverWoWGuid.GetNewGuid(), m_MoverWoWGuid.GetNewGuidLen());
	}
}

void WorldSession::HandleMoveSplineCompleteOpcode(WorldPacket & recvPacket)
{

}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket & recvdata)
{
	CHECK_INWORLD_RETURN

		WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
	data << _player->GetGUID();
	_player->SendMessageToSet(&data, true);
}

void WorldSession::HandleWorldportOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

		uint32 unk;
	uint32 mapid;
	float x, y, z, o;
	recv_data >> unk >> mapid >> x >> y >> z >> o;

	if (!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	LocationVector vec(x, y, z, o);
	_player->SafeTeleport(mapid, 0, vec);
}

void WorldSession::HandleTeleportToUnitOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint8 unk;
	Unit* target;
	recv_data >> unk;

	if (!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	if ((target = _player->GetMapMgr()->GetUnit(_player->GetSelection())) == NULL)
		return;

	_player->SafeTeleport(_player->GetMapId(), _player->GetInstanceID(), target->GetPosition());
}

void WorldSession::HandleTeleportCheatOpcode(WorldPacket & recv_data)
{

	CHECK_INWORLD_RETURN

		float x, y, z, o;
	LocationVector vec;

	if (!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	recv_data >> x >> y >> z >> o;
	vec.ChangeCoords(x, y, z, o);
	_player->SafeTeleport(_player->GetMapId(), _player->GetInstanceID(), vec);
}

MovementInfo::MovementInfo(){
	time = 0;
	pitch = 0.0f;
	redirectSin = 0.0f;
	redirectCos = 0.0f;
	redirect2DSpeed = 0.0f;
	unk11 = 0;
	unk12 = 0;
	unk13 = 0;
	unklast = 0;
	unk_230 = 0;
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	orientation = 0.0f;
	flags = 0;
	redirectVelocity = 0.0f;
	transGuid = 0;
	transX = 0.0f;
	transY = 0.0f;
	transZ = 0.0f;
	transO = 0.0f;
	transUnk = 0.0f;
	transUnk_2 = 0;
}

void MovementInfo::init(WorldPacket & data)
{
	transGuid = 0;
	unk13 = 0;
	data >> flags >> unk_230 >> time;
	data >> x >> y >> z >> orientation;

	if (flags & MOVEMENTFLAG_DISABLE_GRAVITY)
	{
		data >> transGuid >> transX >> transY >> transZ >> transO >> transUnk >> transUnk_2;
	}
	if (flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING) || unk_230 & 0x20)
	{
		data >> pitch;
	}
	/*if(flags & MOVEFLAG_REDIRECTED)
	{
	data >> redirectVelocity >> redirectSin >> redirectCos >> redirect2DSpeed;
	}*/
	if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
	{
		data >> unk12;
	}

	data >> unklast;
	if (data.rpos() != data.wpos())
	{
		if (data.rpos() + 4 == data.wpos())
			data >> unk13;
		else
			LOG_DEBUG("Extra bits of movement packet left");
	}
}

void MovementInfo::write(WorldPacket & data)
{
	data << flags << unk_230 << getMSTime();

	data << x << y << z << orientation;

	if (flags & MOVEMENTFLAG_DISABLE_GRAVITY)
	{
		data << transGuid << transX << transY << transZ << transO << transUnk << transUnk_2;
	}
	if (flags & MOVEMENTFLAG_SWIMMING)
	{
		data << pitch;
	}
	if (flags & MOVEMENTFLAG_FALLING)
	{
		data << redirectVelocity << redirectSin << redirectCos << redirect2DSpeed;
	}
	if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
	{
		data << unk12;
	}
	data << unklast;
	if (unk13)
		data << unk13;
}
