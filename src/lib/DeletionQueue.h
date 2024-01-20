#pragma once

#include <deque>

struct DeletionQueue {
        void push(std::function<void()>&& function) { _deleters.push_back(function); }

        /**
         * @brief Execute all deletion functions
         *
         */
        void flush() {
                for (auto it = _deleters.rbegin(); it != _deleters.rend(); it++) {
                        (*it)();
                }
                _deleters.clear();
        }

      private:
        std::deque<std::function<void()>> _deleters;
};