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
template<typename R, typename T>
concept Drawable = requires(T& t, R *renderer) {
    { t.Draw(renderer) } -> std::same_as<void>;
};

template<typename T, typename... Ts>
concept SameAsAny = (... || std::same_as<T, Ts>);


template<typename R, typename... Drawables>
    requires (... && Drawable<R, Drawables>)
class DrawBatch {
public:
    template<SameAsAny<Drawables...> Storeable>
    auto Store(Storeable item) {
        return std::get<std::vector<Storeable>>(data).push_back(item);
    }

    void DrawAll(MockRenderer *renderer) {
        std::apply([renderer](auto&... vectors) {
            (DrawAllOfType(renderer, vectors), ...);
        }, data);
    }

    template<SameAsAny<Drawables...> Storeable>
    static void DrawAllOfType(MockRenderer *renderer, std::vector<Storeable> items){
        for(auto& item : items){
            item.Draw(renderer);
        }
    }

private:
    std::tuple<std::vector<Drawables>...> data{};
};
```

Beide dieser Prinzipien funktionieren nicht, denn Sie setzen auf Polymorphismus, aber das können wir ehrlich gesagt nicht wirklich machen, und das will ich auch nicht so machen. 

https://www.youtube.com/watch?v=gTNJXVmuRRA Video über Modernes C++ um Virtuelle Funktionen zu eliminieren.


Diese Arbeit hat mich so aus dem Konzept gebracht, dass ich nun keine Lust mehr auf olcPixelGameEngine habe. Ich will SFML verwenden.