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
	page *current = &range_start;
	u64 remianing_pages = page_count;
	while (remianing_pages > 0) {
		int order = 0;
		while (block_aligned(order, current->pfn()) && order <= LastOrder && remianing_pages >= pages_per_block(order)) {
			order += 1;
		}
		order -= 1;
		u64 size = pages_per_block(order);
		insert_free_block(order, *current);
		merge_buddies(order, *current);

		current += size;
		remianing_pages -= size;
	}
}

page *page_allocator_buddy::get_block_from_page(int order, page &target)
{
	page *block = free_list_[order];
	u64 size = pages_per_block(order);
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
	page *current = &range_start;
	u64 remaining_pages = page_count;
	while (remaining_pages > 0) {
		bool found = false;
		for (int order = LastOrder; order >= 0; order--) {
			page *block = get_block_from_page(order, *current);
			if (!block) {
				continue;
			}
			found = true;
			u64 size = pages_per_block(order);
			if (block->pfn() == current->pfn() && remaining_pages >= size) {
				remove_free_block(order, *block);
				current += size;
				remaining_pages -= size;
			} else if (order > 0) {
				split_block(order, *block);
			} else {
				panic("This should not happen.");
			}
		}
		if (!found) {
			current += 1;
			remaining_pages -= 1;
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
	assert(block_aligned(order, block_start.pfn()) && order == order);

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
	page *other_block = &page::get_from_pfn(block_start.pfn() + (pages_per_block(order) / 2));
	remove_free_block(order, block_start);
	insert_free_block(order - 1, block_start);
	insert_free_block(order - 1, *other_block);
}

void page_allocator_buddy::merge_buddies(int order, page &buddy)
{
	page *buddy_1 = &buddy;
	page *buddy_2 = nullptr;
	for (int current_order = order; current_order < LastOrder; current_order++) {
		u64 buddy_2_pfn = buddy_1->pfn() ^ pages_per_block(current_order);
		buddy_2 = free_list_[current_order];
		while (buddy_2 && buddy_2->pfn() != buddy_2_pfn) {
			buddy_2 = buddy_2->next_free_;
		}

		if (!buddy_2) {
			return;
		}

		remove_free_block(current_order, *buddy_1);
		remove_free_block(current_order, *buddy_2);

		buddy_1 = buddy_1->pfn() < buddy_2_pfn ? buddy_1 : buddy_2;
		insert_free_block(current_order + 1, *buddy_1);
	}
}

page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags)
{
	int current_order = order;
	page *block = free_list_[current_order];
	while (!block && current_order <= LastOrder) {
		current_order += 1;
		block = free_list_[current_order];
	}

	while (current_order > order) {
		split_block(current_order, *block);
		current_order -= 1;
	}

	remove_free_block(order, *block);
	return block;
}

void page_allocator_buddy::free_pages(page &block_start, int order)
{
	insert_free_block(order, block_start);
	merge_buddies(order, block_start);
}
