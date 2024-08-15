#include<iostream>
using namespace std;
 
class MyClass
{
	int value;
public:
	MyClass(int i){value=i;}
	MyClass(){value=0;}
	int getvalue(){return value;}
	void setvalue(int v){value=v;}
};
 
int main()
{
	MyClass a[10]={0,1,2,3,4,5,6,7,8,9},b[10];
	cout<<"输出a: "<<endl;
	for(int i=0;i<10;i++)
	{
		cout<<"a["<<i<<"]="<<a[i].getvalue()<<" ";
		if((i+1)%5 == 0)
			cout<<endl;
	}
	cout<<"输出b: "<<endl;
	for(int i=0;i<10;i++)
	{
		cout<<"b["<<i<<"]="<<b[i].getvalue()<<" ";
		if((i+1)%5 == 0)
			cout<<endl;
	}
	return 0;
}