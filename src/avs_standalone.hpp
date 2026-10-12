namespace avs_standalone {

// enables the exc handler, loads the AVS DLL, logs to stdout
bool preboot();
// assumes `init_avs` has been called and thus `avs_boot` and friends are loaded
bool boot(bool print_logs);
void shutdown();
bool load_dll();
} // namespace avs_standalone
