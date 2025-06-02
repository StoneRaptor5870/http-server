#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple JSON parser for user data
int parse_user_json(const char *json, char *name, int name_len, char *email, int email_len,
                    char *password, int password_len)
{
    if (!json || !name || !email || !password)
        return -1;

    // Initialise outputs
    name[0] = '\0';
    email[0] = '\0';
    password[0] = '\0';

    // Find name field
    const char *name_start = strstr(json, "\"name\"");
    if (name_start)
    {
        name_start = strchr(name_start, ':');
        if (name_start)
        {
            name_start++;
            // Skip whitespace and opening quote
            while (*name_start && (isspace(*name_start) || *name_start == '"'))
            {
                name_start++;
            }

            // Copy until closing quote or end
            int i = 0;
            while (*name_start && *name_start != '"' && i < name_len - 1)
            {
                name[i++] = *name_start++;
            }

            name[i] = '\0';
        }
    }

    // Find email field
    const char *email_start = strstr(json, "\"email\"");
    if (email_start)
    {
        email_start = strchr(email_start, ':');
        if (email_start)
        {
            email_start++;
            // Skip whitespace and opening quote
            while (*email_start && (isspace(*email_start) || *email_start == '"'))
            {
                email_start++;
            }

            // Copy until closing quote or end
            int i = 0;
            while (*email_start && *email_start != '"' && i < email_len - 1)
            {
                email[i++] = *email_start++;
            }
            email[i] = '\0';
        }
    }

    // Find email field
    const char *password_start = strstr(json, "\"password\"");
    if (password_start)
    {
        password_start = strchr(password_start, ':');
        if (password_start)
        {
            password_start++;
            // Skip whitespace and opening quote
            while (*password_start && (isspace(*password_start) || *password_start == '"'))
            {
                password_start++;
            }

            // Copy until closing quote or end
            int i = 0;
            while (*password_start && *password_start != '"' && i < password_len - 1)
            {
                password[i++] = *password_start++;
            }
            password[i] = '\0';
        }
    }

    // Validate that we got both fields
    if (strlen(name) == 0 || strlen(email) == 0 || strlen(password) == 0)
    {
        return -1;
    }

    return 0;
}

int parse_json_field(const char *json, const char *field, char *output, int output_len)
{
    if (!json || !field || !output || output_len <= 0)
        return -1;

    output[0] = '\0'; // initialize output to empty string

    // Locate the field in JSON
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", field);

    const char *start = strstr(json, search_key);
    if (!start)
        return -1;

    start = strchr(start, ':');
    if (!start)
        return -1;

    start++; // move past ':'

    // Skip whitespace and opening quote
    while (*start && (isspace(*start) || *start == '"'))
        start++;

    // Copy characters until closing quote or end
    int i = 0;
    while (*start && *start != '"' && i < output_len - 1)
        output[i++] = *start++;

    output[i] = '\0';
    return (i > 0) ? 0 : -1;
}

// Basic email validation
int is_valid_email(const char *email)
{
    if (!email || strlen(email) < 5)
        return 0;

    const char *at_sign = strchr(email, '@');
    if (!at_sign || at_sign == email)
        return 0;

    const char *dot = strrchr(at_sign, '.');
    if (!dot || dot == at_sign + 1 || *(dot + 1) == '\0')
        return 0;

    return 1;
}

// Basic name validation
int is_valid_name(const char *name)
{
    if (!name)
        return 0;

    int len = strlen(name);
    if (len < 3 || len > 50)
        return 0;

    // Check for valid characters (alphanumeric and underscore)
    for (int i = 0; i < len; i++)
    {
        if (!isalnum(name[i]) && name[i] != '_')
        {
            return 0;
        }
    }

    return 1;
}

// Basic password validation
int is_valid_password(const char *password)
{
    if (!password)
        return 0;

    int len = strlen(password);
    if (len < 3 || len > 50)
        return 0;

    // Check for valid characters (alphanumeric and underscore)
    for (int i = 0; i < len; i++)
    {
        if (!isalnum(password[i]) && password[i] != '_')
        {
            return 0;
        }
    }

    return 1;
}