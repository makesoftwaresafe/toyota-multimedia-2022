#ifndef _QNX_LINUX_OF_H
#define _QNX_LINUX_OF_H

#include <linux/property.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/mod_devicetable.h>

typedef u32 phandle;
typedef u32 ihandle;

struct property {
	char	*name;
	int	length;
	void	*value;
	struct property *next;
	unsigned long _flags;
	unsigned int unique_id;
	struct bin_attribute attr;
};

#if defined(CONFIG_SPARC)
struct of_irq_controller;
#endif

struct device_node {
	const char *name;
	const char *type;
	phandle phandle;
	const char *full_name;
	struct fwnode_handle fwnode;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
	struct	kobject kobj;
	unsigned long _flags;
	void	*data;
#if defined(CONFIG_SPARC)
	const char *path_component_name;
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
#endif
};

#define MAX_PHANDLE_ARGS 16
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};

struct of_reconfig_data {
	struct device_node	*dn;
	struct property		*prop;
	struct property		*old_prop;
};

static inline struct device_node *of_node_get(struct device_node *node)
{
	return node;
}

static inline void of_node_put(struct device_node *node)
{
}

static inline struct device_node *of_node(struct fwnode_handle *fwnode)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type)
{
	return NULL;
}

static inline struct device_node *of_find_matching_node_and_match(
	struct device_node *from,
	const struct of_device_id *matches,
	const struct of_device_id **match)
{
	return NULL;
}

static inline struct device_node *of_find_node_by_path(const char *path)
{
	return NULL;
}

static inline struct device_node *of_find_node_opts_by_path(const char *path,
	const char **opts)
{
	return NULL;
}

static inline struct device_node *of_get_parent(const struct device_node *node)
{
	return NULL;
}

static inline struct device_node *of_get_next_child(
	const struct device_node *node, struct device_node *prev)
{
	return NULL;
}

static inline struct device_node *of_get_next_available_child(
	const struct device_node *node, struct device_node *prev)
{
	return NULL;
}

static inline struct device_node *of_find_node_with_property(
	struct device_node *from, const char *prop_name)
{
	return NULL;
}

static inline bool of_have_populated_dt(void)
{
	return false;
}

static inline struct device_node *of_get_child_by_name(
					const struct device_node *node,
					const char *name)
{
	return NULL;
}

static inline int of_device_is_compatible(const struct device_node *device,
					  const char *name)
{
	return 0;
}

static inline bool of_device_is_available(const struct device_node *device)
{
	return false;
}

static inline struct property *of_find_property(const struct device_node *np,
						const char *name,
						int *lenp)
{
	return NULL;
}

static inline struct device_node *of_find_compatible_node(
						struct device_node *from,
						const char *type,
						const char *compat)
{
	return NULL;
}

static inline int of_property_count_elems_of_size(const struct device_node *np,
			const char *propname, int elem_size)
{
	return -ENOSYS;
}

static inline int of_property_read_u32_index(const struct device_node *np,
			const char *propname, u32 index, u32 *out_value)
{
	return -ENOSYS;
}

static inline int of_property_read_u8_array(const struct device_node *np,
			const char *propname, u8 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u32_array(const struct device_node *np,
					     const char *propname,
					     u32 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u64_array(const struct device_node *np,
					     const char *propname,
					     u64 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_string(struct device_node *np,
					  const char *propname,
					  const char **out_string)
{
	return -ENOSYS;
}

static inline int of_property_read_string_helper(struct device_node *np,
						 const char *propname,
						 const char **out_strs, size_t sz, int index)
{
	return -ENOSYS;
}

static inline const void *of_get_property(const struct device_node *node,
				const char *name,
				int *lenp)
{
	return NULL;
}

static inline struct device_node *of_get_cpu_node(int cpu,
					unsigned int *thread)
{
	return NULL;
}

static inline int of_property_read_u64(const struct device_node *np,
				       const char *propname, u64 *out_value)
{
	return -ENOSYS;
}

static inline int of_property_match_string(struct device_node *np,
					   const char *propname,
					   const char *string)
{
	return -ENOSYS;
}

static inline struct device_node *of_parse_phandle(const struct device_node *np,
						   const char *phandle_name,
						   int index)
{
	return NULL;
}

static inline int of_parse_phandle_with_args(struct device_node *np,
					     const char *list_name,
					     const char *cells_name,
					     int index,
					     struct of_phandle_args *out_args)
{
	return -ENOSYS;
}

static inline int of_parse_phandle_with_fixed_args(const struct device_node *np,
	const char *list_name, int cells_count, int index,
	struct of_phandle_args *out_args)
{
	return -ENOSYS;
}

static inline int of_count_phandle_with_args(struct device_node *np,
					     const char *list_name,
					     const char *cells_name)
{
	return -ENOSYS;
}

static inline int of_machine_is_compatible(const char *compat)
{
	return 0;
}

static inline bool of_console_check(const struct device_node *dn, const char *name, int index)
{
	return false;
}

static inline const __be32 *of_prop_next_u32(struct property *prop,
		const __be32 *cur, u32 *pu)
{
	return NULL;
}

static inline const char *of_prop_next_string(struct property *prop,
		const char *cur)
{
	return NULL;
}

#define of_match_ptr(_ptr)	NULL
#define of_match_node(_matches, _node)	NULL

static inline int of_property_read_u32(const struct device_node *np,
				       const char *propname,
				       u32 *out_value)
{
	return of_property_read_u32_array(np, propname, out_value, 1);
}

#define for_each_available_child_of_node(parent, child) \
	for (child = of_get_next_available_child(parent, NULL); child != NULL; \
	     child = of_get_next_available_child(parent, child))

/* flag descriptions (need to be visible even when !CONFIG_OF) */
#define OF_DYNAMIC	1 /* node and properties were allocated via kmalloc */
#define OF_DETACHED	2 /* node has been detached from the device tree */
#define OF_POPULATED	3 /* device already created for the node */
#define OF_POPULATED_BUS	4 /* of_platform_populate recursed to children of this node */

#ifdef CONFIG_OF
#else /* CONFIG_OF */

static inline void of_core_init(void)
{
}

static inline bool is_of_node(struct fwnode_handle *fwnode)
{
	return false;
}

static inline struct device_node *to_of_node(struct fwnode_handle *fwnode)
{
	return NULL;
}

static inline const char* of_node_full_name(const struct device_node *np)
{
	return "<no-node>";
}

static inline void of_node_clear_flag(struct device_node *n, unsigned long flag)
{
}

static inline int of_alias_get_id(struct device_node *np, const char *stem)
{
	return -ENOSYS;
}


#endif /* CONFIG_OF */

#if defined(CONFIG_OF_DYNAMIC)
extern int of_reconfig_notifier_register(struct notifier_block *);
extern int of_reconfig_notifier_unregister(struct notifier_block *);
#else
static inline int of_reconfig_notifier_register(struct notifier_block *nb)
{
	return -EINVAL;
}
static inline int of_reconfig_notifier_unregister(struct notifier_block *nb)
{
	return -EINVAL;
}
#endif /* CONFIG_OF_DYNAMIC */

extern int of_alias_get_highest_id(const char *stem);

#endif /* _QNX_LINUX_OF_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/of.h $ $Rev: 836322 $")
#endif
