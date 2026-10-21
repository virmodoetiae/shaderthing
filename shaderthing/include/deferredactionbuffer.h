/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

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

    void add(std::function<void()>&& execute, int priority = 0)
    {
        add(std::move(execute), [](){ return true; }, priority);
    }

    void add
    (
        std::function<void()>&& execute,
        std::function<bool()>&& condition,
        int priority = 0
    )
    {
        // Kept sorted in descending priority; only a higher-priority insertion
        // at the back breaks that
        if 
        (
            !deferredActions_.empty() && 
            priority > deferredActions_.back().priority
        )
            sorted_ = false;
        deferredActions_.push_back
        (
            {priority, std::move(execute), std::move(condition)}
        );
    }

    void clear() {deferredActions_.clear();}

    void process()
    {
        if (deferredActions_.empty())
            return;

        if (!sorted_)
        {
            std::stable_sort
            (
                deferredActions_.begin(), 
                deferredActions_.end(),
                [](const DeferredAction& a, const DeferredAction& b)
                {
                    return a.priority > b.priority;
                }
            );
        }

        // Take ownership of the current actions. Anything an action add()s
        // while we run lands in the (now empty) member vector instead, so
        // nothing we're iterating over can be reallocated or destroyed.
        std::vector<DeferredAction> pending;
        pending.swap(deferredActions_);
        sorted_ = true;

        std::vector<DeferredAction> kept;
        for (auto& da : pending)
        {
            if (da.condition())
                da.execute();
            else
                kept.push_back(std::move(da));
        }

        // Actions added during execution run on the next process() call
        if (!deferredActions_.empty())
        {
            kept.insert
            (
                kept.end(),
                std::make_move_iterator(deferredActions_.begin()),
                std::make_move_iterator(deferredActions_.end())
            );
            sorted_ = false;
        }
        deferredActions_ = std::move(kept);
    }
};

}