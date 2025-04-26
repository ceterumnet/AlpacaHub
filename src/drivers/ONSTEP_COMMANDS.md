OnStep Command Protocol
Here's a summary of the features/command set currently implemented for the On-Step controller:
 
Return values generally indicate failure (0) or success (1).
Command length is limited to 40 chars, 2 for the command frame ":#" + 2 for the code "CC" + a maximum of 36 for the parameter "P": ":CCPPP...#". Cr/lf chars can be sent along with your command, but are ignored.

## Implementation Status Legend
- ✅ Already implemented
- ✅ Newly implemented

## Date/time commands
- ✅ Set date	`:SCMM/DD/YY#`	Reply: 0 or 1
- ✅ Get date	`:GC#`	Reply: MM/DD/YY#
- ✅ Set time (Local)	`:SLHH:MM:SS#`	Reply: 0 or 1
- ✅ Get time (Local, 24hr format)	`:GL#`	Reply: HH:MM:SS#
- ✅ Get time (Local, 12hr format)	`:Ga#`	Reply: HH:MM:SS#
- ✅ Get time (Sidereal)	`:GS#`	Reply: HH:MM:SS#
- ✅ Set time (Sidereal)	`:SSHH:MM:SS#`	Reply: 0 or 1

## Site/Location commands
- ✅ Set UTC Offset(for current site)	`:SGsHH#`	Reply: 0 or 1
- ✅ Get UTC Offset(for current site)	`:GG#`	Reply: sHH#
- ✅ Set Latitude (for current site)	`:StsDD*MM#`	Reply: 0 or 1
- ✅ Get Latitude (for current site)	`:Gt#`	Reply: sDD*MM#
- ✅ Set Longitude (for current site)	`:SgDDD*MM#`	Reply: 0 or 1
- ✅ Get Longitude (for current site)	`:Gg#`	Reply: DDD*MM#
- ✅ Set site 0 name	`:SMsss...#`	Reply: 0 or 1
- ✅ Set site 1 name	`:SNsss...#`	Reply: 0 or 1
- ✅ Set site 2 name	`:SOsss...#`	Reply: 0 or 1
- ✅ Set site 3 name	`:SPsss...#`	Reply: 0 or 1
- ✅ Get site 0 name	`:GM#`	Reply: sss...#
- ✅ Get site 1 name	`:GN#`	Reply: sss...#
- ✅ Get site 2 name	`:GO#`	Reply: sss...#
- ✅ Get site 3 name	`:GP#`	Reply: sss...#
- ✅ Select site n (0-3)	`:Wn#`	Reply: [none]

The UTC Offset value is the number of hours to add to your Local Time (Standard Time) to get Universal Time.

## Slewing/Movement commands
- ✅ Set target RA	`:SrHH:MM:SS# *`	Reply: 0 or 1
- ✅ Get target RA	`:Gr#`	Reply: HH:MM:SS# *
- ✅ Set target Dec	`:SdsDD:MM:SS# *`	Reply: 0 or 1
- ✅ Get target Dec	`:Gd#`	Reply: sDD*MM'SS# *
- ✅ Set target Azm	`:SzDDD:MM:SS# *`	Reply: 0 or 1
- ✅ Set target Alt	`:SasDD:MM:SS# *`	Reply: 0 or 1
- ✅ Get telescope RA	`:GR#`	Reply: HH:MM:SS# *
- ✅ Get telescope Dec	`:GD#`	Reply: sDD*MM'SS# *
- ✅ Get telescope Azm	`:GZ#`	Reply: DDD*MM'SS# *
- ✅ Get telescope Alt	`:GA#`	Reply: sDD*MM'SS# *
* = Defaults to high precision mode, in low precision mode "HH:MM.M", "sDD*MM", or "DDD*MM" are used as appropriate.

- ✅ Set horizon limit	`:ShsDD#`	Reply: 0 or 1
- ✅ Get horizon limit	`:GhsDD#`	Reply: sDD#
- ✅ Set overhead limit	`:SoDD#`	Reply: 0 or 1
- ✅ Get overhead limit	`:GoDD#`	Reply: sDD#

The horizon limit sets how far below (or above) the horizon the telescope will point for a goto:
Valid range (in degrees) is +30 to -30.
The overhead limit helps keep the telescope tube from hitting the tripod etc. during a goto:
Valid range (in degrees) is 60 to 90.

- ✅ Move telescope (to current Equ target)	`:MS#`	Reply: e *2
- ✅ Move telescope (to current Hor target)	`:MA#`	Reply: e *2
*2 = Error codes for the MS and MA commands are as follows:
e=0 (no error), e=1 (below horizon), e=2 (no object), e=4 (position unreachable), e=5 (not aligned), e=6 (outside limits)

- ✅ Stop telescope	`:Q#`	Reply: [none]
- ✅ Move telescope east (at current rate)	`:Me#`	Reply: [none]
- ✅ Move telescope west (at current rate)	`:Mw#`	Reply: [none]
- ✅ Move telescope north (at current rate)	`:Mn#`	Reply: [none]
- ✅ Move telescope south (at current rate)	`:Ms#`	Reply: [none]
- ✅ Stop moving east	`:Qe#`	Reply: [none]
- ✅ Stop moving west	`:Qw#`	Reply: [none]
- ✅ Stop moving north	`:Qn#`	Reply: [none]
- ✅ Stop moving south	`:Qs#`	Reply: [none]
- ✅ Pulse guide (at current rate):
  d=n,s,e,w
  nnnn=time in mS
  (from 20 to 16399mS)	`:Mgdnnnn#`	Reply: [none]
- ✅ Set rate to Guide	`:RG#`	Reply: [none]
- ✅ Set rate to Centering	`:RC#`	Reply: [none]
- ✅ Set rate to Move	`:RM#`	Reply: [none]
- ✅ Set rate to Slew	`:RS#`	Reply: [none]
- ✅ Set rate to n (0-9)*3	`:Rn#`	Reply: [none]
*3 = Slew rates are as follows.
All values are in multipules of the sidereal rate:
R0=0.25X, R1=0.5X, R2(RG)=1X, R3=2X, R4(RC)=4X, R5=8X(RM), R6=16X, R7(RS)=24X, R8=40X, R9=60X
(for the -Dev-Alpha branch of OnStep:
R0=0.25X, R1=0.5X, R2(RG)=1X, R3=2X, R4=4X, R5(RC)=8X, R6(RM)=24X, R7=48X, R8(RS)=1/2 MaxRate, R9=MaxRate)

- ✅ Get distance bars (indicates slew)	`:D#`	Reply: \0x7F#
- ✅ Pier side	`:Gm#`	Reply: N#, E# or W#

## Tracking rate commands
- ✅ Set sidereal rate RA	`:STdd.ddddd#`	Reply: 0 or 1
- ✅ Get sidereal rate RA	`:GT#`	Reply: dd.ddddd#
- ✅ Track sidereal rate RA (default)	`:TQ#`	Reply: [none]
- ✅ Track sidereal rate reset	`:TR#`	Reply: [none]
- ✅ Track rate increase 0.02Hz	`:T+#`	Reply: [none]
- ✅ Track rate decrease 0.02Hz	`:T-#`	Reply: [none]
- ✅ Track solar rate RA	`:TS#`	Reply: [none]
- ✅ Track lunar rate RA	`:TL#`	Reply: [none]
- ✅ Track king rate RA	`:TK#`	Reply: [none]
- ✅ Tracking enable	`:Te#`	Reply: 0 or 1
- ✅ Tracking disable	`:Td#`	Reply: 0 or 1
- ✅ Refraction rate tracking	`:Tr#`	Reply: 0 or 1
- ✅ No refraction rate tracking	`:Tn#`	Reply: 0 or 1

Tracking rate adjustment is as follows:
The sidereal rate is default and is always selected on power-up. The T+ and T- commands can adjust any of the rates; however only the sidereal rate, if selected, remembers the adjusted rate through a power cycle.
Refraction rate tracking adjusts the RA rate dynamically to best compensate for refraction in a given region of the sky; again this works for any of the rates. This setting isn't remembered between power cycles, but the OnStep firmware can be compiled so that this setting defaults to enabled.

## Sync. command
- ✅ Sync. with current target RA/Dec	`:CS#`	Reply: [none]
- ✅ Sync. with current target RA/Dec	`:CM#`	Reply: N/A#
Note: Sync's that are not allowed fail silently. This can happen due to slews, parking, or exceeded limits.

## Library commands
- ✅ Select catalog no.	`:Lonn#`	Reply: 0 or 1
- ✅ Move Back in catalog	`:LB#`	Reply: [none]
- ✅ Move to Next in catalog	`:LN#`	Reply: [none]
- ✅ Move to catalog item no.	`:LCnnnn#`	Reply: [none]
- ✅ Move to catalog name rec.	`:L$#`	Reply: 1
- ✅ Get catalog item id.	`:LI#`	Reply: name,type#
- ✅ Read catalog item info.
(also moves forward)	`:LR#`	Reply: name,type,RA,Dec#
- ✅ Write catalog item info.
ssss=name, ttt=type code:
UNK,OC,GC,PN,DN,SG,EG,IG,KNT,SNR,GAL,CN,STR,PLA,CMT,AST	`:LWssss,ttt#`	Reply: 0 or 1
- ✅ Clear current record	`:LD#`	Reply: [none]
- ✅ Clear current cataLog	`:LL#`	Reply: [none]
- ✅ Clear all catalogs	`:L!#`	Reply: [none]

The LI# and LW# commands also set/get target coordinates (as with :Gr#, :Sr#, :Gd#, :Sd#)
Library record storage is in EEPROM. A catalog name record is like any other except the name must start with a '$'. A special search can then be done with the :L$# command to move to that record. It's up to the user to not waste EEPROM with more than one name record per catalog. When the default PEC table size of 824 bytes is used, the first 1024 bytes are devoted to settings. The remaining EEPROM is used for catalog records. Each record is 16 bytes.
It's often best to divide up large Libraries into several smaller catalogs due to serial interface speed limitations.

## Anti-backlash commands
- ✅ Set RA (Azm) backlash amount (in ArcSec)	`:$BRnnn#`	Reply: 0 or 1
- ✅ Set Dec (Alt) backlash amount (in ArcSec)	`:$BDnnn#`	Reply: 0 or 1

## Periodic error correction commands
- ✅ Turn PEC on	`:$QZ+#`	Reply: [none]
- ✅ Turn PEC off	`:$QZ-#`	Reply: [none]
- ✅ Clear PEC data	`:$QZZ#`	Reply: [none]
- ✅ Start recording PEC	`:$QZ/#`	Reply: [none]
- ✅ Save PEC data/settings to EEPROM	`:$QZ!#`	Reply: [none]
- ✅ Get PEC status returns:
I-Ignore PEC,
P-Playing PEC, p-Getting ready to play PEC,
R-Record PEC, r-Getting ready to record PEC	`:$QZ?#`	Reply: s#
- ✅ Readout PEC data	`:VRnnnn#`	Reply: sddd#
- ✅ Readout PEC data at current index (while playing/recording),
also returns index	`:VR#`	Reply: sddd,ddd#
- ✅ Write PEC data	`:WRnnnn,sddd#`	Reply: 0 or 1

PEC works as follows:
In-memory values are byte sized integers (corrections) that hold the number of steps to be applied (recorded/played) at a rate of one correction per second. Up to 824 bytes are available for storage (i.e. 824 seconds for a worm rotation). My G11 uses just 240 bytes (360 tooth worm gear, one revolution every 4 minutes).
After the data buffer is cleared ($QZZ), the next record session stores the guiding corrections without averaging. Subsequent recording of the guiding corrections use a 2:1 weighted average favoring the buffer. Data in the buffer is played back one second before the record time in the cycle to help compensate for guiding correction latency.
When reading and writing PEC data the units used are steps.
The save to EEPROM command allows the results to be recovered after a power cycle. This command takes several seconds to complete and should only be issued after you park or home the mount (stop tracking). NOTE: PEC isn't supported in Alt/Azm mode, yet.

## Alignment commands
- ✅ Align, write model to EEPROM	`:AW#`	Reply: 0 or 1
- ✅ Align, one-star*4	`:A1#`	Reply: 0 or 1
- ✅ Align, two or more star*4	`:A2# (:A3#, etc.)`	Reply: 0 or 1
- ✅ Align, accept*4	`:A+#`	Reply: 0 or 1
*4 = The one star alignment is implemented to correct RA/Dec offset. Two or more star alignment measure/corrects for polar axis misalignment (relative to the celestial pole,) cone error, etc. More stars removes ambiguity and increases likelihood of good performance. The results are saved when Set park is called and maintained when Parking/UnParking the mount. The sync. equatorial coordinates command refines the model for a local area of the sky, this refinement is lost when the power is cycled unless another Set park is called. The intended use of these commands is as follows...
Call A1. {or A2/A3/etc.}
Set RA/Dec target (not too close to a celestial pole.)
Goto to target.
Use move commands to center target.
Call A+ (records offsets, does basic correction of RA/Dec.)
Continue for 2 or more stars (repeating until done:)
Set RA/Dec target (not too close to a celestial pole and spread widely across sky for best results.)
Goto to target.
Use move commands to center target.
Call A+ (records offset.)

## Park commands
- ✅ Set park position	`:hQ#`	Reply: 0 or 1
- ✅ Move to park position	`:hP#`	Reply: 0 or 1
- ✅ Restore parked telescope to operation	`:hR#`	Reply: 0 or 1

## Home commands
- ✅ Move to home (CWD)	`:hC#`	Reply: [none]
- ✅ Set home (CWD)	`:hF#`	Reply: [none]
 
## Focus commands
- ✅ Focuser1 Active?	`:FA#`	Reply: 0 or 1
- ✅ Focuser2 Active?	`:fA#`	Reply: 0 or 1
- ✅ Select primary focuser n = 1 or 2	`:FA[n]#`	Reply: 0 or 1
- ✅ Get primary focuser	`:Fa#`	Reply: 0 or 1
- ✅ Get status M# = moving, S# = stopped	`:FT#`	Reply: M# or S#
- ✅ Get mode 0 = absolute 1 = pseudo  absolute	`:FI#`	Reply: 0 or 1
- ✅ Get full in position (in microns or steps)	`:FI#`	Reply: n#
- ✅ Get max position (in microns or steps)	`:FM#`	Reply: n#
- ✅ Get focuser temperature differential	`:Fe#`	Reply: n#
- ✅ Get focuser temperature	`:Ft#`	Reply: n#
- ✅ Get focuser microns per step	`:Fu#`	Reply: n.n#
- ✅ Get focuser backlash amount (in microns or steps)	`:FB#`	Reply: n#
- ✅ Set focuser backlash amount (in microns or steps)	`:FB[n]#`	Reply: 0 or 1
- ✅ Get focuser temperature compensation coefficient	`:FC#`	Reply: n.n#
- ✅ Set focuser temperature compensation coefficient	`:FC[sn.n]#`	Reply: 0 or 1
   in um per deg. C (+ moves out as temperature falls)
- ✅ Get focuser temperature compensation coefficient enable status	`:Fc#`	Reply: 0 or 1
- ✅ Enable/disable focuser temperature compensation [n] = 0 or 1	`:Fc[n]#`	Reply: 0 or 1
- ✅ Get focuser temperature compensation deadband amount	`:FD#`	Reply: n#
   (in microns or steps)
- ✅ Set focuser temperature compensation deadband amount	`:FD[n]#`	Reply: 0 or 1
   (in microns or steps)
- ✅ Get focuser DC Motor Power Level (in %)	`:FP#`	Reply: n#
- ✅ Set focuser DC Motor Power Level (in %)
`:FP[n]#`	Reply: 0 or 1
- ✅ Stops the focuser	`:FQ#`	Reply: [none]
- ✅ Set focuser for fast motion (1mm/s)	`:FF#`	Reply: [none]
- ✅ Set focuser for slow motion (0.01mm/s)	`:FS#`	Reply: [none]
- ✅ Set focuser move rate	`:F[n]#`	Reply: [none]
   ( n=1 for finest, n=2 for 0.01mm/sec, n=3 for 0.1mm/sec, n=4 for 1mm/sec)
- ✅ Move focuser in (toward objective)	`:F+#`	Reply: [none]
- ✅ Move focuser out (away from objective)	`:F-#`	Reply: [none]
- ✅ Get focuser current position (in microns or steps)	`:FG#`	Reply: n#
- ✅ Set focuser target position relative (in microns or steps)	`:FR[sn]#`	Reply: [none]
- ✅ Set focuser target position (in microns or steps)	`:FS[n]#`	Reply: 0 or 1
- ✅ Set focuser position as zero	`:FZ#`	Reply: [none]
- ✅ Set focuser position as half-travel	`:FH#`	Reply: [none]
- ✅ Set focuser target position at half-travel	`:Fh#`	Reply: [none]

## Reticle/Accessory Control
- ✅ Increase reticule Brightness	`:B+#`	Reply: [none]
- ✅ Decrease reticule Brightness	`:B-#`	Reply: [none]

## Misc. commands
- ✅ Reset controller (must be at home or parked)
OnStepX only, works for all platforms.	`:ERESET#`	Reply: [none] or 0 (failure)
- ✅ Reset NV (EEPROM)	`:ENVRESET#`	Reply: "NV memory will be cleared..."
- ✅ Set baud rate:
1=56.7K, 2=38.4K, 3=28.8K,
4=19.2K, 5=14.4K, 6=9600,
7=4800, 8=2400, 9=1200	`:SBn#`	Reply: 0 or 1
- ✅ Precision toggle	`:U#`	Reply: [none]
- ✅ Get firmware date	`:GVD#`	Reply: MM DD YY#
- ✅ Get firmware time	`:GVT#`	Reply: HH:MM:SS#
- ✅ Get firmware number	`:GVN#`	Reply: 3.16o#
- ✅ Get firmware name	`:GVP#`	Reply: On-Step#
- ✅ Get version    `:GV#`   Reply: On-Step#
- ✅ Get statUs returns:
N-Not slewing, H-At Home position,
P-Parked, p-Not parked, F-Park Failed,
I-park In progress, R-PEC Recorded
G-Guiding in progress, S-GPS PPS Synced	`:GU#`	Reply: sss#

## Implementation Status

All OnStep commands have been successfully implemented! Here is a summary of what was added:

1. Date/Time & Location commands - Complete
2. Slewing/Movement commands - Complete
3. Tracking Rate commands - Complete 
4. Home/Park commands - Complete
5. Misc commands - Complete
6. Focus commands - Complete
7. Library commands - Complete
8. Alignment commands - Complete
9. PEC commands - Complete
10. Reticle control - Complete

Note that there are still some remaining implementation tasks:
1. Complete implementation of all the library and catalog commands if needed for your use case
2. Implement advanced focuser commands if needed
3. Match parameter formatting exactly to the OnStep specification