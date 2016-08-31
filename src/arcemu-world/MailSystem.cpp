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
initialiseSingleton(MailSystem);

void MailSystem::StartMailSystem()
{

}

MailError MailSystem::DeliverMessage(uint64 recipent, MailMessage* message)
{
	// assign a new id
	message->message_id = objmgr.GenerateMailID();

	Player* plr = objmgr.GetPlayer((uint32)recipent);
	if(plr != NULL)
	{
		plr->m_mailBox.AddMessage(message);
		if((uint32)UNIXTIME >= message->delivery_time)
		{
			uint32 v = 0;
			plr->GetSession()->OutPacket(SMSG_RECEIVED_MAIL, 4, &v);
		}
	}

	SaveMessageToSQL(message);
	return MAIL_OK;
}

void Mailbox::AddMessage(MailMessage* Message)
{
	Messages[Message->message_id] = *Message;
}

void Mailbox::DeleteMessage(uint32 MessageId, bool sql)
{
	Messages.erase(MessageId);
	if(sql)
		CharacterDatabase.WaitExecute("DELETE FROM mailbox WHERE message_id = %u", MessageId);
}

WorldPacket* Mailbox::BuildMailboxListingPacket()
{
	MessageMap::iterator itr;
	uint32 realcount = 0;
	uint32 count = 0;
	uint32 t = (uint32)UNIXTIME;

    ByteBuffer* mailData = new ByteBuffer;
    WorldPacket* data = new WorldPacket(SMSG_MAIL_LIST_RESULT, 1000);
    *data << uint32(0); // Placeholder

    size_t mailCountPos = data->bitwpos();
    data->WriteBits(0, 18); // Placeholder

	for(itr = Messages.begin(); itr != Messages.end(); ++itr)
	{
		if(itr->second.expire_time && t > itr->second.expire_time)
			continue;	   // expired mail -> skip it

		if((uint32)UNIXTIME < itr->second.delivery_time)
			continue;		// undelivered

		if(count >= 50) //VLack: We could calculate message sizes instead of this, but the original code did a break at 50, so I won't fix this up if no one felt the need to do so before ;-)
		{
			++realcount;
			continue;
		}

		if(itr->second.AddMessageDataToPacket(*data, *mailData))
		{
			++count;
			++realcount;
		}
	}

    data->FlushBits();
    data->append(mailData);

    data->put<uint32>(0, realcount);
    data->PutBits(mailCountPos, count, 18);

	// Do cleanup on request mail
	CleanupExpiredMessages();
	return data;
}

void Mailbox::CleanupExpiredMessages()
{
	MessageMap::iterator itr, it2;
	uint32 curtime = (uint32)UNIXTIME;

	for(itr = Messages.begin(); itr != Messages.end();)
	{
		it2 = itr++;
		if(it2->second.expire_time && it2->second.expire_time < curtime)
		{
			Messages.erase(it2);
		}
	}
}

bool MailMessage::AddMessageDataToPacket(WorldPacket & data, ByteBuffer& mailData)
{
	uint8 i = 0;
	uint32 j;
	vector<uint32>::iterator itr;
	Item* pItem;

	// add stuff
	if(deleted_flag)
		return false;

	uint8 guidsize;
	if(message_type == 0)
		guidsize = 8;
	else
		guidsize = 4;
    
    data.WriteBit(message_type != MAIL_NORMAL ? 1 : 0);
    data.WriteBits(subject.size(), 8);
    data.WriteBits(body.size(), 13);
    data.WriteBit(0);
    data.WriteBit(0);

    size_t itemCountPos = data.bitwpos();
    data.WriteBits(0, 17); // Placeholder

    data.WriteBit(1); // Has guid

    ObjectGuid guid = message_type == MAIL_NORMAL ? MAKE_NEW_GUID(sender_guid, 0, HIGHGUID_TYPE_PLAYER) : 0;
    data.WriteBit(guid[2]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[4]);

    if (!items.empty())
    {
        for (itr = items.begin(); itr != items.end(); ++itr)
        {
            pItem = objmgr.LoadItem(*itr);
            if (pItem == NULL)
                continue;

            data.WriteBit(0);

            mailData << uint32(pItem->GetLowGUID());
            mailData << uint32(4); // Unknown
            mailData << uint32(pItem->GetChargesLeft()); // Or total spell charges?
            mailData << uint32(pItem->GetDurability());
            mailData << uint32(0); // Unknown

            for (j = 0; j < 7; ++j) // 7 or 8?
            {
                mailData << uint32(pItem->GetEnchantmentCharges(j));
                mailData << uint32(pItem->GetEnchantmentDuration(j));
                mailData << uint32(pItem->GetEnchantmentId(j));
            }

            mailData << uint32(0) << uint32(0) << uint32(0); // Look up, it must be < 8

            mailData << uint32(pItem->GetItemRandomSuffixFactor());
            mailData << int32(pItem->GetItemRandomPropertyId());
            mailData << uint32(pItem->GetDurabilityMax());
            mailData << uint32(pItem->GetStackCount());
            mailData << uint8(i++);
            mailData << uint32(pItem->GetEntry());

            delete pItem;
        }
    }

    data.PutBits(itemCountPos, items.size(), 17);

    mailData.WriteString(body);
    mailData << uint32(message_id);
    mailData.WriteByteSeq(guid[4]);
    mailData.WriteByteSeq(guid[0]);
    mailData.WriteByteSeq(guid[5]);
    mailData.WriteByteSeq(guid[3]);
    mailData.WriteByteSeq(guid[1]);
    mailData.WriteByteSeq(guid[7]);
    mailData.WriteByteSeq(guid[2]);
    mailData.WriteByteSeq(guid[6]);
    mailData << uint32(0); // Mail template id
    mailData << uint64(cod);
    mailData.WriteString(subject);
    mailData << uint32(stationery);
    mailData << float(expire_time - uint32(UNIXTIME) / 86400.0f);
    mailData << uint64(money);
    mailData << uint32(checked_flag);

    if (message_type != MAIL_NORMAL)
        mailData << uint32(sender_guid);

    mailData << uint8(message_type);
    mailData << uint32(0); // Unknown

	return true;
}

void MailSystem::SaveMessageToSQL(MailMessage* message)
{
	stringstream ss;


	ss << "DELETE FROM mailbox WHERE message_id = ";
	ss << message->message_id;
	ss << ";";

	CharacterDatabase.ExecuteNA(ss.str().c_str());

	ss.rdbuf()->str("");

	vector< uint32 >::iterator itr;
	ss << "INSERT INTO mailbox VALUES("
	   << message->message_id << ","
	   << message->message_type << ","
	   << message->player_guid << ","
	   << message->sender_guid << ",\'"
	   << CharacterDatabase.EscapeString(message->subject) << "\',\'"
	   << CharacterDatabase.EscapeString(message->body) << "\',"
	   << message->money << ",'";

	for(itr = message->items.begin(); itr != message->items.end(); ++itr)
		ss << (*itr) << ",";

	ss << "',"
	   << message->cod << ","
	   << message->stationery << ","
	   << message->expire_time << ","
	   << message->delivery_time << ","
	   << message->checked_flag << ","
	   << message->deleted_flag << ");";

	CharacterDatabase.ExecuteNA(ss.str().c_str());
}

void WorldSession::HandleSendMail(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	MailMessage msg;
	uint64 gameobject;
	uint32 unk1, unk2;
	uint8 itemCount;
	uint8 i;
	vector< Item* > items;
	vector< Item* >::iterator itr;
	Item* pItem;
    ObjectGuid mailbox;
	//uint32 err = MAIL_OK;

    // Need to be defined because cod & money in msg are uint32
    // Probably we can remove this when we update money handling to 64 bits
    uint64 money, COD;

    recv_data >> unk1 >> unk2; // Unknown
    recv_data >> COD >> money;

    msg.cod = COD;
    msg.money = money;

    uint32 bodyLength, subjectLength, receiverLength;
    std::string receiverName;

    mailbox[0] = recv_data.ReadBit();
    mailbox[6] = recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    bodyLength = recv_data.ReadBits(11);
    mailbox[3] = recv_data.ReadBit();
    receiverLength = recv_data.ReadBits(9);
    mailbox[7] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    itemCount = recv_data.ReadBits(5); // Attached items count

    ObjectGuid itemGuids[MAIL_MAX_ITEM_SLOT]; // To-Do change this name to something simpler
    for (uint8 i = 0; i < itemCount; ++i)
    {
        itemGuids[i][1] = recv_data.ReadBit();
        itemGuids[i][7] = recv_data.ReadBit();
        itemGuids[i][2] = recv_data.ReadBit();
        itemGuids[i][5] = recv_data.ReadBit();
        itemGuids[i][0] = recv_data.ReadBit();
        itemGuids[i][6] = recv_data.ReadBit();
        itemGuids[i][3] = recv_data.ReadBit();
        itemGuids[i][4] = recv_data.ReadBit();
    }

    subjectLength = recv_data.ReadBits(9);
    mailbox[2] = recv_data.ReadBit();

    for (uint8 i = 0; i < itemCount; ++i)
    {
        recv_data.read<uint8>(); // Item slot in mail, not used
        recv_data.ReadByteSeq(itemGuids[i][3]);
        recv_data.ReadByteSeq(itemGuids[i][0]);
        recv_data.ReadByteSeq(itemGuids[i][2]);
        recv_data.ReadByteSeq(itemGuids[i][1]);
        recv_data.ReadByteSeq(itemGuids[i][6]);
        recv_data.ReadByteSeq(itemGuids[i][5]);
        recv_data.ReadByteSeq(itemGuids[i][7]);
        recv_data.ReadByteSeq(itemGuids[i][4]);
    }

    recv_data.ReadByteSeq(mailbox[1]);
    msg.body = recv_data.ReadString(bodyLength);
    recv_data.ReadByteSeq(mailbox[0]);
    msg.subject = recv_data.ReadString(subjectLength);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[7]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[4]);
    receiverName = recv_data.ReadString(receiverLength);

    msg.stationery = MAIL_STATIONERY_CHR; //! To-Do fix this

	if(itemCount > MAIL_MAX_ITEM_SLOT || msg.body.find("%") != string::npos || msg.subject.find("%") != string::npos)
	{
		SendMailError(MAIL_ERR_INTERNAL_ERROR);
		return;
	}

	// Search for the recipient
    PlayerInfo* player = ObjectMgr::getSingleton().GetPlayerInfoByName(receiverName.c_str());
	if(player == NULL)
	{
		SendMailError(MAIL_ERR_RECIPIENT_NOT_FOUND);
		return;
	}

	for(i = 0; i < itemCount; ++i)
	{
		pItem = _player->GetItemInterface()->GetItemByGUID(itemGuids[i]);
		if(pItem == NULL || pItem->IsSoulbound() || pItem->IsConjured())
		{
			SendMailError(MAIL_ERR_INTERNAL_ERROR);
			return;
		}
		if(pItem->IsAccountbound() && GetAccountId() !=  player->acct) // don't mail account-bound items to another account
		{
			WorldPacket data(SMSG_SEND_MAIL_RESULT, 16);
			data << uint32(0);
			data << uint32(0);
			data << uint32(MAIL_ERR_BAG_FULL);
			data << uint32(INV_ERR_ARTEFACTS_ONLY_FOR_OWN_CHARACTERS);
			SendPacket(&data);
			return;
		}

		items.push_back(pItem);
	}

	bool interfaction = false;
	if(sMailSystem.MailOption(MAIL_FLAG_CAN_SEND_TO_OPPOSITE_FACTION) || (HasGMPermissions() && sMailSystem.MailOption(MAIL_FLAG_CAN_SEND_TO_OPPOSITE_FACTION_GM)))
	{
		interfaction = true;
	}

	// Check we're sending to the same faction (disable this for testing)
	if(player->team != _player->GetTeam() && !interfaction)
	{
		SendMailError(MAIL_ERR_NOT_YOUR_ALLIANCE);
		return;
	}

	// Check if we're sending mail to ourselves
	if(strcmp(player->name, _player->GetName()) == 0 && !GetPermissionCount())
	{
		SendMailError(MAIL_ERR_CANNOT_SEND_TO_SELF);
		return;
	}

	if(msg.stationery == MAIL_STATIONERY_GM && !HasGMPermissions())
	{
		SendMailError(MAIL_ERR_INTERNAL_ERROR);
		return;
	}

	// Instant delivery time by default.
	msg.delivery_time = (uint32)UNIXTIME;

	// Set up the cost
	int32 cost = 0;

	// Check for attached money
	if(msg.money > 0)
		cost += msg.money;

	if(cost < 0)
	{
		SendMailError(MAIL_ERR_INTERNAL_ERROR);
		return;
	}

	if(!sMailSystem.MailOption(MAIL_FLAG_DISABLE_POSTAGE_COSTS) && !(GetPermissionCount() && sMailSystem.MailOption(MAIL_FLAG_NO_COST_FOR_GM)))
	{
		cost += 30;
		if(cost < 30)  // Overflow prevention for those silly WPE hoez.
		{
			SendMailError(MAIL_ERR_INTERNAL_ERROR);
			return;
		}
	}

	// Check that we have enough in our backpack
	if(!_player->HasGold(cost))
	{
		SendMailError(MAIL_ERR_NOT_ENOUGH_MONEY);
		return;
	}

	// Check for the item, and required item.
	if(!items.empty())
	{
		for(itr = items.begin(); itr != items.end(); ++itr)
		{
			pItem = *itr;
			if(_player->GetItemInterface()->SafeRemoveAndRetreiveItemByGuid(pItem->GetGUID(), false) != pItem)
				continue;		// should never be hit.

			pItem->RemoveFromWorld();
			pItem->SetOwner(NULL);
			pItem->SaveToDB(INVENTORY_SLOT_NOT_SET, 0, true, NULL);
			msg.items.push_back(pItem->GetLowGUID());

			if(GetPermissionCount() > 0)
			{
				/* log the message */
				sGMLog.writefromsession(this, "sent mail with item entry %u to %s, with gold %u.", pItem->GetEntry(), player->name, msg.money);
			}

			pItem->DeleteMe();
		}
	}

	if(msg.money != 0 || msg.cod != 0 || (!msg.items.size() && player->acct != _player->GetSession()->GetAccountId()))
	{
		if(!sMailSystem.MailOption(MAIL_FLAG_DISABLE_HOUR_DELAY_FOR_ITEMS))
			msg.delivery_time += 3600;  // 1hr
	}

	// take the money
	_player->ModGold(-cost);

	// Fill in the rest of the info
	msg.player_guid = player->guid;
	msg.sender_guid = _player->GetGUID();

	// 30 day expiry time for unread mail
	if(!sMailSystem.MailOption(MAIL_FLAG_NO_EXPIRY))
		msg.expire_time = (uint32)UNIXTIME + (TIME_DAY * MAIL_DEFAULT_EXPIRATION_TIME);
	else
		msg.expire_time = 0;

	msg.deleted_flag = false;
	msg.message_type = 0;
	msg.checked_flag = msg.body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY;

	// Great, all our info is filled in. Now we can add it to the other players mailbox.
	sMailSystem.DeliverMessage(player->guid, &msg);
	// Save/Update character's gold if they've received gold that is. This prevents a rollback.
	CharacterDatabase.Execute("UPDATE characters SET gold = %u WHERE guid = %u", _player->GetGold(), _player->m_playerInfo->guid);
	// Success packet :)
	SendMailError(MAIL_OK);
}

void WorldSession::HandleMarkAsRead(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid mailbox;
    uint32 mailId;

    recv_data >> mailId;

    mailbox[0] = recv_data.ReadBit();
    mailbox[2] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[6] = recv_data.ReadBit();
    mailbox[7] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    recv_data.FlushBits();

    recv_data.ReadByteSeq(mailbox[1]);
    recv_data.ReadByteSeq(mailbox[7]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[0]);

	MailMessage* message = _player->m_mailBox.GetMessage(mailId);
	if(message == 0)
        return;

	// mark the message as read
	message->checked_flag |= MAIL_CHECK_MASK_READ;

	// mail now has a 30 day expiry time
	if(!sMailSystem.MailOption(MAIL_FLAG_NO_EXPIRY))
		message->expire_time = (uint32)UNIXTIME + (TIME_DAY * 30);

	// update it in sql
	CharacterDatabase.WaitExecute("UPDATE mailbox SET checked_flag = %u, expiry_time = %u WHERE message_id = %u",
		message->checked_flag, message->expire_time, message->message_id);
}

void WorldSession::HandleMailDelete(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN
    
    uint32 mailId;

    recv_data >> mailId;
    recv_data.read<uint32>(); // mailTemplateId

	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
    data << mailId << uint32(MAIL_RES_DELETED);

    MailMessage* message = _player->m_mailBox.GetMessage(mailId);
	if(message == 0)
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

    _player->m_mailBox.DeleteMessage(mailId, true);

	data << uint32(MAIL_OK);
	SendPacket(&data);
}

void WorldSession::HandleTakeItem(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid mailbox;
    uint32 mailId;
    uint32 itemId;

    recv_data >> mailId;
    recv_data >> itemId;

    mailbox[6] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    mailbox[2] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    mailbox[0] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[7] = recv_data.ReadBit();

    recv_data.ReadByteSeq(mailbox[0]);
    recv_data.ReadByteSeq(mailbox[1]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[7]);

	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
    data << mailId << uint32(MAIL_RES_ITEM_TAKEN);

    MailMessage* message = _player->m_mailBox.GetMessage(mailId);
	if(message == 0 || message->items.empty())
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

    vector< uint32 >::iterator itr;
	for(itr = message->items.begin(); itr != message->items.end(); ++itr)
	{
		if((*itr) == itemId) //! Is this correct?
			break;
	}

	if(itr == message->items.end())
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	// Check for cod credit
	if(message->cod > 0)
	{
		if(!_player->HasGold(message->cod))
		{
			data << uint32(MAIL_ERR_NOT_ENOUGH_MONEY);
			SendPacket(&data);
			return;
		}
	}

	// grab the item
	Item* item = objmgr.LoadItem(*itr);
	if(item == 0)
	{
		// doesn't exist
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	// Find free slot
	SlotResult result = _player->GetItemInterface()->FindFreeInventorySlot(item->GetProto());
	if(result.Result == 0)
	{
		// End of slots
		data << uint32(MAIL_ERR_BAG_FULL);
		SendPacket(&data);

		item->DeleteMe();
		return;
	}
	item->m_isDirty = true;

	if(!_player->GetItemInterface()->SafeAddItem(item, result.ContainerSlot, result.Slot))
	{
		if(!_player->GetItemInterface()->AddItemToFreeSlot(item))
		{
			//End of slots
			data << uint32(MAIL_ERR_BAG_FULL);
			SendPacket(&data);
			item->DeleteMe();
			return;
		}
	}
	else
		item->SaveToDB(result.ContainerSlot, result.Slot, true, NULL);

	// send complete packet
	data << uint32(MAIL_OK);
	data << item->GetLowGUID();
	data << item->GetStackCount();

	message->items.erase(itr);

	// re-save (update the items field)
	sMailSystem.SaveMessageToSQL(message);
	SendPacket(&data);

	if(message->cod > 0)
	{
		_player->ModGold(-(int32)message->cod);
		string subject = "COD Payment: ";
		subject += message->subject;
		sMailSystem.SendAutomatedMessage(NORMAL, message->player_guid, message->sender_guid, subject, "", message->cod, 0, 0, MAIL_STATIONERY_TEST1, MAIL_CHECK_MASK_COD_PAYMENT);

		message->cod = 0;
		CharacterDatabase.Execute("UPDATE mailbox SET cod = 0 WHERE message_id = %u", message->message_id);
	}

	// probably need to send an item push here
}

void WorldSession::HandleTakeMoney(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid mailbox;
    uint64 money;
    uint32 mailId;

    recv_data >> mailId;
    recv_data >> money;

    mailbox[7] = recv_data.ReadBit();
    mailbox[6] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    mailbox[2] = recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    mailbox[0] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();

    recv_data.ReadByteSeq(mailbox[7]);
    recv_data.ReadByteSeq(mailbox[1]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[0]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[5]);


	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
    data << mailId << uint32(MAIL_RES_MONEY_TAKEN);

    MailMessage* message = _player->m_mailBox.GetMessage(mailId);
	if(message == 0 || !message->money)
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	// Check they don't have more than the max gold
	if(sWorld.GoldCapEnabled)
	{
		if((_player->GetGold() + message->money) > sWorld.GoldLimit)
		{
			_player->GetItemInterface()->BuildInventoryChangeError(NULL, NULL, INV_ERR_TOO_MUCH_GOLD);
			return;
		}
	}

	// Add the money to the player
	_player->ModGold(message->money);

	// Message no longer has any money
	message->money = 0;

	// Update in sql!
	CharacterDatabase.WaitExecute("UPDATE mailbox SET money = 0 WHERE message_id = %u", message->message_id);

	// Send result
	data << uint32(MAIL_OK);
	SendPacket(&data);
}

void WorldSession::HandleReturnToSender(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid mailbox;
    uint32 mailId;

    recv_data >> mailId;

    mailbox[2] = recv_data.ReadBit();
    mailbox[0] = recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[6] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    mailbox[7] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();

    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[0]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[1]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[7]);

	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
	data << mailId << uint32(MAIL_RES_RETURNED_TO_SENDER);

    MailMessage* msg = _player->m_mailBox.GetMessage(mailId);
	if(msg == 0)
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	// Copy into a new struct
	MailMessage message = *msg;

	// Remove the old message
    _player->m_mailBox.DeleteMessage(mailId, true);

	// Re-assign the owner/sender
	message.player_guid = message.sender_guid;
	message.sender_guid = _player->GetGUID();

	message.deleted_flag = false;
	message.checked_flag = MAIL_CHECK_MASK_RETURNED;

	// Null out the cod charges. (the sender doesn't want to have to pay for his own item
	// That he got nothing for.. :p)
	message.cod = 0;

	// Assign new delivery time
	message.delivery_time = message.items.empty() ? (uint32)UNIXTIME : (uint32)UNIXTIME + 3600;

	// Add to the senders mailbox
	sMailSystem.DeliverMessage(message.player_guid, &message);

	// Finish the packet
	data << uint32(MAIL_OK);
	SendPacket(&data);
}

void WorldSession::HandleMailCreateTextItem(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

    ObjectGuid mailbox;
    uint32 mailId;

    recv_data >> mailId;

    mailbox[4] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    mailbox[6] = recv_data.ReadBit();
    mailbox[2] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    mailbox[0] = recv_data.ReadBit();
    mailbox[7] = recv_data.ReadBit();

    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[0]);
    recv_data.ReadByteSeq(mailbox[7]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[1]);


	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
    data << mailId << uint32(MAIL_RES_MADE_PERMANENT);


	ItemPrototype* proto = ItemPrototypeStorage.LookupEntry(8383); //! To-Do check what is this
    MailMessage* message = _player->m_mailBox.GetMessage(mailId);
	if(message == 0 || !proto)
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	SlotResult result = _player->GetItemInterface()->FindFreeInventorySlot(proto);
	if(result.Result == 0)
	{
		data << uint32(MAIL_ERR_INTERNAL_ERROR);
		SendPacket(&data);

		return;
	}

	Item* pItem = objmgr.CreateItem(8383, _player);
	if(pItem == NULL)
		return;

	pItem->SetFlag( ITEM_FIELD_FLAGS, ITEM_FLAG_WRAP_GIFT ); // The flag is probably misnamed
	pItem->SetText( message->body );

	if(_player->GetItemInterface()->AddItemToFreeSlot(pItem))
	{
		data << uint32(MAIL_OK);
		SendPacket(&data);
	}
	else
	{
		pItem->DeleteMe();
	}
}

void WorldSession::HandleItemTextQuery(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	uint64 itemGuid;
	recv_data >> itemGuid;

	Item* pItem = _player->GetItemInterface()->GetItemByGUID(itemGuid);
	WorldPacket data(SMSG_ITEM_TEXT_QUERY_RESPONSE, pItem->GetText().size() + 9 );
	if(!pItem)
		data << uint8(1);
	else
	{
		data << uint8(0);
		data << uint64(itemGuid);
		data << pItem->GetText();
	}

	SendPacket(&data);
}

void Mailbox::FillTimePacket(WorldPacket & data)
{
	uint32 count = 0;
	MessageMap::iterator iter = Messages.begin();
	data << uint32(0) << uint32(0);

	for(; iter != Messages.end(); ++iter)
	{
		if(iter->second.checked_flag & MAIL_CHECK_MASK_READ)
			continue;

		if(iter->second.deleted_flag == 0  && (uint32)UNIXTIME >= iter->second.delivery_time)
		{
			// Unread message, w00t.
            ++count;
            data << uint64(iter->second.message_type == MAIL_NORMAL ? iter->second.sender_guid : 0); // Player
            data << uint64(iter->second.message_type != MAIL_NORMAL ? iter->second.sender_guid : 0); // Not a player
            data << uint32(iter->second.message_type);
            data << uint32(iter->second.stationery);
            data << float(UNIXTIME - iter->second.delivery_time); // Is this right? (maybe iter->second.delivery_time - UNIXTIME)
		}
	}

    if (count)
        data.put<uint32>(4, count);
    else
    {
        data << float(-TIME_DAY);
        data << uint32(0);
    }
}

//! To-Do fix this
void WorldSession::HandleMailTime(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	WorldPacket data(MSG_QUERY_NEXT_MAIL_TIME, 100);
	_player->m_mailBox.FillTimePacket(data);
	SendPacket(&data);
}

void WorldSession::SendMailError(uint32 error)
{
	WorldPacket data(SMSG_SEND_MAIL_RESULT, 12);
	data << uint32(0);
	data << uint32(MAIL_RES_MAIL_SENT);
	data << error;
	SendPacket(&data);
}

void WorldSession::HandleGetMail(WorldPacket & recv_data)
{
    CHECK_INWORLD_RETURN

    ObjectGuid mailbox;

    mailbox[6] = recv_data.ReadBit();
    mailbox[3] = recv_data.ReadBit();
    mailbox[7] = recv_data.ReadBit();
    mailbox[5] = recv_data.ReadBit();
    mailbox[4] = recv_data.ReadBit();
    mailbox[1] = recv_data.ReadBit();
    mailbox[2] = recv_data.ReadBit();
    mailbox[0] = recv_data.ReadBit();

    recv_data.ReadByteSeq(mailbox[7]);
    recv_data.ReadByteSeq(mailbox[1]);
    recv_data.ReadByteSeq(mailbox[6]);
    recv_data.ReadByteSeq(mailbox[5]);
    recv_data.ReadByteSeq(mailbox[4]);
    recv_data.ReadByteSeq(mailbox[2]);
    recv_data.ReadByteSeq(mailbox[3]);
    recv_data.ReadByteSeq(mailbox[0]);

	WorldPacket* data = _player->m_mailBox.BuildMailboxListingPacket();
	SendPacket(data);
	delete data;
}

void MailSystem::RemoveMessageIfDeleted(uint32 message_id, Player* plr)
{
	MailMessage* msg = plr->m_mailBox.GetMessage(message_id);
	if(msg == 0) return;

	if(msg->deleted_flag)   // we've deleted from inbox
		plr->m_mailBox.DeleteMessage(message_id, true);   // wipe the message
}

void MailSystem::SendAutomatedMessage(uint32 type, uint64 sender, uint64 receiver, string subject, string body,
                                      uint32 money, uint32 cod, vector<uint64> &item_guids, uint32 stationery, MailCheckMask checked, uint32 deliverdelay)
{
	// This is for sending automated messages, for example from an auction house.
	MailMessage msg;
	msg.message_type = type;
	msg.sender_guid = sender;
	msg.player_guid = receiver;
	msg.subject = subject;
	msg.body = body;
	msg.money = money;
	msg.cod = cod;
	for(vector<uint64>::iterator itr = item_guids.begin(); itr != item_guids.end(); ++itr)
		msg.items.push_back(Arcemu::Util::GUID_LOPART(*itr));

	msg.stationery = stationery;
	msg.delivery_time = (uint32)UNIXTIME + deliverdelay;

	// 30 days expiration time for unread mail + possible delivery delay.
	if(!sMailSystem.MailOption(MAIL_FLAG_NO_EXPIRY))
		msg.expire_time = (uint32)UNIXTIME + deliverdelay + (TIME_DAY * MAIL_DEFAULT_EXPIRATION_TIME);
	else
		msg.expire_time = 0;

	msg.deleted_flag = false;
	msg.checked_flag = checked;

	// Send the message.
	DeliverMessage(receiver, &msg);
}

//overload to keep backward compatibility (passing just 1 item guid instead of a vector)
void MailSystem::SendAutomatedMessage(uint32 type, uint64 sender, uint64 receiver, string subject, string body, uint32 money,
                                      uint32 cod, uint64 item_guid, uint32 stationery, MailCheckMask checked, uint32 deliverdelay)
{
	vector<uint64> item_guids;
	if(item_guid != 0)
		item_guids.push_back(item_guid);
	SendAutomatedMessage(type, sender, receiver, subject, body, money, cod, item_guids, stationery, checked, deliverdelay);
}

void Mailbox::Load(QueryResult* result)
{
	if(!result)
		return;

	Field* fields;
	MailMessage msg;
	uint32 i;
	char* str;
	char* p;
	uint32 itemguid;

	do
	{
		fields = result->Fetch();

		// Create message struct
		i = 0;
		msg.items.clear();
		msg.message_id = fields[i++].GetUInt32();
		msg.message_type = fields[i++].GetUInt32();
		msg.player_guid = fields[i++].GetUInt32();
		msg.sender_guid = fields[i++].GetUInt32();
		msg.subject = fields[i++].GetString();
		msg.body = fields[i++].GetString();
		msg.money = fields[i++].GetUInt32();
		str = (char*)fields[i++].GetString();
		p = strchr(str, ',');
		if(p == NULL)
		{
			itemguid = atoi(str);
			if(itemguid != 0)
				msg.items.push_back(itemguid);
		}
		else
		{
			while(p)
			{
				*p = 0;
				p++;

				itemguid = atoi(str);
				if(itemguid != 0)
					msg.items.push_back(itemguid);

				str = p;
				p = strchr(str, ',');
			}
		}

		msg.cod = fields[i++].GetUInt32();
		msg.stationery = fields[i++].GetUInt32();
		msg.expire_time = fields[i++].GetUInt32();
		msg.delivery_time = fields[i++].GetUInt32();
		msg.checked_flag = fields[i++].GetUInt32();
		msg.deleted_flag = fields[i++].GetBool();

		// Add to the mailbox
		AddMessage(&msg);

	}
	while(result->NextRow());
}
