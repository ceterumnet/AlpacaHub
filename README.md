# Alpaca Hub

<img src="docs/images/alpaca_hub_main_image.webp" width="500">

With a focus on astrophotography use cases, Alpaca Hub is an open
source ASCOM Alpaca server that provides structure and implementation
for running one or more devices as exposed and discoverable via the
Alpaca Protocol specified here -
https://www.ascom-standards.org/AlpacaDeveloper/Index.htm.

One of the goals of this project is to make sure that ASCOM Alpaca is
more generally available for people that want to leverage network
attached astronomy gear. I really think that the folks over at ASCOM
have done an amazing job of carrying the torch around interoperability
in Astronomy equipment for a long time now.

For example, my favorite astrophotography software is
[NINA](https://nighttime-imaging.eu/). NINA has excellent support for
ASCOM and also the Alpaca protocol which is actually the original
reason I decided to implement this!

## What is supported now?
At the moment, the list of hardware supported is not huge, but I have
written drivers for the hardware I have at home:
- QHY cameras (mono) and attached Filterwheels
- ZWO AM5/AM3 Mount
- Pegasus FocusCube 3

Alpaca Hub also implements the discovery protocol as well which is one
of my favorite parts of the Alpaca protocol.

Also, there is not a binary distribution at the moment, but that is on
the short list of things to get done.

However, it is not hard to build from scratch if you are so
inclined. See the build instructions later in this README for details.
Please reach out to me through an issue on Github if you have any issues
compiling the project.

Here are some screen shots of the web interface:

![Screenshot](docs/images/alpaca_hub_main_web_page.png)
![Screenshot](docs/images/alpaca_hub_telescope_page.png)

As the project matures, I definitely plan on ensuring that there is good
documentation. And I would absolutely appreciate any contributions towards
this.

## Shoulders of Giants

It is very important to me that I acknowledge the work that this has been
built on top of.

Specifically, the following are major projects / products that have
been crucial in building this tool:

- ASCOM
- INDI
- Indigo
- AlpacaPi
- PixInsight

## Background / Intent of System

Just for those that are curious as to "why are you building this?"

When I originally started this, I was simply mildly frustrated with
the lack of Alpaca implementations for the astronomy hardware I like
to use. So I said to myself "don't complain, just build what you need."

I wanted to be able to setup my imaging system in a way that "makes
sense to me." Years ago I started a project that implemented image
acquisition on top of PixInsight, but I abandoned it due to lack of
free time. Back then, I had imagined I would use PixInsight for as
much of my astrophotography imaging needs as possible. I _may_ revisit
that some point in the future, but I'm not sure it makes sense at this
point.

Also, a _big_ motivation for this project is scratching my itch to
write some code for astrophotography. There are more projects than
ever, so I can't exactly claim that this project will be better or
have any particular advantage. However, I do feel strongly about making
a system that is easy to extend. That is one of the main goals of this
project: **It should be very easy to add support for additional drivers**

I personally want to be able to do the following:
- More or less fully automate my image acquisition from the point that
  equipment is powered up, mount is polar aligned, etc... (this is not
  by itself a reason to make something like this...)
- Be able to mix and match components from the standpoint of not
  everything relying on one computer
- treat my astronomy devices like little network attached systems with
  some ultimate dream of having queues / resilience during network
  interruptions...
- I want to have most if not all of my astro imaging gear working as
   members of a network of IoT devices
- I want to connect from NINA to my devices for automation and control
- I don't want to have any of my devices physically connected to my
  NINA instance
- In order to do this, I think the ASCOM Alpaca standard makes sense

## Design and Implementation

### Code Style
I think I'm going to try and stick to Google's C++ guide:
https://google.github.io/styleguide/cppguide.html. However, I don't
necessarily think it needs to compulsively adhered to.

### Architecture

### Name ideas for the project

I don't necessarily know if "Alpaca Hub" is the right name.

- Alpacapus
- Starmada
- AlpacaBerry

### Other Alpaca devices
- What is the pattern for another remote alpaca device?
  - I don't think I should handle this use case actually...
  - I think that this is a simple hub that can host one or more
    directly connected devices to the system it is running on

## Decision Rationale

### Why C++?

I debated whether or not to implement this with a higher level language
such as Rust or C#.

Ultimately, given how far C++17 and beyond have come, I find that C++
is still one of the ultimate gold standards in terms of cross-platform
and high performance. Additionally, most of the vendor SDKs support C++
and so I hope that this makes it attractive for contributions.

Additionally, as I've been implementing this, I have found that
C++ has been a very productive language striking a nice balance of
productivity with tight control over areas of code when needed.

### Why ASCOM Alpaca?

I found that Alpaca really struck a nice balance in leveraging http
with a simple REST API vs a proprietary network protocol.

Additionally, I've found that the ASCOM project in general has been
very successful in becoming the de facto standard when it comes to
interoperability across different software vendors. For a long time
it was just Windows. I think that Alpaca was a stroke of brilliance
to allow the project to become ubiquitous across all platforms while
retaining so much of the value that has already been created through
the ASCOM interface definitions.

#### What about INDI / Indigo?

I'm blown away with the INDI and Indigo projects. I think they have
created something fairly amazing, and I strongly recommend using those
if your use cases warrant that.

# Build notes

## Supported Hardware / OS

- Raspberry Pi 5 (it should work fine on Raspberry Pi 4 as well...but
  needs to be tested)

Initially, I've only been focused on building and running Alpaca Hub
on the Raspberry Pi 5. That said, I've been fairly meticulous about
writing C++ code that _should_ be portable. That, combined with CMake,
there really should be no big challenge compiling and running on other
platforms.

## Dependencies

TODO: Need to do a fresh from scratch build and double check all requirements
and enumerate them here

1. Download and install QHY SDK
2. Checkout Alpaca Hub locally

Generate makefiles
``` bash
cmake -B build src
```

# TODOs and potential ideas:

## Potentially Cool / Random ideas
- Embed a lua interpreter
  - I like the idea of being able to have a cli console where I can interact
    with my gear. I've leveraged embedding lua in the past to support this
    kind of scenario, so it _might_ make sense to do that with this project.
- Create a sophisticated web app to run everything
  - This _might_ make more sense as a separate project...
- Use PCL (PixInsight Class Library) to support image "stuff"
- Create an Alpaca "proxy"
  - This only makes sense if the need arises...so far I'm not seeing one
- Add web based view of logs
  - This is probably a lot of work...and I'm not sure it is worth the effort
- ~~Write a web based planetarium :-) and leverage web asm / webgl/~~
  - Stellarium Web exists...and this is probably more of an insane
    idea than random or cool lol
- OpenPHD2 integration - https://code.google.com/archive/p/open-phd-guiding/wikis/EventMonitoring.wiki
- ~~Fork PHD2 ... perhaps create an alpaca native version?~~

Note: I need to move these to an issues list / tickets in Github

 - [ ] Create web page for project with documentation on usage
 - [ ] Build a downloadable version of this and create a release
 - [ ] Add github actions for build / execution of unit tests
       aka basic CI (Continuous Integration)
 - [x] Add Alpaca support to OpenPHD2
   - This is in my fork here: https://github.com/ceterumnet/phd2/tree/alpaca_support
 - [x] Write web server (restinio based)
 - [ ] Implement QHY Camera driver (color)
   - Note: I really need a QHY color camera to test this with...
 - [x] Implement QHY Camera driver (mono only)
    - [x] Achieve conformance with ASCOM Conform Tool
    - [ ] Refactor implementation to clean up weirdness around
          initialization and general clumsyness of the code
 - [x] Implement QHY filter wheel driver
   - [x] Achieve conformance with ASCOM Conform Tool
 - [x] Implement Alpaca discovery
 - [x] Implement telescope mount driver
    - [x] Achieve conformance with ASCOM Conform Tool (there are a few
      issues to work through on this)
 - [x] Implement focuser driver
 - [ ] Implement better descriptions in device driver implementations
 - [ ] Implement firmware version to data pulled into the device
       driver implementations
 - [ ] Implement support for logging to file as an option
 - [x] Implement CLI help
 - [x] Implement web screen for information
 - [x] Implement web setup screen for devices
 - [x] Implement Alpaca Management API
 - [x] Figure approach to test suite
 - [x] Cleanup error messages to follow consistent format
 - [x] Move device initializer code to be called on PUT connected
 - [x] Need to rethink templates for PUT requests for multiple values
 - [x] Check all device pointers for null (this could probably use a
       bit more robustness. However, I think it is adequate for now.
 - [x] Need to fix crash when invalid action is called on valid device
       type
 - [x] Need to figure out what the threading model will be for
       multiple devices I can do something like 1 thread per device or
       just add a thread pool the size of the device types. The
       disadvantage of a generic thread pool is that if multiple
       requests on a given device are taking a while, this can cause
       thread exhaustion. The disadvantage of a thread per device is
       that it adds a bit of complication to the code potentially.
 - [ ] I want to make the concept of device idx, uuid of a device, and
       devices being plugged in and unplugged as part of the lifecycle
       of adding / removing from the device_map. ~~This might play into
       how I spawn threads...~~
 - [x] Potentially refactor the registration of PUT routes to be
       similar to the GET routes so that I can remove the duplication
       of common paths and also set the device type based on the
       Device_T passed in.
       - This is partially done, I don't love the code structure for this
         but I am not really worried at the moment. This is a good candidate
         for a later refactor.
 - [x] Implement persistent UUID for devices. I can probably use the
       device serial number I think
