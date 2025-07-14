#include "timer.h"
#include <algorithm>
#include <cassert>

CTimer::CTimer(int Interval, std::function<void()> Callback, 
               const std::string& Tag, bool IsRepeating)
	: m_Elapsed(0.0f), m_Interval(Interval), m_Active(false),
	  m_Repeating(IsRepeating), m_Tag(Tag), m_Callback(Callback)
{
}

void CTimer::Start()
{
	m_Elapsed = 0.0f;
	m_Active = true;
}

void CTimer::Stop()
{
	m_Active = false;
}

void CTimer::Reset()
{
	m_Elapsed = 0.0f;
}

void CTimer::Update()
{
	if(!m_Active) 
		return;
	
	m_Elapsed ++;
	if(m_Elapsed >= m_Interval)
	{
		if(m_Callback) 
			m_Callback();
		
		if(m_Repeating)
			m_Elapsed -= m_Interval;
		else
			m_Active = false;
	}
}

void CTimer::SetTag(const std::string& Tag)
{
	m_Tag = Tag;
}

CTimerManager::CTimerManager()
{
	m_Timers.reserve(64); // pre fen pei
}

CTimerManager::~CTimerManager()
{
	// Clear
	m_TagIndex.clear();
	m_Timers.clear();
}

CTimer *CTimerManager::CreateTimer(int Interval, std::function<void()> Callback, 
                                  const std::string& Tag, bool IsRepeating)
{
	// Reuse the timer tag by to be deleted
	auto it = std::find_if(m_Timers.begin(), m_Timers.end(),
		[](const TimerEntry& entry) { return entry.dirty; });
	
	if(it != m_Timers.end())
	{
		// Reuse
		it->pTimer = std::make_unique<CTimer>(Interval, Callback, Tag, IsRepeating);
		it->dirty = false;
		CTimer *pTimer = it->pTimer.get();
		
		// Update
		if(!Tag.empty())
			m_TagIndex[Tag].push_back(pTimer);
		
		return pTimer;
	}
	
	// Create new timer
	m_Timers.emplace_back();
	auto& entry = m_Timers.back();
	entry.pTimer = std::make_unique<CTimer>(Interval, Callback, Tag, IsRepeating);
	entry.dirty = false;
	CTimer *pTimer = entry.pTimer.get();
	
	// Add tag
	if(!Tag.empty())
		m_TagIndex[Tag].push_back(pTimer);
	
	return pTimer;
}

void CTimerManager::UpdateAll()
{
	for(auto& entry : m_Timers)
	{
		if(!entry.dirty && entry.pTimer->IsActive())
		{
			entry.pTimer->Update();
		}
	}

	if(m_NeedCleanup)
	{
		CleanUpDirtyTimers();
		m_NeedCleanup = false;
	}
}

void CTimerManager::CleanUpDirtyTimers()
{
	auto it = std::remove_if(m_Timers.begin(), m_Timers.end(),
		[](const TimerEntry& entry) { return entry.dirty; });
	
	for(auto delIt = it; delIt != m_Timers.end(); ++delIt)
		RemoveFromIndex(delIt->pTimer.get());
	
	m_Timers.erase(it, m_Timers.end());
}

void CTimerManager::StopByTag(const std::string& Tag)
{
	auto it = m_TagIndex.find(Tag);
	if(it != m_TagIndex.end())
	{
		for(CTimer *pTimer : it->second)
		{
			pTimer->Stop();
		}
	}
}

void CTimerManager::StartByTag(const std::string& Tag)
{
	auto it = m_TagIndex.find(Tag);
	if(it != m_TagIndex.end())
	{
		for(CTimer *pTimer : it->second)
		{
			pTimer->Start();
		}
	}
}

void CTimerManager::ResetByTag(const std::string& Tag)
{
	auto it = m_TagIndex.find(Tag);
	if(it != m_TagIndex.end())
	{
		for(CTimer *pTimer : it->second)
		{
			pTimer->Reset();
		}
	}
}

void CTimerManager::RemoveByTag(const std::string& Tag)
{
	StopByTag(Tag);
	
	auto it = m_TagIndex.find(Tag);
	if(it == m_TagIndex.end()) 
		return;

	for(CTimer *pTimer : it->second)
	{
		auto timerIt = std::find_if(m_Timers.begin(), m_Timers.end(),
			[pTimer](const TimerEntry& entry) { 
				return !entry.dirty && entry.pTimer.get() == pTimer; 
			});
		
		if(timerIt != m_Timers.end())
		{
			timerIt->dirty = true;
			m_NeedCleanup = true;
		}
	}
	
	m_TagIndex.erase(it);
}

bool CTimerManager::IsTagActive(const std::string& Tag) const
{
	auto it = m_TagIndex.find(Tag);
	if(it != m_TagIndex.end())
	{
		for(CTimer *pTimer : it->second)
		{
			if(pTimer->IsActive())
				return true;
		}
	}
	return false;
}

size_t CTimerManager::CountByTag(const std::string& Tag) const
{
	auto it = m_TagIndex.find(Tag);
	return (it != m_TagIndex.end()) ? it->second.size() : 0;
}

void CTimerManager::RemoveFromIndex(CTimer *pTimer)
{
	const std::string& Tag = pTimer->GetTag();
	if(!Tag.empty())
	{
		auto it = m_TagIndex.find(Tag);
		if(it != m_TagIndex.end())
		{
			auto& Timers = it->second;
			Timers.erase(std::remove(Timers.begin(), Timers.end(), pTimer), Timers.end());
			
			if(Timers.empty())
				m_TagIndex.erase(it);
		}
	}
}