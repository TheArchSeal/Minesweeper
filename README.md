# Minesweeper
A simple game of minesweeper in powershell

---
Recognized command line arguments:
* w=\<n>        - Sets field width to n
* h=\<n>        - Sets field height to n
* dim=\<n>      - Sets field width and height to n
* dim=\<n>,\<m> - Sets field width to n and height to m
* mc=\<n>       - Sets field mine count to n
* mp=\<n>       - Sets field mine count to n percent of cell count
* --help        - List all valid commands
* --version     - Display the current version information
* --keybinds    - List all keybinds

Rightmost argument (except for --help, --version and --keybinds) takes priority and percentage is calculated last.

Default arguments:
* field width           = 20
* field height          = 15
* field mine percentage = 15

Ingame Keybinds:
* w :      Move cursor up
* s :      Move cursor down
* a :      Move cursor left
* d :      Move cursor right
* f :      Flag/Unflag cell
* Space :  Open cell
* Enter :  Exit game after playing
* Ctrl+c : Exit game
* \<any> : Update timer and redraw

Neat features:
* The terminal will clear away the field after playing
* You can't open a flagged cell
* The first cell will always be 0 if possible
* Opening a 0 will also open its neighbors
* Opening an open cell of the same number as it's number of neighboring flags will open all its non-flagged neighbors
* Add the folder 'build' to the PATH environment variable to call from anywhere using 'mine'
