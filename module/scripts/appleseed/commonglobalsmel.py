
#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

modifiedScripts = """
proc string renderSettingsTabLabel_melToUI (string $mel)
{
    // The arguments passed inside this procedure should not
    // be localized. This procedure uses the first string
    // argument that is passed with the "-addGlobalsTab"
    // flag in the "renderer" command.
    //
    string $result = $mel;

    if($mel == "Common"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kCommon"));
    }
    else if($mel == "Passes"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kPassesTab"));
    }
    else if($mel == "Maya Software"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kMayaSoftware"));
    }
    else if($mel == "Maya Hardware"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kMayaHardware"));
    }
    else if($mel == "Maya Vector"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kMayaVector"));
    }
    else if($mel == "Features"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kFeatures"));
    }
    else if($mel == "Quality"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kQuality"));
    }
    else if($mel == "Indirect Lighting"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kIndirectLighting"));
    }
    else if($mel == "Options"){
        $result = (uiRes("m_unifiedRenderGlobalsWindow.kOptions"));
    }
    else{
        uiToMelMsg( "renderSettingsTabLabel_melToUI", $mel, 0 );
    }

    return $result;
}


global proc addOneTabToGlobalsWindow(
    string $renderer, string $tabLabel, string $createProc)
{
    // Check to see if the unified render globals window existed.
    // If it does not exist, then we don't need to add any tab yet.
    //
    if (!`window -exists unifiedRenderGlobalsWindow`)
    {
        catch(error (uiRes("m_unifiedRenderGlobalsWindow.kCannotAddTabs")));
        return;
    }

    int $displayAllTabs = isDisplayingAllRendererTabs();
    if( !$displayAllTabs )
    {
        // If the current renderer the renderer is not this
        // renderer, then don't add the tab yet.
        //
        if( currentRenderer() != $renderer )
        {
            return;
        }
    }

    setParent unifiedRenderGlobalsWindow;

    // Hide the tabForm while updating.
    //
    int $tabFormManagedStatus = `formLayout -q -manage tabForm`;
    formLayout -edit -manage false tabForm;
    setParent tabForm;

    // Set the correct tabLayout parent.
    //
    string $tabLayoutName;
    if( $displayAllTabs ){
        global string $gMasterLayerRendererName;
        $tabLayoutName = getRendererTabLayout($gMasterLayerRendererName);
    }else{
        $tabLayoutName = getRendererTabLayout($renderer);
    }
    setParent $tabLayoutName;

    // The tabName is the tabLabel with the white space removed
    // and the word "Tab" added to the end.
    // "masterLayer" will act as the renderer name if the tab
    // is in the master layer.
    //
    string $tabName = rendererTabName($renderer, $tabLabel);

    // if the tab-control does not exist, define it and add it
    // to the tabLayout
    //
    if (!`layout -exists $tabName`)
    {
        setUITemplate -pushTemplate renderGlobalsTemplate;
        setUITemplate -pushTemplate attributeEditorTemplate;

        // Define the tab
        //
        formLayout $tabName;

        // get the content of the tab from the createTabProc
        //

        print("create proc " + $createProc + "\\n");
        // Most create procs are now deferred till the tab is selected
        // These two are the default tabs, so we don't defer them
        if (($createProc == "createMayaSoftwareCommonGlobalsTab")
            || ($createProc == "createMayaSoftwareGlobalsTab"))
        {
            eval($createProc);
        }

        if ($createProc == "CoronaCommonGlobalsCreateTab")
            eval($createProc);

        // These end off the layouts of the information in the Tab
        //
        setParent ..;

        setUITemplate -popTemplate;
        setUITemplate -popTemplate;

        // Add the tab to the tabLayout
        //
        tabLayout
            -edit
            -tabLabel $tabName `renderSettingsTabLabel_melToUI $tabLabel`
            $tabLayoutName;
    }

    // Restore the old manage status for the tabForm.
    //
    formLayout -edit -manage $tabFormManagedStatus tabForm;
}

print("mayaToxxx common globals sourced.\\n");
"""
