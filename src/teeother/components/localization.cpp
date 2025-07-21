#include <engine/external/json-parser/json.h>

#include <engine/storage.h>

#include "localization.h"

CLocalization::CLanguage::CLanguage()
 : m_Loaded(false), m_Direction(CLocalization::DIRECTION_LTR)
{
	m_aName[0] = 0;
	m_aFilename[0] = 0;
	m_aParentFilename[0] = 0;
}

CLocalization::CLanguage::CLanguage(const char* pName, const char* pFilename, const char* pParentFilename)
 : m_Loaded(false), m_Direction(CLocalization::DIRECTION_LTR)
{
	str_copy(m_aName, pName, sizeof(m_aName));
	str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
	str_copy(m_aParentFilename, pParentFilename, sizeof(m_aParentFilename));
}

CLocalization::CLanguage::~CLanguage()
{
	hashtable< CEntry, 128 >::iterator Iter = m_Translations.begin();
	while(Iter != m_Translations.end())
	{
		if(Iter.data())
			Iter.data()->Free();

		++Iter;
	}
}

bool CLocalization::CLanguage::Load(IStorage* pStorage)
{
	// read file data into buffer
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "./server_lang/%s.json", m_aFilename);
	const IOHANDLE File = pStorage->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return false;

	const int FileSize = (int)io_length(File);
	char* pFileData = (char*)malloc(FileSize);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value* pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
	free(pFileData);

	if(pJsonData == nullptr)
	{
		dbg_msg("Localization", "Can't load the localization file %s : %s", aBuf, aError);
		return false;
	}

	dynamic_string Buffer;
	int Length;

	// extract data
	const json_value& rStart = (*pJsonData)["translation"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			const char* pKey = rStart[i]["key"];
			if(pKey && pKey[0])
			{
				CEntry* pEntry = m_Translations.set(pKey);

				const char* pSingular = rStart[i]["value"];
				if(pSingular && pSingular[0])
				{
					Length = str_length(pSingular) + 1;
					pEntry->m_apVersions = new char[Length];
					str_copy(pEntry->m_apVersions, pSingular, Length);
				}
			}
		}
	}

	// clean up
	json_value_free(pJsonData);
	m_Loaded = true;

	return true;
}

const char* CLocalization::CLanguage::Localize(const char* pText) const
{
	const CEntry* pEntry = m_Translations.get(pText);
	if(!pEntry)
		return nullptr;

	return pEntry->m_apVersions;
}

CLocalization::CLocalization(IStorage* pStorage) : m_pStorage(pStorage), m_pMainLanguage(nullptr)
{ }

CLocalization::~CLocalization()
{
	for(int i = 0; i < m_pLanguages.size(); i++)
		delete m_pLanguages[i];
}

bool CLocalization::InitConfig(int argc, const char** argv)
{
	m_CfgMainLanguage.copy("en");
	return true;
}

bool CLocalization::Init()
{
	// read file data into buffer
	const char* pFilename = "./server_lang/index.json";
	IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		dbg_msg("Localization", "can't open ./server_lang/index.json");
		return false;
	}

	int FileSize = (int)io_length(File);
	char *pFileData = new char[FileSize+1];
	io_read(File, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
	if(pJsonData == 0)
	{
		delete[] pFileData;
		return true; //return true because it's not a critical error
	}
	// extract data
	m_pMainLanguage = nullptr;
	const json_value& rStart = (*pJsonData)["language indices"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			CLanguage*& pLanguage = m_pLanguages.increment();
			pLanguage = new CLanguage((const char*)rStart[i]["name"], (const char*)rStart[i]["file"], (const char*)rStart[i]["parent"]);

			if(m_CfgMainLanguage == pLanguage->GetFilename())
			{
				pLanguage->Load(Storage());
				m_pMainLanguage = pLanguage;
			}
		}
	}

	// clean up
	json_value_free(pJsonData);
	delete[] pFileData;
	return true;
}

const char* CLocalization::LocalizeWithDepth(const char* pLanguageCode, const char* pText, int Depth)
{
	// found language
	CLanguage* pLanguage = m_pMainLanguage;
	if(pLanguageCode)
	{
		for(int i = 0; i < m_pLanguages.size(); i++)
		{
			if(str_comp(m_pLanguages[i]->GetFilename(), pLanguageCode) == 0)
			{
				pLanguage = m_pLanguages[i];
				break;
			}
		}
	}

	// if no language is found
	if(!pLanguage)
	{
		return pText;
	}

	// load and initilize language if is not loaded
	if(!pLanguage->IsLoaded())
	{
		pLanguage->Load(Storage());
	}

	// found result in hash map
	if(const char* pResult = pLanguage->Localize(pText))
	{
		return pResult;
	}

	// localize with depth
	if(pLanguage->GetParentFilename()[0] && Depth < 4)
	{
		return LocalizeWithDepth(pLanguage->GetParentFilename(), pText, Depth + 1);
	}

	// return default text
	return pText;
}

const char* CLocalization::Localize(const char* pLanguageCode, const char* pText)
{
	return LocalizeWithDepth(pLanguageCode, pText, 0);
}

const char *CLocalization::LanguageCodeByCountryCode(int CountryCode)
{
	// Constants from 'data/countryflags/index.txt'
	switch(CountryCode)
	{
		/* ar - Arabic ************************************/
		case 12: //Algeria
		case 48: //Bahrain
		case 262: //Djibouti
		case 818: //Egypt
		case 368: //Iraq
		case 400: //Jordan
		case 414: //Kuwait
		case 422: //Lebanon
		case 434: //Libya
		case 478: //Mauritania
		case 504: //Morocco
		case 512: //Oman
		case 275: //Palestine
		case 634: //Qatar
		case 682: //Saudi Arabia
		case 706: //Somalia
		case 729: //Sudan
		case 760: //Syria
		case 788: //Tunisia
		case 784: //United Arab Emirates
		case 887: //Yemen
			return "ar";
		/* bg - Bosnian *************************************/
		case 100: //Bulgaria
			return "bg";
		/* bs - Bosnian *************************************/
		case 70: //Bosnia and Hercegovina
			return "bs";
		/* cs - Czech *************************************/
		case 203: //Czechia
			return "cs";
		/* de - German ************************************/
		case 40: //Austria
		case 276: //Germany
		case 438: //Liechtenstein
		case 756: //Switzerland
			return "de";
		/* el - Greek ***********************************/
		case 300: //Greece
		case 196: //Cyprus
			return "el";
		/* es - Spanish ***********************************/
		case 32: //Argentina
		case 68: //Bolivia
		case 152: //Chile
		case 170: //Colombia
		case 188: //Costa Rica
		case 192: //Cuba
		case 214: //Dominican Republic
		case 218: //Ecuador
		case 222: //El Salvador
		case 226: //Equatorial Guinea
		case 320: //Guatemala
		case 340: //Honduras
		case 484: //Mexico
		case 558: //Nicaragua
		case 591: //Panama
		case 600: //Paraguay
		case 604: //Peru
		case 630: //Puerto Rico
		case 724: //Spain
		case 858: //Uruguay
		case 862: //Venezuela
			return "es";
		/* fa - Farsi ************************************/
		case 364: //Islamic Republic of Iran
		case 4: //Afghanistan
			return "fa";
		/* fr - French ************************************/
		case 204: //Benin
		case 854: //Burkina Faso
		case 178: //Republic of the Congo
		case 384: //Cote d’Ivoire
		case 266: //Gabon
		case 324: //Ginea
		case 466: //Mali
		case 562: //Niger
		case 686: //Senegal
		case 768: //Togo
		case 250: //France
		case 492: //Monaco
			return "fr";
		/* hr - Croatian **********************************/
		case 191: //Croatia
			return "hr";
		/* hu - Hungarian *********************************/
		case 348: //Hungary
			return "hu";
		/* it - Italian ***********************************/
		case 380: //Italy
			return "it";
		/* ja - Japanese **********************************/
		case 392: //Japan
			return "ja";
		/* la - Latin *************************************/
		case 336: //Vatican
			return "la";
		/* nl - Dutch *************************************/
		case 533: //Aruba
		case 531: //Curaçao
		case 534: //Sint Maarten
		case 528: //Netherland
		case 740: //Suriname
		case 56: //Belgique
			return "nl";
		/* pl - Polish *************************************/
		case 616: //Poland
			return "pl";
		/* pt - Portuguese ********************************/
		case 24: //Angola
		case 132: //Cape Verde
		//case 226: //Equatorial Guinea: official language, but not national language
		//case 446: //Macao: official language, but spoken by less than 1% of the population
		case 508: //Mozambique
		case 626: //Timor-Leste
		case 678: //São Tomé and Príncipe
			return "pt";
		/* pt-BR - Portuguese (Brazil) ********************************/
		case 76: //Brazil
			return "pt-BR";
		/* ru - Russian ***********************************/
		case 112: //Belarus
		case 643: //Russia
		case 398: //Kazakhstan
			return "ru";
		/* sk - Slovak ************************************/
		case 703: //Slovakia
			return "sk";
		/* sr - Serbian ************************************/
		case 688: //Serbia
			return "sr";
		/* tl - Tagalog ************************************/
		case 608: //Philippines
			return "tl";
		/* tr - Turkish ************************************/
		case 31: //Azerbaijan
		case 792: //Turkey
			return "tr";
		/* uk - Ukrainian **********************************/
		case 804: //Ukraine
			return "uk";
		/* zh-CN - Chinese (Simplified) **********************************/
		case 156: //People’s Republic of China
		case 344: //Hong Kong
		case 446: //Macau
			return "zh-cn";
		case 826: // United Kingdom of Great Britain and Northern Ireland
		case 840: // United States of America
			return "en";
		default:
			return "";
	}
}