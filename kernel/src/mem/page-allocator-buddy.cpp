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
		// If the block is aligned
		// And the order is within rnage
		// And we have enough pages to completely remove that block
		while (block_aligned(order, current->pfn()) && order <= LastOrder && remianing_pages >= pages_per_block(order)) {
			order += 1;
		}
		// -1 since the above case failed for this order
		order -= 1;
		u64 size = pages_per_block(order);
		insert_free_block(order, *current);
		merge_buddies(order, *current);
	
		// Pointer arithmetic automatically scales this up
		// Identical to page::get_from_pfn(current->pfn() + size)
		current += size;
		remianing_pages -= size;
	}
}

page *page_allocator_buddy::get_block_from_page(int order, page &target)
{
	page *block = free_list_[order];
	u64 size = pages_per_block(order);
	// Possibly put block->pfn() <= target.pfn() here
	// But only if the blocks are guaranteed to be in increasing order
	while (block) {
		// If the target pfn falls within the range of the block
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
			// Two Cases:
			// We can remove the entire block
			// We need to split the block

			u64 size = pages_per_block(order);
			// Entire block
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
		// The page was nowhere to be found
		// Ignore it
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
	// Midpoint of this block
	page *other_block = &page::get_from_pfn(block_start.pfn() + (pages_per_block(order) >> 1));
	remove_free_block(order, block_start);
	insert_free_block(order - 1, block_start);
	insert_free_block(order - 1, *other_block);
}

void page_allocator_buddy::merge_buddies(int order, page &buddy)
{
	page *buddy_1 = &buddy;
	page *buddy_2 = nullptr;

	// Work up the order since there may be other buddies waiting for its buddy
	for (int current_order = order; current_order < LastOrder; current_order++) {

		u64 buddy_2_pfn = buddy_1->pfn() ^ pages_per_block(current_order);
		buddy_2 = free_list_[current_order];

		// Look for its buddy
		while (buddy_2 && buddy_2->pfn() != buddy_2_pfn) {
			buddy_2 = buddy_2->next_free_;
		}

		// It's buddy is nowhere to be seen :( (not in this order)
		if (!buddy_2) {
			return;
		}

		// Remove both of them frome the current order
		remove_free_block(current_order, *buddy_1);
		remove_free_block(current_order, *buddy_2);

		// Get the lower pfn and insert it higher
		buddy_1 = buddy_1->pfn() < buddy_2_pfn ? buddy_1 : buddy_2;
		insert_free_block(current_order + 1, *buddy_1);
		// There could be more buddies in the higher order, go back!
	}
}

page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags)
{
	// -1 to keep it DRY
	int current_order = order - 1;
	page *block = nullptr;

	// Go up in order until we find a free block (-1 accommodates for the current order)
	while (!block && current_order <= LastOrder) {
		current_order += 1;
		block = free_list_[current_order];
	}

	// Then, go down however many steps we took, splitting the first block we find
	// This will not run if current_order == order
	for (; current_order > order; current_order--) {
		split_block(current_order, *block);
	}

	// Finally, we have our page
	// The two loops will not loop if there already exists a block in the desired order
	// nullcheck before deference. The allocator will return a nullptr if we're out of memory
	if (block) remove_free_block(order, *block);
	return block;
}

void page_allocator_buddy::free_pages(page &block_start, int order)
{
	// Possible to combine merge_buddies into insert_free_block
	// Since merge_buddies is usually called right after inserting a block
	insert_free_block(order, block_start);
	merge_buddies(order, block_start);
}
