idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES flash_memory led radio_comm)
