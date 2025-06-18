#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#include "Common.h"
#include "GridTileTypes.h"
#include "v2d.h"

namespace Engine {

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

    void DrawAll(R *renderer) {
        std::apply([renderer](auto&... vectors) {
            (DrawAllOfType(renderer, vectors), ...);
        }, data);
    }

    template<SameAsAny<Drawables...> Storeable>
    static void DrawAllOfType(R *renderer, std::vector<Storeable> items){
        for(auto& item : items){
            item.Draw(renderer);
        }
    }

private:
    std::tuple<std::vector<Drawables>...> data{};
};


}  // namespace Engine