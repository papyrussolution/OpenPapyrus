#include <iostream>

#define	ZERO	0
#define	ONE	1
#define	TWO	2

static const char someChars[3] = { '\n', '\'', '@' };

static const struct { int x, y; }
someTbl[3] = {
	{ ZERO, '"' },
	{ ONE, '\'' },
	{ TWO, ' ' }
};

int main()
{
	std::cout <<"Andrew"<<'\''<<"z test"<<std::endl;
}
