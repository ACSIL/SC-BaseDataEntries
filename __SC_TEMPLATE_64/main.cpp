#include<iostream>
#include<vector>


std::vector<int> v{ 1,2,3,4,5 };

void change_vector(std::vector<int> x) //jaktoze tady nemusi byt predani referenci abych zmenil prvky primo v puvodnim ?
{
	std::cout << "puvodni elementy: " << '\n';
	for (int &i : x) //touhle referenci mam pristup k puvodnimu prvku v puvodnim vektoru? ---jakto?
	{
		std::cout << i << '\n';
		i = 0;
	}
	return;
}

void read_vector(std::vector<int> x)
{
	std::cout << "nove elementy: " << '\n';
	for (int i : x)
	{
		std::cout << i << '\n';
	}
	return;
}


int main()
{
	change_vector(v);
	read_vector(v);

	return 0;
}