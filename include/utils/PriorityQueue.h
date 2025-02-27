// FILE: include/utils/PriorityQueue.h
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <vector>
#include <algorithm>
#include <functional>
#include <mutex>

// A priority queue implementation for the traffic simulation
template<typename T>
class PriorityQueue {
public:
    // Element with priority
    struct PriorityElement {
        T element;
        int priority;

        // Constructor
        PriorityElement(const T& e, int p) : element(e), priority(p) {}

        // Comparison operator for std::sort
        bool operator<(const PriorityElement& other) const {
            return priority < other.priority;
        }

        bool operator>(const PriorityElement& other) const {
            return priority > other.priority;
        }
    };

    PriorityQueue() = default;
    ~PriorityQueue() = default;

    // Add element with priority
    void enqueue(const T& element, int priority) {
        std::lock_guard<std::mutex> lock(mutex);

        // Add element with priority
        elements.push_back(PriorityElement(element, priority));

        // Sort in descending order (higher priority first)
        std::sort(elements.begin(), elements.end(), std::greater<PriorityElement>());
    }

    // Get the highest priority element
    T dequeue() {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("PriorityQueue is empty");
        }

        // Get the highest priority element
        T element = elements.front().element;
        elements.erase(elements.begin());

        return element;
    }

    // Peek at the highest priority element without removing it
    T peek() const {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("PriorityQueue is empty");
        }

        return elements.front().element;
    }

    // Update the priority of an element if it exists
    bool updatePriority(const T& element, int newPriority, std::function<bool(const T&, const T&)> comparator) {
        std::lock_guard<std::mutex> lock(mutex);

        // Find the element
        auto it = std::find_if(elements.begin(), elements.end(),
                             [&](const PriorityElement& pe) {
                                 return comparator(pe.element, element);
                             });

        if (it != elements.end()) {
            // Update the priority
            it->priority = newPriority;

            // Re-sort the elements
            std::sort(elements.begin(), elements.end(), std::greater<PriorityElement>());

            return true;
        }

        return false;
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

    // Get all elements in priority order
    std::vector<T> getAllElements() const {
        std::lock_guard<std::mutex> lock(mutex);

        std::vector<T> result;
        result.reserve(elements.size());

        for (const auto& pe : elements) {
            result.push_back(pe.element);
        }

        return result;
    }

private:
    std::vector<PriorityElement> elements;
    mutable std::mutex mutex;
};

#endif // PRIORITY_QUEUE_Hendif // PRIORITY_QUEUE_H
