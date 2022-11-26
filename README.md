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

Rightmost argument take priority and percentage is calculated last.
\
Default arguments:
* field width           = 15
* field height          = 15
* field mine percentage = 20

Ingame Keybinds:
* w :      Move cursor up
* s :      Move cursor down
* a :      Move cursor left
* d :      Move cursor right
* f :      Flag/Unflag cell
* Space :  Open cell
* Enter :  Exit game after playing
* Ctrl+c : Exit game

Neat features:
* The terminal will clean away the field after playing
* You can not open a flagged cell
* Opening a 0 will also open its neighbors
* Opening an open cell of the same number as it's number of neighboring flags will open its non-flagged neighbors
