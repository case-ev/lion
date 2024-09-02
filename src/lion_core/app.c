#include "init.h"
#include <lion/lion.h>
#include <lion_utils/macros.h>
#include <lion_utils/vendor/log.h>
#include <lionu/mem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define CREATE_DIRECTORY(dirname) CreateDirectory(dirname, NULL)
#define DIR_ALREADY_EXISTS_ERROR_CODE ERROR_ALREADY_EXISTS
#define EPOCH_DIFF 11644473600000ULL // Milliseconds
#define GET_CURRENT_TIME(current_time)                                         \
  {                                                                            \
    FILETIME ft;                                                               \
    ULARGE_INTEGER ft64;                                                       \
    GetSystemTimeAsFileTime(&ft);                                              \
    ft64.LowPart = ft.dwLowDateTime;                                           \
    ft64.HighPart = ft.dwHighDateTime;                                         \
    current_time = (int64_t)(ft64.QuadPart / 10000 - EPOCH_DIFF);              \
  }
#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#define CREATE_DIRECTORY(dirname) mkdir(dirname, 0777)
#define DIR_ALREADY_EXISTS_ERROR_CODE EEXIST
#define GET_CURRENT_TIME(current_time)                                         \
  {                                                                            \
    struct timeval tv;                                                         \
    gettimeofday(&tv, NULL);                                                   \
    current_time = (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL; \
  }
#endif

const lion_app_config_t LION_APP_CONFIG_DEFAULT = {
    .app_name = "Application",
    .log_dir = NULL,
    .log_stdlvl = LOG_INFO,
    .log_filelvl = LOG_TRACE,
};

lion_status_t lion_app_config_new(lion_app_config_t *out) {
  lion_app_config_t conf = LION_APP_CONFIG_DEFAULT;
  *out = conf;
  return LION_STATUS_SUCCESS;
}

lion_app_config_t lion_app_config_default(void) {
  return LION_APP_CONFIG_DEFAULT;
}

static lion_status_t create_directory(const char *dirname) {
#ifdef _WIN32
  if (CREATE_DIRECTORY(dirname) == 0) {
    DWORD error = GetLastError();
    if (error == DIR_ALREADY_EXISTS_ERROR_CODE) {
      logi_debug("Log directory '%s' already exists", dirname);
      return LION_STATUS_SUCCESS;
    } else {
      logi_error("Failed to create directory '%s'", dirname);
      return LION_STATUS_FAILURE;
    }
  }
#else
  if (CREATE_DIRECTORY(dirname) == -1) {
    if (errno == DIR_ALREADY_EXISTS_ERROR_CODE) {
      logi_debug("Log directory '%s' already exists", dirname);
      return LION_STATUS_SUCCESS;
    } else {
      logi_error("Failed to create directory '%s'", dirname);
      return LION_STATUS_FAILURE;
    }
  }
#endif
  logi_debug("Succesfully created directory '%s'", dirname);
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_app_new(lion_app_config_t *conf, lion_app_t *out) {
  lion_app_t app = {
      .conf = conf,

#ifndef NDEBUG // Internal debug information
      ._idebug_malloced_total = 0,
#endif
  };

  // Logging setup
  time_t seconds = time(NULL);
  struct tm *time = localtime(&seconds);
  log_set_level(app.conf->log_stdlvl);

  if (app.conf->log_dir == NULL) {
    logi_warn("Log directory not specified, not logging to file");
  } else {
    if (create_directory(app.conf->log_dir) == LION_STATUS_SUCCESS) {
      size_t log_dir_len = strnlen(app.conf->log_dir, FILENAME_MAX);
      strncpy(app.log_filename, app.conf->log_dir, log_dir_len);

      app.log_filename[strftime(app.log_filename + log_dir_len,
                                _LION_LOGFILE_MAX, "/%Y%m%d_%H%M%S.txt", time) +
                       log_dir_len] = '\0';

      app.log_file = fopen(app.log_filename, "w");
      if (app.log_file == NULL) {
        logi_error("Failed to create log file, not logging to file");
      } else {
        logi_info("Log file : '%s'", app.log_filename);
        log_add_fp_internal(app.log_file, app.conf->log_filelvl);
      }
    } else {
      logi_error("Failed to create log directory, not logging to file");
    }
  }

  *out = app;
  return LION_STATUS_SUCCESS;
}

static void lion_app_log_startup_info(lion_app_t *app) {
#ifndef NDEBUG
  logi_warn("!!! RUNNING IN DEBUG MODE !!!");
#endif
  logi_info("+-------------------------------------------------------+");
  logi_info("|################# STARTUP INFORMATION #################|");
  logi_info("+-------------------------------------------------------+");
  logi_info(" * Application name            : %s", app->conf->app_name);
  logi_info(" * Engine version              : %u.%u.%u",
            LION_ENGINE_VERSION_MAJOR, LION_ENGINE_VERSION_MINOR,
            LION_ENGINE_VERSION_PATCH);
  logi_info(" * `lion_app_t` struct size   : %d B", sizeof(lion_app_t));
  logi_info("+-------------------------------------------------------+");
  logi_info("|################# END OF INFORMATION ##################|");
  logi_info("+-------------------------------------------------------+");
}

lion_status_t lion_app_run(lion_app_t *app) {
  logi_info("Application start");
  lion_app_log_startup_info(app);
#ifndef NDEBUG
  LION_CALL_I(lion_init_debug(app), "Failed initializing debug information");
#endif

  logi_info("Initializing application");
  LION_CALL_I(lion_init(app), "Failed initializing app");

  logi_debug("Running application");
  // TODO: Implement application logic

  logi_debug("Cleaning up application");
  LION_CALL_I(lion_app_cleanup(app), "Failed cleaning app");
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_app_cleanup(lion_app_t *app) {
#ifndef NDEBUG
  if (app->_idebug_malloced_total != 0) {
    logi_warn("MEMORY LEAK: Found %lli elements (%d B) in heap after cleanup",
              app->_idebug_malloced_total, app->_idebug_malloced_size);

    logi_warn("MEMORY LEAK LOCATIONS:");
    _idebug_heap_info_t *node = app->_idebug_heap_head;
    while (node != NULL) {
      logi_warn(" * %#p (%d B) @ %s:%d", node->addr, node->size, node->file,
                node->line);
      node = node->next;
    }
  }

  _idebug_heap_info_t *node = app->_idebug_heap_head;
  int64_t count = (int64_t)heapinfo_count(app);
  if (app->_idebug_malloced_total != count) {
    logi_error(
        "Found mismatch between reported (%d) and stored (%d) allocations",
        app->_idebug_malloced_total, count);

    node = app->_idebug_heap_head;
    logi_error("Stored allocations are:");
    while (node != NULL) {
      logi_error(" * %#p (%d B) @ %s:%d", node->addr, node->size, node->file,
                 node->line);
      node = node->next;
    }
  }
  heapinfo_clean(app);
#endif

  return LION_STATUS_SUCCESS;
}

int lion_app_should_close(lion_app_t *app) { return 0; }