#include "resources.h"

/*
placeholder("Wood");placeholder("Coal");placeholder("Copper");placeholder("Iron");placeholder("Gold");placeholder("Diamond");placeholder("Enegry");
*/

const char *GetResourceName(int ID)
{
    switch (ID)
    {
    case RESOURCE_WOOD:
        return "Wood";

    case RESOURCE_COAL:
        return "Coal";

    case RESOURCE_COPPER:
        return "Copper";

    case RESOURCE_IRON:
        return "Iron";

    case RESOURCE_GOLD:
        return "Gold";

    case RESOURCE_DIAMOND:
        return "Diamond";

    case RESOURCE_ENEGRY:
        return "Enegry";

    default:
        return "None";
    }
    return "WTF";
}