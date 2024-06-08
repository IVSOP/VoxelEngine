#include "Client.hpp"

int main(int argc, char **argv) {
	Client client = Client();
	// client.loadWorldFrom("saves/first.world");
	client.mainloop();
	// client.saveWorldTo("saves/first.world");
	return 0;
}

