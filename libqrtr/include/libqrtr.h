/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_QRTR_H
#define LIB_QRTR_H

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#include <linux/qrtr.h>

#ifdef __cplusplus
extern "C" {
#endif


struct qrtr_packet {
	int type;
	uint32_t node;
	uint32_t port;
	uint32_t service;
	uint32_t instance;
	uint32_t version;
	void *data;
	size_t data_len;
};


__attribute__((visibility("default")))
int qrtr_open(int rport);


__attribute__((visibility("default")))
int qrtr_close(int sock);


__attribute__((visibility("default")))
int qrtr_sendto(int sock, uint32_t node, uint32_t port, const void *buf,
		unsigned int len);


__attribute__((visibility("default")))
int qrtr_new_server(int sock, uint32_t service, uint16_t version,
		    uint16_t instance);

__attribute__((visibility("default")))
int qrtr_remove_server(int sock, uint32_t service, uint16_t version,
		       uint16_t instance);

__attribute__((visibility("default")))
int qrtr_new_lookup(int sock, uint32_t service, uint16_t version,
		    uint16_t instance);

__attribute__((visibility("default")))
int qrtr_remove_lookup(int sock, uint32_t service, uint16_t version,
		       uint16_t instance);


__attribute__((visibility("default")))
int qrtr_publish(int sock, uint32_t service, uint16_t version,
		 uint16_t instance);

__attribute__((visibility("default")))
int qrtr_bye(int sock, uint32_t service, uint16_t version, uint16_t instance);


__attribute__((visibility("default")))
int qrtr_poll(int sock, unsigned int ms);


__attribute__((visibility("default")))
int qrtr_recv(int sock, void *buf, unsigned int len);


__attribute__((visibility("default")))
int qrtr_recvfrom(int sock, void *buf, unsigned int len, uint32_t *node,
		  uint32_t *port);


__attribute__((visibility("default")))
int qrtr_decode(struct qrtr_packet *pkt, const void *buf, size_t len,
		const struct sockaddr_qrtr *sq);

#ifdef __cplusplus
}
#endif

#endif
