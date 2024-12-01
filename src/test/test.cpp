#include <base/system.h>

int str_split(const char *str, char c, char ***res)
{
    int count = 0, idx = 0, len;
    const char *p = str, *start = str;
    char **result = NULL;

    if (!str || !res || !p)
        return -1; // Check for null pointers

    while (*p)
    {
        if (*p == c)
        {
            if (start != p)
                count++;
            start = p + 1;
        }
        p++;
    }
    if (start != p)
        count++; // Count last token

    result = (char **)malloc((count + 1) * sizeof(char *));
    if (!result)
        return -1; // Check for malloc failure

    p = str;
    for (idx = 0; idx < count; idx++)
    {
        start = p;
        while (*p && *p != c)
            p++;
        len = p - start;
        result[idx] = (char *)malloc(len + 1);
        if (!result[idx])
        { // Check for malloc failure
            while (--idx >= 0)
                free(result[idx]); // Free previously allocated memory
            free(result);
            return -1;
        }
        mem_copy(result[idx], start, len);
        result[idx][len] = '\0';
        if (*p)
            p++; // Skip the delimiter
    }
    result[idx] = NULL; // Null-terminate the array

    *res = result;
    return count;
}

int main()
{
    dbg_logger_stdout();

    char **pResult;
    int Size = str_split("test|sa|wow|yes|good", '|', &pResult);
    for (int i = 0; i < Size; i++)
    {
        dbg_msg("test", "Part %d: %s", i, pResult[i]);
        free(pResult[i]);
    }

    return 0;
}
