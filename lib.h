#ifndef LIB
#define LIB
#include<string>
struct user_shit{
    std::string nume;
    std::string prenume;
	std::string numar_card;
	std::string pin;
	std::string parola_secreta;
	double sold;//ok, 0*2=0
	bool active=false;
	int failed_attempts=0;
};

#endif
