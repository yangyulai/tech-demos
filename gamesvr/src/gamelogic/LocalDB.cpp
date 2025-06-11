#include "LocalDB.h"
#include "../gamesvr.h"

std::shared_ptr<stEffectDataLoaderBase> stItemDataBase::GetEffectDataBase() const
{
	return m_effectDataBase.lock();
}

std::shared_ptr<stEffectDataLoaderBase> stBuffDataBase::GetEffectDataBase() const
{
	return m_effectDataBase.lock();
}

std::shared_ptr<stEffectDataLoaderBase> stMagicDataBase::GetEffectDataBase() const
{
	return m_effectDataBase.lock();	
}

std::shared_ptr<stItemDataBase> ItemBasePtr::GetItemDataBase() const
{
	return m_itemDataBase.lock();
}

std::shared_ptr<stItemDataBase> stDropItem::GetItemDataBase() const
{
	if (id == 0) return {};
	return pDropItemData.lock();
}

std::shared_ptr<stSubDropItemDataBase> stDropItem::GetSubDropData() const
{
	return m_subDropData.lock();
}