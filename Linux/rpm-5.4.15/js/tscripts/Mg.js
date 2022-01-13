if (loglvl) print("--> Mg.js");

const MAGIC_NONE		= 0x000000; /* No flags */
const MAGIC_DEBUG		= 0x000001; /* Turn on debugging */
const MAGIC_SYMLINK		= 0x000002; /* Follow symlinks */
const MAGIC_COMPRESS		= 0x000004; /* Check inside compressed files */
const MAGIC_DEVICES		= 0x000008; /* Look at the contents of devices */
const MAGIC_MIME_TYPE		= 0x000010; /* Return only the MIME type */
const MAGIC_CONTINUE		= 0x000020; /* Return all matches */
const MAGIC_CHECK		= 0x000040; /* Print warnings to stderr */
const MAGIC_PRESERVE_ATIME	= 0x000080; /* Restore access time on exit */
const MAGIC_RAW			= 0x000100; /* Don't translate unprint chars */
const MAGIC_ERROR		= 0x000200; /* Handle ENOENT etc as real errors */
const MAGIC_MIME_ENCODING	= 0x000400; /* Return only the MIME encoding */
const MAGIC_MIME		= (MAGIC_MIME_TYPE|MAGIC_MIME_ENCODING);
const MAGIC_NO_CHECK_COMPRESS	= 0x001000; /* Don't check for compressed files */
const MAGIC_NO_CHECK_TAR	= 0x002000; /* Don't check for tar files */
const MAGIC_NO_CHECK_SOFT	= 0x004000; /* Don't check magic entries */
const MAGIC_NO_CHECK_APPTYPE	= 0x008000; /* Don't check application type */
const MAGIC_NO_CHECK_ELF	= 0x010000; /* Don't check for elf details */
const MAGIC_NO_CHECK_ASCII	= 0x020000; /* Don't check for ascii files */
const MAGIC_NO_CHECK_TOKENS	= 0x100000; /* Don't check ascii/tokens */

var magicfile = "/usr/lib/rpm/magic";

var GPSEE = require('rpmmg');

var magic = new GPSEE.Mg(magicfile, MAGIC_CHECK);
ack("typeof magic;", "object");
ack("magic instanceof GPSEE.Mg;", true);
ack("magic.debug = 1;", 1);
ack("magic.debug = 0;", 0);

var mime = new GPSEE.Mg(magicfile, MAGIC_CHECK|MAGIC_MIME);
var fn = '/bin/sh';

print(fn+": mime("+mime(fn)+") "+magic(fn));

delete mime;
delete magic;

if (loglvl) print("<-- Mg.js");
