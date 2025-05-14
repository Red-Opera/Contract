#include "QuestList.h"

bool UQuestList::AddQuest(const FQuestInfo& QuestInfo)
{
    // 이미 동일한 이름의 퀘스트가 있는지 확인
    if (HasQuest(QuestInfo.questName))
        return false;

    // 퀘스트 추가
    quests.Add(QuestInfo);

    return true;
}

bool UQuestList::HasQuest(const FString& QuestName) const
{
    return FindQuestIndex(QuestName) != -1;
}

int32 UQuestList::FindQuestIndex(const FString& QuestName) const
{
    for (int32 i = 0; i < quests.Num(); i++)
    {
        if (quests[i].questName == QuestName)
            return i;
    }

    // 퀘스트를 찾지 못함
    return -1;
}