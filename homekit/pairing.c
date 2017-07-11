#include <espressif/esp_common.h>
#include <string.h>

#include "crypto/tlv.h"
#include "crypto/crypto.h"
#include "tcp.h"
#include "enum.h"
#include "pairing.h"

void handle_M1(struct tcp_pcb *pcb) {
    srp_start();
    uint8_t pairing_state = PAIR_M2;

    uint8_t* data = (uint8_t*)malloc(500);
    uint16_t d_length = 0x00;

    tlv_encode_next(data, &d_length, TYPE_STATE, sizeof(pairing_state), &pairing_state);
    tlv_encode_next(data, &d_length, TYPE_SALT, 16, srp_getSalt());
    tlv_encode_next(data, &d_length, TYPE_PUBLIC_KEY, 384, srp_getB());

    send_response(pcb, data, d_length);
    free(data);
}


void handle_M3(struct tcp_pcb *pcb) {
    uint8_t pairing_state = PAIR_M4;

    uint8_t* data = (uint8_t*)malloc(100);
    uint16_t d_length = 0x00;

    tlv_encode_next(data, &d_length, TYPE_STATE, sizeof(pairing_state), &pairing_state);
    tlv_encode_next(data, &d_length, TYPE_PROOF, 64, srp_getM2());

    //send_response(pcb, data, d_length);
    free(data);
}

void pairing_handler(uint8_t state, uint8_t* payload, uint16_t p_length, struct tcp_pcb *pcb) {
    switch(state) {
        case PAIR_M1: 
            printf("[PAIRING] Step M1-M2!\n");
            handle_M1(pcb); 
            break;
        case PAIR_M3:
            printf("[PAIRING] Step M3-M4!\n");
            uint8_t type; 
            uint16_t length;
            uint8_t* value;
            bool halt = false;

            while (tlv_decode_next(&payload, &p_length, &halt, &type, &length, &value)) {
                printf("[PAIRING] Received (%d) (%d bytes) ", type, length);
                switch(type) {
                    case TYPE_PUBLIC_KEY:
                        printf("Public Key.\n");
                        /*
                        if (!srp_setA(value, length, pairing_send_auth_write_reply)) {
                            printf("[PAIRING] Error setting Public Key! Aborting...\n");
                        }
                        */
                        break;
                    case TYPE_PROOF:
                        printf("Proof.\n");
                        if (!srp_checkM1(value, length)) {
                            printf("[PAIRING] Error setting Proof! Aborting...\n");
                        }
                        break;
                }
            }

            handle_M3(pcb);
            break;
    }
}