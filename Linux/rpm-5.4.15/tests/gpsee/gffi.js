// from http://code.google.com/p/gpsee/wiki/GFFITutorial
const ffi    = require("gffi");
const _ctime = new ffi.CFunction(ffi.pointer, "ctime", ffi.pointer);
const _time  = new ffi.CFunction(ffi.time_t, "time", ffi.pointer);

function getTime()
{
  var now = new ffi.Memory(8);
  var ret;

  ret = _time.call(now);
  if (ret == -1)
    throw("failed to get current time from OS");

  ret = _ctime.call(now);
  if (ret == null)
    throw("failed to format current time as a string");

  return ret.asString();
}

print(getTime());

