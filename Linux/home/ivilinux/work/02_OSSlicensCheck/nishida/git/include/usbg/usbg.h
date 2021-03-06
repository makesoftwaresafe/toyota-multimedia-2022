/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef __USBG_H__
#define __USBG_H__

#include <dirent.h>
#include <sys/queue.h>
#include <netinet/ether.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h> /* For FILE * */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file include/usbg/usbg.h
 * @todo Clean up static buffers in structures
 */

/**
 * @addtogroup libusbg
 * Public API for USB gadget-configfs library
 * @{
 */

#define DEFAULT_UDC		NULL
#define LANG_US_ENG		0x0409
#define DEFAULT_CONFIG_LABEL "config"

/* This one has to be at least 18 bytes to hold network address */
#define USBG_MAX_STR_LENGTH 256
#define USBG_MAX_PATH_LENGTH PATH_MAX
#define USBG_MAX_NAME_LENGTH 40
/* Dev name for ffs is a part of function name, we subtract 4 char for "ffs." */
#define USBG_MAX_DEV_LENGTH (USBG_MAX_NAME_LENGTH - 4)
/* ConfigFS just like SysFS uses page size as max size of file content */
#define USBG_MAX_FILE_SIZE 4096

/**
 * @brief Additional option for usbg_rm_* functions.
 * @details This option allows to remove all content
 * of gadget/config/function recursively.
 */
#define USBG_RM_RECURSE 1

/*
 * Internal structures
 */
struct usbg_state;
struct usbg_gadget;
struct usbg_config;
struct usbg_function;
struct usbg_binding;
struct usbg_udc;

/**
 * @brief State of the gadget devices in the system
 */
typedef struct usbg_state usbg_state;

/**
 * @brief USB gadget device
 */
typedef struct usbg_gadget usbg_gadget;

/**
 * @brief USB configuration
 */
typedef struct usbg_config usbg_config;

/**
 * @brief USB function
 */
typedef struct usbg_function usbg_function;

/**
 * @brief USB function to config binding
 */
typedef struct usbg_binding usbg_binding;

/**
 * @brief USB device controller
 */
typedef struct usbg_udc usbg_udc;

/**
 * @typedef usbg_gadget_attr
 * @brief Gadget attributes which can be set using
 * usbg_set_gadget_attr() function.
 */
typedef enum {
	USBG_GADGET_ATTR_MIN = 0,
	BCD_USB = USBG_GADGET_ATTR_MIN,
	B_DEVICE_CLASS,
	B_DEVICE_SUB_CLASS,
	B_DEVICE_PROTOCOL,
	B_MAX_PACKET_SIZE_0,
	ID_VENDOR,
	ID_PRODUCT,
	BCD_DEVICE,
	USBG_GADGET_ATTR_MAX,
} usbg_gadget_attr;

/**
 * @typedef usbg_gadget_attrs
 * @brief USB gadget device attributes
 */
typedef struct
{
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
} usbg_gadget_attrs;

/**
 * @typedef usbg_gadget_strs
 * @brief USB gadget device strings
 */
typedef struct
{
	char str_ser[USBG_MAX_STR_LENGTH];
	char str_mnf[USBG_MAX_STR_LENGTH];
	char str_prd[USBG_MAX_STR_LENGTH];
} usbg_gadget_strs;

/**
 * @typedef usbg_config_attrs
 * @brief USB configuration attributes
 */
typedef struct
{
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} usbg_config_attrs;

/**
 * @typedef usbg_config_strs
 * @brief USB configuration strings
 */
typedef struct
{
	char configuration[USBG_MAX_STR_LENGTH];
} usbg_config_strs;

/**
 * @typedef usbg_function_type
 * @brief Supported USB function types
 */
typedef enum
{
	USBG_FUNCTION_TYPE_MIN = 0,
	F_SERIAL = USBG_FUNCTION_TYPE_MIN,
	F_ACM,
	F_OBEX,
	F_ECM,
	F_SUBSET,
	F_NCM,
	F_EEM,
	F_RNDIS,
	F_PHONET,
	F_FFS,
	F_MASS_STORAGE,
	F_MIDI,
	F_UAC2,
	USBG_FUNCTION_TYPE_MAX,
} usbg_function_type;

/**
 * @typedef usbg_f_serial_attrs
 * @brief Attributes for Serial, ACM, and OBEX USB functions
 */
typedef struct {
	int port_num;
} usbg_f_serial_attrs;

/**
 * @typedef net_attrs
 * @brief Attributes for ECM, ECM subset, NCM, EEM, and RNDIS USB functions
 */
typedef struct {
	struct ether_addr dev_addr;
	struct ether_addr host_addr;
	const char *ifname;
	int qmult;
} usbg_f_net_attrs;

/**
 * @typedef usbg_f_phonet_attrs
 * @brief Attributes for the phonet USB function
 */
typedef struct {
	const char *ifname;
} usbg_f_phonet_attrs;

/**
 * @typedef usbg_f_ffs_attrs
 * @brief Attributes for function fs based functions
 * @details This is read only and a virtual attribute, it is non present
 * on config fs.
 */
typedef struct {
	const char *dev_name;
} usbg_f_ffs_attrs;

/**
 * @typedef usbg_f_ms_attrs
 * @brief Attributes for mass storage functions
 */
typedef struct usbg_f_ms_lun_attrs {
	int id;
	bool cdrom;
	bool ro;
	bool nofua;
	bool removable;
	const char *filename;
} usbg_f_ms_lun_attrs;

/**
 * @typedef usbg_f_ms_attrs
 * @brief Attributes for mass storage functions
 */
typedef struct {
	bool stall;
	int nluns;
	usbg_f_ms_lun_attrs **luns;
} usbg_f_ms_attrs;

/**
 * @typedef usbg_f_midi_attrs
 * @brief Attributes for the MIDI function
 */
typedef struct {
	int index;
	const char *id;
	unsigned int in_ports;
	unsigned int out_ports;
	unsigned int buflen;
	unsigned int qlen;
} usbg_f_midi_attrs;

/**
 * @typedef usbg_f_uac2_attrs
 * @brief Attributes for the UAC2 function
 */
typedef struct {
	int c_chmask;
	const char *c_srate;
	int c_srate_def;
	int c_ssize;
	int delay_tout;
	int p_chmask;
	const char *p_srate;
	int p_srate_def;
	int p_ssize;
} usbg_f_uac2_attrs;

/**
 * @typedef attrs
 * @brief Attributes for a given function type
 */
typedef union {
	usbg_f_serial_attrs serial;
	usbg_f_net_attrs net;
	usbg_f_phonet_attrs phonet;
	usbg_f_ffs_attrs ffs;
	usbg_f_ms_attrs ms;
	usbg_f_midi_attrs midi;
	usbg_f_uac2_attrs uac2;
} usbg_f_attrs;

typedef enum {
	USBG_F_ATTRS_SERIAL = 1,
	USBG_F_ATTRS_NET,
	USBG_F_ATTRS_PHONET,
	USBG_F_ATTRS_FFS,
	USBG_F_ATTRS_MS,
	USBG_F_ATTRS_MIDI,
	USBG_F_ATTRS_UAC2,
} usbg_f_attrs_type;

typedef struct {
	int attrs_type;
} usbg_f_attrs_header;

typedef struct {
	usbg_f_attrs_header header;
	usbg_f_attrs attrs;
} usbg_function_attrs;

/* Error codes */

/**
 * @typedef usbg_error
 * @brief Errors which could be returned by library functions
 */
typedef enum  {
	USBG_SUCCESS = 0,
	USBG_ERROR_NO_MEM = -1,
	USBG_ERROR_NO_ACCESS = -2,
	USBG_ERROR_INVALID_PARAM = -3,
	USBG_ERROR_NOT_FOUND = -4,
	USBG_ERROR_IO = -5,
	USBG_ERROR_EXIST = -6,
	USBG_ERROR_NO_DEV = -7,
	USBG_ERROR_BUSY = -8,
	USBG_ERROR_NOT_SUPPORTED = -9,
	USBG_ERROR_PATH_TOO_LONG = -10,
	USBG_ERROR_INVALID_FORMAT = -11,
	USBG_ERROR_MISSING_TAG = -12,
	USBG_ERROR_INVALID_TYPE = -13,
	USBG_ERROR_INVALID_VALUE = -14,
	USBG_ERROR_OTHER_ERROR = -99
} usbg_error;

/**
 * @brief Get the error name as a constant string
 * @param e error code
 * @return Constant string with error name
 */
extern const char *usbg_error_name(usbg_error e);

/**
 * @brief Get the short description of error
 * @param e error code
 * @return Constant string with error description
 */
extern const char *usbg_strerror(usbg_error e);

/* Library init and cleanup */

/**
 * @brief Initialize the libusbg library state
 * @param configfs_path Path to the mounted configfs filesystem
 * @param state Pointer to be filled with pointer to usbg_state
 * @return 0 on success, usbg_error on error
 */
extern int usbg_init(const char *configfs_path, usbg_state **state);

/**
 * @brief Clean up the libusbg library state
 * @param s Pointer to state
 */
extern void usbg_cleanup(usbg_state *s);

/**
 * @brief Get ConfigFS path
 * @param s Pointer to state
 * @return Path to configfs or NULL if error occurred
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_state is valid.
 * For example path is valid until usbg_cleanup() call.
 */
extern const char *usbg_get_configfs_path(usbg_state *s);

/**
 * @brief Get ConfigFS path length
 * @param s Pointer to state
 * @return Length of path or usbg_error if error occurred.
 */
extern size_t usbg_get_configfs_path_len(usbg_state *s);

/**
 * @brief Copy ConfigFS path to buffer
 * @param s Pointer to state
 * @param buf Buffer where path should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_configfs_path(usbg_state *s, char *buf, size_t len);

/* USB gadget queries */

/**
 * @brief Get a gadget device by name
 * @param s Pointer to state
 * @param name Name of the gadget device
 * @return Pointer to gadget or NULL if a matching gadget isn't found
 */
extern usbg_gadget *usbg_get_gadget(usbg_state *s, const char *name);

/**
 * @brief Get a function by name
 * @param g Pointer to gadget
 * @param type Function type
 * @param instance Instance of function
 * @return Pointer to function or NULL if a matching function isn't found
 */
extern usbg_function *usbg_get_function(usbg_gadget *g,
		usbg_function_type type, const char *instance);

/**
 * @brief Get a configuration by name
 * @param g Pointer to gadget
 * @param id Identify of configuration
 * @param label Configuration label. If not NULL this function will return
 * valid value only if configuration with given id exist and has this label.
 * If NULL this function will return config with given id (if such exist)
 * and ignore this param.
 * @return Pointer to config or NULL if a matching config isn't found
 */
extern usbg_config *usbg_get_config(usbg_gadget *g, int id, const char *label);

/**
 * @brief Get a udc by name
 * @param s Pointer to state
 * @param name Name of the udc
 * @return Pointer to udc or NULL if a matching udc isn't found
 */
extern usbg_udc *usbg_get_udc(usbg_state *s, const char *name);

/* USB gadget/config/function/binding removal */

/**
 * @brief Remove binding between configuration and function
 * @details This function frees also the memory allocated for binding
 * @param b Binding to be removed
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_binding(usbg_binding *b);

/**
 * @brief Remove configuration
 * @details This function frees also the memory allocated for configuration
 * @param c Configuration to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_config(usbg_config *c, int opts);

/**
 * @brief Remove existing USB function
 * @details This function frees also the memory allocated for function
 * @param f Function to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_function(usbg_function *f, int opts);

/**
 * @brief Remove existing USB gadget
 * @details This function frees also the memory allocated for gadget
 * @param g Gadget to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_gadget(usbg_gadget *g, int opts);

/**
 * @brief Remove configuration strings for given language
 * @param c Pointer to configuration
 * @param lang Language of strings which should be deleted
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_config_strs(usbg_config *c, int lang);

/**
 * @brief Remove gadget strings for given language
 * @param g Pointer to gadget
 * @param lang Language of strings which should be deleted
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_gadget_strs(usbg_gadget *g, int lang);

/* USB gadget allocation and configuration */

/**
 * @brief Create a new USB gadget device
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param idVendor Gadget vendor ID
 * @param idProduct Gadget product ID
 * @param g Pointer to be filled with pointer to gadget
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_gadget_vid_pid(usbg_state *s, const char *name,
		uint16_t idVendor, uint16_t idProduct, usbg_gadget **g);

/**
 * @brief Create a new USB gadget device and set given attributes
 * and strings
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param g_attrs Gadget attributes to be set. If NULL setting is omitted.
 * @param g_strs Gadget strings to be set. If NULL setting is omitted.
 * @param g Pointer to be filled with pointer to gadget
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_gadget(usbg_state *s, const char *name,
		const usbg_gadget_attrs *g_attrs, const usbg_gadget_strs *g_strs,
			      usbg_gadget **g);

/**
 * @brief Get string representing selected gadget attribute
 * @param attr code of selected attribute
 * @return String suitable for given attribute or NULL if such
 * string has not been found
 */
extern const char *usbg_get_gadget_attr_str(usbg_gadget_attr attr);

/**
 * @brief Lookup attr code based on its name
 * @param name of attribute
 * @return code of suitable attribute or usbg_error
 */
extern int usbg_lookup_gadget_attr(const char *name);

/**
 * @brief Set selected attribute to value
 * @param g Pointer to gadget
 * @param attr Code of selected attribute
 * @param val value to be set
 * @return 0 on success, usbg_error otherwise
 * @note val is of type int but value provided to this function should
 * be suitable to place it in type dedicated for selected attr (uint16 or uint8)
 */
extern int usbg_set_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr, int val);

/**
 * @brief Get value of selected attribute
 * @param g Pointer to gadget
 * @param attr Code of selected attribute
 * @return Value of selected attribute (always above zero) or
 * usbg_error if error occurred.
 * @note User should use only lowest one or two bytes as attribute value
 * depending on attribute size (see usbg_gadget_attrs for details).
 */
extern int usbg_get_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr);

/**
 * @brief Set the USB gadget attributes
 * @param g Pointer to gadget
 * @param g_attrs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_attrs(usbg_gadget *g,
		const usbg_gadget_attrs *g_attrs);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param g_attrs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_gadget_attrs(usbg_gadget *g, usbg_gadget_attrs *g_attrs);

/**
 * @brief Get gadget name
 * @param g Pointer to gadget
 * @return Gadget name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_gadget is valid.
 * For example gadget name is valid until someone remove gadget.
 */
extern const char *usbg_get_gadget_name(usbg_gadget *g);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_gadget_name_len(usbg_gadget *g);

/**
 * @brief Copy gadget name
 * @param g Pointer to gadget
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_gadget_name(usbg_gadget *g, char *buf, size_t len);

/**
 * @brief Set the USB gadget vendor id
 * @param g Pointer to gadget
 * @param idVendor USB device vendor id
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_vendor_id(usbg_gadget *g, uint16_t idVendor);

/**
 * @brief Set the USB gadget product id
 * @param g Pointer to gadget
 * @param idProduct USB device product id
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_product_id(usbg_gadget *g, uint16_t idProduct);

/**
 * @brief Set the USB gadget device class code
 * @param g Pointer to gadget
 * @param bDeviceClass USB device class code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_class(usbg_gadget *g,
		uint8_t bDeviceClass);

/**
 * @brief Set the USB gadget protocol code
 * @param g Pointer to gadget
 * @param bDeviceProtocol USB protocol code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_protocol(usbg_gadget *g,
		uint8_t bDeviceProtocol);

/**
 * @brief Set the USB gadget device subclass code
 * @param g Pointer to gadget
 * @param bDeviceSubClass USB device subclass code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_subclass(usbg_gadget *g,
		uint8_t bDeviceSubClass);

/**
 * @brief Set the maximum packet size for a gadget
 * @param g Pointer to gadget
 * @param bMaxPacketSize0 Maximum packet size
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_max_packet(usbg_gadget *g,
		uint8_t bMaxPacketSize0);

/**
 * @brief Set the gadget device BCD release number
 * @param g Pointer to gadget
 * @param bcdDevice BCD release number
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_bcd_device(usbg_gadget *g,
		uint16_t bcdDevice);

/**
 * @brief Set the gadget device BCD USB version
 * @param g Pointer to gadget
 * @param bcdUSB BCD USB version
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_bcd_usb(usbg_gadget *g, uint16_t bcdUSB);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param lang Language of strings
 * @param g_strs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_gadget_strs(usbg_gadget *g, int lang,
		usbg_gadget_strs *g_strs);

/**
 * @brief Set the USB gadget strings
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param g_strs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_strs(usbg_gadget *g, int lang,
		const usbg_gadget_strs *g_strs);

/**
 * @brief Set the serial number for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param ser Serial number
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_serial_number(usbg_gadget *g, int lang,
					 const char *ser);

/**
 * @brief Set the manufacturer name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param mnf Manufacturer
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_manufacturer(usbg_gadget *g, int lang,
					const char *mnf);

/**
 * @brief Set the product name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param prd Product
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_product(usbg_gadget *g, int lang,
				   const char *prd);

/* USB function allocation and configuration */

/**
 * @brief Create a new USB gadget function and set its attributes
 * @param g Pointer to gadget
 * @param type Type of function
 * @param instance Function instance name
 * @param f_attrs Function attributes to be set. If NULL setting is omitted.
 * @param f Pointer to be filled with pointer to function
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_function(usbg_gadget *g, usbg_function_type type,
		 const char *instance, const usbg_function_attrs *f_attrs,
				usbg_function **f);

/**
 * @brief Get function instance name
 * @param f Pointer to function
 * @return instance name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_function is valid.
 * For example instance name is valid until someone remove this function.
 */
extern const char *usbg_get_function_instance(usbg_function *f);

/**
 * @brief Get function instance name length
 * @param f function which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_function_instance_len(usbg_function *f);

/**
 * @brief Copy function instance name
 * @param f Pointer to function
 * @param buf Buffer where instance name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_function_instance(usbg_function *f, char *buf, size_t len);

/**
 * @brief Get function type as a string
 * @param type Function type
 * @return String suitable for given function type
 */
extern const char *usbg_get_function_type_str(usbg_function_type type);

/**
 * @brief Lookup function type suitable for given name
 * @param name Name of function
 * @return Function type enum or negative error code
 */
extern int usbg_lookup_function_type(const char *name);

/**
 * @brief Lookup attrs type for given type of function
 * @param f_type type of functions
 * @return Attributes type for this type of function
 */
extern int usbg_lookup_function_attrs_type(int f_type);

/**
 * @brief Cleanup content of function attributes
 * @param f_attrs function attributes which should be cleaned up.
 * @note This function should be called to free
 * additional memory allocated by usbg_get_function_attrs().
 * @warning None of attributes in passed structure should be
 * accessed after returning from this function.
 */
extern void usbg_cleanup_function_attrs(usbg_function_attrs *f_attrs);

/* USB configurations allocation and configuration */

/**
 * @brief Create a new USB gadget configuration
 * @param g Pointer to gadget
 * @param id Identify of configuration
 * @param label configuration label, if NULL, default is used
 * @param c_attrs Configuration attributes to be set
 * @param c_strs Configuration strings to be set
 * @param c Pointer to be filled with pointer to configuration
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_config(usbg_gadget *g, int id, const char *label,
		const usbg_config_attrs *c_attrs, const usbg_config_strs *c_strs,
		usbg_config **c);

/**
 * @brief Get config label
 * @param c Pointer to config
 * @return config label or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_config is valid.
 * For example config label is valid until someone remove this function.
 */
extern const char *usbg_get_config_label(usbg_config *c);

/**
 * @brief Get config label length
 * @param c Config which label length should be returned
 * @return Length of label or usbg_error if error occurred.
 */
extern size_t usbg_get_config_label_len(usbg_config *c);

/**
 * @brief Copy config label
 * @param c Pointer to config
 * @param buf Buffer where label should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_config_label(usbg_config *c, char *buf, size_t len);

/**
 * @brief Get config id
 * @param c Pointer to config
 * @return Configuration id or usbg_error if error occurred.
 */
extern int usbg_get_config_id(usbg_config *c);

/**
 * @brief Set the USB configuration attributes
 * @param c Pointer to configuration
 * @param c_attrs Configuration attributes
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_attrs(usbg_config *c,
		const usbg_config_attrs *c_attrs);

/**
 * @brief Get the USB configuration strings
 * @param c Pointer to configuration
 * @param c_attrs Structure to be filled
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_config_attrs(usbg_config *c, usbg_config_attrs *c_attrs);

/**
 * @brief Set the configuration maximum power
 * @param c Pointer to config
 * @param bMaxPower Maximum power (in 2 mA units)
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_max_power(usbg_config *c, int bMaxPower);

/**
 * @brief Set the configuration bitmap attributes
 * @param c Pointer to config
 * @param bmAttributes Configuration characteristics
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_bm_attrs(usbg_config *c, int bmAttributes);

/**
 * @brief Get the USB configuration strings
 * @param c Pointer to configuration
 * @param lang Language of strings
 * @param c_strs Structure to be filled
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_config_strs(usbg_config *c, int lang,
		usbg_config_strs *c_strs);

/**
 * @brief Set the USB configuration strings
 * @param c Pointer to configuration
 * @param lang USB language ID
 * @param c_strs Configuration strings
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_strs(usbg_config *c, int lang,
		const usbg_config_strs *c_strs);

/**
 * @brief Set the configuration string
 * @param c Pointer to config
 * @param lang USB language ID
 * @param string Configuration description
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_string(usbg_config *c, int lang, const char *string);

/**
 * @brief Add a function to a configuration
 * @param c Pointer to config
 * @param name Name of configuration function binding
 * @param f Pointer to function
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_add_config_function(usbg_config *c, const char *name,
				    usbg_function *f);

/**
 * @brief Get target function of given binding
 * @param b Binding between configuration and function
 * @return Pointer to USB function which is target for this binding
 */
extern usbg_function *usbg_get_binding_target(usbg_binding *b);

/**
 * @brief Get binding name
 * @param b Pointer to binding
 * @return Binding name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_binding is valid.
 * For example binding name is valid until someone remove this binding.
 */
extern const char *usbg_get_binding_name(usbg_binding *b);

/**
 * @brief Get binding name length
 * @param b Binding which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_binding_name_len(usbg_binding *b);

/**
 * @brief Copy binding name
 * @param b Pointer to binding
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_binding_name(usbg_binding *b, char *buf, size_t len);

/* USB gadget setup and teardown */

/**
 * @brief Enable a USB gadget device
 * @param g Pointer to gadget
 * @param udc where gadget should be assigned.
 *  If NULL, default one (first) is used.
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_enable_gadget(usbg_gadget *g, usbg_udc *udc);

/**
 * @brief Disable a USB gadget device
 * @param g Pointer to gadget
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_disable_gadget(usbg_gadget *g);

/**
 * @brief Get name of udc
 * @param u Pointer to udc
 * @return UDC name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_state is valid.
 * For example UDC name is valid until usbg_cleanup().
 */
extern const char *usbg_get_udc_name(usbg_udc *u);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 * @note If gadget isn't enabled on any udc returned size is 0.
 */
extern size_t usbg_get_gadget_udc_len(usbg_gadget *g);

/**
 * @brief Copy name of udc
 * @param u Pointer to udc
 * @param buf Buffer where udc name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_cpy_udc_name(usbg_udc *u, char *buf, size_t len);

/**
 * @brief Get udc to which gadget is bound
 * @param g Pointer to gadget
 * @return Pointer to UDC or NULL if gadget is not enabled
 */
extern usbg_udc *usbg_get_gadget_udc(usbg_gadget *g);

/**
 * @brief Get gadget which is attached to this UDC
 * @param u Pointer to udc
 * @return Pointer to gadget or NULL if UDC is free
 */
extern usbg_gadget *usbg_get_udc_gadget(usbg_udc *u);

/*
 * USB function-specific attribute configuration
 */

/**
 * @brief Get type of given function
 * @param f Pointer to function
 * @return usbg_function_type (0 or above) or
 *  usbg_error (below 0) if error occurred
 */
extern usbg_function_type usbg_get_function_type(usbg_function *f);

/**
 * @brief Get attributes of given function
 * @param f Pointer to function
 * @param f_attrs Union to be filled
 * @return 0 on success usbg_error if error occurred.
 */
extern int usbg_get_function_attrs(usbg_function *f,
		usbg_function_attrs *f_attrs);

/**
 * @brief Set attributes of given function
 * @param f Pointer to function
 * @param f_attrs Attributes to be set
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_function_attrs(usbg_function *f,
		const usbg_function_attrs *f_attrs);

/**
 * @brief Set USB function network device address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_dev_addr(usbg_function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network host address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_host_addr(usbg_function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network qmult
 * @param f Pointer to function
 * @param qmult Queue length multiplier
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_qmult(usbg_function *f, int qmult);

/**
 * @def usbg_for_each_gadget(g, s)
 * Iterates over each gadget
 */
#define usbg_for_each_gadget(g, s) \
	for (g = usbg_get_first_gadget(s); \
	g != NULL; \
	g = usbg_get_next_gadget(g))

/**
 * @def usbg_for_each_function(f, g)
 * Iterates over each function
 */
#define usbg_for_each_function(f, g) \
	for (f = usbg_get_first_function(g); \
	f != NULL; \
	f = usbg_get_next_function(f))

/**
 * @def usbg_for_each_config(c, g)
 * Iterates over each config
 */
#define usbg_for_each_config(c, g) \
	for (c = usbg_get_first_config(g); \
	c != NULL; \
	c = usbg_get_next_config(c))

/**
 * @def usbg_for_each_binding(b, c)
 * Iterates over each binding
 */
#define usbg_for_each_binding(b, c)	\
	for (b = usbg_get_first_binding(c); \
	b != NULL; \
	b = usbg_get_next_binding(b))

/**
 * @def usbg_for_each_udc(b, c)
 * Iterates over each udc
 */
#define usbg_for_each_udc(u, s)	\
	for (u = usbg_get_first_udc(s); \
	u != NULL; \
	u = usbg_get_next_udc(u))

/**
 * @brief Get first gadget in gadget list
 * @param s State of library
 * @return Pointer to gadget or NULL if list is empty.
 * @note Gadgets are sorted in strings (name) order
 */
extern usbg_gadget *usbg_get_first_gadget(usbg_state *s);

/**
 * @brief Get first function in function list
 * @param g Pointer of gadget
 * @return Pointer to function or NULL if list is empty.
 * @note Functions are sorted in strings (name) order
 */
extern usbg_function *usbg_get_first_function(usbg_gadget *g);

/**
 * @brief Get first config in config list
 * @param g Pointer of gadget
 * @return Pointer to configuration or NULL if list is empty.
 * @note Configs are sorted in strings (name) order
 */
extern usbg_config *usbg_get_first_config(usbg_gadget *g);

/**
 * @brief Get first binding in binding list
 * @param c Pointer to configuration
 * @return Pointer to binding or NULL if list is empty.
 * @note Bindings are sorted in strings (name) order
 */
extern usbg_binding *usbg_get_first_binding(usbg_config *c);

/**
 * @brief Get first udc in udc list
 * @param s State of library
 * @return Pointer to udc or NULL if list is empty.
 * @note UDCs are sorted in strings (name) order
 */
extern usbg_udc *usbg_get_first_udc(usbg_state *s);

/**
 * @brief Get the next gadget on a list.
 * @param g Pointer to current gadget
 * @return Next gadget or NULL if end of list.
 */
extern usbg_gadget *usbg_get_next_gadget(usbg_gadget *g);

/**
 * @brief Get the next function on a list.
 * @param f Pointer to current function
 * @return Next function or NULL if end of list.
 */
extern usbg_function *usbg_get_next_function(usbg_function *f);

/**
 * @brief Get the next config on a list.
 * @param c Pointer to current config
 * @return Next config or NULL if end of list.
 */
extern usbg_config *usbg_get_next_config(usbg_config *c);

/**
 * @brief Get the next binding on a list.
 * @param b Pointer to current binding
 * @return Next binding or NULL if end of list.
 */
extern usbg_binding *usbg_get_next_binding(usbg_binding *b);

/**
 * @brief Get the next udc on a list.
 * @param u Pointer to current udc
 * @return Next udc or NULL if end of list.
 */
extern usbg_udc *usbg_get_next_udc(usbg_udc *u);

/* Import / Export API */

/**
 * @brief Exports usb function to file
 * @param f Pointer to function to be exported
 * @param stream where function should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_function(usbg_function *f, FILE *stream);

/**
 * @brief Exports configuration to file
 * @param c Pointer to configuration to be exported
 * @param stream where configuration should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_config(usbg_config *c, FILE *stream);

/**
 * @brief Exports whole gadget to file
 * @param g Pointer to gadget to be exported
 * @param stream where gadget should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_gadget(usbg_gadget *g, FILE *stream);

/**
 * @brief Imports usb function from file and adds it to given gadget
 * @param g Gadget where function should be placed
 * @param stream from which function should be imported
 * @param instance name which should be used for new function
 * @param f place for pointer to imported function
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_function(usbg_gadget *g, FILE *stream,
				const char *instance, usbg_function **f);

/**
 * @brief Imports usb configuration from file and adds it to given gadget
 * @param g Gadget where configuration should be placed
 * @param stream from which configuration should be imported
 * @param id which should be used for new configuration
 * @param c place for pointer to imported configuration
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_config(usbg_gadget *g, FILE *stream, int id,
				usbg_config **c);
/**
 * @brief Imports usb gadget from file
 * @param s current state of library
 * @param stream from which gadget should be imported
 * @param name which should be used for new gadget
 * @param g place for pointer to imported gadget
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_gadget(usbg_state *s, FILE *stream,
			      const char *name, usbg_gadget **g);

/**
 * @brief Get text of error which occurred during last function import
 * @param g gadget where function import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_func_import_error_text(usbg_gadget *g);

/**
 * @brief Get line number where function import error occurred
 * @param g gadget where function import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_func_import_error_line(usbg_gadget *g);

/**
 * @brief Get text of error which occurred during last config import
 * @param g gadget where config import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_config_import_error_text(usbg_gadget *g);

/**
 * @brief Get line number where config import error occurred
 * @param g gadget where config import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_config_import_error_line(usbg_gadget *g);

/**
 * @brief Get text of error which occurred during last gadget import
 * @param s where gadget import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_gadget_import_error_text(usbg_state *s);

/**
 * @brief Get line number where gadget import error occurred
 * @param s where gadget import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_gadget_import_error_line(usbg_state *s);

/**
 * @brief Get the binding between the configuration and the function
 * @param c configuration of the gadget created
 * @param f function of the gadget, for which binding has to be identified
 * @return binding between configuration and function
 */
extern usbg_binding *usbg_get_link_binding(usbg_config *c, usbg_function *f);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __USBG_H__ */
