/*
 * Copyright 2026 JimVulkan
 * SPDX-License-Identifier: MIT
 *
 * One switch for every diagnostic log line and the CPU-side instruments that feed them.
 *
 *   RADV_XCLIPSE_LOG (env)  /  debug.radv_xclipse_log (property)
 *     0 or unset   silent (default). ERROR lines still print on failure paths.
 *     1            the diagnostic markers: [BUILD], [ARM], RADV_KILL, RADV_CHURN, and the
 *                  BCn / CPUSHARE / BO census that feeds them
 *     2            1 + the verbose bring-up trace (RADV_LOGI, including [DRAW] at ~7 MB/s)
 *
 * An empty env var counts as unset. While the property reads 0 it is re-read every 4096 calls, so
 * a setprop reaches a running emulator.
 */
#ifndef AC_XCLIPSE_LOG_H
#define AC_XCLIPSE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

int ac_xclipse_log_level(void);

/* Bring-up identity log: logcat tag RADV_XCLIPSE_ID and, because a busy logcat ring is overwritten
 * within seconds, also radv_xclipse_id.txt in the app's external files directory
 * (/sdcard/Android/data/<package>/files), RADV_XCLIPSE_ID_DIR or debug.radv_xclipse_id_dir.
 * Always on: it is a few dozen lines per process. prio is an android_LogPriority. */
void ac_xclipse_id_log(int prio, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
}
#endif

#ifdef __ANDROID__
#include <android/log.h>

/* __android_log_print, but free while the level is 0: the arguments are not even evaluated. */
#define AC_XCLIPSE_LOGP(prio, tag, ...)                                                            \
   ((void)(((prio) >= ANDROID_LOG_ERROR || ac_xclipse_log_level() >= 1)                           \
              ? __android_log_print((prio), (tag), __VA_ARGS__)                                    \
              : 0))
#endif

#endif /* AC_XCLIPSE_LOG_H */
