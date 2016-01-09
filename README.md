Appleseed renderer plugin for Maya

To run and compile you need the repository mayaToCommon. At the moment the repository should be called mayaToAppleseed and should be at the same level as mayaToCommon:

    SomeDir
        mayaToCommon
        mayaToAppleseed

You will need the Appleseed dependencies. Have a look here for a detailed description how tho get them: https://github.com/appleseedhq/appleseed/wiki/Building-appleseed-on-Windows
I used Boost 1.55. There is a VisualStudio props file which contains some macros with the API/Boost locations, these should be set appropriatly.

To run the debug plugin from maya, maya needs some env variables:

MAYA_MODULE_PATH should point to the module path of mayaToAppleseed. e.g.
MAYA_MODULE_PATH=c:/SomeDir/mayaToAppleseed/mtap_devmodule

The maya script path should contain the common scripts:

MAYA_SCRIPT_PATH=c:/SomeDir/mayaToCommon/python

This can be done via Maya.env file or with a batch script (my preferred way).
Then it should work. This is only necessary for the debug version. In the final deployment the common scripts will be copied to the module script path.
