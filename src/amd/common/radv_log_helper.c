/*
 * Copyright 2026 JimVulkan
 * SPDX-License-Identifier: MIT
 */

#include "radv_logcat.h"
#include "ac_xclipse_log.h"
#include <android/log.h>
#include <sys/system_properties.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* The single Xclipse log switch -- levels and rationale in ac_xclipse_log.h. */
int
ac_xclipse_log_level(void)
{
   static int env_level = -2; /* -2 = not read yet, -1 = env unset */
   static int prop_level;
   static unsigned poll;

   if (env_level == -2) {
      const char *e = getenv("RADV_XCLIPSE_LOG");
      int v = (e && e[0]) ? atoi(e) : -1;
      env_level = v < -1 ? 0 : v;
   }
   if (env_level >= 0)
      return env_level;
   if (prop_level > 0)
      return prop_level;
   /* Off, the default: re-read the property on 1 call in 4096. The rest cost one increment. */
   if (poll++ & 0xfff)
      return 0;
   char v[PROP_VALUE_MAX] = {0};
   if (__system_property_get("debug.radv_xclipse_log", v) > 0 && v[0]) {
      const int l = atoi(v);
      prop_level = l > 0 ? l : 0;
   }
   return prop_level;
}

/* The verbose bring-up trace (RADV_LOGI and friends): debug.radv_xclipse_log level 2. */
__attribute__((used, visibility("default")))
void radv_log_msg(const char *fmt, ...) {
   if (ac_xclipse_log_level() < 2)
      return;
   char buf[512];
   va_list ap;
   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);
   __android_log_print(ANDROID_LOG_INFO, "RADV_XCLIPSE", "%s", buf);
}

/* The bring-up identity log -- see ac_xclipse_log.h. */
static pthread_mutex_t xclipse_id_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *xclipse_id_file;
static bool xclipse_id_tried;

static FILE *
xclipse_id_open(void)
{
   char pkg[256] = {0}, propdir[PROP_VALUE_MAX] = {0}, appdir[320] = {0}, path[400];
   __system_property_get("debug.radv_xclipse_id_dir", propdir);

   /* The process name is the package (up to a ':' for secondary processes). */
   FILE *c = fopen("/proc/self/cmdline", "r");
   if (c) {
      if (fgets(pkg, sizeof(pkg), c)) {
         char *colon = strchr(pkg, ':');
         if (colon)
            *colon = 0;
      }
      fclose(c);
   }
   if (pkg[0] && strchr(pkg, '.'))
      snprintf(appdir, sizeof(appdir), "/sdcard/Android/data/%s/files", pkg);

   const char *dirs[] = {getenv("RADV_XCLIPSE_ID_DIR"), propdir, appdir, getenv("TMPDIR")};
   for (unsigned i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
      if (!dirs[i] || !dirs[i][0])
         continue;
      snprintf(path, sizeof(path), "%s/radv_xclipse_id.txt", dirs[i]);
      FILE *f = fopen(path, "w");
      if (f) {
         __android_log_print(ANDROID_LOG_INFO, "RADV_XCLIPSE_ID", "[LOG] also writing %s", path);
         fprintf(f, "pid %d, process %s\n", getpid(), pkg[0] ? pkg : "?");
         return f;
      }
   }
   __android_log_print(ANDROID_LOG_WARN, "RADV_XCLIPSE_ID", "[LOG] no writable directory for radv_xclipse_id.txt");
   return NULL;
}

void
ac_xclipse_id_log(int prio, const char *fmt, ...)
{
   char buf[1024];
   va_list ap;
   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);

   __android_log_write(prio, "RADV_XCLIPSE_ID", buf);

   pthread_mutex_lock(&xclipse_id_lock);
   if (!xclipse_id_tried) {
      xclipse_id_tried = true;
      xclipse_id_file = xclipse_id_open();
   }
   if (xclipse_id_file) {
      size_t len = strlen(buf);
      fprintf(xclipse_id_file, "%s%s", buf, len && buf[len - 1] == '\n' ? "" : "\n");
      fflush(xclipse_id_file);
   }
   pthread_mutex_unlock(&xclipse_id_lock);
}
