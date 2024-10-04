/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

class IKernel;
class IMap;

struct CMapItemGroup;
struct CMapItemLayer;
struct CMapItemLayerTilemap;

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup* m_pGameGroup;
	CMapItemLayerTilemap* m_pGameLayer;
	IMap* m_pMap;

	void InitTilemapSkip();

public:
	CLayers();
	void Init(IKernel* pKernel, int ID = 0);
	void InitBackground(IMap* pMap);
	int NumGroups() const { return m_GroupsNum; }
	int NumLayers() const { return m_LayersNum; }
	IMap* Map() const { return m_pMap; }
	CMapItemGroup* GameGroup() const { return m_pGameGroup; }
	CMapItemLayerTilemap* GameLayer() const { return m_pGameLayer; }
	CMapItemGroup* GetGroup(int Index) const;
	CMapItemLayer* GetLayer(int Index) const;
};

#endif