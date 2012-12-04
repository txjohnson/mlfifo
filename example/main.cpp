/*/// ============================================================================================
	main.cpp

Copyright (c) 2012 Theo Johnson. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/*/// --------------------------------------------------------------------------------------------

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

