
module;

#include <vector>
#include <functional>
#include "framework.h"

export module general;

export template<typename... Args>
auto get_pattern(Args&&... patterns) {
    static_assert(sizeof...(patterns) >= 1 && sizeof...(patterns) <= 2,
        "Must provide 1 or 2 patterns");

    auto pattern_tuple = std::make_tuple(std::forward<Args>(patterns)...);

    if constexpr (sizeof...(patterns) == 1) {
        return hook::pattern(std::get<0>(pattern_tuple));
    }
    else {
#ifdef SRTT
        return hook::pattern(std::get<0>(pattern_tuple));
#else // SRTTR
        return hook::pattern(std::get<1>(pattern_tuple));
#endif
}
}

export class MixFix
{
public:
    template <typename... Args>
    class Call
    {
    public:
        Call& operator+=(std::function<void(Args...)> cb)
        {
            callbacks_.push_back(std::move(cb));
            return *this;
        }

        void addCallback(std::function<void(Args...)> cb)
        {
            callbacks_.push_back(std::move(cb));
        }

        void executeAll(Args... args)
        {
            for (auto& cb : callbacks_)
            {
                cb(args...);
            }
        }

    private:
        std::vector<std::function<void(Args...)>> callbacks_;
    };

public:
    static Call<>& onAttach() {
        static Call<> onProcessAttach;
        return onProcessAttach;
    }
    static Call<>& onGameInitStage_1() {
        static Call<> onGameInitStage_1;
        return onGameInitStage_1;
    }
    static Call<>& onGameInitStage_2() {
        static Call<> onGameInitStage_2;
        return onGameInitStage_2;
    }
    static Call<>& onGameplaySimulateFrame() {
        static Call<> onGameplaySimulateFrame;
        return onGameplaySimulateFrame;
    }
};
