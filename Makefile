.PHONY: build

build:
	rm -rf build/
	mkdir build
	# SANITIZER=thread, memory, address
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DBUSTUB_SANITIZER=thread ..

format:
	cd build && \
	make format && \
	make check-lint && \
	make check-clang-tidy-p0

test-p0:
	cd build && \
	make trie_test trie_store_test -j$(nproc) && \
	make trie_noncopy_test trie_store_noncopy_test -j$(nproc) && \
	./test/trie_test && \
	./test/trie_noncopy_test && \
	./test/trie_store_test && \
	./test/trie_store_noncopy_test

test-p1:
	cd build && \
	make lru_k_replacer_test -j$(nproc) && \
	make buffer_pool_manager_test -j$(nproc) && \
	make page_guard_test -j$(nproc) && \
    ./test/lru_k_replacer_test && \
	./test/buffer_pool_manager_test && \
	./test/page_guard_test

bpm-bench-build:
	rm -rf cmake-build-relwithdebinfo && \
	mkdir cmake-build-relwithdebinfo && \
    cd cmake-build-relwithdebinfo && \
    cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo && \
    make -j`nproc` bpm-bench

bpm-bench-run:
	cd cmake-build-relwithdebinfo && \
    ./bin/bustub-bpm-bench --duration 5000 --latency 1 && \
    ./bin/bustub-bpm-bench --duration 5000 --latency 0