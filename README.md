# Alpaca Hub


## Code Style
I think I'm going to try and stick to Google's C++ guide: https://google.github.io/styleguide/cppguide.html

## Background / Intent of System
Just for those that are curious as to "why are you building this?"

I wanted to be able to setup my imaging system in a way that "makes sense to me." Years ago I started
a project that implemented image acquisition on top of PixInsight, but I abandoned it due to lack of
free time. Back then, I had imagined I would use PixInsight for as much of my astrophotography imaging
needs as possible. I _may_ revisit that some point in the future.

As far as what "makes sense to me" - I want to be able to do the following:
- More or less fully automate my image acquisition from the point that equipment is powered up, mount is polar aligned, etc...
- Be able to mix and match components from the standpoint of not everything relying on one computer
- treat my astronomy devices like little network attached systems with some ultimate dream of having queues / resilience during network interruptions...
- I want to have most if not all of my astro imaging gear working as
   members of a network of IoT devices
- I want to connect from NINA to my devices for automation and control
- I don't want to have any of my devices physically connected to my
  NINA instance
- In order to do this, I think the ASCOM Alpaca standard makes sense

## Design Thoughts

- I think there needs to be a server which implements a multi-device
  interface for alpaca
  - Or am I better off just implementing a single device per server model?
- There will need to be a generic concept of a device which probably
  is an interface?
- What is the pattern for initialization and then subsequent detection
  of new devices being connected?
- What is the pattern for another remote alpaca device?
  - I don't think I should handle this use case actually...
  - I think that this is a simple hub that can host one or more
    directly connected devices to the system it is running on
- Need to have device level locking / multi-threaded support

### General initialization

- program is run from command line with appropriate options
- scan for device(s)
- for each device found, should we start a worker thread?
- start web loop
  - for sync calls, does the web server block? Or is it only blocking
    on a per device case?
- if there are no devices does the service just terminate?
  - or should this be a state of "waiting for a device to be connected
    to USB or Serial or ?"
- need to implement transaction sequence for returning unique tx ids
  to connected client

### Handling web requests

- GET / PUT request comes in
- check path and invoke handler based on route
- does the loop just sleep a bit in the case of a long running task?
- when, if ever, does the loop block?
- I wonder if I should allocate worker threads for restinio based on
  the number of devices?
- what if there is more than one client making requests?
  - is this something where we spit out an error?
  - or is this a normal use case?
- I think I can leverage chained event handlers for things like the
  common handling of server transaction ids and client id / txid so
  that it is DRY for those cases
  - Here's the example from restinio:
  - https://github.com/Stiffstream/restinio/blob/29dc1b0f7bc133dd0d5ec5824e596e34b3d4abd0/dev/sample/chained_handlers/main.cpp
- Need to understand which are async calls and how to manage that
  state across requests
- Should I reject requests from a different ClientID that are not the
  same as the client id which initiated the connection?

### State management

- Need to have some sort of state management for things like
  transactions and the management of sequences
- Need to have a collection of devices easily accessed through a dictionary
  - telescopes
  - cameras
  - filter wheel(s)
  - auto-focuser


# Build notes


Generate makefiles
``` bash
cmake -B build src
```

To build with support for lsp assistance (generates `compile_commands.json`):
``` bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build src
```


## Download restinio to `src/3rd_party`
```
cd $QHY_ALPACA_DIR/3rd_party
wget https://github.com/Stiffstream/restinio/releases/download/v.0.7.2/restinio-0.7.2-full.tar.bz2
tar jxvf restinio-0.7.2-full.tar.bz2
```

## Download SPD log

``` bash
wget spdlog ...
```

### Notes and good docs

This looks like an attempt to implement the AM5 driver over Alpaca

https://github.com/YugnatD/ASCOM_Alpaca_ZWO_AM5


#### Download and install QHY SDK


# TODOs:
 - [ ] Create web page for project
 - [x] Write web server (restinio based)
 - [x] Implement QHY Camera driver (mono only)
 - [ ] Implement QHY Camera driver (color)
 - [x] Implement QHY filter wheel driver
 - [x] Implement Alpaca discovery
 - [ ] Implement focuser driver
 - [ ] Implement telescope mount driver
 - [ ] Implement better descriptions
 - [ ] Implement firmware version to data pulled
 - [ ] Implement support for log file as an option
 - [ ] Implement CLI help
 - [ ] Implement web setup screen for devices
 - [ ] Implement web screen for information
 - [x] Implement Alpaca Management API
 - [ ] Figure approach to test suite
 - [ ] Achieve conformance with ASCOM Conform Tool
 - [ ] Cleanup error messages to follow consistent format
 - [ ] Move device initializer code to be called on PUT connected
 - [ ] Need to rethink templates for PUT requests for multiple values
 - [ ] Check all pointers for null
 - [x] Need to fix crash when invalid action is called on valid device type
 - [ ] Need to figure out what the threading model will be for multiple devices
       I can do something like 1 thread per device or just add a thread pool the
       size of the device types. The disadvantage of a generic thread pool
       is that if multiple requests on a given device are taking a while, this
       can cause thread exhaustion. The disadvantage of a thread per device is
       that it adds a bit of complication to the code potentially.
 - [ ] I want to make the concept of device idx, uuid of a device, and
       devices being plugged in and unplugged as part of the lifecycle
       of adding / removing from the device_map. This might play into
       how I spawn threads...
 - [ ] Potentially refactor the registration of PUT routes to be similar to the
       GET routes so that I can remove the duplication of common paths
       and also set the device type based on the Device_T passed in
 - [ ] Implement persistent UUID for devices. I can probably use the
       device serial number I think
