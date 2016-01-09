Appleseed renderer plugin for Maya

To run and compile you need the repository mayaToCommon. At the moment the repository should be called mayaToCentileo and should be at the same level as mayaToCommon:

    SomeDir
        mayaToCommon
        mayaToCentileo

You will need the Appleseed dependencies. Have a look here for a detailed description how tho get them: https://github.com/appleseedhq/appleseed/wiki/Building-appleseed-on-Windows
I used Boost 1.55. There is a VisualStudio props file which contains some macros with the API/Boost locations, these should be set appropriatly.

To run the debug plugin from maya, the maya needs some env variables:

MAYA_MODULE_PATH should point to the module path of mayaToCentileo. e.g.
MAYA_MODULE_PATH=c:/SomeDir/mayaToCentileo/mt_devmodule

The maya script path should contain the common scripts:

MAYA_SCRIPT_PATH=c:/SomeDir/mayaToCommon/python

Then it should work. This is only necessary for the debug version. In the final deployment the common scripts will be copied to the module script path.
