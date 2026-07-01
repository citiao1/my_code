#include "stdio.h"

typedef struct _
{
    char name[10];
    int age;
    int count;
    void (*chaifan)(struct _ *);
}person;

void callback(person *_person)
{
    _person ->count ++;
}

void person_init(person *_person)
{
    _person->chaifan = callback;
}

person xiaoming;

int main()
{


}



