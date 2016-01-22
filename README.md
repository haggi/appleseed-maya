Appleseed renderer plugin for Maya

mayaToAppleseed relies on a sumodule called mayaToCommon and is a seperate repository.

You will need the Appleseed dependencies. Have a look here for a detailed description how tho get them: https://github.com/appleseedhq/appleseed/wiki/Building-appleseed-on-Windows
I used Boost 1.55. There is a VisualStudio props file which contains some macros with the API/Boost locations, these should be set appropriatly.

To run the debug plugin from maya, maya needs some env variables:<br>

MAYA_MODULE_PATH should point to the module path of mayaToAppleseed. e.g.<br>
MAYA_MODULE_PATH=c:/SomeDir/mayaToAppleseed/mtap_devmodule<br>

The maya script path should contain the common scripts:

MAYA_SCRIPT_PATH=c:/SomeDir/mayaToCommon/python<br>

This can be done via Maya.env file or with a batch script (my preferred way).<br>
Then it should work. This is only necessary for the debug version. In the final deployment the common scripts will be copied to the module script path.
