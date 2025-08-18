#import <stdio.h>
#import <stdlib.h>
//#import <string.h>

static int SIZE = 15;

void unsort(int *p);
void print(int *p);
void bubble(int *p);
void insertion(int *p);
void selection(int *p);
void quick(int *p);
void merge(int *p, int n);
void mergeHelp(int *l1, int s1, int *l2, int s2, int *p);

int main() {
    {
        int list[15] = {0, 5, 2, 1, 2, 3, 6, 8, 9, 5, 2, 1, 2, 3, 4};
        int *p;
        p = list;
        bubble(p);
        unsort(p);
        insertion(p);
        unsort(p);
        selection(p);
        unsort(p);
        unsort(p);
        merge(p, SIZE);
        return 0;
    }
}


void unsort(int *p) {
	printf("UNSORT\n");
	int i, j;
	for(i = SIZE; i > 0; i--) {
		j = rand() % SIZE;
		p[j] = p[i];
	}
}

void print(int *p) {
 	int i;
	for(i = 0; i < SIZE; i++) {
		printf("i[%d] = %d\n", i, p[i]);
	}
}

void bubble(int *p) {
	printf("BUBBLE SORT\n");
	int i, j;
	
	for(i = 0; i < SIZE; i++) {
		for(j = 0; j < SIZE; j++) {
			if(p[i] < p[j]) { //swapping with XOR for funsies
				p[i] = p[i] ^ p[j];
				p[j] = p[i] ^ p[j];
				p[i] = p[i] ^ p[j];
			}
		}
	}
}

void insertion(int *p) {
	printf("INSERTION SORT\n");
	int i, j;
	int temp;
	for(i=1;i<SIZE;i++) {
		j=i;
		while(j>0 && p[j-1] > p[j]) {
			temp = p[j];
			p[j] = p[j-1];
			p[j-1] = temp;
			j=j-1;
		}
	}
}
void selection(int *p) {
	printf("SELECTION SORT\n");
	int i, j;
	int temp, tempMin;
	for(i=0;i<SIZE-1;i++) {
		tempMin = i;
		for(j=i+1; j<SIZE;j++) {
			if(p[tempMin] > p[j]) {
				tempMin = j;
			}
		}
		temp = p[i];
		p[i] = p[tempMin];
		p[tempMin] = temp;
	}

}
void quick(int *p){}
void merge(int *p, int n) {
	printf("n: %d\n", n);
	int i;
	int *l1, *l2;
	int s1, s2;

	if(n < 2) {
		return;
	}
	s1 = n/2;
	s2 = n-s1;
	l1 = (int*)malloc(sizeof(int)* s1);
	l2 = (int*)malloc(sizeof(int)* s2);

	for(i =0; i<s1; i++){
		l1[i] = p[i];
	}
	for(i=0; i<s2; i++){
		l2[i] = p[i+s1];
	}

	merge(l1,s1);
	merge(l2,s2);

	mergeHelp(l1, s1, l2, s2, p);
	free(l1);
	free(l2);
}

void mergeHelp(int *l1, int s1, int *l2, int s2, int *p) {
	int i,j,k;
	i=0;
	j=0;
	k=0;

	while(i < s1 && j < s2) {
		if(l1[i] <= l2[j]) {
			p[k] = l1[i];
			i++;
			k++;
		}
		else {
			p[k] = l2[j];
			j++;
			k++;
		}
	}
	while (i < s1) {
		p[k] = l1[i];
		i++;
		k++;
	}
	while (j < s2) {
		p[k] = l2[j];
		j++;
		k++;
	}
}

