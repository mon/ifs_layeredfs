#pragma once

typedef struct {
	bool verbose_logs;
	bool developer_mode;
} config_t;

extern config_t config;

void load_config(void);