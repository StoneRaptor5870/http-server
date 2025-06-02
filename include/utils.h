#ifndef UTILS_H
#define UTILS_H

int parse_user_json(const char *json, char *name, int name_len, char *email, int email_len,
                    char *password, int password_len);
int parse_json_field(const char *json, const char *field, char *output, int output_len);
int is_valid_email(const char *email);
int is_valid_name(const char *name);
int is_valid_password(const char *password);

#endif // UTILS_H