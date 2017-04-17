#include "template.h"
#include <iostream>

using namespace std;

template<class T> A<T>::A(){}

template<class T> T A<T>::g(T a,T b){
    return a+b;
}

int main(){
    A<int> a;
    cout<<a.g(2,3.2)<<endl;
}
