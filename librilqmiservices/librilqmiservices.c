

#include <stdint.h>
#include <stddef.h>


#define QMI_IDL_EXPORT __attribute__((visibility("default")))


#define QMI_IDL_NUM_MSG_TYPES 3
#define QMI_IDL_TYPE16(table, type) (((table) << 12) | (type))

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

typedef struct qmi_idl_service_object *qmi_idl_service_object_type;

_Static_assert(sizeof(qmi_idl_service_message_table_entry) == 6, "QMI IDL ABI");
_Static_assert(sizeof(qmi_idl_type_table_entry) == 16, "QMI IDL ABI");
_Static_assert(sizeof(qmi_idl_message_table_entry) == 16, "QMI IDL ABI");
_Static_assert(sizeof(struct qmi_idl_type_table_object) == 40, "QMI IDL ABI");
_Static_assert(sizeof(struct qmi_idl_service_object) == 72, "QMI IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, msgid_to_msg) == 24, "QMI IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, p_type_table) == 48, "QMI IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, idl_minor_version) == 56, "QMI IDL ABI");
_Static_assert(offsetof(struct qmi_idl_service_object, parent_service_obj) == 64, "QMI IDL ABI");


extern const struct qmi_idl_type_table_object common_qmi_idl_type_table_object_v01;




static const qmi_idl_service_message_table_entry
embms_service_command_messages_v01[] = {
	{ 0x0000, QMI_IDL_TYPE16(0, 0), 7 },
	{ 0x0001, QMI_IDL_TYPE16(0, 2), 11 },
	{ 0x0002, QMI_IDL_TYPE16(0, 4), 420 },
	{ 0x0003, QMI_IDL_TYPE16(0, 6), 21 },
	{ 0x0004, QMI_IDL_TYPE16(0, 10), 11 },
	{ 0x0005, QMI_IDL_TYPE16(0, 12), 11 },
	{ 0x0006, QMI_IDL_TYPE16(0, 14), 0 },
	{ 0x0007, QMI_IDL_TYPE16(0, 16), 0 },
	{ 0x0008, QMI_IDL_TYPE16(0, 18), 7 },
	{ 0x0009, QMI_IDL_TYPE16(0, 20), 7 },
	{ 0x000a, QMI_IDL_TYPE16(0, 22), 0 },
	{ 0x0011, QMI_IDL_TYPE16(0, 8), 430 },
	{ 0x0015, QMI_IDL_TYPE16(0, 33), 1036 },
	{ 0x0016, QMI_IDL_TYPE16(0, 35), 2067 },
	{ 0x0017, QMI_IDL_TYPE16(0, 37), 26 },
	{ 0x0018, QMI_IDL_TYPE16(0, 39), 0 },
	{ 0x001a, QMI_IDL_TYPE16(0, 42), 7 },
	{ 0x001b, QMI_IDL_TYPE16(0, 44), 7 },
	{ 0x001d, QMI_IDL_TYPE16(0, 47), 537 },
	{ 0x001f, QMI_IDL_TYPE16(0, 50), 7 },
	{ 0x0021, QMI_IDL_TYPE16(0, 53), 7 },
	{ 0x0023, QMI_IDL_TYPE16(0, 56), 463 },
};

static const qmi_idl_service_message_table_entry
embms_service_response_messages_v01[] = {
	{ 0x0000, QMI_IDL_TYPE16(0, 1), 92 },
	{ 0x0001, QMI_IDL_TYPE16(0, 3), 18 },
	{ 0x0002, QMI_IDL_TYPE16(0, 5), 28 },
	{ 0x0003, QMI_IDL_TYPE16(0, 7), 28 },
	{ 0x0004, QMI_IDL_TYPE16(0, 11), 466 },
	{ 0x0005, QMI_IDL_TYPE16(0, 13), 466 },
	{ 0x0006, QMI_IDL_TYPE16(0, 15), 0 },
	{ 0x0007, QMI_IDL_TYPE16(0, 17), 0 },
	{ 0x0008, QMI_IDL_TYPE16(0, 19), 21 },
	{ 0x0009, QMI_IDL_TYPE16(0, 21), 1955 },
	{ 0x000a, QMI_IDL_TYPE16(0, 23), 14 },
	{ 0x0011, QMI_IDL_TYPE16(0, 9), 41 },
	{ 0x0015, QMI_IDL_TYPE16(0, 34), 1036 },
	{ 0x0016, QMI_IDL_TYPE16(0, 36), 7 },
	{ 0x0017, QMI_IDL_TYPE16(0, 38), 0 },
	{ 0x0018, QMI_IDL_TYPE16(0, 40), 4 },
	{ 0x001a, QMI_IDL_TYPE16(0, 43), 37 },
	{ 0x001b, QMI_IDL_TYPE16(0, 45), 21 },
	{ 0x001d, QMI_IDL_TYPE16(0, 48), 14 },
	{ 0x001f, QMI_IDL_TYPE16(0, 51), 139 },
	{ 0x0021, QMI_IDL_TYPE16(0, 54), 11 },
	{ 0x0023, QMI_IDL_TYPE16(0, 57), 7 },
};

static const qmi_idl_service_message_table_entry
embms_service_indication_messages_v01[] = {
	{ 0x000b, QMI_IDL_TYPE16(0, 24), 74 },
	{ 0x000c, QMI_IDL_TYPE16(0, 25), 459 },
	{ 0x000d, QMI_IDL_TYPE16(0, 26), 14 },
	{ 0x000e, QMI_IDL_TYPE16(0, 27), 7 },
	{ 0x000f, QMI_IDL_TYPE16(0, 28), 459 },
	{ 0x0010, QMI_IDL_TYPE16(0, 29), 466 },
	{ 0x0012, QMI_IDL_TYPE16(0, 30), 26 },
	{ 0x0013, QMI_IDL_TYPE16(0, 31), 14 },
	{ 0x0014, QMI_IDL_TYPE16(0, 32), 2616 },
	{ 0x0019, QMI_IDL_TYPE16(0, 41), 4 },
	{ 0x001c, QMI_IDL_TYPE16(0, 46), 14 },
	{ 0x001e, QMI_IDL_TYPE16(0, 49), 31 },
	{ 0x0020, QMI_IDL_TYPE16(0, 52), 4 },
	{ 0x0022, QMI_IDL_TYPE16(0, 55), 7 },
};

static const uint8_t embms_type_0_data_v01[] = {
	0x50, 0x04, 0x06, 0x04, 0x20,
};

static const uint8_t embms_type_1_data_v01[] = {
	0x02, 0x00, 0x02, 0x04, 0x20,
};

static const uint8_t embms_type_2_data_v01[] = {
	0x56, 0x00, 0x03, 0x56, 0x04, 0x03, 0x20,
};

static const uint8_t embms_msg_0_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_1_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x41, 0x10, 0x00, 0x09, 0x41, 0x11,
	0x56, 0x0b, 0x40, 0xc4, 0x12, 0x02, 0x50,
};

static const uint8_t embms_msg_2_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x00, 0x04,
};

static const uint8_t embms_msg_3_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0xc1, 0x10, 0x00, 0x09,
};

static const uint8_t embms_msg_4_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x03, 0x07, 0x08, 0x00, 0x00, 0x04,
	0x02, 0x14, 0x05, 0x52, 0x1c, 0x20, 0x04, 0xc8, 0x10, 0x52, 0xa4, 0x40,
	0x04,
};

static const uint8_t embms_msg_5_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x41, 0x10, 0x00, 0x09, 0xc2, 0x11,
	0x07, 0x0c, 0x00, 0x00,
};

static const uint8_t embms_msg_6_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x83, 0x07, 0x08, 0x00, 0x00,
};

static const uint8_t embms_msg_7_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x41, 0x10, 0x00, 0x09, 0xc2, 0x11,
	0x07, 0x0c, 0x00, 0x00,
};

static const uint8_t embms_msg_8_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x03, 0x07, 0x08, 0x00, 0x00, 0x04,
	0x07, 0x14, 0x00, 0x00, 0x05, 0x02, 0x20, 0x06, 0x52, 0x28, 0x20, 0x04,
	0xc8, 0x10, 0x52, 0xb0, 0x40, 0x04,
};

static const uint8_t embms_msg_9_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x01, 0x04, 0x03, 0x01, 0x06, 0x41, 0x10, 0x00,
	0x09, 0x42, 0x11, 0x07, 0x0c, 0x00, 0x00, 0xc4, 0x12, 0x07, 0x1c, 0x00,
	0x00,
};

static const uint8_t embms_msg_10_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x00, 0x04,
};

static const uint8_t embms_msg_11_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0xc8, 0x10, 0x57, 0x10, 0x40, 0x04,
	0x00, 0x00,
};

static const uint8_t embms_msg_12_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x00, 0x04,
};

static const uint8_t embms_msg_13_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0xc8, 0x10, 0x57, 0x10, 0x40, 0x04,
	0x00, 0x00,
};

static const uint8_t embms_msg_18_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_19_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0xc4, 0x10, 0x02, 0x0c,
};

static const uint8_t embms_msg_20_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_21_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x48, 0x10, 0x52, 0x10, 0x08, 0x04,
	0x48, 0x11, 0x52, 0x38, 0x08, 0x04, 0x48, 0x12, 0x52, 0x60, 0x08, 0x04,
	0x48, 0x13, 0x52, 0x88, 0x08, 0x04, 0xc8, 0x14, 0x77, 0xb0, 0x00, 0x01,
	0x04, 0x00, 0x00,
};

static const uint8_t embms_msg_23_data_v01[] = {
	0x01, 0x02, 0x00, 0xc4, 0x10, 0x02, 0x08,
};

static const uint8_t embms_msg_24_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x56, 0x04, 0x40,
};

static const uint8_t embms_msg_25_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x57, 0x08, 0x40, 0x04, 0x00, 0x00,
};

static const uint8_t embms_msg_26_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x02, 0x04,
};

static const uint8_t embms_msg_27_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_28_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x57, 0x08, 0x40, 0x04, 0x00, 0x00,
};

static const uint8_t embms_msg_29_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x83, 0x57, 0x0c, 0x40, 0x04, 0x00,
	0x00,
};

static const uint8_t embms_msg_30_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x56, 0x04, 0x03, 0x03, 0x56, 0x08, 0x03, 0x84,
	0x02, 0x0c,
};

static const uint8_t embms_msg_31_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x02, 0x04,
};

static const uint8_t embms_msg_32_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x52, 0x08, 0x40, 0x04, 0x03, 0xd2, 0x0c, 0x01,
	0x09, 0x04, 0x84, 0xf2, 0x34, 0x01, 0x40, 0x02, 0x04,
};

static const uint8_t embms_msg_33_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x72, 0x08, 0x00, 0x01, 0x04,
};

static const uint8_t embms_msg_34_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x72, 0x08, 0x00, 0x01, 0x04,
};

static const uint8_t embms_msg_35_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x83, 0x70, 0x0c, 0x00, 0x08, 0x04,
};

static const uint8_t embms_msg_36_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_37_data_v01[] = {
	0x01, 0x00, 0x00, 0x47, 0x10, 0x03, 0x08, 0xc8, 0x11, 0x03, 0x18,
};

static const uint8_t embms_msg_40_data_v01[] = {
	0x81, 0x00, 0x00,
};

static const uint8_t embms_msg_41_data_v01[] = {
	0x81, 0x00, 0x00,
};

static const uint8_t embms_msg_42_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_43_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0x03, 0x03, 0x08, 0x41, 0x10, 0x00,
	0x11, 0x41, 0x11, 0x00, 0x13, 0xc1, 0x12, 0x00, 0x15,
};

static const uint8_t embms_msg_44_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_45_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x02, 0x04, 0xc4, 0x10, 0x02, 0x0c,
};

static const uint8_t embms_msg_46_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x02, 0x04,
};

static const uint8_t embms_msg_47_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x03, 0x07, 0x08, 0x00, 0x00, 0xc8,
	0x10, 0x57, 0x1c, 0x40, 0x04, 0x01, 0x00,
};

static const uint8_t embms_msg_48_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x02, 0x04,
};

static const uint8_t embms_msg_49_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x07, 0x04, 0x00, 0x00, 0x44, 0x10, 0x02, 0x14,
	0xc4, 0x11, 0x02, 0x1c,
};

static const uint8_t embms_msg_50_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_51_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x57, 0x08, 0x10, 0x04, 0x02, 0x00,
};

static const uint8_t embms_msg_52_data_v01[] = {
	0x81, 0x00, 0x00,
};

static const uint8_t embms_msg_53_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_54_data_v01[] = {
	0x01, 0x02, 0x00, 0x82, 0x00, 0x04,
};

static const uint8_t embms_msg_55_data_v01[] = {
	0x81, 0x02, 0x00,
};

static const uint8_t embms_msg_56_data_v01[] = {
	0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x83, 0x57, 0x0c, 0x40, 0x04, 0x00,
	0x00,
};

static const uint8_t embms_msg_57_data_v01[] = {
	0x81, 0x02, 0x00,
};




static const qmi_idl_service_message_table_entry
Qtuner_service_command_messages_v01[] = {
	{ 0x0020, QMI_IDL_TYPE16(0, 0), 132 },
	{ 0x0021, QMI_IDL_TYPE16(0, 2), 0 },
	{ 0x0022, QMI_IDL_TYPE16(0, 4), 0 },
};

static const qmi_idl_service_message_table_entry
Qtuner_service_response_messages_v01[] = {
	{ 0x0020, QMI_IDL_TYPE16(0, 1), 0 },
	{ 0x0021, QMI_IDL_TYPE16(0, 3), 132 },
	{ 0x0022, QMI_IDL_TYPE16(0, 5), 139 },
};

static const uint8_t Qtuner_msg_0_data_v01[] = {
	0x81, 0x52, 0x04, 0x20, 0x04,
};

static const uint8_t Qtuner_msg_3_data_v01[] = {
	0xc8, 0x10, 0x52, 0x08, 0x20, 0x04,
};

static const uint8_t Qtuner_msg_5_data_v01[] = {
	0x44, 0x10, 0x02, 0x04, 0xc8, 0x11, 0x51, 0x10, 0x40, 0x04, 0x00,
};




static const qmi_idl_service_message_table_entry
ims_qmi_service_command_messages_v01[] = {
	{ 0x0002, QMI_IDL_TYPE16(0, 0), 88 },
	{ 0x0003, QMI_IDL_TYPE16(0, 2), 4 },
	{ 0x0004, QMI_IDL_TYPE16(0, 4), 8 },
	{ 0x0005, QMI_IDL_TYPE16(0, 6), 4 },
};

static const qmi_idl_service_message_table_entry
ims_qmi_service_response_messages_v01[] = {
	{ 0x0002, QMI_IDL_TYPE16(0, 1), 11 },
	{ 0x0003, QMI_IDL_TYPE16(0, 3), 11 },
	{ 0x0004, QMI_IDL_TYPE16(0, 5), 11 },
	{ 0x0005, QMI_IDL_TYPE16(0, 7), 370 },
};

static const qmi_idl_service_message_table_entry
ims_qmi_service_indication_messages_v01[] = {
	{ 0x0001, QMI_IDL_TYPE16(0, 8), 369 },
};

static const uint8_t ims_qmi_type_0_data_v01[] = {
	0x00, 0x00, 0x04, 0x04, 0x04, 0x08, 0x04, 0x0c, 0x20,
};

static const uint8_t ims_qmi_type_1_data_v01[] = {
	0x50, 0x04, 0x51, 0x04, 0x20,
};

static const uint8_t ims_qmi_type_2_data_v01[] = {
	0x50, 0x04, 0xb6, 0x04, 0x20,
};

static const uint8_t ims_qmi_type_3_data_v01[] = {
	0x50, 0x04, 0x51, 0x04, 0x20,
};

static const uint8_t ims_qmi_type_4_data_v01[] = {
	0x00, 0x00, 0x05, 0x04, 0x20,
};

static const uint8_t ims_qmi_msg_0_data_v01[] = {
	0x01, 0x56, 0x00, 0x51, 0xc2, 0x10, 0x04, 0x54,
};

static const uint8_t ims_qmi_msg_1_data_v01[] = {
	0x02, 0x07, 0x00, 0x00, 0x01, 0xc1, 0x10, 0x00, 0x09,
};

static const uint8_t ims_qmi_msg_2_data_v01[] = {
	0x81, 0x00, 0x00,
};

static const uint8_t ims_qmi_msg_3_data_v01[] = {
	0x02, 0x07, 0x00, 0x00, 0x01, 0xc1, 0x10, 0x00, 0x09,
};

static const uint8_t ims_qmi_msg_4_data_v01[] = {
	0x01, 0x00, 0x00, 0x82, 0x04, 0x04,
};

static const uint8_t ims_qmi_msg_5_data_v01[] = {
	0x02, 0x07, 0x00, 0x00, 0x01, 0xc1, 0x10, 0x00, 0x09,
};

static const uint8_t ims_qmi_msg_6_data_v01[] = {
	0x81, 0x00, 0x00,
};

static const uint8_t ims_qmi_msg_7_data_v01[] = {
	0x02, 0x07, 0x00, 0x00, 0x01, 0x44, 0x10, 0x07, 0x0c, 0x00, 0x00, 0x44,
	0x11, 0x07, 0x20, 0x01, 0x00, 0x44, 0x12, 0x07, 0x7c, 0x02, 0x00, 0xc4,
	0x13, 0x87, 0x3c, 0x01, 0x03, 0x00,
};

static const uint8_t ims_qmi_msg_8_data_v01[] = {
	0x01, 0x07, 0x00, 0x00, 0x00, 0x44, 0x10, 0x07, 0x14, 0x01, 0x00, 0x44,
	0x11, 0x07, 0x70, 0x02, 0x00, 0x44, 0x12, 0x87, 0x30, 0x01, 0x03, 0x00,
	0xc4, 0x13, 0x87, 0x8c, 0x01, 0x04, 0x00,
};

static const qmi_idl_type_table_entry embms_type_table_v01[] = {
	{ 12, embms_type_0_data_v01 },
	{ 8, embms_type_1_data_v01 },
	{ 8, embms_type_2_data_v01 },
};

static const qmi_idl_message_table_entry embms_message_table_v01[] = {
	{ 4, embms_msg_0_data_v01 },
	{ 84, embms_msg_1_data_v01 },
	{ 8, embms_msg_2_data_v01 },
	{ 12, embms_msg_3_data_v01 },
	{ 420, embms_msg_4_data_v01 },
	{ 24, embms_msg_5_data_v01 },
	{ 20, embms_msg_6_data_v01 },
	{ 24, embms_msg_7_data_v01 },
	{ 432, embms_msg_8_data_v01 },
	{ 40, embms_msg_9_data_v01 },
	{ 8, embms_msg_10_data_v01 },
	{ 784, embms_msg_11_data_v01 },
	{ 8, embms_msg_12_data_v01 },
	{ 784, embms_msg_13_data_v01 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 0 },
	{ 4, embms_msg_18_data_v01 },
	{ 16, embms_msg_19_data_v01 },
	{ 4, embms_msg_20_data_v01 },
	{ 3248, embms_msg_21_data_v01 },
	{ 1, 0 },
	{ 12, embms_msg_23_data_v01 },
	{ 72, embms_msg_24_data_v01 },
	{ 776, embms_msg_25_data_v01 },
	{ 8, embms_msg_26_data_v01 },
	{ 4, embms_msg_27_data_v01 },
	{ 776, embms_msg_28_data_v01 },
	{ 780, embms_msg_29_data_v01 },
	{ 16, embms_msg_30_data_v01 },
	{ 8, embms_msg_31_data_v01 },
	{ 2612, embms_msg_32_data_v01 },
	{ 1032, embms_msg_33_data_v01 },
	{ 1032, embms_msg_34_data_v01 },
	{ 2060, embms_msg_35_data_v01 },
	{ 4, embms_msg_36_data_v01 },
	{ 32, embms_msg_37_data_v01 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, embms_msg_40_data_v01 },
	{ 1, embms_msg_41_data_v01 },
	{ 4, embms_msg_42_data_v01 },
	{ 24, embms_msg_43_data_v01 },
	{ 4, embms_msg_44_data_v01 },
	{ 16, embms_msg_45_data_v01 },
	{ 8, embms_msg_46_data_v01 },
	{ 540, embms_msg_47_data_v01 },
	{ 8, embms_msg_48_data_v01 },
	{ 32, embms_msg_49_data_v01 },
	{ 4, embms_msg_50_data_v01 },
	{ 136, embms_msg_51_data_v01 },
	{ 1, embms_msg_52_data_v01 },
	{ 4, embms_msg_53_data_v01 },
	{ 8, embms_msg_54_data_v01 },
	{ 4, embms_msg_55_data_v01 },
	{ 780, embms_msg_56_data_v01 },
	{ 4, embms_msg_57_data_v01 },
};

static const struct qmi_idl_type_table_object
embms_qmi_idl_type_table_object_v01;

static const struct qmi_idl_type_table_object
*embms_qmi_idl_type_table_object_referenced_tables_v01[] = {
	&embms_qmi_idl_type_table_object_v01,
	&common_qmi_idl_type_table_object_v01,
};

static const struct qmi_idl_type_table_object embms_qmi_idl_type_table_object_v01 = {
	3,
	58,
	1,
	embms_type_table_v01,
	embms_message_table_v01,
	embms_qmi_idl_type_table_object_referenced_tables_v01,
	NULL
};

static const qmi_idl_message_table_entry Qtuner_message_table_v01[] = {
	{ 132, Qtuner_msg_0_data_v01 },
	{ 0, 0 },
	{ 0, 0 },
	{ 136, Qtuner_msg_3_data_v01 },
	{ 0, 0 },
	{ 144, Qtuner_msg_5_data_v01 },
};

static const struct qmi_idl_type_table_object
Qtuner_qmi_idl_type_table_object_v01;

static const struct qmi_idl_type_table_object
*Qtuner_qmi_idl_type_table_object_referenced_tables_v01[] = {
	&Qtuner_qmi_idl_type_table_object_v01,
	&common_qmi_idl_type_table_object_v01,
};

static const struct qmi_idl_type_table_object Qtuner_qmi_idl_type_table_object_v01 = {
	0,
	6,
	1,
	NULL,
	Qtuner_message_table_v01,
	Qtuner_qmi_idl_type_table_object_referenced_tables_v01,
	NULL
};

static const qmi_idl_type_table_entry ims_qmi_type_table_v01[] = {
	{ 16, ims_qmi_type_0_data_v01 },
	{ 88, ims_qmi_type_1_data_v01 },
	{ 188, ims_qmi_type_2_data_v01 },
	{ 88, ims_qmi_type_3_data_v01 },
	{ 8, ims_qmi_type_4_data_v01 },
};

static const qmi_idl_message_table_entry ims_qmi_message_table_v01[] = {
	{ 88, ims_qmi_msg_0_data_v01 },
	{ 12, ims_qmi_msg_1_data_v01 },
	{ 1, ims_qmi_msg_2_data_v01 },
	{ 12, ims_qmi_msg_3_data_v01 },
	{ 8, ims_qmi_msg_4_data_v01 },
	{ 12, ims_qmi_msg_5_data_v01 },
	{ 1, ims_qmi_msg_6_data_v01 },
	{ 404, ims_qmi_msg_7_data_v01 },
	{ 404, ims_qmi_msg_8_data_v01 },
};

static const struct qmi_idl_type_table_object
ims_qmi_qmi_idl_type_table_object_v01;

static const struct qmi_idl_type_table_object
*ims_qmi_qmi_idl_type_table_object_referenced_tables_v01[] = {
	&ims_qmi_qmi_idl_type_table_object_v01,
	&common_qmi_idl_type_table_object_v01,
};

static const struct qmi_idl_type_table_object ims_qmi_qmi_idl_type_table_object_v01 = {
	5,
	9,
	1,
	ims_qmi_type_table_v01,
	ims_qmi_message_table_v01,
	ims_qmi_qmi_idl_type_table_object_referenced_tables_v01,
	NULL
};





struct qmi_idl_service_object embms_qmi_idl_service_object_v01 QMI_IDL_EXPORT = {
	0x6,
	0x1,
	0x2,
	2616,
	{ 22, 22, 14 },
	{ embms_service_command_messages_v01, embms_service_response_messages_v01, embms_service_indication_messages_v01 },
	&embms_qmi_idl_type_table_object_v01,
	0x2,
	NULL
};


struct qmi_idl_service_object Qtuner_qmi_idl_service_object_v01 QMI_IDL_EXPORT = {
	0x5,
	0x1,
	0x4,
	139,
	{ 3, 3, 0 },
	{ Qtuner_service_command_messages_v01, Qtuner_service_response_messages_v01, NULL },
	&Qtuner_qmi_idl_type_table_object_v01,
	0x0,
	NULL
};


const struct qmi_idl_service_object ims_qmi_qmi_idl_service_object_v01 QMI_IDL_EXPORT = {
	0x2,
	0x1,
	0x13,
	370,
	{ 4, 4, 1 },
	{ ims_qmi_service_command_messages_v01, ims_qmi_service_response_messages_v01, ims_qmi_service_indication_messages_v01 },
	&ims_qmi_qmi_idl_type_table_object_v01,
	0x0,
	NULL
};





qmi_idl_service_object_type QMI_IDL_EXPORT embms_get_service_object_internal_v01
( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version )
{
	if ( 0x1 != idl_maj_version
	     || 0x2 != idl_min_version
	     || 0x6 != library_version) {
		return NULL;
	}
	return (qmi_idl_service_object_type)&embms_qmi_idl_service_object_v01;
}


qmi_idl_service_object_type QMI_IDL_EXPORT Qtuner_get_service_object_internal_v01
( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version )
{
	if ( 0x1 != idl_maj_version
	     || 0x0 != idl_min_version
	     || 0x5 != library_version) {
		return NULL;
	}
	return (qmi_idl_service_object_type)&Qtuner_qmi_idl_service_object_v01;
}


qmi_idl_service_object_type QMI_IDL_EXPORT ims_qmi_get_service_object_internal_v01
( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version )
{
	if ( 0x1 != idl_maj_version
	     || 0x1 != idl_min_version
	     || 0x2 != library_version) {
		return NULL;
	}
	return (qmi_idl_service_object_type)&ims_qmi_qmi_idl_service_object_v01;
}

