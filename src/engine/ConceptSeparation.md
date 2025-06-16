# Konzept für die Separierung von libElecSim und olcPixelGameEngine:
Gewünschten Zustand erstellen: 
 - Alle Relations kappen. Dies wird das Project in einen nicht funktionierenden Zustand versetzen. 
 - Die TileTypeId verwenden und einen neuen Renderer schreiben.
 - Der neue Renderer verwendet anfangs ebenfalls Primitives, sollte aber möglichst bald auf eine Sprite-Map wechseln.
 - Refactoring von GridTile.
## Ansatz ohne TileId
```cpp
// Base Class.
template<typename Renderer>
class Drawable {
  virtual ~Drawable = default();
  virtual void Draw(Renderer *renderer) const = 0;
};

// Here's how it'd be used.
class Wire : public Drawable<olc::PixelGameEngine>{
  std::weak_ptr<elecSim::WireGridTile> tile;
  Wire(elecSim::WireGridTile* tile) : tile {}
  void Draw(olc::PixelGameEngine *renderer) const final override {
    /* Implementation would come here */
  }
}
```

Which would then lead to it being usable like this when it comes to rendering (very simplified):

```cpp

void Draw(olc::PixelGameEngine *renderer,
          std::vector<std::unique_ptr<Drawable<olc::PixelGameEngine>>> drawables){
  for(const auto& drawable : drawables) {
    drawable->Draw(renderer)
  }
}

```