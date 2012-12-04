//
//  mlfdeque.h
//  libdispatchpp
//
//  Created by Theo Johnson on 11/28/12.
//  Copyright (c) 2012 Theo Johnson. All rights reserved.
//

#ifndef libdispatchpp_mlfFIFO_h
#define libdispatchpp_mlfFIFO_h

#include <memory>
#include <stdexcept>
#include <atomic>

namespace tj {

/*/// =================================================================================================================================
	FIFO queue using lock free techniques
/*/// ---------------------------------------------------------------------------------------------------------------------------------
template <typename T, size_t P = 4096, typename M = std::allocator<T>> class mlfFIFO {
public:

	mlfFIFO () : first (0), last (0), freefirst (0), freelast (0), freelock (ATOMIC_FLAG_INIT) {
	}
	
	mlfFIFO (const mlfFIFO&) = delete;
	mlfFIFO& operator= (const mlfFIFO&) = delete;
	
	// right now this isn't thread safe. Only do moves when the queue is empty.
	mlfFIFO (mlfFIFO&& m) : mlfFIFO () {
		m.freefirst .exchange (freefirst .exchange (m.freefirst));
		m.freelast  .exchange (freelast  .exchange (m.freelast));
		m.first     .exchange (first     .exchange (m.first));
		m.last      .exchange (last      .exchange (m.last));
	}
	
	mlfFIFO& operator=(mlfFIFO&& m)  {
		clear (); destroy ();
		m.freefirst .exchange (freefirst .exchange (m.freefirst));
		m.freelast  .exchange (freelast  .exchange (m.freelast));
		m.first     .exchange (first     .exchange (m.first));
		m.last      .exchange (last      .exchange (m.last));
		return *this;
	}
	
	~mlfFIFO () {
		clear ();
		destroy ();
	}
	
	bool put (const T& t) {
		while (!last.load()) get_a_page (); // spin to allocate  a page

		page* pagenow = last.load();
		item* tail = pagenow ->tail.load();
		
		if (tail == pagenow->end) {
			while (pagenow == last.load()) get_a_page();
			pagenow = last.load ();
			tail    = pagenow ->tail.load();
		}
		
		// scan for a slot not being written
		while (tail ->mode.exchange(-2) == -2) {
			++tail;
			if (tail == pagenow->end) {
				while (pagenow == last.load()) get_a_page();
				pagenow = last.load ();
				tail    = pagenow ->tail.load();
			}
			pagenow ->tail.store (tail);
		}
		
		tail->value = t;
		item* newtail = tail + 1;
		
		std::cout << "storing " << tail->value << " for tail at " << newtail - pagenow->beg << std::endl;
		
		// even if it fails, we're still good. it can only mean that a thread further along has updated tail
		pagenow ->tail.compare_exchange_strong (tail, newtail);
		tail ->mode.store (0);  // set ready to read
		return true;
	}
	
	bool get (T& into) {		
		page* pagenow = first.load();
		if (!pagenow) return false;

		item* head = pagenow ->head.load();
		if (head ->mode < 0) return false;
		
		// scan from head downwards until we find an item ready to be read
		while (head ->mode.exchange (1) == 1) {
			item* nhead = head + 1;
			if (nhead == pagenow->end) {
				if (!pagenow->next.load()) return false;
				pagenow = pagenow->next.load();
				head    = pagenow->head.load();
			}
			else {
				if (nhead ->mode < 0) return false;
				pagenow ->head.compare_exchange_strong (head, nhead);
				head = nhead;
			}
		}
		
		item* newhead = head + 1;
		pagenow ->head.compare_exchange_strong (head, newhead); // cause other reader to skip this node

		std::cout << "value is " << head->value << " for head is at " << head - pagenow->beg << std::endl;
		into = head ->value;
		head ->value.~T();
		head ->mode.store (-1);
		
		// last item on this page? we're responsible for removing the page
		if (newhead == pagenow->end) {
			first    .store (pagenow->next .load());
			while (freelock .test_and_set(std::memory_order_acquire)); // spin on lock
			if (freelast .load()) {
				freelast .load() ->freenext .store (pagenow);
				freelast .store (pagenow);
			}
			else {
				freefirst .store (pagenow);
				freelast  .store (pagenow);
			}
			freelock .clear();
		}
		
		return true;
	}
	
	
	// not threadsafe. insure no readers or writers before invoking
	void clear () {
		page* p = first.load ();
		
		while (p) {
			page* t = p ->next;
			p ->~page();
			p = new (p) page ();
			freelast.load() ->freenext .store (p);
			freelast .store (p);
			p = t;
		}
		first .store (0);
		last  .store (0);
		return *this;
	}
	
private:

	bool get_a_page () {
		page* p = 0;
		
		if (freefirst.load()) {
			if (freelock.test_and_set (std::memory_order_acquire)) return false;
			p = freefirst.load();
			freefirst.store (p->freenext.load());
			freelock.clear();
			p ->reset ();
		}
		else {
			typename std::allocator_traits<M>::template rebind_alloc<page> a;
			p = a.allocate (1);
			a.construct (p);
		}
		
		if (!first.load()) { last.store(p); first.store (p); }
		else {
			last.load()->next.store(p);
			last.store (p);
		}
		
		return true;
	}
	
	void destroy () {
		while (freefirst) {
			page* p = freefirst.load() ->freenext.load();
			typename std::allocator_traits<M>::template rebind_alloc<page> a;
			a.deallocate (freefirst.load(), 1);
			freefirst .store (p);
		}
		freelast .store (0);
	}
	
	struct item {
		std::atomic<int>  mode;    // -2 (writing) -1 (writeable) 0 (readable) 1 (writing)
		T                 value;
		
		item () : mode (-1), value(0) { }
		~item () { }
	};
	
	struct page {
		item*    beg;
		item*    end;
		std::atomic <item*> head;
		std::atomic <item*> tail;
		std::atomic <page*> next;
		std::atomic <page*> freenext;
		
		item  items [P+1];
		
		page () : beg (items), end (beg + P), head (beg), tail (beg), next (0), freenext (0) {
			items[P].mode.store (-2);
		}
		
		~page () {
			if (head.load()) {
				while (head != tail) {
					head.load() ->~item();
					++head;
				}
			}
		}
		
		void reset () {
			head     = beg;
			tail     = beg;
			next     = 0;
			freenext = 0;
		}
	};
	
	std::atomic <page*> first;
	std::atomic <page*> last;
	std::atomic <page*> freefirst;
	std::atomic <page*> freelast;
	std::atomic_flag    freelock;
};


}

#endif
