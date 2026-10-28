// Presentation rectangle helper: used by the real Metal path and CPU tests.
#pragma once
#include <math.h>
static inline void acnh_present_rect(double dw,double dh,double cw,double ch,int full,double out[4]) {
 if(dw<=0||dh<=0||cw<=0||ch<=0){for(int i=0;i<4;i++)out[i]=0;return;}
 double s=fmin(dw/cw,dh/ch);out[0]=floor((dw-cw*s)/2);out[1]=floor((dh-ch*s)/2);out[2]=round(cw*s);out[3]=round(ch*s);
 if(full||fabs((dw/dh)/(cw/ch)-1)<.01){out[0]=out[1]=0;out[2]=dw;out[3]=dh;}
}
