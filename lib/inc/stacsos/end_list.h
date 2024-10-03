#pragma once

#include "list.h"
namespace stacsos {

template <typename T> class end_list {
public:
	typedef list_node<T> node;
	end_list()
		: last_(nullptr)
		, head_(nullptr)
		, count_(0)
	{
	}

	~end_list()
	{
		// delete last_;
		node *current = head_;
		while (current) {
			node *next = current->next;
			delete current;
			current = next;
		}
	}

	T first() {
		return head_->data;
	}

	T dequeue()
	{
		
		count_--;
		node *front = head_;
		T ret = head_->data;
		head_ = head_->next;
		delete front;
		if (count_ == 0) {
			last_ = nullptr;
		}
		return ret;
	}

	void enqueue(T const &elem)
	{
		count_ += 1;
		node *new_node = new node(elem);
		new_node->next = nullptr;
		if (head_ == nullptr) {
			head_ = new_node;
			last_ = new_node;
		} else {
			last_->next = new_node;
			last_ = new_node;
		}
	}

	bool empty() {
		return count_ == 0;
	}

	void remove(T const& elem) {
		node **slot = &head_;
		// Incase the node to be remove is the last element
		node **prev = &head_;

		while (*slot && (*slot)->data != elem) {
			prev = slot;
			slot = &(*slot)->next;
		}
		
		if (*slot) {
			node *candidate = *slot;
			if (*slot == last_) {
				last_ = *prev;
			}
			*slot = candidate->next;
			delete candidate;
			count_--;
		}
	}

	int count() {
		return count_;
	}

private:
	list_node<T> *last_;
	list_node<T> *head_;
	int count_;
};
} // namespace stacsos
