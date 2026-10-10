#include "LinkedList.h"
#include <iostream>
#include <iomanip>

LinkedList::LinkedList() : head(nullptr) {}

LinkedList::~LinkedList() {
    Node* cur = head;
    while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
}

void LinkedList::insert(const Problem& p) {
    Node* newNode = new Node(p);
    if (!head) {
        head = newNode;
        return;
    }
    Node* cur = head;
    while (cur->next) cur = cur->next;
    cur->next = newNode;
}

void LinkedList::display() const {
    std::cout << std::left
              << std::setw(6)  << "ID"
              << std::setw(50) << "Name"
              << std::setw(10) << "Difficulty"
              << "\n";
    std::cout << std::string(66, '-') << "\n";

    Node* cur = head;
    while (cur) {
        std::cout << std::left
                  << std::setw(6)  << cur->data.id
                  << std::setw(50) << cur->data.name
                  << std::setw(10) << cur->data.difficulty
                  << "\n";
        cur = cur->next;
    }
    std::cout << "\n";
}

Node* LinkedList::getHead() const { return head; }

bool LinkedList::remove(int id) {
    if (!head) return false;
    if (head->data.id == id) {
        Node* tmp = head;
        head = head->next;
        delete tmp;
        return true;
    }
    Node* cur = head;
    while (cur->next) {
        if (cur->next->data.id == id) {
            Node* tmp = cur->next;
            cur->next = tmp->next;
            delete tmp;
            return true;
        }
        cur = cur->next;
    }
    return false;
}

int LinkedList::size() const {
    int count = 0;
    Node* cur = head;
    while (cur) { count++; cur = cur->next; }
    return count;
}

void LinkedList::sortBy(SortField field) {
    head = mergeSort(head, field);
}

Node* LinkedList::getMiddle(Node* node) {
    if (!node) return node;
    Node* slow = node;
    Node* fast = node->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

Node* LinkedList::mergeSort(Node* node, SortField field) {
    if (!node || !node->next) return node;

    Node* mid = getMiddle(node);
    Node* rightHalf = mid->next;
    mid->next = nullptr;

    Node* left  = mergeSort(node, field);
    Node* right = mergeSort(rightHalf, field);

    return merge(left, right, field);
}

Node* LinkedList::merge(Node* left, Node* right, SortField field) {
    if (!left)  return right;
    if (!right) return left;

    if (compare(left->data, right->data, field)) {
        left->next = merge(left->next, right, field);
        return left;
    } else {
        right->next = merge(left, right->next, field);
        return right;
    }
}

int LinkedList::difficultyRank(const std::string& d) const {
    if (d == "Easy")   return 1;
    if (d == "Medium") return 2;
    if (d == "Hard")   return 3;
    return 4;
}

bool LinkedList::compare(const Problem& a, const Problem& b, SortField field) {
    switch (field) {
        case SortField::ID:         return a.id < b.id;
        case SortField::NAME:       return a.name < b.name;
        case SortField::DIFFICULTY: return difficultyRank(a.difficulty) < difficultyRank(b.difficulty);
    }
    return false;
}
