#include "primer/trie.h"
#include <string_view>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
  auto cursor = root_;
  // try to find the last char node
  int len = static_cast<int>(key.length());
  for (int i = 0; len == 0 || i < len; i++) {
    if (cursor->children_.find(key[i]) == cursor->children_.end()) {
      return nullptr;
    }
    // const cursor read-only
    cursor = cursor->children_.at(key[i]);
    if (len == 0) {
      break;
    }
  }
  // dynamic_cast
  auto val_node = dynamic_cast<const TrieNodeWithValue<T> *>(cursor.get());
  if (val_node == nullptr) {
    return nullptr;
  }
  return val_node->value_.get();
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
  std::shared_ptr<TrieNode> new_root;
  if (root_ == nullptr) {
    new_root = std::make_shared<TrieNode>();
  } else {
    new_root = root_->Clone();
  }

  auto cursor = new_root;
  // walk to the penultimate node
  int len = static_cast<int>(key.length() - 1);
  for (int i = 0; i < len; i++) {
    if (cursor->children_.find(key[i]) == cursor->children_.end()) {
      auto n = std::make_shared<TrieNode>();
      cursor->children_.insert({key[i], n});
      cursor = n;
    } else {
      auto n = cursor->children_[key[i]]->Clone();
      auto new_child = std::shared_ptr<TrieNode>(std::move(n));
      cursor->children_.insert_or_assign(key[i], new_child);
      cursor = new_child;
    }
  }
  // find the last char
  auto last_char = key.length() > 0 ? key[key.length() - 1] : key[0];
  if (cursor->children_[last_char] == nullptr) {
    cursor->children_[last_char] = std::make_shared<TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));
  } else {
    auto children = cursor->children_[last_char]->children_;
    cursor->children_[last_char] =
        std::make_shared<TrieNodeWithValue<T>>(std::move(children), std::make_shared<T>(std::move(value)));
  }
  return Trie(new_root);
}

auto Trie::Remove(std::string_view key) const -> Trie {
  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
  std::shared_ptr<TrieNode> new_root = root_->Clone();
  auto cursor = new_root;
  // try to find the penultimate node
  int len = static_cast<int>(key.length() - 1);
  for (int i = 0; len == -1 || i < len; i++) {
    if (cursor->children_.find(key[i]) == cursor->children_.end()) {
      return Trie(root_);
    }
    auto n = cursor->children_[key[i]]->Clone();
    auto new_child = std::shared_ptr<TrieNode>(std::move(n));
    cursor->children_.insert_or_assign(key[i], new_child);
    cursor = new_child;
    if (len == -1) {
      break;
    }
  }
  auto last_char = key.length() > 0 ? key[key.length() - 1] : key[0];
  if (cursor->children_[last_char]->children_.empty()) {
    cursor->children_.erase(last_char);
  } else {
    auto children = cursor->children_[last_char]->children_;
    cursor->children_[last_char] = std::make_shared<TrieNode>(std::move(children));
  }
  return Trie(new_root);
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub
