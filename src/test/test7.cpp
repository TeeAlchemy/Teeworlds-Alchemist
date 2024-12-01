#include <base/system.h>

char *str_find_before(const char *haystack, const char *needle)
{
    const char *match = str_find(haystack, needle);
    if (match)
    {
        size_t len = match - haystack + 1;
        char *result = (char *)malloc(len + 1); // +1 for the null terminator
        if (result)
        {
            str_copy(result, haystack, len);
            result[len] = '\0'; // Ensure null-termination
            return result;
        }
    }
    return NULL;
}

char *str_find_before_nth(const char *haystack, const char *needle, int n)
{
    if (n == 0)
    {
        // Special case for the 0th occurrence, return everything before the first needle.
        return str_find_before(haystack, needle);
    }
    n++;
    int count = 0;
    const char *start = haystack;
    const char *end = NULL;

    while ((end = str_find(start, needle)) != NULL)
    {
        if (count == n - 1)
        {
            size_t len = end - start + 1;
            char *result = (char *)malloc(len + 1); // +1 for the null terminator
            if (result)
            {
                str_copy(result, start, len);
                result[len] = '\0'; // Ensure null-termination
                return result;
            }
            break;
        }
        start = end + str_length(needle);
        count++;
    }

    return NULL; // If the nth needle is not found, return NULL.
}

int main()
{
    dbg_logger_stdout();

    int Size = str_count("hello|baba|good", "|") + 1; // +1 to include the first substring
    dbg_msg("test", "%d", Size);

    for (int i = 0; i < Size; i++)
    {
        char *pStr = str_find_before_nth("hello|baba|good", "|", i);
        if (pStr)
        {
            dbg_msg("sadad", "%d %s", pStr);
            free(pStr); // Free the allocated memory
        }
        else
        {
            dbg_msg("sadad", "Not found");
        }
    }

    return 0;
}
