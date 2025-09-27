#include<stdio.h>
#include<stdlib.h>

typedef struct table{
    int val;
    int bit_nbr;
    unsigned char * code;
}table;

typedef struct bit_reader{
    unsigned char *data;
    int size;
    int byte_pos;
    int bit_pos;
}bit_reader;

void fusion(table * a, int lo, int mid, int hi){
    int i=lo;
    int j=mid+1;
    int k;
    int l;
    table * aux;

    aux = (table *)malloc(sizeof(table)*(hi-lo+1));
    for(k=lo; k<=hi; k++){
        aux[k-lo].val = a[k].val;
        aux[k-lo].bit_nbr = a[k].bit_nbr;
        aux[k-lo].code = a[k].code;
    }

    for(k=lo; k<=hi; k++){
        if(i>mid){
            a[k].val = aux[j-lo].val;
            a[k].bit_nbr = aux[j-lo].bit_nbr;
            a[k].code = aux[j-lo].code;
            j++;
        }
        else if(j>hi){
            a[k].val = aux[i-lo].val;
            a[k].bit_nbr = aux[i-lo].bit_nbr;
            a[k].code = aux[i-lo].code;
            i++;
        }
        else if(aux[j-lo].bit_nbr < aux[i-lo].bit_nbr){
            a[k].val = aux[j-lo].val;
            a[k].bit_nbr = aux[j-lo].bit_nbr;
            a[k].code = aux[j-lo].code;
            j++;
        }
        else{
            a[k].val = aux[i-lo].val;
            a[k].bit_nbr = aux[i-lo].bit_nbr;
            a[k].code = aux[i-lo].code;
            i++;
        }
    }

    free(aux);
}

void mergesort(table * a, int lo, int hi){
    int mid;

    if(lo>=hi) return;

    mid = lo + (hi-lo)/2;
    mergesort(a, lo, mid);
    mergesort(a, mid+1, hi);
    fusion(a, lo, mid, hi);
}

unsigned char reverse_oct(unsigned char oct){
    int i;
    unsigned char res;
    res=0;
    for(i=0; i<8; i++){
        res = (res << 1);
        res = res | ((oct >> i) & 1);
    }

    return res;
}

void init_bitreader(bit_reader * br, unsigned char * data, int size){
    br->data = data;
    br->size = size;
    br->byte_pos = 1;   /* the firs byte is about the last 0s that where in the last byte of the compression */
    br->bit_pos = reverse_oct(br->data[0]);   /* so we pass the number of 'false(s)' 0 */
}

int read_bit(bit_reader * br){
    if(br->byte_pos >= br->size){
        return -1; // end of buffer
    }
    unsigned char current = br->data[br->byte_pos];
    int bit = (current >> (7 - br->bit_pos)) & 1;

    br->bit_pos++;
    if (br->bit_pos == 8){
        br->bit_pos = 0;
        br->byte_pos++;
    }
    return bit;
}

int main(int argc, char * argv[]){
    int i;
    int j;
    int k;
    int l;
    int m;
    FILE * input;
    FILE * output;
    long oct_nbr;
    unsigned char * meta;
    int height;
    int width;
    table * T1;
    int ok=1;
    unsigned char buffer1;
    unsigned char buffer2;
    unsigned char buffer3;
    unsigned char * buffer4;
    table * T2;
    int hi;
    int oct_count=4;
    unsigned char * buffer;

    input = fopen(argv[1], "rb");
    output = fopen(argv[2], "wb");

    fseek(input, 0, SEEK_END);  /* to get the global size of the compressed image */
    oct_nbr = ftell(input);
    rewind(input);

    if(!input || !output){
        printf("Erreur d'ouverture de fichier");
        return 1;
    }

    meta=(unsigned char *)malloc(sizeof(unsigned char)*4);
    for(i=3; i>=0; i--){
        fread(&meta[i], 1, 1, input);
    }
    height = (meta[3] << 8) + meta[2];
    width = (meta[1] << 8) + meta[0];
    free(meta);

    T1 = (table *)malloc(sizeof(table)*256);
    for(i=0; i<256; i++){
        T1[i].val = -1;
        T1[i].bit_nbr = -1;
    }

    while(ok){
        fread(&buffer1, 1, 1, input);  /* take all the pixel value present in the image with their bits number and associate code */
        fread(&buffer2, 1, 1, input);
        if(buffer2!=0){
            T1[buffer1].val = buffer1;
            T1[buffer1].bit_nbr = buffer2;
            T1[buffer1].code = (unsigned char *)malloc(sizeof(unsigned char)*((buffer2+7)/8));
            for(i=0; i<((buffer2+7)/8); i++){
                fread(&buffer3, 1, 1, input);
                T1[buffer1].code[i] = buffer3;
            }
            oct_count += (1 + 1 + (buffer2+7)/8);
        }
        else{       /* we make sure to stop read T1 whene there is a 0 for the bits nbr, which is impossible by definition of my compression(that mean we have to read the image content now) */
            ok=0;
        }
    }
    oct_count += 2; /* its the two bytes of 0 use for the delimitation */

    mergesort(T1, 0, 255);

    j=0;
    for(i=0; i<256; i++){  /* we travel T1 (who is sorted) until we found a true val, that mean we can take the rest of the T1 values from here, and so the size of T2 is 255-i */
        if(ok==0 && T1[i].val != -1){
            T2 = (table *)malloc(sizeof(table)*(256-i));
            hi = (256-i);
            T2[j].val = T1[i].val;
            T2[j].bit_nbr = T1[i].bit_nbr;
            T2[j].code = (unsigned char *)malloc(sizeof(unsigned char)*((T1[i].bit_nbr+7)/8));
            for(k=1; k<=((T1[i].bit_nbr+7)/8); k++){
                T2[j].code[(T1[i].bit_nbr+7)/8 - k] = reverse_oct(T1[i].code[k-1]);
            }
            free(T1[i].code);
            j++;
            ok=1;
        }
        else if(ok){
            T2[j].val = T1[i].val;
            T2[j].bit_nbr = T1[i].bit_nbr;
            T2[j].code = (unsigned char *)malloc(sizeof(unsigned char)*((T1[i].bit_nbr+7)/8));
            for(k=1; k<=((T1[i].bit_nbr+7)/8); k++){
                T2[j].code[(T1[i].bit_nbr+7)/8 - k] = reverse_oct(T1[i].code[k-1]);
            }
            free(T1[i].code);
            j++;
        }
    }
    free(T1);

    buffer4 = (unsigned char *)malloc(sizeof(unsigned char)*(oct_nbr-oct_count)); /* buffer4 contain all the image content, but we have to totally reverse it cause its more simple to read left to right */
    for(i=1; i<=(oct_nbr-oct_count); i++){
        fread(&buffer1, 1, 1, input);
        buffer4[oct_nbr-oct_count-i]=reverse_oct(buffer1);
    }
    fclose(input);

    buffer = (unsigned char *)malloc(sizeof(unsigned char) * (width * height));
    bit_reader br;
    init_bitreader(&br, buffer4, oct_nbr - oct_count);

    int found;
    int bitcount;
    int matched;
    int save_byte;
    int save_bit;
    int bit;
    int expected;

    i = 0;
    while (i < width * height){  /* for each pixels of the image compressed, we try to recover his real value from his compressed(Huffman) code */
        found = 0;

        // we try all the Huffman code possible
        for (l = 0; l < hi && !found; l++){
            bitcount = T2[l].bit_nbr;
            matched = 1;

            // saving the position in the bitstream
            save_byte = br.byte_pos;
            save_bit = br.bit_pos;

            // read bit for bit and compare to the expected code
            for (m = 0; m < bitcount; m++){
                bit = read_bit(&br);
                if (bit == -1){
                    printf("Erreur: fin de flux\n");
                    exit(1);
                }

                expected = (T2[l].code[m / 8] >> (7 - (m % 8))) & 1;
                if (bit != expected){
                    matched = 0;
                    break;
                }
            }

            if(matched){
                buffer[i] = T2[l].val;
                found = 1;
            }
            else{
                // if not good, we go back to the saved position
                br.byte_pos = save_byte;
                br.bit_pos = save_bit;
            }
        }

        if(!found){
            printf("Erreur: aucun code trouvé à la position %d\n", i);
            exit(1);
        }

        i++;
    }

    for (i=0; i<hi; i++){
        free(T2[i].code);
    }
    free(T2);
    free(buffer4);

    // final writing (we don't forget to reverse all again)
    for (i = 1; i <= width * height; i++){
        fwrite(&buffer[(width * height)-i], 1, 1, output);
    }
    free(buffer);
    fclose(output);

    return 0;
}
