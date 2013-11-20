#include"include/zvmlib.h"
#define F for
#define I int
I P(I a){WRITE(STDOUT,&a,1);return a;}I m=1711276033,N=1,t[1<<25]={2},a,*p,i,s,c
,e=39717691,U=1;void g(I d, I h){F(i=s;i<1<<24;i*=2)d=d*1LL*d%m;F(p=t;p<t+N;p+=s
)F(i=s,c=1;i;i--)a=p[s]*(h?c:1LL)%m,p[s]=(m*1U+*p-a)*(h?1LL:c)%m,*p=(a*1U+*p)%m,
p++,c=c*1LL*d%m;}I main(){while(e/=2){N*=2;U=U*1LL*(m+1)/2%m;F(s=N;s/=2;)g(40,0)
;F(p=t;p<t+N;p++)*p=*p*1LL**p%m*U%m;F(s=1;s<N;s*=2)g(983983719,1);F(a=0,p=t;p<t+
N;)a+=*p<<(e&1),*p++=a%10,a/=10;}while(!*--p);F(t[0]--;p>=t;)P(48+*p--);return 0;}
