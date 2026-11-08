#include "Zormid.h"

int main() {
	zormid::Zormid::init();
	zormid::Zormid& zormid = zormid::Zormid::getInstance();
	zormid.run();
}
