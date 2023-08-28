//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"
#include "fmt/format.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

// Tips:
// 1. The LRU-K algorithm evicts a frame whose backward k-distance is maximum of all frames in the replacer.
// 2. Backward k-distance is computed as the difference in time between current timestamp and the timestamp of kth
// previous access.
// 3. A frame with fewer than k historical accesses is given +inf as its backward k-distance.
// 4. When multiple frames have +inf backward k-distance, the replacer evicts the frame with the earliest overall
// timestamp.
auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::unique_lock<std::mutex> lock(latch_);
  bool evict = false;
  size_t min_delta_ts = 0;                // tip1/tip2
  size_t min_least_recent_ts = LONG_MAX;  // tip4
  auto current_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  // foreach node_store_ to find largest backward k-distance
  for (const auto &item : node_store_) {
    if (debug) {
      puts(fmt::format("find: {}", item.second->ToString()).c_str());
    }

    if (item.second->is_evictable_) {
      auto node = item.second;
      if (node->history_.size() >= k_) {
        auto k_history = node->GetKHistory(k_);  // tip2
        if (k_history == std::nullopt) {
          throw Exception("k_history is null");
        }
        size_t least_recent_ts = *k_history;
        size_t delta_ts = current_timestamp - least_recent_ts;
        if (delta_ts > min_delta_ts) {
          min_delta_ts = delta_ts;
          *frame_id = node->fid_;
          evict = true;
        }
      } else {
        min_delta_ts = LONG_MAX;  // tip3
        evict = true;
        *frame_id = node->fid_;
        size_t least_recent_ts = node->history_.back();
        if (least_recent_ts < min_least_recent_ts) {
          min_least_recent_ts = least_recent_ts;
          *frame_id = node->fid_;
        }
      }
    }
  }

  if (evict) {
    if (debug) {
      puts(fmt::format("evict: {}", node_store_[*frame_id]->ToString()).c_str());
    }
    node_store_.erase(*frame_id);
    curr_size_--;
  }
  return evict;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  BUSTUB_ASSERT(frame_id >= 0 || static_cast<size_t>(frame_id) <= replacer_size_, "frame_id is invalid");
  std::unique_lock<std::mutex> lock(latch_);
  auto iter = node_store_.find(frame_id);
  current_timestamp_ = std::chrono::system_clock::now().time_since_epoch().count();
  if (iter == node_store_.end()) {
    auto node = LRUKNode(frame_id);
    node.history_.push_front(current_timestamp_);
    node_store_.insert({frame_id, std::make_shared<LRUKNode>(node)});
  } else {
    auto node = iter->second;
    node->history_.push_front(current_timestamp_);
  }
}

// Tips:
// If a frame was previously evictable and is to be set to non-evictable, then size should decrement.
// If a frame was previously non-evictable and is to be set to evictable, then size should increment.
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  BUSTUB_ASSERT(frame_id >= 0 || static_cast<size_t>(frame_id) <= replacer_size_, "frame_id is invalid");
  std::unique_lock<std::mutex> lock(latch_);
  auto iter = node_store_.find(frame_id);
  if (iter == node_store_.end()) {
    return;
  }

  auto node = iter->second;
  if (node->is_evictable_ && !set_evictable) {
    node->is_evictable_ = false;
    curr_size_--;
  } else if (!node->is_evictable_ && set_evictable) {
    node->is_evictable_ = true;
    curr_size_++;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  BUSTUB_ASSERT(frame_id >= 0 || static_cast<size_t>(frame_id) <= replacer_size_, "frame_id is invalid");
  std::unique_lock<std::mutex> lock(latch_);
  auto iter = node_store_.find(frame_id);
  if (iter == node_store_.end()) {
    return;
  }
  auto node = iter->second;
  if (!node->is_evictable_) {
    throw Exception("frame is non-evictable");
  }
  node_store_.erase(frame_id);
  curr_size_--;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

auto LRUKNode::ToString() const -> std::string {
  std::list<std::string> history_list;
  for (const auto &item : history_) {
    history_list.push_back(std::to_string(item));
  }
  std::string history_str = fmt::format("[{}]", fmt::join(history_list, ","));
  return fmt::format("LRUKNode:{{ fid_={}, is_evictable_={}, history_={} }}", fid_, is_evictable_, history_str);
}

auto LRUKNode::GetKHistory(size_t k) -> std::optional<size_t> {
  size_t cnt = 1;
  for (const auto &item : history_) {
    if (cnt == k) {
      return item;
    }
    cnt++;
  }
  return std::nullopt;
}

}  // namespace bustub
