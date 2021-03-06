/* HTML parser for Wget.
   Copyright (C) 1998, 2000, 2003 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

/* The only entry point to this module is map_html_tags(), which see.  */

/* TODO:

   - Allow hooks for callers to process contents outside tags.  This
     is needed to implement handling <style> and <script>.  The
     taginfo structure already carries the information about where the
     tags are, but this is not enough, because one would also want to
     skip the comments.  (The funny thing is that for <style> and
     <script> you *don't* want to skip comments!)

   - Create a test suite for regression testing. */

/* HISTORY:

   This is the third HTML parser written for Wget.  The first one was
   written some time during the Geturl 1.0 beta cycle, and was very
   inefficient and buggy.  It also contained some very complex code to
   remember a list of parser states, because it was supposed to be
   reentrant.

   The second HTML parser was written for Wget 1.4 (the first version
   by the name `Wget'), and was a complete rewrite.  Although the new
   parser behaved much better and made no claims of reentrancy, it
   still shared many of the fundamental flaws of the old version -- it
   only regarded HTML in terms tag-attribute pairs, where the
   attribute's value was a URL to be returned.  Any other property of
   HTML, such as <base href=...>, or strange way to specify a URL,
   such as <meta http-equiv=Refresh content="0; URL=..."> had to be
   crudely hacked in -- and the caller had to be aware of these hacks.
   Like its predecessor, this parser did not support HTML comments.

   After Wget 1.5.1 was released, I set out to write a third HTML
   parser.  The objectives of the new parser were to: (1) provide a
   clean way to analyze HTML lexically, (2) separate interpretation of
   the markup from the parsing process, (3) be as correct as possible,
   e.g. correctly skipping comments and other SGML declarations, (4)
   understand the most common errors in markup and skip them or be
   relaxed towrds them, and (5) be reasonably efficient (no regexps,
   minimum copying and minimum or no heap allocation).

   I believe this parser meets all of the above goals.  It is
   reasonably well structured, and could be relatively easily
   separated from Wget and used elsewhere.  While some of its
   intrinsic properties limit its value as a general-purpose HTML
   parser, I believe that, with minimum modifications, it could serve
   as a backend for one.

   Due to time and other constraints, this parser was not integrated
   into Wget until the version 1.7. */

/* DESCRIPTION:

   The single entry point of this parser is map_html_tags(), which
   works by calling a function you specify for each tag.  The function
   gets called with the pointer to a structure describing the tag and
   its attributes.  */

/* To test as standalone, compile with `-DSTANDALONE -I.'.  You'll
   still need Wget headers to compile.  */

#include "system.h"

#include <assert.h>

/* XXX uncouple from wget.h baggage. */

#ifndef PARAMS
# ifdef PROTOTYPES
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

/* Copy the data delimited with BEG and END to alloca-allocated
   storage, and zero-terminate it.  Arguments are evaluated only once,
   in the order BEG, END, PLACE.  */
#define BOUNDED_TO_ALLOCA(beg, end, place) do {	\
  const char *BTA_beg = (beg);			\
  int BTA_len = (end) - BTA_beg;		\
  char **BTA_dest = &(place);			\
  *BTA_dest = alloca (BTA_len + 1);		\
  memcpy (*BTA_dest, BTA_beg, BTA_len);		\
  (*BTA_dest)[BTA_len] = '\0';			\
} while (0)

/* Convert an ASCII hex digit to the corresponding number between 0
   and 15.  X should be a hexadecimal digit that satisfies isxdigit;
   otherwise, the result is undefined.  */
#define XDIGIT_TO_NUM(x) ((x) < 'A' ? (x) - '0' : TOUPPER (x) - 'A' + 10)

/* Returns the number of elements in an array with fixed
   initialization.  For example:

   static char a[] = "foo";     -- countof(a) == 4 (for terminating \0)

   int a[5] = {1, 2};           -- countof(a) == 5

   char *a[] = {                -- countof(a) == 3
     "foo", "bar", "baz"
   }; */
#define countof(array) (sizeof (array) / sizeof (*(array)))

#include "html-parse.h"

#ifdef STANDALONE
# undef xmalloc
# undef xrealloc
# undef xfree
# define xmalloc malloc
# define xrealloc realloc
# define xfree free

# undef ISSPACE
# undef ISDIGIT
# undef ISXDIGIT
# undef ISALPHA
# undef ISALNUM
# undef TOLOWER
# undef TOUPPER

# define ISSPACE(x) isspace (x)
# define ISDIGIT(x) isdigit (x)
# define ISXDIGIT(x) isxdigit (x)
# define ISALPHA(x) isalpha (x)
# define ISALNUM(x) isalnum (x)
# define TOLOWER(x) tolower (x)
# define TOUPPER(x) toupper (x)

struct hash_table {
  int dummy;
};
static void *
hash_table_get (const struct hash_table *ht, void *ptr)
{
  return ptr;
}
#else  /* not STANDALONE */
# include "hash.h"
#endif

/* Pool support.  A pool is a resizable chunk of memory.  It is first
   allocated on the stack, and moved to the heap if it needs to be
   larger than originally expected.  map_html_tags() uses it to store
   the zero-terminated names and values of tags and attributes.

   Thus taginfo->name, and attr->name and attr->value for each
   attribute, do not point into separately allocated areas, but into
   different parts of the pool, separated only by terminating zeros.
   This ensures minimum amount of allocation and, for most tags, no
   allocation because the entire pool is kept on the stack.  */

struct pool {
  char *contents;		/* pointer to the contents. */
  int size;			/* size of the pool. */
  int tail;			/* next available position index. */
  int resized;			/* whether the pool has been resized
				   using malloc. */

  char *orig_contents;		/* original pool contents, usually
                                   stack-allocated.  used by POOL_FREE
                                   to restore the pool to the initial
                                   state. */
  int orig_size;
};

/* Initialize the pool to hold INITIAL_SIZE bytes of storage. */

#define POOL_INIT(p, initial_storage, initial_size) do {	\
  struct pool *P = (p);						\
  P->contents = (initial_storage);				\
  P->size = (initial_size);					\
  P->tail = 0;							\
  P->resized = 0;						\
  P->orig_contents = P->contents;				\
  P->orig_size = P->size;					\
} while (0)

/* Grow the pool to accommodate at least SIZE new bytes.  If the pool
   already has room to accommodate SIZE bytes of data, this is a no-op.  */

#define POOL_GROW(p, increase)					\
  GROW_ARRAY ((p)->contents, (p)->size, (p)->tail + (increase),	\
	      (p)->resized, char)

/* Append text in the range [beg, end) to POOL.  No zero-termination
   is done.  */

#define POOL_APPEND(p, beg, end) do {			\
  const char *PA_beg = (beg);				\
  int PA_size = (end) - PA_beg;				\
  POOL_GROW (p, PA_size);				\
  memcpy ((p)->contents + (p)->tail, PA_beg, PA_size);	\
  (p)->tail += PA_size;					\
} while (0)

/* Append one character to the pool.  Can be used to zero-terminate
   pool strings.  */

#define POOL_APPEND_CHR(p, ch) do {		\
  char PAC_char = (ch);				\
  POOL_GROW (p, 1);				\
  (p)->contents[(p)->tail++] = PAC_char;	\
} while (0)

/* Forget old pool contents.  The allocated memory is not freed. */
#define POOL_REWIND(p) (p)->tail = 0

/* Free heap-allocated memory for contents of POOL.  This calls
   xfree() if the memory was allocated through malloc.  It also
   restores `contents' and `size' to their original, pre-malloc
   values.  That way after POOL_FREE, the pool is fully usable, just
   as if it were freshly initialized with POOL_INIT.  */

#define POOL_FREE(p) do {			\
  struct pool *P = p;				\
  if (P->resized)				\
    xfree (P->contents);			\
  P->contents = P->orig_contents;		\
  P->size = P->orig_size;			\
  P->tail = 0;					\
  P->resized = 0;				\
} while (0)

/* Used for small stack-allocated memory chunks that might grow.  Like
   DO_REALLOC, this macro grows BASEVAR as necessary to take
   NEEDED_SIZE items of TYPE.

   The difference is that on the first resize, it will use
   malloc+memcpy rather than realloc.  That way you can stack-allocate
   the initial chunk, and only resort to heap allocation if you
   stumble upon large data.

   After the first resize, subsequent ones are performed with realloc,
   just like DO_REALLOC.  */

#define GROW_ARRAY(basevar, sizevar, needed_size, resized, type) do {		\
  long ga_needed_size = (needed_size);						\
  long ga_newsize = (sizevar);							\
  while (ga_newsize < ga_needed_size)						\
    ga_newsize <<= 1;								\
  if (ga_newsize != (sizevar))							\
    {										\
      if (resized)								\
	basevar = (type *)xrealloc (basevar, ga_newsize * sizeof (type));	\
      else									\
	{									\
	  void *ga_new = xmalloc (ga_newsize * sizeof (type));			\
	  memcpy (ga_new, basevar, (sizevar) * sizeof (type));			\
	  (basevar) = ga_new;							\
	  resized = 1;								\
	}									\
      (sizevar) = ga_newsize;							\
    }										\
} while (0)

#define AP_DOWNCASE		1
#define AP_PROCESS_ENTITIES	2
#define AP_TRIM_BLANKS		4

/* Copy the text in the range [BEG, END) to POOL, optionally
   performing operations specified by FLAGS.  FLAGS may be any
   combination of AP_DOWNCASE, AP_PROCESS_ENTITIES and AP_TRIM_BLANKS
   with the following meaning:

   * AP_DOWNCASE -- downcase all the letters;

   * AP_PROCESS_ENTITIES -- process the SGML entities and write out
   the decoded string.  Recognized entities are &lt, &gt, &amp, &quot,
   &nbsp and the numerical entities.

   * AP_TRIM_BLANKS -- ignore blanks at the beginning and at the end
   of text.  */

static void
convert_and_copy (struct pool *pool, const char *beg, const char *end, int flags)
{
  int old_tail = pool->tail;
  int size;

  /* First, skip blanks if required.  We must do this before entities
     are processed, so that blanks can still be inserted as, for
     instance, `&#32;'.  */
  if (flags & AP_TRIM_BLANKS)
    {
      while (beg < end && ISSPACE (*beg))
	++beg;
      while (end > beg && ISSPACE (end[-1]))
	--end;
    }
  size = end - beg;

  if (flags & AP_PROCESS_ENTITIES)
    {
      /* Grow the pool, then copy the text to the pool character by
	 character, processing the encountered entities as we go
	 along.

	 It's safe (and necessary) to grow the pool in advance because
	 processing the entities can only *shorten* the string, it can
	 never lengthen it.  */
      const char *from = beg;
      char *to;

      POOL_GROW (pool, end - beg);
      to = pool->contents + pool->tail;

      while (from < end)
	{
	  if (*from != '&')
	    *to++ = *from++;
	  else
	    {
	      const char *save = from;
	      int remain;

	      if (++from == end)
		goto lose;
	      remain = end - from;

	      /* Process numeric entities "&#DDD;" and "&#xHH;".  */
	      if (*from == '#')
		{
		  int numeric = 0, digits = 0;
		  ++from;
		  if (*from == 'x')
		    {
		      ++from;
		      for (; from < end && ISXDIGIT (*from); from++, digits++)
			numeric = (numeric << 4) + XDIGIT_TO_NUM (*from);
		    }
		  else
		    {
		      for (; from < end && ISDIGIT (*from); from++, digits++)
			numeric = (numeric * 10) + (*from - '0');
		    }
		  if (!digits)
		    goto lose;
		  numeric &= 0xff;
		  *to++ = numeric;
		}
#define FROB(x) (remain >= (sizeof (x) - 1)			\
		 && 0 == memcmp (from, x, sizeof (x) - 1)	\
		 && (*(from + sizeof (x) - 1) == ';'		\
		     || remain == sizeof (x) - 1		\
		     || !ISALNUM (*(from + sizeof (x) - 1))))
	      else if (FROB ("lt"))
		*to++ = '<', from += 2;
	      else if (FROB ("gt"))
		*to++ = '>', from += 2;
	      else if (FROB ("amp"))
		*to++ = '&', from += 3;
	      else if (FROB ("quot"))
		*to++ = '\"', from += 4;
	      /* We don't implement the proposed "Added Latin 1"
		 entities (except for nbsp), because it is unnecessary
		 in the context of Wget, and would require hashing to
		 work efficiently.  */
	      else if (FROB ("nbsp"))
		*to++ = 160, from += 4;
	      else
		goto lose;
#undef FROB
	      /* If the entity was followed by `;', we step over the
		 `;'.  Otherwise, it was followed by either a
		 non-alphanumeric or EOB, in which case we do nothing.	*/
	      if (from < end && *from == ';')
		++from;
	      continue;

	    lose:
	      /* This was not an entity after all.  Back out.  */
	      from = save;
	      *to++ = *from++;
	    }
	}
      /* Verify that we haven't exceeded the original size.  (It
	 shouldn't happen, hence the assert.)  */
      assert (to - (pool->contents + pool->tail) <= end - beg);

      /* Make POOL's tail point to the position following the string
	 we've written.  */
      pool->tail = to - pool->contents;
      POOL_APPEND_CHR (pool, '\0');
    }
  else
    {
      /* Just copy the text to the pool.  */
      POOL_APPEND (pool, beg, end);
      POOL_APPEND_CHR (pool, '\0');
    }

  if (flags & AP_DOWNCASE)
    {
      char *p = pool->contents + old_tail;
      for (; *p; p++)
	*p = TOLOWER (*p);
    }
}

/* Originally we used to adhere to rfc 1866 here, and allowed only
   letters, digits, periods, and hyphens as names (of tags or
   attributes).  However, this broke too many pages which used
   proprietary or strange attributes, e.g. <img src="a.gif"
   v:shapes="whatever">.

   So now we allow any character except:
     * whitespace
     * 8-bit and control chars
     * characters that clearly cannot be part of name:
       '=', '>', '/'.

   This only affects attribute and tag names; attribute values allow
   an even greater variety of characters.  */

#define NAME_CHAR_P(x) ((x) > 32 && (x) < 127				\
			&& (x) != '=' && (x) != '>' && (x) != '/')

#ifdef STANDALONE
static int comment_backout_count;
#endif

/* Advance over an SGML declaration, such as <!DOCTYPE ...>.  In
   strict comments mode, this is used for skipping over comments as
   well.

   To recap: any SGML declaration may have comments associated with
   it, e.g.
       <!MY-DECL -- isn't this fun? -- foo bar>

   An HTML comment is merely an empty declaration (<!>) with a comment
   attached, like this:
       <!-- some stuff here -->

   Several comments may be embedded in one comment declaration:
       <!-- have -- -- fun -->

   Whitespace is allowed between and after the comments, but not
   before the first comment.  Additionally, this function attempts to
   handle double quotes in SGML declarations correctly.  */

static const char *
advance_declaration (const char *beg, const char *end)
{
  const char *p = beg;
  char quote_char = '\0';	/* shut up, gcc! */
  char ch;

  enum {
    AC_S_DONE,
    AC_S_BACKOUT,
    AC_S_BANG,
    AC_S_DEFAULT,
    AC_S_DCLNAME,
    AC_S_DASH1,
    AC_S_DASH2,
    AC_S_COMMENT,
    AC_S_DASH3,
    AC_S_DASH4,
    AC_S_QUOTE1,
    AC_S_IN_QUOTE,
    AC_S_QUOTE2
  } state = AC_S_BANG;

  if (beg == end)
    return beg;
  ch = *p++;

  /* It looked like a good idea to write this as a state machine, but
     now I wonder...  */

  while (state != AC_S_DONE && state != AC_S_BACKOUT)
    {
      if (p == end)
	state = AC_S_BACKOUT;
      switch (state)
	{
	case AC_S_DONE:
	case AC_S_BACKOUT:
	  break;
	case AC_S_BANG:
	  if (ch == '!')
	    {
	      ch = *p++;
	      state = AC_S_DEFAULT;
	    }
	  else
	    state = AC_S_BACKOUT;
	  break;
	case AC_S_DEFAULT:
	  switch (ch)
	    {
	    case '-':
	      state = AC_S_DASH1;
	      break;
	    case ' ':
	    case '\t':
	    case '\r':
	    case '\n':
	      ch = *p++;
	      break;
	    case '>':
	      state = AC_S_DONE;
	      break;
	    case '\'':
	    case '\"':
	      state = AC_S_QUOTE1;
	      break;
	    default:
	      if (NAME_CHAR_P (ch))
		state = AC_S_DCLNAME;
	      else
		state = AC_S_BACKOUT;
	      break;
	    }
	  break;
	case AC_S_DCLNAME:
	  if (ch == '-')
	    state = AC_S_DASH1;
	  else if (NAME_CHAR_P (ch))
	    ch = *p++;
	  else
	    state = AC_S_DEFAULT;
	  break;
	case AC_S_QUOTE1:
	  /* We must use 0x22 because broken assert macros choke on
	     '"' and '\"'.  */
	  assert (ch == '\'' || ch == 0x22);
	  quote_char = ch;	/* cheating -- I really don't feel like
				   introducing more different states for
				   different quote characters. */
	  ch = *p++;
	  state = AC_S_IN_QUOTE;
	  break;
	case AC_S_IN_QUOTE:
	  if (ch == quote_char)
	    state = AC_S_QUOTE2;
	  else
	    ch = *p++;
	  break;
	case AC_S_QUOTE2:
	  assert (ch == quote_char);
	  ch = *p++;
	  state = AC_S_DEFAULT;
	  break;
	case AC_S_DASH1:
	  assert (ch == '-');
	  ch = *p++;
	  state = AC_S_DASH2;
	  break;
	case AC_S_DASH2:
	  switch (ch)
	    {
	    case '-':
	      ch = *p++;
	      state = AC_S_COMMENT;
	      break;
	    default:
	      state = AC_S_BACKOUT;
	    }
	  break;
	case AC_S_COMMENT:
	  switch (ch)
	    {
	    case '-':
	      state = AC_S_DASH3;
	      break;
	    default:
	      ch = *p++;
	      break;
	    }
	  break;
	case AC_S_DASH3:
	  assert (ch == '-');
	  ch = *p++;
	  state = AC_S_DASH4;
	  break;
	case AC_S_DASH4:
	  switch (ch)
	    {
	    case '-':
	      ch = *p++;
	      state = AC_S_DEFAULT;
	      break;
	    default:
	      state = AC_S_COMMENT;
	      break;
	    }
	  break;
	}
    }

  if (state == AC_S_BACKOUT)
    {
#ifdef STANDALONE
      ++comment_backout_count;
#endif
      return beg + 1;
    }
  return p;
}

/* Find the first occurrence of the substring "-->" in [BEG, END) and
   return the pointer to the character after the substring.  If the
   substring is not found, return NULL.  */

static const char *
find_comment_end (const char *beg, const char *end)
{
  /* Open-coded Boyer-Moore search for "-->".  Examine the third char;
     if it's not '>' or '-', advance by three characters.  Otherwise,
     look at the preceding characters and try to find a match.  */

  const char *p = beg - 1;

  while ((p += 3) < end)
    switch (p[0])
      {
      case '>':
	if (p[-1] == '-' && p[-2] == '-')
	  return p + 1;
	break;
      case '-':
      at_dash:
	if (p[-1] == '-')
	  {
	  at_dash_dash:
	    if (++p == end) return NULL;
	    switch (p[0])
	      {
	      case '>': return p + 1;
	      case '-': goto at_dash_dash;
	      }
	  }
	else
	  {
	    if ((p += 2) >= end) return NULL;
	    switch (p[0])
	      {
	      case '>':
		if (p[-1] == '-')
		  return p + 1;
		break;
	      case '-':
		goto at_dash;
	      }
	  }
      }
  return NULL;
}

/* Return non-zero of the string inside [b, e) are present in hash
   table HT.  */

static int
name_allowed (const struct hash_table *ht, const char *b, const char *e)
{
  char *copy;
  if (!ht)
    return 1;
  BOUNDED_TO_ALLOCA (b, e, copy);
  return hash_table_get (ht, copy) != NULL;
}

/* Advance P (a char pointer), with the explicit intent of being able
   to read the next character.  If this is not possible, go to finish.  */

#define ADVANCE(p) do {				\
  ++p;						\
  if (p >= end)					\
    goto finish;				\
} while (0)

/* Skip whitespace, if any. */

#define SKIP_WS(p) do {				\
  while (ISSPACE (*p)) {			\
    ADVANCE (p);				\
  }						\
} while (0)

/* Skip non-whitespace, if any. */

#define SKIP_NON_WS(p) do {			\
  while (!ISSPACE (*p)) {			\
    ADVANCE (p);				\
  }						\
} while (0)

#ifdef STANDALONE
static int tag_backout_count;
#endif

/* Map MAPFUN over HTML tags in TEXT, which is SIZE characters long.
   MAPFUN will be called with two arguments: pointer to an initialized
   struct taginfo, and MAPARG.

   ALLOWED_TAG_NAMES should be a NULL-terminated array of tag names to
   be processed by this function.  If it is NULL, all the tags are
   allowed.  The same goes for attributes and ALLOWED_ATTRIBUTE_NAMES.

   (Obviously, the caller can filter out unwanted tags and attributes
   just as well, but this is just an optimization designed to avoid
   unnecessary copying for tags/attributes which the caller doesn't
   want to know about.  These lists are searched linearly; therefore,
   if you're interested in a large number of tags or attributes, you'd
   better set these to NULL and filter them out yourself with a
   hashing process most appropriate for your application.)  */

void
map_html_tags (const char *text, int size,
	       void (*mapfun) (struct taginfo *, void *), void *maparg,
	       int flags,
	       const struct hash_table *allowed_tags,
	       const struct hash_table *allowed_attributes)
{
  /* storage for strings passed to MAPFUN callback; if 256 bytes is
     too little, POOL_APPEND allocates more with malloc. */
  char pool_initial_storage[256];
  struct pool pool;

  const char *p = text;
  const char *end = text + size;

  struct attr_pair attr_pair_initial_storage[8];
  int attr_pair_size = countof (attr_pair_initial_storage);
  int attr_pair_resized = 0;
  struct attr_pair *pairs = attr_pair_initial_storage;

  if (!size)
    return;

  POOL_INIT (&pool, pool_initial_storage, countof (pool_initial_storage));

  {
    int nattrs, end_tag;
    const char *tag_name_begin, *tag_name_end;
    const char *tag_start_position;
    int uninteresting_tag;

  look_for_tag:
    POOL_REWIND (&pool);

    nattrs = 0;
    end_tag = 0;

    /* Find beginning of tag.  We use memchr() instead of the usual
       looping with ADVANCE() for speed. */
    p = memchr (p, '<', end - p);
    if (!p)
      goto finish;

    tag_start_position = p;
    ADVANCE (p);

    /* Establish the type of the tag (start-tag, end-tag or
       declaration).  */
    if (*p == '!')
      {
	if (!(flags & MHT_STRICT_COMMENTS)
	    && p < end + 3 && p[1] == '-' && p[2] == '-')
	  {
	    /* If strict comments are not enforced and if we know
	       we're looking at a comment, simply look for the
	       terminating "-->".  Non-strict is the default because
	       it works in other browsers and most HTML writers can't
	       be bothered with getting the comments right.  */
	    const char *comment_end = find_comment_end (p + 3, end);
	    if (comment_end)
	      p = comment_end;
	  }
	else
	  {
	    /* Either in strict comment mode or looking at a non-empty
	       declaration.  Real declarations are much less likely to
	       be misused the way comments are, so advance over them
	       properly regardless of strictness.  */
	    p = advance_declaration (p, end);
	  }
	if (p == end)
	  goto finish;
	goto look_for_tag;
      }
    else if (*p == '/')
      {
	end_tag = 1;
	ADVANCE (p);
      }
    tag_name_begin = p;
    while (NAME_CHAR_P (*p))
      ADVANCE (p);
    if (p == tag_name_begin)
      goto look_for_tag;
    tag_name_end = p;
    SKIP_WS (p);
    if (end_tag && *p != '>')
      goto backout_tag;

    if (!name_allowed (allowed_tags, tag_name_begin, tag_name_end))
      /* We can't just say "goto look_for_tag" here because we need
         the loop below to properly advance over the tag's attributes.  */
      uninteresting_tag = 1;
    else
      {
	uninteresting_tag = 0;
	convert_and_copy (&pool, tag_name_begin, tag_name_end, AP_DOWNCASE);
      }

    /* Find the attributes. */
    while (1)
      {
	const char *attr_name_begin, *attr_name_end;
	const char *attr_value_begin, *attr_value_end;
	const char *attr_raw_value_begin, *attr_raw_value_end;
	int operation = AP_DOWNCASE; /* stupid compiler. */

	SKIP_WS (p);

	if (*p == '/')
	  {
	    /* A slash at this point means the tag is about to be
	       closed.  This is legal in XML and has been popularized
	       in HTML via XHTML.  */
	    /* <foo a=b c=d /> */
	    /*              ^  */
	    ADVANCE (p);
	    SKIP_WS (p);
	    if (*p != '>')
	      goto backout_tag;
	  }

	/* Check for end of tag definition. */
	if (*p == '>')
	  break;

	/* Establish bounds of attribute name. */
	attr_name_begin = p;	/* <foo bar ...> */
				/*      ^        */
	while (NAME_CHAR_P (*p))
	  ADVANCE (p);
	attr_name_end = p;	/* <foo bar ...> */
				/*         ^     */
	if (attr_name_begin == attr_name_end)
	  goto backout_tag;

	/* Establish bounds of attribute value. */
	SKIP_WS (p);
	if (NAME_CHAR_P (*p) || *p == '/' || *p == '>')
	  {
	    /* Minimized attribute syntax allows `=' to be omitted.
               For example, <UL COMPACT> is a valid shorthand for <UL
               COMPACT="compact">.  Even if such attributes are not
               useful to Wget, we need to support them, so that the
               tags containing them can be parsed correctly. */
	    attr_raw_value_begin = attr_value_begin = attr_name_begin;
	    attr_raw_value_end = attr_value_end = attr_name_end;
	  }
	else if (*p == '=')
	  {
	    ADVANCE (p);
	    SKIP_WS (p);
	    if (*p == '\"' || *p == '\'')
	      {
		int newline_seen = 0;
		char quote_char = *p;
		attr_raw_value_begin = p;
		ADVANCE (p);
		attr_value_begin = p; /* <foo bar="baz"> */
				      /*           ^     */
		while (*p != quote_char)
		  {
		    if (!newline_seen && *p == '\n')
		      {
			/* If a newline is seen within the quotes, it
			   is most likely that someone forgot to close
			   the quote.  In that case, we back out to
			   the value beginning, and terminate the tag
			   at either `>' or the delimiter, whichever
			   comes first.  Such a tag terminated at `>'
			   is discarded.  */
			p = attr_value_begin;
			newline_seen = 1;
			continue;
		      }
		    else if (newline_seen && *p == '>')
		      break;
		    ADVANCE (p);
		  }
		attr_value_end = p; /* <foo bar="baz"> */
				    /*              ^  */
		if (*p == quote_char)
		  ADVANCE (p);
		else
		  goto look_for_tag;
		attr_raw_value_end = p;	/* <foo bar="baz"> */
					/*               ^ */
		operation = AP_PROCESS_ENTITIES;
		if (flags & MHT_TRIM_VALUES)
		  operation |= AP_TRIM_BLANKS;
	      }
	    else
	      {
		attr_value_begin = p; /* <foo bar=baz> */
				      /*          ^    */
		/* According to SGML, a name token should consist only
		   of alphanumerics, . and -.  However, this is often
		   violated by, for instance, `%' in `width=75%'.
		   We'll be liberal and allow just about anything as
		   an attribute value.  */
		while (!ISSPACE (*p) && *p != '>')
		  ADVANCE (p);
		attr_value_end = p; /* <foo bar=baz qux=quix> */
				    /*             ^          */
		if (attr_value_begin == attr_value_end)
		  /* <foo bar=> */
		  /*          ^ */
		  goto backout_tag;
		attr_raw_value_begin = attr_value_begin;
		attr_raw_value_end = attr_value_end;
		operation = AP_PROCESS_ENTITIES;
	      }
	  }
	else
	  {
	    /* We skipped the whitespace and found something that is
	       neither `=' nor the beginning of the next attribute's
	       name.  Back out.  */
	    goto backout_tag;	/* <foo bar [... */
				/*          ^    */
	  }

	/* If we're not interested in the tag, don't bother with any
           of the attributes.  */
	if (uninteresting_tag)
	  continue;

	/* If we aren't interested in the attribute, skip it.  We
           cannot do this test any sooner, because our text pointer
           needs to correctly advance over the attribute.  */
	if (!name_allowed (allowed_attributes, attr_name_begin, attr_name_end))
	  continue;

	GROW_ARRAY (pairs, attr_pair_size, nattrs + 1, attr_pair_resized,
		    struct attr_pair);

	pairs[nattrs].name_pool_index = pool.tail;
	convert_and_copy (&pool, attr_name_begin, attr_name_end, AP_DOWNCASE);

	pairs[nattrs].value_pool_index = pool.tail;
	convert_and_copy (&pool, attr_value_begin, attr_value_end, operation);
	pairs[nattrs].value_raw_beginning = attr_raw_value_begin;
	pairs[nattrs].value_raw_size = (attr_raw_value_end
					- attr_raw_value_begin);
	++nattrs;
      }

    if (uninteresting_tag)
      {
	ADVANCE (p);
	goto look_for_tag;
      }

    /* By now, we have a valid tag with a name and zero or more
       attributes.  Fill in the data and call the mapper function.  */
    {
      int i;
      struct taginfo taginfo;

      taginfo.name      = pool.contents;
      taginfo.end_tag_p = end_tag;
      taginfo.nattrs    = nattrs;
      /* We fill in the char pointers only now, when pool can no
	 longer get realloc'ed.  If we did that above, we could get
	 hosed by reallocation.  Obviously, after this point, the pool
	 may no longer be grown.  */
      for (i = 0; i < nattrs; i++)
	{
	  pairs[i].name = pool.contents + pairs[i].name_pool_index;
	  pairs[i].value = pool.contents + pairs[i].value_pool_index;
	}
      taginfo.attrs = pairs;
      taginfo.start_position = tag_start_position;
      taginfo.end_position   = p + 1;
      /* Ta-dam! */
      (*mapfun) (&taginfo, maparg);
      ADVANCE (p);
    }
    goto look_for_tag;

  backout_tag:
#ifdef STANDALONE
    ++tag_backout_count;
#endif
    /* The tag wasn't really a tag.  Treat its contents as ordinary
       data characters. */
    p = tag_start_position + 1;
    goto look_for_tag;
  }

 finish:
  POOL_FREE (&pool);
  if (attr_pair_resized)
    xfree (pairs);
}

#undef ADVANCE
#undef SKIP_WS
#undef SKIP_NON_WS

#ifdef STANDALONE

#include "rpmio.h"

extern int _rpmio_debug;
extern int _dav_debug;
extern int _ftp_debug;

#if 0
#define	HTMLPATH	"http://download.fedora.redhat.com/pub/fedora/linux/core/3/i386/os/Fedora/RPMS/"
#define HTMLPATH	"http://localhost/rawhide/test/"
#else
#define HTMLPATH	"http://localhost/rawhide/"
#endif
static const char * htmlpath = HTMLPATH;

static void
test_mapper (struct taginfo *taginfo, void *arg)
{
  int i;

  printf ("%s%s", taginfo->end_tag_p ? "/" : "", taginfo->name);
  for (i = 0; i < taginfo->nattrs; i++)
    printf (" %s=%s", taginfo->attrs[i].name, taginfo->attrs[i].value);
  putchar ('\n');
  ++*(int *)arg;
}

int main ()
{
  int size = 256;
  char *x = (char *)xmalloc (size);
  int length = 0;
  int read_count;
  int tag_counter = 0;
  int flags = MHT_TRIM_VALUES;
  struct hash_table *interesting_tags = (struct hash_table *)1;
  struct hash_table *interesting_attributes = (struct hash_table *)1;
  FD_t fd;

_rpmio_debug = 0;
_dav_debug = 0;
  fd = Fopen(htmlpath, "r");
  while ((read_count = Fread (x + length, 1, size - length, fd)))
    {
      if (read_count <= 0)
	break;
      length += read_count;
      size <<= 1;
      x = (char *)xrealloc (x, size);
    }
   (void) Fclose(fd);
   x[length] = '\0';
#if 0
fprintf(stderr, "============== %p[%d]\n%s\n", x, length, x);
#endif

  map_html_tags (x, length, test_mapper, &tag_counter,
	flags, interesting_tags, interesting_attributes);
  printf ("TAGS: %d\n", tag_counter);
  printf ("Tag backouts:     %d\n", tag_backout_count);
  printf ("Comment backouts: %d\n", comment_backout_count);
  return 0;
}
#endif /* STANDALONE */
