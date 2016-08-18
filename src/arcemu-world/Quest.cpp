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

//Packet Building
/////////////////

WorldPacket* WorldSession::BuildQuestQueryResponse(Quest* qst)
{
	WorldPacket* data = new WorldPacket(SMSG_QUEST_QUERY_RESPONSE, 100);
	LocalizedQuest* lci = (language > 0) ? sLocalizationMgr.GetLocalizedQuest(qst->id, language) : NULL;

	*data << uint32(qst->id);
    data->WriteBit(1); // has data
    data->WriteBits(strlen(qst->incompletetext) + 1, 10);
    data->WriteBits(strlen(qst->title) + 1, 9);
    data->WriteBits(strlen(qst->completiontext) + 1, 11);
    data->WriteBits(strlen(qst->details) + 1, 12);

    /////////
	//data->WriteBits(questTurnTargetName.size(), 8);
	//data->WriteBits(qst->questGiverTargetName.size(), 8);
	//data->WriteBits(questGiverTextWindow.size(), 10);
	data->WriteBits(strlen(qst->endtext) + 1, 9);
	//data->WriteBits(quest->m_questObjectives.size(), 19);
	//data->WriteBits(questObjectives.size(), 12);

	ByteBuffer objData;
	/*for (QuestObjectiveSet::const_iterator citr = quest->m_questObjectives.begin(); citr != quest->m_questObjectives.end(); citr++)
	{
		QuestObjective const* questObjective = *citr;

		std::string descriptionText = qst->objectives;
		if (locale > 0)
			if (QuestObjectiveLocale const* questObjectiveLocale = sObjectMgr->GetQuestObjectiveLocale(questObjective->Id))
				ObjectMgr::GetLocaleString(questObjectiveLocale->Description, locale, descriptionText);

		data->WriteBits(descriptionText.size(), 8);
		data->WriteBits(questObjective->VisualEffects.size(), 22);

		objData << int32(questObjective->Amount);
		objData << uint32(questObjective->Id);
		objData.WriteString(descriptionText);
		objData << uint32(questObjective->Flags);
		objData << uint8(questObjective->Index);
		objData << uint8(questObjective->Type);
		objData << uint32(questObjective->ObjectId);

		for (VisualEffectVec::const_iterator citrEffects = questObjective->VisualEffects.begin(); citrEffects != questObjective->VisualEffects.end(); citrEffects++)
			objData << uint32(*citrEffects);
	}*/

	data->FlushBits();

	bool hiddenReward = qst->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS);

	// values need rechecking and zero values need more research
	data->append(objData);
	*data << uint32(qst->required_item[0]);
	*data << uint32(hiddenReward ? 0 : qst->reward_choiceitem[4]);
	*data << uint32(hiddenReward ? 0 : qst->reward_choiceitem[3]);
	*data << uint32(hiddenReward ? 0 : qst->reward_choiceitemcount[1]);
	*data << uint32(hiddenReward ? 0 : qst->reward_choiceitem[2]);

	for (uint32 i = 0; i < 4; i++)
	{
		*data << qst->reward_item[i];				// Forced Reward Item [i]
		*data << qst->reward_itemcount[i];			// Forced Reward Item Count [i]
	}

	*data << uint32(qst->rewardtalents);                               // bonus talents
	*data << float(qst->point_y);
	*data << uint32(0); // quest->GetSoundTurnIn());

	for (int i = 0; i < 5; i++)
	{
		*data << uint32(qst->reward_repfaction[i]);                        // reward factions ids
	}

	//*data << uint32(hiddenReward ? 0 : quest->GetRewMoney());                // reward money (below max lvl)
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemCount[4]);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemCount[1]);
	//*data << uint32(qst-> quest->GetFlags2());
	data->WriteString(qst->endtext);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemId[1]);
	//*data << uint32(quest->GetRewMoneyMaxLevel());                           // used in XP calculation at client
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemId[0]);
	//data->WriteString(questCompletedText);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemId[3]);
	//*data << uint32(quest->GetRewHonorAddition());                           // rewarded honor points
	//data->WriteString(questGiverTextWindow);
	//data->WriteString(questObjectives);
	//*data << uint32(quest->GetRewardSkillPoints());                          // reward skill points
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemId[5]);
	//*data << uint32(quest->GetSuggestedPlayers());                           // suggested players count
	*data << uint32(qst->id);                                    // quest id
	//*data << uint32(quest->RequiredSourceItemId[1]);
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemId[1]);
	*data << int32(qst->min_level);                                    // min level
	//*data << uint32(quest->GetRewardReputationMask());                       // rep mask (unsure on what it does)
	//*data << uint32(quest->GetPointOpt());
	*data << int32(qst->questlevel);                                  // may be -1, static data, in other cases must be used dynamic level: Player::GetQuestLevel (0 is not known, but assuming this is no longer valid for quest intended for client)
	*data << uint32(2); // quest->GetQuestMethod());                                // Accepted values: 0, 1 or 2. 0 == IsAutoComplete() (skip objectives/details)
	//*data << uint32(qst->srcitemcount[2]);
	//*data << uint32(qst->->GetXPId());                                       // seems to always have the same value as the first XP ID field
	data->WriteString(qst->details);
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemIdCount[0]);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemCount[5]);
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemIdCount[2]);
	*data << uint32(qst->effect_on_player);                               // casted spell
	*data << uint32(0);                                                      // unknown
	//data->WriteString(questTurnTargetName);
	*data << uint32(0);                                                      // unknown
	//*data << uint32(quest->RequiredSourceItemCount[1]);
	//*data << uint32(quest->RequiredSourceItemId[2]);
	//*data << uint32(quest->GetQuestTurnInPortrait());
	data->WriteString(qst->title);
	//*data << uint32(quest->GetType());                                       // quest type
	*data << uint32(0); // quest->GetXPId());                                       // used for calculating rewarded experience
	*data << uint32(0);                                                      // unknown
	*data << uint32(0);                                                      // unknown
	*data << uint32(qst->point_mapid);
	//*data << uint32(quest->GetNextQuestInChain());                           // client will request this quest from NPC, if not 0
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemId[0]);
	//data->WriteString(questGiverTargetName);
	*data << uint32(0);                                                      // unknown
	//*data << uint32(quest->RequiredSourceItemId[3]);
	*data << float(qst->point_x);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemId[2]);
	*data << uint32(0);                                                      // unknown
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemIdCount[3]);
	*data << uint32(0);// quest->GetSoundAccept());
	//*data << uint32(hiddenReward ? 0 : quest->RewardItemId[2]);
	*data << float(0); // quest->GetRewHonorMultiplier());
	//*data << uint32(quest->GetCharTitleId());                                // CharTitleId, new 2.4.0, player gets this title (id from CharTitles)
	//data->WriteString(questTurnTextWindow);
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemCount[3]);
	//*data << uint32(quest->RequiredSourceItemCount[0]);
	//*data << int32(quest->GetZoneOrSort());                                  // zone or sort to display in quest log
	//*data << uint32(quest->GetRewardSkillId());                              // reward skill id
	//*data << uint32(hiddenReward ? 0 : quest->RewardChoiceItemCount[0]);
	//*data << uint32(quest->GetRewSpell());                                   // reward spell, this spell will display (icon) (casted if RewSpellCast == 0)
	//*data << uint32(quest->GetQuestGiverPortrait());
	*data << uint32(0);                                                      // unknown
	//*data << uint32(quest->RequiredSourceItemCount[3]);
	*data << uint32(qst->quest_flags & 0xFFFF);                             // quest flags
	//*data << uint32(quest->GetRewardPackageItemId());
	//*data << uint32(quest->GetSrcItemId()); 

	return data;
}


/*****************
* QuestLogEntry *
*****************/
QuestLogEntry::QuestLogEntry()
{
	mInitialized = false;
	m_quest = NULL;
	mDirty = false;
	m_slot = -1;
	completed = 0;
}

QuestLogEntry::~QuestLogEntry()
{

}

void QuestLogEntry::Init(Quest* quest, Player* plr, uint32 slot)
{
	ARCEMU_ASSERT(quest != NULL);
	ARCEMU_ASSERT(plr != NULL);

	m_quest = quest;
	m_plr = plr;
	m_slot = slot;

	iscastquest = false;
	isemotequest = false;
	for(uint32 i = 0; i < 4; ++i)
	{
		if(quest->required_spell[i] != 0)
		{
			iscastquest = true;
			if(!plr->HasQuestSpell(quest->required_spell[i]))
				plr->quest_spells.insert(quest->required_spell[i]);
		}
		else if(quest->required_emote[i] != 0)
		{
			isemotequest = true;
		}
		if(quest->required_mob[i] != 0)
		{
			if(!plr->HasQuestMob(quest->required_mob[i]))
				plr->quest_mobs.insert(quest->required_mob[i]);
		}
	}

	// update slot
	plr->SetQuestLogSlot(this, slot);

	mDirty = true;

	memset(m_mobcount, 0, 4 * 4);
	memset(m_explored_areas, 0, 4 * 4);

	if( m_quest->time > 0 )
		expirytime = UNIXTIME + m_quest->time / 1000;
	else
		expirytime = 0;

	if(!plr->GetSession()->m_loggingInPlayer)  //quest script should not be called on login
		CALL_QUESTSCRIPT_EVENT(this, OnQuestStart)(plr, this);
}

void QuestLogEntry::ClearAffectedUnits()
{
	if(m_affected_units.size() > 0)
		m_affected_units.clear();
}
void QuestLogEntry::AddAffectedUnit(Unit* target)
{
	if(!target)
		return;
	if(!IsUnitAffected(target))
		m_affected_units.insert(target->GetGUID());
}
bool QuestLogEntry::IsUnitAffected(Unit* target)
{
	if(!target)
		return true;
	if(m_affected_units.find(target->GetGUID()) != m_affected_units.end())
		return true;
	return false;
}

void QuestLogEntry::SaveToDB(QueryBuffer* buf)
{
	ARCEMU_ASSERT(m_slot != -1);
	if(!mDirty)
		return;

	std::stringstream ss;

	ss << "DELETE FROM questlog WHERE player_guid = ";
	ss << m_plr->GetLowGUID();
	ss << " AND quest_id = ";
	ss << m_quest->id;
	ss << ";";

	if(buf == NULL)
		CharacterDatabase.Execute(ss.str().c_str());
	else
		buf->AddQueryStr(ss.str());

	ss.rdbuf()->str("");

	ss << "INSERT INTO questlog VALUES(";
	ss << m_plr->GetLowGUID() << "," << m_quest->id << "," << m_slot << "," << expirytime;
	for(int i = 0; i < 4; ++i)
		ss << "," << m_explored_areas[i];

	for(int i = 0; i < 4; ++i)
		ss << "," << m_mobcount[i];

	ss << "," << uint32(completed);

	ss << ")";

	if(buf == NULL)
		CharacterDatabase.Execute(ss.str().c_str());
	else
		buf->AddQueryStr(ss.str());
}

bool QuestLogEntry::LoadFromDB(Field* fields)
{
	// playerguid,questid,timeleft,area0,area1,area2,area3,kill0,kill1,kill2,kill3
	int f = 3;
	ARCEMU_ASSERT(m_plr && m_quest);
	expirytime = fields[f].GetUInt32();
	f++;
	for(int i = 0; i < 4; ++i)
	{
		m_explored_areas[i] = fields[f].GetUInt32();
		f++;
		CALL_QUESTSCRIPT_EVENT(this, OnExploreArea)(m_explored_areas[i], m_plr, this);
	}

	for(int i = 0; i < 4; ++i)
	{
		m_mobcount[i] = fields[f].GetUInt32();
		f++;
		if(GetQuest()->required_mobtype[i] == QUEST_MOB_TYPE_CREATURE)
		{
			CALL_QUESTSCRIPT_EVENT(this, OnCreatureKill)(GetQuest()->required_mob[i], m_plr, this);
		}
		else
		{
			CALL_QUESTSCRIPT_EVENT(this, OnGameObjectActivate)(GetQuest()->required_mob[i], m_plr, this);
		}
	}

	completed = fields[f].GetUInt32();

	mDirty = false;
	return true;
}

bool QuestLogEntry::CanBeFinished()
{
	uint32 i;

	if(m_quest->iscompletedbyspelleffect && ( completed == QUEST_INCOMPLETE ) )
		return false;

	if( completed == QUEST_FAILED )
		return false;
	else
	if( completed == QUEST_COMPLETE )
		return true;

	for(i = 0; i < 4; ++i)
	{
		if(m_quest->required_mob[i])
		{
			if(m_mobcount[i] < m_quest->required_mobcount[i])
			{
				return false;
			}
		}
		if(m_quest->required_spell[i])   // requires spell cast, with no required target
		{
			if(m_mobcount[i] == 0 || m_mobcount[i] < m_quest->required_mobcount[i])
			{
				return false;
			}
		}
		if(m_quest->required_emote[i])   // requires emote, with no required target
		{
			if(m_mobcount[i] == 0 || m_mobcount[i] < m_quest->required_mobcount[i])
			{
				return false;
			}
		}
	}

	for(i = 0; i < MAX_REQUIRED_QUEST_ITEM; ++i)
	{
		if(m_quest->required_item[i])
		{
			if(m_plr->GetItemInterface()->GetItemCount(m_quest->required_item[i]) < m_quest->required_itemcount[i])
			{
				return false;
			}
		}
	}

	//Check for Gold & AreaTrigger Requirements
	if(m_quest->reward_money < 0 && m_plr->GetGold() < uint32(-m_quest->reward_money))
		return false;

	for(i = 0; i < 4; ++i)
	{
		if(m_quest->required_triggers[i])
		{
			if(m_explored_areas[i] == 0)
				return false;
		}
	}

	return true;
}

void QuestLogEntry::SetMobCount(uint32 i, uint32 count)
{
	ARCEMU_ASSERT(i < 4);
	m_mobcount[i] = count;
	mDirty = true;
}

void QuestLogEntry::IncrementMobCount(uint32 i)
{
	ARCEMU_ASSERT(i < 4);
	++m_mobcount[i];
	mDirty = true;
}

void QuestLogEntry::SetTrigger(uint32 i)
{
	ARCEMU_ASSERT(i < 4);
	m_explored_areas[i] = 1;
	mDirty = true;
}

void QuestLogEntry::SetSlot(int32 i)
{
	ARCEMU_ASSERT(i != -1);
	m_slot = i;
}

void QuestLogEntry::Finish()
{
	sEventMgr.RemoveEvents( m_plr, EVENT_TIMED_QUEST_EXPIRE );

	uint32 base = GetBaseField(m_slot);
	m_plr->SetUInt32Value(base + 0, 0);
	m_plr->SetUInt32Value(base + 1, 0);
	m_plr->SetUInt64Value(base + 2, 0);
	m_plr->SetUInt32Value(base + 4, 0);

	// clear from player log
	m_plr->SetQuestLogSlot(NULL, m_slot);
	m_plr->PushToRemovedQuests(m_quest->id);
	m_plr->UpdateNearbyGameObjects();
	// delete ourselves

	delete this;
}

void QuestLogEntry::Fail( bool timerexpired ){
	sEventMgr.RemoveEvents( m_plr, EVENT_TIMED_QUEST_EXPIRE );

	completed = QUEST_FAILED;
	expirytime = 0;
	mDirty = true;
	
	uint32 base = GetBaseField( m_slot );
	m_plr->SetUInt32Value( base + 1, 2 );

	if( timerexpired )
		sQuestMgr.SendQuestUpdateFailedTimer( m_quest, m_plr );
	else
		sQuestMgr.SendQuestUpdateFailed( m_quest, m_plr );
}

void QuestLogEntry::UpdatePlayerFields()
{
	if(!m_plr)
		return;

	uint32 base = GetBaseField(m_slot);
	m_plr->SetUInt32Value(base + 0, m_quest->id);
	uint32 field0 = 0; // 0x01000000 = "Objective Complete" - 0x02 = Quest Failed - 0x04 = Quest Accepted

	// next field is count (kills, etc)
	uint64 field1 = 0;

	// explored areas
	if(m_quest->count_requiredtriggers)
	{
		uint32 count = 0;
		for(int i = 0; i < 4; ++i)
		{
			if(m_quest->required_triggers[i])
			{
				if(m_explored_areas[i] == 1)
				{
					count++;
				}
			}
		}

		if(count == m_quest->count_requiredtriggers)
		{
			field1 |= 0x01000000;
		}
	}

	// spell casts / emotes
	if(iscastquest)
	{
		bool cast_complete = true;
		for(int i = 0; i < 4; ++i)
		{
			if(m_quest->required_spell[i] && m_quest->required_mobcount[i] > m_mobcount[i])
			{
				cast_complete = false;
				break;
			}
		}
		if(cast_complete)
		{
			field0 |= 0x01000000; // "Objective Complete"
		}
	}
	else if(isemotequest)
	{
		bool emote_complete = true;
		for(int i = 0; i < 4; ++i)
		{
			if(m_quest->required_emote[i] && m_quest->required_mobcount[i] > m_mobcount[i])
			{
				emote_complete = false;
				break;
			}
		}
		if(emote_complete)
		{
			field0 |= 0x01000000; // "Objective Complete"
		}
	}

	// mob hunting / counter
	if(m_quest->count_required_mob)
	{
		/*uint8 cnt;
		for(int i = 0; i < 4; ++i)
		{
			if(m_quest->required_mob[i] && m_mobcount[i] > 0)
			{
				// 1 << (offset * 6)
				cnt = m_mobcount[i];
				field1 |= (cnt << (i*8));
			}
		}*/

		// optimized this - burlex
		uint8* p = (uint8*)&field1;
		for(int i = 0; i < 4; ++i)
		{
			if(m_quest->required_mob[i] && m_mobcount[i] > 0)
				p[2 * i] |= (uint8)m_mobcount[i];
		}
	}

	if( ( m_quest->time != 0 ) && ( expirytime < UNIXTIME ) )
		completed = QUEST_FAILED;

	if( completed == QUEST_FAILED )
		field0 |= 2;

	m_plr->SetUInt32Value(base + 1, field0);
	m_plr->SetUInt64Value(base + 2, field1);

	if( ( m_quest->time != 0 ) && ( completed != QUEST_FAILED ) ){
		m_plr->SetUInt32Value( base + 4, expirytime );
		sEventMgr.AddEvent( m_plr, &Player::EventTimedQuestExpire, m_quest->id, EVENT_TIMED_QUEST_EXPIRE, ( expirytime - UNIXTIME ) * 1000, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT );
	}else
		m_plr->SetUInt32Value( base + 4, 0 );
}

void QuestLogEntry::SendQuestComplete()
{
	WorldPacket data(SMSG_QUESTUPDATE_COMPLETE, 4);
	data << uint32(m_quest->id);
	m_plr->GetSession()->SendPacket(&data);
	m_plr->UpdateNearbyGameObjects();
	CALL_QUESTSCRIPT_EVENT(this, OnQuestComplete)(m_plr, this);
}

void QuestLogEntry::SendUpdateAddKill(uint32 i)
{
	sQuestMgr.SendQuestUpdateAddKill(m_plr, m_quest->id, m_quest->required_mob[i], m_mobcount[i], m_quest->required_mobcount[i], 0);
}

void QuestLogEntry::Complete()
{
	completed = QUEST_COMPLETE;
}
