

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "qmi_idl_lib.h"


#define QMI_IDL_EXPORT __attribute__((visibility("default")))


#define QMI_IDL_NOINLINE      __attribute__((noinline))
#define QMI_IDL_ALWAYS_INLINE __attribute__((always_inline))



typedef struct {
	uint16_t qmi_message_id;
	uint16_t message_table_message_id;
	uint16_t max_msg_len;
} qmi_idl_service_message_table_entry;

typedef struct {
	uint32_t c_struct_sz;
	const uint8_t *p_encoded_type_data;
} qmi_idl_type_table_entry;

typedef struct {
	uint32_t c_struct_sz;
	const uint8_t *p_encoded_tlv_data;
} qmi_idl_message_table_entry;

typedef struct {
	int8_t (*range_check)(void *val);
} qmi_idl_range_table_entry;

struct qmi_idl_type_table_object {
	uint16_t n_types;
	uint16_t n_messages;
	uint8_t n_referenced_tables;
	const qmi_idl_type_table_entry *p_types;
	const qmi_idl_message_table_entry *p_messages;
	const struct qmi_idl_type_table_object **p_referenced_tables;
	const qmi_idl_range_table_entry *p_ranges;
};

struct qmi_idl_service_object {
	uint32_t library_version;
	uint32_t idl_version;
	uint32_t service_id;
	uint32_t max_msg_len;
	uint16_t n_msgs[QMI_IDL_NUM_MSG_TYPES];
	const qmi_idl_service_message_table_entry *msgid_to_msg[QMI_IDL_NUM_MSG_TYPES];
	const struct qmi_idl_type_table_object *p_type_table;
	uint32_t idl_minor_version;
	struct qmi_idl_service_object *parent_service_obj;
};

_Static_assert(sizeof(qmi_idl_service_message_table_entry) == 6, "IDL ABI");
_Static_assert(sizeof(qmi_idl_type_table_entry) == 16, "IDL ABI");
_Static_assert(sizeof(qmi_idl_message_table_entry) == 16, "IDL ABI");
_Static_assert(sizeof(struct qmi_idl_type_table_object) == 40, "IDL ABI");
_Static_assert(sizeof(struct qmi_idl_service_object) == 72, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, n_msgs) == 16, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, msgid_to_msg) == 24, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, p_type_table) == 48, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, idl_minor_version) == 56, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, parent_service_obj) == 64, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_type_table_object, p_types) == 8, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_type_table_object, p_messages) == 16, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_type_table_object, p_referenced_tables) == 24, "IDL ABI");
_Static_assert(offsetof(struct qmi_idl_type_table_object, p_ranges) == 32, "IDL ABI");



typedef struct {
	jmp_buf jb;
	int err;
	int v1;
	int v2;
	int v3;
} qmi_idl_lib_exception_type;

_Static_assert(sizeof(jmp_buf) == 0x100, "bionic arm64 jmp_buf is 32 longs");
_Static_assert(offsetof(qmi_idl_lib_exception_type, err) == 0x100, "IDL ABI");
_Static_assert(offsetof(qmi_idl_lib_exception_type, v1) == 0x104, "IDL ABI");
_Static_assert(offsetof(qmi_idl_lib_exception_type, v2) == 0x108, "IDL ABI");
_Static_assert(offsetof(qmi_idl_lib_exception_type, v3) == 0x10c, "IDL ABI");


#define QMI_IDL_LIB_TRY(exc) if(!((exc)->err = setjmp((exc)->jb)))
#define QMI_IDL_LIB_CATCH(exc) else
#define QMI_IDL_HANDLE_ERROR(exc, err_val, value1, value2, value3) \
	do { \
		(exc)->v1 = (value1); \
		(exc)->v2 = (value2); \
		(exc)->v3 = (value3); \
		longjmp((exc)->jb, (err_val)); \
	} while (0)
#define QMI_IDL_LIB_GET_ERROR(exc) ((exc)->err)


#define QMI_IDL_BUF_RANGE(START, END, BUF, LEN, EXC) \
	if (((START) > (BUF)) || ((END) < ((BUF) + (LEN)))) { \
		QMI_IDL_HANDLE_ERROR(EXC, QMI_IDL_LIB_LENGTH_INCONSISTENCY, 0, 0, 0); \
	}



#define QMI_IDL_FLAGS_FIRST_EXTENDED  0x08
#define QMI_IDL_FLAGS_TYPE            0x07
#define QMI_IDL_FLAGS_SECOND_EXTENDED 0x80
#define QMI_IDL_FLAGS_SZ_IS_32        0x40
#define QMI_IDL_FLAGS_ARRAY_DATA_ONLY 0x20
#define QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY   0x10
#define QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS 0x08
#define QMI_IDL_FLAGS_ENUM_IS_UNSIGNED    0x04
#define QMI_IDL_FLAGS_EXTENDED_OFFSET     0x02
#define QMI_IDL_FLAGS_UTF16_STRING        0x01

#define QMI_IDL_FLAGS_THIRD_EXTENDED 0x80
#define QMI_IDL_FLAGS_RANGE_CHECKED  0x40

#define QMI_IDL_FLAG_END_VALUE 0x20

#define QMI_IDL_GET_BITS(VALUE, OFFSET, LENGTH) \
	(((VALUE) >> (OFFSET)) & ((0x1 << (LENGTH)) - 1))
#define QMI_IDL_TYPE_TABLE(type) QMI_IDL_GET_BITS((type), 8, 4)
#define QMI_IDL_TYPE_TYPE(type) \
	(QMI_IDL_GET_BITS((type), 0, 8) | (QMI_IDL_GET_BITS((type), 12, 4) << 8))
#define QMI_IDL_MSG_TYPE_TABLE(type) QMI_IDL_GET_BITS((type), 12, 4)
#define QMI_IDL_MSG_TYPE_TYPE(type) QMI_IDL_GET_BITS((type), 0, 12)

#define QMI_IDL_TLV_FLAGS_LAST_TLV 0x80
#define QMI_IDL_TLV_FLAGS_OPTIONAL 0x40
#define QMI_IDL_TLV_FLAGS_TLV_TYPE_FOR_REQUIRED_TLV 0x0F
#define QMI_IDL_TLV_FLAGS_OFFSET_FOR_OPTIONAL_TLV 0x0F

#define QMI_IDL_GENERIC_1_BYTE 0
#define QMI_IDL_GENERIC_2_BYTE 1
#define QMI_IDL_GENERIC_4_BYTE 2
#define QMI_IDL_GENERIC_8_BYTE 3
#define QMI_IDL_1_BYTE_ENUM    4
#define QMI_IDL_2_BYTE_ENUM    5
#define QMI_IDL_STRING         6
#define QMI_IDL_AGGREGATE      7

#define QMI_IDL_RANGE_RESPONSE_IGNORE  -2
#define QMI_IDL_RANGE_RESPONSE_DEFAULT -1
#define QMI_IDL_RANGE_RESPONSE_SUCCESS 0

#define QMI_IDL_RESP_TLV_LEN 7

#define QMI_IDL_FLAGS_MAX_SZ_IS_16 \
	(0x20 | (QMI_IDL_FLAGS_SZ_IS_32 << 8))

#define GET_8_OR_16(VAR, FLAG, PTR) \
	do { \
		(VAR) = *(PTR)++; \
		if ((FLAG)) { \
			(VAR) |= (*(PTR)++) << 8; \
		} \
	} while (0)

#define GET_16(VAR, PTR) \
	do { \
		(VAR) = *(PTR)++; \
		(VAR) |= (*(PTR)++) << 8; \
	} while (0)

#define BYTE0(VAR) ((uint8_t)((VAR) & 0xFF))
#define BYTE1(VAR) ((uint8_t)(((VAR) >> 8) & 0xFF))


static size_t qmi_idl_strlen(const char *s)
{
	return strlen(s);
}

QMI_IDL_EXPORT qmi_idl_service_object_type qmi_idl_get_inherited_service_object(
	qmi_idl_service_object_type service);


static qmi_idl_service_object_type qmi_idl_inherited_service(
	qmi_idl_service_object_type service)
{
	if (!service || service->library_version < 5) {
		return NULL;
	}
	return service->parent_service_obj;
}


static const uint8_t *qmi_idl_next_element(const uint8_t *p_element)
{
	uint32_t flags = *p_element++;

	if (flags & QMI_IDL_FLAGS_FIRST_EXTENDED) {
		flags |= *p_element++ << 8;
	}
	if (flags & (QMI_IDL_FLAGS_SECOND_EXTENDED << 8)) {
		flags |= *p_element++ << 16;
	}

	if (QMI_IDL_FLAG_END_VALUE == flags) {
		return p_element;
	}

	++p_element;

	if (flags & 0x80) {
		++p_element;
	} else if (flags & (QMI_IDL_FLAGS_EXTENDED_OFFSET << 8)) {
		p_element += 2;
	}

	if (flags & 0x40) {
		++p_element;
	}

	if (flags & QMI_IDL_FLAGS_MAX_SZ_IS_16) {
		++p_element;
	}

	if (flags & (QMI_IDL_FLAGS_SZ_IS_32 << 8)) {
		p_element += 2;
	}

	if ((flags & 0x10) &&
	    ((flags & QMI_IDL_FLAGS_TYPE) != QMI_IDL_STRING) &&
	    !(flags & (QMI_IDL_FLAGS_UTF16_STRING << 8))) {
		++p_element;
	}

	if ((flags & QMI_IDL_FLAGS_TYPE) == QMI_IDL_AGGREGATE) {
		p_element += 2;
	}
	if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
		p_element += 4;
	}

	return p_element;
}


QMI_IDL_EXPORT QMI_IDL_NOINLINE const qmi_idl_message_table_entry *qmi_idl_find_msg(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type message_type,
	uint16_t message_id,
	qmi_idl_lib_exception_type *exc,
	const qmi_idl_type_table_object **out_type_table)
{
	const qmi_idl_service_message_table_entry *p_srvmsg = NULL;
	qmi_idl_service_object_type inheritance_obj;
	uint32_t i;
	uint32_t table_index;
	uint32_t type_index;

	if (!p_service) {
		QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_PARAMETER_ERROR, 0, 0, 0);
	}

	inheritance_obj = p_service;
	for (;;) {
		i = inheritance_obj->n_msgs[message_type];
		if (i != 0) {
			p_srvmsg = inheritance_obj->msgid_to_msg[message_type];
			while (i != 0 && p_srvmsg->qmi_message_id != message_id) {
				++p_srvmsg;
				--i;
			}
			if (i != 0) {
				break;
			}
		}
		inheritance_obj =
			qmi_idl_get_inherited_service_object(inheritance_obj);
		if (!inheritance_obj) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_MESSAGE_ID_NOT_FOUND,
					     message_id, 0, 0);
		}
	}


	table_index = QMI_IDL_MSG_TYPE_TABLE(p_srvmsg->message_table_message_id);
	type_index = QMI_IDL_MSG_TYPE_TYPE(p_srvmsg->message_table_message_id);
	if (out_type_table) {
		*out_type_table = inheritance_obj->p_type_table->
				  p_referenced_tables[table_index];
	}
	return &(inheritance_obj->p_type_table->
		 p_referenced_tables[table_index]->p_messages[type_index]);
}


QMI_IDL_EXPORT QMI_IDL_ALWAYS_INLINE const uint8_t *qmi_idl_decode_find_tlv(const uint8_t *p_element,
						      uint8_t msg_type,
						      int32_t *optional_offset)
{
	uint8_t tlv_flags;
	uint8_t msg_type2;

	for (;;) {
		tlv_flags = *p_element++;

		if (QMI_IDL_TLV_FLAGS_OPTIONAL & tlv_flags) {
			msg_type2 = *p_element++;
			if (msg_type == msg_type2) {
				*optional_offset =
					QMI_IDL_TLV_FLAGS_OFFSET_FOR_OPTIONAL_TLV &
					tlv_flags;
				return p_element;
			}
		} else {
			msg_type2 =
				QMI_IDL_TLV_FLAGS_TLV_TYPE_FOR_REQUIRED_TLV &
				tlv_flags;
			if (msg_type == msg_type2) {
				*optional_offset = 0;
				return p_element;
			}
		}

		if (QMI_IDL_TLV_FLAGS_LAST_TLV & tlv_flags) {
			return NULL;
		}

		p_element = qmi_idl_next_element(p_element);
	}
}


QMI_IDL_EXPORT QMI_IDL_NOINLINE void qmi_idl_decode_verify_mandatory(const uint8_t *p_element,
						    uint8_t *found,
						    qmi_idl_lib_exception_type *exc)
{
	uint8_t tlv_flags;
	uint8_t msg_type;

	if (p_element == NULL) {
		return;
	}

	for (;;) {
		tlv_flags = *p_element++;

		if (!(QMI_IDL_TLV_FLAGS_OPTIONAL & tlv_flags)) {
			msg_type =
				QMI_IDL_TLV_FLAGS_TLV_TYPE_FOR_REQUIRED_TLV &
				tlv_flags;
			if (!found[msg_type]) {
				QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_MISSING_TLV,
						     msg_type, 0, 0);
			}
		} else {
			break;
		}

		if (QMI_IDL_TLV_FLAGS_LAST_TLV & tlv_flags) {
			break;
		}

		p_element = qmi_idl_next_element(p_element);
	}
}


static uint32_t qmi_idl_encode_length(uint8_t **pp_dst, uint32_t dst_bytes,
				      uint32_t flags, uint32_t length,
				      qmi_idl_lib_exception_type *exc)
{
	if (!(flags & (0x20 | (QMI_IDL_FLAGS_SZ_IS_32 << 8)))) {
		if (dst_bytes == 0) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     11, 0, 0);
		}

		**pp_dst = length & 0xFF;
		(*pp_dst)++;
		dst_bytes--;
	} else if (flags & 0x20) {
		if (dst_bytes < 2) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     12, 0, 0);
		}

		**pp_dst = length & 0xFF;
		(*pp_dst)++;
		**pp_dst = (length >> 8) & 0xFF;
		(*pp_dst)++;
		dst_bytes -= 2;
	} else {
		if (dst_bytes < 4) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     13, 0, 0);
		}

		**pp_dst = length & 0xFF;
		(*pp_dst)++;
		**pp_dst = (length >> 8) & 0xFF;
		(*pp_dst)++;
		**pp_dst = 0;
		(*pp_dst)++;
		**pp_dst = 0;
		(*pp_dst)++;
		dst_bytes -= 4;
	}

	return dst_bytes;
}


static uint32_t qmi_idl_decode_element(
	uint8_t *p_dst, const uint8_t **pp_src, uint32_t src_len,
	int32_t optional_offset, const uint8_t **pp_element,
	const qmi_idl_type_table_object *p_type_table,
	qmi_idl_lib_exception_type *exc, uint8_t *range_error_ignore,
	uint8_t *p_dst_start, uint8_t *p_dst_end)
{
	const uint8_t *p_element = *pp_element;
	const uint8_t *p_element2;
	uint8_t *valid_flag = NULL;
	uint32_t flags;
	uint32_t array_max_size;
	uint32_t length;
	uint32_t i;
	uint32_t ag_type;
	uint32_t table_index;
	uint32_t type_index;
	uint32_t c_struct_sz;
	uint32_t range_index = 0;

	flags = *p_element++;
	if (flags & QMI_IDL_FLAGS_FIRST_EXTENDED) {
		flags |= *p_element++ << 8;
	}
	if (flags & (QMI_IDL_FLAGS_SECOND_EXTENDED << 8)) {
		flags |= *p_element++ << 16;
	}


	p_dst += *p_element++;
	if (flags & 0x80) {
		p_dst += (uint8_t)(*p_element++) << 8;
	} else if (flags & (QMI_IDL_FLAGS_EXTENDED_OFFSET << 8)) {
		p_dst += (uint8_t)(*p_element++) << 8;
		p_dst += (uint8_t)(*p_element++) << 16;
	}

	if (optional_offset > 0) {

		QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, (p_dst - optional_offset),
				  0, exc);
		*(uint8_t *)(p_dst - optional_offset) = 1;
		valid_flag = (p_dst - optional_offset);
	}

	if (flags & 0x40) {
		GET_8_OR_16(array_max_size, (flags & 0x20), p_element);
		if (flags & (QMI_IDL_FLAGS_SZ_IS_32 << 8)) {
			p_element += 2;
		}
		if (flags & 0x10) {
			if (optional_offset != -1 &&
			    (flags & QMI_IDL_FLAGS_TYPE) == QMI_IDL_STRING) {
				length = src_len;
			} else {
				if (!(flags & (0x20 |
					       (QMI_IDL_FLAGS_SZ_IS_32 << 8) |
					       (QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
					       (QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8)))) {
					if (src_len < 1) {
						QMI_IDL_HANDLE_ERROR(
							exc,
							QMI_IDL_LIB_LENGTH_INCONSISTENCY,
							14, 0, 0);
					}
					length = *(*pp_src)++;
					src_len--;
				} else if (!(flags & ((QMI_IDL_FLAGS_SZ_IS_32 << 8) |
						      (QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
						      (QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8)))) {
					if (src_len < 2) {
						QMI_IDL_HANDLE_ERROR(
							exc,
							QMI_IDL_LIB_LENGTH_INCONSISTENCY,
							14, 0, 0);
					}
					length = *(*pp_src)++;
					length |= (*(*pp_src)++) << 8;
					src_len -= 2;
				} else if (!(flags & ((QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
						      (QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8)))) {
					if (src_len < 4) {
						QMI_IDL_HANDLE_ERROR(
							exc,
							QMI_IDL_LIB_LENGTH_INCONSISTENCY,
							14, 0, 0);
					}
					length = *(*pp_src)++;
					length |= (*(*pp_src)++) << 8;
					*pp_src += 2;
					src_len -= 4;
				} else if (!(flags & (QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8))) {

					length = *(uint32_t *)(p_dst - *p_element);
				} else {
					length = src_len;
					switch (flags & QMI_IDL_FLAGS_TYPE) {
					case QMI_IDL_GENERIC_1_BYTE:
					case QMI_IDL_GENERIC_2_BYTE:
					case QMI_IDL_GENERIC_4_BYTE:
					case QMI_IDL_GENERIC_8_BYTE:
						length = length >>
							 (flags & QMI_IDL_FLAGS_TYPE);
						break;
					case QMI_IDL_2_BYTE_ENUM:
						length = length >> 1;
						break;
					}
				}
			}

			if (((flags & QMI_IDL_FLAGS_TYPE) != QMI_IDL_STRING) &&
			    !(flags & (QMI_IDL_FLAGS_UTF16_STRING << 8))) {

				QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end,
						  (p_dst - *p_element), 4, exc);
				*(uint32_t *)(p_dst - *p_element++) = length;
			}

			if (length > array_max_size) {
				QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_ARRAY_TOO_BIG,
						     length, array_max_size, 0);
			}

			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				if ((flags & QMI_IDL_FLAGS_TYPE) == QMI_IDL_AGGREGATE) {
					p_element += 2;
				}
				*pp_element = p_element;
				return src_len;
			}
		} else {
			length = array_max_size;
		}
	} else {
		length = 1;
	}

	if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
		range_index = (*p_element++);
		range_index |= (*p_element++) << 8;
		range_index |= (*p_element++) << 16;
		range_index |= (*p_element++) << 24;
	}

	*pp_element = p_element;

	switch (flags & QMI_IDL_FLAGS_TYPE) {
	case QMI_IDL_GENERIC_1_BYTE:
	case QMI_IDL_GENERIC_2_BYTE:
	case QMI_IDL_GENERIC_4_BYTE:
	case QMI_IDL_GENERIC_8_BYTE:

		length = length << (flags & QMI_IDL_FLAGS_TYPE);

		if (length > src_len) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     3, length, src_len);
		}
		src_len -= length;
		QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, p_dst, length, exc);
		memcpy(p_dst, *pp_src, length);
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			uint64_t tmp_val = 0;
			length = length >> (flags & QMI_IDL_FLAGS_TYPE);
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				if ((flags & QMI_IDL_FLAGS_TYPE) > 3) {
					QMI_IDL_HANDLE_ERROR(
						exc,
						QMI_IDL_LIB_LENGTH_INCONSISTENCY,
						0, 0, 0);
				}
				memcpy(&tmp_val, *pp_src,
				       (1 << (flags & QMI_IDL_FLAGS_TYPE)));
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val == QMI_IDL_RANGE_RESPONSE_IGNORE) {
					if (optional_offset > 0) {
						*valid_flag = 0;
					} else {
						*range_error_ignore = 1;
					}
				} else if (ret_val > 0) {

					QMI_IDL_HANDLE_ERROR(exc, ret_val, 0, 0, 0);
				} else if (ret_val == QMI_IDL_RANGE_RESPONSE_DEFAULT) {
					uint64_t offset =
						i * (1 << (flags & QMI_IDL_FLAGS_TYPE));
					QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end,
							  (p_dst + offset),
							  (1 << (flags & QMI_IDL_FLAGS_TYPE)),
							  exc);
					memcpy(p_dst + offset, &tmp_val,
					       (1 << (flags & QMI_IDL_FLAGS_TYPE)));
				}
				*pp_src += (1 << (flags & QMI_IDL_FLAGS_TYPE));
			}
		} else {
			*pp_src = *pp_src + length;
		}
		if (flags & (QMI_IDL_FLAGS_UTF16_STRING << 8)) {
			QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, p_dst, length,
					  exc);
			*(p_dst + length) = 0;
		}
		break;

	case QMI_IDL_1_BYTE_ENUM:
		if (length > src_len) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					      0, 0, 0);
		}
		src_len -= length;
		QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, p_dst, (length * 4),
				  exc);
		if (flags & (QMI_IDL_FLAGS_ENUM_IS_UNSIGNED << 8)) {
			for (i = 0; i < length; i++) {
				*(int32_t *)p_dst = (int32_t)(uint8_t) *(*pp_src)++;
				p_dst += 4;
			}
		} else {
			for (i = 0; i < length; i++) {
				*(int32_t *)p_dst = (int32_t)(int8_t) *(*pp_src)++;
				p_dst += 4;
			}
		}
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			int32_t tmp_val = 0;
			p_dst -= 4 * length;
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				memcpy(&tmp_val, p_dst, 4);
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val == QMI_IDL_RANGE_RESPONSE_IGNORE) {
					if (optional_offset > 0) {
						*valid_flag = 0;
					} else {
						*range_error_ignore = 1;
					}
				} else if (ret_val > 0) {
					QMI_IDL_HANDLE_ERROR(exc, ret_val, 0, 0, 0);
				} else if (ret_val == QMI_IDL_RANGE_RESPONSE_DEFAULT) {
					uint64_t offset = i * 4;
					memcpy(p_dst + offset, &tmp_val, 4);
				}
				p_dst += 4;
			}
		}
		break;

	case QMI_IDL_2_BYTE_ENUM:
		if (2 * length > src_len) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     5, length, src_len);
		}
		src_len -= 2 * length;
		QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, p_dst, (length * 4),
				  exc);
		if (flags & (QMI_IDL_FLAGS_ENUM_IS_UNSIGNED << 8)) {
			for (i = 0; i < length; i++) {
				*(int32_t *)p_dst = (int32_t)(uint16_t)(
					(uint16_t)**pp_src |
					(((uint16_t)(*(*pp_src + 1))) << 8));
				p_dst += 4;
				*pp_src += 2;
			}
		} else {
			for (i = 0; i < length; i++) {
				*(int32_t *)p_dst = (int32_t)(int16_t)(
					(uint16_t)**pp_src |
					(((uint16_t)(*(*pp_src + 1))) << 8));
				p_dst += 4;
				*pp_src += 2;
			}
		}
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			int32_t tmp_val = 0;
			p_dst -= 4 * length;
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				memcpy(&tmp_val, p_dst, 4);
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val == QMI_IDL_RANGE_RESPONSE_IGNORE) {
					if (optional_offset > 0) {
						*valid_flag = 0;
					} else {
						*range_error_ignore = 1;
					}
				} else if (ret_val > 0) {
					QMI_IDL_HANDLE_ERROR(exc, ret_val, 0, 0, 0);
				} else if (ret_val == QMI_IDL_RANGE_RESPONSE_DEFAULT) {
					uint64_t offset = i * 4;
					memcpy(p_dst + offset, &tmp_val, 4);
				}
				p_dst += 4;
			}
		}
		break;

	case QMI_IDL_STRING:

		if (length > src_len) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     6, length, src_len);
		}
		src_len -= length;
		QMI_IDL_BUF_RANGE(p_dst_start, p_dst_end, p_dst, length, exc);
		memcpy(p_dst, *pp_src, length);
		*(p_dst + length) = 0;
		*pp_src += length;
		break;

	case QMI_IDL_AGGREGATE:

		GET_16(ag_type, *pp_element);
		table_index = QMI_IDL_TYPE_TABLE(ag_type);
		p_type_table = p_type_table->p_referenced_tables[table_index];
		type_index = QMI_IDL_TYPE_TYPE(ag_type);
		p_element = p_type_table->p_types[type_index].p_encoded_type_data;
		c_struct_sz = p_type_table->p_types[type_index].c_struct_sz;
		for (i = 0; i < length; i++) {
			p_element2 = p_element;

			do {
				src_len = qmi_idl_decode_element(
					p_dst, pp_src, src_len, -1, &p_element2,
					p_type_table, exc, range_error_ignore,
					p_dst_start, p_dst_end);
				if (*range_error_ignore && valid_flag) {
					*valid_flag = 0;
				}
			} while (*p_element2 != QMI_IDL_FLAG_END_VALUE);

			p_dst += c_struct_sz;
		}
		break;
	}

	return src_len;
}


QMI_IDL_EXPORT QMI_IDL_NOINLINE void qmi_idl_message_decode_v6(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type req_resp_ind, uint16_t message_id,
	const void *p_src_void, uint32_t src_len, void *p_dst_void,
	uint32_t dst_len, qmi_idl_lib_exception_type *exc)
{
	const qmi_idl_message_table_entry *p_msg;
	const qmi_idl_type_table_object *msg_type_table;
	uint8_t found[256];
	const uint8_t *p_element;
	const uint8_t *p_src = p_src_void;
	uint8_t *p_dst = p_dst_void;
	uint8_t *p_dst_start, *p_dst_end;

	uint8_t msg_type;
	uint32_t msg_len;
	uint8_t resp_error = 0;
	uint8_t range_err = 0;
	uint32_t bytes_remaining;
	int32_t optional_offset;

	memset(found, 0, 256);

	p_msg = qmi_idl_find_msg(p_service, req_resp_ind, message_id, exc,
				 &msg_type_table);

	if (dst_len < p_msg->c_struct_sz) {
		QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_BUFFER_TOO_SMALL,
				     1, dst_len, p_msg->c_struct_sz);
	}
	p_dst_start = p_dst;
	p_dst_end = p_dst_start + p_msg->c_struct_sz;
	if (p_dst) {
		memset(p_dst, 0, p_msg->c_struct_sz);
	}

	while (src_len) {
		if (src_len < 3) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     13, 3, src_len);
		}

		msg_type = *p_src++;
		GET_16(msg_len, p_src);
		src_len -= 3;

		if (msg_len > src_len) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     14, msg_len, src_len);
		}

		p_element = qmi_idl_decode_find_tlv(p_msg->p_encoded_tlv_data,
						    msg_type, &optional_offset);

		if (p_element != NULL) {
			if (found[msg_type]) {
				QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_TLV_DUPLICATED,
						     msg_type, 0, 0);
			}

			found[msg_type] = 1;
			if (req_resp_ind == QMI_IDL_RESPONSE) {
				if (msg_type == 2) {
					resp_error = p_src[0];
				}
			}
			bytes_remaining = qmi_idl_decode_element(
				p_dst, &p_src, msg_len, optional_offset,
				&p_element, msg_type_table, exc, &range_err,
				p_dst_start, p_dst_end);

			if (bytes_remaining != 0) {
				QMI_IDL_HANDLE_ERROR(
					exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					15, bytes_remaining, 0);
			}

			src_len -= msg_len;
		} else if (msg_type < 0x10) {
			QMI_IDL_HANDLE_ERROR(
				exc, QMI_IDL_LIB_UNKNOWN_MANDATORY_TLV,
				msg_type, 0, 0);
		} else {
			p_src += msg_len;
			src_len -= msg_len;
		}
	}

	if (!resp_error) {
		qmi_idl_decode_verify_mandatory(p_msg->p_encoded_tlv_data,
						found, exc);
	}
}


static uint32_t qmi_idl_encode_element(
	uint8_t **pp_dst, uint32_t dst_bytes, const uint8_t *p_src,
	const uint8_t **pp_element,
	const qmi_idl_type_table_object *p_type_table, int32_t optional_offset,
	qmi_idl_lib_exception_type *exc)
{
	const uint8_t *p_element = *pp_element;
	const uint8_t *p_element2;
	uint32_t flags;
	uint32_t array_max_size;
	uint32_t length;
	uint32_t i;
	uint32_t ag_type;
	uint32_t table_index;
	uint32_t type_index;
	uint32_t c_struct_sz;
	uint32_t range_index = 0;

	flags = *p_element++;
	if (flags & QMI_IDL_FLAGS_FIRST_EXTENDED) {
		flags |= *p_element++ << 8;
	}
	if (flags & (QMI_IDL_FLAGS_SECOND_EXTENDED << 8)) {
		flags |= *p_element++ << 16;
	}

	p_src += *p_element++;
	if (flags & 0x80) {
		p_src += (uint32_t)(*p_element++) << 8;
	} else if (flags & (QMI_IDL_FLAGS_EXTENDED_OFFSET << 8)) {
		p_src += (uint32_t)(*p_element++) << 8;
		p_src += (uint32_t)(*p_element++) << 16;
	}

	if (optional_offset > 0) {

		if (!*(uint8_t *)(p_src - optional_offset)) {
			if (flags & 0x40) {
				if (flags & 0x20) {
					p_element += 2;
				} else {
					p_element += 1;
				}

				if ((flags & 0x10) &&
				    ((flags & QMI_IDL_FLAGS_TYPE) != QMI_IDL_STRING) &&
				    !(flags & (QMI_IDL_FLAGS_UTF16_STRING << 8))) {
					p_element += 1;
				}
			}

			if ((flags & QMI_IDL_FLAGS_TYPE) == QMI_IDL_AGGREGATE) {
				p_element += 2;
			}
			if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
				p_element += 4;
			}
			*pp_element = p_element;
			return dst_bytes;
		}
	}

	if (optional_offset != -1) {

		if (dst_bytes < 3) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_BUFFER_TOO_SMALL,
					     2, dst_bytes, 3);
		}

		*pp_dst = *pp_dst + 3;
		dst_bytes -= 3;
	}

	if ((flags & QMI_IDL_FLAGS_TYPE) == QMI_IDL_STRING) {
		GET_8_OR_16(array_max_size, (flags & 0x20), p_element);
		if (flags & (QMI_IDL_FLAGS_SZ_IS_32 << 8)) {
			p_element += 2;
		}
		length = qmi_idl_strlen((char *)p_src);
		if (length > array_max_size) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_ARRAY_TOO_BIG,
					     length, array_max_size, 0);
		}

		if (optional_offset == -1 &&
		    (!(flags & (QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8)))) {
			dst_bytes = qmi_idl_encode_length(pp_dst, dst_bytes, flags,
							  length, exc);
			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				*pp_element = p_element;
				return dst_bytes;
			}
		}

		if (length > dst_bytes) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     7, length, dst_bytes);
		}
		dst_bytes -= length;

		memcpy(*pp_dst, p_src, length);
		*pp_dst = *pp_dst + length;
	} else if (flags & (QMI_IDL_FLAGS_UTF16_STRING << 8)) {

		uint16_t character;
		GET_8_OR_16(array_max_size, (flags & 0x20), p_element);
		if (flags & (QMI_IDL_FLAGS_SZ_IS_32 << 8)) {
			p_element += 2;
		}
		character = (*p_src++);
		character |= (*p_src++ << 8);
		length = 0;
		while (character != 0) {
			length++;
			character = (*p_src++);
			character |= (*p_src++ << 8);
		}
		if (length > array_max_size) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_ARRAY_TOO_BIG,
					     length, array_max_size, 0);
		}
		p_src -= (length * 2 + 2);
	} else if (flags & 0x40) {
		GET_8_OR_16(array_max_size, (flags & 0x20), p_element);
		if (flags & (QMI_IDL_FLAGS_SZ_IS_32 << 8)) {
			p_element += 2;
		}
		if (flags & 0x10) {

			length = *(uint32_t *)(p_src - *p_element++);

			if (length > array_max_size) {
				QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_ARRAY_TOO_BIG,
						     length, array_max_size, 0);
			}
		} else {
			length = array_max_size;
		}
	} else {
		length = 1;
	}

	if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
		range_index = (*p_element++);
		range_index |= ((*p_element++) << 8);
		range_index |= ((*p_element++) << 16);
		range_index |= ((*p_element++) << 24);
	}

	*pp_element = p_element;

	switch (flags & QMI_IDL_FLAGS_TYPE) {
	case QMI_IDL_GENERIC_1_BYTE:
	case QMI_IDL_GENERIC_2_BYTE:
	case QMI_IDL_GENERIC_4_BYTE:
	case QMI_IDL_GENERIC_8_BYTE:
		if ((flags & 0x40) && (flags & 0x10) &&
		    (!(flags & ((QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
				(QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8))))) {
			dst_bytes = qmi_idl_encode_length(pp_dst, dst_bytes, flags,
							  length, exc);
			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				return dst_bytes;
			}
		}

		length = length << (flags & QMI_IDL_FLAGS_TYPE);

		if (length > dst_bytes) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     8, length, dst_bytes);
		}
		dst_bytes -= length;

		memcpy(*pp_dst, p_src, length);
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			uint64_t tmp_val = 0;
			length = length >> (flags & QMI_IDL_FLAGS_TYPE);
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				memcpy(&tmp_val, p_src,
				       (1 << (flags & QMI_IDL_FLAGS_TYPE)));
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val != QMI_IDL_RANGE_RESPONSE_SUCCESS) {
					QMI_IDL_HANDLE_ERROR(
						exc, QMI_IDL_LIB_RANGE_FAILURE,
						0, 0, 0);
				}
				*pp_dst += (1 << (flags & QMI_IDL_FLAGS_TYPE));
			}
		} else {
			*pp_dst = *pp_dst + length;
		}
		break;

	case QMI_IDL_1_BYTE_ENUM:
		if ((flags & 0x40) && (flags & 0x10) &&
		    (!(flags & ((QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
				(QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8))))) {
			dst_bytes = qmi_idl_encode_length(pp_dst, dst_bytes, flags,
							  length, exc);
			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				return dst_bytes;
			}
		}

		if (length > dst_bytes) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     9, length, dst_bytes);
		}
		dst_bytes -= length;

		for (i = 0; i < length; i++) {
			*(*pp_dst)++ = *p_src;
			p_src += 4;
		}
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			uint32_t tmp_val = 0;
			p_src -= 4 * length;
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				memcpy(&tmp_val, p_src, 4);
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val != QMI_IDL_RANGE_RESPONSE_SUCCESS) {
					QMI_IDL_HANDLE_ERROR(
						exc, QMI_IDL_LIB_RANGE_FAILURE,
						0, 0, 0);
				}
				p_src += 4;
			}
		}
		break;

	case QMI_IDL_2_BYTE_ENUM:
		if ((flags & 0x40) && (flags & 0x10) &&
		    (!(flags & ((QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
				(QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8))))) {
			dst_bytes = qmi_idl_encode_length(pp_dst, dst_bytes, flags,
							  length, exc);
			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				return dst_bytes;
			}
		}

		if (2 * length > dst_bytes) {
			QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					     10, length, dst_bytes);
		}
		dst_bytes -= 2 * length;

		for (i = 0; i < length; i++) {
			*(*pp_dst)++ = *p_src++;
			*(*pp_dst)++ = *p_src;
			p_src += 3;
		}
		if (flags & (QMI_IDL_FLAGS_RANGE_CHECKED << 16)) {
			int8_t ret_val = 0;
			uint32_t tmp_val = 0;
			p_src -= 4 * length;
			for (i = 0; i < length; i++) {
				tmp_val = 0;
				memcpy(&tmp_val, p_src, 4);
				ret_val = p_type_table->p_ranges[range_index].
					  range_check(&tmp_val);
				if (ret_val != QMI_IDL_RANGE_RESPONSE_SUCCESS) {
					QMI_IDL_HANDLE_ERROR(
						exc, QMI_IDL_LIB_RANGE_FAILURE,
						0, 0, 0);
				}
				p_src += 4;
			}
		}
		break;

	case QMI_IDL_STRING:

		break;

	case QMI_IDL_AGGREGATE:
		if ((flags & 0x40) && (flags & 0x10) &&
		    (!(flags & ((QMI_IDL_FLAGS_ARRAY_DATA_ONLY << 8) |
				(QMI_IDL_FLAGS_ARRAY_IS_LENGTHLESS << 8))))) {
			dst_bytes = qmi_idl_encode_length(pp_dst, dst_bytes, flags,
							  length, exc);
			if (flags & (QMI_IDL_FLAGS_ARRAY_LENGTH_ONLY << 8)) {
				*pp_element += 2;
				return dst_bytes;
			}
		}

		GET_16(ag_type, *pp_element);
		table_index = QMI_IDL_TYPE_TABLE(ag_type);
		p_type_table = p_type_table->p_referenced_tables[table_index];

		type_index = QMI_IDL_TYPE_TYPE(ag_type);
		p_element = p_type_table->p_types[type_index].p_encoded_type_data;
		c_struct_sz = p_type_table->p_types[type_index].c_struct_sz;

		for (i = 0; i < length; i++) {
			p_element2 = p_element;

			do {
				dst_bytes = qmi_idl_encode_element(
					pp_dst, dst_bytes, p_src, &p_element2,
					p_type_table, -1, exc);
			} while (*p_element2 != QMI_IDL_FLAG_END_VALUE);

			p_src += c_struct_sz;
		}
		break;
	}

	return dst_bytes;
}


static uint32_t qmi_idl_message_encode_v6(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type req_resp_ind, uint16_t message_id,
	const void *p_src, uint32_t src_len, void *p_dst, uint32_t dst_len,
	qmi_idl_lib_exception_type *exc)
{
	const qmi_idl_message_table_entry *p_msg;
	const uint8_t *p_element;
	const qmi_idl_type_table_object *msg_type_table;
	uint8_t *p_dst2 = p_dst;
	int32_t optional_offset;
	uint8_t msg_type;
	uint8_t *p_hdr;
	uint32_t new_dst_len;
	uint32_t length;
	uint32_t total_length;
	uint8_t tlv_flags;

	p_msg = qmi_idl_find_msg(p_service, req_resp_ind, message_id, exc,
				 &msg_type_table);

	p_element = p_msg->p_encoded_tlv_data;
	total_length = 0;

	if (src_len < p_msg->c_struct_sz) {
		QMI_IDL_HANDLE_ERROR(exc, QMI_IDL_LIB_BUFFER_TOO_SMALL,
				     1, dst_len, p_msg->c_struct_sz);
	} else if (p_msg->c_struct_sz == 0 || dst_len == 0 ||
		   p_element == NULL) {
		return total_length;
	}
	do {
		tlv_flags = *p_element++;

		if (QMI_IDL_TLV_FLAGS_OPTIONAL & tlv_flags) {
			msg_type = *p_element++;
			optional_offset =
				QMI_IDL_TLV_FLAGS_OFFSET_FOR_OPTIONAL_TLV &
				tlv_flags;
		} else {
			msg_type =
				QMI_IDL_TLV_FLAGS_TLV_TYPE_FOR_REQUIRED_TLV &
				tlv_flags;
			optional_offset = 0;
		}

		p_hdr = p_dst2;

		new_dst_len = qmi_idl_encode_element(&p_dst2, dst_len, p_src,
						     &p_element, msg_type_table,
						     optional_offset, exc);

		if (new_dst_len != dst_len) {
			if (dst_len < (3 + new_dst_len)) {
				QMI_IDL_HANDLE_ERROR(
					exc, QMI_IDL_LIB_LENGTH_INCONSISTENCY,
					0, 0, 0);
			}
			length = dst_len - 3 - new_dst_len;
			*p_hdr++ = msg_type;
			*p_hdr++ = BYTE0(length);
			*p_hdr++ = BYTE1(length);
			total_length += length + 3;
			dst_len = new_dst_len;
		}
	} while (!(QMI_IDL_TLV_FLAGS_LAST_TLV & tlv_flags));

	return total_length;
}


QMI_IDL_EXPORT int32_t qmi_idl_message_decode(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type req_resp_ind, uint16_t message_id,
	const void *p_src, uint32_t src_len, void *p_dst, uint32_t dst_len)
{
	qmi_idl_lib_exception_type exc;

	QMI_IDL_LIB_TRY(&exc) {
		if (p_service == NULL ||
		    req_resp_ind >= QMI_IDL_NUM_MSG_TYPES ||
		    (p_src == NULL && src_len > 0) ||
		    (p_dst == NULL && dst_len > 0)) {
			QMI_IDL_HANDLE_ERROR(&exc, QMI_IDL_LIB_PARAMETER_ERROR,
					     0, 0, 0);
		}

		switch (p_service->library_version) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			qmi_idl_message_decode_v6(p_service, req_resp_ind,
						  message_id, p_src, src_len,
						  p_dst, dst_len, &exc);
			break;
		default:
			QMI_IDL_HANDLE_ERROR(
				&exc, QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION,
				p_service->library_version, 0, 0);
			break;
		}
	}
	QMI_IDL_LIB_CATCH(&exc) {
		return QMI_IDL_LIB_GET_ERROR(&exc);
	}

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT int32_t qmi_idl_message_encode(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type req_resp_ind, uint16_t message_id,
	const void *p_src, uint32_t src_len, void *p_dst, uint32_t dst_len,
	uint32_t *dst_encoded_len)
{
	qmi_idl_lib_exception_type exc;

	QMI_IDL_LIB_TRY(&exc) {
		if (p_service == NULL ||
		    req_resp_ind >= QMI_IDL_NUM_MSG_TYPES ||
		    p_src == NULL ||
		    p_dst == NULL) {
			QMI_IDL_HANDLE_ERROR(&exc, QMI_IDL_LIB_PARAMETER_ERROR,
					     0, 0, 0);
		}
		switch (p_service->library_version) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			*dst_encoded_len = qmi_idl_message_encode_v6(
				p_service, req_resp_ind, message_id, p_src,
				src_len, p_dst, dst_len, &exc);
			break;
		default:
			QMI_IDL_HANDLE_ERROR(
				&exc, QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION,
				p_service->library_version, 0, 0);
			break;
		}
	}
	QMI_IDL_LIB_CATCH(&exc) {
		return QMI_IDL_LIB_GET_ERROR(&exc);
	}

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT int32_t qmi_idl_encode_resp_tlv(uint16_t result, uint16_t error,
					       void *p_dst, uint32_t dst_len)
{
	uint8_t *p_dst2 = p_dst;

	if (!p_dst2 || dst_len < QMI_IDL_RESP_TLV_LEN) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	memset(p_dst2, 0, QMI_IDL_RESP_TLV_LEN);

	memset(p_dst2, 2, 1);
	++p_dst2;


	memset(p_dst2, QMI_IDL_RESP_TLV_LEN - 3, 1);
	p_dst2 += 2;

	memcpy(p_dst2, &result, 2);
	p_dst2 += 2;
	memcpy(p_dst2, &error, 2);

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT uint32_t qmi_idl_get_std_resp_tlv_len(void)
{
	return QMI_IDL_RESP_TLV_LEN;
}




QMI_IDL_EXPORT int32_t qmi_idl_get_idl_version(
	const qmi_idl_service_object_type p_service, uint32_t *idl_version)
{
	int32_t ret = QMI_IDL_LIB_NO_ERR;

	if (p_service == NULL || idl_version == NULL) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	switch (p_service->library_version) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		*idl_version = p_service->idl_version;
		break;
	default:
		ret = QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION;
		break;
	}

	return ret;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_idl_minor_version(
	const qmi_idl_service_object_type p_service, uint32_t *idl_version)
{
	int32_t ret = QMI_IDL_LIB_NO_ERR;

	if (p_service == NULL || idl_version == NULL) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	switch (p_service->library_version) {
	case 1:
	case 2:
	case 3:
	case 4:
		ret = QMI_IDL_LIB_INCOMPATIBLE_SERVICE_VERSION;
		break;
	case 5:
	case 6:
		*idl_version = p_service->idl_minor_version;
		break;
	default:
		ret = QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION;
		break;
	}

	return ret;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_service_id(
	const qmi_idl_service_object_type p_service, uint32_t *service_id)
{
	int32_t ret = QMI_IDL_LIB_NO_ERR;

	if (p_service == NULL || service_id == NULL) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	switch (p_service->library_version) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		*service_id = p_service->service_id;
		break;
	default:
		ret = QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION;
		break;
	}

	return ret;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_max_service_len(
	const qmi_idl_service_object_type p_service, uint32_t *service_len)
{
	int32_t ret = QMI_IDL_LIB_NO_ERR;

	if (p_service == NULL || service_len == NULL) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	switch (p_service->library_version) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		*service_len = p_service->max_msg_len;
		break;
	default:
		ret = QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION;
		break;
	}

	return ret;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_max_message_len(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type message_type, uint16_t message_id,
	uint32_t *message_len)
{
	const qmi_idl_service_message_table_entry *p_srvmsg = NULL;
	qmi_idl_service_object_type inheritance_obj;
	uint32_t i = 0;
	qmi_idl_lib_exception_type exc;

	QMI_IDL_LIB_TRY(&exc) {
		if (p_service == NULL ||
		    message_type >= QMI_IDL_NUM_MSG_TYPES ||
		    message_len == NULL) {
			QMI_IDL_HANDLE_ERROR(&exc, QMI_IDL_LIB_PARAMETER_ERROR,
					     0, 0, 0);
		}
		inheritance_obj = p_service;
		switch (p_service->library_version) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			while (inheritance_obj) {
				p_srvmsg =
					inheritance_obj->msgid_to_msg[message_type];
				i = inheritance_obj->n_msgs[message_type];
				while (i != 0 &&
				       p_srvmsg->qmi_message_id != message_id) {
					++p_srvmsg;
					--i;
				}
				if (i != 0) {
					break;
				}
				inheritance_obj =
					qmi_idl_inherited_service(
						inheritance_obj);
			}
			if (0 == i) {
				QMI_IDL_HANDLE_ERROR(
					&exc, QMI_IDL_LIB_MESSAGE_ID_NOT_FOUND,
					message_id, 0, 0);
			}

			*message_len = (uint32_t)p_srvmsg->max_msg_len;
			break;

		default:
			QMI_IDL_HANDLE_ERROR(
				&exc, QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION,
				p_service->library_version, 0, 0);
			break;
		}
	}
	QMI_IDL_LIB_CATCH(&exc) {
		return QMI_IDL_LIB_GET_ERROR(&exc);
	}

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_message_c_struct_len(
	const qmi_idl_service_object_type p_service,
	qmi_idl_type_of_message_type message_type, uint16_t message_id,
	uint32_t *c_struct_len)
{
	const qmi_idl_message_table_entry *p_srvmsg;
	qmi_idl_lib_exception_type exc;

	QMI_IDL_LIB_TRY(&exc) {
		if (p_service == NULL ||
		    message_type >= QMI_IDL_NUM_MSG_TYPES ||
		    c_struct_len == NULL) {
			QMI_IDL_HANDLE_ERROR(&exc, QMI_IDL_LIB_PARAMETER_ERROR,
					     0, 0, 0);
		}

		switch (p_service->library_version) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			p_srvmsg = qmi_idl_find_msg(p_service, message_type,
						    message_id, &exc, NULL);
			*c_struct_len = (uint32_t)p_srvmsg->c_struct_sz;
			break;
		default:
			QMI_IDL_HANDLE_ERROR(
				&exc, QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION,
				p_service->library_version, 0, 0);
			break;
		}
	}
	QMI_IDL_LIB_CATCH(&exc) {
		return QMI_IDL_LIB_GET_ERROR(&exc);
	}

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT int32_t qmi_idl_get_max_c_struct_len(
	const qmi_idl_service_object_type p_service, uint32_t *c_struct_len)
{
	const qmi_idl_message_table_entry *p_srvmsg = NULL;
	qmi_idl_service_object_type inheritance_obj;
	int max_msgs, i;
	qmi_idl_lib_exception_type exc;

	QMI_IDL_LIB_TRY(&exc) {
		if (p_service == NULL || c_struct_len == NULL) {
			QMI_IDL_HANDLE_ERROR(&exc, QMI_IDL_LIB_PARAMETER_ERROR,
					     0, 0, 0);
		}
		*c_struct_len = 0;
		inheritance_obj = p_service;
		switch (p_service->library_version) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			while (inheritance_obj) {
				max_msgs = inheritance_obj->n_msgs[QMI_IDL_REQUEST] +
					   inheritance_obj->n_msgs[QMI_IDL_RESPONSE] +
					   inheritance_obj->n_msgs[QMI_IDL_INDICATION];
				for (i = 0; i < max_msgs; ++i) {
					p_srvmsg = &(inheritance_obj->p_type_table->
						     p_messages[i]);
					if (*c_struct_len <
					    (uint32_t)p_srvmsg->c_struct_sz) {
						*c_struct_len =
							(uint32_t)p_srvmsg->c_struct_sz;
					}
				}
				inheritance_obj =
					qmi_idl_inherited_service(
						inheritance_obj);
			}
			break;

		default:
			QMI_IDL_HANDLE_ERROR(
				&exc, QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION,
				p_service->library_version, 0, 0);
			break;
		}
	}
	QMI_IDL_LIB_CATCH(&exc) {
		return QMI_IDL_LIB_GET_ERROR(&exc);
	}

	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT int32_t qmi_idl_inherit_service_object(
	qmi_idl_service_object_type child_service,
	qmi_idl_service_object_type parent_service)
{
	if (!child_service || !parent_service) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	if (child_service->library_version < 5 ||
	    parent_service->library_version < 5) {
		return QMI_IDL_LIB_INCOMPATIBLE_SERVICE_VERSION;
	}
	if (parent_service->parent_service_obj) {
		return QMI_IDL_LIB_PARAMETER_ERROR;
	}
	if (child_service->library_version > QMI_IDL_LIB_ENCDEC_VERSION ||
	    parent_service->library_version > QMI_IDL_LIB_ENCDEC_VERSION) {
		return QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION;
	}

	child_service->parent_service_obj = parent_service;
	return QMI_IDL_LIB_NO_ERR;
}


QMI_IDL_EXPORT QMI_IDL_NOINLINE qmi_idl_service_object_type
qmi_idl_get_inherited_service_object(qmi_idl_service_object_type service)
{
	return qmi_idl_inherited_service(service);
}
