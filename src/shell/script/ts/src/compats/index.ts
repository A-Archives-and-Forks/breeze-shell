import { breeze } from "mshell";
import { doOneCommanderCompat } from "./onecommander_compat";
import { doExplorerCompat } from "./explorer_compat";

export const doCompats = () => {
    if (breeze.current_process_name() == "OneCommander.exe")
        doOneCommanderCompat();
    else if (breeze.current_process_name() == "explorer.exe")
        doExplorerCompat();
}