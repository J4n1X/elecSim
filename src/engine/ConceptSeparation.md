# Konzept für die Separierung von libElecSim und olcPixelGameEngine:
Gewünschten Zustand erstellen: 
 - Alle Relations kappen. Dies wird das Project in einen nicht funktionierenden Zustand versetzen. 
 - Die TileTypeId verwenden und einen neuen Renderer schreiben.
 - Der neue Renderer verwendet anfangs ebenfalls Primitives, sollte aber möglichst bald auf eine Sprite-Map wechseln.
 - Refactoring von GridTile.