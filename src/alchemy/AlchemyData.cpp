#include <engine/storage.h>

#include "Alchemy.h"

CAlchemyData::CAlchemyData() {}
CAlchemyData::~CAlchemyData() {}

json_value *CAlchemyData::LoadJson(IStorage *pStorage, const char *pFileName)
{
    const IOHANDLE File = pStorage->OpenFile(pFileName, IOFLAG_READ, IStorage::TYPE_ALL);
    dbg_assert(!!File, "Json file not found.");

    const int FileSize = (int)io_length(File);
    char *pFileData = (char *)malloc(FileSize);

    io_read(File, pFileData, FileSize);
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));

    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    free(pFileData);

    dbg_assert(!!pJsonData, "Json data not found.");

    return pJsonData;
}