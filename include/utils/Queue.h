// FILE: include/utils/Queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include <vector>
#include <mutex>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <functional>

// A thread-safe queue implementation for the traffic simulation
template<typename T>
class Queue {
public:
    Queue() = default;
    ~Queue() = default;

    // Add element to the queue
    void enqueue(const T& element) {
        std::lock_guard<std::mutex> lock(mutex);
        elements.push_back(element);
    }

    // Remove and return the front element
    T dequeue() {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        T element = elements.front();
        elements.erase(elements.begin());

        return element;
    }

    // Peek at the front element without removing it
    T peek() const {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        return elements.front();
    }

    // Check if the queue is empty
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements.empty();
    }

    // Get the size of the queue
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements.size();
    }

    // Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        elements.clear();
    }

    // Remove a specific element from anywhere in the queue (used for vehicle removal by ID)
    bool remove(const T& element, std::function<bool(const T&, const T&)> comparator) {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = std::find_if(elements.begin(), elements.end(),
                             [&](const T& e) {
                                 return comparator(e, element);
                             });

        if (it != elements.end()) {
            elements.erase(it);
            return true;
        }

        return false;
    }

    // Get all elements for iteration (e.g., for rendering)
    const std::vector<T>& getAllElements() const {
        // Note: This returns a const reference, so caller must not modify the vector
        // This avoids copying the entire vector while still providing access for iteration
        return elements;
    }

private:
    std::vector<T> elements;
    mutable std::mutex mutex;
};

#endif // QUEUE_Hendif // QUEUE_Hendif // QUEUE_H
