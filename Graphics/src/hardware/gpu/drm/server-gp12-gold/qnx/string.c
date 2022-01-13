#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/string.h>

#include <errno.h>

void *check_bytes8(const u8 *start, u8 value, unsigned int bytes)
{
	while (bytes) {
		if (*start != value)
			return (void *)start;
		start++;
		bytes--;
	}
	return NULL;
}

/**
 * memchr_inv - Find an unmatching character in an area of memory.
 * @start: The memory area
 * @c: Find a character other than c
 * @bytes: The size of the area.
 *
 * returns the address of the first character other than @c, or %NULL
 * if the whole buffer contains just @c.
 */
void *memchr_inv(const void *start, int c, size_t bytes)
{
	u8 value = c;
	u64 value64;
	unsigned int words, prefix;

	if (bytes <= 16)
		return check_bytes8(start, value, bytes);

	value64 = value;
#if defined(ARCH_HAS_FAST_MULTIPLIER) && BITS_PER_LONG == 64
	value64 *= 0x0101010101010101;
#elif defined(ARCH_HAS_FAST_MULTIPLIER)
	value64 *= 0x01010101;
	value64 |= value64 << 32;
#else
	value64 |= value64 << 8;
	value64 |= value64 << 16;
	value64 |= value64 << 32;
#endif

	prefix = (unsigned long)start % 8;
	if (prefix) {
		u8 *r;

		prefix = 8 - prefix;
		r = check_bytes8(start, value, prefix);
		if (r)
			return r;
		start += prefix;
		bytes -= prefix;
	}

	words = bytes / 8;

	while (words) {
		if (*(u64 *)start != value64)
			return check_bytes8(start, value, 8);
		start += 8;
		words--;
	}

	return check_bytes8(start, value, bytes % 8);
}

int strtobool(const char *s, bool *res)
{
	switch (s[0]) {
	case 'y':
	case 'Y':
	case '1':
		*res = true;
		break;
	case 'n':
	case 'N':
	case '0':
		*res = false;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/**
 * match_string - matches given string in an array
 * @array:	array of strings
 * @n:		number of strings in the array or -1 for NULL terminated arrays
 * @string:	string to match with
 *
 * Return:
 * index of a @string in the @array if matches, or %-EINVAL otherwise.
 */
int match_string(const char * const *array, size_t n, const char *string)
{
	int index;
	const char *item;

	for (index = 0; index < n; index++) {
		item = array[index];
		if (!item)
			break;
		if (!strcmp(item, string))
			return index;
	}

	return -EINVAL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/string.c $ $Rev: 836322 $")
#endif
