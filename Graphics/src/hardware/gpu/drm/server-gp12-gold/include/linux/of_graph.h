#ifndef _QNX_LINUX_OF_GRAPH_H
#define _QNX_LINUX_OF_GRAPH_H

#include <linux/of.h>
#include <linux/errno.h>

/**
 * struct of_endpoint - the OF graph endpoint data structure
 * @port: identifier (value of reg property) of a port this endpoint belongs to
 * @id: identifier (value of reg property) of this endpoint
 * @local_node: pointer to device_node of this endpoint
 */
struct of_endpoint {
	unsigned int port;
	unsigned int id;
	const struct device_node *local_node;
};

static inline int of_graph_parse_endpoint(const struct device_node *node,
					struct of_endpoint *endpoint)
{
	return -ENOSYS;
}

static inline struct device_node *of_graph_get_next_endpoint(
					const struct device_node *parent,
					struct device_node *previous)
{
	return NULL;
}

static inline struct device_node *of_graph_get_remote_port_parent(
					const struct device_node *node)
{
	return NULL;
}

static inline struct device_node *of_graph_get_remote_port(
					const struct device_node *node)
{
	return NULL;
}

#define for_each_endpoint_of_node(parent, child) \
         for (child = of_graph_get_next_endpoint(parent, NULL); child != NULL; \
              child = of_graph_get_next_endpoint(parent, child))

#endif /* _QNX_LINUX_OF_GRAPH_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/of_graph.h $ $Rev: 836322 $")
#endif
