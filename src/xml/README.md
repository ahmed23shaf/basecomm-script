# Windows Base Communciations and Sniffer App XML files

The XML files, in this repo, contain the commands the base supports. The tools/Apps use these commands so you can send them and/or decode a sniffer box trace provided by others (e.g. SQA team). For Everest the "dtCommandsTMEV.xml" file is for PTC treadmills (i.e. TM = TreadMill, EV = EVerest). The "dtCommandsPNC.xml" is for the Everest non-treadmills or PNC base board. These were originally posted in SharePoint in the "Base Firmware Documentation" project. But we want a simple version control which is why we created a git repo. 

## Usage
Create a local file directory at "C:\LF\Shared Resources\XML". Then place all of the xml files from this repo inside the XML folder. Update if/as needed. When you launch any supported Applications, they will use the *.xml files in this path/folder.

## Updates
You can edit any of theses file(s) and post changes/updates to the repo. You can add in any missing (or less often used) command(s) as desired. You can make local changes to support any test code used. But Please don't commit changes made for test code (i.e. keep them locally and share via email/teams/slack/USB/etc.).

## Details
Any file ending in *_TE.xml are for use with any sniffer traces from Test Engineering. The TE department uses 0xBC as the "Console" like ID instead of "0xF0" (which is the ID our Consoles use to identify themselves). We can update these files as needed (and sometimes TE can tweak their testers to use 0xF0 instead of 0xBC). The plan is to eventually get rid of the need for these files.

There are also plans to refactor the xml files so that each data item, in any command/response packet, that contains data, can be individually grouped (i.e. its name, type, sizes and details). Today, the xml files use a comma delimited string that contains all names, then another string for all types, etc. This makes it hard to update/fix commands with many data items (e.g. Push_Report).

The dtSettings.xml file is currently only used for the Base communication Application (i.e. Console simulator). The plan is to get rid of this file altogether.

*IMDB* xml file is for the new Intergity treadmills (i.e. Project "Neon")

*RCB* xml files are for Integirty RCB non-treadmill bases (including RCB McFly, white boards)

*PM* xml file for Integrity PowerMill bases (includes both the MDB and SIB boards)

*TM* xml file for Integrity TreadMill bases (includes both the MDB and SIB boards)

*MM* xml file is for Myles Treadmill (i.e. slated belt/deck) project

March 2024