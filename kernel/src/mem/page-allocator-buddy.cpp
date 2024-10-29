/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/mem/page-allocator-buddy.h>
#include <stacsos/kernel/mem/page.h>
#include <stacsos/memops.h>

using namespace stacsos;
using namespace stacsos::kernel;
using namespace stacsos::kernel::mem;

void page_allocator_buddy::dump() const
{
	dprintf("*** buddy page allocator - free list ***\n");

	for (int i = 0; i <= LastOrder; i++) {
		dprintf("[%02u] ", i);

		page *c = free_list_[i];
		while (c) {
			dprintf("%lx--%lx ", c->base_address(), (c->base_address() + ((1 << i) << PAGE_BITS)) - 1);
			c = c->next_free_;
		}

		dprintf("\n");
	}
}

void page_allocator_buddy::insert_pages(page &range_start, u64 page_count)
{
	page *slot = &range_start;
	u64 remaining_pages = page_count;

	while (remaining_pages > 0) {
		int order = 0;
		u64 pfn = slot->pfn();
		while (block_aligned(order, pfn) && remaining_pages >= (1 << order) && order <= LastOrder) {
			order += 1;
		}
		order -= 1;
		insert_free_block(order, *slot);
		// Pointer Arithemtic under the hood
		// Since a page is 2 ^ 12, it'll automatically scale it.
		slot += 1 << order;
		remaining_pages -= 1 << order;
	}
}

page *page_allocator_buddy::get_block_from_page(int order, page &target)
{
	page *block = free_list_[order];
	int size = 1 << order;
	while (block) {
		if (block->pfn() <= target.pfn() && block->pfn() + size > target.pfn()) {
			return block;
		}
		block = block->next_free_;
	}
	return nullptr;
}

void page_allocator_buddy::remove_pages(page &range_start, u64 page_count)
{
	u64 pages_remaining = page_count;
	page *current = &range_start;

	while (pages_remaining > 0) {
		u64 current_pfn = current->pfn();
		bool found = false;
		for (int order = LastOrder; order >= 0; order--) {
			page *block = get_block_from_page(order, *current);
			if (block == nullptr) {
				continue;
			}
			found = true;
			// Cases:
			// Entire Block
			// Inside the Block

			// Entire Block
			if (block->pfn() == current_pfn && pages_remaining >= (1 << order)) {
				remove_free_block(order, *current);
				current += 1 << order;
				pages_remaining -= 1 << order;
			}
			// Inside the block
			else if (order > 0) {
				split_block(order, *block);
			} else {
				panic("This sould not happen.");
			}
		}

		if (!found) {
			current += 1;
			pages_remaining -= 1;
		}
	}
}

void page_allocator_buddy::insert_free_block(int order, page &block_start)
{
	// assert order in range
	assert(order >= 0 && order <= LastOrder);

	// assert block_start aligned to order
	assert(block_aligned(order, block_start.pfn()));

	page *target = &block_start;
	page **slot = &free_list_[order];
	while (*slot && *slot < target) {
		slot = &((*slot)->next_free_);
	}

	assert(*slot != target);

	target->next_free_ = *slot;
	*slot = target;
}

void page_allocator_buddy::remove_free_block(int order, page &block_start)
{
	// assert order in range
	assert(order >= 0 && order <= LastOrder);

	// assert block_start aligned to order
	assert(block_aligned(order, block_start.pfn()));

	page *target = &block_start;
	page **candidate_slot = &free_list_[order];
	while (*candidate_slot && *candidate_slot != target) {
		candidate_slot = &((*candidate_slot)->next_free_);
	}

	// assert candidate block exists
	assert(*candidate_slot == target);

	*candidate_slot = target->next_free_;
	target->next_free_ = nullptr;
}

void page_allocator_buddy::split_block(int order, page &block_start)
{
	// Find the halfway point between the pages and insert them into the previous order
	page *block_1 = &block_start;
	page *block_2 = &page::get_from_pfn(block_1->pfn() + (1ULL << (order - 1)));
	remove_free_block(order, *block_1);
	insert_free_block(order - 1, *block_1);
	insert_free_block(order - 1, *block_2);
}

void page_allocator_buddy::merge_buddies(int order, page &buddy)
{
	while (order <= LastOrder) {
		u64 target_pfn = buddy.pfn() ^ (1 << order);
		page *target = free_list_[order];
		while (target && target->pfn() != target_pfn) {
			target = target->next_free_;
		}

		if (!target) {
			return;
		}

		remove_free_block(order, *target);
		if (target_pfn > buddy.pfn()) {
			insert_free_block(order, buddy);
		} else {
			insert_free_block(order, *target);
		}
		order += 1;
	}
}

page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags) {
	int current_order = order;
	page *block = free_list_[order];
	while (!block) {
		block = free_list_[current_order];
		current_order += 1;
	}
	
	while (current_order > order) {
		split_block(current_order, *block);
		current_order -=1;
	}

	remove_free_block(order, *block);
	return block;
}

void page_allocator_buddy::free_pages(page &block_start, int order) {
	insert_free_block(order, block_start);
	merge_buddies(order, block_start);
}
