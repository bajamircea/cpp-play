#pragma once

#include <bit>
#include <cassert>
#include <cstddef>

namespace cpp_util
{
  // WORK IN PROGRESS
  // Intrusive red-black tree
  //
  // Note: useful to use by deleting a node based on a reference to that node
  // - intrusive
  // - non-ownning (because intrusive)
  // - no size (because user can store size themselves)
  // - has parent (because that allows an efficient insert after search and not found)

  template<typename Node, Node * Node::*parent, Node * Node::*left, Node * Node::*right, char * Node::*color, typename ThreeWayCompare>
  class intrusive_llrb_tree
  {
    Node * root_ = nullptr;

  public:
    bool empty() const noexcept
    {
      return (root_ == nullptr);
    }

    void find(Node * new_node) noexcept
    {
      ThreeWayCompare threeWayCompare;
      Node * crt = root_;
      while (crt != nullptr)
      {
        auto result = threeWayCompare(*new_node, *crt);
        if (result == 0)
        {
          return crt;
        }
        else if (result < 0)
        {
          crt = crt->*left;
        }
        else
        {
          crt = crt->*right;
        }
      }
      return nullptr;
    }
  };
}