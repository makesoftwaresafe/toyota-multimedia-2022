--
-- Copyright © 2003-2007 The Kepler Project.
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the "Software"),
-- to deal in the Software without restriction, including without limitation
-- the rights to use, copy, modify, merge, publish, distribute, sublicense,
-- and/or sell copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included
-- in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
-- THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.
--
-- $Id: lom.lua,v 1.1 2008/10/24 22:25:45 jbj Exp $

require "lxp"

local tinsert, tremove, getn = table.insert, table.remove, table.getn
local assert, type, print = assert, type, print
local lxp = lxp

module ("lxp.lom")

local function starttag (p, tag, attr)
  local stack = p:getcallbacks().stack
  local newelement = {tag = tag, attr = attr}
  tinsert(stack, newelement)
end

local function endtag (p, tag)
  local stack = p:getcallbacks().stack
  local element = tremove(stack)
  assert(element.tag == tag)
  local level = getn(stack)
  tinsert(stack[level], element)
end

local function text (p, txt)
  local stack = p:getcallbacks().stack
  local element = stack[getn(stack)]
  local n = getn(element)
  if type(element[n]) == "string" then
    element[n] = element[n] .. txt
  else
    tinsert(element, txt)
  end
end

function  parse (o)
  local c = { StartElement = starttag,
              EndElement = endtag,
              CharacterData = text,
              _nonstrict = true,
              stack = {{}}
            }
  local p = lxp.new(c)
  local status, err
  if type(o) == "string" then
    status, err = p:parse(o)
    if not status then return nil, err end
  else
    for l in o do
      status, err = p:parse(l)
      if not status then return nil, err end
    end
  end
  status, err = p:parse()
  if not status then return nil, err end
  p:close()
  return c.stack[1][1]
end

