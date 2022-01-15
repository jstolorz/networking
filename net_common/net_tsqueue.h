#pragma once

#include "net_common.h"

namespace bluesoft {
    namespace net {
        template<typename T>
        class tsqueue {
        public:
            tsqueue() = default;
            tsqueue(const tsqueue<T>&) = delete;

            virtual ~tsqueue(){
                clear();
            }

        public:
            // Return and maintains item at front of Queue
            const T& front(){
                std::scoped_lock lock(muxQueue);
                return deqQueue.front();
            }

            // Return and maintains item at back of Queue
            const T& back(){
                std::scoped_lock lock(muxQueue);
                return deqQueue.back();
            }

            // Adds an item to back of queue
            void push_back(const T& item){
                std::scoped_lock lock(muxQueue);
                deqQueue.template emplace_back(std::move(item));
            }

            // Adds an item to front of queue
            void push_front(const T& item){
                std::scoped_lock lock(muxQueue);
                deqQueue.template emplace_front(std::move(item));
            }

            // Return number of item in Queue
            size_t count(){
                std::scoped_lock lock(muxQueue);
                return deqQueue.size();
            }

            // Remove and return item from front of queue
            T pop_front(){
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.front());
                deqQueue.pop_front();
                return t;
            }

            // Remove and return item from back of queue
            T pop_back(){
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.back());
                deqQueue.pop_back();
                return t;
            }

            // Clear Queue
            void clear(){
                std::scoped_lock lock(muxQueue);
                deqQueue.clear();
            }



        protected:
            std::mutex muxQueue;
            std::deque<T> deqQueue;
        };
    }
}