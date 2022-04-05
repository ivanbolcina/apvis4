#include <stdio.h>
#include <string>
#include <gtkmm.h>
#include <giomm/resource.h>
//#include <epoxy/gl.h>
//#include <epoxy/glx.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include "utils.h"

// From Stack Overflow, with thanks:
// http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
// minor mods to make independent of C99.
// more significant changes make it not malloc memory
// needs to initialise the docoding table first

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'
};
static int decoding_table[256]; // an incoming char can range over ASCII, but by mistake could be all 8 bits.

static int mod_table[] = {0, 2, 1};

void initialise_decoding_table() {
    int i;
    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

// pass in a pointer to the data, its length, a pointer to the output buffer and a pointer to an int containing its maximum length
// the actual length will be returned.

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    char *encoded_data,
                    size_t *output_length) {

    size_t calculated_output_length = 4 * ((input_length + 2) / 3);
    if (calculated_output_length > *output_length)
        return (NULL);
    *output_length = calculated_output_length;

    int i, j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char) data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char) data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char) data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


// pass in a pointer to the data, its length, a pointer to the output buffer and a pointer to an int containing its maximum length
// the actual length will be returned.
int base64_decode(const char *data,
                  size_t input_length,
                  unsigned char *decoded_data,
                  size_t *output_length) {

    //remember somewhere to call initialise_decoding_table();

    if (input_length % 4 != 0) return -1;

    size_t calculated_output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') calculated_output_length--;
    if (data[input_length - 2] == '=') calculated_output_length--;
    if (calculated_output_length > *output_length)
        return (-1);
    *output_length = calculated_output_length;

    int i, j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
                          + (sextet_b << 2 * 6)
                          + (sextet_c << 1 * 6)
                          + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return 0;
}


void compute_mvp(float *res,
                        float phi,
                        float theta,
                        float psi) {
    float x{phi * ((float) G_PI / 180.f)};
    float y{theta * ((float) G_PI / 180.f)};
    float z{psi * ((float) G_PI / 180.f)};
    float c1{cosf(x)};
    float s1{sinf(x)};
    float c2{cosf(y)};
    float s2{sinf(y)};
    float c3{cosf(z)};
    float s3{sinf(z)};
    float c3c2{c3 * c2};
    float s3c1{s3 * c1};
    float c3s2s1{c3 * s2 * s1};
    float s3s1{s3 * s1};
    float c3s2c1{c3 * s2 * c1};
    float s3c2{s3 * c2};
    float c3c1{c3 * c1};
    float s3s2s1{s3 * s2 * s1};
    float c3s1{c3 * s1};
    float s3s2c1{s3 * s2 * c1};
    float c2s1{c2 * s1};
    float c2c1{c2 * c1};

    /* apply all three rotations using the three matrices:
     *
     * ⎡  c3 s3 0 ⎤ ⎡ c2  0 -s2 ⎤ ⎡ 1   0  0 ⎤
     * ⎢ -s3 c3 0 ⎥ ⎢  0  1   0 ⎥ ⎢ 0  c1 s1 ⎥
     * ⎣   0  0 1 ⎦ ⎣ s2  0  c2 ⎦ ⎣ 0 -s1 c1 ⎦
     */
    res[0] = c3c2;
    res[4] = s3c1 + c3s2s1;
    res[8] = s3s1 - c3s2c1;
    res[12] = 0.f;
    res[1] = -s3c2;
    res[5] = c3c1 - s3s2s1;
    res[9] = c3s1 + s3s2c1;
    res[13] = 0.f;
    res[2] = s2;
    res[6] = -c2s1;
    res[10] = c2c1;
    res[14] = 0.f;
    res[3] = 0.f;
    res[7] = 0.f;
    res[11] = 0.f;
    res[15] = 1.f;
}
