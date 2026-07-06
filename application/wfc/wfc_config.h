/*
 * wfc_config.h — compile-time target configuration for the WatchFace Compiler.
 *
 * wfc is built for two very different hosts:
 *
 *   WFC_TARGET_PC  (default) — desktop / CI. Generous capacity caps, file I/O,
 *                              stderr diagnostics, exit() on error.
 *   WFC_TARGET_MCU           — on-device (e.g. Cortex-M33 + FPU). Minimal static
 *                              RAM, no file I/O, error returned via longjmp so
 *                              the compiler is a re-entrant library call.
 *
 * Select the target with -DWFC_TARGET_MCU (or -DWFC_TARGET_PC). Any individual
 * WFC_MAX_* / WFC_STRPOOL_CAP may be overridden per product from the build,
 * e.g. -DWFC_MAX_WIDGETS=64, because they are only defined here when unset.
 */
#ifndef WFC_CONFIG_H
#define WFC_CONFIG_H

/* Exactly one target; default to PC when the build specifies neither. */
#if !defined(WFC_TARGET_PC) && !defined(WFC_TARGET_MCU)
#  define WFC_TARGET_PC 1
#endif
#if defined(WFC_TARGET_PC) && defined(WFC_TARGET_MCU)
#  error "Define only one of WFC_TARGET_PC / WFC_TARGET_MCU"
#endif

#if defined(WFC_TARGET_MCU)
/* ---- Embedded: caps sized to a realistic single-watchface budget. ------- */
#  ifndef WFC_MAX_WIDGETS
#    define WFC_MAX_WIDGETS 128
#  endif
#  ifndef WFC_MAX_IMAGES
#    define WFC_MAX_IMAGES 256
#  endif
#  ifndef WFC_MAX_FONTS
#    define WFC_MAX_FONTS 16
#  endif
#  ifndef WFC_MAX_ANIMS
#    define WFC_MAX_ANIMS 32
#  endif
#  ifndef WFC_MAX_EVENTS
#    define WFC_MAX_EVENTS 128
#  endif
#  ifndef WFC_MAX_STYLES
#    define WFC_MAX_STYLES 64
#  endif
#  ifndef WFC_STRPOOL_CAP
#    define WFC_STRPOOL_CAP 2048
#  endif
/* No hosted stdio (files / stderr / exit) on the target. */
#  define WFC_HAVE_STDIO 0

#else
/* ---- PC / CI: keep the original generous caps and behaviour. ------------ */
#  ifndef WFC_MAX_WIDGETS
#    define WFC_MAX_WIDGETS 512
#  endif
#  ifndef WFC_MAX_IMAGES
#    define WFC_MAX_IMAGES 1024
#  endif
#  ifndef WFC_MAX_FONTS
#    define WFC_MAX_FONTS 64
#  endif
#  ifndef WFC_MAX_ANIMS
#    define WFC_MAX_ANIMS 64
#  endif
#  ifndef WFC_MAX_EVENTS
#    define WFC_MAX_EVENTS 512
#  endif
#  ifndef WFC_MAX_STYLES
#    define WFC_MAX_STYLES 128
#  endif
#  ifndef WFC_STRPOOL_CAP
#    define WFC_STRPOOL_CAP 8192
#  endif
#  define WFC_HAVE_STDIO 1
#endif

#endif /* WFC_CONFIG_H */
