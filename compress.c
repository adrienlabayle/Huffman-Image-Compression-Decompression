#include<stdio.h>
#include<stdlib.h>

typedef struct CASE{
 int val;
 int occ;
}CASE;

typedef struct node{
    int val;
    int occ;
    struct node * fg;
    struct node * fd;
}node;

typedef struct clip{
    node * tree;
    struct clip * next;
}clip;

typedef struct code_case{
    int bit_nbr;
    unsigned char * code;
}code_case;

void fusion(CASE * a, int lo, int mid, int hi){
    int i=lo;
    int j=mid+1;
    int k;
    CASE aux[256];

    for(k=0; k<=hi; k++){
        aux[k].val = a[k].val;
        aux[k].occ = a[k].occ;
    }

    for(k=lo; k<=hi; k++){
        if(i>mid){
            a[k].val = aux[j].val;
            a[k].occ = aux[j++].occ;
        }
        else if(j>hi){
            a[k].val = aux[i].val;
            a[k].occ = aux[i++].occ;
        }
        else if(aux[j].occ < aux[i].occ){
            a[k].val = aux[j].val;
            a[k].occ = aux[j++].occ;
        }
        else{
            a[k].val = aux[i].val;
            a[k].occ = aux[i++].occ;
        }
    }
}

void mergesort(CASE * a, int lo, int hi){
    int mid;

    if(lo>=hi) return;

    mid = lo + (hi-lo)/2;
    mergesort(a, lo, mid);
    mergesort(a, mid+1, hi);
    fusion(a, lo, mid, hi);
}

/* create the huffman tree from the list of 'clip' */
clip * huffman_tree(clip * h){  /* The function builds the Huffman tree by repeatedly grouping the first two clips in the list,
            which always represent the smallest pixel occurrence values, since the list is kept sorted in ascending order of occurrences.
            A new clip is created, whose associated tree becomes the parent of the trees of the two selected clips. This new clip is then reinserted into the list, maintaining the sorted order based on occurrences.
            The process continues until only one clip remains, whose associated tree becomes the root of the complete Huffman tree. */
    clip * a;
    clip * b;
    node * new_node;
    clip * new_clip;
    clip * cur;

    if(!h->next){
        new_node = (node *)malloc(sizeof(node));
        new_node->val = -1;
        new_node->occ = h->tree->occ;
        new_node->fg = h->tree;
        new_node->fd = NULL;

        h->tree=new_node;
    }

    while (h && h->next) {
        a = h;
        b = h->next;
        h = b->next;

        new_node = (node *)malloc(sizeof(node));
        new_node->val = -1;
        new_node->occ = a->tree->occ + b->tree->occ;
        new_node->fg = a->tree;
        new_node->fd = b->tree;

        free(a);
        free(b);

        new_clip = (clip *)malloc(sizeof(clip));
        new_clip->tree = new_node;
        new_clip->next = NULL;

        if (!h || new_node->occ <= h->tree->occ) {
            new_clip->next = h;
            h = new_clip;
        } else {
            cur = h;
            while (cur->next && cur->next->tree->occ < new_node->occ) {
                cur = cur->next;
            }
            new_clip->next = cur->next;
            cur->next = new_clip;
        }
    }
    return h;
}

/* create the Huffman code from the Huffman tree */
void encode(node * h, int depth, code_case * T2, int bit, unsigned char * code){ /* simply descend in the huffman tree in search of node-value, in order to associate their Huffman code,
            which is updated according to the descent by adding a new bit at the end of the code at each descent (bit = 1 if I descend to the right, 0 otherwise  */
    int i;
    int r;
    unsigned char * newcode;

    int j;

    if(bit==-1){
        if(h->fd!=NULL){
            encode(h->fd, depth+1, T2, 1, code);
        }
        if(h->fg!=NULL){
            encode(h->fg, depth+1, T2, 0, code);
        }
    }
    else{
        newcode=(unsigned char *)malloc(sizeof(unsigned char)*((depth+7)/8));
        for(i=((depth+7)/8)-1; i>=0; i--){
            if(depth%8==0){
                r=7;
            }
            else{
                r=(depth%8)-1;
            }
            if((depth+6)/8 < (depth+7)/8){  /* coresponde to  the case where we have a new byte*/
                if(i==0){
                    newcode[i] = bit;
                }
                else{
                    newcode[i] = code[i-1];
                }
            }
            else{
                if(i==0){
                    newcode[i] = code[i] | (bit << r);
                }
                else{
                    newcode[i] = code[i];
                }
            }
        }
        if(h->val!=-1){
            T2[h->val].bit_nbr=depth;
            T2[h->val].code=newcode;
            newcode=NULL;

        }
        else{
            if(h->fd!=NULL){
                encode(h->fd, depth+1, T2, 1, newcode);
            }
            if(h->fg!=NULL){
                encode(h->fg, depth+1, T2, 0, newcode);
            }
        }
        free(newcode);
    }
}

void bit_write(FILE * f, int * nbits, unsigned char * buffer, int bit){

    *buffer = (*buffer << 1) | (bit & 1);
    (*nbits)++;

    if(*nbits==8){
        fwrite(buffer, 1, 1, f);
        *buffer=0;
        *nbits=0;
    }

}

void free_huffman_tree(node *root){
    if (!root) return;
    free_huffman_tree(root->fg);
    free_huffman_tree(root->fd);
    free(root);
}

void out_write(FILE * f, int height, int width, unsigned char * buffer, code_case * T2, int * rest){   /* writes the entire compressed image file */
    int i;
    int j;
    int k;
    int count1;
    unsigned char * meta;
    int nbits;
    unsigned char buffer2;

    meta=(unsigned char *)malloc(sizeof(unsigned char)*4);  /* we write the size of the image */
    meta[2] = height & 0xFF;
    meta[3] = (height >> 8) & 0xFF;
    meta[0] = width & 0xFF;
    meta[1] = (width >> 8) & 0xFF;

    for(i=3; i>=0; i--){
        fwrite(&meta[i], 1, 1, f);
    }
    free(meta);

    meta=(unsigned char *)malloc(sizeof(unsigned char));  /* we write the necessary information to find the original value from the compressed code of the pixels */
    for(i=0; i<256; i++){
        if(T2[i].bit_nbr!=-1){
            *meta = i;
            fwrite(meta, 1, 1, f);
            *meta = T2[i].bit_nbr;
            fwrite(meta, 1, 1, f);
            for(j=0; j<((T2[i].bit_nbr+7)/8); j++){
                *meta = T2[i].code[j];
                fwrite(meta, 1, 1, f);
            }
        }
    }
    *meta=0;
    fwrite(meta, 1, 1, f);  /* write two byte of 0 to delimit the metadata from the image content */
    fwrite(meta, 1, 1, f);
    free(meta);

    nbits=0;
    buffer2=0;

    for(i=0; i<(width*height); i++){  /* we write all the content image with the compressed code of the pixels */
        for(j=0; j<((T2[buffer[i]].bit_nbr+7)/8); j++){
            if(j==0 && (T2[buffer[i]].bit_nbr % 8)!=0){
                count1= -1 + (T2[buffer[i]].bit_nbr % 8);
            }
            else{
                count1=7;
            }
            for(k=count1; k>=0; k--){
                bit_write(f, &nbits, &buffer2, ((T2[buffer[i]].code[j] >> k) & 1));
            }
        }
    }

    if(nbits!=0){   /* in case there are some bits left in the buffer at the end, we just arrange to write it well with the extra 0s at the end (right) */
        buffer2=(buffer2 << (8-nbits));
        fwrite(&buffer2, 1, 1, f);
        *rest = 8-nbits; /* number of extra 'unnecessary' 0s */
    }
    else{
        *rest = 0;
    }
}

int main(int argc, char *argv[]){
    unsigned char * buffer;
    unsigned char * CODE;
    code_case * T2;
    int height, width;
    int i;
    CASE * T1;
    clip * h;
    clip * pt1;
    int rest;

    h = (clip *)malloc(sizeof(clip));
    h->tree=NULL;
    h->next=NULL;

    T1 = (CASE *)malloc(sizeof(CASE) * 256);
    for(i=0; i<256; i++){
        T1[i].val = i;
        T1[i].occ = 0;
    }

    FILE * meta = fopen(argv[2], "r");
    FILE * input = fopen(argv[1], "rb");
    FILE * output = fopen(argv[3], "wb");

    if (!meta || !input || !output){
        printf("Erreur d'ouverture de fichier");
        return 1;
    }

    fscanf(meta, "%d %d", &height, &width);
    fclose(meta);

    buffer = (unsigned char *)malloc(sizeof(unsigned char) * height * width);
    if(!buffer){
        printf("Erreur d'allocation mémoire\n");
        return 1;
    }

    fread(buffer, 1, height * width, input);
    fclose(input);

    for(i=0; i < (height * width); i++){
        T1[(int)buffer[i]].occ++;
    }

    mergesort(T1, 0, 255);

    pt1 = h;
    for(i=0; i<256; i++){
        if(T1[i].occ>=1){  /* we create a list of 'clip' which correspond to all the present pixel in the image */
            if(h->tree==NULL){
                h->tree=(node *)malloc(sizeof(node));
                h->tree->val=T1[i].val;
                h->tree->occ=T1[i].occ;
                h->tree->fg=NULL;
                h->tree->fd=NULL;
            }
            else{
                pt1->next=(clip *)malloc(sizeof(clip));
                pt1->next->tree=(node *)malloc(sizeof(node));
                pt1->next->tree->val=T1[i].val;
                pt1->next->tree->occ=T1[i].occ;
                pt1->next->tree->fg=NULL;
                pt1->next->tree->fd=NULL;
                pt1 = pt1->next;
                pt1->next=NULL;
            }
        }
    }
    free(T1);

    h = huffman_tree(h); /* then we use this list to create the Huffman tree */

    T2 = (code_case *)malloc(sizeof(code_case)*256);
    CODE=(unsigned char *)malloc(sizeof(unsigned char));
    for(i=0; i<256; i++){
        T2[i].bit_nbr=-1;
        T2[i].code=NULL;
    }
    *CODE=0;

    encode(h->tree, 0, T2, -1, CODE);

    out_write(output, height, width, buffer, T2, &rest);
    fwrite(&rest, 1, 1, output);  /* we write a last byte who correspond to the number of 'unnecessary' 0 */

    free_huffman_tree(h->tree);
    free(h);
    free(buffer);
    for (i = 0; i < 256; i++){
        if(T2[i].code != NULL){
            free(T2[i].code);
        }
    }
    free(T2);

    fclose(output);

    return 0;
}
