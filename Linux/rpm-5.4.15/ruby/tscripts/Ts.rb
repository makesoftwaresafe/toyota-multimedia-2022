if ($loglvl)
  print("--> Ts.rb\n");
end

$ts = Ts.new
ack("$ts.instance_of?(Ts)", true)
ack("$ts.kind_of?(Ts)", true)
ack("$ts.class.to_s", "Ts")
ack("$ts.debug = 1", 1)
ack("$ts.debug = 0", 0)

ack("$ts.rootdir = '/path/to/rootdir'", '/path/to/rootdir')
ack("$ts.rootdir = '/'", '/')

ack("$ts.vsflags = 0x1234", 0x1234)
ack("$ts.vsflags = 0", 0)

ack("$ts.NVRA[37]", nil);

# $ts.methods.each {|x| puts x}
# puts $ts.methods

if ($loglvl)
  print("<-- Ts.rb\n");
end
