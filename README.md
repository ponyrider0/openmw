ModExporter (Modified OpenMW-CS)
=======
Description:
-----
The ModExporter is designed to load a TES3 (Morrowind) based ESP/ESM file and export it as a TES4 (Oblivion) ESP which can be played in Morroblivion. It is based off the OpenMW code base. Although it was designed with Tamriel Rebuilt in mind, it can be used with many other Morrowind-based mods. Currently, it will convert landscapes, static meshes, NPCs and weapons. There is partial conversion of creatues, armor, clothing (meshes are substituted instead of converted). Dialog and quests are converted, but not fully usable due to incomplete conversion of scripts. A script translator and byte compiler are partially complete. Future versions will have more complete conversion of scripts and thus more usable dialog and quests.

Technical Details:
-----
The OpenMW base classes related to loading and saving record types are modified to allow exporting to TES4 format.  There is also a proof-of-concept branch which can export OpenMW-only mods to a TES3-compatible ESP/ESM.  Much of the data-mapping, preparation and translation as well as ESP/ESM file structure organization is managed through this class: https://github.com/ponyrider0/openmw/blob/Export-to-Morroblivion/apps/opencs/model/doc/exportToTES4.cpp

Most of the relevant modifications for those interested in the TES4 ESP/ESM file format can be found in this directory (look at loadXXXX.cpp files) : https://github.com/ponyrider0/openmw/tree/Export-to-Morroblivion/components/esm

An example of the export function for the statics class is here (bool Static::exportTESx, line 69) : https://github.com/ponyrider0/openmw/blob/Export-to-Morroblivion/components/esm/loadstat.cpp

TES4-export support added to these classes: Globals, Scripts, Spells, Races, Sounds, Classes, Factions, Enchantments, Regions, LandTextures, Weapons, Ammo, Misc, Keys, Soulgems, Lights, Ingredients, Clothing, Books, Armor, Apparati, Potions, LeveledItems, Activators, Statics, Furniture, Doors, Containers, Flora, NPCs, Creatures, LeveledCreatures, Cell References, Exterior Cells, Interior Cells, Quests, Dialog.

For those interested in the TES4 ESP/ESM file header, check out this function (void ESM::HeaderTES4::exportTES4, line 89): https://github.com/ponyrider0/openmw/blob/Export-to-Morroblivion/components/esm/loadtes4.cpp

Since TES4 game engine has moved to a hardcoded 8+24 bit FormID to identify records rather than dynamically resolving text-based identifiers, lookup tables were created to assign and manage FormID to textID resolution, with compatibility for current FormIDs used in Morroblivion.  These lookup tables can be found here: https://github.com/ponyrider0/Mod-Exporter-CSVfiles

Currently, scripts are translated and byte-compiled in a single-stage, line by line, using this class:
https://github.com/ponyrider0/openmw/blob/Export-to-Morroblivion/components/esm/scriptconverter.cpp.  However, work is currently being done to separate the byte-compiler from the translator in this branch: https://github.com/ponyrider0/openmw/tree/Script-Translator-Update-2019

A tutorial for using ModExporter to convert Tamriel Rebuilt can be found here: http://tesrenewal.com/forums/morroblivion-development/tutorial-converting-tamriel-rebuilt-to-morroblivion

Credits:
-----
OpenMW developers/source-cdoe for doing all the heavy lifting of loading and parsing the TES3-based files. The Unofficial Elder Scrolls Pages (uesp.net) community for sharing valuable information regarding game engine and file formats of Morrowind and Oblivion.
TESxEdit developers/source-code for definitions of the TES4-based file format.
Zilav for insights and helpful advice in emails and forum posts related to TES4Edit script writing and Oblivion interior and exterior cell cluster organization.
TESRenewal.com community for encouragement, suggestions and help testing and bug reporting throughout the development process.
And also Galadrielle for original ESPConverter and the long list of contributors to Morroblivion, including Puddles for his work on the Tamriel Rebuilt creature and armor replacers!
