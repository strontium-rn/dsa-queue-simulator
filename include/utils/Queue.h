// include/utils/Queue.h
#pragma once
#include <memory>
#include <stdexcept>

template<typename T>
class Queue {
protected:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        Node(const T& value) : data(value), next(nullptr) {}
    };

    std::shared_ptr<Node> front;
    std::shared_ptr<Node> rear;
    size_t size;

public:
    Queue() : front(nullptr), rear(nullptr), size(0) {}

    virtual ~Queue() = default;

    void enqueue(const T& value) {
        auto newNode = std::make_shared<Node>(value);
        if (isEmpty()) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
        size++;
    }

    T dequeue() {
        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }

        T value = front->data;
        front = front->next;
        size--;

        if (isEmpty()) {
            rear = nullptr;
        }

        return value;
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    size_t getSize() const {
        return size;
    }

    T peek() const {
        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }
        return front->data;
    }

    // Add index-based peek
    T peek(size_t index) const {
        if (index >= size) {
            throw std::out_of_range("Index out of bounds");
        }

        auto current = front;
        for (size_t i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
};
