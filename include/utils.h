#ifndef UTILS_H
#define UTILS_H

/**
 * Misc utils
 */

/**
 * Initializes base64 decoding
 */
void initialise_decoding_table();

/**
 * Encode base64
 * @param data
 * @param input_length
 * @param encoded_data
 * @param output_length
 * @return
 */
char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    char *encoded_data,
                    size_t *output_length);

/**
 * Decode base64
 * @param data
 * @param input_length
 * @param decoded_data
 * @param output_length
 * @return
 */
int base64_decode(const char *data,
                  size_t input_length,
                  unsigned char *decoded_data,
                  size_t *output_length);

/**
 * Computes opengl model-view-projection matrix
 * @param res
 * @param phi
 * @param theta
 * @param psi
 */
void compute_mvp(float *res,
                        float phi,
                        float theta,
                        float psi);
#endif
