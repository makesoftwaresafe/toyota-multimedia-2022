#include <linux/qnx.h>
#include <linux/linux.h>

/*
 * userns count is 1 for root user, 1 for init_uts_ns,
 * and 1 for... ?
 */
struct user_namespace init_user_ns = {
	.uid_map = {
		.nr_extents = 1,
		.extent[0] = {
			.first = 0,
			.lower_first = 0,
			.count = 4294967295U,
		},
	},
	.gid_map = {
		.nr_extents = 1,
		.extent[0] = {
			.first = 0,
			.lower_first = 0,
			.count = 4294967295U,
		},
	},
	.projid_map = {
		.nr_extents = 1,
		.extent[0] = {
			.first = 0,
			.lower_first = 0,
			.count = 4294967295U,
		},
	},
	.count = ATOMIC_INIT(3),
	.owner = GLOBAL_ROOT_UID,
	.group = GLOBAL_ROOT_GID,
	.ns.inum = PROC_USER_INIT_INO,
#ifdef CONFIG_USER_NS
	.ns.ops = &userns_operations,
#endif
	.flags = USERNS_INIT_FLAGS,
#ifdef CONFIG_PERSISTENT_KEYRINGS
	.persistent_keyring_register_sem =
	__RWSEM_INITIALIZER(init_user_ns.persistent_keyring_register_sem),
#endif
};

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/user.c $ $Rev: 836322 $")
#endif
