# GENIVI AudioManager Plugins
Plugin interface for the GENIVI Audio Manager

# Copyright and License
Copyright (C) 2012, GENIVI Alliance  
Copyright (C) 2012, BMW AG  
Copyright (C) 2017, Advanced Driver Information Technology  

This file is part of GENIVI Project AudioManager.

Author [Christian Linke](christian.linke@bmw.de) BMW 2011-2015  
Author [Jens Lorenz](jlorenz@de.adit-jv.com) Advanced Driver Information Technology 2015-2017

## Repositories
For further information see  
https://github.com/GENIVI/AudioManager

This repository holds sample plugins for the AudioManager Project and can be found here  
https://github.com/GENIVI/AudioManagerPlugins

## License
Contribution is done under MPL2.0 or MIT License, depending on the location of the code.

(C) Copyright
This Source Code Form is subject to the terms of the MIT Licence

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Documentation
A very detailed documentation is provided by doxygen. In order to use this, please compile the plugins with
```
cmake -DWITH_DOCUMENTATION=ON
make
```

The README is compiled into README.html with asciidoc

## Compile plug-ins
This section shows a list of available plug-ins with respective switches and their default values. Some of the plug-ins will deploy configuration files like the Generic Controller.
Those files will be deployed to CMAKE_INSTALL_PREFIX. In order to change the configuration files location following switch can be used
```
cmake -DCONFIG_PREFIX=/etc
```

**_Common API Plug-ins_**  
These example plug-ins works only in case WITH_CAPI_WRAPPER is activated by AudioManagerDaemon  
<tt>-- WITH_COMMAND_INTERFACE_CAPI	= OFF</tt>  
<tt>-- WITH_ROUTING_INTERFACE_CAPI	= OFF</tt>

**_DBus Plug-ins_**  
These plug-ins works only in case WITH_DBUS_WRAPPER is activated by AudioManagerDaemon  
<tt>-- WITH_COMMAND_INTERFACE_DBUS	= OFF</tt>  
<tt>-- WITH_ROUTING_INTERFACE_DBUS	= OFF</tt>

**_Test Plugins_**  
These plug-ins are needed in case WITH_TEST is activated by AudioManagerDaemon  
<tt>-- WITH_ROUTING_INTERFACE_ASYNC	= OFF</tt>  
<tt>-- WITH_TEST_CONTROLLER		= OFF</tt>

**_Generic Controller_**  
The Controller has a build dependency to libxml2.  
<tt>-- WITH_GENERIC_CONTROLLER		= OFF</tt>  

**_Routing Adapter for ALSA_**  
The Routing Adapter has a build dependency to libxml2.  
<tt>-- WITH_ROUTING_ADAPTER_ALSA	= OFF</tt>  

## Build targets
just install the plug-ins (only available if plug-ins are compiled)
```
sudo make plugins-install
```

just install the test for the plugins (only available if plugins are compiled)
```
sudo make plugin-test-install
```

## Tests
In order to build the unit tests please follow the instruction given on AudioManager repository. Unit tests are not supported by all plug-ins. More details in the CMake Files CMakeList.txt in the respective plugin directories.

## History
The git history can be found in the AudioManager repo. There all the commits of the Plugins until the split of the repos can be found.

