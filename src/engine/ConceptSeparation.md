# Konzept für die Separierung von libElecSim und olcPixelGameEngine:
Gewünschten Zustand erstellen: 
 - Alle Relations kappen. Dies wird das Project in einen nicht funktionierenden Zustand versetzen. 
 - Die TileTypeId verwenden und einen neuen Renderer schreiben.
 - Der neue Renderer verwendet anfangs ebenfalls Primitives, sollte aber möglichst bald auf eine Sprite-Map wechseln.
 - Refactoring von GridTile. (Entfernen von Funktionien und erstellen von den wichtigen Constructors und Operator, also Move, Rvalue, von denen hatte ich, als ich mit diesem Projekt angefangen habe, noch keine Ahnung.)
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
  Wire(elecSim::WireGridTile* tile) : tile(tile) {}
  void Draw(olc::PixelGameEngine *renderer) const final override {
    /* Implementation would come here */
  }
}

// Other way one could do it:
class Emitter : public EmitterGridTile, public Drawable<olc::PixelGameEngine>{
  std::weak_ptr<elecSim::WireGridTile> tile;
  Wire() : EmitterGridTile() {}
  void Draw(olc::PixelGameEngine *renderer) const final override {
    /* Implementation would come here */
  }
}
```

Und dann kann man diese Objekte so verwenden.

```cpp

void Draw(olc::PixelGameEngine *renderer,
          std::vector<std::unique_ptr<Drawable<olc::PixelGameEngine>>> drawables){
  for(const auto& drawable : drawables) {
    drawable->Draw(renderer)
  }
}

```

Alternativ kann man das ganze mit Concepts angehen.

```cpp
template<typename T, typename R>
concept Drawable = requires(const T& t, R* renderer) {
    { t.Draw(renderer) } -> std::same_as<void>;
};

// Type-erased drawable object without type identification
template<typename Renderer>
class DrawableObject {
private:
    std::function<void(Renderer*)> draw_fn;

public:
    template<typename T>
    requires Drawable<T, Renderer>
    explicit DrawableObject(T&& obj) {
        // Capture object by value using perfect forwarding
        draw_fn = [captured_obj = std::forward<T>(obj)]
                 (Renderer* renderer) {
            captured_obj.Draw(renderer);
        };
    }
    
    // Draw method - simply calls the stored function
    void Draw(Renderer* renderer) const {
        draw_fn(renderer);
    }
};

// Container for multiple drawable objects
template<typename Renderer>
class DrawBatch {
private:
    std::vector<DrawableObject<Renderer>> drawables;

public:
    // Add any object that satisfies the Drawable concept
    template<typename T>
    requires Drawable<T, Renderer>
    void Add(T&& obj) {
        drawables.emplace_back(std::forward<T>(obj));
    }
    
    // Draw all objects in the batch
    void DrawAll(Renderer* renderer) const {
        for (const auto& drawable : drawables)
            drawable.Draw(renderer);
    }
    
    // Basic collection operations
    void Clear() { drawables.clear(); }
    size_t Size() const { return drawables.size(); }
    bool Empty() const { return drawables.empty(); }
};
```

https://www.youtube.com/watch?v=gTNJXVmuRRA Video über Modernes C++ um Virtuelle Funktionen zu eliminieren.