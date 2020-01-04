#include<stdio.h> 
#include<string.h> 
#include "rflib.h"
int rf_clamp_int(int min,int max,int out) {
	if(out < min) return min;
	if(out > max) return max;
	return out;
}
float rf_clamp_float(float min, float max, float out) {
	if(out < min) return min;
	if(out > max) return max;
	return out;
}
int rf_strcmp(const char *leftStr, const char* rightStr) {
	return strcmp(leftStr,rightStr) == 0 ? 1 : 0;
}
int rf_listcmp(const char *leftStr, const char* listChars) {
	if(strpbrk(leftStr, listChars) == NULL)
		return 0;
   return 1;
}