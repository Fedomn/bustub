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
