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

// @todo change the name to BuildTradeStatus
WorldPacket* WorldSession::SendTradeStatus(uint8 status)
{
    WorldPacket* data = new WorldPacket(SMSG_TRADE_STATUS, 1 + 4 + 4);

    data->WriteBit(0); // unk bit, usually 0
    data->WriteBits(status, 5);

    switch (status)
    {
    case TRADE_STATUS_PROPOSED:
        data->WriteBits(0, 8); // Zero guid
        data->FlushBits();
        break;
    case TRADE_STATUS_INITIATED:
        data->FlushBits();
        *data << uint32(0); // unk
        break;
    case TRADE_STATUS_FAILED:
        data->WriteBit(0); // unk
        data->FlushBits();
        *data << uint32(0); // unk
        *data << uint32(0); // unk
        break;
    case TRADE_STATUS_WRONG_REALM: // Not implemented
    case TRADE_STATUS_NOT_ON_TAPLIST: // Not implemented
        data->FlushBits();
        *data << uint8(0); // unk
        break;
    case TRADE_STATUS_NOT_ENOUGH_CURRENCY: // Not implemented
    case TRADE_STATUS_CURRENCY_NOT_TRADABLE: // Not implemented
        data->FlushBits();
        *data << uint32(0); // unk
        *data << uint32(0); // unk
    default:
        data->FlushBits();
        break;
    }

    return data;
}

void WorldSession::HandleInitiateTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid guid;

    guid[5] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    guid[0] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[0]);
    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[1]);

	Player* pTarget = _player->GetMapMgr()->GetPlayer((uint32)guid);
	uint32 TradeStatus = TRADE_STATUS_PROPOSED;

	if(pTarget == 0)
	{
        //SendTradeStatus(pTarget, TRADE_STATUS_PLAYER_NOT_FOUND); // Target is NULL
        WorldPacket* data = SendTradeStatus(TRADE_STATUS_PLAYER_NOT_FOUND);
        SendPacket(data);
		return;
	}

	// Handle possible error outcomes
	if(pTarget->CalcDistance(_player) > 10.0f)		// This needs to be checked
		TradeStatus = TRADE_STATUS_TOO_FAR_AWAY;
	else if(pTarget->IsDead())
		TradeStatus = TRADE_STATUS_DEAD;
	else if(pTarget->mTradeTarget != 0)
		TradeStatus = TRADE_STATUS_ALREADY_TRADING;
	else if(pTarget->GetTeam() != _player->GetTeam() && GetPermissionCount() == 0 && !sWorld.interfaction_trade)
		TradeStatus = TRADE_STATUS_WRONG_FACTION;

	if(TradeStatus == TRADE_STATUS_PROPOSED)
	{
		_player->ResetTradeVariables();
		pTarget->ResetTradeVariables();

		pTarget->mTradeTarget = _player->GetLowGUID();
		_player->mTradeTarget = pTarget->GetLowGUID();

		pTarget->mTradeStatus = TradeStatus;
		_player->mTradeStatus = TradeStatus;
	}

    ObjectGuid playerGuid = _player->GetGUID();

    WorldPacket data(SMSG_TRADE_STATUS, 2 + 7);
    data.WriteBit(0); // Unknown bit, usually 0
    data.WriteBits(TRADE_STATUS_PROPOSED, 5);

    data.WriteBit(playerGuid[6]);
    data.WriteBit(playerGuid[2]);
    data.WriteBit(playerGuid[1]);
    data.WriteBit(playerGuid[4]);
    data.WriteBit(playerGuid[7]);
    data.WriteBit(playerGuid[3]);
    data.WriteBit(playerGuid[0]);
    data.WriteBit(playerGuid[5]);

    data.WriteByteSeq(playerGuid[6]);
    data.WriteByteSeq(playerGuid[2]);
    data.WriteByteSeq(playerGuid[1]);
    data.WriteByteSeq(playerGuid[7]);
    data.WriteByteSeq(playerGuid[5]);
    data.WriteByteSeq(playerGuid[4]);
    data.WriteByteSeq(playerGuid[0]);
    data.WriteByteSeq(playerGuid[3]);

    pTarget->GetSession()->SendPacket(&data);
}

void WorldSession::HandleBeginTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint32 TradeStatus = TRADE_STATUS_INITIATED;

	Player* plr = _player->GetTradeTarget();
	if(_player->mTradeTarget == 0 || plr == 0)
	{
		TradeStatus = TRADE_STATUS_PLAYER_NOT_FOUND;
        WorldPacket* data = SendTradeStatus(TradeStatus);
		return;
	}
	// We're too far from target now?
	if(_player->CalcDistance(objmgr.GetPlayer(_player->mTradeTarget)) > 10.0f)
		TradeStatus = TRADE_STATUS_TOO_FAR_AWAY;

    WorldPacket* data = SendTradeStatus(TradeStatus);
    plr->m_session->SendPacket(data);
    SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;
}

void WorldSession::HandleBusyTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint32 TradeStatus = TRADE_STATUS_PLAYER_BUSY;

	Player* plr = _player->GetTradeTarget();
	if(_player->mTradeTarget == 0 || plr == 0)
	{

		TradeStatus = TRADE_STATUS_PLAYER_NOT_FOUND;

		OutPacket(TRADE_STATUS_PLAYER_NOT_FOUND, 4, &TradeStatus);
		return;
	}

	OutPacket(SMSG_TRADE_STATUS, 4, &TradeStatus);
	plr->m_session->OutPacket(SMSG_TRADE_STATUS, 4, &TradeStatus);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;

	plr->mTradeTarget = 0;
	_player->mTradeTarget = 0;
}

void WorldSession::HandleIgnoreTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint32 TradeStatus = TRADE_STATUS_PLAYER_IGNORED;

	Player* plr = _player->GetTradeTarget();
	if(_player->mTradeTarget == 0 || plr == 0)
	{
		TradeStatus = TRADE_STATUS_PLAYER_NOT_FOUND;

        WorldPacket* data = SendTradeStatus(TradeStatus);
        SendPacket(data);
		return;
	}

    WorldPacket* data = SendTradeStatus(TradeStatus);
    SendPacket(data);
	plr->m_session->SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;

	plr->mTradeTarget = 0;
	_player->mTradeTarget = 0;
}

void WorldSession::HandleCancelTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	if(_player->mTradeTarget == 0 || _player->mTradeStatus == TRADE_STATUS_COMPLETE)
		return;

	uint32 TradeStatus = TRADE_STATUS_CANCELLED;

    WorldPacket* data = SendTradeStatus(TradeStatus);
    SendPacket(data);

	Player* plr = _player->GetTradeTarget();
	if(plr)
	{
        if (plr->m_session && plr->m_session->GetSocket())
            plr->m_session->SendPacket(data);

		plr->ResetTradeVariables();
	}

	_player->ResetTradeVariables();
}

void WorldSession::HandleUnacceptTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	Player* plr = _player->GetTradeTarget();
	//_player->ResetTradeVariables();

	if(_player->mTradeTarget == 0 || plr == 0)
		return;

	uint32 TradeStatus = TRADE_STATUS_UNACCEPTED;

    WorldPacket* data = SendTradeStatus(TradeStatus);
    SendPacket(data);
    plr->m_session->SendPacket(data);

	TradeStatus = TRADE_STATUS_STATE_CHANGED;

    data = SendTradeStatus(TradeStatus);
    SendPacket(data);
    plr->m_session->SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;
}

void WorldSession::HandleSetTradeItem(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	if(_player->mTradeTarget == 0)
		return;

    uint8 TradeSlot;
    uint8 SourceBag;
    uint8 SourceSlot;

    recv_data >> TradeSlot;
    recv_data >> SourceSlot;
    recv_data >> SourceBag;

	Player* pTarget = _player->GetMapMgr()->GetPlayer(_player->mTradeTarget);

	Item* pItem = _player->GetItemInterface()->GetInventoryItem(SourceBag, SourceSlot);

	if(pTarget == NULL || pItem == NULL || TradeSlot > 6)
		return;
	if(TradeSlot < 6)
	{
		if(pItem->IsAccountbound())
			return;//dual accounting is not allowed so noone can trade Accountbound items. Btw the client doesn't send any client-side notification
		if(pItem->IsSoulbound())
		{
			sCheatLog.writefromsession(this, "tried to cheat trade a soulbound item");
			Disconnect();
			return;
		}
	}

	uint32 TradeStatus = TRADE_STATUS_STATE_CHANGED;
	Player* plr = _player->GetTradeTarget();
	if(!plr)
        return;

    WorldPacket* data = SendTradeStatus(TradeStatus);
    SendPacket(data);
    plr->m_session->SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;

	if(pItem->IsContainer())
	{
		if(TO< Container* >(pItem)->HasItems())
		{
			_player->GetItemInterface()->BuildInventoryChangeError(pItem, 0, INV_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS);

			//--trade cancel

			TradeStatus = TRADE_STATUS_CANCELLED;
            data = SendTradeStatus(TradeStatus);

            SendPacket(data);
			_player->ResetTradeVariables();

            plr->m_session->SendPacket(data);
			plr->ResetTradeVariables();

			return;
		}
	}

	for(uint32 i = 0; i < 8; ++i)
	{
		// duping little shits
		if(_player->mTradeItems[i] == pItem || pTarget->mTradeItems[i] == pItem)
		{
			sCheatLog.writefromsession(this, "tried to dupe an item through trade");
			Disconnect();
			return;
		}
	}

	if(SourceSlot >= INVENTORY_SLOT_BAG_START && SourceSlot < INVENTORY_SLOT_BAG_END)
	{
		Item* itm =  _player->GetItemInterface()->GetInventoryItem(SourceBag); //NULL if it's the backpack or the item is equipped
		if(itm == NULL || SourceSlot >= itm->GetProto()->ContainerSlots) //Required as there are bags with SourceSlot > INVENTORY_SLOT_BAG_START
		{
			//More duping woohoo
			sCheatLog.writefromsession(this, "tried to cheat trade a soulbound item");
			Disconnect();
		}
	}

	_player->mTradeItems[TradeSlot] = pItem;
	_player->SendTradeUpdate();
}

void WorldSession::HandleSetTradeGold(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	if(_player->mTradeTarget == 0)
		return;
	// cebernic: TradeGold sameway.
	uint32 TradeStatus = TRADE_STATUS_STATE_CHANGED;
	Player* plr = _player->GetTradeTarget();
	if(!plr)
        return;

    WorldPacket* data = SendTradeStatus(TradeStatus);
    SendPacket(data);
    plr->m_session->SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;

	uint64 Gold;
	recv_data >> Gold;

	if(_player->mTradeGold != Gold)
	{
		_player->mTradeGold = (Gold > _player->GetGold() ? _player->GetGold() : Gold);
		_player->SendTradeUpdate();
	}
}

void WorldSession::HandleClearTradeItem(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	if(_player->mTradeTarget == 0)
		return;

    uint8 TradeSlot;
    recv_data >> TradeSlot;

	if(TradeSlot > 6)
		return;

	// clean status
	Player* plr = _player->GetTradeTarget();
	if(!plr)
        return;

	uint32 TradeStatus = TRADE_STATUS_STATE_CHANGED;

    WorldPacket* data = SendTradeStatus(TradeStatus);

    SendPacket(data);
    plr->m_session->SendPacket(data);

	plr->mTradeStatus = TradeStatus;
	_player->mTradeStatus = TradeStatus;

	_player->mTradeItems[TradeSlot] = 0;
	_player->SendTradeUpdate();
}

void WorldSession::HandleAcceptTrade(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	Player* plr = _player->GetTradeTarget();
	if(_player->mTradeTarget == 0 || !plr)
		return;

	uint32 TradeStatus = TRADE_STATUS_ACCEPTED;

	// Tell the other player we're green.
    WorldPacket* data = SendTradeStatus(TradeStatus);
    plr->m_session->SendPacket(data);
	_player->mTradeStatus = TradeStatus;

	if(plr->mTradeStatus == TRADE_STATUS_ACCEPTED)
	{
		// Ready!
		uint32 ItemCount = 0;
		uint32 TargetItemCount = 0;
		Player* pTarget = plr;

		/*		// Calculate Item Count
				for(uint32 Index = 0; Index < 7; ++Index)
				{
					if(_player->mTradeItems[Index] != 0)	++ItemCount;
					if(pTarget->mTradeItems[Index] != 0)	++TargetItemCount;
				}*/


		// Calculate Count
		for(uint32 Index = 0; Index < 6; ++Index) // cebernic: checking for 6items ,untradable item check via others func.
		{
			Item* pItem;

			// safely trade checking
			pItem = _player->mTradeItems[Index];
			if(pItem)
			{
				if((pItem->IsContainer() && TO< Container* >(pItem)->HasItems())   || (pItem->GetProto()->Bonding == ITEM_BIND_ON_PICKUP))
				{
					ItemCount = 0;
					TargetItemCount = 0;
					break;
				}
				else ++ItemCount;
			}

			pItem = pTarget->mTradeItems[Index];
			if(pItem)
			{
				if((pItem->IsContainer() && TO< Container* >(pItem)->HasItems())   || (pItem->GetProto()->Bonding == ITEM_BIND_ON_PICKUP))
				{
					ItemCount = 0;
					TargetItemCount = 0;
					break;
				}
				else ++TargetItemCount;
			}

			//if(_player->mTradeItems[Index] != 0)	++ItemCount;
			//if(pTarget->mTradeItems[Index] != 0)	++TargetItemCount;
		}

		if((_player->m_ItemInterface->CalculateFreeSlots(NULL) + ItemCount) < TargetItemCount ||
		        (pTarget->m_ItemInterface->CalculateFreeSlots(NULL) + TargetItemCount) < ItemCount ||
		        (ItemCount == 0 && TargetItemCount == 0 && !pTarget->mTradeGold && !_player->mTradeGold))	// cebernic added it
		{
			// Not enough slots on one end.
			TradeStatus = TRADE_STATUS_CANCELLED;
		}
		else
		{
			uint64 Guid;
			Item* pItem;

			// Remove all items from the players inventory
			for(uint32 Index = 0; Index < 6; ++Index)
			{
				Guid = _player->mTradeItems[Index] ? _player->mTradeItems[Index]->GetGUID() : 0;
				if(Guid != 0)
				{
					if(_player->mTradeItems[Index]->GetProto()->Bonding == ITEM_BIND_ON_PICKUP ||
					        _player->mTradeItems[Index]->GetProto()->Bonding  >=  ITEM_BIND_QUEST)
					{
						_player->mTradeItems[Index] = NULL;
					}
					else
					{
						if(GetPermissionCount() > 0)
						{
							sGMLog.writefromsession(this, "traded item %s to %s", _player->mTradeItems[Index]->GetProto()->Name1, pTarget->GetName());
						}
						pItem = _player->m_ItemInterface->SafeRemoveAndRetreiveItemByGuid(Guid, true);
					}
				}

				Guid = pTarget->mTradeItems[Index] ? pTarget->mTradeItems[Index]->GetGUID() : 0;
				if(Guid != 0)
				{
					if(pTarget->mTradeItems[Index]->GetProto()->Bonding == ITEM_BIND_ON_PICKUP ||
					        pTarget->mTradeItems[Index]->GetProto()->Bonding  >=  ITEM_BIND_QUEST)
					{
						pTarget->mTradeItems[Index] = NULL;
					}
					else
					{
						pTarget->m_ItemInterface->SafeRemoveAndRetreiveItemByGuid(Guid, true);
					}
				}
			}

			// Dump all items back into the opposite players inventory
			for(uint32 Index = 0; Index < 6; ++Index)
			{
				pItem = _player->mTradeItems[Index];
				if(pItem != NULL)
				{
					pItem->SetOwner(pTarget); // crash fixed.
					if(!pTarget->m_ItemInterface->AddItemToFreeSlot(pItem))
						pItem->DeleteMe();
				}

				pItem = pTarget->mTradeItems[Index];
				if(pItem != NULL)
				{
					pItem->SetOwner(_player);
					if(!_player->m_ItemInterface->AddItemToFreeSlot(pItem))
						pItem->DeleteMe();
				}
			}

			// Trade Gold
			if(pTarget->mTradeGold)
			{
				// Check they don't have more than the max gold
				if(sWorld.GoldCapEnabled && (_player->GetGold() + pTarget->mTradeGold) > sWorld.GoldLimit)
				{
					_player->GetItemInterface()->BuildInventoryChangeError(NULL, NULL, INV_ERR_TOO_MUCH_GOLD);
				}
				else
				{
					_player->ModGold(pTarget->mTradeGold);
					pTarget->ModGold(-(int32)pTarget->mTradeGold);
				}
			}

			if(_player->mTradeGold)
			{
				// Check they don't have more than the max gold
				if(sWorld.GoldCapEnabled && (pTarget->GetGold() + _player->mTradeGold) > sWorld.GoldLimit)
				{
					pTarget->GetItemInterface()->BuildInventoryChangeError(NULL, NULL, INV_ERR_TOO_MUCH_GOLD);
				}
				else
				{
					pTarget->ModGold(_player->mTradeGold);
					_player->ModGold(-(int32)_player->mTradeGold);
				}
			}

			// Close trade window
			TradeStatus = TRADE_STATUS_COMPLETE;
		}

        WorldPacket* data = SendTradeStatus(TradeStatus);
        SendPacket(data);
        plr->m_session->SendPacket(data);

		_player->mTradeStatus = TRADE_STATUS_COMPLETE;
		plr->mTradeStatus = TRADE_STATUS_COMPLETE;

		// Reset Trade Vars
		_player->ResetTradeVariables();
		pTarget->ResetTradeVariables();

		// Save for each other
		plr->SaveToDB(false);
		_player->SaveToDB(false);
	}
}