#include "node.hpp"
#include <sstream>

Node::Node(int id) : id_(id) {}

int Node::id() const {
    return id_;
}

std::string Node::info() const {
    std::ostringstream ss;
    ss << "Node(id=" << id_ << ")";
    return ss.str();
}
