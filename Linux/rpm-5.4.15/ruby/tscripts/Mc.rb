if ($loglvl)
  print("--> Mc.rb\n");
end

$mc = Mc.new
ack("$mc.instance_of?(Mc)", true)
ack("$mc.kind_of?(Mc)", true)
ack("$mc.class.to_s", "Mc")
ack("$mc.debug = 1;", 1);
ack("$mc.debug = 0;", 0);

# ack('$mc.expand("%{_bindir}")', "/usr/bin");

# XXX noisy, adding pattern/index/level filters todo++
# nack('$mc.list()', null);
ack('$mc.add("foo bar")', true);
ack('$mc.expand("%{foo}")', "bar");
ack('$mc.del("foo")', true);
ack('$mc.expand("%{foo}")', "%{foo}");
# XXX noisy, adding pattern/index/level filters todo++
# ack('$mc.list()', false);

ack('$mc.expand("%{lua:print(\\"lua\\")}")',     "lua");

# FIXME: reloading rpm modules within embedded interpreter segfaults
# ack('$mc.expand("%{perl:print \\"perl\\"}")', "perl");
# ack('$mc.expand("%{python:print \\"python\\"}")', "python");
# FIXME: ruby can't load ruby
# ack('$mc.expand("%{ruby:puts \\"ruby\\"}")',     "ruby");
ack('$mc.expand("%{tcl:puts \\"tcl\\"}")',       "tcl");

# $mc = Mc.new("cli");
ack('$mc.list()', nil);
ack('$mc.add("foo bar")', true);
ack('$mc.list()', "%foo\tbar");
ack('$mc.expand("%{foo}")', "bar");
ack('$mc.del("foo")', true);
ack('$mc.list()', nil);
ack('$mc.expand("%{foo}")', "%{foo}");

# $mc = Mc.new("tscripts/macros");
ack('$mc.list()', nil);
ack('$mc.add("foo bar")', true);
ack('$mc.list()', "%foo\tbar");
ack('$mc.expand("%{foo}")', "bar");
ack('$mc.del("foo")', true);
ack('$mc.list()', nil);
ack('$mc.expand("%{foo}")', "%{foo}");

# $mc = Mc.new("");
ack('$mc.list()', nil);
ack('$mc.add("foo bar")', true);
ack('$mc.list()', "%foo\tbar");
ack('$mc.expand("%{foo}")', "bar");
ack('$mc.del("foo")', true);
ack('$mc.list()', nil);
ack('$mc.expand("%{foo}")', "%{foo}");

# FIXME: there's no internal code path error returns to force an error out.
# $mc = Mc.new("tscripts/nonexistent");

if ($loglvl)
  print("<-- Mc.rb\n");
end
