/*
 * This file was originally distributed as part of
 * shadow-utils-4.0.17-12.fc6.src.rpm and has been modified
 * in WRLinux for inclusion in rpm.
 */

extern struct sgrp *__sgr_dup (const struct sgrp *);
extern void __sgr_set_changed (void);
extern int sgr_close (void);
extern int sgr_file_present (void);
extern const struct sgrp *sgr_locate (const char *);
extern int sgr_lock (void);
extern int sgr_name (const char *);
extern const struct sgrp *sgr_next (void);
extern int sgr_open (int);
extern int sgr_remove (const char *);
extern int sgr_rewind (void);
extern int sgr_unlock (void);
extern int sgr_update (const struct sgrp *);
extern int sgr_sort (void);
