#pragma once
#include "Problem.h"
#include <string>

struct Node {
    Problem data;
    Node* next;
    Node(Problem p) : data(p), next(nullptr) {}
};

enum class SortField { ID, NAME, DIFFICULTY };

class LinkedList {
public:
    LinkedList();
    ~LinkedList();

    void insert(const Problem& p);
    bool remove(int id);
    void sortBy(SortField field);
    void display() const;
    int size() const;
    Node* getHead() const;

private:
    Node* head;

    Node* mergeSort(Node* node, SortField field);
    Node* merge(Node* left, Node* right, SortField field);
    Node* getMiddle(Node* node);
    bool compare(const Problem& a, const Problem& b, SortField field);
    int difficultyRank(const std::string& d) const;
};
