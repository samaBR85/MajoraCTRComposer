static void ToolRun(int t)
{
    if (t == T_SEARCH) ToolSearch();
    else if (t == T_RAMDUMP) ToolRamDump();
    else if (t == T_HEXEDIT) ToolHexEdit();
    else if (t == T_ABOUT) ToolAbout();
#if !TOOLS_ONLY
    else if (t == T_GAMEGUIDE) ToolGameGuide();
    else if (t == T_TRACKER)   ToolChecklist();
#endif
    else if (t == T_PLUGINGUIDE) ToolPluginGuide();
}
