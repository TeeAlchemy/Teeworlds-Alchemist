#ifndef TEEOTHER_COMPONENTS_LOCALIZATION_H
#define TEEOTHER_COMPONENTS_LOCALIZATION_H

#include <base/big_int.h>
#include <teeother/tl/hashtable.h>
#include <deque>

class CLocalization
{
	class IStorage* m_pStorage;
	IStorage* Storage() const { return m_pStorage; }

public:
	class CLanguage
	{
	protected:
		char m_aName[64];
		char m_aFilename[64];
		char m_aParentFilename[64];
		bool m_Loaded;
		int m_Direction;

		class CEntry
		{
		public:
			char* m_apVersions;

			CEntry() : m_apVersions(nullptr) {}

			void Free()
			{
				if (m_apVersions)
				{
					delete[] m_apVersions;
					m_apVersions = nullptr;
				}
			}
		};

		hashtable< CEntry, 128 > m_Translations;

	public:
		CLanguage();
		CLanguage(const char* pName, const char* pFilename, const char* pParentFilename);
		~CLanguage();

		const char* GetParentFilename() const { return m_aParentFilename; }
		const char* GetFilename() const { return m_aFilename; }
		const char* GetName() const { return m_aName; }
		bool IsLoaded() const { return m_Loaded; }
		bool Load(IStorage* pStorage);
		const char* Localize(const char* pKey) const;
	};

	enum
	{
		DIRECTION_LTR=0,
		DIRECTION_RTL,
		NUM_DIRECTIONS,
	};

protected:
	CLanguage* m_pMainLanguage;

public:
	array<CLanguage*> m_pLanguages;
	fixed_string128 m_CfgMainLanguage;

protected:
	const char* LocalizeWithDepth(const char* pLanguageCode, const char* pText, int Depth);

public:
	CLocalization(IStorage* pStorage);
	virtual ~CLocalization();

	virtual bool InitConfig(int argc, const char** argv);
	virtual bool Init();

private:
	// reformat types to string
	template <typename T>
	std::string ToString(const char* pLanguageCode, T Value)
	{
		if constexpr(std::is_same_v<T, double> || std::is_same_v<T, float>)
		{
			return std::to_string(Value);
		}
		else if constexpr(std::is_arithmetic_v<T>)
		{
			return get_commas(std::to_string(Value));
		}
		else if constexpr(std::is_same_v<T, BigInt>)
		{
			return get_label(Value.to_string());
		}
		else if constexpr(std::is_convertible_v<T, std::string>)
		{
			std::string localizeFmt(Localize(pLanguageCode, std::string(Value).c_str()));
			return localizeFmt;
		}
		else
		{
			static_assert(!std::is_same_v<T, T>, "One of the passed arguments cannot be converted to a string");
		}

		return "error convertable";
	}

	// end unpacking args function
	inline static std::string FormatImpl(const char*, const char* pText, std::deque<std::string>& vStrPack)
	{
		std::string Result{};
		bool ArgStarted = false;
		int TextLength = str_length(pText);

		// found args
		for(int i = 0; i <= TextLength; ++i)
		{
			bool IsLast = (i == TextLength);
			char IterChar = pText[i];

			// start arg iterate
			if(IterChar == '{')
			{
				ArgStarted = true;
				continue;
			}

			// arg started
			if(ArgStarted)
			{
				if(IterChar == '}')
				{
					ArgStarted = false;
					if(!vStrPack.empty())
					{
						Result += vStrPack.front();
						vStrPack.pop_front();
					}
				}
				continue;
			}

			// add next char
			if(!IsLast)
			{
				Result += IterChar;
			}
		}

		// result string
		return Result;
	}

	// unpacking ellipsis args pack
	template<typename T, typename ... Ts>
	std::string FormatImpl(const char* pLanguageCode, const char* pText, std::deque<std::string>& vStrPack, T&& arg, Ts&& ... argsfmt)
	{
		vStrPack.push_back(ToString(pLanguageCode, std::forward<T>(arg)));
		return FormatImpl(pLanguageCode, pText, vStrPack, std::forward<Ts>(argsfmt) ...);
	}

public:
	// format without args pack
	std::string Format(const char* pLanguageCode, const char* pText)
	{
		return std::string(Localize(pLanguageCode, pText));
	}

	// format args pack
	template<typename ... Ts>
	std::string Format(const char* pLanguageCode, const char* pText, Ts&& ... argsfmt)
	{
		std::deque<std::string> vStrPack;
		return FormatImpl(pLanguageCode, Localize(pLanguageCode, pText), vStrPack, std::forward<Ts>(argsfmt) ...);
	}

	// format with dynamic_string
	template<typename ... Ts>
	void Format(dynamic_string& Buffer, const char* pLanguageCode, const char* pText, Ts&& ... argsfmt)
	{
		std::string fmtStr = Format(pLanguageCode, pText, std::forward<Ts>(argsfmt) ...);
		Buffer.append(fmtStr.c_str());
	}

	// Localize
	const char* Localize(const char* pLanguageCode, const char* pText);
};

#endif