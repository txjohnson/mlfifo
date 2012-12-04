//
//  main.cpp
//  libdispatchpp
//
//  Created by Theo Johnson on 11/27/12.
//  Copyright (c) 2012 Theo Johnson. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <thread>
#include "mlfFIFO.h"

typedef tj::mlfFIFO<int, 64> intFIFO;
intFIFO f;

void read_items (int no) {
	std::cout << "Entering thead " << no << std::endl;
	int x;
	while (true) {
		while (!f.get(x));
		std::cout << "thread " << no << ": item " << x << std::endl;
		if (x == 1000) return;
		std::chrono::milliseconds dura( std::rand() % 2000 );
		std::this_thread::sleep_for( dura );
	}
}

int main(int argc, const char * argv[])
{
	
	for (int i = 0; i != 256; ++i) f .put (i);

	f .put (1000);
	f .put (1000);
	f .put (1000);
	f .put (1000);

	std::cout << "Spawing threads...\n";
	
	std::thread t1 (read_items, 1);
	std::thread t2 (read_items, 2);
	std::thread t3 (read_items, 3);
	std::thread t4 (read_items, 4);
	
	t1 .join ();
	t2 .join ();
	t3 .join ();
	t4 .join ();
	
    return 0;
}

