#ifndef SELECT_H
#define SELECT_H
/*=========================================================================*\
* Select implementation
* LuaSocket toolkit
*
* Each object that can be passed to the select function has to export 
* method getfd() which returns the descriptor to be passed to the
* underlying select function. Another method, dirty(), should return 
* true if there is data ready for reading (required for buffered input).
*
* RCS ID: $Id: select.h,v 1.1 2008/10/24 22:58:40 jbj Exp $
\*=========================================================================*/

int select_open(lua_State *L);

#endif /* SELECT_H */
