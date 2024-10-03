/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/sched/alg/rr.h>

// *** COURSEWORK NOTE *** //
// This will be where you are implementing your round-robin scheduling algorithm.
// Please edit this file in any way you see fit.  You may also edit the rr.h
// header file.

using namespace stacsos::kernel::sched;
using namespace stacsos::kernel::sched::alg;

void round_robin::add_to_runqueue(tcb &tcb) {
	tcb_list.append(&tcb);
}

void round_robin::remove_from_runqueue(tcb &tcb) {
	tcb_list.remove(&tcb);
}

tcb *round_robin::select_next_task(tcb *current) {
	if (tcb_list.empty()) {
		return nullptr;
	}

	// A true RR implementation does time slicing to each process,
	// however, I couldn't find a way to implement from editing rr.h and rr.cpp only.
	
	// Avoid modifying the list if there's only one elemet
	if (tcb_list.count() == 1) {
		return tcb_list.first();
	}
	
	// Get the next in line and remove it from the list
	tcb *first = tcb_list.dequeue();
	// Add it back to the end of the list 
	// A Circular LL would give constant time, but currenty it's O(n)
	tcb_list.enqueue(first);

	return first;
}
