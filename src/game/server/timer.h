#ifndef GAME_TIMER_H
#define GAME_TIMER_H

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// 计时器类
class CTimer
{
public:
	CTimer(int Interval, std::function<void()> Callback, 
          const std::string& Tag = "", bool IsRepeating = false);
	
	void Start();
	void Stop();
	void Reset();
	void Update();
	
	bool IsActive() const { return m_Active; }
	const std::string& GetTag() const { return m_Tag; }
	void SetTag(const std::string& Tag);
	
private:
	int m_Elapsed;
	int m_Interval;
	bool m_Active;
	bool m_Repeating;
	std::string m_Tag;
	std::function<void()> m_Callback;
};

// 计时器管理器
class CTimerManager
{
public:
	CTimerManager();
	~CTimerManager();
	
	CTimer* CreateTimer(int Interval, std::function<void()> Callback, 
                       const std::string& Tag = "", bool IsRepeating = false);
	
	void UpdateAll();
	void StopByTag(const std::string& Tag);
	void StartByTag(const std::string& Tag);
	void ResetByTag(const std::string& Tag);
	void RemoveByTag(const std::string& Tag);
	
	bool IsTagActive(const std::string& Tag) const;
	size_t CountByTag(const std::string& Tag) const;

private:
	struct TimerEntry {
		std::unique_ptr<CTimer> pTimer;
		bool dirty = false; // 标记是否待删除
	};
	
	void CleanUpDirtyTimers();
	void RemoveFromIndex(CTimer* pTimer);
	
	std::vector<TimerEntry> m_Timers;
	std::unordered_map<std::string, std::vector<CTimer*>> m_TagIndex;
	bool m_NeedCleanup = false;
};

#endif // GAME_TIMER_H