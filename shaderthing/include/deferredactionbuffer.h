#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace ShaderThing
{

class DeferredActionBuffer
{

private:

    struct DeferredAction
    {
        // Higher priority (in value) actions are executed first. Negative
        // values are allowed and indicate lower-priority actions
        int priority = 0;
        std::function<void()> execute;
        std::function<bool()> condition = [](){return true;};
    };
    bool                        sorted_ = true;
    std::vector<DeferredAction> deferredActions_;

public:

    void add(std::function<void()>&& execute, int priority=0)
    {
        deferredActions_.emplace_back
        (
            DeferredAction{priority, std::move(execute)}
        );
        if 
        (
            sorted_ && 
            !deferredActions_.empty() && 
            priority <= deferredActions_.back().priority
        )
            return;
        sorted_ = deferredActions_.size() < 2;
    }

    void add
    (
        std::function<void()>&& execute, 
        std::function<bool()>&& condition, 
        int priority=0
    )
    {
        deferredActions_.emplace_back
        (
            DeferredAction{priority, std::move(execute), std::move(condition)}
        );
        if 
        (
            sorted_ && 
            !deferredActions_.empty() && 
            priority <= deferredActions_.back().priority
        )
            return;
        sorted_ = deferredActions_.size() < 2;
    }

    void clear() {deferredActions_.clear();}

    void process()
    {
        if (!sorted_)
        {
            //   stable_sort preserves insertion order of equal-priority actions
            std::stable_sort 
            (
                deferredActions_.begin(), 
                deferredActions_.end(),
                [](const DeferredAction& a, const DeferredAction& b) 
                {
                    return a.priority > b.priority;
                }
            );
            sorted_ = true;
        }
        
        // Execute all actions that can be executed and remove those after
        // execution
        auto it = std::remove_if
        (
            deferredActions_.begin(), 
            deferredActions_.end(),
            [](auto& da) 
            {
                if (da.condition()) 
                {
                    da.execute();
                    return true;  // Mark for removal
                }
                return false;
            }
        );
        deferredActions_.erase(it, deferredActions_.end());
    }
};

}