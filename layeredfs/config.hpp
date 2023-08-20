#pragma once

#include <unordered_set>
#include <string>

typedef struct {
    bool verbose_logs;
    bool developer_mode;
    bool disable;
    const char *logfile;
    std::unordered_set<std::string> allowlist;
    std::unordered_set<std::string> blocklist;
} config_t;

extern config_t config;

void load_config(void);
void print_config(void);
