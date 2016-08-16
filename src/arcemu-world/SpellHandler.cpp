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

void WorldSession::HandleUseItemOpcode(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	Player* pUser = _player;
	Unit* mover = _player->m_mover;
	
	uint8 bagIndex, slot;
	uint8 castCount = 0;
	uint8 castFlags = 0;
	uint32 spellId = 0;
	uint32 glyphIndex = 0;
	uint32 targetMask = 0;
	uint32 targetStringLength = 0;
	float elevation = 0.0f;
	float missileSpeed = 0.0f;
	ObjectGuid itemGuid = 0;
	ObjectGuid targetGuid = 0;
	ObjectGuid itemTargetGuid = 0;
	ObjectGuid destTransportGuid = 0;
	ObjectGuid srcTransportGuid = 0;
	Position srcPos;
	Position destPos;
	std::string targetString;

	// Movement data
	MovementInfo3 movementInfo;
	ObjectGuid movementTransportGuid = 0;
	ObjectGuid movementGuid = 0;
	bool hasTransport = false;
	bool hasTransportTime2 = false;
	bool hasTransportTime3 = false;
	bool hasFallData = false;
	bool hasFallDirection = false;
	bool hasTimestamp = false;
	bool hasSplineElevation = false;
	bool hasPitch = false;
	bool hasOrientation = false;
	bool hasUnkMovementField = false;
	bool found = false;
	uint32 unkMovementLoopCounter = 0;
	Unit* caster = mover;

	recvPacket >> slot >> bagIndex;

	bool hasElevation = !recvPacket.ReadBit();
	itemGuid[6] = recvPacket.ReadBit();
	bool hasTargetString = !recvPacket.ReadBit();
	itemGuid[1] = recvPacket.ReadBit();
	bool hasCastFlags = !recvPacket.ReadBit();
	bool hasDestLocation = recvPacket.ReadBit();
	itemGuid[2] = recvPacket.ReadBit();
	itemGuid[7] = recvPacket.ReadBit();
	itemGuid[0] = recvPacket.ReadBit();
	bool hasTargetMask = !recvPacket.ReadBit();
	bool hasMissileSpeed = !recvPacket.ReadBit();
	bool hasMovement = recvPacket.ReadBit();
	bool hasCastCount = !recvPacket.ReadBit();
	bool hasSpellId = !recvPacket.ReadBit();
	recvPacket.ReadBit();
	bool hasGlyphIndex = !recvPacket.ReadBit();
	recvPacket.ReadBit();
	itemGuid[4] = recvPacket.ReadBit();
	bool hasSrcLocation = recvPacket.ReadBit();
	itemGuid[3] = recvPacket.ReadBit();
	itemGuid[5] = recvPacket.ReadBit();
	uint8 researchDataCount = recvPacket.ReadBits(2);

	for (uint8 i = 0; i < researchDataCount; ++i)
		recvPacket.ReadBits(2);

	if (hasMovement)
	{
		hasPitch = !recvPacket.ReadBit();
		hasTransport = recvPacket.ReadBit();
		recvPacket.ReadBit();

		if (hasTransport)
		{
			movementTransportGuid[7] = recvPacket.ReadBit();
			movementTransportGuid[2] = recvPacket.ReadBit();
			movementTransportGuid[4] = recvPacket.ReadBit();
			movementTransportGuid[5] = recvPacket.ReadBit();
			movementTransportGuid[6] = recvPacket.ReadBit();
			movementTransportGuid[0] = recvPacket.ReadBit();
			movementTransportGuid[1] = recvPacket.ReadBit();
			hasTransportTime3 = recvPacket.ReadBit();
			movementTransportGuid[4] = recvPacket.ReadBit();
			hasTransportTime2 = recvPacket.ReadBit();
		}

		movementGuid[6] = recvPacket.ReadBit();
		movementGuid[2] = recvPacket.ReadBit();
		movementGuid[1] = recvPacket.ReadBit();
		unkMovementLoopCounter = recvPacket.ReadBits(22);
		recvPacket.ReadBit();
		bool hasMovementFlags2 = !recvPacket.ReadBit();
		hasFallData = recvPacket.ReadBit();
		movementGuid[5] = recvPacket.ReadBit();
		hasSplineElevation = !recvPacket.ReadBit();
		recvPacket.ReadBit();
		movementGuid[7] = recvPacket.ReadBit();
		movementGuid[0] = recvPacket.ReadBit();

		if (hasFallData)
			hasFallDirection = recvPacket.ReadBit();

		hasOrientation = !recvPacket.ReadBit();
		movementGuid[4] = recvPacket.ReadBit();
		movementGuid[3] = recvPacket.ReadBit();
		hasTimestamp = !recvPacket.ReadBit();
		hasUnkMovementField = !recvPacket.ReadBit();
		bool hasMovementFlags = !recvPacket.ReadBit();

		if (hasMovementFlags2)
			movementInfo.flags2 = recvPacket.ReadBits(13);

		if (hasMovement)
			movementInfo.flags = recvPacket.ReadBits(30);
	}

	if (hasSrcLocation)
	{
		srcTransportGuid[3] = recvPacket.ReadBit();
		srcTransportGuid[1] = recvPacket.ReadBit();
		srcTransportGuid[7] = recvPacket.ReadBit();
		srcTransportGuid[4] = recvPacket.ReadBit();
		srcTransportGuid[2] = recvPacket.ReadBit();
		srcTransportGuid[0] = recvPacket.ReadBit();
		srcTransportGuid[6] = recvPacket.ReadBit();
		srcTransportGuid[5] = recvPacket.ReadBit();
	}

	if (hasDestLocation)
	{
		destTransportGuid[2] = recvPacket.ReadBit();
		destTransportGuid[4] = recvPacket.ReadBit();
		destTransportGuid[1] = recvPacket.ReadBit();
		destTransportGuid[7] = recvPacket.ReadBit();
		destTransportGuid[6] = recvPacket.ReadBit();
		destTransportGuid[0] = recvPacket.ReadBit();
		destTransportGuid[3] = recvPacket.ReadBit();
		destTransportGuid[5] = recvPacket.ReadBit();
	}

	if (hasTargetString)
		targetStringLength = recvPacket.ReadBits(7);

	itemTargetGuid[1] = recvPacket.ReadBit();
	itemTargetGuid[0] = recvPacket.ReadBit();
	itemTargetGuid[5] = recvPacket.ReadBit();
	itemTargetGuid[3] = recvPacket.ReadBit();
	itemTargetGuid[6] = recvPacket.ReadBit();
	itemTargetGuid[4] = recvPacket.ReadBit();
	itemTargetGuid[7] = recvPacket.ReadBit();
	itemTargetGuid[2] = recvPacket.ReadBit();

	targetGuid[4] = recvPacket.ReadBit();
	targetGuid[5] = recvPacket.ReadBit();
	targetGuid[0] = recvPacket.ReadBit();
	targetGuid[1] = recvPacket.ReadBit();
	targetGuid[3] = recvPacket.ReadBit();
	targetGuid[7] = recvPacket.ReadBit();
	targetGuid[6] = recvPacket.ReadBit();
	targetGuid[2] = recvPacket.ReadBit();

	if (hasCastFlags)
		castFlags = recvPacket.ReadBits(5);

	if (hasTargetMask)
		targetMask = recvPacket.ReadBits(20);

	recvPacket.ReadByteSeq(itemGuid[0]);
	recvPacket.ReadByteSeq(itemGuid[5]);
	recvPacket.ReadByteSeq(itemGuid[6]);
	recvPacket.ReadByteSeq(itemGuid[3]);
	recvPacket.ReadByteSeq(itemGuid[4]);
	recvPacket.ReadByteSeq(itemGuid[2]);
	recvPacket.ReadByteSeq(itemGuid[1]);

	for (uint8 i = 0; i < researchDataCount; ++i)
	{
		recvPacket.read<uint32>();
		recvPacket.read<uint32>();
	}

	recvPacket.ReadByteSeq(itemGuid[7]);

	if (hasMovement)
	{
		for (uint8 i = 0; i != unkMovementLoopCounter; i++)
			recvPacket.read<uint32>();

		if (hasTransport)
		{
			recvPacket >> movementInfo.transport.pos.m_positionY;
			recvPacket >> movementInfo.transport.pos.m_positionZ;
			recvPacket.ReadByteSeq(movementTransportGuid[1]);

			if (hasTransportTime3)
				recvPacket >> movementInfo.transport.time3;

			recvPacket.ReadByteSeq(movementTransportGuid[7]);
			recvPacket.ReadByteSeq(movementTransportGuid[5]);
			recvPacket.ReadByteSeq(movementTransportGuid[2]);
			recvPacket.ReadByteSeq(movementTransportGuid[4]);
			recvPacket >> movementInfo.transport.pos.m_positionX;
			movementInfo.transport.pos.SetOrientation(recvPacket.read<float>());
			recvPacket.ReadByteSeq(movementTransportGuid[0]);
			recvPacket >> movementInfo.transport.seat;
			recvPacket >> movementInfo.transport.time;
			recvPacket.ReadByteSeq(movementTransportGuid[6]);
			recvPacket.ReadByteSeq(movementTransportGuid[3]);

			if (hasTransportTime2)
				recvPacket >> movementInfo.transport.time2;
		}

		if (hasFallData)
		{
			recvPacket >> movementInfo.jump.zspeed;

			if (hasFallDirection)
			{
				recvPacket >> movementInfo.jump.sinAngle;
				recvPacket >> movementInfo.jump.cosAngle;
				recvPacket >> movementInfo.jump.xyspeed;
			}

			recvPacket >> movementInfo.jump.fallTime;
		}
		recvPacket.ReadByteSeq(movementTransportGuid[3]);
		recvPacket.ReadByteSeq(movementTransportGuid[7]);
		recvPacket.ReadByteSeq(movementTransportGuid[6]);
		recvPacket.ReadByteSeq(movementTransportGuid[1]);
		recvPacket >> movementInfo.pos.m_positionY;

		if (hasSplineElevation)
			recvPacket >> movementInfo.splineElevation;

		if (hasUnkMovementField)
			recvPacket.read<uint32>();

		if (hasOrientation)
			movementInfo.pos.SetOrientation(recvPacket.read<float>());

		recvPacket.ReadByteSeq(movementTransportGuid[2]);
		recvPacket >> movementInfo.pos.m_positionZ;

		if (hasTimestamp)
			recvPacket >> movementInfo.time;

		recvPacket >> movementInfo.pos.m_positionX;
		recvPacket.ReadByteSeq(movementTransportGuid[5]);
		recvPacket.ReadByteSeq(movementTransportGuid[0]);

		if (hasPitch)
			movementInfo.pitch = G3D::wrap(recvPacket.read<float>(), float(-M_PI), float(M_PI));

		recvPacket.ReadByteSeq(movementTransportGuid[4]);
	}

	if (hasDestLocation)
	{
		float x, y, z;

		recvPacket.ReadByteSeq(destTransportGuid[7]);
		recvPacket >> x;
		recvPacket.ReadByteSeq(destTransportGuid[0]);
		recvPacket.ReadByteSeq(destTransportGuid[6]);
		recvPacket.ReadByteSeq(destTransportGuid[1]);
		recvPacket.ReadByteSeq(destTransportGuid[3]);
		recvPacket >> y;
		recvPacket.ReadByteSeq(destTransportGuid[5]);
		recvPacket >> z;
		recvPacket.ReadByteSeq(destTransportGuid[4]);
		recvPacket.ReadByteSeq(destTransportGuid[2]);

		destPos.Relocate(x, y, z);
	}
	//else
	//{
		//destTransportGuid = caster->GetTransGUID();

		//if (destTransportGuid)
		//	destPos.Relocate(caster-> GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
		//else
			//destPos.Relocate(caster);
	//}

	recvPacket.ReadByteSeq(targetGuid[6]);
	recvPacket.ReadByteSeq(targetGuid[7]);
	recvPacket.ReadByteSeq(targetGuid[2]);
	recvPacket.ReadByteSeq(targetGuid[0]);
	recvPacket.ReadByteSeq(targetGuid[3]);
	recvPacket.ReadByteSeq(targetGuid[4]);
	recvPacket.ReadByteSeq(targetGuid[1]);
	recvPacket.ReadByteSeq(targetGuid[5]);
	if (hasSrcLocation)
	{
		float x, y, z;

		recvPacket.ReadByteSeq(srcTransportGuid[7]);
		recvPacket >> x;
		recvPacket.ReadByteSeq(srcTransportGuid[1]);
		recvPacket.ReadByteSeq(srcTransportGuid[5]);
		recvPacket.ReadByteSeq(srcTransportGuid[4]);
		recvPacket >> z;
		recvPacket.ReadByteSeq(srcTransportGuid[6]);
		recvPacket.ReadByteSeq(srcTransportGuid[0]);
		recvPacket.ReadByteSeq(srcTransportGuid[3]);
		recvPacket >> y;
		recvPacket.ReadByteSeq(srcTransportGuid[2]);

		srcPos.Relocate(x, y, z);
	}
	//else
	//{
	//	srcTransportGuid = caster->GetTransGUID();
	//	if (srcTransportGuid)
	//		srcPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
	//	else
	//		srcPos.Relocate(caster);
	//}

	if (hasSpellId)
	{
		recvPacket >> spellId;
	}

	recvPacket.ReadByteSeq(itemTargetGuid[1]);
	recvPacket.ReadByteSeq(itemTargetGuid[4]);
	recvPacket.ReadByteSeq(itemTargetGuid[3]);
	recvPacket.ReadByteSeq(itemTargetGuid[6]);
	recvPacket.ReadByteSeq(itemTargetGuid[2]);
	recvPacket.ReadByteSeq(itemTargetGuid[0]);
	recvPacket.ReadByteSeq(itemTargetGuid[7]);
	recvPacket.ReadByteSeq(itemTargetGuid[5]);

	if (hasTargetString)
		targetString = recvPacket.ReadString(targetStringLength);

	if (hasElevation)
		recvPacket >> elevation;

	if (hasGlyphIndex)
		recvPacket >> glyphIndex;

	if (hasMissileSpeed)
		recvPacket >> missileSpeed;

	if (hasCastCount)
		recvPacket >> castCount;

	if (glyphIndex >= 6)
	{
		//pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
		return;
	}

	Item* tmpItem = NULL;
	tmpItem = pUser->GetItemInterface()->GetInventoryItem(bagIndex, slot);
	if (!tmpItem)
	{
		tmpItem = pUser->GetItemInterface()->GetInventoryItem(slot);
	}

	if (!tmpItem)
	{
		return;
	}
	ItemPrototype* itemProto = tmpItem->GetProto();

	// only some consumable items can be used in arenas
	if( ( itemProto->Class == ITEM_CLASS_CONSUMABLE ) &&
		!itemProto->HasFlag( ITEM_FLAG_USEABLE_IN_ARENA ) &&
		( GetPlayer()->m_bg != NULL ) &&
		IS_ARENA( GetPlayer()->m_bg->GetType() ) )
	{
		GetPlayer()->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_NOT_DURING_ARENA_MATCH);
		return;
	}
	if(tmpItem->IsSoulbound())     // SouldBind item will be used after SouldBind()
	{
		if (sScriptMgr.CallScriptedItem(tmpItem, _player))
		{
			return;
		}
	}

	if (_player->getDeathState() == CORPSE)
	{
		return;
	}

	//if(itemProto->Bonding == ITEM_BIND_ON_USE)
		//tmpItem->SoulBind();

	if (sScriptMgr.CallScriptedItem(tmpItem, _player))
	{
		return;
	}
	if (itemProto->InventoryType != 0 && !_player->GetItemInterface()->IsEquipped(itemProto->ItemId))  //Equipable items cannot be used before they're equipped. Prevents exploits
	{
		return;//Prevents exploits such as keeping an on-use trinket in your bag and using WPE to use it from your bag in mid-combat.
	}
	if(itemProto->QuestId)
	{
		// Item Starter
		Quest* qst = QuestStorage.LookupEntry(itemProto->QuestId);
		if(!qst)
		{
			return;
		}
			
		WorldPacket data;
		sQuestMgr.BuildQuestDetails(&data, qst, tmpItem, 0, language, _player);
		SendPacket(&data);
	}

	// Let's check if the item even has that spell
	for(int i = 0; i < 5; ++i)
	{
		if(itemProto->Spells[i].Trigger == USE && itemProto->Spells[i].Id == spellId)
		{
			found = true;
			break;//found 1 already
		}
	}

	// Let's see if it is an onuse spellid
	if(tmpItem->HasOnUseSpellID(spellId))
		found = true;

	// We didn't find the spell, so the player is probably trying to cheat
	// with an edited itemcache.wdb
	//
	// Altough this could also happen after a DB update
	// if he/she didn't delete his/her cache.
	if(found == false)
	{

		this->Disconnect();
		Anticheat_Log->writefromsession(this, "Player tried to use an item with a spell that didn't match the spell in the database.");
		Anticheat_Log->writefromsession(this, "Possibly corrupted or intentionally altered itemcache.wdb");
		Anticheat_Log->writefromsession(this, "Itemid: %lu", itemProto->ItemId);
		Anticheat_Log->writefromsession(this, "Spellid: %lu", spellId);
		Anticheat_Log->writefromsession(this, "Player was disconnected");
		return;
	}

	SpellCastTargets targets(recvPacket, _player->GetGUID());
	SpellEntry* spellInfo = dbcSpell.LookupEntryForced(spellId);
	if(spellInfo == NULL)
	{
		sLog.outError("WORLD: unknown spell id %i", spellId);
		return;
	}

	if(spellInfo->AuraInterruptFlags & AURA_INTERRUPT_ON_STAND_UP)
	{
		if(pUser->CombatStatus.IsInCombat() || pUser->IsMounted())
		{
			_player->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_CANT_DO_IN_COMBAT);
			return;
		}

		if(pUser->GetStandState() != 1)
			pUser->SetStandState(STANDSTATE_SIT);
		// loop through the auras and removing existing eating spells
	}
	else   // cebernic: why not stand up
	{
		if(!pUser->CombatStatus.IsInCombat() && !pUser->IsMounted())
		{
			if(pUser->GetStandState())
			{
				pUser->SetStandState(STANDSTATE_STAND);
			}
		}
	}

	// cebernic: remove stealth on using item
	if(!(spellInfo->AuraInterruptFlags & ATTRIBUTESEX_NOT_BREAK_STEALTH))
	{
		if(pUser->IsStealth())
			pUser->RemoveAllAuraType(SPELL_AURA_MOD_STEALTH);
	}

	if(itemProto->RequiredLevel)
	{
		if(_player->getLevel() < itemProto->RequiredLevel)
		{
			_player->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_ITEM_RANK_NOT_ENOUGH);
			return;
		}
	}

	if(itemProto->RequiredSkill)
	{
		if(!_player->_HasSkillLine(itemProto->RequiredSkill))
		{
			_player->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_ITEM_RANK_NOT_ENOUGH);
			return;
		}

		if(itemProto->RequiredSkillRank)
		{
			if(_player->_GetSkillLineCurrent(itemProto->RequiredSkill, false) < itemProto->RequiredSkillRank)
			{
				_player->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_ITEM_RANK_NOT_ENOUGH);
				return;
			}
		}
	}

	if((itemProto->AllowableClass && !(_player->getClassMask() & itemProto->AllowableClass)) || (itemProto->AllowableRace && !(_player->getRaceMask() & itemProto->AllowableRace)))
	{
		_player->GetItemInterface()->BuildInventoryChangeError(tmpItem, NULL, INV_ERR_YOU_CAN_NEVER_USE_THAT_ITEM);
		return;
	}

	if(!_player->Cooldown_CanCast(spellInfo))
	{
		_player->SendCastResult(spellInfo->Id, SPELL_FAILED_NOT_READY, castCount, 0);
		return;
	}


	if(_player->m_currentSpell)
	{
		_player->SendCastResult(spellInfo->Id, SPELL_FAILED_SPELL_IN_PROGRESS, castCount, 0);
		return;
	}

	if(itemProto->ForcedPetId >= 0)
	{
		if(itemProto->ForcedPetId == 0)
		{
			if(_player->GetGUID() != targets.m_unitTarget)
			{
				_player->SendCastResult(spellInfo->Id, SPELL_FAILED_BAD_TARGETS, castCount, 0);
				return;
			}
		}
		else
		{

			if(!_player->GetSummon() || _player->GetSummon()->GetEntry() != (uint32)itemProto->ForcedPetId)
			{
				_player->SendCastResult(spellInfo->Id, SPELL_FAILED_SPELL_IN_PROGRESS, castCount, 0);
				return;
			}
		}
	}
	
	Spell* spell = sSpellFactoryMgr.NewSpell(_player, spellInfo, false, NULL);
	spell->extra_cast_number = castCount;
	spell->i_caster = tmpItem;
	spell->m_glyphslot = glyphIndex;

	//GetPlayer()->setCurrentSpell(spell);
	spell->prepare(&targets);
#ifdef ENABLE_ACHIEVEMENTS
	_player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM, itemProto->ItemId, 0, 0);
#endif

}

void WorldSession::HandleSpellClick(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	LOG_DETAIL("WORLD: got CMSG_SPELLCLICK packet, data length = %i", recvPacket.size());

	if(_player->getDeathState() == CORPSE)
		return;

	uint64 target_guid; // this will store the guid of the object we are going to use it's spell. There must be a dbc that indicates what spells a unit has

	recvPacket >> target_guid;

	//we have only 1 example atm for entry : 28605
	Unit* target_unit = _player->GetMapMgr()->GetUnit(target_guid);

	if(!target_unit)
		return;

	if( !_player->isInRange( target_unit, MAX_INTERACTION_RANGE ) )
		return;

	if( target_unit->IsVehicle() ){
		if( target_unit->GetVehicleComponent() != NULL )
			target_unit->GetVehicleComponent()->AddPassenger( _player );
		return;
	}

	uint32 creature_id = target_unit->GetEntry();
	uint32 cast_spell_id = 0;

	if(!_player->HasAurasWithNameHash(SPELL_HASH_LIGHTWELL_RENEW) && target_unit->RemoveAura(59907))
	{
		SpellClickSpell *sp = SpellClickSpellStorage.LookupEntry( creature_id );
		if( sp == NULL ){
			if( target_unit->IsCreature() ){
				Creature *c = TO< Creature* >( target_unit );
				
				sChatHandler.BlueSystemMessage( this, "NPC Id %u ( %s ) has no spellclick spell associated with it.", c->GetProto()->Id, c->GetCreatureInfo()->Name  );
				LOG_ERROR("Spellclick packet received for creature %u but there is no spell associated with it.", creature_id );
				return;
			}
		}

		cast_spell_id = sp->SpellID;

		target_unit->CastSpell(_player, cast_spell_id, true);

		if(!target_unit->HasAura(59907))
			TO_CREATURE(target_unit)->Despawn(0, 0); //IsCreature() check is not needed, refer to r2387 and r3230

		return;
	}
	
	SpellClickSpell *sp = SpellClickSpellStorage.LookupEntry( creature_id );
	if( sp == NULL ){
		if( target_unit->IsCreature() ){
			Creature *c = TO< Creature* >( target_unit );

			sChatHandler.BlueSystemMessage( this, "NPC Id %u ( %s ) has no spellclick spell associated with it.", c->GetProto()->Id, c->GetCreatureInfo()->Name  );
			LOG_ERROR("Spellclick packet received for creature %u but there is no spell associated with it.", creature_id );
			return;
		}
	}

	cast_spell_id = sp->SpellID;

	if(cast_spell_id == 0)
		return;

	SpellEntry* spellInfo = dbcSpell.LookupEntryForced(cast_spell_id);
	if(spellInfo == NULL)
		return;
	Spell* spell = sSpellFactoryMgr.NewSpell(_player, spellInfo, false, NULL);
	SpellCastTargets targets(target_guid);
	spell->prepare(&targets);
}

void WorldSession::HandleCastSpellOpcode(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	// ignore for remote control state (for player case)
	Unit* mover = _player->m_mover;
	//if (mover != _player && mover->GetTypeId() == TYPEID_PLAYER)
	//{
	//	recvPacket.rfinish(); // prevent spam at ignore packet
	//	return;
	//}

	uint8 castCount = 0;
	uint8 castFlags = 0;
	uint32 spellId = 0;
	uint32 glyphIndex = 0;
	uint32 targetMask = 0;
	uint32 targetStringLength = 0;
	float elevation = 0.0f;
	float missileSpeed = 0.0f;
	ObjectGuid targetGuid = 0;
	ObjectGuid itemTargetGuid = 0;
	ObjectGuid destTransportGuid = 0;
	ObjectGuid srcTransportGuid = 0;
	Position srcPos;
	Position destPos;
	std::string targetString;

	// Movement data
	MovementInfo3 movementInfo;
	ObjectGuid movementTransportGuid = 0;
	ObjectGuid movementGuid = 0;
	bool hasTransport = false;
	bool hasTransportTime2 = false;
	bool hasTransportTime3 = false;
	bool hasFallData = false;
	bool hasFallDirection = false;
	bool hasTimestamp = false;
	bool hasSplineElevation = false;
	bool hasPitch = false;
	bool hasOrientation = false;
	bool hasUnkMovementField = false;
	uint32 unkMovementLoopCounter = 0;
	Unit* caster = mover;

	recvPacket.ReadBit(); // Fake bit
	bool hasTargetString = !recvPacket.ReadBit();
	recvPacket.ReadBit(); // Fake Bit
	bool hasCastCount = !recvPacket.ReadBit();
	bool hasSrcLocation = recvPacket.ReadBit();
	bool hasDestLocation = recvPacket.ReadBit();
	bool hasSpellId = !recvPacket.ReadBit();
	uint8 researchDataCount = recvPacket.ReadBits(2);
	bool hasTargetMask = !recvPacket.ReadBit();
	bool hasMissileSpeed = !recvPacket.ReadBit();

	for (uint8 i = 0; i < researchDataCount; ++i)
		recvPacket.ReadBits(2);

	bool hasGlyphIndex = !recvPacket.ReadBit();
	bool hasMovement = recvPacket.ReadBit();
	bool hasElevation = !recvPacket.ReadBit();
	bool hasCastFlags = !recvPacket.ReadBit();

	targetGuid[5] = recvPacket.ReadBit();
	targetGuid[4] = recvPacket.ReadBit();
	targetGuid[2] = recvPacket.ReadBit();
	targetGuid[7] = recvPacket.ReadBit();
	targetGuid[1] = recvPacket.ReadBit();
	targetGuid[6] = recvPacket.ReadBit();
	targetGuid[3] = recvPacket.ReadBit();
	targetGuid[0] = recvPacket.ReadBit();

	if (hasDestLocation)
	{
		destTransportGuid[1] = recvPacket.ReadBit();
		destTransportGuid[3] = recvPacket.ReadBit();
		destTransportGuid[5] = recvPacket.ReadBit();
		destTransportGuid[0] = recvPacket.ReadBit();
		destTransportGuid[2] = recvPacket.ReadBit();
		destTransportGuid[6] = recvPacket.ReadBit();
		destTransportGuid[7] = recvPacket.ReadBit();
		destTransportGuid[4] = recvPacket.ReadBit();
	}


	if (hasMovement)
	{
		unkMovementLoopCounter = recvPacket.ReadBits(22);
		recvPacket.ReadBit();
		movementGuid[4] = recvPacket.ReadBit();
		hasTransport = recvPacket.ReadBit();

		if (hasTransport)
		{
			hasTransportTime2 = recvPacket.ReadBit();
			movementTransportGuid[7] = recvPacket.ReadBit();
			movementTransportGuid[4] = recvPacket.ReadBit();
			movementTransportGuid[1] = recvPacket.ReadBit();
			movementTransportGuid[0] = recvPacket.ReadBit();
			movementTransportGuid[6] = recvPacket.ReadBit();
			movementTransportGuid[3] = recvPacket.ReadBit();
			movementTransportGuid[5] = recvPacket.ReadBit();
			hasTransportTime3 = recvPacket.ReadBit();
			movementTransportGuid[2] = recvPacket.ReadBit();
		}

		recvPacket.ReadBit();
		movementGuid[7] = recvPacket.ReadBit();
		hasOrientation = !recvPacket.ReadBit();
		movementGuid[6] = recvPacket.ReadBit();
		hasSplineElevation = !recvPacket.ReadBit();
		hasPitch = !recvPacket.ReadBit();
		movementGuid[0] = recvPacket.ReadBit();
		recvPacket.ReadBit();
		bool hasMovementFlags = !recvPacket.ReadBit();
		hasTimestamp = !recvPacket.ReadBit();
		hasUnkMovementField = !recvPacket.ReadBit();

		if (hasMovementFlags)
			movementInfo.flags = recvPacket.ReadBits(30);

		movementGuid[1] = recvPacket.ReadBit();
		movementGuid[3] = recvPacket.ReadBit();
		movementGuid[2] = recvPacket.ReadBit();
		movementGuid[5] = recvPacket.ReadBit();
		hasFallData = recvPacket.ReadBit();

		if (hasFallData)
			hasFallDirection = recvPacket.ReadBit();

		bool hasMovementFlags2 = !recvPacket.ReadBit();

		if (hasMovementFlags2)
			movementInfo.flags2 = recvPacket.ReadBits(13);
	}

	itemTargetGuid[1] = recvPacket.ReadBit();
	itemTargetGuid[0] = recvPacket.ReadBit();
	itemTargetGuid[7] = recvPacket.ReadBit();
	itemTargetGuid[4] = recvPacket.ReadBit();
	itemTargetGuid[6] = recvPacket.ReadBit();
	itemTargetGuid[5] = recvPacket.ReadBit();
	itemTargetGuid[3] = recvPacket.ReadBit();
	itemTargetGuid[2] = recvPacket.ReadBit();

	if (hasSrcLocation)
	{
		srcTransportGuid[4] = recvPacket.ReadBit();
		srcTransportGuid[5] = recvPacket.ReadBit();
		srcTransportGuid[3] = recvPacket.ReadBit();
		srcTransportGuid[0] = recvPacket.ReadBit();
		srcTransportGuid[7] = recvPacket.ReadBit();
		srcTransportGuid[1] = recvPacket.ReadBit();
		srcTransportGuid[6] = recvPacket.ReadBit();
		srcTransportGuid[2] = recvPacket.ReadBit();
	}

	if (hasTargetMask)
		targetMask = recvPacket.ReadBits(20);

	if (hasCastFlags)
		castFlags = recvPacket.ReadBits(5);

	if (hasTargetString)
		targetStringLength = recvPacket.ReadBits(7);

	for (uint8 i = 0; i < researchDataCount; ++i)
	{
		recvPacket.read<uint32>();
		recvPacket.read<uint32>();
	}

	if (hasMovement)
	{
		recvPacket >> movementInfo.pos.m_positionX;
		recvPacket.ReadByteSeq(movementGuid[0]);

		if (hasTransport)
		{
			recvPacket.ReadByteSeq(movementTransportGuid[2]);
			recvPacket >> movementInfo.transport.seat;
			recvPacket.ReadByteSeq(movementTransportGuid[3]);
			recvPacket.ReadByteSeq(movementTransportGuid[7]);
			recvPacket >> movementInfo.transport.pos.m_positionX;
			recvPacket.ReadByteSeq(movementTransportGuid[5]);

			if (hasTransportTime3)
				recvPacket >> movementInfo.transport.time3;

			recvPacket >> movementInfo.transport.pos.m_positionZ;
			recvPacket >> movementInfo.transport.pos.m_positionY;

			recvPacket.ReadByteSeq(movementTransportGuid[6]);
			recvPacket.ReadByteSeq(movementTransportGuid[1]);
			movementInfo.transport.pos.SetOrientation(recvPacket.read<float>());

			recvPacket.ReadByteSeq(movementTransportGuid[4]);

			if (hasTransportTime2)
				recvPacket >> movementInfo.transport.time2;

			recvPacket.ReadByteSeq(movementTransportGuid[0]);
			recvPacket >> movementInfo.transport.time;

		}

		recvPacket.ReadByteSeq(movementGuid[5]);

		if (hasFallData)
		{
			recvPacket >> movementInfo.jump.fallTime;
			recvPacket >> movementInfo.jump.zspeed;

			if (hasFallDirection)
			{
				recvPacket >> movementInfo.jump.sinAngle;
				recvPacket >> movementInfo.jump.xyspeed;
				recvPacket >> movementInfo.jump.cosAngle;
			}
		}

		if (hasSplineElevation)
			recvPacket >> movementInfo.splineElevation;

		recvPacket.ReadByteSeq(movementGuid[6]);

		if (hasUnkMovementField)
			recvPacket.read<uint32>();

		recvPacket.ReadByteSeq(movementGuid[4]);

		if (hasOrientation)
			movementInfo.pos.SetOrientation(recvPacket.read<float>());

		if (hasTimestamp)
			recvPacket >> movementInfo.time;

		recvPacket.ReadByteSeq(movementGuid[1]);

		if (hasPitch)
			movementInfo.pitch = G3D::wrap(recvPacket.read<float>(), float(-M_PI), float(M_PI));

		recvPacket.ReadByteSeq(movementGuid[3]);

		for (uint8 i = 0; i != unkMovementLoopCounter; i++)
			recvPacket.read<uint32>();

		recvPacket >> movementInfo.pos.m_positionY;
		recvPacket.ReadByteSeq(movementGuid[7]);
		recvPacket >> movementInfo.pos.m_positionZ;
		recvPacket.ReadByteSeq(movementGuid[2]);
	}

	recvPacket.ReadByteSeq(itemTargetGuid[4]);
	recvPacket.ReadByteSeq(itemTargetGuid[2]);
	recvPacket.ReadByteSeq(itemTargetGuid[1]);
	recvPacket.ReadByteSeq(itemTargetGuid[5]);
	recvPacket.ReadByteSeq(itemTargetGuid[7]);
	recvPacket.ReadByteSeq(itemTargetGuid[3]);
	recvPacket.ReadByteSeq(itemTargetGuid[6]);
	recvPacket.ReadByteSeq(itemTargetGuid[0]);

	if (hasDestLocation)
	{
		float x, y, z;
		recvPacket.ReadByteSeq(destTransportGuid[2]);
		recvPacket >> x;
		recvPacket.ReadByteSeq(destTransportGuid[4]);
		recvPacket.ReadByteSeq(destTransportGuid[1]);
		recvPacket.ReadByteSeq(destTransportGuid[0]);
		recvPacket.ReadByteSeq(destTransportGuid[3]);
		recvPacket >> y;
		recvPacket.ReadByteSeq(destTransportGuid[7]);
		recvPacket >> z;
		recvPacket.ReadByteSeq(destTransportGuid[5]);
		recvPacket.ReadByteSeq(destTransportGuid[6]);
		destPos.Relocate(x, y, z);
	}
	//else
	//{
	//	destTransportGuid = caster->GetTransGUID();
	//	if (destTransportGuid)
	//		destPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
	//	else
	//		destPos.Relocate(caster);
	//}

	recvPacket.ReadByteSeq(targetGuid[3]);
	recvPacket.ReadByteSeq(targetGuid[4]);
	recvPacket.ReadByteSeq(targetGuid[7]);
	recvPacket.ReadByteSeq(targetGuid[6]);
	recvPacket.ReadByteSeq(targetGuid[2]);
	recvPacket.ReadByteSeq(targetGuid[0]);
	recvPacket.ReadByteSeq(targetGuid[1]);
	recvPacket.ReadByteSeq(targetGuid[5]);

	if (hasSrcLocation)
	{
		float x, y, z;
		recvPacket >> y;
		recvPacket.ReadByteSeq(srcTransportGuid[5]);
		recvPacket.ReadByteSeq(srcTransportGuid[1]);
		recvPacket.ReadByteSeq(srcTransportGuid[7]);
		recvPacket.ReadByteSeq(srcTransportGuid[6]);
		recvPacket >> x;
		recvPacket.ReadByteSeq(srcTransportGuid[3]);
		recvPacket.ReadByteSeq(srcTransportGuid[2]);
		recvPacket.ReadByteSeq(srcTransportGuid[0]);
		recvPacket.ReadByteSeq(srcTransportGuid[4]);
		recvPacket >> z;
		srcPos.Relocate(x, y, z);
	}
	//else
	//{
	//	srcTransportGuid = caster->GetTransGUID();
	//	if (srcTransportGuid)
	//		srcPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
	//	else
	//		srcPos.Relocate(caster);
	//}

	if (hasTargetString)
		targetString = recvPacket.ReadString(targetStringLength);

	if (hasMissileSpeed)
		recvPacket >> missileSpeed;

	if (hasElevation)
		recvPacket >> elevation;

	if (hasCastCount)
		recvPacket >> castCount;

	if (hasSpellId)
		recvPacket >> spellId;

	if (hasGlyphIndex)
		recvPacket >> glyphIndex;

	sLog.outError("WORLD: got cast spell packet, castCount: %u, spellId: %u, castFlags: %u, data length = %u", castCount, spellId, castFlags, (uint32)recvPacket.size());

	
	// check for spell id
	SpellEntry* spellInfo = dbcSpell.LookupEntryForced(spellId);

	if(!spellInfo)
	{
		LOG_ERROR("WORLD: unknown spell id %i", spellId);
		return;
	}

	if(!_player->isAlive() && _player->GetShapeShift() != FORM_SPIRITOFREDEMPTION && !(spellInfo->Attributes & ATTRIBUTES_DEAD_CASTABLE)) //They're dead, not in spirit of redemption and the spell can't be cast while dead.
		return;

	LOG_DETAIL("WORLD: got cast spell packet, spellId - %i (%s), data length = %i",
	           spellId, spellInfo->Name, recvPacket.size());

	// Cheat Detection only if player and not from an item
	// this could fuck up things but meh it's needed ALOT of the newbs are using WPE now
	// WPE allows them to mod the outgoing packet and basically choose what ever spell they want :(

	if(!GetPlayer()->HasSpell(spellId))
	{
		sCheatLog.writefromsession(this, "Cast spell %lu but doesn't have that spell.", spellId);
		LOG_DETAIL("WORLD: Spell isn't cast because player \'%s\' is cheating", GetPlayer()->GetName());
		return;
	}
	if(spellInfo->Attributes & ATTRIBUTES_PASSIVE)
	{
		sCheatLog.writefromsession(this, "Cast passive spell %lu.", spellId);
		LOG_DETAIL("WORLD: Spell isn't cast because player \'%s\' is cheating", GetPlayer()->GetName());
		return;
	}

	if(GetPlayer()->GetOnMeleeSpell() != spellId)
	{
		//autoshot 75
		if((spellInfo->AttributesExB & ATTRIBUTESEXB_ACTIVATE_AUTO_SHOT) /*spellInfo->Attributes == 327698*/)	// auto shot..
		{
			//sLog.outString( "HandleSpellCast: Auto Shot-type spell cast (id %u, name %s)" , spellInfo->Id , spellInfo->Name );
			Item* weapon = GetPlayer()->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_RANGED);
			if(!weapon)
				return;
			uint32 spellid;
			switch(weapon->GetProto()->SubClass)
			{
				case 2:			 // bows
				case 3:			 // guns
				case 18:		 // crossbow
					spellid = SPELL_RANGED_GENERAL;
					break;
				case 16:			// thrown
					spellid = SPELL_RANGED_THROW;
					break;
				case 19:			// wands
					spellid = SPELL_RANGED_WAND;
					break;
				default:
					spellid = 0;
					break;
			}

			if(!spellid)
				spellid = spellInfo->Id;

			if(!_player->m_onAutoShot)
			{
				_player->m_AutoShotTarget = _player->GetSelection();
				uint32 duration = _player->GetBaseAttackTime(RANGED);
				SpellCastTargets targets(recvPacket, GetPlayer()->GetGUID());
				if(!targets.m_unitTarget)
				{
					LOG_DEBUG("Cancelling auto-shot cast because targets.m_unitTarget is null!");
					return;
				}
				SpellEntry* sp = dbcSpell.LookupEntry(spellid);

				_player->m_AutoShotSpell = sp;
				_player->m_AutoShotDuration = duration;
				//This will fix fast clicks
				if(_player->m_AutoShotAttackTimer < 500)
					_player->m_AutoShotAttackTimer = 500;
				_player->m_onAutoShot = true;
			}

			return;
		}

		if(_player->m_currentSpell)
		{
			if(_player->m_currentSpell->getState() == SPELL_STATE_CASTING)
			{
				// cancel the existing channel spell, cast this one
				_player->m_currentSpell->cancel();
			}
			else
			{
				// send the error message
				_player->SendCastResult(spellInfo->Id, SPELL_FAILED_SPELL_IN_PROGRESS, castCount, 0);
				return;
			}
		}

		SpellCastTargets targets(recvPacket, GetPlayer()->GetGUID());

		// some anticheat stuff
		if(spellInfo->self_cast_only)
		{
			if(targets.m_unitTarget && targets.m_unitTarget != _player->GetGUID())
			{
				// send the error message
				_player->SendCastResult(spellInfo->Id, SPELL_FAILED_BAD_TARGETS, castCount, 0);
				return;
			}
		}

		Spell* spell = sSpellFactoryMgr.NewSpell(GetPlayer(), spellInfo, false, NULL);
		spell->extra_cast_number = castCount;
		spell->prepare(&targets);
	}
}

void WorldSession::HandleCancelCastOpcode(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	uint32 spellId;
	recvPacket >> spellId;

	if(GetPlayer()->m_currentSpell)
		GetPlayer()->m_currentSpell->cancel();
}

void WorldSession::HandleCancelAuraOpcode(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	uint32 spellId;
	recvPacket >> spellId;

	if(_player->m_currentSpell && _player->m_currentSpell->GetProto()->Id == spellId)
		_player->m_currentSpell->cancel();
	else
	{
		SpellEntry* info = dbcSpell.LookupEntryForced(spellId);

		if(info != NULL && !(info->Attributes & static_cast<uint32>(ATTRIBUTES_CANT_CANCEL)))
		{
			_player->RemoveAllAuraById(spellId);
			LOG_DEBUG("Removing all auras with ID: %u", spellId);
		}
	}
}

void WorldSession::HandleCancelChannellingOpcode(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	uint32 spellId;
	recvPacket >> spellId;

	Player* plyr = GetPlayer();
	if(!plyr)
		return;
	if(plyr->m_currentSpell)
	{
		plyr->m_currentSpell->cancel();
	}
}

void WorldSession::HandleCancelAutoRepeatSpellOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	//sLog.outString("Received CMSG_CANCEL_AUTO_REPEAT_SPELL message.");
	//on original we automatically enter combat when creature got close to us
//	GetPlayer()->GetSession()->OutPacket(SMSG_CANCEL_COMBAT);
	GetPlayer()->m_onAutoShot = false;
}

void WorldSession::HandlePetCastSpell(WorldPacket & recvPacket)
{
	CHECK_INWORLD_RETURN

	uint64 guid = 0;
	uint8  castCount = 0;
	uint32 spellid = 0;
	uint8  castflags = 0;
	uint32 targetmask = 0;

	recvPacket >> guid;
	recvPacket >> castCount;
	recvPacket >> spellid;
	recvPacket >> castflags;

	SpellEntry* sp = dbcSpell.LookupEntryForced(spellid);
	if(sp == NULL)
		return;
	// Summoned Elemental's Freeze
	if(spellid == 33395)
	{
		if(!_player->GetSummon())
			return;
	}
	else if(guid != _player->m_CurrentCharm)
	{
		if( _player->GetCharmedUnitGUID() != guid )
			return;
	}

	SpellCastTargets targets;
	targets.read( recvPacket, guid );

	float missilepitch = 0.0f;
	float missilespeed = 0;
	uint32 traveltime  = 0;
	
	if( castflags & 2 ){
		recvPacket >> missilepitch;
		recvPacket >> missilespeed;

		float dx = targets.m_destX - targets.m_srcX;
		float dy = targets.m_destY - targets.m_srcY;

		if( ( missilepitch != M_PI / 4 ) && ( missilepitch != -M_PI / 4 ) ) //lets not divide by 0 lul
			traveltime = ( sqrtf( dx * dx + dy * dy ) / ( cosf( missilepitch ) * missilespeed ) ) * 1000;
	}

	if(spellid == 33395)	// Summoned Water Elemental's freeze
	{
		Spell* pSpell = sSpellFactoryMgr.NewSpell(_player->GetSummon(), sp, false, 0);
		pSpell->prepare(&targets);
	}
	else			// trinket?
	{
		uint64 charmguid = _player->m_CurrentCharm;
		if( charmguid == 0 )
			charmguid = _player->GetCharmedUnitGUID();

		Unit* nc = _player->GetMapMgr()->GetUnit( charmguid );
		if(nc)
		{
			bool check = false;
			for(list<AI_Spell*>::iterator itr = nc->GetAIInterface()->m_spells.begin(); itr != nc->GetAIInterface()->m_spells.end(); ++itr)//.......meh. this is a crappy way of doing this, I bet.
			{
				if((*itr)->spell->Id == spellid)
				{
					check = true;
					break;
				}
			}

			if(nc->IsCreature())
			{
				Creature* c = TO< Creature* >(nc);

				if(c->GetProto()->spelldataid != 0)
				{
					CreatureSpellDataEntry* spe = dbcCreatureSpellData.LookupEntry(c->GetProto()->spelldataid);

					if(spe != NULL)
						for(uint32 i = 0; i < 3; i++)
							if(spe->Spells[ i ] == spellid)
							{
								check = true;
								break;
							}
				}

				for(uint32 i = 0; i < 4; i++)
					if(c->GetProto()->AISpells[ i ] == spellid)
					{
						check = true;
						break;
					}
			}

			if(!check)
				return;

			Spell* pSpell = sSpellFactoryMgr.NewSpell(nc, sp, false, 0);
			pSpell->m_missilePitch = missilepitch;
			pSpell->m_missileTravelTime = traveltime;

			pSpell->prepare(&targets);
		}
	}
}

void WorldSession::HandleCancelTotem(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint8 slot;
	recv_data >> slot;

	if(slot >= UNIT_SUMMON_SLOTS)
	{
		LOG_ERROR("Player %u %s tried to cancel a summon at slot %u, slot number is out of range. ( tried to crash the server? )", _player->GetLowGUID(), _player->GetName(), slot);
		return;
	}

	_player->summonhandler.RemoveSummonFromSlot(slot);
}
