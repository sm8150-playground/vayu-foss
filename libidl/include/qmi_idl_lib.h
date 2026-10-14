
#ifndef QMI_IDL_LIB_H
#define QMI_IDL_LIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QMI_IDL_LIB_NO_ERR                           0
#define QMI_IDL_LIB_EXTENDED_ERR                    -40
#define QMI_IDL_LIB_BUFFER_TOO_SMALL                -41
#define QMI_IDL_LIB_ARRAY_TOO_BIG                   -42
#define QMI_IDL_LIB_MESSAGE_ID_NOT_FOUND            -43
#define QMI_IDL_LIB_TLV_DUPLICATED                  -44
#define QMI_IDL_LIB_LENGTH_INCONSISTENCY            -45
#define QMI_IDL_LIB_MISSING_TLV                     -46
#define QMI_IDL_LIB_PARAMETER_ERROR                 -47
#define QMI_IDL_LIB_UNRECOGNIZED_SERVICE_VERSION    -48
#define QMI_IDL_LIB_UNKNOWN_MANDATORY_TLV           -49
#define QMI_IDL_LIB_INCOMPATIBLE_SERVICE_VERSION    -50
#define QMI_IDL_LIB_RANGE_FAILURE                   -51


#define QMI_IDL_LIB_ENCDEC_VERSION 5

typedef enum {
	QMI_IDL_REQUEST = 0,
	QMI_IDL_RESPONSE,
	QMI_IDL_INDICATION,
	QMI_IDL_NUM_MSG_TYPES
} qmi_idl_type_of_message_type;

typedef struct qmi_idl_service_object *qmi_idl_service_object_type;

typedef struct qmi_idl_type_table_object qmi_idl_type_table_object;

struct qmi_idl_lib_exception_type;

int32_t qmi_idl_get_service_id(const qmi_idl_service_object_type service,
			       uint32_t *service_id);
int32_t qmi_idl_get_idl_version(const qmi_idl_service_object_type service,
				uint32_t *idl_version);
int32_t qmi_idl_get_idl_minor_version(const qmi_idl_service_object_type service,
				      uint32_t *idl_version);
int32_t qmi_idl_get_max_service_len(const qmi_idl_service_object_type service,
				    uint32_t *service_len);
int32_t qmi_idl_get_max_message_len(const qmi_idl_service_object_type service,
				    qmi_idl_type_of_message_type message_type,
				    uint16_t message_id, uint32_t *message_len);
int32_t qmi_idl_get_message_c_struct_len(const qmi_idl_service_object_type service,
					 qmi_idl_type_of_message_type message_type,
					 uint16_t message_id, uint32_t *c_struct_len);
int32_t qmi_idl_get_max_c_struct_len(const qmi_idl_service_object_type service,
				     uint32_t *c_struct_len);
int32_t qmi_idl_inherit_service_object(qmi_idl_service_object_type child_service,
				       qmi_idl_service_object_type parent_service);
qmi_idl_service_object_type qmi_idl_get_inherited_service_object(
	qmi_idl_service_object_type service);

int32_t qmi_idl_message_decode(const qmi_idl_service_object_type p_service,
			       qmi_idl_type_of_message_type req_resp_ind,
			       uint16_t message_id, const void *p_src,
			       uint32_t src_len, void *p_dst, uint32_t dst_len);
int32_t qmi_idl_message_encode(const qmi_idl_service_object_type p_service,
			       qmi_idl_type_of_message_type req_resp_ind,
			       uint16_t message_id, const void *p_src,
			       uint32_t src_len, void *p_dst, uint32_t dst_len,
			       uint32_t *dst_encoded_len);
int32_t qmi_idl_encode_resp_tlv(uint16_t result, uint16_t error, void *p_dst,
				uint32_t dst_len);
uint32_t qmi_idl_get_std_resp_tlv_len(void);

#ifdef __cplusplus
}
#endif
#endif
