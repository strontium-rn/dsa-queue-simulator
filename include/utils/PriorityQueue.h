// include/utils/PriorityQueue.hpp
#pragma once
#include "Queue.h"

template <typename T> class PriorityQueue : public Queue<T> {
  struct PriorityNode : public Queue<T>::Node {
    int priority;
    PriorityNode(const T &value, int p) : Queue<T>::Node(value), priority(p) {}
  };

public:
  void enqueuePriority(const T &value, int priority) {
    auto newNode = std::make_shared<PriorityNode>(value, priority);

    if (this->isEmpty() ||
        static_cast<PriorityNode *>(this->front.get())->priority < priority) {
      newNode->next = this->front;
      this->front = newNode;
    } else {
      auto current = this->front;
      while (current->next &&
             static_cast<PriorityNode *>(current->next.get())->priority >=
                 priority) {
        current = current->next;
      }
      newNode->next = current->next;
      current->next = newNode;
    }
    this->size++;
  }
};
