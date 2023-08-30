//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::FindAvailableFrame(frame_id_t *frame_id) -> bool {
  bool find = false;
  if (!free_list_.empty()) {
    *frame_id = free_list_.front();
    free_list_.pop_front();
    find = true;
  } else if (replacer_->Evict(frame_id)) {
    Page *page = &pages_[*frame_id];
    if (page->is_dirty_) {
      disk_manager_->WritePage(page->GetPageId(), page->GetData());
      page->is_dirty_ = false;
    }
    // important: erase page from buffer pool
    page_table_.erase(page->GetPageId());
    find = true;
  }
  return find;
}

void BufferPoolManager::PinPage(frame_id_t frame_id) {
  replacer_->SetEvictable(frame_id, false);
  replacer_->RecordAccess(frame_id);
}

// Tips:
// 1. freelist or replacer to find available frame
// 2. dirty frame need to write back to disk
// 3. reset page metadata with new page_id and pin page
auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::unique_lock<std::mutex> l(latch_);
  frame_id_t found_frame_id;
  if (!FindAvailableFrame(&found_frame_id)) {  // tip1 / tip2
    return nullptr;
  }
  Page &page = pages_[found_frame_id];
  // tip3
  page.page_id_ = *page_id = AllocatePage();
  page.ResetMemory();
  page.pin_count_ = 1;
  PinPage(found_frame_id);
  page_table_[*page_id] = found_frame_id;  // keep tracking in buffer pool
  return &page;
}

// Tips:
// 1. find in keeping tracked buffer pool pages: pate_table_
// 2. not found -> freelist or replacer to find available frame -> dirty need write
// 3. disk_manager read page which linked to offset in db file
// 4. reset page metadata and pin page
auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  std::unique_lock<std::mutex> l(latch_);
  // tip1
  auto iter = page_table_.find(page_id);
  if (iter != page_table_.end()) {
    frame_id_t frame_id = iter->second;
    Page &page = pages_[frame_id];
    page.pin_count_++;
    PinPage(frame_id);
    return &page;
  }

  frame_id_t found_frame_id;
  if (!FindAvailableFrame(&found_frame_id)) {  // tip2
    return nullptr;
  }

  Page &page = pages_[found_frame_id];
  // read db file into available page
  disk_manager_->ReadPage(page_id, page.GetData());  // tip3
  page.page_id_ = page_id;
  page.pin_count_ = 1;
  page_table_[page_id] = found_frame_id;  // keep tracking in buffer pool
  PinPage(found_frame_id);                // tip4
  return &page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::unique_lock<std::mutex> l(latch_);
  auto iter = page_table_.find(page_id);
  if (iter == page_table_.end()) {
    return false;
  }
  frame_id_t frame_id = iter->second;
  Page &page = pages_[frame_id];
  if (page.pin_count_ == 0) {
    return false;
  }

  page.pin_count_--;
  if (page.pin_count_ == 0) {
    replacer_->SetEvictable(frame_id, true);
  }
  page.is_dirty_ |= is_dirty;
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::unique_lock<std::mutex> l(latch_);
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  auto iter = page_table_.find(page_id);
  if (iter == page_table_.end()) {
    return false;
  }
  frame_id_t frame_id = iter->second;
  Page &page = pages_[frame_id];
  disk_manager_->WritePage(page_id, page.GetData());
  page.is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::unique_lock<std::mutex> l(latch_);
  for (auto &iter : page_table_) {
    FlushPage(iter.first);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::unique_lock<std::mutex> l(latch_);
  auto iter = page_table_.find(page_id);
  if (iter == page_table_.end()) {
    return false;
  }
  frame_id_t frame_id = iter->second;
  Page &page = pages_[frame_id];
  if (page.pin_count_ > 0) {
    return false;
  }
  // delete page
  page_table_.erase(page_id);
  replacer_->Remove(frame_id);
  free_list_.emplace_front(frame_id);
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard { return {this, nullptr}; }

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, nullptr}; }

}  // namespace bustub
